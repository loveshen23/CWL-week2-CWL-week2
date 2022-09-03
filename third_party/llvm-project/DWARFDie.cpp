//===- DWARFDie.cpp -------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/DebugInfo/DWARF/DWARFDie.h"
#include "llvm/ADT/None.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/BinaryFormat/Dwarf.h"
#include "llvm/DebugInfo/DWARF/DWARFAbbreviationDeclaration.h"
#include "llvm/DebugInfo/DWARF/DWARFContext.h"
#include "llvm/DebugInfo/DWARF/DWARFDebugRangeList.h"
#include "llvm/DebugInfo/DWARF/DWARFExpression.h"
#include "llvm/DebugInfo/DWARF/DWARFFormValue.h"
#include "llvm/DebugInfo/DWARF/DWARFUnit.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Support/DataExtractor.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/FormatAdapters.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/WithColor.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>
#include <cassert>
#include <cinttypes>
#include <cstdint>
#include <string>
#include <utility>

using namespace llvm;
using namespace dwarf;
using namespace object;

static void dumpApplePropertyAttribute(raw_ostream &OS, uint64_t Val) {
  OS << " (";
  do {
    uint64_t Shift = countTrailingZeros(Val);
    assert(Shift < 64 && "undefined behavior");
    uint64_t Bit = 1ULL << Shift;
    auto PropName = ApplePropertyString(Bit);
    if (!PropName.empty())
      OS << PropName;
    else
      OS << format("DW_APPLE_PROPERTY_0x%" PRIx64, Bit);
    if (!(Val ^= Bit))
      break;
    OS << ", ";
  } while (true);
  OS << ")";
}

static void dumpRanges(const DWARFObject &Obj, raw_ostream &OS,
                       const DWARFAddressRangesVector &Ranges,
                       unsigned AddressSize, unsigned Indent,
                       const DIDumpOptions &DumpOpts) {
  if (!DumpOpts.ShowAddresses)
    return;

  ArrayRef<SectionName> SectionNames;
  if (DumpOpts.Verbose)
    SectionNames = Obj.getSectionNames();

  for (const DWARFAddressRange &R : Ranges) {
    OS << '\n';
    OS.indent(Indent);
    R.dump(OS, AddressSize);

    DWARFFormValue::dumpAddressSection(Obj, OS, DumpOpts, R.SectionIndex);
  }
}

static void dumpLocation(raw_ostream &OS, DWARFFormValue &FormValue,
                         DWARFUnit *U, unsigned Indent,
                         DIDumpOptions DumpOpts) {
  DWARFContext &Ctx = U->getContext();
  const DWARFObject &Obj = Ctx.getDWARFObj();
  const MCRegisterInfo *MRI = Ctx.getRegisterInfo();
  if (FormValue.isFormClass(DWARFFormValue::FC_Block) ||
      FormValue.isFormClass(DWARFFormValue::FC_Exprloc)) {
    ArrayRef<uint8_t> Expr = *FormValue.getAsBlock();
    DataExtractor Data(StringRef((const char *)Expr.data(), Expr.size()),
                       Ctx.isLittleEndian(), 0);
    DWARFExpression(Data, U->getVersion(), U->getAddressByteSize())
        .print(OS, MRI, U);
    return;
  }

  if (FormValue.isFormClass(DWARFFormValue::FC_SectionOffset)) {
    uint64_t Offset = *FormValue.getAsSectionOffset();
    uint64_t BaseAddr = 0;
    if (Optional<object::SectionedAddress> BA = U->getBaseAddress())
      BaseAddr = BA->Address;
    auto LLDumpOpts = DumpOpts;
    LLDumpOpts.Verbose = false;

    if (!U->isDWOUnit() && !U->getLocSection()->Data.empty()) {
      DWARFDebugLoc DebugLoc;
      DWARFDataExtractor Data(Obj, *U->getLocSection(), Ctx.isLittleEndian(),
                              Obj.getAddressSize());

      FormValue.dump(OS, DumpOpts);
      OS << ": ";

      if (Expected<DWARFDebugLoc::LocationList> LL =
              DebugLoc.parseOneLocationList(Data, &Offset)) {
        LL->dump(OS, BaseAddr, Ctx.isLittleEndian(), Obj.getAddressSize(), MRI,
                 U, LLDumpOpts, Indent);
      } else {
        OS << '\n';
        OS.indent(Indent);
        OS << formatv("error extracting location list: {0}",
                      fmt_consume(LL.takeError()));
      }
      return;
    }

    bool UseLocLists = !U->isDWOUnit();
    auto Data =
        UseLocLists
            ? DWARFDataExtractor(Obj, Obj.getLoclistsSection(),
                                 Ctx.isLittleEndian(), Obj.getAddressSize())
            : DWARFDataExtractor(U->getLocSectionData(), Ctx.isLittleEndian(),
                                 Obj.getAddressSize());

    if (!Data.getData().empty()) {
      // Old-style location list were used in DWARF v4 (.debug_loc.dwo section).
      // Modern locations list (.debug_loclists) are used starting from v5.
      // Ideally we should take the version from the .debug_loclists section
      // header, but using CU's version for simplicity.
      DWARFDebugLoclists::dumpLocationList(
          Data, &Offset, UseLocLists ? U->getVersion() : 4, OS, BaseAddr, MRI,
          U, LLDumpOpts, Indent);
    }
    return;
  }

  FormValue.dump(OS, DumpOpts);
}

/// Dump the name encoded in the type tag.
static void dumpTypeTagName(raw_ostream &OS, dwarf::Tag T) {
  StringRef TagStr = TagString(T);
  if (!TagStr.startswith("DW_TAG_") || !TagStr.endswith("_type"))
    return;
  OS << TagStr.substr(7, TagStr.size() - 12) << " ";
}

static void dumpArrayType(raw_ostream &OS, const DWARFDie &D) {
  Optional<uint64_t> Bound;
  for (const DWARFDie &C : D.children())
    if (C.getTag() == DW_TAG_subrange_type) {
      Optional<uint64_t> LB;
      Optional<uint64_t> Count;
      Optional<uint64_t> UB;
      Optional<unsigned> DefaultLB;
      if (Optional<DWARFFormValue> L = C.find(DW_AT_lower_bound))
        LB = L->getAsUnsignedConstant();
      if (Optional<DWARFFormValue> CountV = C.find(DW_AT_count))
        Count = CountV->getAsUnsignedConstant();
      if (Optional<DWARFFormValue> UpperV = C.find(DW_AT_upper_bound))
        UB = UpperV->getAsUnsignedConstant();
      if (Optional<DWARFFormValue> LV =
              D.getDwarfUnit()->getUnitDIE().find(DW_AT_language))
        if (Optional<uint64_t> LC = LV->getAsUnsignedConstant())
          if ((DefaultLB =
                   LanguageLowerBound(static_cast<dwarf::SourceLanguage>(*LC))))
            if (LB && *LB == *DefaultLB)
              LB = None;
      if (!LB && !Count && !UB)
        OS << "[]";
      else if (!LB && (Count || UB) && DefaultLB)
        OS << '[' << (Count ? *Count : *UB - *DefaultLB + 1) << ']';
      else {
        OS << "[[";
        if (LB)
          OS << *LB;
        else
          OS << '?';
        OS << ", ";
        if (Count)
          if (LB)
            OS << *LB + *Count;
          else
            OS << "? + " << *Count;
        else if (UB)
          OS << *UB + 1;
        else
          OS << '?';
        OS << ")]";
      }
    }
}

/// Recursively dump the DIE type name when applicable.
static void dumpTypeName(raw_ostream &OS, const DWARFDie &D) {
  if (!D.isValid())
    return;

  if (const char *Name = D.getName(DINameKind::LinkageName)) {
    OS << Name;
    return;
  }

  // FIXME: We should have pretty printers per language. Currently we print
  // everything as if it was C++ and fall back to the TAG type name.
  const dw