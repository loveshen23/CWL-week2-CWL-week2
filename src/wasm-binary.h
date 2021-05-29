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
// We mostly stream into a buffer as we create the binary format, however,
// sometimes we need to backtrack and write to a location behind us - wasm
// is optimized for reading, not writing.
//
class BufferWithRandomAccess : public std::vector<uint8_t> {
public:
  BufferWithRandomAccess() = default;

  BufferWithRandomAccess& operator<<(int8_t x) {
    BYN_TRACE("writeInt8: " << (int)(uint8_t)x << " (at " << size() << ")\n");
    push_back(x);
    return *this;
  }
  BufferWithRandomAccess& operator<<(int16_t x) {
    BYN_TRACE("writeInt16: " << x << " (at " << size() << ")\n");
    push_back(x & 0xff);
    push_back(x >> 8);
    return *this;
  }
  BufferWithRandomAccess& operator<<(int32_t x) {
    BYN_TRACE("writeInt32: " << x << " (at " << size() << ")\n");
    push_back(x & 0xff);
    x >>= 8;
    push_back(x & 0xff);
    x >>= 8;
    push_back(x & 0xff);
    x >>= 8;
    push_back(x & 0xff);
    return *this;
  }
  BufferWithRandomAccess& operator<<(int64_t x) {
    BYN_TRACE("writeInt64: " << x << " (at " << size() << ")\n");
    push_back(x & 0xff);
    x >>= 8;
    push_back(x & 0xff);
    x >>= 8;
    push_back(x & 0xff);
    x >>= 8;
    push_back(x & 0xff);
    x >>= 8;
    push_back(x & 0xff);
    x >>= 8;
    push_back(x & 0xff);
    x >>= 8;
    push_back(x & 0xff);
    x >>= 8;
    push_back(x & 0xff);
    return *this;
  }
  BufferWithRandomAccess& operator<<(U32LEB x) {
    [[maybe_unused]] size_t before = -1;
    BYN_DEBUG(before = size(); std::cerr << "writeU32LEB: " << x.value
                                         << " (at " << before << ")"
                                         << std::endl;);
    x.write(this);
    BYN_DEBUG(for (size_t i = before; i < size(); i++) {
      std::cerr << "  " << (int)at(i) << " (at " << i << ")\n";
    });
    return *this;
  }
  BufferWithRandomAccess& operator<<(U64LEB x) {
    [[maybe_unused]] size_t before = -1;
    BYN_DEBUG(before = size(); std::cerr << "writeU64LEB: " << x.value
                                         << " (at " << before << ")"
                                         << std::endl;);
    x.write(this);
    BYN_DEBUG(for (size_t i = before; i < size(); i++) {
      std::cerr << "  " << (int)at(i) << " (at " << i << ")\n";
    });
    return *this;
  }
  BufferWithRandomAccess& operator<<(S32LEB x) {
    [[maybe_unused]] size_t before = -1;
    BYN_DEBUG(before = size(); std::cerr << "writeS32LEB: " << x.value
                                         << " (at " << before << ")"
                                         << std::endl;);
    x.write(this);
    BYN_DEBUG(for (size_t i = before; i < size(); i++) {
      std::cerr << "  " << (int)at(i) << " (at " << i << ")\n";
    });
    return *this;
  }
  BufferWithRandomAccess& operator<<(S64LEB x) {
    [[maybe_unused]] size_t before = -1;
    BYN_DEBUG(before = size(); std::cerr << "writeS64LEB: " << x.value
                                         << " (at " << before << ")"
                                         << std::endl;);
    x.write(this);
    BYN_DEBUG(for (size_t i = before; i < size(); i++) {
      std::cerr << "  " << (int)at(i) << " (at " << i << ")\n";
    });
    return *this;
  }

  BufferWithRandomAccess& operator<<(uint8_t x) { return *this << (int8_t)x; }
  BufferWithRandomAccess& operator<<(uint16_t x) { return *this << (int16_t)x; }
  BufferWithRandomAccess& operator<<(uint32_t x) { return *this << (int32_t)x; }
  BufferWithRandomAccess& operator<<(uint64_t x) { return *this << (int64_t)x; }

  BufferWithRandomAccess& operator<<(float x) {
    BYN_TRACE("writeFloat32: " << x << " (at " << size() << ")\n");
    return *this << Literal(x).reinterpreti32();
  }
  BufferWithRandomAccess& operator<<(double x) {
    BYN_TRACE("writeFloat64: " << x << " (at " << size() << ")\n");
    return *this << Literal(x).reinterpreti64();
  }

  void writeAt(size_t i, uint16_t x) {
    BYN_TRACE("backpatchInt16: " << x << " (at " << i << ")\n");
    (*this)[i] = x & 0xff;
    (*this)[i + 1] = x >> 8;
  }
  void writeAt(size_t i, uint32_t x) {
    BYN_TRACE("backpatchInt32: " << x << " (at " << i << ")\n");
    (*this)[i] = x & 0xff;
    x >>= 8;
    (*this)[i + 1] = x & 0xff;
    x >>= 8;
    (*this)[i + 2] = x & 0xff;
    x >>= 8;
    (*this)[i + 3] = x & 0xff;
  }

  // writes out an LEB to an arbitrary location. this writes the LEB as a full
  // 5 bytes, the fixed amount that can easily be set aside ahead of time
  void writeAtFullFixedSize(size_t i, U32LEB x) {
    BYN_TRACE("backpatchU32LEB: " << x.value << " (at " << i << ")\n");
    // fill all 5 bytes, we have to do this when backpatching
    x.writeAt(this, i, MaxLEB32Bytes);
  }
  // writes out an LEB of normal size
  // returns how many bytes were written
  size_t writeAt(size_t i, U32LEB x) {
    BYN_TRACE("writeAtU32LEB: " << x.value << " (at " << i << ")\n");
    return x.writeAt(this, i);
  }

  template<typename T> void writeTo(T& o) {
    for (auto c : *this) {
      o << c;
    }
  }

  std::vector<char> getAsChars() {
    std::vector<char> ret;
    ret.resize(size());
    std::copy(begin(), end(), ret.begin());
    return ret;
  }
};

namespace BinaryConsts {

enum Meta { Magic = 0x6d736100, Version = 0x01 };

enum Section {
  Custom = 0,
  Type = 1,
  Import = 2,
  Function = 3,
  Table = 4,
  Memory = 5,
  Global = 6,
  Export = 7,
  Start = 8,
  Element = 9,
  Code = 10,
  Data = 11,
  DataCount = 12,
  Tag = 13,
  Strings = 14,
};

// A passive segment is a segment that will not be automatically copied into a
//   memory or table on instantiation, and must instead be applied manually
//   using the instructions memory.init or table.init.
// An active segment is equivalent to a passive segment, but with an implicit
//   memory.init followed by a data.drop (or table.init followed by a elem.drop)
//   that is prepended to the module's start function.
// A declarative element segment is not available at runtime but merely serves
//   to forward-declare references that are formed in code with instructions
//   like ref.func.
enum SegmentFlag {
  // Bit 0: 0 = active, 1 = passive
  IsPassive = 1 << 0,
  // Bit 1 if passive: 0 = passive, 1 = declarative
  IsDeclarative = 1 << 1,
  // Bit 1 if active: 0 = index 0, 1 = index given
  HasIndex = 1 << 1,
  // Table element segments only:
  // Bit 2: 0 = elemType is funcref and a vector of func indexes given
  //        1 = elemType is given and a vector of ref expressions is given
  UsesExpressions = 1 << 2
};

enum EncodedType {
  // value_type
  i32 = -0x1,  // 0x7f
  i64 = -0x2,  // 0x7e
  f32 = -0x3,  // 0x7d
  f64 = -0x4,  // 0x7c
  v128 = -0x5, // 0x7b
  i8 = -0x6,   // 0x7a
  i16 = -0x7,  // 0x79
  // function reference type
  funcref = -0x10, // 0x70
  // external (host) references
  externref = -0x11, // 0x6f
  // top type of references to non-function Wasm data.
  anyref = -0x12, // 0x6e
  // comparable reference type
  eqref = -0x13, // 0x6d
  // nullable typed function reference type, with parameter
  nullable = -0x14, // 0x6c
  // non-nullable typed function reference type, with parameter
  nonnullable = -0x15, // 0x6b
  // integer reference type
  i31ref = -0x16, // 0x6a
  // gc and string reference types
  structref = -0x19,        // 0x67
  arrayref = -0x1a,         // 0x66
  stringref = -0x1c,        // 0x64
  stringview_wtf8 = -0x1d,  // 0x63
  stringview_wtf16 = -0x1e, // 0x62
  stringview_iter = -0x1f,  // 0x61
  // bottom types
  nullexternref = -0x17, // 0x69
  nullfuncref = -0x18,   // 0x68
  nullref = -0x1b,       // 0x65
  // type forms
  Func = -0x20,   // 0x60
  Struct = -0x21, // 0x5f
  Array = -0x22,  // 0x5e
  Sub = -0x30,    // 0x50
  // prototype nominal forms we still parse
  FuncSubtype = -0x23,   // 0x5d
  StructSubtype = -0x24, // 0x5c
  ArraySubtype = -0x25,  // 0x5b
  // isorecursive recursion groups
  Rec = -0x31, // 0x4f
  // block_type
  Empty = -0x40 // 0x40
};

enum EncodedHeapType {
  func = -0x10,    // 0x70
  ext = -0x11,     // 0x6f
  any = -0x12,     // 0x6e
  eq = -0x13,      // 0x6d
  i31 = -0x16,     // 0x6a
  struct_ = -0x19, // 0x67
  array = -0x1a,   // 0x66
  string = -0x1c,  // 0x64
  // stringview/iter constants are identical to type, and cannot be duplicated
  // here as that would be a compiler error, so add _heap suffixes. See
  // https://github.com/WebAssembly/stringref/issues/12
  stringview_wtf8_heap = -0x1d,  // 0x63
  stringview_wtf16_heap = -0x1e, // 0x62
  stringview_iter_heap = -0x1f,  // 0x61
  // bottom types
  noext = -0x17,  // 0x69
  nofunc = -0x18, // 0x68
  none = -0x1b,   // 0x65
};

namespace CustomSections {
extern const char* Name;
extern const char* SourceMapUrl;
extern const char* Dylink;
extern const char* Dylink0;
extern const char* Linking;
extern const char* Producers;
extern const char* TargetFeatures;

extern const char* AtomicsFeature;
extern const char* BulkMemoryFeature;
extern const char* ExceptionHandlingFeature;
extern const char* MutableGlobalsFeature;
extern const char* TruncSatFeature;
extern const char* SignExtFeature;
extern const char* SIMD128Feature;
extern const char* ExceptionHandlingFeature;
extern const char* TailCallFeature;
extern const char* ReferenceTypesFeature;
extern const char* MultivalueFeature;
extern const char* GCFeature;
extern const char* Memory64Feature;
extern const char* RelaxedSIMDFeature;
extern const char* ExtendedConstFeature;
extern const char* StringsFeature;
extern const char* MultiMemoriesFeature;

enum Subsection {
  NameModule = 0,
  NameFunction = 1,
  NameLocal = 2,
  // see: https://github.com/WebAssembly/extended-name-section
  NameLabel = 3,
  NameType = 4,
  NameTable = 5,
  NameMemory = 6,
  NameGlobal = 7,
  NameElem = 8,
  NameData = 9,
  // see: https://github.com/WebAssembly/gc/issues/193
  NameField = 10,
  NameTag = 11,

  DylinkMemInfo = 1,
  DylinkNeeded = 2,
};

} // namespace CustomSections

enum ASTNodes {
  Unreachable = 0x00,
  Nop = 0x01,
  Block = 0x02,
  Loop = 0x03,
  If = 0x04,
  Else = 0x05,

  End = 0x0b,
  Br = 0x0c,
  BrIf = 0x0d,
  BrTable = 0x0e,
  Return = 0x0f,

  CallFunction = 0x10,
  CallIndirect = 0x11,
  RetCallFunction = 0x12,
  RetCallIndirect = 0x13,

  Drop = 0x1a,
  Select = 0x1b,
  SelectWithType = 0x1c, // added in reference types proposal

  LocalGet = 0x20,
  LocalSet = 0x21,
  LocalTee = 0x22,
  GlobalGet = 0x23,
  GlobalSet = 0x24,

  TableGet = 0x25,
  TableSet = 0x26,

  I32LoadMem = 0x28,
  I64LoadMem = 0x29,
  F32LoadMem = 0x2a,
  F64LoadMem = 0x2b,

  I32LoadMem8S = 0x2c,
  I32LoadMem8U = 0x2d,
  I32LoadMem16S = 0x2e,
  I32LoadMem16U = 0x2f,
  I64LoadMem8S = 0x30,
  I64LoadMem8U = 0x31,
  I64LoadMem16S = 0x32,
  I64LoadMem16U = 0x33,
  I64LoadMem32S = 0x34,
  I64LoadMem32U = 0x35,

  I32StoreMem = 0x36,
  I64StoreMem = 0x37,
  F32StoreMem = 0x38,
  F64StoreMem = 0x39,

  I32StoreMem8 = 0x3a,
  I32StoreMem16 = 0x3b,
  I64StoreMem8 = 0x3c,
  I64StoreMem16 = 0x3d,
  I64StoreMem32 = 0x3e,

  MemorySize = 0x3f,
  MemoryGrow = 0x40,

  I32Const = 0x41,
  I64Const = 0x42,
  F32Const = 0x43,
  F64Const = 0x44,

  I32EqZ = 0x45,
  I32Eq = 0x46,
  I32Ne = 0x47,
  I32LtS = 0x48,
  I32LtU = 0x49,
  I32GtS = 0x4a,
  I32GtU = 0x4b,
  I32LeS = 0x4c,
  I32LeU = 0x4d,
  I32GeS = 0x4e,
  I32GeU = 0x4f,
  I64EqZ = 0x50,
  I64Eq = 0x51,
  I64Ne = 0x52,
  I64LtS = 0x53,
  I64LtU = 0x54,
  I64GtS = 0x55,
  I64GtU = 0x56,
  I64LeS = 0x57,
  I64LeU = 0x58,
  I64GeS = 0x59,
  I64GeU = 0x5a,
  F32Eq = 0x5b,
  F32Ne = 0x5c,
  F32Lt = 0x5d,
  F32Gt = 0x5e,
  F32Le = 0x5f,
  F32Ge = 0x60,
  F64Eq = 0x61,
  F64Ne = 0x62,
  F64Lt = 0x63,
  F64Gt = 0x64,
  F64Le = 0x65,
  F64Ge = 0x66,

  I32Clz = 0x67,
  I32Ctz = 0x68,
  I32Popcnt = 0x69,
  I32Add = 0x6a,
  I32Sub = 0x6b,
  I32Mul = 0x6c,
  I32DivS = 0x6d,
  I32DivU = 0x6e,
  I32RemS = 0x6f,
  I32RemU = 0x70,
  I32And = 0x71,
  I32Or = 0x72,
  I32Xor = 0x73,
  I32Shl = 0x74,
  I32ShrS = 0x75,
  I32ShrU = 0x76,
  I32RotL = 0x77,
  I32RotR = 0x78,

  I64Clz = 0x79,
  I64Ctz = 0x7a,
  I64Popcnt = 0x7b,
  I64Add = 0x7c,
  I64Sub = 0x7d,
  I64Mul = 0x7e,
  I64DivS = 0x7f,
  I64DivU = 0x80,
  I64RemS = 0x81,
  I64RemU = 