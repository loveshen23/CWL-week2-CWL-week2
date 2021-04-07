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
        case HeapType::struct_:
        case HeapType::array:
          WASM_UNREACHABLE("invalid type");
        case HeapType::string:
        case HeapType::stringview_wtf8:
        case HeapType::stringview_wtf16:
        case HeapType::stringview_iter:
          WASM_UNREACHABLE("TODO: string literals");
      }
    }
  }
}

Literal::~Literal() {
  // Early exit for the common case; basic types need no special handling.
  if (type.isBasic()) {
    return;
  }
  if (isNull() || isData()) {
    gcData.~shared_ptr();
  }
}

Literal& Literal::operator=(const Literal& other) {
  if (this != &other) {
    this->~Literal();
    new (this) auto(other);
  }
  return *this;
}

template<typename LaneT, int Lanes>
static void extractBytes(uint8_t (&dest)[16], const LaneArray<Lanes>& lanes) {
  std::array<uint8_t, 16> bytes;
  const size_t lane_width = 16 / Lanes;
  for (size_t lane_index = 0; lane_index < Lanes; ++lane_index) {
    uint8_t bits[16];
    lanes[lane_index].getBits(bits);
    LaneT lane;
    memcpy(&lane, bits, sizeof(lane));
    for (size_t offset = 0; offset < lane_width; ++offset) {
      bytes.at(lane_index * lane_width + offset) =
        uint8_t(lane >> (8 * offset));
    }
  }
  memcpy(&dest, bytes.data(), sizeof(bytes));
}

Literal::Literal(const LaneArray<16>& lanes) : type(Type::v128) {
  extractBytes<uint8_t, 16>(v128, lanes);
}

Literal::Literal(const LaneArray<8>& lanes) : type(Type::v128) {
  extractBytes<uint16_t, 8>(v128, lanes);
}

Literal::Literal(const LaneArray<4>& lanes) : type(Type::v128) {
  extractBytes<uint32_t, 4>(v128, lanes);
}

Literal::Literal(const LaneArray<2>& lanes) : type(Type::v128) {
  extractBytes<uint64_t, 2>(v128, lanes);
}

Literals Literal::makeZeros(Type type) {
  assert(type.isConcrete());
  Literals zeroes;
  for (const auto& t : type) {
    zeroes.push_back(makeZero(t));
  }
  return zeroes;
}

Literals Literal::makeOnes(Type type) {
  assert(type.isConcrete());
  Literals units;
  for (const auto& t : type) {
    units.push_back(makeOne(t));
  }
  return units;
}

Literals Literal::makeNegOnes(Type type) {
  assert(type.isConcrete());
  Literals units;
  for (const auto& t : type) {
    units.push_back(makeNegOne(t));
  }
  return units;
}

Literal Literal::makeZero(Type type) {
  assert(type.isSingle());
  if (type.isRef()) {
    return makeNull(type.getHeapType());
  } else {
    return makeFromInt32(0, type);
  }
}

Literal Literal::makeOne(Type type) {
  assert(type.isNumber());
  return makeFromInt32(1, type);
}

Literal Literal::makeNegOne(Type type) {
  assert(type.isNumber());
  return makeFromInt32(-1, type);
}

Literal Literal::makeFromMemory(void* p, Type type) {
  assert(type.isNumber());
  switch (type.getBasic()) {
    case Type::i32: {
      int32_t i;
      memcpy(&i, p, sizeof(i));
      return Literal(i);
    }
    case Type::i64: {
      int64_t i;
      memcpy(&i, p, sizeof(i));
      return Literal(i);
    }
    case Type::f32: {
      int32_t i;
      memcpy(&i, p, sizeof(i));
      return Literal(bit_cast<float>(i));
    }
    case Type::f64: {
      int64_t i;
      memcpy(&i, p, sizeof(i));
      return Literal(bit_cast<double>(i));
    }
    case Type::v128: {
      uint8_t bytes[16];
      memcpy(bytes, p, sizeof(bytes));
      return Literal(bytes);
    }
    default:
      WASM_UNREACHABLE("unexpected type");
  }
}

Literal Literal::makeFromMemory(void* p, const Field& field) {
  switch (field.packedType) {
    case Field::not_packed:
      return makeFromMemory(p, field.type);
    case Field::i8: {
      int8_t i;
      memcpy(&i, p, sizeof(i));
      return Literal(int32_t(i));
    }
    case Field::i16: {
      int16_t i;
      memcpy(&i, p, sizeof(i));
      return Literal(int32_t(i));
    }
  }
  WASM_UNREACHABLE("unexpected type");
}

Literal Literal::standardizeNaN(const Literal& input) {
  if (!std::isnan(input.getFloat())) {
    return input;
  }
  // Pick a simple canonical payload, and positive.
  if (input.type == Type::f32) {
    return Literal(bit_cast<float>(uint32_t(0x7fc00000u)));
  } else if (input.type == Type::f64) {
    return Literal(bit_cast<double>(uint64_t(0x7ff8000000000000ull)));
  } else {
    WASM_UNREACHABLE("unexpected type");
  }
}

std::array<uint8_t, 16> Literal::getv128() const {
  assert(type == Type::v128);
  std::array<uint8_t, 16> ret;
  memcpy(ret.data(), v128, sizeof(ret));
  return ret;
}

std::shared_ptr<GCData> Literal::getGCData() const {
  assert(isNull() || isData());
  return gcData;
}

Literal Literal::castToF32() {
  assert(type == Type::i32);
  Literal ret(Type::f32);
  ret.i32 = i32;
  return ret;
}

Literal Literal::castToF64() {
  assert(type == Type::i64);
  Literal ret(Type::f64);
  ret.i64 = i64;
  return ret;
}

Literal Literal::castToI32() {
  assert(type == Type::f32);
  Literal ret(Type::i32);
  ret.i32 = i32;
  return ret;
}

Literal Literal::castToI64() {
  assert(type == Type::f64);
  Literal ret(Type::i64);
  ret.i64 = i64;
  return ret;
}

int64_t Literal::getInteger() const {
  switch (type.getBasic()) {
    case Type::i32:
      return i32;
    case Type::i64:
      return i64;
    default:
      abort();
  }
}

uint64_t Literal::getUnsigned() const {
  switch (type.getBasic()) {
    case Type::i32:
      return static_cast<uint32_t>(i32);
    case Type::i64:
      return i64;
    default:
      abort();
  }
}

double Literal::getFloat() const {
  switch (type.getBasic()) {
    case Type::f32:
      return getf32();
    case Type::f64:
      return getf64();
    default:
      abort();
  }
}

void Literal::getBits(uint8_t (&buf)[16]) const {
  memset(buf, 0, 16);
  switch (type.getBasic()) {
    case Type::i32:
    case Type::f32:
      memcpy(buf, &i32, sizeof(i32));
      break;
    case Type::i64:
    case Type::f64:
      memcpy(buf, &i64, sizeof(i64));
      break;
    case Type::v128:
      memcpy(buf, &v128, sizeof(v128));
      break;
    case Type::none:
    case Type::unreachable:
      WASM_UNREACHABLE("invalid type");
  }
}

bool Literal::operator==(const Literal& other) const {
  if (type != other.type) {
    return false;
  }
  if (type.isBasic()) {
    switch (type.getBasic()) {
      case Type::none:
        return true; // special voided literal
      case Type::i32:
      case Type::f32:
        return i32 == other.i32;
      case Type::i64:
      case Type::f64:
        return i64 == other.i64;
      case Type::v128:
        return memcmp(v128, other.v128, 16) == 0;
      case Type::unreachable:
        break;
    }
  } else if (type.isRef()) {
    assert(type.isRef());
    if (type.isNull()) {
      return true;
    }
    if (type.isFunction()) {
      assert(func.is() && other.func.is());
      return func == other.func;
    }
    if (type.isString()) {
      return gcData->values == other.gcData->values;
    }
    if (type.isData()) {
      return gcData == other.gcData;
    }
    if (type.getHeapType() == HeapType::i31) {
      return i32 == other.i32;
    }
    WASM_UNREACHABLE("unexpected type");
  }
  WASM_UNREACHABLE("unexpected type");
}

bool Literal::operator!=(const Literal& other) const {
  return !(*this == other);
}

bool Literal::isNaN() {
  if (type == Type::f32 && std::isnan(getf32())) {
    return true;
  }
  if (type == Type::f64 && std::isnan(getf64())) {
    return true;
  }
  // TODO: SIMD?
  return false;
}

uint32_t Literal::NaNPayload(float f) {
  assert(std::isnan(f) && "expected a NaN");
  // SEEEEEEE EFFFFFFF FFFFFFFF FFFFFFFF
  // NaN has all-one exponent and non-zero fraction.
  return ~0xff800000u & bit_cast<uint32_t>(f);
}

uint64_t Literal::NaNPayload(double f) {
  assert(std::isnan(f) && "expected a NaN");
  // SEEEEEEE EEEEFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF
  // NaN has all-one exponent and non-zero fraction.
  return ~0xfff0000000000000ull & bit_cast<uint64_t>(f);
}

float Literal::setQuietNaN(float f) {
  assert(std::isnan(f) && "expected a NaN");
  // An SNaN is a NaN with the most significant fraction bit clear.
  return bit_cast<float>(0x00400000u | bit_cast<uint32_t>(f));
}

double Literal::setQuietNaN(double f) {
  assert(std::isnan(f) && "expected a NaN");
  // An SNaN is a NaN with the most significant fraction bit clear.
  return bit_cast<double>(0x0008000000000000ull | bit_cast<uint64_t>(f));
}

void Literal::printFloat(std::ostream& o, float f) {
  if (std::isnan(f)) {
    const char* sign = std::signbit(f) ? "-" : "";
    o << sign << "nan";
    if (uint32_t payload = NaNPayload(f)) {
      o << ":0x" << std::hex << payload << std::dec;
    }
    return;
  }
  printDouble(o, f);
}

void Literal::printDouble(std::ostream& o, double d) {
  if (d == 0 && std::signbit(d)) {
    o << "-0";
    return;
  }
  if (std::isnan(d)) {
    const char* sign = std::signbit(d) ? "-" : "";
    o << sign << "nan";
    if (uint64_t payload = NaNPayload(d)) {
      o << ":0x" << std::hex << payload << std::dec;
    }
    return;
  }
  if (!std::isfinite(d)) {
    o << (std::signbit(d) ? "-inf" : "inf");
    return;
  }
  const char* text = cashew::JSPrinter::numToString(d);
  // spec interpreter hates floats starting with '.'
  if (text[0] == '.') {
    o << '0';
  } else if (text[0] == '-' && text[1] == '.') {
    o << "-0";
    text++;
  }
  o << text;
}

void Literal::printVec128(std::ostream& o, const std::array<uint8_t, 16>& v) {
  o << std::hex;
  for (auto i = 0; i < 16; i += 4) {
    if (i) {
      o << " ";
    }
    o << "0x" << std::setfill('0') << std::setw(8)
      << uint32_t(v[i] | (v[i + 1] << 8) | (v[i + 2] << 16) | (v[i + 3] << 24));
  }
  o << std::dec;
}

std::ostream& operator<<(std::ostream& o, Literal literal) {
  prepareMinorColor(o);
  assert(literal.type.isSingle());
  if (literal.type.isBasic()) {
    switch (literal.type.getBasic()) {
      case Type::none:
        o << "?";
        break;
      case Type::i32:
        o << literal.geti32();
        break;
      case Type::i64:
        o << literal.geti64();
        break;
      case Type::f32:
        literal.printFloat(o, literal.getf32());
        break;
      case Type::f64:
        literal.printDouble(o, literal.getf64());
        break;
      case Type::v128:
        o << "i32x4 ";
        literal.printVec128(o, literal.getv128());
        break;
      case Type::unreachable:
        WASM_UNREACHABLE("unexpected type");
    }
  } else {
    assert(literal.type.isRef());
    auto heapType = literal.type.getHeapType();
    if (heapType.isBasic()) {
      switch (heapType.getBasic()) {
        case HeapType::i31:
          o << "i31ref(" << literal.geti31() << ")";
          break;
        case HeapType::none:
          o << "nullref";
          break;
        case HeapType::noext:
          o << "nullexternref";
          break;
        case HeapType::nofunc:
          o << "nullfuncref";
          break;
        case HeapType::ext:
        case HeapType::any:
          WASM_UNREACHABLE("TODO: extern literals");
        case HeapType::eq:
        case HeapType::func:
        case HeapType::struct_:
        case HeapType::array:
          WASM_UNREACHABLE("invalid type");
        case HeapType::string: {
          auto data = literal.getGCData();
          if (!data) {
            o << "nullstring";
          } else {
            o << "string(\"";
            for (auto c : data->values) {
              // TODO: more than ascii
              o << char(c.getInteger());
            }
            o << "\")";
          }
          break;
        }
        case HeapType::stringview_wtf8:
        case HeapType::stringview_wtf16:
        case HeapType::stringview_iter:
          WASM_UNREACHABLE("TODO: string literals");
      }
    } else if (heapType.isSignature()) {
      o << "funcref(" << literal.getFunc() << ")";
    } else {
      assert(literal.isData());
      auto data = literal.getGCData(