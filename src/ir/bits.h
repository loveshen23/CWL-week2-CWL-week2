/*
 * Copyright 2017 WebAssembly Community Group participants
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

#ifndef wasm_ir_bits_h
#define wasm_ir_bits_h

#include "ir/boolean.h"
#include "ir/literal-utils.h"
#include "ir/load-utils.h"
#include "support/bits.h"
#include "wasm-builder.h"

namespace wasm::Bits {

// get a mask to keep only the low # of bits
inline int32_t lowBitMask(int32_t bits) {
  uint32_t ret = -1;
  if (bits >= 32) {
    return ret;
  }
  return ret >> (32 - bits);
}

// checks if the input is a mask of lower bits, i.e., all 1s up to some high
// bit, and all zeros from there. returns the number of masked bits, or 0 if
// this is not such a mask
inline uint32_t getMaskedBits(uint32_t mask) {
  if (mask == uint32_t(-1)) {
    return 32; // all the bits
  }
  if (mask == 0) {
    return 0; // trivially not a mask
  }
  // otherwise, see if x & (x + 1) turns this into non-zero value
  // 00011111 & (00011111 + 1) => 0
  if (mask & (mask + 1)) {
    return 0;
  }
  // this is indeed a mask
  return 32 - countLeadingZeroes(mask);
}

// gets the number of effective shifts a shift operation does. In
// wasm, only 5 bits matter for 32-bit shifts, and 6 for 64.
inline Index getEffectiveShifts(Index amount, Type type) {
  if (type == Type::i32) {
    return amount & 31;
  } else if (type == Type::i64) {
    return amount & 63;
  }
  WASM_UNREACHABLE("unexpected type");
}

inline Index getEffectiveShifts(Expression* expr) {
  auto* amount = expr->cast<Const>();
  if (amount->type == Type::i32) {
    return getEffectiveShifts(amount->value.geti32(), Type::i32);
  } else if (amount->type == Type::i64) {
    return getEffectiveShifts(amount->value.geti64(), Type::i64);
  }
  WASM_UNREACHABLE("unexpected type");
}

inline Expression* makeSignExt(Expression* value, Index bytes, Module& wasm) {
  if (value->type == Type::i32) {
    if (bytes == 1 || bytes == 2) {
      auto shifts = bytes == 1 ? 24 : 16;
      Builder builder(wasm);
      return builder.makeBinary(
        ShrSInt32,
        builder.makeBinary(
          ShlInt32,
          value,
          LiteralUtils::makeFromInt32(shifts, Type::i32, wasm)),
        LiteralUtils::makeFromInt32(shifts, Type::i32, wasm));
    }
    assert(bytes == 4);
    return value; // nothing to do
  } else {
    assert(value->type == Type::i64);
    if (bytes == 1 || bytes == 2 || bytes == 4) {
      auto shifts = bytes == 1 ? 56 : (bytes == 2 ? 48 : 32);
      Builder builder(wasm);
      return builder.makeBinary(
        ShrSInt64,
        builder.makeBinary(
          ShlInt64,
          value,
          LiteralUtils::makeFromInt32(shifts, Type::i64, wasm)),
        LiteralUtils::makeFromInt32(shifts, Type::i64, wasm));
    }
    assert(bytes == 8);
    return value; // nothing to do
  }
}

// getMaxBits() helper that has pessimistic results for the bits used in locals.
struct DummyLocalInfoProvider {
  Index getMaxBitsForLocal(LocalGet* get) {
    if (get->type == Type::i32) {
      return 32;
    } else if (get->type == Type::i64) {
      return 64;
    }
    WASM_UNREACHABLE("type has no integer bit size");
  }
};

// Returns the maximum amount of bits used in an integer expression
// not extremely precise (doesn't look into add operands, etc.)
// LocalInfoProvider is an optional class that can provide answers about
// local.get.
template<typename LocalInfoProvider = DummyLocalInfoProvider>
Index getMaxBits(Expression* curr,
                 LocalInfoProvider* localInfoProvider = nullptr) {
  if (Properties::emitsBoolean(curr)) {
    return 1;
  }
  if (auto* c = curr->dynCast<Const>()) {
    switch (curr->type.getBasic()) {
      case Type::i32:
        return 32 - c->value.countLeadingZeroes().geti32();
      case Type::i64:
        return 64 - c->value.countLeadingZeroes().geti64();
      default:
        WA