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
      if (!fun