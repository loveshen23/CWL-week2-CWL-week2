//===- DWARFDebugLine.h -----------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_DEBUGINFO_DWARFDEBUGLINE_H
#define LLVM_DEBUGINFO_DWARFDEBUGLINE_H

#include "llvm/ADT/Optional.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/DebugInfo/DIContext.h"
#include "llvm/DebugInfo/DWARF/DWARFCompileUnit.h"
#include "llvm/DebugInfo/DWARF/DWARFDataExtractor.h"
#include "llvm/DebugInfo/DWARF/DWARFFormValue.h"
#include "llvm/DebugInfo/DWARF/DWARFRelocMap.h"
#include "llvm/DebugInfo/DWARF/DWARFTypeUnit.h"
#include "llvm/Support/MD5.h"
#include "llvm/Support/Path.h"
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace llvm {

class DWARFUnit;
class raw_ostream;

class DWARFDebugLine {
public:
  struct FileNameEntry {
    FileNameEntry() = default;

    DWARFFormValue Name;
    uint64_t DirIdx = 0;
    uint64_t ModTime = 0;
    uint64_t Length = 0;
    MD5::MD5Result Checksum;
    DWARFFormValue Source;
  };

  /// Tracks which optional content types are present in a DWARF file name
  /// entry format.
  struct ContentTypeTracker {
    ContentTypeTracker() = default;

    /// Whether filename entries provide a modification timestamp.
    bool HasModTime = false;
    /// Whether filename entries provide a file size.
    bool HasLength = false;
    /// For v5, whether filename entries provide an MD5 checksum.
    bool HasMD5 = false;
    /// For v5, whether filename entries provide source text.
    bool HasSource = false;

    /// Update tracked content types with \p ContentType.
    void trackContentType(dwarf::LineNumberEntryFormat ContentType);
  };

  struct Prologue {
    Prologue();

    /// The size in bytes of the statement information for this compilation unit
    /// (not including the total_length field itself).
    uint64_t TotalLength;
    /// Version, address size (starting in v5), and DWARF32/64 format; these
    /// parameters affect interpretation of forms (used in the directory and
    /// file tables starting with v5).
    dwarf::FormParams FormParams;
    /// The number of bytes following the prologue_length field to the beginning
    /// of the first byte of the statement program itself.
    uint64_t PrologueLength;
    /// In v5, size in bytes of a segment selector.
    uint8_t SegSelectorSize;
    /// The size in bytes of the smallest target machine instruction. Statement
    /// program opcodes that alter the address register first multiply their
    /// operands by this value.
    uint8_t MinInstLength;
    /// The maximum number of individual operations that may be encoded in an
    /// instruction.
    uint8_t MaxOpsPerInst;
    /// The initial value of theis_stmtregister.
    uint8_t DefaultIsStmt;
    /// This parameter affects the meaning of the special opcodes. See below.
    int8_t LineBase;
    /// This parameter affects the meaning of the special opcodes. See below.
    uint8_t LineRange;
    /// The number assigned to the first special opcode.
    uint8_t OpcodeBase;
    /// This tracks which optional file format content types are present.
    ContentTypeTracker ContentTypes;
    std::vector<uint8_t> StandardOpcodeLengths;
    std::vector<DWARFFormValue> IncludeDirectories;
    std::vector<FileNameEntry> FileNames;

    const dwarf::FormParams getFormParams() const { return FormParams; }
    uint16_t getVersion() const { return FormParams.Version; }
    uint8_t getAddressSize() const { return FormParams.AddrSize; }
    bool isDWARF64() const { return FormParams.Format == dwarf::DWARF64; }

    uint32_t sizeofTotalLength() const { return isDWARF64() ? 12 : 4; }

    uint32_t sizeofPrologueLength() const { return isDWARF64() ? 8 : 4; }

    bool totalLengthIsValid() const;

    /// Length of the prologue in bytes.
    uint32_t getLength() const {
      return PrologueLength + sizeofTotalLength() + sizeof(getVersion()) +
             sizeofPrologueLength();
    }

    /// Length of the line table data in bytes (not including the prologue).
    uint32_t getStatementTableLength() const {
      return TotalLength + sizeofTotalLength() - getLength();
    }

    int32_t getMaxLineIncrementForSpecialOpcode() const {
      return LineBase + (int8_t)LineRange - 1;
    }

    /// Get DWARF-version aware access to the file name entry at the provided
    /// index.
    const llvm::DWARFDebugLine::FileNameEntry &
    getFileNameEntry(uint64_t Index) const;

    bool hasFileAtIndex(uint64_t FileIndex) const;

    bool
    getFileNameByIndex(uint64_t FileIndex, StringRef CompDir,
                       DILineInfoSpecifier::FileLineInfoKind Kind,
                       std::string &Result,
                       sys::path::Style Style = sys::path::Style::native) const;

    void clear();
    void dump(raw_ostream &OS, DIDumpOptions DumpOptions) const;
    Error parse(const DWARFDataExtractor &DebugLineData, uint64_t *OffsetPtr,
                const DWARFContext &Ctx, const DWARFUnit *U = nullptr);
  };

  /// Standard .debug_line state machine structure.
  struct Row {
    explicit Row(bool DefaultIsStmt = false);

    /// Called after a row is appended to the matrix.
    void postAppend();
    void reset(bool DefaultIsStmt);
    void dump(raw_ostream &OS) const;

    static void dumpTableHeader(raw_ostream &OS);

    static bool orderByAddress(const Row &LHS, const Row &RHS) {
      return std::tie(LHS.Address.SectionIndex, LHS.Address.Address) <
             std::tie(RHS.Address.SectionIndex, RHS.Address.Address);
    }

    /// The program-counter value corresponding to a machine instruction
    /// generated by the compiler and section index pointing to the section
    /// containg this PC. If relocation information is present then section
    /// index is the index of the section which contains above address.
    /// Otherwise this is object::SectionedAddress::Undef value.
    object::SectionedAddress Address;
    /// An unsigned integer indicating a source line number. Lines are numbered
    /// beginning at 1. The compiler may emit the value 0 in cases where an
    /// instruction cannot be attributed to any source line.
    uint32_t Line;
    /// An unsigned integer indicating a column number within a source line.
    /// Columns are numbered beginning at 1. The value 0 is reserved to indicate
    /// that a statement begins at the 'left edge' of the line.
    uint16_t Column;
    /// An unsigned integer indicating the identity of the source file
    /// corresponding to a machine instruction.
    uint16_t File;
    /// An unsigned integer representing the DWARF path discriminator value
    /// for this location.
    uint32_t Discriminator;
    /// An unsigned integer whose value encodes the applicable instruction set
    /// architecture for the current instruction.
    uint8_t Isa;
    /// A boolean indicating that the current instruction is the beginning of a
    /// statement.
    uint8_t IsStmt : 1,
        /// A boolean indicating that the current instruction is the
        /// beginning of a basic block.
        BasicBlock : 1,
        /// A boolean indicating that the current address is that of the
        /// first byte after the end of a sequence of target machine
        /// instructions.
        EndSequence : 1,
        /// A boolean indicating that the current address is one (of possibly
        /// many) where execution should be suspended for an entry breakpoint
        /// of a function.
        PrologueEnd : 1,
        /// A boolean indicating that the current address is one (of possibly
        /// many) where execution should be suspended for an exit breakpoint
        /// of a function.
        EpilogueBegin : 1;
  };

  /// Represents a series of contiguous machine instructions. Line table for
  /// each compilation unit may consist of multiple sequences, which are not
  /// guaranteed to be in the order of ascending instruction address.
  struct Sequence {
    Sequence();

    /// Sequence describes instructions at address range [LowPC, HighPC)
    /// and is described by line table rows [FirstRowIndex, LastRowIndex).
    uint64_t LowPC;
    uint64_t HighPC;
    /// If relocation information is present then this is the index of the
    /// section which contains above addresses. Otherwise this is
    /// object::SectionedAddress::Undef value.
    uint64_t SectionIndex;
    unsigned FirstRowIndex;
    unsigned LastRowIndex;
    bool Empty;

    void reset();

    static bool orderByHighPC(const Sequence &LHS, const Sequence &RHS) {
      return std::tie(LHS.SectionIndex, LHS.HighPC) <
             std::tie(RHS.SectionIndex, RHS.HighPC);
    }

    bool isValid() const {
      return !Empty && (LowPC < HighPC) && (FirstRowIndex < LastRowIndex);
    }

    bool containsPC(object::SectionedAddress PC) const {
      return SectionIndex == PC.SectionIndex &&
             (LowPC <= PC.Address && PC.Address < HighPC);
    }
  };

  struct LineTable {
    LineTable();

    /// Represents an invalid row
    const uint32_t UnknownRowIndex = UINT32_MAX;

    void appendRow(const DWARFDebugLine::Row &R) { Rows.push_back(R); }

    void appendSequence(const DWARFDebugLine::Sequence &S) {
      Sequences.push_back(S);
    }

    /// Returns the index of the row with file/line info for a given address,
    /// or UnknownRowIndex if there is no such row.
    uint32_t lookupAddress(object::SectionedAddress Address) const;

    bool lookupAddressRange(object::SectionedAddress Address, uint64_t Size,
                            std::vector<uint32_t> &Result) const;

    bool hasFileAtIndex(uint64_t FileIndex) const {
      return Prologue.hasFileAtIndex(FileIndex);
    }

    /// Extracts filename by its index in filename table in prologue.
    /// In Dwarf 4, the files are 1-indexed and the current compilation file
    /// name is not represented in the list. In DWARF v5, the files are
    /// 0-indexed and the primary source file has the index 0.
    /// Returns true on success.
    bool getFileNameByIndex(uint64_t FileIndex, StringRef CompDir,
                            DILineInfoSpecifier::FileLineInfoKind Kind,
                            std::string &Result) const {
      return Prologue.getFileNameByIndex(FileIndex, CompDir, Kind, Result);
    }

    /// Fills the Result argument with the file and