/*
 * Copyright 2015 WebAssembly Community Group participants
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

//
// Parses WebAssembly code in S-Expression format, as in .wast files
// such as are in the spec test suite.
//

#ifndef wasm_wasm_s_parser_h
#define wasm_wasm_s_parser_h

#include "mixed_arena.h"
#include "parsing.h" // for UniqueNameMapper. TODO: move dependency to cpp file?
#include "wasm-builder.h"
#include "wasm.h"

namespace wasm {

class SourceLocation {
public:
  IString filename;
  uint32_t line;
  uint32_t column;
  SourceLocation(IString filename_, uint32_t line_, uint32_t column_ = 0)
    : filename(filename_), line(line_), column(column_) {}
};

//
// An element in an S-Expression: a list or a string
//
class Element {
  using List = ArenaVector<Element*>;

  bool isList_ = true;
  List list_;
  IString str_;
  bool dollared_;
  bool quoted_;

public:
  Element(MixedArena& allocator) : list_(allocator) {}

  bool isList() const { return isList_; }
  bool isStr() const { return !isList_; }
  bool dollared() const { return isStr() && dollared_; }
  bool quoted() const { return isStr() && quoted_; }

  size_t line = -1;
  size_t col = -1;
  // original locations at the start/end of the S-Expression list
  SourceLocation* startLoc = nullptr;
  SourceLocation* endLoc = nullptr;

  // list methods
  List& list();
  Element* operator[](unsigned i);
  size_t size() { return list().size(); }
  List::Iterator begin() { return list().begin(); }
  List::Iterator end() { return list().end(); }

  // string methods
  IString str() const;
  std::string toString() const;
  Element* setString(IString str__, bool dollared__, bool quoted__);
  Element* setMetadata(size_t line_, size_t col_, SourceLocation* startLoc_);

  // comparisons
  bool operator==(Name name) { return isStr() && str() == name; }

  template<typename T> bool operator!=(T t) { return !(*this == t); }

  // printing
  friend std::ostream& operator<<(std::ostream& o, Element& e);
  void dump();
};

//
// Generic S-Expression parsing into lists
//
class SExpressionParser {
  const char* input;
  size_t line;
  char const* lineStart;
  SourceLocation* loc = nullptr;

  MixedArena allocator;

public:
  // Assumes control of and modifies the input.
  SExpressionParser(const char* input);
  Element* root;

private:
  Element* parse();
  void skipWhitespace();
  void parseDebugLocation();
  Element* parseString();
};

//
// SExpressions => WebAssembly module
//
class SExpressionWasmBuilder {
  Module& wasm;
  MixedArena& allocator;
  IRProfile profile;

  // The main list of types declared in the module
  std::vector<HeapType> types;
  std::unordered_map<std::string, size_t> typeIndices;

  std::vector<Name> functionNames;
  std::vector<Name> tableNames;
  std::vector<Name> memoryNames;
  std::vector<Name> globalNames;
  std::vector<Name> tagNames;
  int functionCounter = 0;
  int globalCounter = 0;
  int tagCounter = 0;
  int tableCounter = 0;
  int elemCounter = 0;
  int memoryCounter = 0;
  int dataCounter = 0;
  // we need to know function return types before we parse their contents
  std::map<Name, HeapType> functionTypes;
  std::unordered_map<IString, Index> debugInfoFileIndices;

  // Maps type indexes to a mapping of field index => name. This is not the same
  // as the field names stored on the wasm object, as that maps types after
  // their canonicalization. Canonicalization loses information, which means
  // that structurally identical types cannot have different names. However,
  // while parsing the text format we keep this mapping of type indexes to names
  // which does allow reading such content.
  std::unordered_map<size_t, std::unordered_map<Index, Name>> fieldNames;

public:
  // Assumes control of and modifies the input.
  SExpressionWasmBuilder(Module& wasm, Element& module, IRProfile profile);

private:
  void preParseHeapTypes(Element& module);
  // pre-parse types and function definitions, so we know function return types
  // before parsing their contents
  void preParseFunctionType(Element& s);
  bool isImport(Element& curr);
  void preParseImports(Element& curr);
  void preParseMemory(Element& curr);
  void parseModuleElement(Element& curr);

  // function parsing state
  std::unique_ptr<Function> currFunction;
  bool brokeToAutoBlock;

  UniqueNameMapper nameMapper;

  int parseIndex(Element& s);

  Name getFunctionName(Element& s);
  Name getTableName(Element& s);
  Name getMemoryName(Element& s);
  Name getGlobalName(Element& s);
  Name getTagName(Element& s);
  void parseStart(Element& s) { wasm.addStart(getFunctionName(*s[1])); }

  Name getMemoryNameAtIdx(Index i);
  bool isMemory64(Name memoryName);
  bool hasMemoryIdx(Element& s, Index defaultSize, Index i);

  // returns the next index in s
  size_t parseFunctionNames(Element& s, Name& name, Name& exportName);
  void parseFunction(Element& s, bool preParseImport = false);

  Type stringToType(IString str, bool allowError = false, bool prefix = false) {
    return stringToType(str.str, allowError, prefix);
  }
  Type stringToType(std::string_view str,
                    bool allowError = false,
                    bool prefix = false);
  HeapType st