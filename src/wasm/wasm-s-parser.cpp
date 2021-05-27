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
      size_t index = parseIndex(referent);
      if (index >= numTypes) {
        throw ParseException("invalid type index", elem.line, elem.col);
      }
      return builder.getTempRefType(builder[index], nullable);
    } else {
      return Type(stringToHeapType(referent.str()), nullable);
    }
  };

  auto parseValType = [&](Element& elem) {
    if (elem.isStr()) {
      return stringToType(elem.str());
    } else if (*elem[0] == REF) {
      return parseRefType(elem);
    } else {
      throw ParseException("unknown valtype kind", elem[0]->line, elem[0]->col);
    }
  };

  auto parseParams = [&](Element& elem) {
    auto it = ++elem.begin();
    if (it != elem.end() && (*it)->dollared()) {
      ++it;
    }
    std::vector<Type> params;
    for (auto end = elem.end(); it != end; ++it) {
      params.push_back(parseValType(**it));
    }
    return params;
  };

  auto parseResults = [&](Element& elem) {
    std::vector<Type> results;
    for (auto it = ++elem.begin(); it != elem.end(); ++it) {
      results.push_back(parseValType(**it));
    }
    return results;
  };

  auto parseSignatureDef = [&](Element& elem, bool nominal) {
    // '(' 'func' vec(param) vec(result) ')'
    // param ::= '(' 'param' id? valtype ')'
    // result ::= '(' 'result' valtype ')'
    std::vector<Type> params, results;
    auto end = elem.end() - (nominal ? 1 : 0);
    for (auto it = ++elem.begin(); it != end; ++it) {
      Element& curr = **it;
      if (elementStartsWith(curr, PARAM)) {
        auto newParams = parseParams(curr);
        params.insert(params.end(), newParams.begin(), newParams.end());
      } else if (elementStartsWith(curr, RESULT)) {
        auto newResults = parseResults(curr);
        results.insert(results.end(), newResults.begin(), newResults.end());
      }
    }
    return Signature(builder.getTempTupleType(params),
                     builder.getTempTupleType(results));
  };

  // Parses a field, and notes the name if one is found.
  auto parseField = [&](Element* elem, Name& name) {
    Mutability mutable_ = Immutable;
    // elem is a list, containing either
    //   TYPE
    // or
    //   (field TYPE)
    // or
    //   (field $name TYPE)
    if (elementStartsWith(elem, FIELD)) {
      if (elem->size() == 3) {
        name = (*elem)[1]->str();
      }
      elem = (*elem)[elem->size() - 1];
    }
    // The element may also be (mut (..)).
    if (elementStartsWith(elem, MUT)) {
      mutable_ = Mutable;
      elem = (*elem)[1];
    }
    if (elem->isStr()) {
      // elem is a simple string name like "i32". It can be a normal wasm
      // type, or one of the special types only available in fields.
      if (*elem == I8) {
        return Field(Field::i8, mutable_);
      } else if (*elem == I16) {
        return Field(Field::i16, mutable_);
      }
    }
    // Otherwise it's an arbitrary type.
    return Field(parseValType(*elem), mutable_);
  };

  auto parseStructDef = [&](Element& elem, size_t typeIndex, bool nominal) {
    FieldList fields;
    Index end = elem.size() - (nominal ? 1 : 0);
    for (Index i = 1; i < end; i++) {
      Name name;
      fields.emplace_back(parseField(elem[i], name));
      if (name.is()) {
        // Only add the name to the map if it exists.
        fieldNames[typeIndex][i - 1] = name;
      }
    }
    return Struct(fields);
  };

  auto parseArrayDef = [&](Element& elem) {
    Name unused;
    return Array(parseField(elem[1], unused));
  };

  size_t index = 0;
  forEachType([&](Element& elem, size_t) {
    Element& def = elem[1]->dollared() ? *elem[2] : *elem[1];
    Element& kind = *def[0];
    bool hasSupertype =
      kind == FUNC_SUBTYPE || kind == STRUCT_SUBTYPE || kind == ARRAY_SUBTYPE;
    if (kind == FUNC || kind == FUNC_SUBTYPE) {
      builder[index] = parseSignatureDef(def, hasSupertype);
    } else if (kind == STRUCT || kind == STRUCT_SUBTYPE) {
      builder[index] = parseStructDef(def, index, hasSupertype);
    } else if (kind == ARRAY || kind == ARRAY_SUBTYPE) {
      builder[index] = parseArrayDef(def);
    } else {
      throw ParseException("unknown heaptype kind", kind.line, kind.col);
    }
    Element* super = nullptr;
    if (hasSupertype) {
      super = def[def.size() - 1];
      if (super->dollared()) {
        // OK
      } else if (kind == FUNC_SUBTYPE && super->str() == FUNC) {
        // OK; no supertype
        super = nullptr;
      } else if ((kind == STRUCT_SUBTYPE || kind == ARRAY_SUBTYPE) &&
                 super->str() == DATA) {
        // OK; no supertype
        super = nullptr;
      } else {
        throw ParseException("unknown supertype", super->line, super->col);
      }
    } else if (elementStartsWith(elem[elem.size() - 1], EXTENDS)) {
      // '(' 'extends' $supertype ')'
      Element& extends = *elem[elem.size() - 1];
      super = extends[1];
    }
    if (super) {
      auto it = typeIndices.find(super->toString());
      if (it == typeIndices.end()) {
        throw ParseException("unknown supertype", super->line, super->col);
      }
      builder[index].subTypeOf(builder[it->second]);
    }
    ++index;
  });

  auto result = builder.build();
  if (auto* err = result.getError()) {
    // Find the name to provide a better error message.
    std::stringstream msg;
    msg << "Invalid type: " << err->reason;
    for (auto& [name, index] : typeIndices) {
      if (index == err->index) {
        Fatal() << msg.str() << " at type $" << name;
      }
    }
    // No name, just report the index.
    Fatal() << msg.str() << " at index " << err->index;
  }
  types = *result;

  for (auto& [name, index] : typeIndices) {
    auto type = types[index];
    // A type may appear in the type section more than once, but we canonicalize
    // types internally, so there will be a single name chosen for that type. Do
    // so determistically.
    if (wasm.typeNames.count(type) && wasm.typeNames[type].name.str < name) {
      continue;
    }
    auto& currTypeNames = wasm.typeNames[type];
    currTypeNames.name = name;
    if (type.isStruct()) {
      currTypeNames.fieldNames = fieldNames[index];
    }
  }
}

void SExpressionWasmBuilder::preParseFunctionType(Element& s) {
  IString id = s[0]->str();
  if (id != FUNC) {
    return;
  }
  size_t i = 1;
  Name name, exportName;
  i = parseFunctionNames(s, name, exportName);
  if (!name.is()) {
    // unnamed, use an index
    name = Name::fromInt(functionCounter);
  }
  functionNames.push_back(name);
  functionCounter++;
  parseTypeUse(s, i, functionTypes[name]);
}

size_t SExpressionWasmBuilder::parseFunctionNames(Element& s,
                                                  Name& name,
                                                  Name& exportName) {
  size_t i = 1;
  while (i < s.size() && i < 3 && s[i]->isStr()) {
    if (s[i]->quoted()) {
      // an export name
      exportName = s[i]->str();
      i++;
    } else if (s[i]->dollared()) {
      name = s[i]->str();
      i++;
    } else {
      break;
    }
  }
  if (i < s.size() && s[i]->isList()) {
    auto& inner = *s[i];
    if (elementStartsWith(inner, EXPORT)) {
      exportName = inner[1]->str();
      i++;
    }
  }
#if 0
  if (exportName.is() && !name.is()) {
    name = exportName; // useful for debugging
  }
#endif
  return i;
}

void SExpressionWasmBuilder::parseFunction(Element& s, bool preParseImport) {
  brokeToAutoBlock = false;

  Name name, exportName;
  size_t i = parseFunctionNames(s, name, exportName);
  bool hasExplicitName = name.is();
  if (!preParseImport) {
    if (!name.is()) {
      // unnamed, use an index
      name = Name::fromInt(functionCounter);
    }
    functionCounter++;
  } else {
    // just preparsing, functionCounter was incremented by preParseFunctionType
    if (!name.is()) {
      // unnamed, use an index
      name = functionNames[functionCounter - 1];
    }
  }
  if (exportName.is()) {
    auto ex = make_unique<Export>();
    ex->name = exportName;
    ex->value = name;
    ex->kind = ExternalKind::Function;
    if (wasm.getExportOrNull(ex->name)) {
      throw ParseException("duplicate export", s.line, s.col);
    }
    wasm.addExport(ex.release());
  }

  // parse import
  Name importModule, importBase;
  if (i < s.size() && elementStartsWith(*s[i], IMPORT)) {
    Element& curr = *s[i];
    importModule = curr[1]->str();
    importBase = curr[2]->str();
    i++;
  }

  // parse typeuse: type/param/result
  HeapType type;
  std::vector<NameType> params;
  i = parseTypeUse(s, i, type, params);

  // when (import) is inside a (func) element, this is not a function definition
  // but an import.
  if (importModule.is()) {
    if (!importBase.size()) {
      throw ParseException("module but no base for import", s.line, s.col);
    }
    if (!preParseImport) {
      throw ParseException("!preParseImport in func", s.line, s.col);
    }
    auto im = make_unique<Function>();
    im->setName(name, hasExplicitName);
    im->module = importModule;
    im->base = importBase;
    im->type = type;
    functionTypes[name] = type;
    if (wasm.getFunctionOrNull(im->name)) {
      throw ParseException("duplicate import", s.line, s.col);
    }
    wasm.addFunction(std::move(im));
    if (currFunction) {
      throw ParseException("import module inside function dec", s.line, s.col);
    }
    nameMapper.clear();
    return;
  }
  // at this point this not an import but a real function definition.
  if (preParseImport) {
    throw ParseException("preParseImport in func", s.line, s.col);
  }

  size_t localIndex = params.size(); // local index for params and locals

  // parse locals
  std::vector<NameType> vars;
  while (i < s.size() && elementStartsWith(*s[i], LOCAL)) {
    auto newVars = parseParamOrLocal(*s[i++], localIndex);
    vars.insert(vars.end(), newVars.begin(), newVars.end());
  }

  // make a new function
  currFunction = std::unique_ptr<Function>(
    Builder(wasm).makeFunction(name, std::move(params), type, std::move(vars)));
  currFunction->profile = profile;

  // parse body
  Block* autoBlock = nullptr; // may need to add a block for the very top level
  auto ensureAutoBlock = [&]() {
    if (!autoBlock) {
      autoBlock = allocator.alloc<Block>();
      autoBlock->list.push_back(currFunction->body);
      currFunction->body = autoBlock;
    }
  };
  while (i < s.size()) {
    Expression* ex = parseExpression(*s[i++]);
    if (!currFunction->body) {
      currFunction->body = ex;
    } else {
      ensureAutoBlock();
      autoBlock->list.push_back(ex);
    }
  }

  if (brokeToAutoBlock) {
    ensureAutoBlock();
    autoBlock->name = FAKE_RETURN;
  }
  if (autoBlock) {
    autoBlock->finalize(type.getSignature().results);
  }
  if (!currFunction->body) {
    currFunction->body = allocator.alloc<Nop>();
  }
  if (s.startLoc) {
    currFunction->prologLocation.insert(getDebugLocation(*s.startLoc));
  }
  if (s.endLoc) {
    currFunction->epilogLocation.insert(getDebugLocation(*s.endLoc));
  }
  if (wasm.getFunctionOrNull(currFunction->name)) {
    throw ParseException("duplicate function", s.line, s.col);
  }
  wasm.addFunction(currFunction.release());
  nameMapper.clear();
}

Type SExpressionWasmBuilder::stringToType(std::string_view str,
                                          bool allowError,
                                          bool prefix) {
  if (str.size() >= 3) {
    if (str[0] == 'i') {
      if (str[1] == '3' && str[2] == '2' && (prefix || str.size() == 3)) {
        return Type::i32;
      }
      if (str[1] == '6' && str[2] == '4' && (prefix || str.size() == 3)) {
        return Type::i64;
      }
    }
    if (str[0] == 'f') {
      if (str[1] == '3' && str[2] == '2' && (prefix || str.size() == 3)) {
        return Type::f32;
      }
      if (str[1] == '6' && str[2] == '4' && (prefix || str.size() == 3)) {
        return Type::f64;
      }
    }
  }
  if (str.size() >= 4) {
    if (str[0] == 'v') {
      if (str[1] == '1' && str[2] == '2' && str[3] == '8' &&
          (prefix || str.size() == 4)) {
        return Type::v128;
      }
    }
  }
  if (str.substr(0, 7) == "funcref" && (prefix || str.size() == 7)) {
    return Type(HeapType::func, Nullable);
  }
  if (str.substr(0, 9) == "externref" && (prefix || str.size() == 9)) {
    return Type(HeapType::ext, Nullable);
  }
  if (str.substr(0, 6) == "anyref" && (prefix || str.size() == 6)) {
    return Type(HeapType::any, Nullable);
  }
  if (str.substr(0, 5) == "eqref" && (prefix || str.size() == 5)) {
    return Type(HeapType::eq, Nullable);
  }
  if (str.substr(0, 6) == "i31ref" && (prefix || str.size() == 6)) {
    return Type(HeapType::i31, Nullable);
  }
  if (str.substr(0, 9) == "structref" && (prefix || str.size() == 9)) {
    return Type(HeapType::struct_, Nullable);
  }
  if (str.substr(0, 8) == "arrayref" && (prefix || str.size() == 8)) {
    return Type(HeapType::array, Nullable);
  }
  if (str.substr(0, 9) == "stringref" && (prefix || str.size() == 9)) {
    return Type(HeapType::string, Nullable);
  }
  if (str.substr(0, 15) == "stringview_wtf8" && (prefix || str.size() == 15)) {
    return Type(HeapType::stringview_wtf8, Nullable);
  }
  if (str.substr(0, 16) == "stringview_wtf16" && (prefix || str.size() == 16)) {
    return Type(HeapType::stringview_wtf16, Nullable);
  }
  if (str.substr(0, 15) == "stringview_iter" && (prefix || str.size() == 15)) {
    return Type(HeapType::stringview_iter, Nullable);
  }
  if (str.substr(0, 7) == "nullref" && (prefix || str.size() == 7)) {
    return Type(HeapType::none, Nullable);
  }
  if (str.substr(0, 13) == "nullexternref" && (prefix || str.size() == 13)) {
    return Type(HeapType::noext, Nullable);
  }
  if (str.substr(0, 11) == "nullfuncref" && (prefix || str.size() == 11)) {
    return Type(HeapType::nofunc, Nullable);
  }
  if (allowError) {
    return Type::none;
  }
  throw ParseException(std::string("invalid wasm type: ") +
                       std::string(str.data(), str.size()));
}

HeapType SExpressionWasmBuilder::stringToHeapType(std::string_view str,
                                                  bool prefix) {
  if (str.substr(0, 4) == "func" && (prefix || str.size() == 4)) {
    return HeapType::func;
  }
  if (str.substr(0, 2) == "eq" && (prefix || str.size() == 2)) {
    return HeapType::eq;
  }
  if (str.substr(0, 6) == "extern" && (prefix || str.size() == 6)) {
    return HeapType::ext;
  }
  if (str.substr(0, 3) == "any" && (prefix || str.size() == 3)) {
    return HeapType::any;
  }
  if (str.substr(0, 3) == "i31" && (prefix || str.size() == 3)) {
    return HeapType::i31;
  }
  if (str.substr(0, 6) == "struct" && (prefix || str.size() == 6)) {
    return HeapType::struct_;
  }
  if (str.substr(0, 5) == "array" && (prefix || str.size() == 5)) {
    return HeapType::array;
  }
  if (str.substr(0, 6) == "string" && (prefix || str.size() == 6)) {
    return HeapType::string;
  }
  if (str.substr(0, 15) == "stringview_wtf8" && (prefix || str.size() == 15)) {
    return HeapType::stringview_wtf8;
  }
  if (str.substr(0, 16) == "stringview_wtf16" && (prefix || str.size() == 16)) {
    return HeapType::stringview_wtf16;
  }
  if (str.substr(0, 15) == "stringview_iter" && (prefix || str.size() == 15)) {
    return HeapType::stringview_iter;
  }
  if (str.substr(0, 4) == "none" && (prefix || str.size() == 4)) {
    return HeapType::none;
  }
  if (str.substr(0, 8) == "noextern" && (prefix || str.size() == 8)) {
    return HeapType::noext;
  }
  if (str.substr(0, 6) == "nofunc" && (prefix || str.size() == 6)) {
    return HeapType::nofunc;
  }
  throw ParseException(std::string("invalid wasm heap type: ") +
                       std::string(str.data(), str.size()));
}

Type SExpressionWasmBuilder::elementToType(Element& s) {
  if (s.isStr()) {
    return stringToType(s.str());
  }
  auto& list = s.list();
  auto size = list.size();
  if (elementStartsWith(s, REF)) {
    // It's a reference. It should be in the form
    //   (ref $name)
    // or
    //   (ref null $name)
    // and also $name can be the expanded structure of the type and not a name,
    // so something like (ref (func (result i32))), etc.
    if (size != 2 && size != 3) {
      throw ParseException(
        std::string("invalid reference type size"), s.line, s.col);
    }
    if (size == 3 && *list[1] != NULL_) {
      throw ParseException(
        std::string("invalid reference type qualifier"), s.line, s.col);
    }
    Nullability nullable = NonNullable;
    size_t i = 1;
    if (size == 3) {
      nullable = Nullable;
      i++;
    }
    return Type(parseHeapType(*s[i]), nullable);
  }
  // It's a tuple.
  std::vector<Type> types;
  for (size_t i = 0; i < s.size(); ++i) {
    types.push_back(elementToType(*list[i]));
  }
  return Type(types);
}

Type SExpressionWasmBuilder::stringToLaneType(const char* str) {
  if (strcmp(str, "i8x16") == 0) {
    return Type::i32;
  }
  if (strcmp(str, "i16x8") == 0) {
    return Type::i32;
  }
  if (strcmp(str, "i32x4") == 0) {
    return Type::i32;
  }
  if (strcmp(str, "i64x2") == 0) {
    return Type::i64;
  }
  if (strcmp(str, "f32x4") == 0) {
    return Type::f32;
  }
  if (strcmp(str, "f64x2") == 0) {
    return Type::f64;
  }
  return Type::none;
}

HeapType SExpressionWasmBuilder::getFunctionType(Name name, Element& s) {
  auto iter = functionTypes.find(name);
  if (iter == functionTypes.end()) {
    throw ParseException(
      "invalid call target: " + std::string(name.str), s.line, s.col);
  }
  return iter->second;
}

Function::DebugLocation
SExpressionWasmBuilder::getDebugLocation(const SourceLocation& loc) {
  IString file = loc.filename;
  auto& debugInfoFileNames = wasm.debugInfoFileNames;
  auto iter = debugInfoFileIndices.find(file);
  if (iter == debugInfoFileIndices.end()) {
    Index index = debugInfoFileNames.size();
    debugInfoFileNames.push_back(file.toString());
    debugInfoFileIndices[file] = index;
  }
  uint32_t fileIndex = debugInfoFileIndices[file];
  return {fileIndex, loc.line, loc.column};
}

Expression* SExpressionWasmBuilder::parseExpression(Element& s) {
  Expression* result = makeExpression(s);
  if (s.startLoc && currFunction) {
    currFunction->debugLocations[result] = getDebugLocation(*s.startLoc);
  }
  return result;
}

Expression* SExpressionWasmBuilder::makeExpression(Element& s){
#define INSTRUCTION_PARSER
#include "gen-s-parser.inc"
}

Expression* SExpressionWasmBuilder::makeUnreachable() {
  return allocator.alloc<Unreachable>();
}

Expression* SExpressionWasmBuilder::makeNop() { return allocator.alloc<Nop>(); }

Expression* SExpressionWasmBuilder::makeBinary(Element& s, BinaryOp op) {
  auto ret = allocator.alloc<Binary>();
  ret->op = op;
  ret->left = parseExpression(s[1]);
  ret->right = parseExpression(s[2]);
  ret->finalize();
  return ret;
}

Expression* SExpressionWasmBuilder::makeUnary(Element& s, UnaryOp op) {
  auto ret = allocator.alloc<Unary>();
  ret->op = op;
  ret->value = parseExpression(s[1]);
  ret->finalize();
  return ret;
}

Expression* SExpressionWasmBuilder::makeSelect(Element& s) {
  auto ret = allocator.alloc<Select>();
  Index i = 1;
  Type type = parseOptionalResultType(s, i);
  ret->ifTrue = parseExpression(s[i++]);
  ret->ifFalse = parseExpression(s[i++]);
  ret->condition = parseExpression(s[i]);
  if (type.isConcrete()) {
    ret->finalize(type);
  } else {
    ret->finalize();
  }
  return ret;
}

Expression* SExpressionWasmBuilder::makeDrop(Element& s) {
  auto ret = allocator.alloc<Drop>();
  ret->value = parseExpression(s[1]);
  ret->finalize();
  return ret;
}

Expression* SExpressionWasmBuilder::makeMemorySize(Element& s) {
  auto ret = allocator.alloc<MemorySize>();
  Index i = 1;
  Name memory;
  if (s.size() > 1) {
    memory = getMemoryName(*s[i++]);
  } else {
    memory = getMemoryNameAtIdx(0);
  }
  ret->memory = memory;
  if (isMemory64(memory)) {
    ret->make64();
  }
  ret->finalize();
  return ret;
}

Expression* SExpressionWasmBuilder::makeMemoryGrow(Element& s) {
  auto ret = allocator.alloc<MemoryGrow>();
  Index i = 1;
  Name memory;
  if (s.size() > 2) {
    memory = getMemoryName(*s[i++]);
  } else {
    memory = getMemoryNameAtIdx(0);
  }
  ret->memory = memory;
  if (isMemory64(memory)) {
    ret->make64();
  }
  ret->delta = parseExpression(s[i]);
  ret->finalize();
  return ret;
}

Index SExpressionWasmBuilder::getLocalIndex(Element& s) {
  if (!currFunction) {
    throw ParseException("local access in non-function scope", s.line, s.col);
  }
  if (s.dollared()) {
    auto ret = s.str();
    if (currFunction->localIndices.count(ret) == 0) {
      throw ParseException("bad local name", s.line, s.col);
    }
    return currFunction->getLocalIndex(ret);
  }
  // this is a numeric index
  Index ret = parseIndex(s);
  if (ret >= currFunction->getNumLocals()) {
    throw ParseException("bad local index", s.line, s.col);
  }
  return ret;
}

Expression* SExpressionWasmBuilder::makeLocalGet(Element& s) {
  auto ret = allocator.alloc<LocalGet>();
  ret->index = getLocalIndex(*s[1]);
  ret->type = currFunction->getLocalType(ret->index);
  return ret;
}

Expression* SExpressionWasmBuilder::makeLocalTee(Element& s) {
  auto ret = allocator.alloc<LocalSet>();
  ret->index = getLocalIndex(*s[1]);
  ret->value = parseExpression(s[2]);
  ret->makeTee(currFunction->getLocalType(ret->index));
  ret->finalize();
  return ret;
}

Expression* SExpressionWasmBuilder::makeLocalSet(Element& s) {
  auto ret = allocator.alloc<LocalSet>();
  ret->index = getLocalIndex(*s[1]);
  ret->value = parseExpression(s[2]);
  ret->makeSet();
  ret->finalize();
  return ret;
}

Expression* SExpressionWasmBuilder::makeGlobalGet(Element& s) {
  auto ret = allocator.alloc<GlobalGet>();
  ret->name = getGlobalName(*s[1]);
  auto* global = wasm.getGlobalOrNull(ret->name);
  if (!global) {
    throw ParseException("bad global.get name", s.line, s.col);
  }
  ret->type = global->type;
  return ret;
}

Expression* SExpressionWasmBuilder::makeGlobalSet(Element& s) {
  auto ret = allocator.alloc<GlobalSet>();
  ret->name = getGlobalName(*s[1]);
  if (wasm.getGlobalOrNull(ret->name) &&
      !wasm.getGlobalOrNull(ret->name)->mutable_) {
    throw ParseException("global.set of immutable", s.line, s.col);
  }
  ret->value = parseExpression(s[2]);
  ret->finalize();
  return ret;
}

Expression* SExpressionWasmBuilder::makeBlock(Element& s) {
  if (!currFunction) {
    throw ParseException(
      "block is unallowed outside of functions", s.line, s.col);
  }
  // special-case Block, because Block nesting (in their first element) can be
  // incredibly deep
  auto curr = allocator.alloc<Block>();
  auto* sp = &s;
  // The information we need for the stack of blocks here is the element we are
  // converting, the block we are converting it to, and whether it originally
  // had a name or not (which will be useful later).
  struct Info {
    Element* element;
    Block* block;
    bool hadName;
  };
  std::vector<Info> stack;
  while (1) {
    auto& s = *sp;
    Index i = 1;
    Name sName;
    bool hadName = false;
    if (i < s.size() && s[i]->isStr()) {
      // could be a name or a type
      if (s[i]->dollared() ||
          stringToType(s[i]->str(), true /* allowError */) == Type::none) {
        sName = s[i++]->str();
        hadName = true;
      } else {
        sName = "block";
      }
    } else {
      sName = "block";
    }
    stack.emplace_back(Info{sp, curr, hadName});
    curr->name = nameMapper.pushLabelName(sName);
    // block signature
    curr->type = parseOptionalResultType(s, i);
    if (i >= s.size()) {
      break; // empty block
    }
    auto& first = *s[i];
    if (elementStartsWith(first, BLOCK)) {
      // recurse
      curr = allocator.alloc<Block>();
      if (first.startLoc) {
        currFunction->debugLocations[curr] = getDebugLocation(*first.startLoc);
      }
      sp = &first;
      continue;
    }
    break;
  }
  // we now have a stack of Blocks, with their labels, but no contents yet
  for (int t = int(stack.size()) - 1; t >= 0; t--) {
    auto* sp = stack[t].element;
    auto* curr = stack[t].block;
    auto hadName = stack[t].hadName;
    auto& s = *sp;
    size_t i = 1;
    if (i < s.size()) {
      while (i < s.size() && s[i]->isStr()) {
        i++;
      }
      if (i < s.size() && elementStartsWith(*s[i], RESULT)) {
        i++;
      }
      if (t < int(stack.size()) - 1) {
        // first child is one of our recursions
        curr->list.push_back(stack[t + 1].block);
        i++;
      }
      for (; i < s.size(); i++) {
        curr->list.push_back(parseExpression(s[i]));
      }
    }
    nameMapper.popLabelName(curr->name);
    curr->finalize(curr->type);
    // If the block never had a name, and one was not needed in practice (even
    // if one did not exist, perhaps a break targeted it by index), then we can
    // remove the name. Note that we only do this if it never had a name: if it
    // did, we don't want to change anything; we just want to be the same as
    // the code we are loading - if there was no name before, we don't want one
    // now, so that we roundtrip text precisely.
    if (!hadName && !BranchUtils::BranchSeeker::has(curr, curr->name)) {
      curr->name = Name();
    }
  }
  return stack[0].block;
}

// Similar to block, but the label is handled by the enclosing if (since there
// might not be a then or else, ick)
Expression* SExpressionWasmBuilder::makeThenOrElse(Element& s) {
  auto ret = allocator.alloc<Block>();
  size_t i = 1;
  if (s[1]->isStr()) {
    i++;
  }
  for (; i < s.size(); i++) {
    ret->list.push_back(parseExpression(s[i]));
  }
  ret->finalize();
  return ret;
}

static Expression* parseConst(IString s, Type type, MixedArena& allocator) {
  const char* str = s.str.data();
  auto ret = allocator.alloc<Const>();
  ret->type = type;
  if (type.isFloat()) {
    if (s == _INFINITY) {
      switch (type.getBasic()) {
        case Type::f32:
          ret->value = Literal(std::numeric_limits<float>::infinity());
          break;
        case Type::f64:
          ret->value = Literal(std::numeric_limits<double>::infinity());
          break;
        default:
          return nullptr;
      }
      return ret;
    }
    if (s == NEG_INFINITY) {
      switch (type.getBasic()) {
        case Type::f32:
          ret->value = Literal(-std::numeric_limits<float>::infinity());
          break;
        case Type::f64:
          ret->value = Literal(-std::numeric_limits<double>::infinity());
          break;
        default:
          return nullptr;
      }
      return ret;
    }
    if (s == _NAN) {
      switch (type.getBasic()) {
        case Type::f32:
          ret->value = Literal(float(std::nan("")));
          break;
        case Type::f64:
          ret->value = Literal(double(std::nan("")));
          break;
        default:
          return nullptr;
      }
      return ret;
    }
    bool negative = str[0] == '-';
    const char* positive = negative ? str + 1 : str;
    if (!negative) {
      if (positive[0] == '+') {
        positive++;
      }
    }
    if (positive[0] == 'n' && positive[1] == 'a' && positive[2] == 'n') {
      const char* modifier = positive[3] == ':' ? positive + 4 : nullptr;
      if (!(modifier ? positive[4] == '0' && positive[5] == 'x' : 1)) {
        throw ParseException("bad nan input");
      }
      switch (type.getBasic()) {
        case Type::f32: {
          uint32_t pattern;
          if (modifier) {
            std::istringstream istr(modifier);
            istr >> std::hex >> pattern;
            if (istr.fail()) {
              throw ParseException("invalid f32 format");
            }
            pattern |= 0x7f800000U;
          } else {
            pattern = 0x7fc00000U;
          }
          if (negative) {
            pattern |= 0x80000000U;
          }
          if (!std::isnan(bit_cast<float>(pattern))) {
            pattern |= 1U;
          }
          ret->value = Literal(pattern).castToF32();
          break;
        }
        case Type::f64: {
          uint64_t pattern;
          if (modifier) {
            std::istringstream istr(modifier);
            istr >> std::hex >> pattern;
            if (istr.fail()) {
              throw ParseException("invalid f64 format");
            }
            pattern |= 0x7ff0000000000000ULL;
          } else {
            pattern = 0x7ff8000000000000UL;
          }
          if (negative) {
            pattern |= 0x8000000000000000ULL;
          }
          if (!std::isnan(bit_cast<double>(pattern))) {
            pattern |= 1ULL;
          }
          ret->value = Literal(pattern).castToF64();
          break;
        }
        default:
          return nullptr;
      }
      // std::cerr << "make constant " << str << " ==> " << ret->value << '\n';
      return ret;
    }
    if (s == NEG_NAN) {
      switch (type.getBasic()) {
        case Type::f32:
          ret->value = Literal(float(-std::nan("")));
          break;
        case Type::f64:
          ret->value = Literal(double(-std::nan("")));
          break;
        default:
          return nullptr;
      }
      // std::cerr << "make constant " << str << " ==> " << ret->value << '\n';
      return ret;
    }
  }
  switch (type.getBasic()) {
    case Type::i32: {
      if ((str[0] == '0' && str[1] == 'x') ||
          (str[0] == '-' && str[1] == '0' && str[2] == 'x')) {
        bool negative = str[0] == '-';
        if (negative) {
          str++;
        }
        std::istringstream istr(str);
        uint32_t temp;
        istr >> std::hex >> temp;
        if (istr.fail()) {
          throw ParseException("invalid i32 format");
        }
        ret->value = Literal(negative ? -temp : temp);
      } else {
        std::istringstream istr(str[0] == '-' ? str + 1 : str);
        uint32_t temp;
        istr >> temp;
        if (istr.fail()) {
          throw ParseException("invalid i32 format");
        }
        ret->value = Literal(str[0] == '-' ? -temp : temp);
      }
      break;
    }
    case Type::i64: {
      if ((str[0] == '0' && str[1] == 'x') ||
          (str[0] == '-' && str[1] == '0' && str[2] == 'x')) {
        bool negative = str[0] == '-';
        if (negative) {
          str++;
        }
        std::istringstream istr(str);
        uint64_t temp;
        istr >> std::hex >> temp;
        if (istr.fail()) {
          throw ParseException("invalid i64 format");
        }
        ret->value = Literal(negative ? -temp : temp);
      } else {
        std::istringstream istr(str[0] == '-' ? str + 1 : str);
        uint64_t temp;
        istr >> temp;
        if (istr.fail()) {
          throw ParseException("invalid i64 format");
        }
        ret->value = Literal(str[0] == '-' ? -temp : temp);
      }
      break;
    }
    case Type::f32: {
      char* end;
      ret->value = Literal(strtof(str, &end));
      break;
    }
    case Type::f64: {
      char* end;
      ret->value = Literal(strtod(str, &end));
      break;
    }
    case Type::v128:
      WASM_UNREACHABLE("unexpected const type");
    case Type::none:
    case Type::unreachable: {
      return nullptr;
    }
  }
  if (ret->value.type != type) {
    throw ParseException("parsed type does not match expected type");
  }
  return ret;
}

template<int Lanes>
static Literal makeLanes(Element& s, MixedArena& allocator, Type lane_t) {
  std::array<Literal, Lanes> lanes;
  for (size_t i = 0; i < Lanes; ++i) {
    Expression* lane = parseConst(s[i + 2]->str(), lane_t, allocator);
    if (lane) {
      lanes[i] = lane->cast<Const>()->value;
    } else {
      throw ParseException(
        "Could not parse v128 lane", s[i + 2]->line, s[i + 2]->col);
    }
  }
  return Literal(lanes);
}

Expression* SExpressionWasmBuilder::makeConst(Element& s, Type type) {
  if (type != Type::v128) {
    auto ret = parseConst(s[1]->str(), type, allocator);
    if (!ret) {
      throw ParseException("bad const", s[1]->line, s[1]->col);
    }
    return ret;
  }

  auto ret = allocator.alloc<Const>();
  Type lane_t = stringToLaneType(s[1]->str().str.data());
  size_t lanes = s.size() - 2;
  switch (lanes) {
    case 2: {
      if (lane_t != Type::i64 && lane_t != Type::f64) {
        throw ParseException(
          "Unexpected v128 literal lane type", s[1]->line, s[1]->col);
      }
      ret->value = makeLanes<2>(s, allocator, lane_t);
      break;
    }
    case 4: {
      if (lane_t != Type::i32 && lane_t != Type::f32) {
        throw ParseException(
          "Unexpected v128 literal lane type", s[1]->line, s[1]->col);
      }
      ret->value = makeLanes<4>(s, allocator, lane_t);
      break;
    }
    case 8: {
      if (lane_t != Type::i32) {
        throw ParseException(
          "Unexpected v128 literal lane type", s[1]->line, s[1]->col);
      }
      ret->value = makeLanes<8>(s, allocator, lane_t);
      break;
    }
    case 16: {
      if (lane_t != Type::i32) {
        throw ParseException(
          "Unexpected v128 literal lane type", s[1]->line, s[1]->col);
      }
      ret->value = makeLanes<16>(s, allocator, lane_t);
      break;
    }
    default:
      throw ParseException(
        "Unexpected number of lanes in v128 literal", s[1]->line, s[1]->col);
  }
  ret->finalize();
  return ret;
}

static size_t parseMemAttributes(size_t i,
                                 Element& s,
                                 Address& offset,
                                 Address& align,
                                 bool memory64) {
  // Parse "align=X" and "offset=X" arguments, bailing out on anything else.
  while (!s[i]->isList()) {
    const char* str = s[i]->str().str.data();
    if (strncmp(str, "align", 5) != 0 && strncmp(str, "offset", 6) != 0) {
      return i;
    }
    const char* eq = strchr(str, '=');
    if (!eq) {
      throw ParseException(
        "missing = in memory attribute", s[i]->line, s[i]->col);
    }
    eq++;
    if (*eq == 0) {
      throw ParseException(
        "missing value in memory attribute", s[i]->line, s[i]->col);
    }
    char* endptr;
    uint64_t value = strtoll(eq, &endptr, 10);
    if (*endptr != 0) {
      throw ParseException(
        "bad memory attribute immediate", s[i]->line, s[i]->col);
    }
    if (str[0] == 'a') {
      if (value > std::numeric_limits<uint32_t>::max()) {
        throw ParseException("bad align", s[i]->line, s[i]->col);
      }
      align = value;
    } else if (str[0] == 'o') {
      if (!memory64 && value > std::numeric_limits<uint32_t>::max()) {
        throw ParseException("bad offset", s[i]->line, s[i]->col);
      }
      offset = value;
    } else {
      throw ParseException("bad memory attribute", s[i]->line, s[i]->col);
    }
    i++;
  }
  return i;
}

bool SExpressionWasmBuilder::hasMemoryIdx(Element& s,
                                          Index defaultSize,
                                          Index i) {
  if (s.size() > defaultSize && !s[i]->isList() &&
      strncmp(s[i]->str().str.data(), "align", 5) != 0 &&
      strncmp(s[i]->str().str.data(), "offset", 6) != 0) {
    return true;
  }
  return false;
}

Expression* SExpressionWasmBuilder::makeLoad(
  Element& s, Type type, bool signed_, int bytes, bool isAtomic) {
  auto* ret = allocator.alloc<Load>();
  ret->type = type;
  ret->bytes = bytes;
  ret->signed_ = signed_;
  ret->offset = 0;
  ret->align = bytes;
  ret->isAtomic = isAtomic;
  Index i = 1;
  Name memory;
  // Check to make sure there are more than the default args & this str isn't
  // the mem attributes
  if (hasMemoryIdx(s, 2, i)) {
    memory = getMemoryName(*s[i++]);
  } else {
    memory = getMemoryNameAtIdx(0);
  }
  ret->memory = memory;
  i = parseMemAttributes(i, s, ret->offset, ret->align, isMemory64(memory));
  ret->ptr = parseExpression(s[i]);
  ret->finalize();
  return ret;
}

Expression* SExpressionWasmBuilder::makeStore(Element& s,
                                              Type type,
                                              int bytes,
                                              bool isAtomic) {
  auto ret = allocator.alloc<Store>();
  ret->bytes = bytes;
  ret->offset = 0;
  ret->align = bytes;
  ret->isAtomic = isAtomic;
  ret->valueType = type;
  Index i = 1;
  Name memory;
  // Check to make sure there are more than the default args & this str isn't
  // the mem attributes
  if (hasMemoryIdx(s, 3, i)) {
    memory = getMemoryName(*s[i++]);
  } else {
    memory = getMemoryNameAtIdx(0);
  }
  ret->memory = memory;
  i = parseMemAttributes(i, s, ret->offset, ret->align, isMemory64(memory));
  ret->ptr = parseExpression(s[i]);
  ret->value = parseExpression(s[i + 1]);
  ret->finalize();
  return ret;
}

Expression* SExpressionWasmBuilder::makeAtomicRMW(Element& s,
                                                  AtomicRMWOp op,
                                                  Type type,
                                                  uint8_t bytes) {
  auto ret = allocator.alloc<AtomicRMW>();
  ret->type = type;
  ret->op = op;
  ret->bytes = bytes;
  ret->offset = 0;
  Index i = 1;
  Name memory;
  // Check to make sure there are more than the default args & this str isn't
  // the mem attributes
  if (hasMemoryIdx(s, 3, i)) {
    memory = getMemoryName(*s[i++]);
  } else {
    memory = getMemoryNameAtIdx(0);
  }
  ret->memory = memory;
  Address align = bytes;
  i = parseMemAttributes(i, s, ret->offset, align, isMemory64(memory));
  if (align != ret->bytes) {
    throw ParseException("Align of Atomic RMW must match size", s.line, s.col);
  }
  ret->ptr = parseExpression(s[i]);
  ret->value = parseExpression(s[i + 1]);
  ret->finalize();
  return ret;
}

Expression* SExpressionWasmBuilder::makeAtomicCmpxchg(Element& s,
                                                      Type type,
                                                      uint8_t bytes) {
  auto ret = allocator.alloc<AtomicCmpxchg>();
  ret->type = type;
  ret->bytes = bytes;
  ret->offset = 0;
  Index i = 1;
  Name memory;
  // Check to make sure there are more than the default args & this str isn't
  // the mem attributes
  if (hasMemoryIdx(s, 4, i)) {
    memory = getMemoryName(*s[i++]);
  } else {
    memory = getMemoryNameAtIdx(0);
  }
  ret->memory = memory;
  Address align = ret->bytes;
  i = parseMemAttributes(i, s, ret->offset, align, isMemory64(memory));
  if (align != ret->bytes) {
    throw ParseException(
      "Align of Atomic Cmpxchg must match size", s.line, s.col);
  }
  ret->ptr = parseExpression(s[i]);
  ret->expected = parseExpression(s[i + 1]);
  ret->replacement = parseExpression(s[i + 2]);
  ret->finalize();
  return ret;
}

Expression* SExpressionWasmBuilder::makeAtomicWait(Element& s, Type type) {
  auto ret = allocator.alloc<AtomicWait>();
  ret->type = Type::i32;
  ret->offset = 0;
  ret->expectedType = type;
  Index i = 1;
  Name memory;
  // Check to make sure there are more than the default args & this str isn't
  // the mem attributes
  if (hasMemoryIdx(s, 4, i)) {
    memory = getMemoryName(*s[i++]);
  } else {
    memory = getMemoryNameAtIdx(0);
  }
  ret->memory = memory;
  Address expectedAlign = type == Type::i64 ? 8 : 4;
  Address align = expectedAlign;
  i = parseMemAttributes(i, s, ret->offset, align, isMemory64(memory));
  if (align != expectedAlign) {
    throw ParseException(
      "Align of memory.atomic.wait must match size", s.line, s.col);
  }
  ret->ptr = parseExpression(s[i]);
  ret->expected = parseExpression(s[i + 1]);
  ret->timeout = parseExpression(s[i + 2]);
  ret->finalize();
  return ret;
}

Expression* SExpressionWasmBuilder::makeAtomicNotify(Element& s) {
  auto ret = allocator.alloc<AtomicNotify>();
  ret->type = Type::i32;
  ret->offset = 0;
  Index i = 1;
  Name memory;
  // Check to make sure there are more than the default args & this str isn't
  // the mem attributes
  if (hasMemoryIdx(s, 3, i)) {
    memory = getMemoryName(*s[i++]);
  } else {
    memory = getMemoryNameAtIdx(0);
  }
  ret->memory = memory;
  Address align = 4;
  i = parseMemAttributes(i, s, ret->offset, align, isMemory64(memory));
  if (align != 4) {
    throw ParseException(
      "Align of memory.atomic.notify must be 4", s.line, s.col);
  }
  ret->ptr = parseExpression(s[i]);
  ret->notifyCount = parseExpression(s[i + 1]);
  ret->finalize();
  return ret;
}

Expression* SExpressionWasmBuilder::makeAtomicFence(Element& s) {
  return allocator.alloc<AtomicFence>();
}

static uint8_t parseLaneIndex(const Element* s, size_t lanes) {
  const char* str = s->str().str.data();
  char* end;
  auto n = static_cast<unsigned long long>(strtoll(str, &end, 10));
  if (end == str || *end != '\0') {
    throw ParseException("Expected lane index", s->line, s->col);
  }
  if (n > lanes) {
    throw ParseException(
      "lane index must be less than " + std::to_string(lanes), s->line, s->col);
  }
  return uint8_t(n);
}

Expression* SExpressionWasmBuilder::makeSIMDExtract(Element& s,
                                                    SIMDExtractOp op,
                                                    size_t lanes) {
  auto ret = allocator.alloc<SIMDExtract>();
  ret->op = op;
  ret->index = parseLaneIndex(s[1], lanes);
  ret->vec = parseExpression(s[2]);
  ret->finalize();
  return ret;
}

Expression* SExpressionWasmBuilder::makeSIMDReplace(Element& s,
                                                    SIMDReplaceOp op,
                                                    size_t lanes) {
  auto ret = allocator.alloc<SIMDReplace>();
  ret->op = op;
  ret->index = parseLaneIndex(s[1], lanes);
  ret->vec = parseExpression(s[2]);
  ret->value = parseExpression(s[3]);
  ret->finalize();
  return ret;
}

Expression* SExpressionWasmBuilder::makeSIMDShuffle(Element& s) {
  auto ret = allocator.alloc<SIMDShuffle>();
  for (size_t i = 0; i < 16; ++i) {
    ret->mask[i] = parseLaneIndex(s[i + 1], 32);
  }
  ret->left = parseExpression(s[17]);
  ret->right = parseExpression(s[18]);
  ret->finalize();
  return ret;
}

Expression* SExpressionWasmBuilder::makeSIMDTernary(Element& s,
                                                    SIMDTernaryOp op) {
  auto ret = allocator.alloc<SIMDTernary>();
  ret->op = op;
  ret->a = parseExpression(s[1]);
  ret->b = parseExpression(s[2]);
  ret->c = parseExpression(s[3]);
  ret->finalize();
  return ret;
}

Expression* SExpressionWasmBuilder::makeSIMDShift(Element& s, SIMDShiftOp op) {
  auto ret = allocator.alloc<SIMDShift>();
  ret->op = op;
  ret->vec = parseExpression(s[1]);
  ret->shift = parseExpression(s[2]);
  ret->finalize();
  return ret;
}

Expression*
SExpressionWasmBuilder::makeSIMDLoad(Element& s, SIMDLoadOp op, int bytes) {
  auto ret = allocator.alloc<SIMDLoad>();
  ret->op = op;
  ret->offset = 0;
  ret->align = bytes;
  Index i = 1;
  Name memory;
  // Check to make sure there are more than the default args & this str isn't
  // the mem attributes
  if (hasMemoryIdx(s, 2, i)) {
    memory = getMemoryName(*s[i++]);
  } else {
    memory = getMemoryNameAtIdx(0);
  }
  ret->memory = memory;
  i = parseMemAttributes(i, s, ret->offset, ret->align, isMemory64(memory));
  ret->ptr = parseExpression(s[i]);
  ret->finalize();
  return ret;
}

Expression* SExpressionWasmBuilder::makeSIMDLoadStoreLane(
  Element& s, SIMDLoadStoreLaneOp op, int bytes) {
  auto* ret = allocator.alloc<SIMDLoadStoreLane>();
  ret->op = op;
  ret->offset = 0;
  ret->align = bytes;
  size_t lanes;
  switch (op) {
    case Load8LaneVec128:
    case Store8LaneVec128:
      lanes = 16;
      break;
    case Load16LaneVec128:
    case Store16LaneVec128:
      lanes = 8;
      break;
    case Load32LaneVec128:
    case Store32LaneVec128:
      lanes = 4;
      break;
    case Load64LaneVec128:
    case Store64LaneVec128:
      lanes = 2;
      break;
    default:
      WASM_UNREACHABLE("Unexpected SIMDLoadStoreLane op");
  }
  Index i = 1;
  Name memory;
  // Check to make sure there are more than the default args & this str isn't
  // the mem attributes
  if (hasMemoryIdx(s, 4, i)) {
    memory = getMemoryName(*s[i++]);
  } else {
    memory = getMemoryNameAtIdx(0);
  }
  ret->memory = memory;
  i = parseMemAttributes(i, s, ret->offset, ret->align, isMemory64(memory));
  ret->index = parseLaneIndex(s[i++], lanes);
  ret->ptr = parseExpression(s[i++]);
  ret->vec = parseExpression(s[i]);
  ret->finalize();
  return ret;
}

Expression* SExpressionWasmBuilder::makeMemoryInit(Element& s) {
  auto ret = allocator.alloc<MemoryInit>();
  Index i = 1;
  Name memory;
  if (s.size() > 5) {
    memory = getMemoryName(*s[i++]);
  } else {
    memory = getMemoryNameAtIdx(0);
  }
  ret->memory = memory;
  ret->segment = parseIndex(*s[i++]);
  ret->dest = parseExpression(s[i++]);
  ret->offset = parseExpression(s[i++]);
  ret->size = parseExpression(s[i]);
  ret->finalize();
  return ret;
}

Expression* SExpressionWasmBuilder::makeDataDrop(Element& s) {
  auto ret = allocator.alloc<DataDrop>();
  ret->segment = parseIndex(*s[1]);
  ret->finalize();
  return ret;
}

Expression* SExpressionWasmBuilder::makeMemoryCopy(Element& s) {
  auto ret = allocator.alloc<MemoryCopy>();
  Index i = 1;
  Name destMemory;
  Name sourceMemory;
  if (s.size() > 4) {
    destMemory = getMemoryName(*s[i++]);
    sourceMemory = getMemoryName(*s[i++]);
  } else {
    destMemory = getMemoryNameAtIdx(0);
    sourceMemory = getMemoryNameAtIdx(0);
  }
  ret->destMemory = destMemory;
  ret->sourceMemory = sourceMemory;
  ret->dest = parseExpression(s[i++]);
  ret->source = parseExpression(s[i++]);
  ret->size = parseExpression(s[i]);
  ret->finalize();
  return ret;
}

Expression* SExpressionWasmBuilder::makeMemoryFill(Element& s) {
  auto ret = allocator.alloc<MemoryFill>();
  Index i = 1;
  Name memory;
  if (s.size() > 4) {
    memory = getMemoryName(*s[i++]);
  } else {
    memory = getMemoryNameAtIdx(0);
  }
  ret->memory = memory;
  ret->dest = parseExpression(s[i++]);
  ret->value = parseExpression(s[i++]);
  ret->size = parseExpression(s[i]);
  ret->finalize();
  return ret;
}

Expression* SExpressionWasmBuilder::makePop(Element& s) {
  auto ret = allocator.alloc<Pop>();
  std::vector<Type> types;
  for (size_t i = 1; i < s.size(); ++i) {
    types.push_back(elementToType(*s[i]));
  }
  ret->type = Type(types);
  ret->finalize();
  return ret;
}

Expression* SExpressionWasmBuilder::makeIf(Element& s) {
  auto ret = allocator.alloc<If>();
  Index i = 1;
  Name sName;
  if (s[i]->dollared()) {
    // the if is labeled
    sName = s[i++]->str();
  } else {
    sName = "if";
  }
  auto label = nameMapper.pushLabelName(sName);
  // if signature
  Type type = parseOptionalResultType(s, i);
  ret->condition = parseExpression(s[i++]);
  ret->ifTrue = parseExpression(*s[i++]);
  if (i < s.size()) {
    ret->ifFalse = parseExpression(*s[i++]);
  }
  ret->finalize(type);
  nameMapper.popLabelName(label);
  // create a break target if we must
  if (BranchUtils::BranchSeeker::has(ret, label)) {
    auto* block = allocator.alloc<Block>();
    block->name = label;
    block->list.push_back(ret);
    block->finalize(type);
    return block;
  }
  return ret;
}

Expression*
SExpressionWasmBuilder::makeMaybeBlock(Element& s, size_t i, Type type) {
  Index stopAt = -1;
  if (s.size() == i) {
    return allocator.alloc<Nop>();
  }
  if (s.size() == i + 1) {
    return parseExpression(s[i]);
  }
  auto ret = allocator.alloc<Block>();
  for (; i < s.size() && i < stopAt; i++) {
    ret->list.push_back(parseExpression(s[i]));
  }
  ret->finalize(type);
  // Note that we do not name these implicit/synthetic blocks. They
  // are the effects of syntactic sugar, and nothing can branch to
  // them anyhow.
 