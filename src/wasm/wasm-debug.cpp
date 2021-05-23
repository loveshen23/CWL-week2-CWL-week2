/*
 * Copyright 2019 WebAssembly Community Group participants
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "wasm-debug.h"
#include "wasm.h"

#ifdef BUILD_LLVM_DWARF
#include "llvm/ObjectYAML/DWARFEmitter.h"
#include "llvm/ObjectYAML/DWARFYAML.h"
#include "llvm/include/llvm/DebugInfo/DWARFContext.h"

std::error_code dwarf2yaml(llvm::DWARFContext& DCtx, llvm::DWARFYAML::Data& Y);
#endif

#include "wasm-binary.h"
#include "wasm-debug.h"
#include "wasm.h"

namespace wasm::Debug {

bool isDWARFSection(Name name) { return name.startsWith(".debug_"); }

bool hasDWARFSections(const Module& wasm) {
  for (auto& section : wasm.customSections) {
    if (isDWARFSection(section.name)) {
      return true;
    }
  }
  return false;
}

#ifdef BUILD_LLVM_DWARF

// In wasm32 the address size is 32 bits.
static const size_t AddressSize = 4;

struct BinaryenDWARFInfo {
  llvm::StringMap<std::unique_ptr<llvm::MemoryBuffer>> sections;
  std::unique_ptr<llvm::DWARFContext> context;

  BinaryenDWARFInfo(const Module& wasm) {
    // Get debug sections from the wasm.
    for (auto& section : wasm.customSections) {
      if (Name(section.name).startsWith(".debug_") && section.data.data()) {
        // TODO: efficiency
        sections[section.name.substr(1)] = llvm::MemoryBuffer::getMemBufferCopy(
          llvm::StringRef(section.data.data(), section.data.size()));
      }
    }
    // Parse debug sections.
    uint8_t addrSize = AddressSize;
    bool isLittleEndian = true;
    context = llvm::DWARFContext::create(sections, addrSize, isLittleEndian);
    if (context->getMaxVersion() > 4) {
      std::cerr << "warning: unsupported DWARF version ("
                << context->getMaxVersion() << ")\n";
    }
  }
};

void dumpDWARF(const Module& wasm) {
  BinaryenDWARFInfo info(wasm);
  std::cout << "DWARF debug info\n";
  std::cout << "================\n\n";
  for (auto& section : wasm.customSections) {
    if (Name(section.name).startsWith(".debug_")) {
      std::cout << "Contains section " << section.name << " ("
                << section.data.size() << " bytes)\n";
    }
  }
  llvm::DIDumpOptions options;
  options.DumpType = llvm::DIDT_All;
  options.ShowChildren = true;
  options.Verbose = true;
  info.context->dump(llvm::outs(), options);
}

bool shouldPreserveDWARF(PassOptions& options, Module& wasm) {
  return options.debugInfo && hasDWARFSections(wasm);
}

//
// Big picture: We use a DWARFContext to read data, then DWARFYAML support
// code to write it. That is not the main LLVM Dwarf code used for writing
// object files, but it avoids us create a "fake" MC layer, and provides a
// simple way to write out the debug info. Likely the level of info represented
// in the DWARFYAML::Data object is sufficient for Binaryen's needs, but if not,
// we may need a different approach.
//
// In more detail:
//
// 1. Binary sections => DWARFContext:
//
//     llvm::DWARFContext::create(sections..)
//
// 2. DWARFContext => DWARFYAML::Data
//
//     std::error_code dwarf2yaml(DWARFContext &DCtx, DWARFYAML::Data &Y) {
//
// 3. DWARFYAML::Data => binary sections
//
//     StringMap<std::unique_ptr<MemoryBuffer>>
//     EmitDebugSections(llvm::DWARFYAML::Data &DI, bool ApplyFixups);
//

// Represents the state when parsing a line table.
struct LineState {
  uint32_t addr = 0;
  // TODO sectionIndex?
  uint32_t line = 1;
  uint32_t col = 0;
  uint32_t file = 1;
  uint32_t isa = 0;
  uint32_t discriminator = 0;
  bool isStmt;
  bool basicBlock = false;
  bool prologueEnd = false;
  bool epilogueBegin = false;
  // Each instruction is part of a sequence, all of which get the same ID. The
  // order within a sequence may change if binaryen reorders things, which means
  // that we can't track the end_sequence location and assume it is at the end -
  // we must track sequences and then emit an end for each one.
  // -1 is an invalid marker value (note that this assumes we can fit all ids
  // into just under 32 bits).
  uint32_t sequenceId = -1;

  LineState(const LineState& other) = default;
  LineState(const llvm::DWARFYAML::LineTable& table, uint32_t sequenceId)
    : isStmt(table.DefaultIsStmt), sequenceId(sequenceId) {}

  LineState& operator=(const LineState& other) = default;

  // Updates the state, and returns whether a new row is ready to be emitted.
  bool update(llvm::DWARFYAML::LineTableOpcode& opcode,
              const llvm::DWARFYAML::LineTable& table) {
    switch (opcode.Opcode) {
      case 0: {
        // Extended opcodes
        switch (opcode.SubOpcode) {
          case llvm::dwarf::DW_LNE_set_address: {
            addr = opcode.Data;
            break;
          }
          case llvm::dwarf::DW_LNE_end_sequence: {
            return true;
          }
          case llvm::dwarf::DW_LNE_set_discriminator: {
            discriminator = opcode.Data;
            break;
          }
          case llvm::dwarf::DW_LNE_define_file: {
            Fatal() << "TODO: DW_LNE_define_file";
          }
          default: {
            // An unknown opcode, ignore.
            std::cerr << "warning: unknown subopcode " << opcode.SubOpcode
                      << " (this may be an unsupported version of DWARF)\n";
          }
        }
        break;
      }
      case llvm::dwarf::DW_LNS_set_column: {
        col = opcode.Data;
        break;
      }
      case llvm::dwarf::DW_LNS_set_prologue_end: {
        prologueEnd = true;
        break;
      }
      case llvm::dwarf::DW_LNS_copy: {
        return true;
      }
      case llvm::dwarf::DW_LNS_advance_pc: {
        if (table.MinInstLength != 1) {
          std::cerr << "warning: bad MinInstLength "
                       "(this may be an unsupported DWARF version)";
        }
        addr += opcode.Data;
        break;
      }
      case llvm::dwarf::DW_LNS_advance_line: {
        line += opcode.SData;
        break;
      }
      case llvm::dwarf::DW_LNS_set_file: {
        file = opcode.Data;
        break;
      }
      case llvm::dwarf::DW_LNS_negate_stmt: {
        isStmt = !isStmt;
        break;
      }
      case llvm::dwarf::DW_LNS_set_basic_block: {
        basicBlock = true;
        break;
      }
      case llvm::dwarf::DW_LNS_const_add_pc: {
        uint8_t AdjustOpcode = 255 - table.OpcodeBase;
        uint64_t AddrOffset =
          (AdjustOpcode / table.LineRange) * table.MinInstLength;
        addr += AddrOffset;
        break;
      }
      case llvm::dwarf::DW_LNS_fixed_advance_pc: {
        addr += opcode.Data;
        break;
      }
      case llvm::dwarf::DW_LNS_set_isa: {
        isa = opcode.Data;
        break;
      }
      default: {
        if (opcode.Opcode >= table.OpcodeBase) {
          // Special opcode: adjust line and addr, using some math.
          uint8_t AdjustOpcode =
            opcode.Opcode - table.OpcodeBase; // 20 - 13 = 7
          uint64_t AddrOffset = (AdjustOpcode / table.LineRange) *
                                table.MinInstLength; // (7 / 14) * 1 = 0
          int32_t LineOffset =
            table.LineBase +
            (AdjustOpcode % table.LineRange); // -5 + (7 % 14) = 2
          line += LineOffset;
          addr += AddrOffset;
          return true;
        } else {
          Fatal() << "unknown debug line opcode: " << std::hex << opcode.Opcode;
        }
      }
    }
    return false;
  }

  // Checks if this starts a new range of addresses. Each range is a set of
  // related addresses, where in particular, if the first has been zeroed out
  // by the linker, we must omit the entire range. (If we do not, then the
  // initial range is 0 and the others are offsets relative to it, which will
  // look like random addresses, perhaps into the middle of instructions, and
  // perhaps that happen to collide with real ones.)
  bool startsNewRange(llvm::DWARFYAML::LineTableOpcode& opcode) {
    return opcode.Opcode == 0 &&
           opcode.SubOpcode == llvm::dwarf::DW_LNE_set_address;
  }

  bool needToEmit() {
    // Zero values imply we can ignore this line.
    // https://github.com/WebAssembly/debugging/issues/9#issuecomment-567720872
    return line != 0 && addr != 0;
  }

  // Given an old state, emit the diff from it to this state into a new line
  // table entry (that will be emitted in the updated DWARF debug line section).
  void emitDiff(const LineState& old,
                std::vector<llvm::DWARFYAML::LineTableOpcode>& newOpcodes,
                const llvm::DWARFYAML::LineTable& table,
                bool endSequence) const {
    bool useSpecial = false;
    if (addr != old.addr || line != old.line) {
      // Try to use a special opcode TODO
    }
    if (addr != old.addr && !useSpecial) {
      // len = 1 (subopcode) + 4 (wasm32 address)
      // FIXME: look at AddrSize on the Unit.
      auto item = makeItem(llvm::dwarf::DW_LNE_set_address, 5);
      item.Data = addr;
      newOpcodes.push_back(item);
    }
    if (line != old.line && !useSpecial) {
      auto item = makeItem(llvm::dwarf::DW_LNS_advance_line);
      // In wasm32 we have 32-bit addresses, and the delta here might be
      // negative (note that SData is 64-bit, as LLVM supports 64-bit
      // addresses too).
      item.SData = int32_t(line - old.line);
      newOpcodes.push_back(item);
    }
    if (col != old.col) {
      auto item = makeItem(llvm::dwarf::DW_LNS_set_column);
      item.Data = col;
      newOpcodes.push_back(item);
    }
    if (file != old.file) {
      auto item = makeItem(llvm::dwarf::DW_LNS_set_file);
      item.Data = file;
      newOpcodes.push_back(item);
    }
    if (isa != old.isa) {
      auto item = makeItem(llvm::dwarf::DW_LNS_set_isa);
      item.Data = isa;
      newOpcodes.push_back(item);
    }
    if (discriminator != old.discriminator) {
      // len = 1 (subopcode) + 4 (wasm32 address)
      auto item = makeItem(llvm::dwarf::DW_LNE_set_discriminator, 5);
      item.Data = discriminator;
      newOpcodes.push_back(item);
    }
    if (isStmt != old.isStmt) {
      newOpcodes.push_back(makeItem(llvm::dwarf::DW_LNS_negate_stmt));
    }
    if (basicBlock != old.basicBlock) {
      assert(basicBlock);
      newOpcodes.push_back(makeItem(llvm::dwarf::DW_LNS_set_basic_block));
    }
    if (prologueEnd) {
      newOpcodes.push_back(makeItem(llvm::dwarf::DW_LNS_set_prologue_end));
    }
    if (epilogueBegin != old.epilogueBegin) {
      Fatal() << "eb";
    }
    if (useSpecial) {
      // Emit a special, which emits a line automatically.
      // TODO
    } else {
      // Emit the line manually.
      if (endSequence) {
        // len = 1 (subopcode)
        newOpcodes.push_back(makeItem(llvm::dwarf::DW_LNE_end_sequence, 1));
      } else {
        newOpcodes.push_back(makeItem(llvm::dwarf::DW_LNS_copy));
      }
    }
  }

  // Some flags are automatically reset after each debug line.
  void resetAfterLine() { prologueEnd = false; }

private:
  llvm::DWARFYAML::LineTableOpcode
  makeItem(llvm::dwarf::LineNumberOps opcode) const {
    llvm::DWARFYAML::LineTableOpcode item = {};
    item.Opcode = opcode;
    return item;
  }

  llvm::DWARFYAML::LineTableOpcode
  makeItem(llvm::dwarf::LineNumberExtendedOps opcode, uint64_t len) const {
    auto item = makeItem(llvm::dwarf::LineNumberOps(0));
    // All the length after the len field itself, including the subopcode
    // (1 byte).
    item.ExtLen = len;
    item.SubOpcode = opcode;
    return item;
  }
};

// Represents a mapping of addresses to expressions. We track beginnings and
// endings of expressions separately, since the end of one (which is one past
// the end in DWARF notation) overlaps with the beginning of the next, and also
// to let us use contextual information (we may know we are looking up the end
// of an instruction).
struct AddrExprMap {
  std::unordered_map<BinaryLocation, Expression*> startMap;
  std::unordered_map<BinaryLocation, Expression*> endMap;

  // Some instructions have delimiter binary locations, like the else and end in
  // and if. Track those separately, including their expression and their id
  // ("else", "end", etc.), as they are rare, and we don't want to
  // bloat the common case which is represented in the earlier maps.
  struct DelimiterInfo {
    Expression* expr;
    size_t id;
  };
  std::unordered_map<BinaryLocation, DelimiterInfo> delimiterMap;

  // Construct the map from the binaryLocations loaded from the wasm.
  AddrExprMap(const Module& wasm) {
    for (auto& func : wasm.functions) {
      for (auto& [expr, span] : func->expressionLocations) {
        add(expr, span);
      }
      for (auto& [expr, delim] : func->delimiterLocations) {
        add(expr, delim);
      }
    }
  }

  Expression* getStart(BinaryLocation addr) const {
    auto iter = startMap.find(addr);
    if (iter != startMap.end()) {
      return iter->second;
    }
    return nullptr;
  }

  Expression* getEnd(BinaryLocation addr) const {
    auto iter = endMap.find(addr);
    if (iter != endMap.end()) {
      return iter->second;
    }
    return nullptr;
  }

  DelimiterInfo getDelimiter(BinaryLocation addr) const {
    auto iter = delimiterMap.find(addr);
    if (iter != delimiterMap.end()) {
      return iter->second;
    }
    return DelimiterInfo{nullptr, BinaryLocations::Invalid};
  }

private:
  void add(Expression* expr, const BinaryLocations::Span span) {
    assert(startMap.count(span.start) == 0);
    startMap[span.start] = expr;
    assert(endMap.count(span.end) == 0);
    endMap[span.end] = expr;
  }

  void add(Expression* expr,
           const BinaryLocations::DelimiterLocations& delimiter) {
    for (Index i = 0; i < delimiter.size(); i++) {
      if (delimiter[i] != 0) {
        assert(delimiterMap.count(delimiter[i]) == 0);
        delimiterMap[delimiter[i]] = DelimiterInfo{expr, i};
      }
    }
  }
};

// Represents a mapping of addresses to expressions. As with expressions, we
// track both start and end; here, however, "start" means the "start" and
// "declarations" fields in FunctionLocations, and "end" means the two locations
// of one past the end, and one before it which is the "end" opcode that is
// emitted.
struct FuncAddrMap {
  std::unordered_map<BinaryLocation, Function*> startMap, endMap;

  // Construct the map from the binaryLocations loaded from the wasm.
  FuncAddrMap(const Module& wasm) {
    for (auto& func : wasm.functions) {
      startMap[func->funcLocation.start] = func.get();
      startMap[func->funcLocation.declarations] = func.get();
      endMap[func->funcLocation.end - 1] = func.get();
      endMap[func->funcLocation.end] = func.get();
    }
  }

  Function* getStart(BinaryLocation addr) const {
    auto iter = startMap.find(addr);
    if (iter != startMap.end()) {
      return iter->second;
    }
    return nullptr;
  }

  Function* getEnd(BinaryLocation addr) const {
    auto iter = endMap.find(addr);
    if (iter != endMap.end()) {
      return iter->second;
    }
    return nullptr;
  }
};

// Track locations from the original binary and the new one we wrote, so that
// we can update debug positions.
// We track expressions and functions separately, instead of having a single
// big map of (oldAddr) => (newAddr) because of the potentially ambiguous case
// of the final expression in a function: it's end might be identical in offset
// to the end of the function. So we have two different things that map to the
// same offset. However, if the context is "the end of the function" then the
// updated address is the new end of the function, even if the function ends
// with a different instruction now, as the old last instruction might have
// moved or been optimized out.
struct LocationUpdater {
  Module& wasm;
  const BinaryLocations& newLocations;

  AddrExprMap oldExprAddrMap;
  FuncAddrMap oldFuncAddrMap;

  // Map offsets of location list entries in the debug_loc section to the index
  // of their compile unit.
  std::unordered_map<BinaryLocation, size_t> locToUnitMap;

  // Map start of line tables in the debug_line section to their new locations.
  std::unordered_map<BinaryLocation, BinaryLocation> debugLineMap;

  using OldToNew = std::pair<BinaryLocation, BinaryLocation>;

  // Map of compile unit index => old and new base offsets (i.e., in the
  // original binary and in the new one).
  std::unordered_map<size_t, OldToNew> compileUnitBases;

  // TODO: for memory efficiency, we may want to do this in a streaming manner,
  //       binary to binary, without YAML IR.

  LocationUpdater(Module& was