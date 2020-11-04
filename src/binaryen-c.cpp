
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

//===============================
// Binaryen C API implementation
//===============================

#include <mutex>

#include "binaryen-c.h"
#include "cfg/Relooper.h"
#include "ir/utils.h"
#include "pass.h"
#include "shell-interface.h"
#include "support/colors.h"
#include "wasm-binary.h"
#include "wasm-builder.h"
#include "wasm-interpreter.h"
#include "wasm-s-parser.h"
#include "wasm-stack.h"
#include "wasm-validator.h"
#include "wasm.h"
#include "wasm2js.h"
#include <iostream>
#include <sstream>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

using namespace wasm;

// Literal utilities

static_assert(sizeof(BinaryenLiteral) == sizeof(Literal),
              "Binaryen C API literal must match wasm.h");

BinaryenLiteral toBinaryenLiteral(Literal x) {
  BinaryenLiteral ret;
  ret.type = x.type.getID();
  assert(x.type.isSingle());
  if (x.type.isBasic()) {
    switch (x.type.getBasic()) {
      case Type::i32:
        ret.i32 = x.geti32();
        return ret;
      case Type::i64:
        ret.i64 = x.geti64();
        return ret;
      case Type::f32:
        ret.i32 = x.reinterpreti32();
        return ret;
      case Type::f64:
        ret.i64 = x.reinterpreti64();
        return ret;
      case Type::v128:
        memcpy(&ret.v128, x.getv128Ptr(), 16);
        return ret;
      case Type::none:
      case Type::unreachable:
        WASM_UNREACHABLE("unexpected type");
    }
  }
  assert(x.type.isRef());
  auto heapType = x.type.getHeapType();
  if (heapType.isBasic()) {
    switch (heapType.getBasic()) {
      case HeapType::i31:
        WASM_UNREACHABLE("TODO: i31");
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
      case HeapType::none:
      case HeapType::noext:
      case HeapType::nofunc:
        // Null.
        return ret;
    }
  }
  if (heapType.isSignature()) {
    ret.func = x.getFunc().str.data();
    return ret;
  }
  assert(x.isData());
  WASM_UNREACHABLE("TODO: gc data");
}

Literal fromBinaryenLiteral(BinaryenLiteral x) {
  auto type = Type(x.type);
  if (type.isBasic()) {
    switch (type.getBasic()) {
      case Type::i32:
        return Literal(x.i32);
      case Type::i64:
        return Literal(x.i64);
      case Type::f32:
        return Literal(x.i32).castToF32();
      case Type::f64:
        return Literal(x.i64).castToF64();
      case Type::v128:
        return Literal(x.v128);
      case Type::none:
      case Type::unreachable:
        WASM_UNREACHABLE("unexpected type");
    }
  }
  assert(type.isRef());
  auto heapType = type.getHeapType();
  if (heapType.isBasic()) {
    switch (heapType.getBasic()) {
      case HeapType::i31:
        WASM_UNREACHABLE("TODO: i31");
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
      case HeapType::none:
      case HeapType::noext:
      case HeapType::nofunc:
        assert(type.isNullable());
        return Literal::makeNull(heapType);
    }
  }
  if (heapType.isSignature()) {
    return Literal::makeFunc(Name(x.func), heapType);
  }
  assert(heapType.isData());
  WASM_UNREACHABLE("TODO: gc data");
}

// Mutexes (global for now; in theory if multiple modules
// are used at once this should be optimized to be per-
// module, but likely it doesn't matter)

static std::mutex BinaryenFunctionMutex;

// Optimization options
static PassOptions globalPassOptions =
  PassOptions::getWithDefaultOptimizationOptions();

extern "C" {

//
// ========== Module Creation ==========
//

// Core types

BinaryenType BinaryenTypeNone(void) { return Type::none; }
BinaryenType BinaryenTypeInt32(void) { return Type::i32; }
BinaryenType BinaryenTypeInt64(void) { return Type::i64; }
BinaryenType BinaryenTypeFloat32(void) { return Type::f32; }
BinaryenType BinaryenTypeFloat64(void) { return Type::f64; }
BinaryenType BinaryenTypeVec128(void) { return Type::v128; }
BinaryenType BinaryenTypeFuncref(void) {
  return Type(HeapType::func, Nullable).getID();
}
BinaryenType BinaryenTypeExternref(void) {
  return Type(HeapType::ext, Nullable).getID();
}
BinaryenType BinaryenTypeAnyref(void) {
  return Type(HeapType::any, Nullable).getID();
}
BinaryenType BinaryenTypeEqref(void) {
  return Type(HeapType::eq, Nullable).getID();
}
BinaryenType BinaryenTypeI31ref(void) {
  return Type(HeapType::i31, Nullable).getID();
}
BinaryenType BinaryenTypeStructref(void) {
  return Type(HeapType::struct_, Nullable).getID();
}
BinaryenType BinaryenTypeArrayref(void) {
  return Type(HeapType::array, Nullable).getID();
}
BinaryenType BinaryenTypeStringref() {
  return Type(HeapType::string, Nullable).getID();
}
BinaryenType BinaryenTypeStringviewWTF8() {
  return Type(HeapType::stringview_wtf8, Nullable).getID();
}
BinaryenType BinaryenTypeStringviewWTF16() {
  return Type(HeapType::stringview_wtf16, Nullable).getID();
}
BinaryenType BinaryenTypeStringviewIter() {
  return Type(HeapType::stringview_iter, Nullable).getID();
}
BinaryenType BinaryenTypeNullref() {
  return Type(HeapType::none, Nullable).getID();
}
BinaryenType BinaryenTypeNullExternref(void) {
  return Type(HeapType::noext, Nullable).getID();
}
BinaryenType BinaryenTypeNullFuncref(void) {
  return Type(HeapType::nofunc, Nullable).getID();
}
BinaryenType BinaryenTypeUnreachable(void) { return Type::unreachable; }
BinaryenType BinaryenTypeAuto(void) { return uintptr_t(-1); }

BinaryenType BinaryenTypeCreate(BinaryenType* types, BinaryenIndex numTypes) {
  std::vector<Type> typeVec;
  typeVec.reserve(numTypes);
  for (BinaryenIndex i = 0; i < numTypes; ++i) {
    typeVec.push_back(Type(types[i]));
  }
  return Type(typeVec).getID();
}

uint32_t BinaryenTypeArity(BinaryenType t) { return Type(t).size(); }

void BinaryenTypeExpand(BinaryenType t, BinaryenType* buf) {
  Type types(t);
  size_t i = 0;
  for (const auto& type : types) {
    buf[i++] = type.getID();
  }
}

WASM_DEPRECATED BinaryenType BinaryenNone(void) { return Type::none; }
WASM_DEPRECATED BinaryenType BinaryenInt32(void) { return Type::i32; }
WASM_DEPRECATED BinaryenType BinaryenInt64(void) { return Type::i64; }
WASM_DEPRECATED BinaryenType BinaryenFloat32(void) { return Type::f32; }
WASM_DEPRECATED BinaryenType BinaryenFloat64(void) { return Type::f64; }
WASM_DEPRECATED BinaryenType BinaryenUndefined(void) { return uint32_t(-1); }

// Packed types

BinaryenPackedType BinaryenPackedTypeNotPacked(void) {
  return Field::PackedType::not_packed;
}
BinaryenPackedType BinaryenPackedTypeInt8(void) {
  return Field::PackedType::i8;
}
BinaryenPackedType BinaryenPackedTypeInt16(void) {
  return Field::PackedType::i16;
}

// Heap types

BinaryenHeapType BinaryenHeapTypeExt() {
  return static_cast<BinaryenHeapType>(HeapType::BasicHeapType::ext);
}
BinaryenHeapType BinaryenHeapTypeFunc() {
  return static_cast<BinaryenHeapType>(HeapType::BasicHeapType::func);
}
BinaryenHeapType BinaryenHeapTypeAny() {
  return static_cast<BinaryenHeapType>(HeapType::BasicHeapType::any);
}
BinaryenHeapType BinaryenHeapTypeEq() {
  return static_cast<BinaryenHeapType>(HeapType::BasicHeapType::eq);
}
BinaryenHeapType BinaryenHeapTypeI31() {
  return static_cast<BinaryenHeapType>(HeapType::BasicHeapType::i31);
}
BinaryenHeapType BinaryenHeapTypeStruct() {
  return static_cast<BinaryenHeapType>(HeapType::BasicHeapType::struct_);
}
BinaryenHeapType BinaryenHeapTypeArray() {
  return static_cast<BinaryenHeapType>(HeapType::BasicHeapType::array);
}
BinaryenHeapType BinaryenHeapTypeString() {
  return static_cast<BinaryenHeapType>(HeapType::BasicHeapType::string);
}
BinaryenHeapType BinaryenHeapTypeStringviewWTF8() {
  return static_cast<BinaryenHeapType>(
    HeapType::BasicHeapType::stringview_wtf8);
}
BinaryenHeapType BinaryenHeapTypeStringviewWTF16() {
  return static_cast<BinaryenHeapType>(
    HeapType::BasicHeapType::stringview_wtf16);
}
BinaryenHeapType BinaryenHeapTypeStringviewIter() {
  return static_cast<BinaryenHeapType>(
    HeapType::BasicHeapType::stringview_iter);
}
BinaryenHeapType BinaryenHeapTypeNone() {
  return static_cast<BinaryenHeapType>(HeapType::BasicHeapType::none);
}
BinaryenHeapType BinaryenHeapTypeNoext() {
  return static_cast<BinaryenHeapType>(HeapType::BasicHeapType::noext);
}
BinaryenHeapType BinaryenHeapTypeNofunc() {
  return static_cast<BinaryenHeapType>(HeapType::BasicHeapType::nofunc);
}

bool BinaryenHeapTypeIsBasic(BinaryenHeapType heapType) {
  return HeapType(heapType).isBasic();
}
bool BinaryenHeapTypeIsSignature(BinaryenHeapType heapType) {
  return HeapType(heapType).isSignature();
}
bool BinaryenHeapTypeIsStruct(BinaryenHeapType heapType) {
  return HeapType(heapType).isStruct();
}
bool BinaryenHeapTypeIsArray(BinaryenHeapType heapType) {
  return HeapType(heapType).isArray();
}
bool BinaryenHeapTypeIsBottom(BinaryenHeapType heapType) {
  return HeapType(heapType).isBottom();
}
BinaryenHeapType BinaryenHeapTypeGetBottom(BinaryenHeapType heapType) {
  return static_cast<BinaryenHeapType>(HeapType(heapType).getBottom());
}
bool BinaryenHeapTypeIsSubType(BinaryenHeapType left, BinaryenHeapType right) {
  return HeapType::isSubType(HeapType(left), HeapType(right));
}
BinaryenIndex BinaryenStructTypeGetNumFields(BinaryenHeapType heapType) {
  auto ht = HeapType(heapType);
  assert(ht.isStruct());
  return ht.getStruct().fields.size();
}
BinaryenType BinaryenStructTypeGetFieldType(BinaryenHeapType heapType,
                                            BinaryenIndex index) {
  auto ht = HeapType(heapType);
  assert(ht.isStruct());
  auto& fields = ht.getStruct().fields;
  assert(index < fields.size());
  return fields[index].type.getID();
}
BinaryenPackedType
BinaryenStructTypeGetFieldPackedType(BinaryenHeapType heapType,
                                     BinaryenIndex index) {
  auto ht = HeapType(heapType);
  assert(ht.isStruct());
  auto& fields = ht.getStruct().fields;
  assert(index < fields.size());
  return fields[index].packedType;
}
bool BinaryenStructTypeIsFieldMutable(BinaryenHeapType heapType,
                                      BinaryenIndex index) {
  auto ht = HeapType(heapType);
  assert(ht.isStruct());
  auto& fields = ht.getStruct().fields;
  assert(index < fields.size());
  return fields[index].mutable_;
}
BinaryenType BinaryenArrayTypeGetElementType(BinaryenHeapType heapType) {
  auto ht = HeapType(heapType);
  assert(ht.isArray());
  return ht.getArray().element.type.getID();
}
BinaryenPackedType
BinaryenArrayTypeGetElementPackedType(BinaryenHeapType heapType) {
  auto ht = HeapType(heapType);
  assert(ht.isArray());
  return ht.getArray().element.packedType;
}
bool BinaryenArrayTypeIsElementMutable(BinaryenHeapType heapType) {
  auto ht = HeapType(heapType);
  assert(ht.isArray());
  return ht.getArray().element.mutable_;
}
BinaryenType BinaryenSignatureTypeGetParams(BinaryenHeapType heapType) {
  auto ht = HeapType(heapType);
  assert(ht.isSignature());
  return ht.getSignature().params.getID();
}
BinaryenType BinaryenSignatureTypeGetResults(BinaryenHeapType heapType) {
  auto ht = HeapType(heapType);
  assert(ht.isSignature());
  return ht.getSignature().results.getID();
}

BinaryenHeapType BinaryenTypeGetHeapType(BinaryenType type) {
  return Type(type).getHeapType().getID();
}
bool BinaryenTypeIsNullable(BinaryenType type) {
  return Type(type).isNullable();
}
BinaryenType BinaryenTypeFromHeapType(BinaryenHeapType heapType,
                                      bool nullable) {
  return Type(HeapType(heapType),
              nullable ? Nullability::Nullable : Nullability::NonNullable)
    .getID();
}

// TypeSystem

BinaryenTypeSystem BinaryenTypeSystemNominal() {
  return static_cast<BinaryenTypeSystem>(TypeSystem::Nominal);
}
BinaryenTypeSystem BinaryenTypeSystemIsorecursive() {
  return static_cast<BinaryenTypeSystem>(TypeSystem::Isorecursive);
}
BinaryenTypeSystem BinaryenGetTypeSystem() {
  return BinaryenTypeSystem(getTypeSystem());
}
void BinaryenSetTypeSystem(BinaryenTypeSystem typeSystem) {
  setTypeSystem(TypeSystem(typeSystem));
}

// Expression ids

BinaryenExpressionId BinaryenInvalidId(void) {
  return Expression::Id::InvalidId;
}

#define DELEGATE(CLASS_TO_VISIT)                                               \
  BinaryenExpressionId Binaryen##CLASS_TO_VISIT##Id(void) {                    \
    return Expression::Id::CLASS_TO_VISIT##Id;                                 \
  }

#include "wasm-delegations.def"

// External kinds

BinaryenExternalKind BinaryenExternalFunction(void) {
  return static_cast<BinaryenExternalKind>(ExternalKind::Function);
}
BinaryenExternalKind BinaryenExternalTable(void) {
  return static_cast<BinaryenExternalKind>(ExternalKind::Table);
}
BinaryenExternalKind BinaryenExternalMemory(void) {
  return static_cast<BinaryenExternalKind>(ExternalKind::Memory);
}
BinaryenExternalKind BinaryenExternalGlobal(void) {
  return static_cast<BinaryenExternalKind>(ExternalKind::Global);
}
BinaryenExternalKind BinaryenExternalTag(void) {
  return static_cast<BinaryenExternalKind>(ExternalKind::Tag);
}

// Features

BinaryenFeatures BinaryenFeatureMVP(void) {
  return static_cast<BinaryenFeatures>(FeatureSet::MVP);
}
BinaryenFeatures BinaryenFeatureAtomics(void) {
  return static_cast<BinaryenFeatures>(FeatureSet::Atomics);
}
BinaryenFeatures BinaryenFeatureBulkMemory(void) {
  return static_cast<BinaryenFeatures>(FeatureSet::BulkMemory);
}
BinaryenFeatures BinaryenFeatureMutableGlobals(void) {
  return static_cast<BinaryenFeatures>(FeatureSet::MutableGlobals);
}
BinaryenFeatures BinaryenFeatureNontrappingFPToInt(void) {
  return static_cast<BinaryenFeatures>(FeatureSet::TruncSat);
}
BinaryenFeatures BinaryenFeatureSignExt(void) {
  return static_cast<BinaryenFeatures>(FeatureSet::SignExt);
}
BinaryenFeatures BinaryenFeatureSIMD128(void) {
  return static_cast<BinaryenFeatures>(FeatureSet::SIMD);
}
BinaryenFeatures BinaryenFeatureExceptionHandling(void) {
  return static_cast<BinaryenFeatures>(FeatureSet::ExceptionHandling);
}
BinaryenFeatures BinaryenFeatureTailCall(void) {
  return static_cast<BinaryenFeatures>(FeatureSet::TailCall);
}
BinaryenFeatures BinaryenFeatureReferenceTypes(void) {
  return static_cast<BinaryenFeatures>(FeatureSet::ReferenceTypes);
}
BinaryenFeatures BinaryenFeatureMultivalue(void) {
  return static_cast<BinaryenFeatures>(FeatureSet::Multivalue);
}
BinaryenFeatures BinaryenFeatureGC(void) {
  return static_cast<BinaryenFeatures>(FeatureSet::GC);
}
BinaryenFeatures BinaryenFeatureMemory64(void) {
  return static_cast<BinaryenFeatures>(FeatureSet::Memory64);
}
BinaryenFeatures BinaryenFeatureRelaxedSIMD(void) {
  return static_cast<BinaryenFeatures>(FeatureSet::RelaxedSIMD);
}
BinaryenFeatures BinaryenFeatureExtendedConst(void) {
  return static_cast<BinaryenFeatures>(FeatureSet::ExtendedConst);
}
BinaryenFeatures BinaryenFeatureStrings(void) {
  return static_cast<BinaryenFeatures>(FeatureSet::Strings);
}
BinaryenFeatures BinaryenFeatureMultiMemories(void) {
  return static_cast<BinaryenFeatures>(FeatureSet::MultiMemories);
}
BinaryenFeatures BinaryenFeatureAll(void) {
  return static_cast<BinaryenFeatures>(FeatureSet::All);
}

// Modules

BinaryenModuleRef BinaryenModuleCreate(void) { return new Module(); }
void BinaryenModuleDispose(BinaryenModuleRef module) { delete (Module*)module; }

// Literals

BinaryenLiteral BinaryenLiteralInt32(int32_t x) {
  return toBinaryenLiteral(Literal(x));
}
BinaryenLiteral BinaryenLiteralInt64(int64_t x) {
  return toBinaryenLiteral(Literal(x));
}
BinaryenLiteral BinaryenLiteralFloat32(float x) {
  return toBinaryenLiteral(Literal(x));
}
BinaryenLiteral BinaryenLiteralFloat64(double x) {
  return toBinaryenLiteral(Literal(x));
}
BinaryenLiteral BinaryenLiteralVec128(const uint8_t x[16]) {
  return toBinaryenLiteral(Literal(x));
}
BinaryenLiteral BinaryenLiteralFloat32Bits(int32_t x) {
  return toBinaryenLiteral(Literal(x).castToF32());
}
BinaryenLiteral BinaryenLiteralFloat64Bits(int64_t x) {
  return toBinaryenLiteral(Literal(x).castToF64());
}

// Expressions

BinaryenOp BinaryenClzInt32(void) { return ClzInt32; }
BinaryenOp BinaryenCtzInt32(void) { return CtzInt32; }
BinaryenOp BinaryenPopcntInt32(void) { return PopcntInt32; }
BinaryenOp BinaryenNegFloat32(void) { return NegFloat32; }
BinaryenOp BinaryenAbsFloat32(void) { return AbsFloat32; }
BinaryenOp BinaryenCeilFloat32(void) { return CeilFloat32; }
BinaryenOp BinaryenFloorFloat32(void) { return FloorFloat32; }
BinaryenOp BinaryenTruncFloat32(void) { return TruncFloat32; }
BinaryenOp BinaryenNearestFloat32(void) { return NearestFloat32; }
BinaryenOp BinaryenSqrtFloat32(void) { return SqrtFloat32; }
BinaryenOp BinaryenEqZInt32(void) { return EqZInt32; }
BinaryenOp BinaryenClzInt64(void) { return ClzInt64; }
BinaryenOp BinaryenCtzInt64(void) { return CtzInt64; }
BinaryenOp BinaryenPopcntInt64(void) { return PopcntInt64; }
BinaryenOp BinaryenNegFloat64(void) { return NegFloat64; }
BinaryenOp BinaryenAbsFloat64(void) { return AbsFloat64; }
BinaryenOp BinaryenCeilFloat64(void) { return CeilFloat64; }
BinaryenOp BinaryenFloorFloat64(void) { return FloorFloat64; }
BinaryenOp BinaryenTruncFloat64(void) { return TruncFloat64; }
BinaryenOp BinaryenNearestFloat64(void) { return NearestFloat64; }
BinaryenOp BinaryenSqrtFloat64(void) { return SqrtFloat64; }
BinaryenOp BinaryenEqZInt64(void) { return EqZInt64; }
BinaryenOp BinaryenExtendSInt32(void) { return ExtendSInt32; }
BinaryenOp BinaryenExtendUInt32(void) { return ExtendUInt32; }
BinaryenOp BinaryenWrapInt64(void) { return WrapInt64; }
BinaryenOp BinaryenTruncSFloat32ToInt32(void) { return TruncSFloat32ToInt32; }
BinaryenOp BinaryenTruncSFloat32ToInt64(void) { return TruncSFloat32ToInt64; }
BinaryenOp BinaryenTruncUFloat32ToInt32(void) { return TruncUFloat32ToInt32; }
BinaryenOp BinaryenTruncUFloat32ToInt64(void) { return TruncUFloat32ToInt64; }
BinaryenOp BinaryenTruncSFloat64ToInt32(void) { return TruncSFloat64ToInt32; }
BinaryenOp BinaryenTruncSFloat64ToInt64(void) { return TruncSFloat64ToInt64; }
BinaryenOp BinaryenTruncUFloat64ToInt32(void) { return TruncUFloat64ToInt32; }
BinaryenOp BinaryenTruncUFloat64ToInt64(void) { return TruncUFloat64ToInt64; }
BinaryenOp BinaryenReinterpretFloat32(void) { return ReinterpretFloat32; }
BinaryenOp BinaryenReinterpretFloat64(void) { return ReinterpretFloat64; }
BinaryenOp BinaryenExtendS8Int32(void) { return ExtendS8Int32; }
BinaryenOp BinaryenExtendS16Int32(void) { return ExtendS16Int32; }
BinaryenOp BinaryenExtendS8Int64(void) { return ExtendS8Int64; }
BinaryenOp BinaryenExtendS16Int64(void) { return ExtendS16Int64; }
BinaryenOp BinaryenExtendS32Int64(void) { return ExtendS32Int64; }
BinaryenOp BinaryenConvertSInt32ToFloat32(void) {
  return ConvertSInt32ToFloat32;
}
BinaryenOp BinaryenConvertSInt32ToFloat64(void) {
  return ConvertSInt32ToFloat64;
}
BinaryenOp BinaryenConvertUInt32ToFloat32(void) {
  return ConvertUInt32ToFloat32;
}
BinaryenOp BinaryenConvertUInt32ToFloat64(void) {
  return ConvertUInt32ToFloat64;
}
BinaryenOp BinaryenConvertSInt64ToFloat32(void) {
  return ConvertSInt64ToFloat32;
}
BinaryenOp BinaryenConvertSInt64ToFloat64(void) {
  return ConvertSInt64ToFloat64;
}
BinaryenOp BinaryenConvertUInt64ToFloat32(void) {
  return ConvertUInt64ToFloat32;
}
BinaryenOp BinaryenConvertUInt64ToFloat64(void) {
  return ConvertUInt64ToFloat64;
}
BinaryenOp BinaryenPromoteFloat32(void) { return PromoteFloat32; }
BinaryenOp BinaryenDemoteFloat64(void) { return DemoteFloat64; }
BinaryenOp BinaryenReinterpretInt32(void) { return ReinterpretInt32; }
BinaryenOp BinaryenReinterpretInt64(void) { return ReinterpretInt64; }
BinaryenOp BinaryenAddInt32(void) { return AddInt32; }
BinaryenOp BinaryenSubInt32(void) { return SubInt32; }
BinaryenOp BinaryenMulInt32(void) { return MulInt32; }
BinaryenOp BinaryenDivSInt32(void) { return DivSInt32; }
BinaryenOp BinaryenDivUInt32(void) { return DivUInt32; }
BinaryenOp BinaryenRemSInt32(void) { return RemSInt32; }
BinaryenOp BinaryenRemUInt32(void) { return RemUInt32; }
BinaryenOp BinaryenAndInt32(void) { return AndInt32; }
BinaryenOp BinaryenOrInt32(void) { return OrInt32; }
BinaryenOp BinaryenXorInt32(void) { return XorInt32; }
BinaryenOp BinaryenShlInt32(void) { return ShlInt32; }
BinaryenOp BinaryenShrUInt32(void) { return ShrUInt32; }
BinaryenOp BinaryenShrSInt32(void) { return ShrSInt32; }
BinaryenOp BinaryenRotLInt32(void) { return RotLInt32; }
BinaryenOp BinaryenRotRInt32(void) { return RotRInt32; }
BinaryenOp BinaryenEqInt32(void) { return EqInt32; }
BinaryenOp BinaryenNeInt32(void) { return NeInt32; }
BinaryenOp BinaryenLtSInt32(void) { return LtSInt32; }
BinaryenOp BinaryenLtUInt32(void) { return LtUInt32; }
BinaryenOp BinaryenLeSInt32(void) { return LeSInt32; }
BinaryenOp BinaryenLeUInt32(void) { return LeUInt32; }
BinaryenOp BinaryenGtSInt32(void) { return GtSInt32; }
BinaryenOp BinaryenGtUInt32(void) { return GtUInt32; }
BinaryenOp BinaryenGeSInt32(void) { return GeSInt32; }
BinaryenOp BinaryenGeUInt32(void) { return GeUInt32; }
BinaryenOp BinaryenAddInt64(void) { return AddInt64; }
BinaryenOp BinaryenSubInt64(void) { return SubInt64; }
BinaryenOp BinaryenMulInt64(void) { return MulInt64; }
BinaryenOp BinaryenDivSInt64(void) { return DivSInt64; }
BinaryenOp BinaryenDivUInt64(void) { return DivUInt64; }
BinaryenOp BinaryenRemSInt64(void) { return RemSInt64; }
BinaryenOp BinaryenRemUInt64(void) { return RemUInt64; }
BinaryenOp BinaryenAndInt64(void) { return AndInt64; }
BinaryenOp BinaryenOrInt64(void) { return OrInt64; }
BinaryenOp BinaryenXorInt64(void) { return XorInt64; }
BinaryenOp BinaryenShlInt64(void) { return ShlInt64; }
BinaryenOp BinaryenShrUInt64(void) { return ShrUInt64; }
BinaryenOp BinaryenShrSInt64(void) { return ShrSInt64; }