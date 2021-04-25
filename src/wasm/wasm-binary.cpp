/*
 * Copyright 2016 WebAssembly Community Group participants
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

#include <algorithm>
#include <fstream>

#include "ir/eh-utils.h"
#include "ir/module-utils.h"
#include "ir/table-utils.h"
#include "ir/type-updating.h"
#include "support/bits.h"
#include "support/debug.h"
#include "wasm-binary.h"
#include "wasm-debug.h"
#include "wasm-stack.h"

#define DEBUG_TYPE "binary"

namespace wasm {

void WasmBinaryWriter::prepare() {
  // Collect function types and their frequencies. Collect information in each
  // function in parallel, then merge.
  indexedTypes = ModuleUtils::getOptimizedIndexedHeapTypes(*wasm);
  importInfo = wasm::make_unique<ImportInfo>(*wasm);
}

void WasmBinaryWriter::write() {
  writeHeader();

  writeDylinkSection();

  initializeDebugInfo();
  if (sourceMap) {
    writeSourceMapProlog();
  }

  writeTypes();
  writeImports();
  writeFunctionSignatures();
  writeTableDeclarations();
  writeMemories();
  writeTags();
  if (wasm->features.hasStrings()) {
    writeStrings();
  }
  writeGlobals();
  writeExports();
  writeStart();
  writeElementSegments();
  writeDataCount();
  writeFunctions();
  writeDataSegments();
  if (debugInfo || emitModuleName) {
    writeNames();
  }
  if (sourceMap && !sourceMapUrl.empty()) {
    writeSourceMapUrl();
  }
  if (symbolMap.size() > 0) {
    writeSymbolMap();
  }

  if (sourceMap) {
    writeSourceMapEpilog();
  }

#ifdef BUILD_LLVM_DWARF
  // Update DWARF user sections after writing the data they refer to
  // (function bodies), and before writing the user sections themselves.
  if (Debug::hasDWARFSections(*wasm)) {
    Debug::writeDWARFSections(*wasm, binaryLocations);
  }
#endif

  writeLateCustomSections();
  writeFeaturesSection();
}

void WasmBinaryWriter::writeHeader() {
  BYN_TRACE("== writeHeader\n");
  o << int32_t(BinaryConsts::Magic); // magic number \0asm
  o << int32_t(BinaryConsts::Version);
}

int32_t WasmBinaryWriter::writeU32LEBPlaceholder() {
  int32_t ret = o.size();
  o << int32_t(0);
  o << int8_t(0);
  return ret;
}

void WasmBinaryWriter::writeResizableLimits(
  Address initial, Address maximum, bool hasMaximum, bool shared, bool is64) {
  uint32_t flags = (hasMaximum ? (uint32_t)BinaryConsts::HasMaximum : 0U) |
                   (shared ? (uint32_t)BinaryConsts::IsShared : 0U) |
                   (is64 ? (uint32_t)BinaryConsts::Is64 : 0U);
  o << U32LEB(flags);
  if (is64) {
    o << U64LEB(initial);
    if (hasMaximum) {
      o << U64LEB(maximum);
    }
  } else {
    o << U32LEB(initial);
    if (hasMaximum) {
      o << U32LEB(maximum);
    }
  }
}

template<typename T> int32_t WasmBinaryWriter::startSection(T code) {
  o << uint8_t(code);
  if (sourceMap) {
    sourceMapLocationsSizeAtSectionStart = sourceMapLocations.size();
  }
  binaryLocationsSizeAtSectionStart = binaryLocations.expressions.size();
  return writeU32LEBPlaceholder(); // section size to be filled in later
}

void WasmBinaryWriter::finishSection(int32_t start) {
  // section size does not include the reserved bytes of the size field itself
  int32_t size = o.size() - start - MaxLEB32Bytes;
  auto sizeFieldSize = o.writeAt(start, U32LEB(size));
  // We can move things back if the actual LEB for the size doesn't use the
  // maximum 5 bytes. In that case we need to adjust offsets after we move
  // things backwards.
  auto adjustmentForLEBShrinking = MaxLEB32Bytes - sizeFieldSize;
  if (adjustmentForLEBShrinking) {
    // we can save some room, nice
    assert(sizeFieldSize < MaxLEB32Bytes);
    std::move(&o[start] + MaxLEB32Bytes,
              &o[start] + MaxLEB32Bytes + size,
              &o[start] + sizeFieldSize);
    o.resize(o.size() - adjustmentForLEBShrinking);
    if (sourceMap) {
      for (auto i = sourceMapLocationsSizeAtSectionStart;
           i < sourceMapLocations.size();
           ++i) {
        sourceMapLocations[i].first -= adjustmentForLEBShrinking;
      }
    }
  }

  if (binaryLocationsSizeAtSectionStart != binaryLocations.expressions.size()) {
    // We added the binary locations, adjust them: they must be relative
    // to the code section.
    assert(binaryLocationsSizeAtSectionStart == 0);
    // The section type byte is right before the LEB for the size; we want
    // offsets that are relative to the body, which is after that section type
    // byte and the the size LEB.
    auto body = start + sizeFieldSize;
    // Offsets are relative to the body of the code section: after the
    // section type byte and the size.
    // Everything was moved by the adjustment, track that. After this,
    // we are at the right absolute address.
    // We are relative to the section start.
    auto totalAdjustment = adjustmentForLEBShrinking + body;
    for (auto& [_, locations] : binaryLocations.expressions) {
      locations.start -= totalAdjustment;
      locations.end -= totalAdjustment;
    }
    for (auto& [_, locations] : binaryLocations.functions) {
      locations.start -= totalAdjustment;
      locations.declarations -= totalAdjustment;
      locations.end -= totalAdjustment;
    }
    for (auto& [_, locations] : binaryLocations.delimiters) {
      for (auto& item : locations) {
        item -= totalAdjustment;
      }
    }
  }
}

int32_t WasmBinaryWriter::startSubsection(
  BinaryConsts::CustomSections::Subsection code) {
  return startSection(code);
}

void WasmBinaryWriter::finishSubsection(int32_t start) { finishSection(start); }

void WasmBinaryWriter::writeStart() {
  if (!wasm->start.is()) {
    return;
  }
  BYN_TRACE("== writeStart\n");
  auto start = startSection(BinaryConsts::Section::Start);
  o << U32LEB(getFunctionIndex(wasm->start.str));
  finishSection(start);
}

void WasmBinaryWriter::writeMemories() {
  if (importInfo->getNumDefinedMemories() == 0) {
    return;
  }
  BYN_TRACE("== writeMemories\n");
  auto start = startSection(BinaryConsts::Section::Memory);
  auto num = importInfo->getNumDefinedMemories();
  o << U32LEB(num);
  ModuleUtils::iterDefinedMemories(*wasm, [&](Memory* memory) {
    writeResizableLimits(memory->initial,
                         memory->max,
                         memory->hasMax(),
                         memory->shared,
                         memory->is64());
  });
  finishSection(start);
}

void WasmBinaryWriter::writeTypes() {
  if (indexedTypes.types.size() == 0) {
    return;
  }
  // Count the number of recursion groups, which is the number of elements in
  // the type section. With nominal typing there is always one group and with
  // equirecursive typing there is one group per type.
  size_t numGroups = 0;
  // MVP types are structural and do not use recursion groups.
  TypeSystem typeSystem = getTypeSystem();
  if (!wasm->features.hasGC()) {
    typeSystem = TypeSystem::Isorecursive;
  }
  switch (typeSystem) {
    case TypeSystem::Nominal:
      numGroups = 1;
      break;
    case TypeSystem::Isorecursive: {
      std::optional<RecGroup> lastGroup;
      for (auto type : indexedTypes.types) {
        auto currGroup = type.getRecGroup();
        numGroups += lastGroup != currGroup;
        lastGroup = currGroup;
      }
    }
  }
  BYN_TRACE("== writeTypes\n");
  auto start = startSection(BinaryConsts::Section::Type);
  o << U32LEB(numGroups);
  if (typeSystem == TypeSystem::Nominal) {
    // The nominal recursion group contains every type.
    o << S32LEB(BinaryConsts::EncodedType::Rec)
      << U32LEB(indexedTypes.types.size());
  }
  std::optional<RecGroup> lastGroup = std::nullopt;
  for (Index i = 0; i < indexedTypes.types.size(); ++i) {
    auto type = indexedTypes.types[i];
    // Check whether we need to start a new recursion group. Recursion groups of
    // size 1 are implicit, so only emit a group header for larger groups. This
    // gracefully handles non-isorecursive type systems, which only have groups
    // of size 1 internally (even though nominal types are emitted as a single
    // large group).
    auto currGroup = type.getRecGroup();
    if (lastGroup != currGroup && currGroup.size() > 1) {
      o << S32LEB(BinaryConsts::EncodedType::Rec) << U32LEB(currGroup.size());
    }
    lastGroup = currGroup;
    // Emit the type definition.
    BYN_TRACE("write " << type << std::endl);
    if (auto super = type.getSuperType()) {
      // Subtype constructor and vector of 1 supertype.
      o << S32LEB(BinaryConsts::EncodedType::Sub) << U32LEB(1);
      writeHeapType(*super);
    }
    if (type.isSignature()) {
      o << S32LEB(BinaryConsts::EncodedType::Func);
      auto sig = type.getSignature();
      for (auto& sigType : {sig.params, sig.results}) {
        o << U32LEB(sigType.size());
        for (const auto& type : sigType) {
          writeType(type);
        }
      }
    } else if (type.isStruct()) {
      o << S32LEB(BinaryConsts::EncodedType::Struct);
      auto fields = type.getStruct().fields;
      o << U32LEB(fields.size());
      for (const auto& field : fields) {
        writeField(field);
      }
    } else if (type.isArray()) {
      o << S32LEB(BinaryConsts::EncodedType::Array);
      writeField(type.getArray().element);
    } else {
      WASM_UNREACHABLE("TODO GC type writing");
    }
  }
  finishSection(start);
}

void WasmBinaryWriter::writeImports() {
  auto num = importInfo->getNumImports();
  if (num == 0) {
    return;
  }
  BYN_TRACE("== writeImports\n");
  auto start = startSection(BinaryConsts::Section::Import);
  o << U32LEB(num);
  auto writeImportHeader = [&](Importable* import) {
    writeInlineString(import->module.str);
    writeInlineString(import->base.str);
  };
  ModuleUtils::iterImportedFunctions(*wasm, [&](Function* func) {
    BYN_TRACE("write one function\n");
    writeImportHeader(func);
    o << U32LEB(int32_t(ExternalKind::Function));
    o << U32LEB(getTypeIndex(func->type));
  });
  ModuleUtils::iterImportedGlobals(*wasm, [&](Global* global) {
    BYN_TRACE("write one global\n");
    writeImportHeader(global);
    o << U32LEB(int32_t(ExternalKind::Global));
    writeType(global->type);
    o << U32LEB(global->mutable_);
  });
  ModuleUtils::iterImportedTags(*wasm, [&](Tag* tag) {
    BYN_TRACE("write one tag\n");
    writeImportHeader(tag);
    o << U32LEB(int32_t(ExternalKind::Tag));
    o << uint8_t(0); // Reserved 'attribute' field. Always 0.
    o << U32LEB(getTypeIndex(tag->sig));
  });
  ModuleUtils::iterImportedMemories(*wasm, [&](Memory* memory) {
    BYN_TRACE("write one memory\n");
    writeImportHeader(memory);
    o << U32LEB(int32_t(ExternalKind::Memory));
    writeResizableLimits(memory->initial,
                         memory->max,
                         memory->hasMax(),
                         memory->shared,
                         memory->is64());
  });
  ModuleUtils::iterImportedTables(*wasm, [&](Table* table) {
    BYN_TRACE("write one table\n");
    writeImportHeader(table);
    o << U32LEB(int32_t(ExternalKind::Table));
    writeType(table->type);
    writeResizableLimits(table->initial,
                         table->max,
                         table->hasMax(),
                         /*shared=*/false,
                         /*is64*/ false);
  });
  finishSection(start);
}

void WasmBinaryWriter::writeFunctionSignatures() {
  if (importInfo->getNumDefinedFunctions() == 0) {
    return;
  }
  BYN_TRACE("== writeFunctionSignatures\n");
  auto start = startSection(BinaryConsts::Section::Function);
  o << U32LEB(importInfo->getNumDefinedFunctions());
  ModuleUtils::iterDefinedFunctions(*wasm, [&](Function* func) {
    BYN_TRACE("write one\n");
    o << U32LEB(getTypeIndex(func->type));
  });
  finishSection(start);
}

void WasmBinaryWriter::writeExpression(Expression* curr) {
  BinaryenIRToBinaryWriter(*this, o).visit(curr);
}

void WasmBinaryWriter::writeFunctions() {
  if (importInfo->getNumDefinedFunctions() == 0) {
    return;
  }
  BYN_TRACE("== writeFunctions\n");
  auto sectionStart = startSection(BinaryConsts::Section::Code);
  o << U32LEB(importInfo->getNumDefinedFunctions());
  bool DWARF = Debug::hasDWARFSections(*getModule());
  ModuleUtils::iterDefinedFunctions(*wasm, [&](Function* func) {
    assert(binaryLocationTrackedExpressionsForFunc.empty());
    size_t sourceMapLocationsSizeAtFunctionStart = sourceMapLocations.size();
    BYN_TRACE("write one at" << o.size() << std::endl);
    size_t sizePos = writeU32LEBPlaceholder();
    size_t start = o.size();
    BYN_TRACE("writing" << func->name << std::endl);
    // Emit Stack IR if present, and if we can
    if (func->stackIR && !sourceMap && !DWARF) {
      BYN_TRACE("write Stack IR\n");
      StackIRToBinaryWriter writer(*this, o, func);
      writer.write();
      if (debugInfo) {
        funcMappedLocals[func->name] = std::move(writer.getMappedLocals());
      }
    } else {
      BYN_TRACE("write Binaryen IR\n");
      BinaryenIRToBinaryWriter writer(*this, o, func, sourceMap, DWARF);
      writer.write();
      if (debugInfo) {
        funcMappedLocals[func->name] = std::move(writer.getMappedLocals());
      }
    }
    size_t size = o.size() - start;
    assert(size <= std::numeric_limits<uint32_t>::max());
    BYN_TRACE("body size: " << size << ", writing at " << sizePos
                            << ", next starts at " << o.size() << "\n");
    auto sizeFieldSize = o.writeAt(sizePos, U32LEB(size));
    // We can move things back if the actual LEB for the size doesn't use the
    // maximum 5 bytes. In that case we need to adjust offsets after we move
    // things backwards.
    auto adjustmentForLEBShrinking = MaxLEB32Bytes - sizeFieldSize;
    if (adjustmentForLEBShrinking) {
      // we can save some room, nice
      assert(sizeFieldSize < MaxLEB32Bytes);
      std::move(&o[start], &o[start] + size, &o[sizePos] + sizeFieldSize);
      o.resize(o.size() - adjustmentForLEBShrinking);
      if (sourceMap) {
        for (auto i = sourceMapLocationsSizeAtFunctionStart;
             i < sourceMapLocations.size();
             ++i) {
          sourceMapLocations[i].first -= adjustmentForLEBShrinking;
        }
      }
      for (auto* curr : binaryLocationTrackedExpressionsForFunc) {
        // We added the binary locations, adjust them: they must be relative
        // to the code section.
        auto& span = binaryLocations.expressions[curr];
        span.start -= adjustmentForLEBShrinking;
        span.end -= adjustmentForLEBShrinking;
        auto iter = binaryLocations.delimiters.find(curr);
        if (iter != binaryLocations.delimiters.end()) {
          for (auto& item : iter->second) {
            item -= adjustmentForLEBShrinking;
          }
        }
      }
    }
    if (!binaryLocationTrackedExpressionsForFunc.empty()) {
      binaryLocations.functions[func] = BinaryLocations::FunctionLocations{
        BinaryLocation(sizePos),
        BinaryLocation(start - adjustmentForLEBShrinking),
        BinaryLocation(o.size())};
    }
    tableOfContents.functionBodies.emplace_back(
      func->name, sizePos + sizeFieldSize, size);
    binaryLocationTrackedExpressionsForFunc.clear();

    if (func->getParams().size() > WebLimitations::MaxFunctionParams) {
      std::cerr << "Some VMs may not accept this binary because it has a large "
                << "number of parameters in function " << func->name << ".\n";
    }
  });
  finishSection(sectionStart);
}

void WasmBinaryWriter::writeStrings() {
  assert(wasm->features.hasStrings());

  // Scan the entire wasm to find the relevant strings.
  // To find all the string literals we must scan all the code.
  using StringSet = std::unordered_set<Name>;

  struct StringWalker : public PostWalker<StringWalker> {
    StringSet& strings;

    StringWalker(StringSet& strings) : strings(strings) {}

    void visitStringConst(StringConst* curr) { strings.insert(curr->string); }
  };

  ModuleUtils::ParallelFunctionAnalysis<StringSet> analysis(
    *wasm, [&](Function* func, StringSet& strings) {
      if (!func->imported()) {
        StringWalker(strings).walk(func->body);
      }
    });

  // Also walk the global module code (for simplicity, also add it to the
  // function map, using a "function" key of nullptr).
  auto& globalStrings = analysis.map[nullptr];
  StringWalker(globalStrings).walkModuleCode(wasm);

  // Generate the indexes from the combined set of necessary strings,
  // which we sort for determinism.
  StringSet allStrings;
  for (auto& [func, strings] : analysis.map) {
    for (auto& string : strings) {
      allStrings.insert(string);
    }
  }
  std::vector<Name> sorted;
  for (auto& string : allStrings) {
    sorted.push_back(string);
  }
  std::sort(sorted.begin(), sorted.end());
  for (Index i = 0; i < sorted.size(); i++) {
    stringIndexes[sorted[i]] = i;
  }

  auto num = sorted.size();
  if (num == 0) {
    return;
  }

  auto start = startSection(BinaryConsts::Section::Strings);

  // Placeholder for future use in the spec.
  o << U32LEB(0);

  // The number of strings and then their contents.
  o << U32LEB(num);
  for (auto& string : sorted) {
    writeInlineString(string.str);
  }

  finishSection(start);
}

void WasmBinaryWriter::writeGlobals() {
  if (importInfo->getNumDefinedGlobals() == 0) {
    return;
  }
  BYN_TRACE("== writeglobals\n");
  auto start = startSection(BinaryConsts::Section::Global);
  // Count and emit the total number of globals after tuple globals have been
  // expanded into their constituent parts.
  Index num = 0;
  ModuleUtils::iterDefinedGlobals(
    *wasm, [&num](Global* global) { num += global->type.size(); });
  o << U32LEB(num);
  ModuleUtils::iterDefinedGlobals(*wasm, [&](Global* global) {
    BYN_TRACE("write one\n");
    size_t i = 0;
    for (const auto& t : global->type) {
      writeType(t);
      o << U32LEB(global->mutable_);
      if (global->type.size() == 1) {
        writeExpression(global->init);
      } else {
        writeExpression(global->init->cast<TupleMake>()->operands[i]);
      }
      o << int8_t(BinaryConsts::End);
      ++i;
    }
  });
  finishSection(start);
}

void WasmBinaryWriter::writeExports() {
  if (wasm->exports.size() == 0) {
    return;
  }
  BYN_TRACE("== writeexports\n");
  auto start = startSection(BinaryConsts::Section::Export);
  o << U32LEB(wasm->exports.size());
  for (auto& curr : wasm->exports) {
    BYN_TRACE("write one\n");
    writeInlineString(curr->name.str);
    o << U32LEB(int32_t(curr->kind));
    switch (curr->kind) {
      case ExternalKind::Function:
        o << U32LEB(getFunctionIndex(curr->value));
        break;
      case ExternalKind::Table:
        o << U32LEB(getTableIndex(curr->value));
        break;
      case ExternalKind::Memory:
        o << U32LEB(getMemoryIndex(curr->value));
        break;
      case ExternalKind::Global:
        o << U32LEB(getGlobalIndex(curr->value));
        break;
      case ExternalKind::Tag:
        o << U32LEB(getTagIndex(curr->value));
        break;
      default:
        WASM_UNREACHABLE("unexpected extern kind");
    }
  }
  finishSection(start);
}

void WasmBinaryWriter::writeDataCount() {
  if (!wasm->features.hasBulkMemory() || !wasm->dataSegments.size()) {
    return;
  }
  auto start = startSection(BinaryConsts::Section::DataCount);
  o << U32LEB(wasm->dataSegments.size());
  finishSection(start);
}

void WasmBinaryWriter::writeDataSegments() {
  if (wasm->dataSegments.size() == 0) {
    return;
  }
  if (wasm->dataSegments.size() > WebLimitations::MaxDataSegments) {
    std::cerr << "Some VMs may not accept this binary because it has a large "
              << "number of data segments. Run the limit-segments pass to "
              << "merge segments.\n";
  }
  auto start = startSection(BinaryConsts::Section::Data);
  o << U32LEB(wasm->dataSegments.size());
  for (auto& segment : wasm->dataSegments) {
    uint32_t flags = 0;
    if (segment->isPassive) {
      flags |= BinaryConsts::IsPassive;
    }
    o << U32LEB(flags);
    if (!segment->isPassive) {
      writeExpression(segment->offset);
      o << int8_t(BinaryConsts::End);
    }
    writeInlineBuffer(segment->data.data(), segment->data.size());
  }
  finishSection(start);
}

uint32_t WasmBinaryWriter::getFunctionIndex(Name name) const {
  auto it = indexes.functionIndexes.find(name);
  assert(it != indexes.functionIndexes.end());
  return it->second;
}

uint32_t WasmBinaryWriter::getTableIndex(Name name) const {
  auto it = indexes.tableIndexes.find(name);
  assert(it != indexes.tableIndexes.end());
  return it->second;
}

uint32_t WasmBinaryWriter::getMemoryIndex(Name name) const {
  auto it = indexes.memoryIndexes.find(name);
  assert(it != indexes.memoryIndexes.end());
  return it->second;
}

uint32_t WasmBinaryWriter::getGlobalIndex(Name name) const {
  auto it = indexes.globalIndexes.find(name);
  assert(it != indexes.globalIndexes.end());
  return it->second;
}

uint32_t WasmBinaryWriter::getTagIndex(Name name) const {
  auto it = indexes.tagIndexes.find(name);
  assert(it != indexes.tagIndexes.end());
  return it->second;
}

uint32_t WasmBinaryWriter::getTypeIndex(HeapType type) const {
  auto it = indexedTypes.indices.find(type);
#ifndef NDEBUG
  if (it == indexedTypes.indices.end()) {
    std::cout << "Missing type: " << type << '\n';
    assert(0);
  }
#endif
  return it->second;
}

uint32_t WasmBinaryWriter::getStringIndex(Name string) const {
  auto it = stringIndexes.find(string);
  assert(it != stringIndexes.end());
  return it->second;
}

void WasmBinaryWriter::writeTableDeclarations() {
  if (importInfo->getNumDefinedTables() == 0) {
    // std::cerr << std::endl << "(WasmBinaryWriter::writeTableDeclarations) No
    // defined tables found. skipping" << std::endl;
    return;
  }
  BYN_TRACE("== writeTableDeclarations\n");
  auto start = startSection(BinaryConsts::Section::Table);
  auto num = importInfo->getNumDefinedTables();
  o << U32LEB(num);
  ModuleUtils::iterDefinedTables(*wasm, [&](Table* table) {
    writeType(table->type);
    writeResizableLimits(table->initial,
                         table->max,
                         table->hasMax(),
                         /*shared=*/false,
                         /*is64*/ false);
  });
  finishSection(start);
}

void WasmBinaryWriter::writeElementSegments() {
  size_t elemCount = wasm->elementSegments.size();
  auto needingElemDecl = TableUtils::getFunctionsNeedingElemDeclare(*wasm);
  if (!needingElemDecl.empty()) {
    elemCount++;
  }
  if (elemCount == 0) {
    return;
  }

  BYN_TRACE("== writeElementSegments\n");
  auto start = startSection(BinaryConsts::Section::Element);
  o << U32LEB(elemCount);

  Type funcref = Type(HeapType::func, Nullable);
  for (auto& segment : wasm->elementSegments) {
    Index tableIdx = 0;

    bool isPassive = segment->table.isNull();
    // If the segment is MVP, we can use the shorter form.
    bool usesExpressions = TableUtils::usesExpressions(segment.get(), wasm);

    // The table index can and should be elided for active segments of table 0
    // when table 0 has type funcref. This was the only type of segment
    // supported by the MVP, which also did not support table indices in the
    // segment encoding.
    bool hasTableIndex = false;
    if (!isPassive) {
      tableIdx = getTableIndex(segment->table);
      hasTableIndex =
        tableIdx > 0 || wasm->getTable(segment->table)->type != funcref;
    }

    uint32_t flags = 0;
    if (usesExpressions) {
      flags |= BinaryConsts::UsesExpressions;
    }
    if (isPassive) {
      flags |= BinaryConsts::IsPassive;
    } else if (hasTableIndex) {
      flags |= BinaryConsts::HasIndex;
    }

    o << U32LEB(flags);
    if (!isPassive) {
      if (hasTableIndex) {
        o << U32LEB(tableIdx);
      }
      writeExpression(segment->offset);
      o << int8_t(BinaryConsts::End);
    }

    if (isPassive || hasTableIndex) {
      if (usesExpressions) {
        // elemType
        writeType(segment->type);
      } else {
        // MVP elemKind of funcref
        o << U32LEB(0);
      }
    }
    o << U32LEB(segment->data.size());
    if (usesExpressions) {
      for (auto* item : segment->data) {
        writeExpression(item);
        o << int8_t(BinaryConsts::End);
      }
    } else {
      for (auto& item : segment->data) {
        // We've ensured that all items are ref.func.
        auto& name = item->cast<RefFunc>()->func;
        o << U32LEB(getFunctionIndex(name));
      }
    }
  }

  if (!needingElemDecl.empty()) {
    o << U32LEB(BinaryConsts::IsPassive | BinaryConsts::IsDeclarative);
    o << U32LEB(0); // type (indicating funcref)
    o << U32LEB(needingElemDecl.size());
    for (auto name : needingElemDecl) {
      o << U32LEB(indexes.functionIndexes[name]);
    }
  }

  finishSection(start);
}

void WasmBinaryWriter::writeTags() {
  if (importInfo->getNumDefinedTags() == 0) {
    return;
  }
  BYN_TRACE("== writeTags\n");
  auto start = startSection(BinaryConsts::Section::Tag);
  auto num = importInfo->getNumDefinedTags();
  o << U32LEB(num);
  ModuleUtils::iterDefinedTags(*wasm, [&](Tag* tag) {
    BYN_TRACE("write one\n");
    o << uint8_t(0); // Reserved 'attribute' field. Always 0.
    o << U32LEB(getTypeIndex(tag->sig));
  });

  finishSection(start);
}

void WasmBinaryWriter::writeNames() {
  BYN_TRACE("== writeNames\n");
  auto start = startSection(BinaryConsts::Section::Custom);
  writeInlineString(BinaryConsts::CustomSections::Name);

  // module name
  if (emitModuleName && wasm->name.is()) {
    auto substart =
      startSubsection(BinaryConsts::CustomSections::Subsection::NameModule);
    writeEscapedName(wasm->name.str);
    finishSubsection(substart);
  }

  if (!debugInfo) {
    // We were only writing the module name.
    finishSection(start);
    return;
  }

  // function names
  {
    auto substart =
      startSubsection(BinaryConsts::CustomSections::Subsection::NameFunction);
    o << U32LEB(indexes.functionIndexes.size());
    Index emitted = 0;
    auto add = [&](Function* curr) {
      o << U32LEB(emitted);
      writeEscapedName(curr->name.str);
      emitted++;
    };
    ModuleUtils::iterImportedFunctions(*wasm, add);
    ModuleUtils::iterDefinedFunctions(*wasm, add);
    assert(emitted == indexes.functionIndexes.size());
    finishSubsection(substart);
  }

  // local names
  {
    // Find all functions with at least one local name and only emit the
    // subsection if there is at least one.
    std::vector<std::pair<Index, Function*>> functionsWithLocalNames;
    Index checked = 0;
    auto check = [&](Function* curr) {
      auto numLocals = curr->getNumLocals();
      for (Index i = 0; i < numLocals; ++i) {
        if (curr->hasLocalName(i)) {
          functionsWithLocalNames.push_back({checked, curr});
          break;
        }
      }
      checked++;
    };
    ModuleUtils::iterImportedFunctions(*wasm, check);
    ModuleUtils::iterDefinedFunctions(*wasm, check);
    assert(checked == indexes.functionIndexes.size());
    if (functionsWithLocalNames.size() > 0) {
      // Otherwise emit those functions but only include locals with a name.
      auto substart =
        startSubsection(BinaryConsts::CustomSections::Subsection::NameLocal);
      o << U32LEB(functionsWithLocalNames.size());
      Index emitted = 0;
      for (auto& [index, func] : functionsWithLocalNames) {
        // Pairs of (local index in IR, name).
        std::vector<std::pair<Index, Name>> localsWithNames;
        auto numLocals = func->getNumLocals();
        for (Index indexInFunc = 0; indexInFunc < numLocals; ++indexInFunc) {
          if (func->hasLocalName(indexInFunc)) {
            Index indexInBinary;
            auto iter = funcMappedLocals.find(func->name);
            if (iter != funcMappedLocals.end()) {
              // TODO: handle multivalue
              indexInBinary = iter->second[{indexInFunc, 0}];
            } else {
              // No data on funcMappedLocals. That is only possible if we are an
              // imported function, where there are no locals to map, and in
              // that case the index is unchanged anyhow: parameters always have
              // the same index, they are not mapped in any way.
              assert(func->imported());
              indexInBinary = indexInFunc;
            }
            localsWithNames.push_back(
              {indexInBinary, func->getLocalName(indexInFunc)});
          }
        }
        assert(localsWithNames.size());
        std::sort(localsWithNames.begin(), localsWithNames.end());
        o << U32LEB(index);
        o << U32LEB(localsWithNames.size());
        for (auto& [indexInBinary, name] : localsWithNames) {
          o << U32LEB(indexInBinary);
          writeEscapedName(name.str);
        }
        emitted++;
      }
      assert(emitted == functionsWithLocalNames.size());
      finishSubsection(substart);
    }
  }

  // type names
  {
    std::vector<HeapType> namedTypes;
    for (auto& [type, _] : indexedTypes.indices) {
      if (wasm->typeNames.count(type) && wasm->typeNames[type].name.is()) {
        namedTypes.push_back(type);
      }
    }
    if (!namedTypes.empty()) {
      auto substart =
        startSubsection(BinaryConsts::CustomSections::Subsection::NameType);
      o << U32LEB(namedTypes.size());
      for (auto type : namedTypes) {
        o << U32LEB(indexedTypes.indices[type]);
        writeEscapedName(wasm->typeNames[type].name.str);
      }
      finishSubsection(substart);
    }
  }

  // table names
  {
    std::vector<std::pair<Index, Table*>> tablesWithNames;
    Index checked = 0;
    auto check = [&](Table* curr) {
      if (curr->hasExplicitName) {
        tablesWithNames.push_back({checked, curr});
      }
      checked++;
    };
    ModuleUtils::iterImportedTables(*wasm, check);
    ModuleUtils::iterDefinedTables(*wasm, check);
    assert(checked == indexes.tableIndexes.size());

    if (tablesWithNames.size() > 0) {
      auto substart =
        startSubsection(BinaryConsts::CustomSections::Subsection::NameTable);
      o << U32LEB(tablesWithNames.size());

      for (auto& [index, table] : tablesWithNames) {
        o << U32LEB(index);
        writeEscapedName(table->name.str);
      }

      finishSubsection(substart);
    }
  }

  // memory names
  {
    std::vector<std::pair<Index, Memory*>> memoriesWithNames;
    Index checked = 0;
    auto check = [&](Memory* curr) {
      if (curr->hasExplicitName) {
        memoriesWithNames.push_back({checked, curr});
      }
      checked++;
    };
    ModuleUtils::iterImportedMemories(*wasm, check);
    ModuleUtils::iterDefinedMemories(*wasm, check);
    assert(checked == indexes.memoryIndexes.size());
    if (memoriesWithNames.size() > 0) {
      auto substart =
        startSubsection(BinaryConsts::CustomSections::Subsection::NameMemory);
      o << U32LEB(memoriesWithNames.size());
      for (auto& [index, memory] : memoriesWithNames) {
        o << U32LEB(index);
        writeEscapedName(memory->name.str);
      }
      finishSubsection(substart);
    }
  }

  // global names
  {
    std::vector<std::pair<Index, Global*>> globalsWithNames;
    Index checked = 0;
    auto check = [&](Global* curr) {
      if (curr->hasExplicitName) {
        globalsWithNames.push_back({checked, curr});
      }
      checked++;
    };
    ModuleUtils::iterImportedGlobals(*wasm, check);
    ModuleUtils::iterDefinedGlobals(*wasm, check);
    assert(checked == indexes.globalIndexes.size());
    if (globalsWithNames.size() > 0) {
      auto substart =
        startSubsection(BinaryConsts::CustomSections::Subsection::NameGlobal);
      o << U32LEB(globalsWithNames.size());
      for (auto& [index, global] : globalsWithNames) {
        o << U32LEB(index);
        writeEscapedName(global->name.str);
      }
      finishSubsection(substart);
    }
  }

  // elem segment names
  {
    std::vector<std::pair<Index, ElementSegment*>> elemsWithNames;
    Index checked = 0;
    for (auto& curr : wasm->elementSegments) {
      if (curr->hasExplicitName) {
        elemsWithNames.push_back({checked, curr.get()});
      }
      checked++;
    }
    assert(checked == indexes.elemIndexes.size());

    if (elemsWithNames.size() > 0) {
      auto substart =
        startSubsection(BinaryConsts::CustomSections::Subsection::NameElem);
      o << U32LEB(elemsWithNames.size());

      for (auto& [index, elem] : elemsWithNames) {
        o << U32LEB(index);
        writeEscapedName(elem->name.str);
      }

      finishSubsection(substart);
    }
  }

  // data segment names
  if (!wasm->memories.empty()) {
    Index count = 0;
    for (auto& seg : wasm->dataSegments) {
      if (seg->hasExplicitName) {
        count++;
      }
    }

    if (count) {
      auto substart =
        startSubsection(BinaryConsts::CustomSections::Subsection::NameData);
      o << U32LEB(count);
      for (Index i = 0; i < wasm->dataSegments.size(); i++) {
        auto& seg = wasm->dataSegments[i];
        if (seg->name.is()) {
          o << U32LEB(i);
          writeEscapedName(seg->name.str);
        }
      }
      finishSubsection(substart);
    }
  }

  // TODO: label, type, and element names
  // see: https://github.com/WebAssembly/extended-name-section

  // GC field names
  if (wasm->features.hasGC()) {
    std::vector<HeapType> relevantTypes;
    for (auto& type : indexedTypes.types) {
      if (type.isStruct() && wasm->typeNames.count(type) &&
          !wasm->typeNames[type].fieldNames.empty()) {
        relevantTypes.push_back(type);
      }
    }
    if (!relevantTypes.empty()) {
      auto substart =
        startSubsection(BinaryConsts::CustomSections::Subsection::NameField);
      o << U32LEB(relevantTypes.size());
      for (Index i = 0; i < relevantTypes.size(); i++) {
        auto type = relevantTypes[i];
        o << U32LEB(indexedTypes.indices[type]);
        std::unordered_map<Index, Name>& fieldNames =
          wasm->typeNames.at(type).fieldNames;
        o << U32LEB(fieldNames.size());
        for (auto& [index, name] : fieldNames) {
          o << U32LEB(index);
          writeEscapedName(name.str);
        }
      }
      finishSubsection(substart);
    }
  }

  // tag names
  if (!wasm->tags.empty()) {
    Index count = 0;
    for (auto& tag : wasm->tags) {
      if (tag->hasExplicitName) {
        count++;
      }
    }

    if (count) {
      auto substart =
        startSubsection(BinaryConsts::CustomSections::Subsection::NameTag);
      o << U32LEB(count);
      for (Index i = 0; i < wasm->tags.size(); i++) {
        auto& tag = wasm->tags[i];
        if (tag->hasExplicitName) {
          o << U32LEB(i);
          writeEscapedName(tag->name.str);
        }
      }
      finishSubsection(substart);
    }
  }

  finishSection(start);
}

void WasmBinaryWriter::writeSourceMapUrl() {
  BYN_TRACE("== writeSourceMapUrl\n");
  auto start = startSection(BinaryConsts::Section::Custom);
  writeInlineString(BinaryConsts::CustomSections::SourceMapUrl);
  writeInlineString(sourceMapUrl.c_str());
  finishSection(start);
}

void WasmBinaryWriter::writeSymbolMap() {
  std::ofstream file(symbolMap);
  auto write = [&](Function* func) {
    file << getFunctionIndex(func->name) << ":" << func->name.str << std::endl;
  };
  ModuleUtils::iterImportedFunctions(*wasm, write);
  ModuleUtils::iterDefinedFunctions(*wasm, write);
  file.close();
}

void WasmBinaryWriter::initializeDebugInfo() {
  lastDebugLocation = {0, /* lineNumber = */ 1, 0};
}

void WasmBinaryWriter::writeSourceMapProlog() {
  *sourceMap << "{\"version\":3,\"sources\":[";
  for (size_t i = 0; i < wasm->debugInfoFileNames.size(); i++) {
    if (i > 0) {
      *sourceMap << ",";
    }
    // TODO respect JSON string encoding, e.g. quotes and control chars.
    *sourceMap << "\"" << wasm->debugInfoFileNames[i] << "\"";
  }
  *sourceMap << "],\"names\":[],\"mappings\":\"";
}

static void writeBase64VLQ(std::ostream& out, int32_t n) {
  uint32_t value = n >= 0 ? n << 1 : ((-n) << 1) | 1;
  while (1) {
    uint32_t digit = value & 0x1F;
    value >>= 5;
    if (!value) {
      // last VLQ digit -- base64 codes 'A'..'Z', 'a'..'f'
      out << char(digit < 26 ? 'A' + digit : 'a' + digit - 26);
      break;
    }
    // more VLG digit will follow -- add continuation bit (0x20),
    // base64 codes 'g'..'z', '0'..'9', '+', '/'
    out << char(digit < 20
                  ? 'g' + digit
                  : digit < 30 ? '0' + digit - 20 : digit == 30 ? '+' : '/');
  }
}

void WasmBinaryWriter::writeSourceMapEpilog() {
  // write source map entries
  size_t lastOffset = 0;
  Function::DebugLocation lastLoc = {0, /* lineNumber = */ 1, 0};
  for (const auto& [offset, loc] : sourceMapLocations) {
    if (lastOffset > 0) {
      *sourceMap << ",";
    }
    writeBase64VLQ(*sourceMap, int32_t(offset - lastOffset));
    writeBase64VLQ(*sourceMap, int32_t(loc->fileIndex - lastLoc.fileIndex));
    writeBase64VLQ(*sourceMap, int32_t(loc->lineNumber - lastLoc.lineNumber));
    writeBase64VLQ(*sourceMap,
                   int32_t(loc->columnNumber - lastLoc.columnNumber));
    lastLoc = *loc;
    lastOffset = offset;
  }
  *sourceMap << "\"}";
}

void WasmBinaryWriter::writeLateCustomSections() {
  for (auto& section : wasm->customSections) {
    if (section.name != BinaryConsts::CustomSections::Dylink) {
      writeCustomSection(section);
    }
  }
}

void WasmBinaryWriter::writeCustomSection(const CustomSection& section) {
  auto start = startSection(BinaryConsts::Custom);
  writeInlineString(section.name.c_str());
  for (size_t i = 0; i < section.data.size(); i++) {
    o << uint8_t(section.data[i]);
  }
  finishSection(start);
}

void WasmBinaryWriter::writeFeaturesSection() {
  if (!wasm->hasFeaturesSection || wasm->features.isMVP()) {
    return;
  }

  // TODO(tlively): unify feature names with rest of toolchain and use
  // FeatureSet::toString()
  auto toString = [](FeatureSet::Feature f) {
    switch (f) {
      case FeatureSet::Atomics:
        return BinaryConsts::CustomSections::AtomicsFeature;
      case FeatureSet::MutableGlobals:
        return BinaryConsts::CustomSections::MutableGlobalsFeature;
      case FeatureSet::TruncSat:
        return BinaryConsts::CustomSections::TruncSatFeature;
      case FeatureSet::SIMD:
        return BinaryConsts::CustomSections::SIMD128Feature;
      case FeatureSet::BulkMemory:
        return BinaryConsts::CustomSections::BulkMemoryFeature;
      case FeatureSet::SignExt:
        return BinaryConsts::CustomSections::SignExtFeature;
      case FeatureSet::ExceptionHandling:
        return BinaryConsts::CustomSections::ExceptionHandlingFeature;
      case FeatureSet::TailCall:
        return BinaryConsts::CustomSections::TailCallFeature;
      case FeatureSet::ReferenceTypes:
        return BinaryConsts::CustomSections::ReferenceTypesFeature;
      case FeatureSet::Multivalue:
        return BinaryConsts::CustomSections::MultivalueFeature;
      case FeatureSet::GC:
        return BinaryConsts::CustomSections::GCFeature;
      case FeatureSet::Memory64:
        return BinaryConsts::CustomSections::Memory64Feature;
      case FeatureSet::RelaxedSIMD:
        return BinaryConsts::CustomSections::RelaxedSIMDFeature;
      case FeatureSet::ExtendedConst:
        return BinaryConsts::CustomSections::ExtendedConstFeature;
      case FeatureSet::Strings:
        return BinaryConsts::CustomSections::StringsFeature;
      case FeatureSet::MultiMemories:
        return BinaryConsts::CustomSections::MultiMemoriesFeature;
      default:
        WASM_UNREACHABLE("unexpected feature flag");
    }
  };

  std::vector<const char*> features;
  wasm->features.iterFeatures(
    [&](FeatureSet::Feature f) { features.push_back(toString(f)); });

  auto start = startSection(BinaryConsts::Custom);
  writeInlineString(BinaryConsts::CustomSections::TargetFeatures);
  o << U32LEB(features.size());
  for (auto& f : features) {
    o << uint8_t(BinaryConsts::FeatureUsed);
    writeInlineString(f);
  }
  finishSection(start);
}

void WasmBinaryWriter::writeLegacyDylinkSection() {
  if (!wasm->dylinkSection) {
    return;
  }

  auto start = startSection(BinaryConsts::Custom);
  writeInlineString(BinaryConsts::CustomSections::Dylink);
  o << U32LEB(wasm->dylinkSection->memorySize);
  o << U32LEB(wasm->dylinkSection->memoryAlignment);
  o << U32LEB(wasm->dylinkSection->tableSize);
  o << U32LEB(wasm->dylinkSection->tableAlignment);
  o << U32LEB(wasm->dylinkSection->neededDynlibs.size());
  for (auto& neededDynlib : wasm->dylinkSection->neededDynlibs) {
    writeInlineString(neededDynlib.str);
  }
  finishSection(start);
}

void WasmBinaryWriter::writeDylinkSection() {
  if (!wasm->dylinkSection) {
    return;
  }

  if (wasm->dylinkSection->isLegacy) {
    writeLegacyDylinkSection();
    return;
  }

  auto start = startSection(BinaryConsts::Custom);
  writeInlineString(BinaryConsts::CustomSections::Dylink0);

  auto substart =
    startSubsection(BinaryConsts::CustomSections::Subsection::DylinkMemInfo);
  o << U32LEB(wasm->dylinkSection->memorySize);
  o << U32LEB(wasm->dylinkSection->memoryAlignment);
  o << U32LEB(wasm->dylinkSection->tableSize);
  o << U32LEB(wasm->dylinkSection->tableAlignment);
  finishSubsection(substart);

  if (wasm->dylinkSection->neededDynlibs.size()) {
    substart =
      startSubsection(BinaryConsts::CustomSections::Subsection::DylinkNeeded);
    o << U32LEB(wasm->dylinkSection->neededDynlibs.size());
    for (auto& neededDynlib : wasm->dylinkSection->neededDynlibs) {
      writeInlineString(neededDynlib.str);
    }
    finishSubsection(substart);
  }

  writeData(wasm->dylinkSection->tail.data(), wasm->dylinkSection->tail.size());
  finishSection(start);
}

void WasmBinaryWriter::writeDebugLocation(const Function::DebugLocation& loc) {
  if (loc == lastDebugLocation) {
    return;
  }
  auto offset = o.size();
  sourceMapLocations.emplace_back(offset, &loc);
  lastDebugLocation = loc;
}

void WasmBinaryWriter::writeDebugLocation(Expression* curr, Function* func) {
  if (sourceMap) {
    auto& debugLocations = func->debugLocations;
    auto iter = debugLocations.find(curr);
    if (iter != debugLocations.end()) {
      writeDebugLocation(iter->second);
    }
  }
  // If this is an instruction in a function, and if the original wasm had
  // binary locations tracked, then track it in the output as well.
  if (func && !func->expressionLocations.empty()) {
    binaryLocations.expressions[curr] =
      BinaryLocations::Span{BinaryLocation(o.size()), 0};
    binaryLocationTrackedExpressionsForFunc.push_back(curr);
  }
}

void WasmBinaryWriter::writeDebugLocationEnd(Expression* curr, Function* func) {
  if (func && !func->expressionLocations.empty()) {
    auto& span = binaryLocations.expressions.at(curr);
    span.end = o.size();
  }
}

void WasmBinaryWriter::writeExtraDebugLocation(Expression* curr,
                                               Function* func,
                                               size_t id) {
  if (func && !func->expressionLocations.empty()) {
    binaryLocations.delimiters[curr][id] = o.size();
  }
}

void WasmBinaryWriter::writeData(const char* data, size_t size) {
  for (size_t i = 0; i < size; i++) {
    o << int8_t(data[i]);
  }
}

void WasmBinaryWriter::writeInlineString(std::string_view name) {
  o << U32LEB(name.size());
  writeData(name.data(), name.size());
}

static bool isHexDigit(char ch) {
  return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') ||
         (ch >= 'A' && ch <= 'F');
}

static int decodeHexNibble(char ch) {
  return ch <= '9' ? ch & 15 : (ch & 15) + 9;
}

void WasmBinaryWriter::writeEscapedName(std::string_view name) {
  if (name.find('\\') == std::string_view::npos) {
    writeInlineString(name);
    return;
  }
  // decode escaped by escapeName (see below) function names
  std::string unescaped;
  for (size_t i = 0; i < name.size();) {
    char ch = name[i++];
    // support only `\xx` escapes; ignore invalid or unsupported escapes
    if (ch != '\\' || i + 1 >= name.size() || !isHexDigit(name[i]) ||
        !isHexDigit(name[i + 1])) {
      unescaped.push_back(ch);
      continue;
    }
    unescaped.push_back(
      char((decodeHexNibble(name[i]) << 4) | decodeHexNibble(name[i + 1])));
    i += 2;
  }
  writeInlineString({unescaped.data(), unescaped.size()});
}

void WasmBinaryWriter::writeInlineBuffer(const char* data, size_t size) {
  o << U32LEB(size);
  writeData(data, size);
}

void WasmBinaryWriter::writeType(Type type) {
  if (type.isRef()) {
    auto heapType = type.getHeapType();
    if (heapType.isBasic() && type.isNullable()) {
      switch (heapType.getBasic()) {
        case HeapType::ext:
          o << S32LEB(BinaryConsts::EncodedType::externref);
          return;
        case HeapType::any:
          o << S32LEB(BinaryConsts::EncodedType::anyref);
          return;
        case HeapType::func:
          o << S32LEB(BinaryConsts::EncodedType::funcref);
          return;
        case HeapType::eq:
          o << S32LEB(BinaryConsts::EncodedType::eqref);
          return;
        case HeapType::i31:
          o << S32LEB(BinaryConsts::EncodedType::i31ref);
          return;
        case HeapType::struct_:
          o << S32LEB(BinaryConsts::EncodedType::structref);
          return;
        case HeapType::array:
          o << S32LEB(BinaryConsts::EncodedType::arrayref);
          return;
        case HeapType::string:
          o << S32LEB(BinaryConsts::EncodedType::stringref);
          return;
        case HeapType::stringview_wtf8:
          o << S32LEB(BinaryConsts::EncodedType::stringview_wtf8);
          return;
        case HeapType::stringview_wtf16:
          o << S32LEB(BinaryConsts::EncodedType::stringview_wtf16);
          return;
        case HeapType::stringview_iter:
          o << S32LEB(BinaryConsts::EncodedType::stringview_iter);
          return;
        case HeapType::none:
          o << S32LEB(BinaryConsts::EncodedType::nullref);
          return;
        case HeapType::noext:
          // See comment on writeHeapType.
          if (!wasm->features.hasGC()) {
            o << S32LEB(BinaryConsts::EncodedType::externref);
          } else {
            o << S32LEB(BinaryConsts::EncodedType::nullexternref);
          }
          return;
        case HeapType::nofunc:
          // See comment on writeHeapType.
          if (!wasm->features.hasGC()) {
            o << S32LEB(BinaryConsts::EncodedType::funcref);
          } else {
            o << S32LEB(BinaryConsts::EncodedType::nullfuncref);
          }
          return;
      }
    }
    if (type.isNullable()) {
      o << S32LEB(BinaryConsts::EncodedType::nullable);
    } else {
      o << S32LEB(BinaryConsts::EncodedType::nonnullable);
    }
    writeHeapType(type.getHeapType());
    return;
  }
  int ret = 0;
  TODO_SINGLE_COMPOUND(type);
  switch (type.getBasic()) {
    // None only used for block signatures. TODO: Separate out?
    case Type::none:
      ret = BinaryConsts::EncodedType::Empty;
      break;
    case Type::i32:
      ret = BinaryConsts::EncodedType::i32;
      break;
    case Type::i64:
      ret = BinaryConsts::EncodedType::i64;
      break;
    case Type::f32:
      ret = BinaryConsts::EncodedType::f32;
      break;
    case Type::f64:
      ret = BinaryConsts::EncodedType::f64;
      break;
    case Type::v128:
      ret = BinaryConsts::EncodedType::v128;
      break;
    default:
      WASM_UNREACHABLE("unexpected type");
  }
  o << S32LEB(ret);
}

void WasmBinaryWriter::writeHeapType(HeapType type) {
  // ref.null always has a bottom heap type in Binaryen IR, but those types are
  // only actually valid with GC enabled. When GC is not enabled, emit the
  // corresponding valid top types instead.
  if (!wasm->features.hasGC()) {
    if (type == HeapType::nofunc || type.isSignature()) {
      type = HeapType::func;
    } else if (type == HeapType::noext) {
      type = HeapType::ext;
    }
  }

  if (type.isSignature() || type.isStruct() || type.isArray()) {
    o << S64LEB(getTypeIndex(type)); // TODO: Actually s33
    return;
  }
  int ret = 0;
  assert(type.isBasic());
  switch (type.getBasic()) {
    case HeapType::ext:
      ret = BinaryConsts::EncodedHeapType::ext;
      break;
    case HeapType::func:
      ret = BinaryConsts::EncodedHeapType::func;
      break;
    case HeapType::any:
      ret = BinaryConsts::EncodedHeapType::any;
      break;
    case HeapType::eq:
      ret = BinaryConsts::EncodedHeapType::eq;
      break;
    case HeapType::i31:
      ret = BinaryConsts::EncodedHeapType::i31;
      break;
    case HeapType::struct_:
      ret = BinaryConsts::EncodedHeapType::struct_;
      break;
    case HeapType::array:
      ret = BinaryConsts::EncodedHeapType::array;
      break;
    case HeapType::string:
      ret = BinaryConsts::EncodedHeapType::string;
      break;
    case HeapType::stringview_wtf8:
      ret = BinaryConsts::EncodedHeapType::stringview_wtf8_heap;
      break;
    case HeapType::stringview_wtf16:
      ret = BinaryConsts::EncodedHeapType::stringview_wtf16_heap;
      break;
    case HeapType::stringview_iter:
      ret = BinaryConsts::EncodedHeapType::stringview_iter_heap;
      break;
    case HeapType::none:
      ret = BinaryConsts::EncodedHeapType::none;
      break;
    case HeapType::noext:
      ret = BinaryConsts::EncodedHeapType::noext;
      break;
    case HeapType::nofunc:
      ret = BinaryConsts::EncodedHeapType::nofunc;
      break;
  }
  o << S64LEB(ret); // TODO: Actually s33
}

void WasmBinaryWriter::writeIndexedHeapType(HeapType type) {
  o << U32LEB(getTypeIndex(type));
}

void WasmBinaryWriter::writeField(const Field& field) {
  if (field.type == Type::i32 && field.packedType != Field::not_packed) {
    if (field.packedType == Field::i8) {
      o << S32LEB(BinaryConsts::EncodedType::i8);
    } else if (field.packedType == Field::i16) {
      o << S32LEB(BinaryConsts::EncodedType::i16);
    } else {
      WASM_UNREACHABLE("invalid packed type");
    }
  } else {
    writeType(field.type);
  }
  o << U32LEB(field.mutable_);
}

// reader

WasmBinaryBuilder::WasmBinaryBuilder(Module& wasm,
                                     FeatureSet features,
                                     const std::vector<char>& input)
  : wasm(wasm), allocator(wasm.allocator), input(input),
    sourceMap(nullptr), nextDebugLocation{0, 0, {0, 0, 0}}, debugLocation() {
  wasm.features = features;
}

bool WasmBinaryBuilder::hasDWARFSections() {
  assert(pos == 0);
  getInt32(); // magic
  getInt32(); // version
  bool has = false;
  while (more()) {
    uint8_t sectionCode = getInt8();
    uint32_t payloadLen = getU32LEB();
    if (uint64_t(pos) + uint64_t(payloadLen) > input.size()) {
      throwError("Section extends beyond end of input");
    }
    auto oldPos = pos;
    if (sectionCode == BinaryConsts::Section::Custom) {
      auto sectionName = getInlineString();
      if (Debug::isDWARFSection(sectionName)) {
        has = true;
        break;
      }
    }
    pos = oldPos + payloadLen;
  }
  pos = 0;
  return has;
}

void WasmBinaryBuilder::read() {
  if (DWARF) {
    // In order to update dwarf, we must store info about each IR node's
    // binary position. This has noticeable memory overhead, so we don't do it
    // by default: the user must request it by setting "DWARF", and even if so
    // we scan ahead to see that there actually *are* DWARF sections, so that
    // we don't do unnecessary work.
    if (!hasDWARFSections()) {
      DWARF = false;
    }
  }

  readHeader();
  read