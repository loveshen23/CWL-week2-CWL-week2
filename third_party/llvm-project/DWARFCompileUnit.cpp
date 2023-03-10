
//===-- DWARFCompileUnit.cpp ----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/DebugInfo/DWARF/DWARFCompileUnit.h"
#include "llvm/DebugInfo/DWARF/DWARFDebugAbbrev.h"
#include "llvm/DebugInfo/DWARF/DWARFDie.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

void DWARFCompileUnit::dump(raw_ostream &OS, DIDumpOptions DumpOpts) {
  OS << format("0x%08" PRIx64, getOffset()) << ": Compile Unit:"
     << " length = " << format("0x%08" PRIx64, getLength())
     << " version = " << format("0x%04x", getVersion());
  if (getVersion() >= 5)
    OS << " unit_type = " << dwarf::UnitTypeString(getUnitType());
  if (auto* Abbreviations = getAbbreviations()) { // XXX BINARYEN
    OS << " abbr_offset = "
       << format("0x%04" PRIx64, Abbreviations->getOffset());
  }
  OS << " addr_size = " << format("0x%02x", getAddressByteSize());
  if (getVersion() >= 5 && getUnitType() != dwarf::DW_UT_compile)
    OS << " DWO_id = " << format("0x%016" PRIx64, *getDWOId());
  OS << " (next unit at " << format("0x%08" PRIx64, getNextUnitOffset())
     << ")\n";

  if (DWARFDie CUDie = getUnitDIE(false))
    CUDie.dump(OS, 0, DumpOpts);
  else
    OS << "<compile unit can't be parsed!>\n\n";
}

// VTable anchor.
DWARFCompileUnit::~DWARFCompileUnit() = default;