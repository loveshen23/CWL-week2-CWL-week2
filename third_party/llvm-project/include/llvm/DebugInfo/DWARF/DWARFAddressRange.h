
//===- DWARFAddressRange.h --------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_DEBUGINFO_DWARF_DWARFADDRESSRANGE_H
#define LLVM_DEBUGINFO_DWARF_DWARFADDRESSRANGE_H

#include "llvm/DebugInfo/DIContext.h"
#include <cstdint>
#include <tuple>
#include <vector>

namespace llvm {

class raw_ostream;

struct DWARFAddressRange {
  uint64_t LowPC;
  uint64_t HighPC;
  uint64_t SectionIndex;

  DWARFAddressRange() = default;

  /// Used for unit testing.
  DWARFAddressRange(uint64_t LowPC, uint64_t HighPC, uint64_t SectionIndex = 0)
      : LowPC(LowPC), HighPC(HighPC), SectionIndex(SectionIndex) {}

  /// Returns true if LowPC is smaller or equal to HighPC. This accounts for
  /// dead-stripped ranges.
  bool valid() const { return LowPC <= HighPC; }

  /// Returns true if [LowPC, HighPC) intersects with [RHS.LowPC, RHS.HighPC).
  bool intersects(const DWARFAddressRange &RHS) const {
    assert(valid() && RHS.valid());
    // Empty ranges can't intersect.
    if (LowPC == HighPC || RHS.LowPC == RHS.HighPC)
      return false;
    return LowPC < RHS.HighPC && RHS.LowPC < HighPC;
  }

  void dump(raw_ostream &OS, uint32_t AddressSize,
            DIDumpOptions DumpOpts = {}) const;
};

static inline bool operator<(const DWARFAddressRange &LHS,
                             const DWARFAddressRange &RHS) {
  return std::tie(LHS.LowPC, LHS.HighPC) < std::tie(RHS.LowPC, RHS.HighPC);
}

raw_ostream &operator<<(raw_ostream &OS, const DWARFAddressRange &R);

/// DWARFAddressRangesVector - represents a set of absolute address ranges.
using DWARFAddressRangesVector = std::vector<DWARFAddressRange>;

} // end namespace llvm

#endif // LLVM_DEBUGINFO_DWARF_DWARFADDRESSRANGE_H