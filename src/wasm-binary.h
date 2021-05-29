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
// Parses and emits WebAssembly binary code
//

#ifndef wasm_wasm_binary_h
#define wasm_wasm_binary_h

#include <cassert>
#include <ostream>
#include <type_traits>

#include "ir/import-utils.h"
#include "ir/module-utils.h"
#include "parsing.h"
#include "support/debug.h"
#include "wasm-builder.h"
#include "wasm-traversal.h"
#include "wasm-validator.h"
#include "wasm.h"

#define DEBUG_TYPE "binary"

namespace wasm {

enum {
  // the maximum amount of bytes we emit per LEB
  MaxLEB32Bytes = 5,
};

// wasm VMs on the web have decided to impose some limits on what they
// accept
enum WebLimitations : uint32_t {
  MaxDataSegments = 100 * 1000,
  MaxFunctionBodySize = 128 * 1024,
  MaxFunctionLocals = 50 * 1000,
  MaxFunctionParams = 1000
};

template<typename T, typename MiniT> struct LEB {
  static_assert(sizeof(MiniT) == 1, "MiniT must be a byte");

  T value;

  LEB() = default;
  LEB(T value) : value(value) {}

  bool hasMore(T temp, MiniT byte) {
    // for signed, we must ensure the last bit has the right sign, as it will
    // zero extend
    return std::is_signed<T>::value
             ? (temp != 0 && temp != T(-1)) || (value >= 0 && (byte & 64)) ||
                 (value < 0 && !(byte & 64))
             : (temp != 0);
  }

  void write(std::vector<uint8_t>* out) {
    T temp = value;
    bool more;
    do {
      uint8_t byte = temp & 127;
      temp >>= 7;
      more = hasMore(temp, byte);
      if (more) {
        byte = byte | 128;
      }
      out->push_back(byte);
    } while (more);
  }

  // @minimum: a minimum number of bytes to write, padding as necessary
  // returns the number of bytes written
  size_t writeAt(std::vector<uint8_t>* out, size_t at, size_t minimum = 0) {
    T temp = value;
    size_t offset = 0;
    bool more;
    do {
      uint8_t byte = temp & 127;
      temp >>= 7;
      more = hasMore(temp, byte) || offset + 1 < minimum;
      if (more) {
        byte = byte | 128;
      }
      (*out)[at + offset] = byte;
      offset++;
    } while (more);
    return offset;
  }

  LEB<T, MiniT>& read(std::function<MiniT()> get) {
    value = 0;
    T shift = 0;
    MiniT byte;
    while (1) {
      byte = get();
      bool last = !(byte & 128);
      T payload = byte & 127;
      using mask_type = typename std::make_unsigned<T>::type;
      auto shift_mask = 0 == shift
                          ? ~mask_type(0)
                          : ((mask_type(1) << (sizeof(T) * 8 - shift)) - 1u);
      T significant_payload = payload & shift_mask;
      if (significant_payload != payload) {
        if (!(std::is_signed<T>::value && last)) {
          throw ParseException("LEB dropped bits only valid for signed LEB");
        }
      }
      value |= significant_payload << shift;
      if (last) {
        break;
      }
      shift += 7;
      if (size_t(shift) >= sizeof(T) * 8) {
        throw ParseException("LEB overflow");
      }
    }
    // If signed LEB, then we might need to sign-extend. (compile should
    // optimize this out if not needed).
    if (std::is_signed<T>::value) {
      shift += 7;
      if ((byte & 64) && size_t(shift) < 8 * sizeof(T)) {
        size_t sext_bits = 8 * sizeof(T) - size_t(shift);
        value <<= sext_bits;
        value >>= sext_bits;
        if (value >= 0) {
          throw ParseException(
            " LEBsign-extend should produce a negative value");
        }
      }
    }
    return *this;
  }
};

using U32LEB = LEB<uint32_t, uint8_t>;
using U64LEB = LEB<uint64_t, uint8_t>;
using S32LEB = LEB<int32_t, int8_t>;
using S64LEB = LEB<int64_t, int8_t>;

//
// We mostly stream into a buffer as we create the binary format, how