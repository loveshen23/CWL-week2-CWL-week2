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
      curr++;                                               