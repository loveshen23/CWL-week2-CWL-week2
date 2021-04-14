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
      auto data = literal.getGCData();
      assert(data);
      o << "[ref " << data->type << ' ' << data->values << ']';
    }
  }
  restoreNormalColor(o);
  return o;
}

std::ostream& operator<<(std::ostream& o, wasm::Literals literals) {
  if (literals.size() == 1) {
    return o << literals[0];
  } else {
    o << '(';
    if (literals.size() > 0) {
      o << literals[0];
    }
    for (size_t i = 1; i < literals.size(); ++i) {
      o << ", " << literals[i];
    }
    return o << ')';
  }
}

Literal Literal::countLeadingZeroes() const {
  if (type == Type::i32) {
    return Literal((int32_t)Bits::countLeadingZeroes(i32));
  }
  if (type == Type::i64) {
    return Literal((int64_t)Bits::countLeadingZeroes(i64));
  }
  WASM_UNREACHABLE("invalid type");
}

Literal Literal::countTrailingZeroes() const {
  if (type == Type::i32) {
    return Literal((int32_t)Bits::countTrailingZeroes(i32));
  }
  if (type == Type::i64) {
    return Literal((int64_t)Bits::countTrailingZeroes(i64));
  }
  WASM_UNREACHABLE("invalid type");
}

Literal Literal::popCount() const {
  if (type == Type::i32) {
    return Literal((int32_t)Bits::popCount(i32));
  }
  if (type == Type::i64) {
    return Literal((int64_t)Bits::popCount(i64));
  }
  WASM_UNREACHABLE("invalid type");
}

Literal Literal::extendToSI64() const {
  assert(type == Type::i32);
  return Literal((int64_t)i32);
}

Literal Literal::extendToUI64() const {
  assert(type == Type::i32);
  return Literal((uint64_t)(uint32_t)i32);
}

Literal Literal::extendToF64() const {
  assert(type == Type::f32);
  return Literal(double(getf32()));
}

Literal Literal::extendS8() const {
  if (type == Type::i32) {
    return Literal(int32_t(int8_t(geti32() & 0xFF)));
  }
  if (type == Type::i64) {
    return Literal(int64_t(int8_t(geti64() & 0xFF)));
  }
  WASM_UNREACHABLE("invalid type");
}

Literal Literal::extendS16() const {
  if (type == Type::i32) {
    return Literal(int32_t(int16_t(geti32() & 0xFFFF)));
  }
  if (type == Type::i64) {
    return Literal(int64_t(int16_t(geti64() & 0xFFFF)));
  }
  WASM_UNREACHABLE("invalid type");
}

Literal Literal::extendS32() const {
  if (type == Type::i64) {
    return Literal(int64_t(int32_t(geti64() & 0xFFFFFFFF)));
  }
  WASM_UNREACHABLE("invalid type");
}

Literal Literal::wrapToI32() const {
  assert(type == Type::i64);
  return Literal((int32_t)i64);
}

Literal Literal::convertSIToF32() const {
  if (type == Type::i32) {
    return Literal(float(i32));
  }
  if (type == Type::i64) {
    return Literal(float(i64));
  }
  WASM_UNREACHABLE("invalid type");
}

Literal Literal::convertUIToF32() const {
  if (type == Type::i32) {
    return Literal(float(uint32_t(i32)));
  }
  if (type == Type::i64) {
    return Literal(float(uint64_t(i64)));
  }
  WASM_UNREACHABLE("invalid type");
}

Literal Literal::convertSIToF64() const {
  if (type == Type::i32) {
    return Literal(double(i32));
  }
  if (type == Type::i64) {
    return Literal(double(i64));
  }
  WASM_UNREACHABLE("invalid type");
}

Literal Literal::convertUIToF64() const {
  if (type == Type::i32) {
    return Literal(double(uint32_t(i32)));
  }
  if (type == Type::i64) {
    return Literal(double(uint64_t(i64)));
  }
  WASM_UNREACHABLE("invalid type");
}

template<typename F> struct AsInt { using type = void; };
template<> struct AsInt<float> { using type = int32_t; };
template<> struct AsInt<double> { using type = int64_t; };

template<typename F, typename I, bool (*RangeCheck)(typename AsInt<F>::type)>
static Literal saturating_trunc(typename AsInt<F>::type val) {
  if (std::isnan(bit_cast<F>(val))) {
    return Literal(I(0));
  }
  if (!RangeCheck(val)) {
    if (std::signbit(bit_cast<F>(val))) {
      return Literal(std::numeric_limits<I>::min());
    } else {
      return Literal(std::numeric_limits<I>::max());
    }
  }
  return Literal(I(std::trunc(bit_cast<F>(val))));
}

Literal Literal::truncSatToSI32() const {
  if (type == Type::f32) {
    return saturating_trunc<float, int32_t, isInRangeI32TruncS>(
      Literal(*this).castToI32().geti32());
  }
  if (type == Type::f64) {
    return saturating_trunc<double, int32_t, isInRangeI32TruncS>(
      Literal(*this).castToI64().geti64());
  }
  WASM_UNREACHABLE("invalid type");
}

Literal Literal::truncSatToSI64() const {
  if (type == Type::f32) {
    return saturating_trunc<float, int64_t, isInRangeI64TruncS>(
      Literal(*this).castToI32().geti32());
  }
  if (type == Type::f64) {
    return saturating_trunc<double, int64_t, isInRangeI64TruncS>(
      Literal(*this).castToI64().geti64());
  }
  WASM_UNREACHABLE("invalid type");
}

Literal Literal::truncSatToUI32() const {
  if (type == Type::f32) {
    return saturating_trunc<float, uint32_t, isInRangeI32TruncU>(
      Literal(*this).castToI32().geti32());
  }
  if (type == Type::f64) {
    return saturating_trunc<double, uint32_t, isInRangeI32TruncU>(
      Literal(*this).castToI64().geti64());
  }
  WASM_UNREACHABLE("invalid type");
}

Literal Literal::truncSatToUI64() const {
  if (type == Type::f32) {
    return saturating_trunc<float, uint64_t, isInRangeI64TruncU>(
      Literal(*this).castToI32().geti32());
  }
  if (type == Type::f64) {
    return saturating_trunc<double, uint64_t, isInRangeI64TruncU>(
      Literal(*this).castToI64().geti64());
  }
  WASM_UNREACHABLE("invalid type");
}

Literal Literal::eqz() const {
  switch (type.getBasic()) {
    case Type::i32:
      return eq(Literal(int32_t(0)));
    case Type::i64:
      return eq(Literal(int64_t(0)));
    case Type::f32:
      return eq(Literal(float(0)));
    case Type::f64:
      return eq(Literal(double(0)));
    case Type::v128:
    case Type::none:
    case Type::unreachable:
      WASM_UNREACHABLE("unexpected type");
  }
  WASM_UNREACHABLE("invalid type");
}

Literal Literal::neg() const {
  switch (type.getBasic()) {
    case Type::i32:
      return Literal(-uint32_t(i32));
    case Type::i64:
      return Literal(-uint64_t(i64));
    case Type::f32:
      return Literal(i32 ^ 0x80000000).castToF32();
    case Type::f64:
      return Literal(int64_t(i64 ^ 0x8000000000000000ULL)).castToF64();
    case Type::v128:
    case Type::none:
    case Type::unreachable:
      WASM_UNREACHABLE("unexpected type");
  }
  WASM_UNREACHABLE("invalid type");
}

Literal Literal::abs() const {
  switch (type.getBasic()) {
    case Type::i32:
      return Literal(std::abs(i32));
    case Type::i64:
      return Literal(std::abs(i64));
    case Type::f32:
      return Literal(i32 & 0x7fffffff).castToF32();
    case Type::f64:
      return Literal(int64_t(i64 & 0x7fffffffffffffffULL)).castToF64();
    case Type::v128:
    case Type::none:
    case Type::unreachable:
      WASM_UNREACHABLE("unexpected type");
  }
  WASM_UNREACHABLE("unexpected type");
}

Literal Literal::ceil() const {
  switch (type.getBasic()) {
    case Type::f32:
      return Literal(std::ceil(getf32()));
    case Type::f64:
      return Literal(std::ceil(getf64()));
    default:
      WASM_UNREACHABLE("unexpected type");
  }
}

Literal Literal::floor() const {
  switch (type.getBasic()) {
    case Type::f32:
      return Literal(std::floor(getf32()));
    case Type::f64:
      return Literal(std::floor(getf64()));
    default:
      WASM_UNREACHABLE("unexpected type");
  }
}

Literal Literal::trunc() const {
  switch (type.getBasic()) {
    case Type::f32:
      return Literal(std::trunc(getf32()));
    case Type::f64:
      return Literal(std::trunc(getf64()));
    default:
      WASM_UNREACHABLE("unexpected type");
  }
}

Literal Literal::nearbyint() const {
  switch (type.getBasic()) {
    case Type::f32:
      return Literal(std::nearbyint(getf32()));
    case Type::f64:
      return Literal(std::nearbyint(getf64()));
    default:
      WASM_UNREACHABLE("unexpected type");
  }
}

Literal Literal::sqrt() const {
  switch (type.getBasic()) {
    case Type::f32:
      return Literal(std::sqrt(getf32()));
    case Type::f64:
      return Literal(std::sqrt(getf64()));
    default:
      WASM_UNREACHABLE("unexpected type");
  }
}

Literal Literal::demote() const {
  auto f64 = getf64();
  if (std::isnan(f64)) {
    return Literal(float(f64));
  }
  if (std::isinf(f64)) {
    return Literal(float(f64));
  }
  // when close to the limit, but still truncatable to a valid value, do that
  // see
  // https://github.com/WebAssembly/sexpr-wasm-prototype/blob/2d375e8d502327e814d62a08f22da9d9b6b675dc/src/wasm-interpreter.c#L247
  uint64_t bits = reinterpreti64();
  if (bits > 0x47efffffe0000000ULL && bits < 0x47effffff0000000ULL) {
    return Literal(std::numeric_limits<float>::max());
  }
  if (bits > 0xc7efffffe0000000ULL && bits < 0xc7effffff0000000ULL) {
    return Literal(-std::numeric_limits<float>::max());
  }
  // when we must convert to infinity, do that
  if (f64 < -std::numeric_limits<float>::max()) {
    return Literal(-std::numeric_limits<float>::infinity());
  }
  if (f64 > std::numeric_limits<float>::max()) {
    return Literal(std::numeric_limits<float>::infinity());
  }
  return Literal(float(getf64()));
}

Literal Literal::add(const Literal& other) const {
  switch (type.getBasic()) {
    case Type::i32:
      return Literal(uint32_t(i32) + uint32_t(other.i32));
    case Type::i64:
      return Literal(uint64_t(i64) + uint64_t(other.i64));
    case Type::f32:
      return standardizeNaN(Literal(getf32() + other.getf32()));
    case Type::f64:
      return standardizeNaN(Literal(getf64() + other.getf64()));
    case Type::v128:
    case Type::none:
    case Type::unreachable:
      WASM_UNREACHABLE("unexpected type");
  }
  WASM_UNREACHABLE("unexpected type");
}

Literal Literal::sub(const Literal& other) const {
  switch (type.getBasic()) {
    case Type::i32:
      return Literal(uint32_t(i32) - uint32_t(other.i32));
    case Type::i64:
      return Literal(uint64_t(i64) - uint64_t(other.i64));
    case Type::f32:
      return standardizeNaN(Literal(getf32() - other.getf32()));
    case Type::f64:
      return standardizeNaN(Literal(getf64() - other.getf64()));
    case Type::v128:
    case Type::none:
    case Type::unreachable:
      WASM_UNREACHABLE("unexpected type");
  }
  WASM_UNREACHABLE("unexpected type");
}

template<typename T> static T add_sat_s(T a, T b) {
  static_assert(std::is_signed<T>::value,
                "Trying to instantiate add_sat_s with unsigned type");
  using UT = typename std::make_unsigned<T>::type;
  UT ua = static_cast<UT>(a);
  UT ub = static_cast<UT>(b);
  UT ures = ua + ub;
  // overflow if sign of result is different from sign of a and b
  if (static_cast<T>((ures ^ ua) & (ures ^ ub)) < 0) {
    return (a < 0) ? std::numeric_limits<T>::min()
                   : std::numeric_limits<T>::max();
  }
  return static_cast<T>(ures);
}

template<typename T> static T sub_sat_s(T a, T b) {
  static_assert(std::is_signed<T>::value,
                "Trying to instantiate sub_sat_s with unsigned type");
  using UT = typename std::make_unsigned<T>::type;
  UT ua = static_cast<UT>(a);
  UT ub = static_cast<UT>(b);
  UT ures = ua - ub;
  // overflow if a and b have different signs and result and a differ in sign
  if (static_cast<T>((ua ^ ub) & (ures ^ ua)) < 0) {
    return (a < 0) ? std::numeric_limits<T>::min()
                   : std::numeric_limits<T>::max();
  }
  return static_cast<T>(ures);
}

template<typename T> static T add_sat_u(T a, T b) {
  static_assert(std::is_unsigned<T>::value,
                "Trying to instantiate add_sat_u with signed type");
  T res = a + b;
  // overflow if result is less than arguments
  return (res < a) ? std::numeric_limits<T>::max() : res;
}

template<typename T> static T sub_sat_u(T a, T b) {
  static_assert(std::is_unsigned<T>::value,
                "Trying to instantiate sub_sat_u with signed type");
  T res = a - b;
  // overflow if result is greater than a
  return (res > a) ? 0 : res;
}

Literal Literal::addSatSI8(const Literal& other) const {
  return Literal(add_sat_s<int8_t>(geti32(), other.geti32()));
}
Literal Literal::addSatUI8(const Literal& other) const {
  return Literal(add_sat_u<uint8_t>(geti32(), other.geti32()));
}
Literal Literal::addSatSI16(const Literal& other) const {
  return Literal(add_sat_s<int16_t>(geti32(), other.geti32()));
}
Literal Literal::addSatUI16(const Literal& other) const {
  return Literal(add_sat_u<uint16_t>(geti32(), other.geti32()));
}
Literal Literal::subSatSI8(const Literal& other) const {
  return Literal(sub_sat_s<int8_t>(geti32(), other.geti32()));
}
Literal Literal::subSatUI8(const Literal& other) const {
  return Literal(sub_sat_u<uint8_t>(geti32(), other.geti32()));
}
Literal Literal::subSatSI16(const Literal& other) const {
  return Literal(sub_sat_s<int16_t>(geti32(), other.geti32()));
}
Literal Literal::subSatUI16(const Literal& other) const {
  return Literal(sub_sat_u<uint16_t>(geti32(), other.geti32()));
}

Literal Literal::q15MulrSatSI16(const Literal& other) const {
  int64_t value =
    (int64_t(geti32()) * int64_t(other.geti32()) + 0x4000LL) >> 15LL;
  int64_t lower = std::numeric_limits<int16_t>::min();
  int64_t upper = std::numeric_limits<int16_t>::max();
  return Literal(int16_t(std::min(std::max(value, lower), upper)));
}

Literal Literal::mul(const Literal& other) const {
  switch (type.getBasic()) {
    case Type::i32:
      return Literal(uint32_t(i32) * uint32_t(other.i32));
    case Type::i64:
      return Literal(uint64_t(i64) * uint64_t(other.i64));
    case Type::f32:
      return standardizeNaN(Literal(getf32() * other.getf32()));
    case Type::f64:
      return standardizeNaN(Literal(getf64() * other.getf64()));
    case Type::v128:
    case Type::none:
    case Type::unreachable:
      WASM_UNREACHABLE("unexpected type");
  }
  WASM_UNREACHABLE("unexpected type");
}

Literal Literal::div(const Literal& other) const {
  switch (type.getBasic()) {
    case Type::f32: {
      float lhs = getf32(), rhs = other.getf32();
      float sign = std::signbit(lhs) == std::signbit(rhs) ? 0.f : -0.f;
      switch (std::fpclassify(rhs)) {
        case FP_ZERO:
          switch (std::fpclassify(lhs)) {
            case FP_NAN:
            case FP_ZERO:
              return standardizeNaN(Literal(lhs / rhs));
            case FP_NORMAL:    // fallthrough
            case FP_SUBNORMAL: // fallthrough
            case FP_INFINITE:
              return Literal(
                std::copysign(std::numeric_limits<float>::infinity(), sign));
            default:
              WASM_UNREACHABLE("invalid fp classification");
          }
        case FP_NAN:      // fallthrough
        case FP_INFINITE: // fallthrough
        case FP_NORMAL:   // fallthrough
        case FP_SUBNORMAL:
          return standardizeNaN(Literal(lhs / rhs));
        default:
          WASM_UNREACHABLE("invalid fp classification");
      }
    }
    case Type::f64: {
      double lhs = getf64(), rhs = other.getf64();
      double sign = std::signbit(lhs) == std::signbit(rhs) ? 0. : -0.;
      switch (std::fpclassify(rhs)) {
        case FP_ZERO:
          switch (std::fpclassify(lhs)) {
            case FP_NAN:
            case FP_ZERO:
              return standardizeNaN(Literal(lhs / rhs));
            case FP_NORMAL:    // fallthrough
            case FP_SUBNORMAL: // fallthrough
            case FP_INFINITE:
              return Literal(
                std::copysign(std::numeric_limits<double>::infinity(), sign));
            default:
              WASM_UNREACHABLE("invalid fp classification");
          }
        case FP_NAN:      // fallthrough
        case FP_INFINITE: // fallthrough
        case FP_NORMAL:   // fallthrough
        case FP_SUBNORMAL:
          return standardizeNaN(Literal(lhs / rhs));
        default:
          WASM_UNREACHABLE("invalid fp classification");
      }
    }
    default:
      WASM_UNREACHABLE("unexpected type");
  }
}

Literal Literal::divS(const Literal& other) const {
  switch (type.getBasic()) {
    case Type::i32:
      return Literal(i32 / other.i32);
    case Type::i64:
      return Literal(i64 / other.i64);
    default:
      WASM_UNREACHABLE("unexpected type");
  }
}

Literal Literal::divU(const Literal& other) const {
  switch (type.getBasic()) {
    case Type::i32:
      return Literal(uint32_t(i32) / uint32_t(other.i32));
    case Type::i64:
      return Literal(uint64_t(i64) / uint64_t(other.i64));
    default:
      WASM_UNREACHABLE("unexpected type");
  }
}

Literal Literal::remS(const Literal& other) const {
  switch (type.getBasic()) {
    case Type::i32:
      return Literal(i32 % other.i32);
    case Type::i64:
      return Literal(i64 % other.i64);
    default:
      WASM_UNREACHABLE("unexpected type");
  }
}

Literal Literal::remU(const Literal& other) const {
  switch (type.getBasic()) {
    case Type::i32:
      return Literal(uint32_t(i32) % uint32_t(other.i32));
    case Type::i64:
      return Literal(uint64_t(i64) % uint64_t(other.i64));
    default:
      WASM_UNREACHABLE("unexpected type");
  }
}

Literal Literal::minInt(const Literal& other) const {
  return geti32() < other.geti32() ? *this : other;
}
Literal Literal::maxInt(const Literal& other) const {
  return geti32() > other.geti32() ? *this : other;
}
Literal Literal::minUInt(const Literal& other) const {
  return uint32_t(geti32()) < uint32_t(other.geti32()) ? *this : other;
}
Literal Literal::maxUInt(const Literal& other) const {
  return uint32_t(geti32()) > uint32_t(other.geti32()) ? *this : other;
}

Literal Literal::avgrUInt(const Literal& other) const {
  return Literal((geti32() + other.geti32() + 1) / 2);
}

Literal Literal::and_(const Literal& other) const {
  switch (type.getBasic()) {
    case Type::i32:
      return Literal(i32 & other.i32);
    case Type::i64:
      return Literal(i64 & other.i64);
    default:
      WASM_UNREACHABLE("unexpected type");
  }
}

Literal Literal::or_(const Literal& other) const {
  switch (type.getBasic()) {
    case Type::i32:
      return Literal(i32 | other.i32);
    case Type::i64:
      return Literal(i64 | other.i64);
    default:
      WASM_UNREACHABLE("unexpected type");
  }
}

Literal Literal::xor_(const Literal& other) const {
  switch (type.getBasic()) {
    case Type::i32:
      return Literal(i32 ^ other.i32);
    case Type::i64:
      return Literal(i64 ^ other.i64);
    default:
      WASM_UNREACHABLE("unexpected type");
  }
}

Literal Literal::shl(const Literal& other) const {
  switch (type.getBasic()) {
    case Type::i32:
      return Literal(uint32_t(i32)
                     << Bits::getEffectiveShifts(other.i32, Type::i32));
    case Type::i64:
      return Literal(uint64_t(i64)
                     << Bits::getEffectiveShifts(other.i64, Type::i64));
    default:
      WASM_UNREACHABLE("unexpected type");
  }
}

Literal Literal::shrS(const Literal& other) const {
  switch (type.getBasic()) {
    case Type::i32:
      return Literal(i32 >> Bits::getEffectiveShifts(other.i32, Type::i32));
    case Type::i64:
      return Literal(i64 >> Bits::getEffectiveShifts(other.i64, Type::i64));
    default:
      WASM_UNREACHABLE("unexpected type");
  }
}

Literal Literal::shrU(const Literal& other) const {
  switch (type.getBasic()) {
    case Type::i32:
      return Literal(uint32_t(i32) >>
                     Bits::getEffectiveShifts(other.i32, Type::i32));
    case Type::i64:
      return Literal(uint64_t(i64) >>
                     Bits::getEffectiveShifts(other.i64, Type::i64));
    default:
      WASM_UNREACHABLE("unexpected type");
  }
}

Literal Literal::rotL(const Literal& other) const {
  switch (type.getBasic()) {
    case Type::i32:
      return Literal(Bits::rotateLeft(uint32_t(i32), uint32_t(other.i32)));
    case Type::i64:
      return Literal(Bits::rotateLeft(uint64_t(i64), uint64_t(other.i64)));
    default:
      WASM_UNREACHABLE("unexpected type");
  }
}

Literal Literal::rotR(const Literal& other) const {
  switch (type.getBasic()) {
    case Type::i32:
      return Literal(Bits::rotateRight(uint32_t(i32), uint32_t(other.i32)));
    case Type::i64:
      return Literal(Bits::rotateRight(uint64_t(i64), uint64_t(other.i64)));
    default:
      WASM_UNREACHABLE("unexpected type");
  }
}

Literal Literal::eq(const Literal& other) const {
  switch (type.getBasic()) {
    case Type::i32:
      return Literal(i32 == other.i32);
    case Type::i64:
      return Literal(i64 == other.i64);
    case Type::f32:
      return Literal(getf32() == other.getf32());
    case Type::f64:
      return Literal(getf64() == other.getf64());
    case Type::v128:
    case Type::none:
    case Type::unreachable:
      WASM_UNREACHABLE("unexpected type");
  }
  WASM_UNREACHABLE("unexpected type");
}

Literal Literal::ne(const Literal& other) const {
  switch (type.getBasic()) {
    case Type::i32:
      return Literal(i32 != other.i32);
    case Type::i64:
      return Literal(i64 != other.i64);
    case Type::f32:
      return Literal(getf32() != other.getf32());
    case Type::f64:
      return Literal(getf64() != other.getf64());
    case Type::v128:
    case Type::none:
    case Type::unreachable:
      WASM_UNREACHABLE("unexpected type");
  }
  WASM_UNREACHABLE("unexpected type");
}

Literal Literal::ltS(const Literal& other) const {
  switch (type.getBasic()) {
    case Type::i32:
      return Literal(i32 < other.i32);
    case Type::i64:
      return Literal(i64 < other.i64);
    default:
      WASM_UNREACHABLE("unexpected type");
  }
}

Literal Literal::ltU(const Literal& other) const {
  switch (type.getBasic()) {
    case Type::i32:
      return Literal(uint32_t(i32) < uint32_t(other.i32));
    case Type::i64:
      return Literal(uint64_t(i64) < uint64_t(other.i64));
    default:
      WASM_UNREACHABLE("unexpected type");
  }
}

Literal Literal::lt(const Literal& other) const {
  switch (type.getBasic()) {
    case Type::f32:
      return Literal(getf32() < other.getf32());
    case Type::f64:
      return Literal(getf64() < other.getf64());
    default:
      WASM_UNREACHABLE("unexpected type");
  }
}

Literal Literal::leS(const Literal& other) const {
  switch (type.getBasic()) {
    case Type::i32:
      return Literal(i32 <= other.i32);
    case Type::i64:
      return Literal(i64 <= other.i64);
    default:
      WASM_UNREACHABLE("unexpected type");
  }
}

Literal Literal::leU(const Literal& other) const {
  switch (type.getBasic()) {
    case Type::i32:
      return Literal(uint32_t(i32) <= uint32_t(other.i32));
    case Type::i64:
      return Literal(uint64_t(i64) <= uint64_t(other.i64));
    default:
      WASM_UNREACHABLE("unexpected type");
  }
}

Literal Literal::le(const Literal& other) const {
  switch (type.getBasic()) {
    case Type::f32:
      return Literal(getf32() <= other.getf32());
    case Type::f64:
      return Literal(getf64() <= other.getf64());
    default:
      WASM_UNREACHABLE("unexpected type");
  }
}

Literal Literal::gtS(const Literal& other) const {
  switch (type.getBasic()) {
    case Type::i32:
      return Literal(i32 > other.i32);
    case Type::i64:
      return Literal(i64 > other.i64);
    default:
      WASM_UNREACHABLE("unexpected type");
  }
}

Literal Literal::gtU(const Literal& other) const {
  switch (type.getBasic()) {
    case Type::i32:
      return Literal(uint32_t(i32) > uint32_t(other.i32));
    case Type::i64:
      return Literal(uint64_t(i64) > uint64_t(other.i64));
    default:
      WASM_UNREACHABLE("unexpected type");
  }
}

Literal Literal::gt(const Literal& other) const {
  switch (type.getBasic()) {
    case Type::f32:
      return Literal(getf32() > other.getf32());
    case Type::f64:
      return Literal(getf64() > other.getf64());
    default:
      WASM_UNREACHABLE("unexpected type");
  }
}

Literal Literal::geS(const Literal& other) const {
  switch (type.getBasic()) {
    case Type::i32:
      return 