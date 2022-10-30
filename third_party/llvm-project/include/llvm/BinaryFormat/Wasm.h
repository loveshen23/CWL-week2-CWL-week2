//===- Wasm.h - Wasm object file format -------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines manifest constants for the wasm object file format.
// See: https://github.com/WebAssembly/design/blob/master/BinaryEncoding.md
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_BINARYFORMAT_WASM_H
#define LLVM_BINARYFORMAT_WASM_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"

namespace llvm {
namespace wasm {

// Object file magic string.
const char WasmMagic[] = {'\0', 'a', 's', 'm'};
// Wasm binary format version
const uint32_t WasmVersion = 0x1;
// Wasm linking metadata version
const uint32_t WasmMetadataVersion = 0x2;
// Wasm uses a 64k page size
const uint32_t WasmPageSize = 65536;

struct WasmObjectHeader {
  StringRef Magic;
  uint32_t Version;
};

struct WasmDylinkInfo {
  uint32_t MemorySize; // Memory size in bytes
  uint32_t MemoryAlignment;  // P2 alignment of memory
  uint32_t TableSize;  // Table size in elements
  uint32_t TableAlignment;  // P2 alignment of table
  std::vector<StringRef> Needed; // Shared library depenedencies
};

struct WasmProducerInfo {
  std::vector<std::pair<std::string, std::string>> Languages;
  std::vector<std::pair<std::string, std::string>> Tools;
  std::vector<std::pair<std::string, std::string>> SDKs;
};

struct WasmFeatureEntry {
  uint8_t Prefix;
  std::string Name;
};

struct WasmExport {
  StringRef Name;
  uint8_t Kind;
  uint32_t Index;
};

struct WasmLimits {
  uint8_t Flags;
  uint32_t Initial;
  uint32_t Maximum;
};

struct WasmTable {
  uint8_t ElemType;
  WasmLimits Limits;
};

struct WasmInitExpr {
  uint8_t Opcode;
  union {
    int32_t Int32;
    int64_t Int64;
    int32_t Float32;
    int64_t Float64;
    uint32_t Global;
  } Value;
};

struct WasmGlobalType {
  uint8_t Type;
  bool Mutable;
};

struct WasmGlobal {
  uint32_t Index;
  WasmGlobalType Type;
  WasmInitExpr InitExpr;
  StringRef SymbolName; // from the "linking" section
};

struct WasmEventType {
  // Kind of event. Currently only WASM_EVENT_ATTRIBUTE_EXCEPTION is possible.
  uint32_t Attribute;
  uint32_t SigIndex;
};

struct WasmEvent {
  uint32_t Index;
  WasmEventType Type;
  StringRef SymbolName; // from the "linking" section
};

struct WasmImport {
  StringRef Module;
  StringRef Field;
  uint8_t Kind;
  union {
    uint32_t SigIndex;
    WasmGlobalType Global;
    WasmTable Table;
    WasmLimits Memory;
    WasmEventType Event;
  };
};

struct WasmLocalDecl {
  uint8_t Type;
  uint32_t Count;
};

struct WasmFunction {
  uint32_t Index;
  std::vector<WasmLocalDecl> Locals;
  ArrayRef<uint8_t> Body;
  uint32_t CodeSectionOffset;
  uint32_t Size;
  uint32_t CodeOffset;  // start of Locals and Body
  StringRef SymbolName; // from the "linking" section
  StringRef DebugName;  // from the "name" section
  uint32_t Comdat;      // from the "comdat info" section
};

struct WasmDataSegment {
  uint32_t InitFlags;
  uint32_t MemoryIndex; // present if InitFlags & WASM_SEGMENT_HAS_MEMINDEX
  WasmInitExpr Offset; // present if InitFlags & WASM_SEGMENT_IS_PASSIVE == 0
  ArrayRef<uint8_t> Content;
  StringRef Name; // from the "segment info" section
  uint32_t Alignment;
  uint32_t LinkerFlags;
  uint32_t Comdat; // from the "comdat info" section
};

struct WasmElemSegment {
  uint32_t TableIndex;
  WasmInitExpr Offset;
  std::vector<uint32_t> Functions;
};

// Represents the location of a Wasm data symbol within a WasmDataSegment, as
// the index of the segment, and the offset and size within the segment.
struct WasmDataReference {
  uint32_t Segment;
  uint32_t Offset;
  uint32_t Size;
};

struct WasmRelocation {
  uint8_t Type;    // The type of the relocation.
  uint32_t Index;  // Index into either symbol or type index space.
  uint64_t Offset; // Offset from the start of the section.
  int64_t Addend;  // A value to add to the symbol.
};

struct WasmInitFunc {
  uint32_t Priority;
  uint32_t Symbol;
};

struct WasmSymbolInfo {
  StringRef Name;
  uint8_t Kind;
  uint32_t Flags;
  StringRef ImportModule; // For undefined symbols the module of the import
  StringRef ImportName;   // For undefined symbols the name of the import
  union {
    // For function or global symbols, the index in function or global index
    // space.
    uint32_t ElementIndex;
    // For a data symbols, the address of the data relative to segment.
    WasmDataReference DataRef;
  };
};

struct WasmFunctionName {
  uint32_t Index;
  StringRef Name;
};

struct WasmLinkingData {
  uint32_t Version;
  std::vector<WasmInitFunc> InitFunctions;
  std::vector<StringRef> Comdats;
  std::vector<WasmSymbolInfo> SymbolTable;
};

enum : unsigned {
  WASM_SEC_CUSTOM = 0,     // Custom / User-defined section
  WASM_SEC_TYPE = 1,       // Function signature declarations
  WASM_SEC_IMPORT = 2,     // Import declarations
  WASM_SEC_FUNCTION = 3,   // Function declarations
  WASM_SEC_TABLE = 4,      // Indirect function table and other tables
  WASM_SEC_MEMORY = 5,     // Memory attributes
  WASM_SEC_GLOBAL = 6,     // Global declarations
  WASM_SEC_EXPORT = 7,     // Exports
  WASM_SEC_START = 8,      /