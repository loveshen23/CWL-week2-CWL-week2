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

#include "literal.h"

#include <cassert>
#include <cmath>

#include "emscripten-optimizer/simple_ast.h"
#include "ir/bits.h"
#include "pretty_printing.h"
#include "support/bits.h"
#include "support/utilities.h"

namespace wasm {

template<int N> using LaneArray = std::array<Literal, N>;

Literal::Literal(Type type) : type(type) {
  if (type.isBasic()) {
    switch (type.getBasic()) {
      case Type::i32:
      case Type::f32:
        i32 = 0;
        return;
      case Type::i64:
      case Type::f64:
        i64 = 0;
        return;
      case Type::v128:
        memset(&v128, 0, 16);
        return;
      case Type::none:
      case Type::unreachable:
        WASM_UNREACHABLE("Invalid literal type");
        return;
    }
  }

  if (type.isNull()) {
    assert(type.isNullable());
    new (&gcData) std::shared_ptr<GCData>();
    return;
  }

  if (type.isRef() && type.getHeapType() == HeapType::i31) {
    assert(type.isNonNullable());
    i32 = 0;
    return;
  }

  WASM_UNREACHABLE("Unexpected literal type");
}

Literal::Literal(const uint8_t init[16]) : type(Type::v128) {
  memcpy(&v128, init, 16);
}

Literal::Literal(std::shared_ptr<GCData> gcData, HeapType type)
  : gcData(gcData), type(type, NonNullable) {
  // The type must be a proper type for GC data: either a struct, array, or
  // string; or a null.
  assert((isData() && gcData) || (type.isBottom() && !gcData));
}

Literal::Literal(std::string string)
  : gcData(nullptr), type(Type(HeapType::string, NonNullable)) {
  // TODO: we could in theory internalize strings
  Literals contents;
  for (auto c : string) {
    contents.push_back(Literal(int32_t(c)));
  }
  gcData = std::make_shared<GCData>(HeapType::string, contents);
}

Literal::Literal(const Literal& other) : type(other.type) {
  if (type.isBasic()) {
    switch (type.getBasic()) {
      case Type::i32:
      case Type::f32:
        i32 = other.i32;
        return;
      case Type::i64:
      case Type::f64:
        i64 = other.i64;
        return;
      case Type::v128:
        memcpy(&v128, other.v128, 16);
        return;
      case Type::none:
        return;
      case Type::unreachable:
        break;
    }
  }
  if (other.isNull()) {
    new (&gcData) std::shared_ptr<GCData>();
    return;
  }
  if (other.isData()) {
    new (&gcData) std::shared_ptr<GCData>(other.gcData);
    return;
  }
  if (type.isFunction()) {
    func = other.func;
    return;
  }
  if (type.isRef()) {
    assert(!type.isNullable());
    auto heapType = type.getHeapType();
    if (heapType.isBasic()) {
      switch (heapType.getBasic()) {
        case HeapType::i31:
          i32 = other.i32;
          return;
        case HeapType::none:
        case HeapType::noext:
        case HeapType::nofunc:
          // Null
          return;
        case HeapType::ext:
        case HeapType::any:
          WASM_UNREACHABLE("TODO: extern literals");
        case HeapType::eq:
        case HeapType::func:
   