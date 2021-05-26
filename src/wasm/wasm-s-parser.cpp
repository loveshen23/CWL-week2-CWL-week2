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

#include "wasm-s-parser.h"

#include <cctype>
#include <cmath>
#include <limits>

#include "ir/branch-utils.h"
#include "ir/table-utils.h"
#include "shared-constants.h"
#include "support/string.h"
#include "wasm-binary.h"
#include "wasm-builder.h"

#define abort_on(str)                                                          \
  { throw ParseException(std::string("abort_on ") + str); }
#define element_assert(condition)                                              \
  assert((condition) ? true : (std::cerr << "on: " << *this << '\n' && 0));

namespace {
int unhex(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  }
  if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  }
  if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  }
  throw wasm::ParseException("invalid hexadecimal");
}
} // namespace

namespace wasm {

static Name STRUCT("struct"), FIELD("field"), ARRAY("array"),
  FUNC_SUBTYPE("func_subtype"), STRUCT_SUBTYPE("struct_subtype"),
  ARRAY_SUBTYPE("array_subtype"), EXTENDS("extends"), REC("rec"), I8("i8"),
  I16("i16"), DECLARE("declare"), ITEM("item"), OFFSET("offset");

static Address getAddress(const Element* s) {
  return std::stoll(s->toString());
}

static void
checkAddress(Address a, const char* errorText, const Element* errorElem) {
  if (a > std::numeric_limits<Address::address32_t>::max()) {
    throw ParseException(errorText, errorElem->line, errorElem->col);
  }
}

static bool elementStartsWith(Element& s, IString str) {
  return s.isList() && s.size() > 0 && s[0]->isStr() && s[0]->str() == str;
}

static bool elementStartsWith(Element* s, IString str) {
  return elementStartsWith(*s, str);
}

Element::List& Element::list() {
  if (!isList()) {
    throw ParseException("expected list", line, col);
  }
  return list_;
}

Element* Element::operator[](unsigned i) {
  if (!isList()) {
    throw ParseException("expected list", line, col);
  }
  if (i >= list().size()) {
    throw ParseException("expected more elements in list", line, col);
  }
  return list()[i];
}

IString Element::str() const {
  if (!isStr()) {
    throw ParseException("expected string", line, col);
  }
  return str_;
}

std::string Element::toString() const {
  if (!isStr()) {
    throw ParseException("expected string", line, col);
  }
  return str_.toString();
}

Element* Element::setString(IString str__, bool dollared__, bool quoted__) {
  isList_ = false;
  str_ = str__;
  dollared_ = dollared__;
  quoted_ = quoted__;
  return this;
}

Element*
Element::setMetadata(size_t line_, size_t col_, SourceLocation* startLoc_) {
  line = line_;
  col = col_;
  startLoc = startLoc_;
  return this;
}

std::ostream& operator<<(std::ostream& o, Element& e) {
  if (e.isList_) {
    o << '(';
    for (auto item : e.list_) {
      o << ' ' << *item;
    }
    o << " )";
  } else {
    if (e.dollared()) {
      o << '$';
    }
    o << e.str_.str;
  }
  return o;
}

void Element::dump() {
  std::cout << "dumping " << this << " : " << *this << ".\n";
}

SExpressionParser::SExpressionParser(char const* input) : input(input) {
  root = nullptr;
  line = 1;
  lineStart = input;
  while (!root) { // keep parsing until we pass an initial comment
    root = parse();
  }
}

Element* SExpressionParser::parse() {
  std::vector<Element*> stack;
  std::vector<SourceLocation*> stackLocs;
  Element* curr = allocator.alloc<Element>();
  while (1) {
    skipWhitespace();
    if (input[0] == 0) {
      break;
    }
    if (input[0] == '(') {
      input++;
      stack.push_back(curr);
      curr = allocator.alloc<Element>()->setMetadata(
        line, input - lineStart - 1, loc);
      stackLocs.push_back(loc);
      assert(stack.size() == stackLocs.size());
    } else if (input[0] == ')') {
      input++;
      curr->endLoc = loc;
      auto last = curr;
      if (stack.empty()) {
        throw ParseException("s-expr stack empty");
      }
      curr = stack.back();
      assert(stack.size() == stackLocs.size());
      stack.pop_back();
      loc = stackLocs.back();
      stackLocs.pop_back();
      curr->list().push_back(last);
    } else {
      curr->list().push_back(parseString());
    }
  }
  if (stack.size() != 0) {
    throw ParseException("stack is not empty", curr->line, curr->col);
  }
  return curr;
}

void SExpressionParser::parseDebugLocation() {
  // Extracting debug location (if valid)
  char const* debugLoc = input + 3; // skipping ";;@"
  while (debugLoc[0] && debugLoc[0] == ' ') {
    debugLoc++;
  }
  char const* debugLocEnd = debugLoc;
  while (debugLocEnd[0] && debugLocEnd[0] != '\n') {
    debugLocEnd++;
  }
  char const* pos = debugLoc;
  while (pos < debugLocEnd && pos[0] != ':') {
    pos++;
  }
  if (pos >= debugLocEnd) {
    return; // no line number
  }
  std::string name(debugLoc, pos);
  char const* lineStart = ++pos;
  while (pos < debugLocEnd && pos[0] != ':') {
    pos++;
  }
  std::string lineStr(lineStart, pos);
  if (pos >= debugLocEnd) {
    return; // no column number
  }
  std::string colStr(++pos, debugLocEnd);
  void* buf =
    allocator.allocSpace(sizeof(SourceLocation), alignof(SourceLocation));
  loc = new (buf) SourceLocation(
    IString(name.c_str(), false), atoi(lineStr.c_str()), atoi(colStr.c_str()));
}

void SExpressionParser::skipWhitespace() {
  while (1) {
    while (isspace(input[0])) {
      if (input[0] == '\n') {
        line++;
        lineStart = input + 1;
      }
      input++;
    }
    if (input[0] == ';' && input[1] == ';') {
      if (input[2] == '@') {
        parseDebugLocation();
      }
      while (input[0] && input[0] != '\n') {
        input++;
      }
      line++;
      if (!input[0]) {
        return;
      }
      lineStart = ++input;
    } else if (input[0] == '(' && input[1] == ';') {
      // Skip nested block comments.
      input += 2;
      int depth = 1;
      while (1) {
        if (!input[0]) {
          return;
        }
        if (input[0] == '(' && input[1] == ';') {
          input += 2;
          depth++;
        } else if (input[0] == ';' && input[1] == ')') {
          input += 2;
          --depth;
          if (depth == 0) {
            break;
          }
        } else if (input[0] == '\n') {
          line++;
          lineStart = input;
          input++;
        } else {
          input++;
        }
      }
    } else {
      return;
    }
  }
}

Element* SExpressionParser::parseString() {
  bool dollared = false;
  if (input[0] == '$') {
    input++;
    dollared = true;
  }
  char const* start = input;
  if (input[0] == '"') {
    // parse escaping \", but leave code escaped - we'll handle escaping in
    // memory segments specifically
    input++;
    std::string str;
    while (1) {
      if (input[0] == 0) {
        throw ParseException("unterminated string", line, start - lineStart);
      }
      if (input[0] == '"') {
        break;
      }
      if (input[0] == '\\') {
        str += input[0];
        if (input[1] == 0) {
          throw ParseException(
            "unterminated string escape", line, start - lineStart);
        }
        str += input[1];
        input += 2;
        continue;
      }
      str += input[0];
      input++;
    }
    input++;
    return allocator.alloc<Element>()
      ->setString(IString(str.c_str(), false), dollared, true)
      ->setMetadata(line, start - lineStart, loc);
  }
  while (input[0] && !isspace(input[0]) && input[0] != ')' && input[0] != '(' &&
         input[0] != ';') {
    input++;
  }
  if (start == input) {
    throw ParseException("expected string", line, input - lineStart);
  }

  std::string temp;
  temp.assign(start, input - start);

  auto ret = allocator.alloc<Element>()
               ->setString(IString(temp.c_str(), false), dollared, false)
               ->setMetadata(line, start - lineStart, loc);

  return ret;
}

SExpressionWasmBuilder::SExpressionWasmBuilder(Module& wasm,
                                               Element& module,
                                               IRProfile profile)
  : wasm(wasm), allocator(wasm.allocator), profile(profile) {
  if (module.size() == 0) {
    throw ParseException("empty toplevel, expected module");
  }
  if (module[0]->str() != MODULE) {
    throw ParseException("toplevel does not start with module");
  }
  if (module.size() == 1) {
    return;
  }
  Index i = 1;
  if (module[i]->dollared()) {
    wasm.name = module[i]->str();
    if (module.size() == 2) {
      return;
    }
    i++;
  }

  // spec tests have a `binary` keyword after the optional module name. Skip it
  Name BINARY("binary");
  if (module[i]->isStr() && module[i]->str() == BINARY &&
      !module[i]->quoted()) {
    i++;
  }

  if (i < module.size() && module[i]->isStr()) {
    // these s-expressions contain a binary module, actually
    std::vector<char> data;
    for (; i < module.size(); ++i) {
      stringToBinary(*module[i], module[i]->str().str, data);
    }
    // TODO: support applying features here
    WasmBinaryBuilder binaryBuilder(wasm, FeatureSet::MVP, data);
    binaryBuilder.read();
    return;
  }

  preParseHeapTypes(module);

  Index implementedFunctions = 0;
  functionCounter = 0;
  for (unsigned j = i; j < module.size(); j++) {
    auto& s = *module[j];
    preParseFunctionType(s);
    preParseImports(s);
    preParseMemory(s);
    if (elementStartsWith(s, FUNC) && !isImport(s)) {
      implementedFunctions++;
    }
  }
  // we go through the functions again, now parsing them, and the counter begins
  // from where imports ended
  functionCounter -= implementedFunctions;
  for (unsigned j = i; j < module.size(); j++) {
    parseModuleElement(*module[j]);
  }
}

bool SExpressionWasmBuilder::isImport(Element& curr) {
  for (Index i = 0; i < curr.size(); i++) {
    auto& x = *curr[i];
    if (elementStartsWith(x, IMPORT)) {
      return true;
    }
  }
  return false;
}

void SExpressionWasmBuilder::preParseImports(Element& curr) {
  IString id = curr[0]->str();
  if (id == IMPORT) {
    parseImport(curr);
  }
  if (isImport(curr)) {
    if (id == FUNC) {
      parseFunction(curr, true /* preParseImport */);
    } else if (id == GLOBAL) {
      parseGlobal(curr, true /* preParseImport */);
    } else if (id == TABLE) {
      parseTable(curr, true /* preParseImport */);
    } else if (id == MEMORY) {
      parseMemory(curr, true /* preParseImport */);
    } else if (id == TAG) {
      parseTag(curr, true /* preParseImport */);
    } else {
      throw ParseException(
        "fancy import we don't support yet", curr.line, curr.col);
    }
  }
}

void SExpressionWasmBuilder::preParseMemory(Element& curr) {
  IString id = curr[0]->str();
  if (id == MEMORY && !isImport(curr)) {
    parseMemory(curr);
  }
}

void SExpressionWasmBuilder::parseModuleElement(Element& curr) {
  if (isImport(curr)) {
    return; // already done
  }
  IString id = curr[0]->str();
  if (id == MEMORY) {
    return; // already done
  }
  if (id == START) {
    return parseStart(curr);
  }
  if (id == FUNC) {
    return parseFunction(curr);
  }
  if (id == DATA) {
    return parseData(curr);
  }
  if (id == EXPORT) {
    return parseExport(curr);
  }
  if (id == IMPORT) {
    return; // already done
  }
  if (id == GLOBAL) {
    return parseGlobal(curr);
  }
  if (id == TABLE) {
    return parseTable(curr);
  }
  if (id == ELEM) {
    return parseElem(curr);
  }
  if (id == TYPE) {
    return; // already done
  }
  if (id == REC) {
    return; // already done
  }
  if (id == TAG) {
    return parseTag(curr);
  }
  std::cerr << "bad module element " << id.str << '\n';
  throw ParseException("unknown module element", curr.line, curr.col);
}

int SExpressionWasmBuilder::parseIndex(Element& s) {
  try {
    return std::stoi(s.toString());
  } catch (...) {
    throw ParseException("expected integer", s.line, s.col);
  }
}

Name SExpressionWasmBuilder::getFunctionName(Element& s) {
  if (s.dollared()) {
    return s.str();
  } else {
    // index
    size_t offset = parseIndex(s);
    if (offset >= functionNames.size()) {
      throw ParseException(
        "unknown function in getFunctionName", s.line, s.col);
    }
    return functionNames[offset];
  }
}

Name SExpressionWasmBuilder::getTableName(Element& s) {
  if (s.dollared()) {
    return s.str();
  } else {
    // index
    size_t offset = parseIndex(s);
    if (offset >= tableNames.size()) {
      throw ParseException("unknown table in getTableName", s.line, s.col);
    }
    return tableNames[offset];
  }
}

bool SExpressionWasmBuilder::isMemory64(Name memoryName) {
  auto* memory = wasm.getMemoryOrNull(memoryName);
  if (!memory) {
    throw ParseException("invalid memory name in isMemory64");
  }
  return memory->is64();
}

Name SExpressionWasmBuilder::getMemoryNameAtIdx(Index i) {
  if (i >= memoryNames.size()) {
    throw ParseException("unknown memory in getMemoryName");
  }
  return memoryNames[i];
}

Name SExpressionWasmBuilder::getMemoryName(Element& s) {
  if (s.dollared()) {
    return s.str();
  } else {
    // index
    size_t offset = parseIndex(s);
    return getMemoryNameAtIdx(offset);
  }
}

Name SExpressionWasmBuilder::getGlobalName(Element& s) {
  if (s.dollared()) {
    return s.str();
  } else {
    // index
    size_t offset = parseIndex(s);
    if (offset >= globalNames.size()) {
      throw ParseException("unknown global in getGlobalName", s.line, s.col);
    }
    return globalNames[offset];
  }
}

Name SExpressionWasmBuilder::getTagName(Element& s) {
  if (s.dollared()) {
    return s.str();
  } else {
    // index
    size_t offset = parseIndex(s);
    if (offset >= tagNames.size()) {
      throw ParseException("unknown tag in getTagName", s.line, s.col);
    }
    return tagNames[offset];
  }
}

// Parse various forms of (param ...) or (local ...) element. This ignores all
// parameter or local names when specified.
std::vector<Type> SExpressionWasmBuilder::parseParamOrLocal(Element& s) {
  size_t fakeIndex = 0;
  std::vector<NameType> namedParams = parseParamOrLocal(s, fakeIndex);
  std::vector<Type> params;
  for (auto& p : namedParams) {
    params.push_back(p.type);
  }
  return params;
}

// Parses various forms of (param ...) or (local ...) element:
// (param $name type) (e.g. (param $a i32))
// (param type+)      (e.g. (param i32 f64))
// (local $name type) (e.g. (local $a i32))
// (local type+)      (e.g. (local i32 f64))
// If the name is unspecified, it will create one using localIndex.
std::vector<NameType>
SExpressionWasmBuilder::parseParamOrLocal(Element& s, size_t& localIndex) {
  assert(elementStartsWith(s, PARAM) || elementStartsWith(s, LOCAL));
  std::vector<NameType> namedParams;
  if (s.size() == 1) { // (param) or (local)
    return namedParams;
  }

  for (size_t i = 1; i < s.size(); i++) {
    IString name;
    if (s[i]->dollared()) {
      if (i != 1) {
        throw ParseException("invalid wasm type", s[i]->line, s[i]->col);
      }
      if (i + 1 >= s.size()) {
        throw ParseException("invalid param entry", s.line, s.col);
      }
      name = s[i]->str();
      i++;
    } else {
      name = Name::fromInt(localIndex);
    }
    localIndex++;
    Type type;
    type = elementToType(*s[i]);
    if (elementStartsWith(s, PARAM) && type.isTuple()) {
      throw ParseException(
        "params may not have tuple types", s[i]->line, s[i]->col);
    }
    namedParams.emplace_back(name, type);
  }
  return namedParams;
}

// Parses (result type) element. (e.g. (result i32))
std::vector<Type> SExpressionWasmBuilder::parseResults(Element& s) {
  assert(elementStartsWith(s, RESULT));
  std::vector<Type> types;
  for (size_t i = 1; i < s.size(); i++) {
    types.push_back(elementToType(*s[i]));
  }
  return types;
}

// Parses an element that references an entry in the type section. The element
// should be in the form of (type name) or (type index).
// (e.g. (type $a), (type 0))
HeapType SExpressionWasmBuilder::parseTypeRef(Element& s) {
  assert(elementStartsWith(s, TYPE));
  if (s.size() != 2) {
    throw ParseException("invalid type reference", s.line, s.col);
  }
  auto heapType = parseHeapType(*s[1]);
  if (!heapType.isSignature()) {
    throw ParseException("expected signature type", s.line, s.col);
  }
  return heapType;
}

// Parses typeuse, a reference to a type definition. It is in the form of either
// (type index) or (type name), possibly augmented by inlined (param) and
// (result) nodes. (type) node can be omitted as well. Outputs are returned by
// parameter references.
// typeuse ::= (type index|name)+ |
//             (type index|name)+ (param ..)* (result ..)* |
//             (param ..)* (result ..)*
size_t
SExpressionWasmBuilder::parseTypeUse(Element& s,
                                     size_t startPos,
                                     HeapType& functionType,
                                     std::vector<NameType>& namedParams) {
  std::vector<Type> params, results;
  size_t i = startPos;

  bool typeExists = false, paramsOrResultsExist = false;
  if (i < s.size() && elementStartsWith(*s[i], TYPE)) {
    typeExists = true;
    functionType = parseTypeRef(*s[i++]);
  }

  size_t paramPos = i;
  size_t localIndex = 0;

  while (i < s.size() && elementStartsWith(*s[i], PARAM)) {
    paramsOrResultsExist = true;
    auto newParams = parseParamOrLocal(*s[i++], localIndex);
    namedParams.insert(namedParams.end(), newParams.begin(), newParams.end());
    for (auto p : newParams) {
      params.push_back(p.type);
    }
  }

  while (i < s.size() && elementStartsWith(*s[i], RESULT)) {
    paramsOrResultsExist = true;
    auto newResults = parseResults(*s[i++]);
    results.insert(results.end(), newResults.begin(), newResults.end());
  }

  auto inlineSig = Signature(Type(params), Type(results));

  // If none of type/param/result exists, this is equivalent to a type that does
  // not have parameters and returns nothing.
  if (!typeExists && !paramsOrResultsExist) {
    paramsOrResultsExist = true;
  }

  if (!typeExists) {
    functionType = inlineSig;
  } else if (paramsOrResultsExist) {
    // verify that (type) and (params)/(result) match
    if (inlineSig != functionType.getSignature()) {
      throw ParseException("type and param/result don't match",
                           s[paramPos]->line,
                           s[paramPos]->col);
    }
  }

  // Add implicitly defined type to global list so it has an index
  if (std::find(types.begin(), types.end(), functionType) == types.end()) {
    types.push_back(functionType);
  }

  // If only (type) is specified, populate `namedParams`
  if (!paramsOrResultsExist) {
    size_t index = 0;
    assert(functionType.isSignature());
    Signature sig = functionType.getSignature();
    for (const auto& param : sig.params) {
      namedParams.emplace_back(Name::fromInt(index++), param);
    }
  }

  return i;
}

// Parses a typeuse. Use this when only FunctionType* is needed.
size_t SExpressionWasmBuilder::parseTypeUse(Element& s,
                                            size_t startPos,
                                            HeapType& functionType) {
  std::vector<NameType> params;
  return parseTypeUse(s, startPos, functionType, params);
}

void SExpressionWasmBuilder::preParseHeapTypes(Element& module) {
  // Iterate through each individual type definition, calling `f` with the
  // definition and its recursion group number.
  auto forEachType = [&](auto f) {
    size_t groupNumber = 0;
    for (auto* elemPtr : module) {
      auto& elem = *elemPtr;
      if (elementStartsWith(elem, TYPE)) {
        f(elem, groupNumber++);
      } else if (elementStartsWith(elem, REC)) {
        for (auto* innerPtr : elem) {
          auto& inner = *innerPtr;
          if (elementStartsWith(inner, TYPE)) {
            f(inner, groupNumber);
          }
        }
        ++groupNumber;
      }
    }
  };

  // Map type names to indices
  size_t numTypes = 0;
  forEachType([&](Element& elem, size_t) {
    if (elem[1]->dollared()) {
      std::string name = elem[1]->toString();
      if (!typeIndices.insert({name, numTypes}).second) {
        throw ParseException("duplicate function type", elem.line, elem.col);
      }
    }
    ++numTypes;
  });

  TypeBuilder builder(numTypes);

  // Create recursion groups
  size_t currGroup = 0, groupStart = 0, groupLength = 0;
  auto finishGroup = [&]() {
    builder.createRecGroup(groupStart, groupLength);
    groupStart = groupStart + groupLength;
    groupLength = 0;
  };
  forEachType([&](Element&, size_t group) {
    if (group != currGroup) {
      finishGroup();
      currGroup = group;
    }
    ++groupLength;
  });
  finishGroup();

  auto parseRefType = [&](Element& elem) -> Type {
    // '(' 'ref' 'null'? ht ')'
    auto nullable =
      elem[1]->isStr() && *elem[1] == NULL_ ? Nullable : NonNullable;
    auto& referent = nullable ? *elem[2] : *elem[1];
    auto name = referent.toString();
    if (referent.dollared()) {
      return builder.getTempRefType(builder[typeIndices[name]], nullable);
    } else if (String::isNumber(name)) {
      size_t index = parseIndex(ref