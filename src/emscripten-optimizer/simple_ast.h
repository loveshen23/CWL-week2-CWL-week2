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

#ifndef wasm_simple_ast_h
#define wasm_simple_ast_h

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <ostream>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "mixed_arena.h"
#include "parser.h"
#include "snprintf.h"
#include "support/safe_integer.h"

#define err(str) fprintf(stderr, str "\n");
#define errv(str, ...) fprintf(stderr, str "\n", __VA_ARGS__);
#define printErr err

namespace cashew {

struct Value;
struct Ref;

void dump(const char* str, Ref node, bool pretty = false);

// Reference to a value, plus some operators for convenience
struct Ref {
  Value* inst;

  Ref(Value* v = nullptr) : inst(v) {}

  Value* get() { return inst; }

  Value& operator*() { return *inst; }
  Value* operator->() { return inst; }
  Ref& operator[](unsigned x);
  Ref& operator[](IString x);

  // special conveniences
  bool
  operator==(std::string_view str); // comparison to string, which is by value
  bool operator!=(std::string_view str);
  bool operator==(const IString& str);
  bool operator!=(const IString& str);
  // prevent Ref == number, which is potentially ambiguous; use ->getNumber() ==
  // number
  bool operator==(double d) {
    abort();
    return false;
  }
  bool operator==(Ref other);
  bool operator!(); // check if null, in effect
};

// Arena allocation, free it all on process exit

// A mixed arena for global allocation only, so members do not
// receive an allocator, they all use the global one anyhow
class GlobalMixedArena : public MixedArena {
public:
  template<class T> T* alloc() {
    auto* ret = static_cast<T*>(allocSpace(sizeof(T), alignof(T)));
    new (ret) T();
    return ret;
  }
};

extern GlobalMixedArena arena;

class ArrayStorage : public ArenaVectorBase<ArrayStorage, Ref> {
public:
  void allocate(size_t size) {
    allocatedElements = size;
    data = static_cast<Ref*>(
      arena.allocSpace(sizeof(Ref) * allocatedElements, alignof(Ref)));
  }
};

struct Assign;
struct AssignName;

// Main value type
struct Value {
  enum Type {
    String = 0,
    Number = 1,
    Array = 2,
    Null = 3,
    Bool = 4,
    Object = 5,
    Assign_ = 6, // ref = target
    AssignName_ = 7
  };

  Type type = Null;

  using ObjectStorage = std::unordered_map<IString, Ref>;

  // MSVC does not allow unrestricted unions:
  // http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2008/n2544.pdf
#ifdef _MSC_VER
  IString str;
#endif
  union { // TODO: optimize
#ifndef _MSC_VER
    IString str;
#endif
    double num;
    ArrayStorage* arr;
    bool boo;
    ObjectStorage* obj;
    Ref ref;
  };

  // constructors all copy their input
  Value() {}
  explicit Value(const char* s) { setString(s); }
  explicit Value(double n) { setNumber(n); }
  explicit Value(ArrayStorage& a) {
    setArray();
    *arr = a;
  }
  // no bool constructor - would endanger the double one (int might convert the
  // wrong way)

  ~Value() { free(); }

  void free() {
    if (type == Array) {
      arr->clear();
    } else if (type == Object) {
      delete obj;
    }
    type = Null;
    num = 0;
  }

  Value& setString(const char* s) {
    free();
    type = String;
    str = IString(s);
    return *this;
  }
  Value& setString(const IString& s) {
    free();
    type = String;
    str = s;
    return *this;
  }
  Value& setNumber(double n) {
    free();
    type = Number;
    num = n;
    return *this;
  }
  Value& setArray(ArrayStorage& a) {
    free();
    type = Array;
    arr = arena.alloc<ArrayStorage>();
    *arr = a;
    return *this;
  }
  Value& setArray(size_t size_hint = 0) {
    free();
    type = Array;
    arr = arena.alloc<ArrayStorage>();
    arr->reserve(size_hint);
    return *this;
  }
  Value& setNull() {
    free();
    type = Null;
    return *this;
  }
  // Bool in the name, as otherwise might overload over int
  Value& setBool(bool b) {
    free();
    type = Bool;
    boo = b;
    return *this;
  }
  Value& setObject() {
    free();
    type = Object;
    obj = new ObjectStorage();
    return *this;
  }
  Value& setAssign(Ref target, Ref value);
  Value& setAssignName(IString target, Ref value);

  bool isString() { return type == String; }
  bool isNumber() { return type == Number; }
  bool isArray() { return type == Array; }
  bool isNull() { return type == Null; }
  bool isBool() { return type == Bool; }
  bool isObject() { return type == Object; }
  bool isAssign() { return type == Assign_; }
  bool isAssignName() { return type == AssignName_; }

  // avoid overloading == as it might overload over int
  bool isBool(bool b) { return type == Bool && b == boo; }

  // convenience function to check if something is an array and
  // also has a certain string as the first element. This is a
  // very common operation as the first element defines the node
  // type for most ast nodes
  bool isArray(IString name) { return isArray() && (*this)[0] == name; }

  const char* getCString() {
    assert(isString());
    return str.str.data();
  }
  IString& getIString() {
    assert(isString());
    return str;
  }
  double& getNumber() {
    assert(isNumber());
    return num;
  }
  ArrayStorage& getArray() {
    assert(isArray());
    return *arr;
  }
  bool& getBool() {
    assert(isBool());
    return boo;
  }

  Assign* asAssign();
  AssignName* asAssignName();

  int32_t getInteger() { // convenience function to get a known integer
    assert(wasm::isInteger(getNumber()));
    int32_t ret = getNumber();
    assert(double(ret) == getNumber()); // no loss in conversion
    return ret;
  }

  Value& operator=(const Value& other) {
    free();
    switch (other.type) {
      case String:
        setString(other.str);
        break;
      case Number:
        setNumber(other.num);
        break;
      case Array:
        setArray(*other.arr);
        break;
      case Null:
        setNull();
        break;
      case Bool:
        setBool(other.boo);
        break;
      default:
        abort(); // TODO
    }
    return *this;
  }

  bool operator==(const Value& other) {
    if (type != other.type) {
      return false;
    }
    switch (other.type) {
      case String:
        return str == other.str;
      case Number:
        return num == other.num;
      case Array:
        return this == &other; // if you want a deep compare, use deepCompare
      case Null:
        break;
      case Bool:
        return boo == other.boo;
      case Object:
        return this == &other; // if you want a deep compare, use deepCompare
      default:
        abort();
    }
    return true;
  }

  char* parse(char* curr) {
  /* space, tab, linefeed/newline, or return */
#define is_json_space(x) (x == 32 || x == 9 || x == 10 || x == 13)
#define skip()                                                                 \
  {                                                                            \
    while (*curr && is_json_space(*curr))                                      \
      curr++;                                                                  \
  }
    skip();
    if (*curr == '"') {
      // String
      curr++;
      char* close = strchr(curr, '"');
      assert(close);
      *close = 0; // end this string, and reuse it straight from the input
      setString(curr);
      curr = close + 1;
    } else if (*curr == '[') {
      // Array
      curr++;
      skip();
      setArray();
      while (*curr != ']') {
        Ref temp = arena.alloc<Value>();
        arr->push_back(temp);
        curr = temp->parse(curr);
        skip();
        if (*curr == ']') {
          break;
        }
        assert(*curr == ',');
        curr++;
        skip();
      }
      curr++;
    } else if (*curr == 'n') {
      // Null
      assert(strncmp(curr, "null", 4) == 0);
      setNull();
      curr += 4;
    } else if (*curr == 't') {
      // Bool true
      assert(strncmp(curr, "true", 4) == 0);
      setBool(true);
      curr += 4;
    } else if (*curr == 'f') {
      // Bool false
      assert(strncmp(curr, "false", 5) == 0);
      setBool(false);
      curr += 5;
    } else if (*curr == '{') {
      // Object
      curr++;
      skip();
      setObject();
      while (*curr != '}') {
        assert(*curr == '"');
        curr++;
        char* close = strchr(curr, '"');
        assert(close);
        *close = 0; // end this string, and reuse it straight from the input
        IString key(curr);
        curr = close + 1;
        skip();
        assert(*curr == ':');
        curr++;
        skip();
        Ref value = arena.alloc<Value>();
        curr = value->parse(curr);
        (*obj)[key] = value;
        skip();
        if (*curr == '}') {
          break;
        }
        assert(*curr == ',');
        curr++;
        skip();
      }
      curr++;
    } else {
      // Number
      char* after;
      setNumber(strtod(curr, &after));
      curr = after;
    }
    return curr;
  }

  void stringify(std::ostream& os, bool pretty = false);

  // String operations

  // Number operations

  // Array operations

  size_t size() {
    assert(isArray());
    return arr->size();
  }

  bool empty() { return size() == 0; }

  void setSize(size_t size) {
    assert(isArray());
    auto old = arr->size();
    if (old != size) {
      arr->resize(size);
    }
    if (old < size) {
      for (auto i = old; i < size; i++) {
        (*arr)[i] = arena.alloc<Value>();
      }
    }
  }

  Ref& operator[](unsigned x) {
    assert(isArray());
    return (*arr)[x];
  }

  Value& push_back(Ref r) {
    assert(isArray());
    arr->push_back(r);
    return *this;
  }
  Ref pop_back() {
    assert(isArray());
    Ref ret = arr->back();
    arr->pop_back();
    return ret;
  }

  Ref back() {
    assert(isArray());
    if (arr->size() == 0) {
      return nullptr;
    }
    return arr->back();
  }

  void splice(int x, int num) {
    assert(isArray());
    arr->erase(arr->begin() + x, arr->begin() + x + num);
  }

  int indexOf(Ref other) {
    assert(isArray());
    for (size_t i = 0; i < arr->size(); i++) {
      if (other == (*arr)[i]) {
        return i;
      }
    }
    return -1;
  }

  Ref map(std::function<Ref(Ref node)> func) {
    assert(isArray());
    Ref ret = arena.alloc<Value>();
    ret->setArray();
    for (size_t i = 0; i < arr->size(); i++) {
      ret->push_back(func((*arr)[i]));
    }
    return ret;
  }

  Ref filter(std::function<bool(Ref node)> func) {
    assert(isArray());
    Ref ret = arena.alloc<Value>();
    ret->setArray();
    for (size_t i = 0; i < arr->size(); i++) {
      Ref curr = (*arr)[i];
      if (func(curr)) {
        ret->push_back(curr);
      }
    }
    return ret;
  }

  /*
  void forEach(std::function<void (Ref)> func) {
    for (size_t i = 0; i < arr->size(); i++) {
      func((*arr)[i]);
    }
  }
  */

  // Null operations

  // Bool operations

  // Object operations

  Ref& operator[](IString x) {
    assert(isObject());
    return (*obj)[x];
  }

  bool has(IString x) {
    assert(isObject());
    return obj->count(x) > 0;
  }
};

struct Assign : public Value {
  Ref value_;

  Assign(Ref targetInit, Ref valueInit) {
    type = Assign_;
    target() = targetInit;
    value() = valueInit;
  }

  Assign() : Assign(nullptr, nullptr) {}

  Ref& target() { return ref; }
  Ref& value() { return value_; }
};

struct AssignName : public Value {
  IString target_;

  AssignName(IString targetInit, Ref valueInit) {
    type = AssignName_;
    target() = targetInit;
    value() = valueInit;
  }

  AssignName() : AssignName(IString(), nullptr) {}

  IString& target() { return target_; }
  Ref& value() { return ref; }
};

// JS printing support

struct JSPrinter {
  bool pretty, finalize;

  char* buffer = nullptr;
  size_t size = 0;
  size_t used = 0;

  int indent = 0;
  bool possibleSpace = false; // add a space to separate identifiers

  Ref ast;

  JSPrinter(bool pretty_, bool finalize_, Ref ast_)
    : pretty(pretty_), finalize(finalize_), ast(ast_) {}

  ~JSPrinter() { free(buffer); }

  void printAst() {
    print(ast);
    ensure(1);
    buffer[used] = 0;
  }

  // Utils

  void ensure(int safety = 100) {
    if (size >= used + safety) {
      return;
    }
    size = std::max((size_t)1024, size * 2) + safety;
    if (!buffer) {
      buffer = (char*)malloc(size);
      if (!buffer) {
        errv("Out of memory allocating %zd bytes for output buffer!", size);
        abort();
      }
    } else {
      char* buf = (char*)realloc(buffer, size);
      if (!buf) {
        free(buffer);
        errv("Out of memory allocating %zd bytes for output buffer!", size);
        abort();
      }
      buffer = buf;
    }
  }

  void emit(char c) {
    maybeSpace(c);
    if (!pretty && c == '}' && buffer[used - 1] == ';') {
      used--; // optimize ;} into }, the ; is not separating anything
    }
    ensure(1);
    buffer[used++] = c;
  }

  void emit(const char* s) {
    maybeSpace(*s);
    int len = strlen(s);
    ensure(len + 1);
    strncpy(buffer + used, s, len + 1);
    used += len;
  }

  void newline() {
    if (!pretty) {
      return;
    }
    emit('\n');
    for (int i = 0; i < indent; i++) {
      emit(' ');
    }
  }

  void space() {
    if (pretty) {
      emit(' ');
    }
  }

  void safeSpace() {
    if (pretty) {
      emit(' ');
    } else {
      possibleSpace = true;
    }
  }

  void maybeSpace(char s) {
    if (possibleSpace) {
      possibleSpace = false;
      if (isIdentPart(s)) {
        emit(' ');
      }
    }
  }

  bool isNothing(Ref node) {
    return node->isArray() && node[0] == TOPLEVEL && node[1]->size() == 0;
  }

  bool isDefun(Ref node) { return node->isArray() && node[0] == DEFUN; }

  bool endsInBlock(Ref node) {
    if (node->isArray() && node[0] == BLOCK) {
      return true;
    }
    // Check for a label on a block
    if (node->isArray() && node[0] == LABEL && endsInBlock(node[2])) {
      return true;
    }
    // Check for an if
    if (node->isArray() && node[0] == IF &&
        endsInBlock(ifHasElse(node) ? node[3] : node[2])) {
      return true;
    }
    return false;
  }

  bool isIf(Ref node) { return node->isArray() && node[0] == IF; }

  void print(Ref node) {
    ensure();
    if (node->isString()) {
      printName(node);
      return;
    }
    if (node->isNumber()) {
      printNum(node);
      return;
    }
    if (node->isAssignName()) {
      printAssignName(node);
      return;
    }
    if (node->isAssign()) {
      printAssign(node);
      return;
    }
    IString type = node[0]->getIString();
    switch (type.str[0]) {
      case 'a': {
        if (type == ARRAY) {
          printArray(node);
        } else {
          abort();
        }
        break;
      }
      case 'b': {
        if (type == BINARY) {
          printBinary(node);
        } else if (type == BLOCK) {
          printBlock(node);
        } else if (type == BREAK) {
          printBreak(node);
        } else {
          abort();
        }
        break;
      }
      case 'c': {
        if (type == CALL) {
          printCall(node);
        } else if (type == CONDITIONAL) {
          printConditional(node);
        } else if (type == CONTINUE) {
          printContinue(node);
        } else {
          abort();
        }
        break;
      }
      case 'd': {
        if (type == DEFUN) {
          printDefun(node);
        } else if (type == DO) {
          printDo(node);
        } else if (type == DOT) {
          printDot(node);
        } else {
          abort();
        }
        break;
      }
      case 'i': {
        if (type == IF) {
          printIf(node);
        } else {
          abort();
        }
        break;
      }
      case 'l': {
        if (type == LABEL) {
          printLabel(node);
        } else {
          abort();
        }
        break;
      }
      case 'n': {
        if (type == NEW) {
          printNew(node);
        } else {
          abort();
        }
        break;
      }
      case 'o': {
        if (type == OBJECT) {
          printObject(node);
        }
        break;
      }
      case 'r': {
        if (type == RETURN) {
          printReturn(node);
        } else {
          abort();
        }
        break;
      }
      case 's': {
        if (type == SUB) {
          printSub(node);
        } else if (type == SEQ) {
          printSeq(node);
        } else if (type == SWITCH) {
          printSwitch(node);
        } else if (type == STRING) {
          printString(node);
        } else {
          abort();
        }
        break;
      }
      case 't': {
        if (type == TOPLEVEL) {
          printToplevel(node);
        } else if (type == TRY) {
          printTry(node);
        } else {
          abort();
        }
        break;
      }
      case 'u': {
        if (type == UNARY_PREFIX) {
          printUnaryPrefix(node);
        } else {
          abort();
        }
        break;
      }
      case 'v': {
        if (type == VAR) {
          printVar(node);
        } else {
          abort();
        }
        break;
      }
      case 'w': {
        if (type == WHILE) {
          printWhile(node);
        } else {
          abort();
        }
        break;
      }
      default: {
        errv("cannot yet print %s\n", type.str.data());
        abort();
      }
    }
  }

  // print a node, and if nothing is emitted, emit something instead
  void print(Ref node, const char* otherwise) {
    auto last = used;
    print(node);
    if (used == last) {
      emit(otherwise);
    }
  }

  void printStats(Ref stats) {
    bool first = true;
    for (size_t i = 0; i < stats->size(); i++) {
      Ref curr = stats[i];
      if (!isNothing(curr)) {
        if (first) {
          first = false;
        } else {
          newline();
        }
        print(curr);
        if (!isDefun(curr) && !endsInBlock(curr) && !isIf(curr)) {
          emit(';');
        }
      }
    }
  }

  void printToplevel(Ref node) {
    if (node[1]->size() > 0) {
      printStats(node[1]);
    }
  }

  void printBlock(Ref node) {
    if (node->size() == 1 || node[1]->size() == 0) {
      emit("{}");
      return;
    }
    emit('{');
    indent++;
    newline();
    printStats(node[1]);
    indent--;
    newline();
    emit('}');
  }

  void printDefun(Ref node) {
    emit("function ");
    emit(node[1]->getCString());
    emit('(');
    Ref args = node[2];
    for (size_t i = 0; i < args->size(); i++) {
      if (i > 0) {
        (pretty ? emit(", ") : emit(','));
      }
      emit(args[i]->getCString());
    }
    emit(')');
    space();
    if (node->size() == 3 || node[3]->size() == 0) {
      emit("{}");
      return;
    }
    emit('{');
    indent++;
    newline();
    printStats(node[3]);
    indent--;
    newline();
    emit('}');
    newline();
  }

  void printAssign(Ref node) {
    auto* assign = node->asAssign();
    printChild(assign->target(), node, -1);
    space();
    emit('=');
    space();
    printChild(assign->value(), node, 1);
  }

  void printAssignName