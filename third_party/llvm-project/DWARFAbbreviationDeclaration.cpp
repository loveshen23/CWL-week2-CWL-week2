//===- DWARFAbbreviationDeclaration.cpp -----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/DebugInfo/DWARF/DWARFAbbreviationDeclaration.h"

#include "llvm/ADT/None.h"
#include "llvm/ADT/Optional.h"
#include "llvm/BinaryFormat/Dwarf.h"
#include "llvm/DebugInfo/DWARF/DWARFFormValue.h"
#include "llvm/DebugInfo/DWARF/DWARFUnit.h"
#include "llvm/Support/DataExtractor.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/raw_ostream.h"
#include <cstddef>
#include <cstdint>

using namespace llvm;
using namespace dwarf;

void DWARFAbbreviationDeclaration::clear() {
  Code = 0;
  Tag = DW_TAG_null;
  CodeByteSize = 0;
  HasChildren = false;
  AttributeSpecs.clear();
  FixedAttributeSize.reset();
}

DWARFAbbreviationDeclaration::DWARFAbbreviationDeclaration() {
  clear();
}

bool
DWARFAbbreviationDeclaration::extract(DataExtractor Data,
                                      uint64_t* OffsetPtr) {
  clear();
  const uint64_t Offset = *OffsetPtr;
  Code = Data.getULEB128(OffsetPtr);
  if (Code == 0) {
    return false;
  }
  CodeByteSize = *OffsetPtr - Offset;
  Tag = static_cast<llvm::dwarf::Tag>(Data.getULEB128(OffsetPtr));
  if (Tag == DW_TAG_null) {
    clear();
    return false;
  }
  uint8_t ChildrenByte = Data.getU8(OffsetPtr);
  HasChildren = (ChildrenByte == DW_CHILDREN_yes);
  // Assign a value to our optional FixedAttributeSize member variable. If
  // this member variable still has a value after the while loop below, then
  // all attribute data in this abbreviation declaration has a fixed byte size.
  FixedAttributeSize = FixedSizeInfo();

  // Read all of the abbreviation attributes and forms.
  while (true) {
    auto A = static_cast<Attribute>(Data.getULEB128(OffsetPtr));
    auto F = static_cast<Form>(Data.getULEB128(OffsetPtr));
    if (A && F) {
      bool IsImplicitConst = (F == DW_FORM_implicit_const);
      if (IsImplicitConst) {
        int64_t V = Data.getSLEB128(OffsetPtr);
        AttributeSpecs.push_back(AttributeSpec(A, F, V));
        continue