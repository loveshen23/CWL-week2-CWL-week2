//===- DWARFFormValue.cpp -------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/DebugInfo/DWARF/DWARFFormValue.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/None.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/BinaryFormat/Dwarf.h"
#include "llvm/DebugInfo/DWARF/DWARFContext.h"
#include "llvm/DebugInfo/DWARF/DWARFRelocMap.h"
#include "llvm/DebugInfo/DWARF/DWARFUnit.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/WithColor.h"
#include "llvm/Support/raw_ostream.h"
#include <cinttypes>
#include <cstdint>
#include <limits>

using namespace llvm;
using namespace dwarf;

static const DWARFFormValue::FormClass DWARF5FormClasses[] = {
    DWARFFormValue::FC_Unknown,  // 0x0
    DWARFFormValue::FC_Address,  // 0x01 DW_FORM_addr
    DWARFFormValue::FC_Unknown,  // 0x02 unused
    DWARFFormValue::FC_Block,    // 0x03 DW_FORM_block2
    DWARFFormValue::FC_Block,    // 0x04 DW_FORM_block4
    DWARFFormValue::FC_Constant, // 0x05 DW_FORM_data2
    // --- These can be FC_SectionOffset in DWARF3 and below:
    DWARFFormValue::FC_Constant, // 0x06 DW_FORM_data4
    DWARFFormValue::FC_Constant, // 0x07 DW_FORM_data8
    // ---
    DWARFFormValue::FC_String,        // 0x08 DW_FORM_string
    DWARFFormValue::FC_Block,         // 0x09 DW_FORM_block
    DWARFFormValue::FC_Block,         // 0x0a DW_FORM_block1
    DWARFFormValue::FC_Constant,      // 0x0b DW_FORM_data1
    DWARFFormValue::FC_Flag,          // 0x0c DW_FORM_flag
    DWARFFormValue::FC_Constant,      // 0x0d DW_FORM_sdata
    DWARFFormValue::FC_String,        // 0x0e DW_FORM_strp
    DWARFFormValue::FC_Constant,      // 0x0f DW_FORM_udata
    DWARFFormValue::FC_Reference,     // 0x10 DW_FORM_ref_addr
    DWARFFormValue::FC_Reference,     // 0x11 DW_FORM_ref1
    DWARFFormValue::FC_Reference,     // 0x12 DW_FORM_ref2
    DWARFFormValue::FC_Reference,     // 0x13 DW_FORM_ref4
    DWARFFormValue::FC_Reference,     // 0x14 DW_FORM_ref8
    DWARFFormValue::FC_Reference,     // 0x15 DW_FORM_ref_udata
    DWARFFormValue::FC_Indirect,      // 0x16 DW_FORM_indirect
    DWARFFormValue::FC_SectionOffset, // 0x17 DW_FORM_sec_offset
    DWARFFormValue::FC_Exprloc,       // 0x18 DW_FORM_exprloc
    DWARFFormValue::FC_Flag,          // 0x19 DW_FORM_flag_present
    DWARFFormValue::FC_String,        // 0x1a DW_FORM_strx
    DWARFFormValue::FC_Address,       // 0x1b DW_FORM_addrx
    DWARFFormValue::FC_Reference,     // 0x1c DW_FORM_ref_sup4
    DWARFFormValue::FC_String,        // 0x1d DW_FORM_strp_sup
    DWARFFormValue::FC_Constant,      // 0x1e DW_FORM_data16
    DWARFFormValue::FC_String,        // 0x1f DW_FORM_line_strp
    DWARFFormValue::FC_Reference,     // 0x20 DW_FORM_ref_sig8
    DWARFFormValue::FC_Constant,      // 0x21 DW_FORM_implicit_const
    DWARFFormValue::FC_SectionOffset, // 0x22 DW_FORM_loclistx
    DWARFFormValue::FC_SectionOffset, // 0x23 DW_FORM_rnglistx
    DWARFFormValue::FC_Reference,     // 0x24 DW_FORM_ref_sup8
    DWARFFormValue::FC_String,        // 0x25 DW_FORM_strx1
    DWARFFormValue::FC_String,        // 0x26 DW_FORM_strx2
    DWARFFormValue::FC_String,        // 0x27 DW_FORM_strx3
    DWARFFormValue::FC_String,        // 0x28 DW_FORM_strx4
    DWARFFormValue::FC_Address,       // 0x29 DW_FORM_addrx1
    DWARFFormValue::FC_Address,       // 0x2a DW_FORM_addrx2
    DWARFFormValue::FC_Address,       // 0x2b DW_FORM_addrx3
    DWARFFormValue::FC_Address,       // 0x2c DW_FORM_addrx4

};

DWARFFormValue DWARFFormValue::createFromSValue(dwarf::Form F, int64_t V) {
  return DWARFFormValue(F, ValueType(V));
}

DWARFFormValue DWARFFormValue::createFromUValue(dwarf::Form F, uint64_t V) {
  return DWARFFormValue(F, ValueType(V));
}

DWARFFormValue DWARFFormValue::createFromPValue(dwarf::Form F, const char *V) {
  return DWARFFormValue(F, ValueType(V));
}

DWARFFormValue DWARFFormValue::createFromBlockValue(dwarf::Form F,
                                                    ArrayRef<uint8_t> D) {
  ValueType V;
  V.uval = D.size();
  V.data = D.data();
  return DWARFFormValue(F, V);
}

DWARFFormValue DWARFFormValue::createFromUnit(dwarf::Form F, const DWARFUnit *U,
                                              uint64_t *OffsetPtr) {
  DWARFFormValue FormValue(F);
  FormValue.extractValue(U->getDebugInfoExtractor(), OffsetPtr,
                         U->getFormParams(), U);
  return FormValue;
}

bool DWARFFormValue::skipValue(dwarf::Form Form, DataExtractor DebugInfoData,
                               uint64_t *OffsetPtr,
                               const dwarf::FormParams Params) {
  bool Indirect = false;
  do {
    switch (Form) {
    // Blocks of inlined data that have a length field and the data bytes
    // inlined in the .debug_info.
    case DW_FORM_exprloc:
    case DW_FORM_block: {
      uint64_t size = DebugInfoData.getULEB128(OffsetPtr);
      *OffsetPtr += size;
      return true;
    }
    case DW_FORM_block1: {
      uint8_t size = DebugInfoData.getU8(OffsetPtr);
      *OffsetPtr += size;
      return true;
    }
    case DW_FORM_block2: {
      uint16_t size = DebugInfoData.getU16(OffsetPtr);
      *OffsetPtr += size;
      return true;
    }
    case DW_FORM_block4: {
      uint32_t size = DebugInfoData.getU32(OffsetPtr);
      *OffsetPtr += size;
      return true;
    }

    //