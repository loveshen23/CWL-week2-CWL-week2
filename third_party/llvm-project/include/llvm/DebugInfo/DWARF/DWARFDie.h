//===- DWARFDie.h -----------------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_DEBUGINFO_DWARFDIE_H
#define LLVM_DEBUGINFO_DWARFDIE_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/iterator.h"
#include "llvm/ADT/iterator_range.h"
#include "llvm/BinaryFormat/Dwarf.h"
#include "llvm/DebugInfo/DIContext.h"
#include "llvm/DebugInfo/DWARF/DWARFAddressRange.h"
#include "llvm/DebugInfo/DWARF/DWARFAttribute.h"
#include "llvm/DebugInfo/DWARF/DWARFDebugInfoEntry.h"
#include <cassert>
#include <cstdint>
#include <iterator>

namespace llvm {

class DWARFUnit;
class raw_ostream;

//===----------------------------------------------------------------------===//
/// Utility class that carries the DWARF compile/type unit and the debug info
/// entry in an object.
///
/// When accessing information from a debug info entry we always need to DWARF
/// compile/type unit in order to extract the info correctly as some information
/// is relative to the compile/type unit. Prior to this class the DWARFUnit and
/// the DWARFDebugInfoEntry was passed around separately and there was the
/// possibility for error if the wrong DWARFUnit was used to extract a unit
/// relative offset. This class helps to ensure that this doesn't happen and
/// also simplifies the attribute extraction calls by not having to specify the
/// DWARFUnit for each call.
class DWARFDie {
  DWARFUnit *U = nullptr;
  const DWARFDebugInfoEntry *Die = nullptr;

public:
  DWARFDie() = default;
  DWARFDie(DWARFUnit *Unit, const DWARFDebugInfoEntry *D) : U(Unit), Die(D) {}

  bool isValid() const { return U && Die; }
  explicit operator bool() const { return isValid(); }
  const DWARFDebugInfoEntry *getDebugInfoEntry() const { return Die; }
  DWARFUnit *getDwarfUnit() const { return U; }

  /// Get the abbreviation declaration for this DIE.
  ///
  /// \returns the abbreviation declaration or NULL for null tags.
  const DWARFAbbreviationDeclaration *getAbbreviationDeclarationPtr() const {
    assert(isValid() && "must check validity prior to calling");
    return Die->getAbbreviationDeclarationPtr();
  }

  /// Get the absolute offset into the debug info or types section.
  ///
  /// \returns the DIE offset or -1U if invalid.
  uint64_t getOffset() const {
    assert(isValid() && "must check validity prior to calling");
    return Die->getOffset();
  }

  dwarf::Tag getTag() const {
    auto AbbrevDecl = getAbbreviationDeclarationPtr();
    if (AbbrevDecl)
      return AbbrevDecl->getTag();
    return dwarf::DW_TAG_null;
  }

  bool hasChildren() const {
    assert(isValid() && "must check validity prior to calling");
    return Die->hasChildren();
  }

  /// Returns true for a valid DIE that terminates a sibling chain.
  bool isNULL() const { return getAbbreviationDeclarationPtr() == nullptr; }

  /// Returns true if DIE represents a subprogram (not inlined).
  bool isSubprogramDIE() const;

  /// Returns true if DIE represents a subprogram or an inlined subroutine.
  bool isSubroutineDIE() const;

  /// Get the parent of this DIE object.
  ///
  /// \returns a valid DWARFDie instance if this object has a parent or an
  /// invalid DWARFDie instance if it doesn't.
  DWARFDie getParent() const;

  /// Get the sibling of this DIE object.
  ///
  /// \returns a valid DWARFDie instance if this object has a sibling or an
  /// invalid DWARFDie instance if it doesn't.
  DWARFDie getSibling() const;

  /// Get the previous sibling of this DIE object.
  ///
  /// \returns a valid DWARFDie instance if this object has a sibling or an
  /// invalid DWARFDie instance if it doesn't.
  DWARFDie getPreviousSibling() const;

  /// Get the first child of this DIE object.
  ///
  /// \returns a valid DWARFDie instance if this object has children or an
  /// invalid DWARFDie instance if it doesn't.
  DWARFDie getFirstChild() const;

  /// Get the last child of this DIE object.
  ///
  /// \returns a valid null DWARFDie instance if this object has children or an
  /// invalid DWARFDie instance if it doesn't.
  DWARFDie getLastChild() const;

  /// Dump the DIE and all of its attributes to the supplied stream.
  ///
  /// \param OS the stream to use for output.
  /// \param indent the number of characters to indent each line that is output.
  void dump(raw_ostream &OS, unsigned indent = 0,
            DIDumpOptions DumpOpts = DIDumpOptions()) const;

  /// Convenience zero-argument overload for debugging.
  LLVM_DUMP_METHOD void dump() const;

  /// Extract the specified attribute from this DIE.
  ///
  /// Extract an attribute value from this DIE only. This call doesn't look
  /// for the attribute value in any DW_AT_specification or
  /// DW_AT_abstract_origin referenced DIEs.
  ///
  /// \param Attr the attribute to extract.
  /// \returns an optional DWARFFormValue that will have the form value if the
  /// attribute was successfully extracted.
  Optional<DWARFFormValue> find(dwarf::Attribute Attr) const;

  /// Extract the first value of any attribute in Attrs from this DIE.
  ///
  /// Extract the first attribute that matches from this DIE only. This call
  /// doesn't look for the attribute value in any DW_AT_specification or
  /// DW_AT_abstract_origin referenced DIEs. The attributes will be searched
  /// linearly in the order they are specified within Attrs.
  ///
  /// \param Attrs an array of DWARF attribute to look for.
  /// \returns an optional that has a valid DWARFFormValue for the first
  /// matching attribute in Attrs, or None if none of the attributes in Attrs
  /// exist in this DIE.
  Optional<DWARFFormValue> find(ArrayRef<dwarf::Attribute> Attrs) const;

  /// Extract the first value of any attribute in Attrs from this DIE and
  /// recurse into any DW_AT_specification or DW_AT_abstract_origin referenced
  /// DIEs.
  ///
  /// \param Attrs an array of DWARF attribute to look for.
  /// \returns an optional that has a valid DWARFFormValue for the first
  /// matching attribute in Attrs, or None if none of the attributes in Attrs
  /// exist in this DIE or in any DW_AT_specification or DW_AT_abstract_origin
  /// DIEs.
  Optional<DWARFFormValue>
  findRecursively(ArrayRef<dwarf::Attribute> Attrs) const;

  /// Extract the specified attribute from this DIE as the referenced DIE.
  ///
  /// Regardless of the reference type, return the correct DWARFDie instance if
  /// the attribute exists. The returned DWARFDie object might be from another
  /// DWARFUnit, but that is all encapsulated in the new DWARFDie object.
  ///
  /// Extract an attribute value from this DIE only. This call doesn't look
  /// for the attribute value in any DW_AT_specification or
  /// DW_AT_abstract_origin referenced DIEs.
  ///
  /// \param Attr the attribute to extract.
  /// \returns a valid DWARFDie instance if the attribute exists, or an invalid
  /// DWARFDie object if it doesn't.
  DWARFDie getAttributeValueAsReferencedDie(dwarf::Attribute Attr) const;
  DWARFDie getAttributeValueAsReferencedDie(const DWARFFormValue &V) const;

  /// Extract the range base attribute from this DIE as absolute section offset.
  ///
  /// This is a utility function that checks for either the DW_AT_rnglists_base
  /// or DW_AT_GNU_ranges_base attribute.
  ///
  /// \returns anm optional absolute section offset value for the attribute.
  Optional<uint64_t> getRangesBaseAttribute() const;

  /// Get the DW_AT_high_pc attribute value as an address.
  ///
  /// In DWARF version 4 and later the high PC can be encoded as an offset from
  /// the DW_AT_low_pc. This function takes care of extracting the value as an
  /// address or offset and adds it to the low PC if needed and returns the
  /// value as an optional in case the DIE doesn't have a DW_AT_high_pc
  /// attribute.
  ///
  /// \param LowPC the low PC that might be needed to calculate the high PC.
  /// \returns an optional address value for the attribute.
  Optional<uint64_t> getHighPC(uint64_t LowPC) const;

  /// Retrieves DW_AT_low_pc and DW_AT_high_pc from CU.
  /// Returns true if both attributes are present.
  bool getLowAndHighPC(uint64_t &LowPC, uint64_t &HighPC,
                       uint64_t &SectionIndex) const;

  /// Get the address ranges for this DIE.
  ///
  /// Get the hi/low PC range if both attributes are available or exrtracts the
  /// non-contiguous address ranges from the DW_AT_ranges attribute.
  ///
  /// Extracts the range information from this DIE only. This call doesn't look
  /// for the range in any DW_AT_specification or DW_AT_abstract_origin DIEs.
  ///
  /// \returns a address range vector that might be empty if no address range
  /// information is available.
  Expected<DWARFAddressRangesVector> getAddressRanges() const;

  /// Get all address ranges for any DW_TAG_subprogram DIEs in this DIE or any
  /// of its children.
  ///
  /// Get the hi/low PC range if both attributes are available or exrtracts the
  /// non-contiguous address ranges from the DW_AT_ranges attribute for this DIE
  /// and all children.
  ///
  /// \param Ranges the addres range vector to fill in.
  void collectChildrenAddressRanges(DWARFAddressRangesVector &Ranges) const;

  bool addressRangeContainsAddress(const uint64_t Address) const;

  /// If a DIE represents a subprogram (or inlined subroutine), returns its
  /// mangled name (or short name, if mangled is missing). This name may be
  /// fetched from specification or abstract origin for this subprogram.
  /// Returns null if no name is found.
  const char