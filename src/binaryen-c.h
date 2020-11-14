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

//================
// Binaryen C API
//
// The first part of the API lets you create modules and their parts.
//
// The second part of the API lets you perform operations on modules.
//
// The third part of the API lets you provide a general control-flow
//   graph (CFG) as input.
//
// The final part of the API contains miscellaneous utilities like
//   debugging for the API itself.
//
// ---------------
//
// Thread safety: You can create Expressions in parallel, as they do not
//                refer to global state. BinaryenAddFunction is also
//                thread-safe, which means that you can create functions and
//                their contents in multiple threads. This is important since
//                functions are where the majority of the work is done.
//                Other methods - creating imports, exports, etc. - are
//                not currently thread-safe (as there is typically no need
//                to parallelize them).
//
//================

#ifndef wasm_binaryen_c_h
#define wasm_binaryen_c_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __GNUC__
#define WASM_DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
#define WASM_DEPRECATED __declspec(deprecated)
#else
#define WASM_DEPRECATED
#endif

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#define BINARYEN_API EMSCRIPTEN_KEEPALIVE
#elif defined(_MSC_VER) && !defined(BUILD_STATIC_LIBRARY)
#define BINARYEN_API __declspec(dllexport)
#else
#define BINARYEN_API
#endif

#ifdef __cplusplus
#define BINARYEN_REF(NAME)                                                     \
  namespace wasm {                                                             \
  class NAME;                                                                  \
  };                                                                           \
  typedef class wasm::NAME* Binaryen##NAME##Ref;
#else
#define BINARYEN_REF(NAME) typedef struct Binaryen##NAME* Binaryen##NAME##Ref;
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
// ========== Module Creation ==========
//

// BinaryenIndex
//
// Used for internal indexes and list sizes.

typedef uint32_t BinaryenIndex;

// Core types (call to get the value of each; you can cache them, they
// never change)

typedef uintptr_t BinaryenType;

BINARYEN_API BinaryenType BinaryenTypeNone(void);
BINARYEN_API BinaryenType BinaryenTypeInt32(void);
BINARYEN_API BinaryenType BinaryenTypeInt64(void);
BINARYEN_API BinaryenType BinaryenTypeFloat32(void);
BINARYEN_API BinaryenType BinaryenTypeFloat64(void);
BINARYEN_API BinaryenType BinaryenTypeVec128(void);
BINARYEN_API BinaryenType BinaryenTypeFuncref(void);
BINARYEN_API BinaryenType BinaryenTypeExternref(void);
BINARYEN_API BinaryenType BinaryenTypeAnyref(void);
BINARYEN_API BinaryenType BinaryenTypeEqref(void);
BINARYEN_API BinaryenType BinaryenTypeI31ref(void);
BINARYEN_API BinaryenType BinaryenTypeStructref(void);
BINARYEN_API BinaryenType BinaryenTypeArrayref(void);
BINARYEN_API BinaryenType BinaryenTypeStringref(void);
BINARYEN_API BinaryenType BinaryenTypeStringviewWTF8(void);
BINARYEN_API BinaryenType BinaryenTypeStringviewWTF16(void);
BINARYEN_API BinaryenType BinaryenTypeStringviewIter(void);
BINARYEN_API BinaryenType BinaryenTypeNullref(void);
BINARYEN_API BinaryenType BinaryenTypeNullExternref(void);
BINARYEN_API BinaryenType BinaryenTypeNullFuncref(void);
BINARYEN_API BinaryenType BinaryenTypeUnreachable(void);
// Not a real type. Used as the last parameter to BinaryenBlock to let
// the API figure out the type instead of providing one.
BINARYEN_API BinaryenType BinaryenTypeAuto(void);
BINARYEN_API BinaryenType BinaryenTypeCreate(BinaryenType* valueTypes,
                                             BinaryenIndex numTypes);
BINARYEN_API uint32_t BinaryenTypeArity(BinaryenType t);
BINARYEN_API void BinaryenTypeExpand(BinaryenType t, BinaryenType* buf);

WASM_DEPRECATED BinaryenType BinaryenNone(void);
WASM_DEPRECATED BinaryenType BinaryenInt32(void);
WASM_DEPRECATED BinaryenType BinaryenInt64(void);
WASM_DEPRECATED BinaryenType BinaryenFloat32(void);
WASM_DEPRECATED BinaryenType BinaryenFloat64(void);
WASM_DEPRECATED BinaryenType BinaryenUndefined(void);

// Packed types (call to get the value of each; you can cache them)

typedef uint32_t BinaryenPackedType;

BINARYEN_API BinaryenPackedType BinaryenPackedTypeNotPacked(void);
BINARYEN_API BinaryenPackedType BinaryenPackedTypeInt8(void);
BINARYEN_API BinaryenPackedType BinaryenPackedTypeInt16(void);

// Heap types

typedef uintptr_t BinaryenHeapType;

BINARYEN_API BinaryenHeapType BinaryenHeapTypeExt(void);
BINARYEN_API BinaryenHeapType BinaryenHeapTypeFunc(void);
BINARYEN_API BinaryenHeapType BinaryenHeapTypeAny(void);
BINARYEN_API BinaryenHeapType BinaryenHeapTypeEq(void);
BINARYEN_API BinaryenHeapType BinaryenHeapTypeI31(void);
BINARYEN_API BinaryenHeapType BinaryenHeapTypeStruct(void);
BINARYEN_API BinaryenHeapType BinaryenHeapTypeArray(void);
BINARYEN_API BinaryenHeapType BinaryenHeapTypeString(void);
BINARYEN_API BinaryenHeapType BinaryenHeapTypeStringviewWTF8(void);
BINARYEN_API BinaryenHeapType BinaryenHeapTypeStringviewWTF16(void);
BINARYEN_API BinaryenHeapType BinaryenHeapTypeStringviewIter(void);
BINARYEN_API BinaryenHeapType BinaryenHeapTypeNone(void);
BINARYEN_API BinaryenHeapType BinaryenHeapTypeNoext(void);
BINARYEN_API BinaryenHeapType BinaryenHeapTypeNofunc(void);

BINARYEN_API bool BinaryenHeapTypeIsBasic(BinaryenHeapType heapType);
BINARYEN_API bool BinaryenHeapTypeIsSignature(BinaryenHeapType heapType);
BINARYEN_API bool BinaryenHeapTypeIsStruct(BinaryenHeapType heapType);
BINARYEN_API bool BinaryenHeapTypeIsArray(BinaryenHeapType heapType);
BINARYEN_API bool BinaryenHeapTypeIsBottom(BinaryenHeapType heapType);
BINARYEN_API BinaryenHeapType
BinaryenHeapTypeGetBottom(BinaryenHeapType heapType);
BINARYEN_API bool BinaryenHeapTypeIsSubType(BinaryenHeapType left,
                                            BinaryenHeapType right);
BINARYEN_API BinaryenIndex
BinaryenStructTypeGetNumFields(BinaryenHeapType heapType);
BINARYEN_API BinaryenType
BinaryenStructTypeGetFieldType(BinaryenHeapType heapType, BinaryenIndex index);
BINARYEN_API BinaryenPackedType BinaryenStructTypeGetFieldPackedType(
  BinaryenHeapType heapType, BinaryenIndex index);
BINARYEN_API bool BinaryenStructTypeIsFieldMutable(BinaryenHeapType heapType,
                                                   BinaryenIndex index);
BINARYEN_API BinaryenType
BinaryenArrayTypeGetElementType(BinaryenHeapType heapType);
BINARYEN_API BinaryenPackedType
BinaryenArrayTypeGetElementPackedType(BinaryenHeapType heapType);
BINARYEN_API bool BinaryenArrayTypeIsElementMutable(BinaryenHeapType heapType);
BINARYEN_API BinaryenType
BinaryenSignatureTypeGetParams(BinaryenHeapType heapType);
BINARYEN_API BinaryenType
BinaryenSignatureTypeGetResults(BinaryenHeapType heapType);

BINARYEN_API BinaryenHeapType BinaryenTypeGetHeapType(BinaryenType type);
BINARYEN_API bool BinaryenTypeIsNullable(BinaryenType type);
BINARYEN_API BinaryenType BinaryenTypeFromHeapType(BinaryenHeapType heapType,
                                                   bool nullable);

// TypeSystem

typedef uint32_t BinaryenTypeSystem;

BINARYEN_API BinaryenTypeSystem BinaryenTypeSystemNominal(void);
BINARYEN_API BinaryenTypeSystem BinaryenTypeSystemIsorecursive(void);
BINARYEN_API BinaryenTypeSystem BinaryenGetTypeSystem(void);
BINARYEN_API void BinaryenSetTypeSystem(BinaryenTypeSystem typeSystem);

// Expression ids (call to get the value of each; you can cache them)

typedef uint32_t BinaryenExpressionId;

BINARYEN_API BinaryenExpressionId BinaryenInvalidId(void);

#define DELEGATE(CLASS_TO_VISIT)                                               \
  BINARYEN_API BinaryenExpressionId Binaryen##CLASS_TO_VISIT##Id(void);

#include "wasm-delegations.def"

// External kinds (call to get the value of each; you can cache them)

typedef uint32_t BinaryenExternalKind;

BINARYEN_API BinaryenExternalKind BinaryenExternalFunction(void);
BINARYEN_API BinaryenExternalKind BinaryenExternalTable(void);
BINARYEN_API BinaryenExternalKind BinaryenExternalMemory(void);
BINARYEN_API BinaryenExternalKind BinaryenExternalGlobal(void);
BINARYEN_API BinaryenExternalKind BinaryenExternalTag(void);

// Features. Call to get the value of each; you can cache them. Use bitwise
// operators to combine and test particular features.

typedef uint32_t BinaryenFeatures;

BINARYEN_API BinaryenFeatures BinaryenFeatureMVP(void);
BINARYEN_API BinaryenFeatures BinaryenFeatureAtomics(void);
BINARYEN_API BinaryenFeatures BinaryenFeatureBulkMemory(void);
BINARYEN_API BinaryenFeatures BinaryenFeatureMutableGlobals(void);
BINARYEN_API BinaryenFeatures BinaryenFeatureNontrappingFPToInt(void);
BINARYEN_API BinaryenFeatures BinaryenFeatureSignExt(void);
BINARYEN_API BinaryenFeatures BinaryenFeatureSIMD128(void);
BINARYEN_API BinaryenFeatures BinaryenFeatureExceptionHandling(void);
BINARYEN_API BinaryenFeatures BinaryenFeatureTailCall(void);
BINARYEN_API BinaryenFeatures BinaryenFeatureReferenceTypes(void);
BINARYEN_API BinaryenFeatures BinaryenFeatureMultivalue(void);
BINARYEN_API BinaryenFeatures BinaryenFeatureGC(void);
BINARYEN_API BinaryenFeatures BinaryenFeatureMemory64(void);
BINARYEN_API BinaryenFeatures BinaryenFeatureRelaxedSIMD(void);
BINARYEN_API BinaryenFeatures BinaryenFeatureExtendedConst(void);
BINARYEN_API BinaryenFeatures BinaryenFeatureStrings(void);
BINARYEN_API BinaryenFeatures BinaryenFeatureMultiMemories(void);
BINARYEN_API BinaryenFeatures BinaryenFeatureAll(void);

// Modules
//
// Modules contain lists of functions, imports, exports, function types. The
// Add* methods create them on a module. The module owns them and will free
// their memory when the module is disposed of.
//
// Expressions are also allocated inside modules, and freed with the module.
// They are not created by Add* methods, since they are not added directly on
// the module, instead, they are arguments to other expressions (and then they
// are the children of that AST node), or to a function (and then they are the
// body of that function).
//
// A module can also contain a function table for indirect calls, a memory,
// and a start method.

BINARYEN_REF(Module);

BINARYEN_API BinaryenModuleRef BinaryenModuleCreate(void);
BINARYEN_API void BinaryenModuleDispose(BinaryenModuleRef module);

// Literals. These are passed by value.

struct BinaryenLiteral {
  uintptr_t type;
  union {
    int32_t i32;
    int64_t i64;
    float f32;
    double f64;
    uint8_t v128[16];
    const char* func;
  };
};

BINARYEN_API struct BinaryenLiteral BinaryenLiteralInt32(int32_t x);
BINARYEN_API struct BinaryenLiteral BinaryenLiteralInt64(int64_t x);
BINARYEN_API struct BinaryenLiteral BinaryenLiteralFloat32(float x);
BINARYEN_API struct BinaryenLiteral BinaryenLiteralFloat64(double x);
BINARYEN_API struct BinaryenLiteral BinaryenLiteralVec128(const uint8_t x[16]);
BINARYEN_API struct BinaryenLiteral BinaryenLiteralFloat32Bits(int32_t x);
BINARYEN_API struct BinaryenLiteral BinaryenLiteralFloat64Bits(int64_t x);

// Expressions
//
// Some expressions have a BinaryenOp, which is the more
// specific operation/opcode.
//
// Some expressions have optional parameters, like Return may not
// return a value. You can supply a NULL pointer in those cases.
//
// For more information, see wasm.h

typedef int32_t BinaryenOp;

BINARYEN_API BinaryenOp BinaryenClzInt32(void);
BINARYEN_API BinaryenOp BinaryenCtzInt32(void);
BINARYEN_API BinaryenOp BinaryenPopcntInt32(void);
BINARYEN_API BinaryenOp BinaryenNegFloat32(void);
BINARYEN_API BinaryenOp BinaryenAbsFloat32(void);
BINARYEN_API BinaryenOp BinaryenCeilFloat32(void);
BINARYEN_API BinaryenOp BinaryenFloorFloat32(void);
BINARYEN_API BinaryenOp BinaryenTruncFloat32(void);
BINARYEN_API BinaryenOp BinaryenNearestFloat32(void);
BINARYEN_API BinaryenOp BinaryenSqrtFloat32(void);
BINARYEN_API BinaryenOp BinaryenEqZInt32(void);
BINARYEN_API BinaryenOp BinaryenClzInt64(void);
BINARYEN_API BinaryenOp BinaryenCtzInt64(void);
BINARYEN_API BinaryenOp BinaryenPopcntInt64(void);
BINARYEN_API BinaryenOp BinaryenNegFloat64(void);
BINARYEN_API BinaryenOp BinaryenAbsFloat64(void);
BINARYEN_API BinaryenOp BinaryenCeilFloat64(void);
BINARYEN_API BinaryenOp BinaryenFloorFloat64(void);
BINARYEN_API BinaryenOp BinaryenTruncFloat64(void);
BINARYEN_API BinaryenOp BinaryenNearestFloat64(void);
BINARYEN_API BinaryenOp BinaryenSqrtFloat64(void);
BINARYEN_API BinaryenOp BinaryenEqZInt64(void);
BINARYEN_API BinaryenOp BinaryenExtendSInt32(void);
BINARYEN_API BinaryenOp BinaryenExtendUInt32(void);
BINARYEN_API BinaryenOp BinaryenWrapInt64(void);
BINARYEN_API BinaryenOp BinaryenTruncSFloat32ToInt32(void);
BINARYEN_API BinaryenOp BinaryenTruncSFloat32ToInt64(void);
BINARYEN_API BinaryenOp BinaryenTruncUFloat32ToInt32(void);
BINARYEN_API BinaryenOp BinaryenTruncUFloat32ToInt64(void);
BINARYEN_API BinaryenOp BinaryenTruncSFloat64ToInt32(void);
BINARYEN_API BinaryenOp BinaryenTruncSFloat64ToInt64(void);
BINARYEN_API BinaryenOp BinaryenTruncUFloat64ToInt32(void);
BINARYEN_API BinaryenOp BinaryenTruncUFloat64ToInt64(void);
BINARYEN_API BinaryenOp BinaryenReinterpretFloat32(void);
BINARYEN_API BinaryenOp BinaryenReinterpretFloat64(void);
BINARYEN_API BinaryenOp BinaryenConvertSInt32ToFloat32(void);
BINARYEN_API BinaryenOp BinaryenConvertSInt32ToFloat64(void);
BINARYEN_API BinaryenOp BinaryenConvertUInt32ToFloat32(void);
BINARYEN_API BinaryenOp BinaryenConvertUInt32ToFloat64(void);
BINARYEN_API BinaryenOp BinaryenConvertSInt64ToFloat32(void);
BINARYEN_API BinaryenOp BinaryenConvertSInt64ToFloat64(void);
BINARYEN_API BinaryenOp BinaryenConvertUInt64ToFloat32(void);
BINARYEN_API BinaryenOp BinaryenConvertUInt64ToFloat64(void);
BINARYEN_API BinaryenOp BinaryenPromoteFloat32(void);
BINARYEN_API BinaryenOp BinaryenDemoteFloat64(void);
BINARYEN_API BinaryenOp BinaryenReinterpretInt32(void);
BINARYEN_API BinaryenOp BinaryenReinterpretInt64(void);
BINARYEN_API BinaryenOp BinaryenExtendS8Int32(void);
BINARYEN_API BinaryenOp BinaryenExtendS16Int32(void);
BINARYEN_API BinaryenOp BinaryenExtendS8Int64(void);
BINARYEN_API BinaryenOp BinaryenExtendS16Int64(void);
BINARYEN_API BinaryenOp BinaryenExtendS32Int64(void);
BINARYEN_API BinaryenOp BinaryenAddInt32(void);
BINARYEN_API BinaryenOp BinaryenSubInt32(void);
BINARYEN_API BinaryenOp BinaryenMulInt32(void);
BINARYEN_API BinaryenOp BinaryenDivSInt32(void);
BINARYEN_API BinaryenOp BinaryenDivUInt32(void);
BINARYEN_API BinaryenOp BinaryenRemSInt32(void);
BINARYEN_API BinaryenOp BinaryenRemUInt32(void);
BINARYEN_API BinaryenOp BinaryenAndInt32(void);
BINARYEN_API BinaryenOp BinaryenOrInt32(void);
BINARYEN_API BinaryenOp BinaryenXorInt32(void);
BINARYEN_API BinaryenOp BinaryenShlInt32(void);
BINARYEN_API BinaryenOp BinaryenShrUInt32(void);
BINARYEN_API BinaryenOp BinaryenShrSInt32(void);
BINARYEN_API BinaryenOp BinaryenRotLInt32(void);
BINARYEN_API BinaryenOp BinaryenRotRInt32(void);
BINARYEN_API BinaryenOp BinaryenEqInt32(void);
BINARYEN_API BinaryenOp BinaryenNeInt32(void);
BINARYEN_API BinaryenOp BinaryenLtSInt32(void);
BINARYEN_API BinaryenOp BinaryenLtUInt32(void);
BINARYEN_API BinaryenOp BinaryenLeSInt32(void);
BINARYEN_API BinaryenOp BinaryenLeUInt32(void);
BINARYEN_API BinaryenOp BinaryenGtSInt32(void);
BINARYEN_API BinaryenOp BinaryenGtUInt32(void);
BINARYEN_API BinaryenOp BinaryenGeSInt32(void);
BINARYEN_API BinaryenOp BinaryenGeUInt32(void);
BINARYEN_API BinaryenOp BinaryenAddInt64(void);
BINARYEN_API BinaryenOp BinaryenSubInt64(void);
BINARYEN_API BinaryenOp BinaryenMulInt64(void);
BINARYEN_API BinaryenOp BinaryenDivSInt64(void);
BINARYEN_API BinaryenOp BinaryenDivUInt64(void);
BINARYEN_API BinaryenOp BinaryenRemSInt64(void);
BINARYEN_API BinaryenOp BinaryenRemUInt64(void);
BINARYEN_API BinaryenOp BinaryenAndInt64(void);
BINARYEN_API BinaryenOp BinaryenOrInt64(void);
BINARYEN_API BinaryenOp BinaryenXorInt64(void);
BINARYEN_API BinaryenOp BinaryenShlInt64(void);
BINARYEN_API BinaryenOp BinaryenShrUInt64(void);
BINARYEN_API BinaryenOp BinaryenShrSInt64(void);
BINARYEN_API BinaryenOp BinaryenRotLInt64(void);
BINARYEN_API BinaryenOp BinaryenRotRInt64(void);
BINARYEN_API BinaryenOp BinaryenEqInt64(void);
BINARYEN_API BinaryenOp BinaryenNeInt64(void);
BINARYEN_API BinaryenOp BinaryenLtSInt64(void);
BINARYEN_API BinaryenOp BinaryenLtUInt64(void);
BINARYEN_API BinaryenOp BinaryenLeSInt64(void);
BINARYEN_API BinaryenOp BinaryenLeUInt64(void);
BINARYEN_API BinaryenOp BinaryenGtSInt64(void);
BINARYEN_API BinaryenOp BinaryenGtUInt64(void);
BINARYEN_API BinaryenOp BinaryenGeSInt64(void);
BINARYEN_API BinaryenOp BinaryenGeUInt64(void);
BINARYEN_API BinaryenOp BinaryenAddFloat32(void);
BINARYEN_API BinaryenOp BinaryenSubFloat32(void);
BINARYEN_API BinaryenOp BinaryenMulFloat32(void);
BINARYEN_API BinaryenOp BinaryenDivFloat32(void);
BINARYEN_API BinaryenOp BinaryenCopySignFloat32(void);
BINARYEN_API BinaryenOp BinaryenMinFloat32(void);
BINARYEN_API BinaryenOp BinaryenMaxFloat32(void);
BINARYEN_API BinaryenOp BinaryenEqFloat32(void);
BINARYEN_API BinaryenOp BinaryenNeFloat32(void);
BINARYEN_API BinaryenOp BinaryenLtFloat32(void);
BINARYEN_API BinaryenOp BinaryenLeFloat32(void);
BINARYEN_API BinaryenOp BinaryenGtFloat32(void);
BINARYEN_API BinaryenOp BinaryenGeFloat32(void);
BINARYEN_API BinaryenOp BinaryenAddFloat64(void);
BINARYEN_API BinaryenOp BinaryenSubFloat64(void);
BINARYEN_API BinaryenOp BinaryenMulFloat64(void);
BINARYEN_API BinaryenOp BinaryenDivFloat64(void);
BINARYEN_API BinaryenOp BinaryenCopySignFloat64(void);
BINARYEN_API BinaryenOp BinaryenMinFloat64(void);
BINARYEN_API BinaryenOp BinaryenMaxFloat64(void);
BINARYEN_API BinaryenOp BinaryenEqFloat64(void);
BINARYEN_API BinaryenOp BinaryenNeFloat64(void);
BINARYEN_API BinaryenOp BinaryenLtFloat64(void);
BINARYEN_API BinaryenOp BinaryenLeFloat64(void);
BINARYEN_API BinaryenOp BinaryenGtFloat64(void);
BINARYEN_API BinaryenOp BinaryenGeFloat64(void);
BINARYEN_API BinaryenOp BinaryenAtomicRMWAdd(void);
BINARYEN_API BinaryenOp BinaryenAtomicRMWSub(void);
BINARYEN_API BinaryenOp BinaryenAtomicRMWAnd(void);
BINARYEN_API BinaryenOp BinaryenAtomicRMWOr(void);
BINARYEN_API BinaryenOp BinaryenAtomicRMWXor(void);
BINARYEN_API BinaryenOp BinaryenAtomicRMWXchg(void);
BINARYEN_API BinaryenOp BinaryenTruncSatSFloat32ToInt32(void);
BINARYEN_API BinaryenOp BinaryenTruncSatSFloat32ToInt64(void);
BINARYEN_API BinaryenOp BinaryenTruncSatUFloat32ToInt32(void);
BINARYEN_API BinaryenOp BinaryenTruncSatUFloat32ToInt64(void);
BINARYEN_API BinaryenOp BinaryenTruncSatSFloat64ToInt32(void);
BINARYEN_API BinaryenOp BinaryenTruncSatSFloat64ToInt64(void);
BINARYEN_API BinaryenOp BinaryenTruncSatUFloat64ToInt32(void);
BINARYEN_API BinaryenOp BinaryenTruncSatUFloat64ToInt64(void);
BINARYEN_API BinaryenOp BinaryenSplatVecI8x16(void);
BINARYEN_API BinaryenOp BinaryenExtractLaneSVecI8x16(void);
BINARYEN_API BinaryenOp BinaryenExtractLaneUVecI8x16(void);
BINARYEN_API BinaryenOp BinaryenReplaceLaneVecI8x16(void);
BINARYEN_API BinaryenOp BinaryenSplatVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenExtractLaneSVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenExtractLaneUVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenReplaceLaneVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenSplatVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenExtractLaneVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenReplaceLaneVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenSplatVecI64x2(void);
BINARYEN_API BinaryenOp BinaryenExtractLaneVecI64x2(void);
BINARYEN_API BinaryenOp BinaryenReplaceLaneVecI64x2(void);
BINARYEN_API BinaryenOp BinaryenSplatVecF32x4(void);
BINARYEN_API BinaryenOp BinaryenExtractLaneVecF32x4(void);
BINARYEN_API BinaryenOp BinaryenReplaceLaneVecF32x4(void);
BINARYEN_API BinaryenOp BinaryenSplatVecF64x2(void);
BINARYEN_API BinaryenOp BinaryenExtractLaneVecF64x2(void);
BINARYEN_API BinaryenOp BinaryenReplaceLaneVecF64x2(void);
BINARYEN_API BinaryenOp BinaryenEqVecI8x16(void);
BINARYEN_API BinaryenOp BinaryenNeVecI8x16(void);
BINARYEN_API BinaryenOp BinaryenLtSVecI8x16(void);
BINARYEN_API BinaryenOp BinaryenLtUVecI8x16(void);
BINARYEN_API BinaryenOp BinaryenGtSVecI8x16(void);
BINARYEN_API BinaryenOp BinaryenGtUVecI8x16(void);
BINARYEN_API BinaryenOp BinaryenLeSVecI8x16(void);
BINARYEN_API BinaryenOp BinaryenLeUVecI8x16(void);
BINARYEN_API BinaryenOp BinaryenGeSVecI8x16(void);
BINARYEN_API BinaryenOp BinaryenGeUVecI8x16(void);
BINARYEN_API BinaryenOp BinaryenEqVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenNeVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenLtSVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenLtUVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenGtSVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenGtUVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenLeSVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenLeUVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenGeSVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenGeUVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenEqVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenNeVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenLtSVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenLtUVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenGtSVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenGtUVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenLeSVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenLeUVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenGeSVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenGeUVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenEqVecI64x2(void);
BINARYEN_API BinaryenOp BinaryenNeVecI64x2(void);
BINARYEN_API BinaryenOp BinaryenLtSVecI64x2(void);
BINARYEN_API BinaryenOp BinaryenGtSVecI64x2(void);
BINARYEN_API BinaryenOp BinaryenLeSVecI64x2(void);
BINARYEN_API BinaryenOp BinaryenGeSVecI64x2(void);
BINARYEN_API BinaryenOp BinaryenEqVecF32x4(void);
BINARYEN_API BinaryenOp BinaryenNeVecF32x4(void);
BINARYEN_API BinaryenOp BinaryenLtVecF32x4(void);
BINARYEN_API BinaryenOp BinaryenGtVecF32x4(void);
BINARYEN_API BinaryenOp BinaryenLeVecF32x4(void);
BINARYEN_API BinaryenOp BinaryenGeVecF32x4(void);
BINARYEN_API BinaryenOp BinaryenEqVecF64x2(void);
BINARYEN_API BinaryenOp BinaryenNeVecF64x2(void);
BINARYEN_API BinaryenOp BinaryenLtVecF64x2(void);
BINARYEN_API BinaryenOp BinaryenGtVecF64x2(void);
BINARYEN_API BinaryenOp BinaryenLeVecF64x2(void);
BINARYEN_API BinaryenOp BinaryenGeVecF64x2(void);
BINARYEN_API BinaryenOp BinaryenNotVec128(void);
BINARYEN_API BinaryenOp BinaryenAndVec128(void);
BINARYEN_API BinaryenOp BinaryenOrVec128(void);
BINARYEN_API BinaryenOp BinaryenXorVec128(void);
BINARYEN_API BinaryenOp BinaryenAndNotVec128(void);
BINARYEN_API BinaryenOp BinaryenBitselectVec128(void);
BINARYEN_API BinaryenOp BinaryenRelaxedFmaVecF32x4(void);
BINARYEN_API BinaryenOp BinaryenRelaxedFmsVecF32x4(void);
BINARYEN_API BinaryenOp BinaryenRelaxedFmaVecF64x2(void);
BINARYEN_API BinaryenOp BinaryenRelaxedFmsVecF64x2(void);
BINARYEN_API BinaryenOp BinaryenLaneselectI8x16(void);
BINARYEN_API BinaryenOp BinaryenLaneselectI16x8(void);
BINARYEN_API BinaryenOp BinaryenLaneselectI32x4(void);
BINARYEN_API BinaryenOp BinaryenLaneselectI64x2(void);
BINARYEN_API BinaryenOp BinaryenDotI8x16I7x16AddSToVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenAnyTrueVec128(void);
BINARYEN_API BinaryenOp BinaryenPopcntVecI8x16(void);
BINARYEN_API BinaryenOp BinaryenAbsVecI8x16(void);
BINARYEN_API BinaryenOp BinaryenNegVecI8x16(void);
BINARYEN_API BinaryenOp BinaryenAllTrueVecI8x16(void);
BINARYEN_API BinaryenOp BinaryenBitmaskVecI8x16(void);
BINARYEN_API BinaryenOp BinaryenShlVecI8x16(void);
BINARYEN_API BinaryenOp BinaryenShrSVecI8x16(void);
BINARYEN_API BinaryenOp BinaryenShrUVecI8x16(void);
BINARYEN_API BinaryenOp BinaryenAddVecI8x16(void);
BINARYEN_API BinaryenOp BinaryenAddSatSVecI8x16(void);
BINARYEN_API BinaryenOp BinaryenAddSatUVecI8x16(void);
BINARYEN_API BinaryenOp BinaryenSubVecI8x16(void);
BINARYEN_API BinaryenOp BinaryenSubSatSVecI8x16(void);
BINARYEN_API BinaryenOp BinaryenSubSatUVecI8x16(void);
BINARYEN_API BinaryenOp BinaryenMinSVecI8x16(void);
BINARYEN_API BinaryenOp BinaryenMinUVecI8x16(void);
BINARYEN_API BinaryenOp BinaryenMaxSVecI8x16(void);
BINARYEN_API BinaryenOp BinaryenMaxUVecI8x16(void);
BINARYEN_API BinaryenOp BinaryenAvgrUVecI8x16(void);
BINARYEN_API BinaryenOp BinaryenAbsVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenNegVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenAllTrueVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenBitmaskVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenShlVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenShrSVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenShrUVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenAddVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenAddSatSVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenAddSatUVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenSubVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenSubSatSVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenSubSatUVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenMulVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenMinSVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenMinUVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenMaxSVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenMaxUVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenAvgrUVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenQ15MulrSatSVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenExtMulLowSVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenExtMulHighSVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenExtMulLowUVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenExtMulHighUVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenAbsVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenNegVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenAllTrueVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenBitmaskVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenShlVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenShrSVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenShrUVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenAddVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenSubVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenMulVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenMinSVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenMinUVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenMaxSVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenMaxUVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenDotSVecI16x8ToVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenExtMulLowSVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenExtMulHighSVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenExtMulLowUVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenExtMulHighUVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenAbsVecI64x2(void);
BINARYEN_API BinaryenOp BinaryenNegVecI64x2(void);
BINARYEN_API BinaryenOp BinaryenAllTrueVecI64x2(void);
BINARYEN_API BinaryenOp BinaryenBitmaskVecI64x2(void);
BINARYEN_API BinaryenOp BinaryenShlVecI64x2(void);
BINARYEN_API BinaryenOp BinaryenShrSVecI64x2(void);
BINARYEN_API BinaryenOp BinaryenShrUVecI64x2(void);
BINARYEN_API BinaryenOp BinaryenAddVecI64x2(void);
BINARYEN_API BinaryenOp BinaryenSubVecI64x2(void);
BINARYEN_API BinaryenOp BinaryenMulVecI64x2(void);
BINARYEN_API BinaryenOp BinaryenExtMulLowSVecI64x2(void);
BINARYEN_API BinaryenOp BinaryenExtMulHighSVecI64x2(void);
BINARYEN_API BinaryenOp BinaryenExtMulLowUVecI64x2(void);
BINARYEN_API BinaryenOp BinaryenExtMulHighUVecI64x2(void);
BINARYEN_API BinaryenOp BinaryenAbsVecF32x4(void);
BINARYEN_API BinaryenOp BinaryenNegVecF32x4(void);
BINARYEN_API BinaryenOp BinaryenSqrtVecF32x4(void);
BINARYEN_API BinaryenOp BinaryenAddVecF32x4(void);
BINARYEN_API BinaryenOp BinaryenSubVecF32x4(void);
BINARYEN_API BinaryenOp BinaryenMulVecF32x4(void);
BINARYEN_API BinaryenOp BinaryenDivVecF32x4(void);
BINARYEN_API BinaryenOp BinaryenMinVecF32x4(void);
BINARYEN_API BinaryenOp BinaryenMaxVecF32x4(void);
BINARYEN_API BinaryenOp BinaryenPMinVecF32x4(void);
BINARYEN_API BinaryenOp BinaryenPMaxVecF32x4(void);
BINARYEN_API BinaryenOp BinaryenCeilVecF32x4(void);
BINARYEN_API BinaryenOp BinaryenFloorVecF32x4(void);
BINARYEN_API BinaryenOp BinaryenTruncVecF32x4(void);
BINARYEN_API BinaryenOp BinaryenNearestVecF32x4(void);
BINARYEN_API BinaryenOp BinaryenAbsVecF64x2(void);
BINARYEN_API BinaryenOp BinaryenNegVecF64x2(void);
BINARYEN_API BinaryenOp BinaryenSqrtVecF64x2(void);
BINARYEN_API BinaryenOp BinaryenAddVecF64x2(void);
BINARYEN_API BinaryenOp BinaryenSubVecF64x2(void);
BINARYEN_API BinaryenOp BinaryenMulVecF64x2(void);
BINARYEN_API BinaryenOp BinaryenDivVecF64x2(void);
BINARYEN_API BinaryenOp BinaryenMinVecF64x2(void);
BINARYEN_API BinaryenOp BinaryenMaxVecF64x2(void);
BINARYEN_API BinaryenOp BinaryenPMinVecF64x2(void);
BINARYEN_API BinaryenOp BinaryenPMaxVecF64x2(void);
BINARYEN_API BinaryenOp BinaryenCeilVecF64x2(void);
BINARYEN_API BinaryenOp BinaryenFloorVecF64x2(void);
BINARYEN_API BinaryenOp BinaryenTruncVecF64x2(void);
BINARYEN_API BinaryenOp BinaryenNearestVecF64x2(void);
BINARYEN_API BinaryenOp BinaryenExtAddPairwiseSVecI8x16ToI16x8(void);
BINARYEN_API BinaryenOp BinaryenExtAddPairwiseUVecI8x16ToI16x8(void);
BINARYEN_API BinaryenOp BinaryenExtAddPairwiseSVecI16x8ToI32x4(void);
BINARYEN_API BinaryenOp BinaryenExtAddPairwiseUVecI16x8ToI32x4(void);
BINARYEN_API BinaryenOp BinaryenTruncSatSVecF32x4ToVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenTruncSatUVecF32x4ToVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenConvertSVecI32x4ToVecF32x4(void);
BINARYEN_API BinaryenOp BinaryenConvertUVecI32x4ToVecF32x4(void);
BINARYEN_API BinaryenOp BinaryenLoad8SplatVec128(void);
BINARYEN_API BinaryenOp BinaryenLoad16SplatVec128(void);
BINARYEN_API BinaryenOp BinaryenLoad32SplatVec128(void);
BINARYEN_API BinaryenOp BinaryenLoad64SplatVec128(void);
BINARYEN_API BinaryenOp BinaryenLoad8x8SVec128(void);
BINARYEN_API BinaryenOp BinaryenLoad8x8UVec128(void);
BINARYEN_API BinaryenOp BinaryenLoad16x4SVec128(void);
BINARYEN_API BinaryenOp BinaryenLoad16x4UVec128(void);
BINARYEN_API BinaryenOp BinaryenLoad32x2SVec128(void);
BINARYEN_API BinaryenOp BinaryenLoad32x2UVec128(void);
BINARYEN_API BinaryenOp BinaryenLoad32ZeroVec128(void);
BINARYEN_API BinaryenOp BinaryenLoad64ZeroVec128(void);
BINARYEN_API BinaryenOp BinaryenLoad8LaneVec128(void);
BINARYEN_API BinaryenOp BinaryenLoad16LaneVec128(void);
BINARYEN_API BinaryenOp BinaryenLoad32LaneVec128(void);
BINARYEN_API BinaryenOp BinaryenLoad64LaneVec128(void);
BINARYEN_API BinaryenOp BinaryenStore8LaneVec128(void);
BINARYEN_API BinaryenOp BinaryenStore16LaneVec128(void);
BINARYEN_API BinaryenOp BinaryenStore32LaneVec128(void);
BINARYEN_API BinaryenOp BinaryenStore64LaneVec128(void);
BINARYEN_API BinaryenOp BinaryenNarrowSVecI16x8ToVecI8x16(void);
BINARYEN_API BinaryenOp BinaryenNarrowUVecI16x8ToVecI8x16(void);
BINARYEN_API BinaryenOp BinaryenNarrowSVecI32x4ToVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenNarrowUVecI32x4ToVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenExtendLowSVecI8x16ToVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenExtendHighSVecI8x16ToVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenExtendLowUVecI8x16ToVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenExtendHighUVecI8x16ToVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenExtendLowSVecI16x8ToVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenExtendHighSVecI16x8ToVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenExtendLowUVecI16x8ToVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenExtendHighUVecI16x8ToVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenExtendLowSVecI32x4ToVecI64x2(void);
BINARYEN_API BinaryenOp BinaryenExtendHighSVecI32x4ToVecI64x2(void);
BINARYEN_API BinaryenOp BinaryenExtendLowUVecI32x4ToVecI64x2(void);
BINARYEN_API BinaryenOp BinaryenExtendHighUVecI32x4ToVecI64x2(void);
BINARYEN_API BinaryenOp BinaryenConvertLowSVecI32x4ToVecF64x2(void);
BINARYEN_API BinaryenOp BinaryenConvertLowUVecI32x4ToVecF64x2(void);
BINARYEN_API BinaryenOp BinaryenTruncSatZeroSVecF64x2ToVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenTruncSatZeroUVecF64x2ToVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenDemoteZeroVecF64x2ToVecF32x4(void);
BINARYEN_API BinaryenOp BinaryenPromoteLowVecF32x4ToVecF64x2(void);
BINARYEN_API BinaryenOp BinaryenRelaxedTruncSVecF32x4ToVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenRelaxedTruncUVecF32x4ToVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenRelaxedTruncZeroSVecF64x2ToVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenRelaxedTruncZeroUVecF64x2ToVecI32x4(void);
BINARYEN_API BinaryenOp BinaryenSwizzleVecI8x16(void);
BINARYEN_API BinaryenOp BinaryenRelaxedSwizzleVecI8x16(void);
BINARYEN_API BinaryenOp BinaryenRelaxedMinVecF32x4(void);
BINARYEN_API BinaryenOp BinaryenRelaxedMaxVecF32x4(void);
BINARYEN_API BinaryenOp BinaryenRelaxedMinVecF64x2(void);
BINARYEN_API BinaryenOp BinaryenRelaxedMaxVecF64x2(void);
BINARYEN_API BinaryenOp BinaryenRelaxedQ15MulrSVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenDotI8x16I7x16SToVecI16x8(void);
BINARYEN_API BinaryenOp BinaryenRefAsNonNull(void);
BINARYEN_API BinaryenOp BinaryenRefAsExternInternalize(void);
BINARYEN_API BinaryenOp BinaryenRefAsExternExternalize(void);
BINARYEN_API BinaryenOp BinaryenBrOnNull(void);
BINARYEN_API BinaryenOp BinaryenBrOnNonNull(void);
BINARYEN_API BinaryenOp BinaryenBrOnCast(void);
BINARYEN_API BinaryenOp BinaryenBrOnCastFail(void);
BINARYEN_API BinaryenOp BinaryenStringNewUTF8(void);
BINARYEN_API BinaryenOp BinaryenStringNewWTF8(void);
BINARYEN_API BinaryenOp BinaryenStringNewReplace(void);
BINARYEN_API BinaryenOp BinaryenStringNewWTF16(void);
BINARYEN_API BinaryenOp BinaryenStringNewUTF8Array(void);
BINARYEN_API BinaryenOp BinaryenStringNewWTF8Array(void);
BINARYEN_API BinaryenOp BinaryenStringNewReplaceArray(void);
BINARYEN_API BinaryenOp BinaryenStringNewWTF16Array(void);
BINARYEN_API BinaryenOp BinaryenStringNewFromCodePoint(void);
BINARYEN_API BinaryenOp BinaryenStringMeasureUTF8(void);
BINARYEN_API BinaryenOp BinaryenStringMeasureWTF8(void);
BINARYEN_API BinaryenOp BinaryenStringMeasureWTF16(void);
BINARYEN_API BinaryenOp BinaryenStringMeasureIsUSV(void);
BINARYEN_API BinaryenOp BinaryenStringMeasureWTF16View(void);
BINARYEN_API BinaryenOp BinaryenStringEncodeUTF8(void);
BINARYEN_API BinaryenOp BinaryenStringEncodeWTF8(void);
BINARYEN_API BinaryenOp BinaryenStringEncodeWTF16(void);
BINARYEN_API BinaryenOp BinaryenStringEncodeUTF8Array(void);
BINARYEN_API BinaryenOp BinaryenStringEncodeWTF8Array(void);
BINARYEN_API BinaryenOp BinaryenStringEncodeWTF16Array(void);
BINARYEN_API BinaryenOp BinaryenStringAsWTF8(void);
BINARYEN_API BinaryenOp BinaryenStringAsWTF16(void);
BINARYEN_API BinaryenOp BinaryenStringAsIter(void);
BINARYEN_API BinaryenOp BinaryenStringIterMoveAdvance(void);
BINARYEN_API BinaryenOp BinaryenStringIterMoveRewind(void);
BINARYEN_API BinaryenOp BinaryenStringSliceWTF8(void);
BINARYEN_API BinaryenOp BinaryenStringSliceWTF16(void);
BINARYEN_API BinaryenOp BinaryenStringEqEqual(void);
BINARYEN_API BinaryenOp BinaryenStringEqCompare(void);

BINARYEN_REF(Expression);

// Block: name can be NULL. Specifying BinaryenUndefined() as the 'type'
//        parameter indicates that the block's type shall be figured out
//        automatically instead of explicitly providing it. This conforms
//        to the behavior before the 'type' parameter has been introduced.
BINARYEN_API BinaryenExpressionRef
BinaryenBlock(BinaryenModuleRef module,
              const char* name,
              BinaryenExpressionRef* children,
              BinaryenIndex numChildren,
              BinaryenType type);
// If: ifFalse can be NULL
BINARYEN_API BinaryenExpressionRef BinaryenIf(BinaryenModuleRef module,
                                              BinaryenExpressionRef condition,
                                              BinaryenExpressionRef ifTrue,
                                              BinaryenExpressionRef ifFalse);
BINARYEN_API BinaryenExpressionRef BinaryenLoop(BinaryenModuleRef module,
                                                const char* in,
                                                BinaryenExpressionRef body);
// Break: value and condition can be NULL
BINARYEN_API BinaryenExpressionRef
BinaryenBreak(BinaryenModuleRef module,
              const char* name,
              BinaryenExpressionRef condition,
              BinaryenExpressionRef value);
// Switch: value can be NULL
BINARYEN_API BinaryenExpressionRef
BinaryenSwitch(BinaryenModuleRef module,
               const char** names,
               BinaryenIndex numNames,
               const char* defaultName,
               BinaryenExpressionRef condition,
               BinaryenExpressionRef value);
// Call: Note the 'returnType' parameter. You must declare the
//       type returned by the function being called, as that
//       function might not have been created yet, so we don't
//       know what it is.
BINARYEN_API BinaryenExpressionRef BinaryenCall(BinaryenModuleRef module,
                                                const char* target,
                                                BinaryenExpressionRef* operands,
                                                BinaryenIndex numOperands,
                                                BinaryenType returnType);
BINARYEN_API BinaryenExpressionRef
BinaryenCallIndirect(BinaryenModuleRef module,
                     const char* table,
                     BinaryenExpressionRef target,
                     BinaryenExpressionRef* operands,
                     BinaryenIndex numOperands,
                     BinaryenType params,
                     BinaryenType results);
BINARYEN_API BinaryenExpressionRef
BinaryenReturnCall(BinaryenModuleRef module,
                   const char* target,
                   BinaryenExpressionRef* operands,
                   BinaryenIndex numOperands,
                   BinaryenType returnType);
BINARYEN_API BinaryenExpressionRef
BinaryenReturnCallIndirect(BinaryenModuleRef module,
                           const char* table,
                           BinaryenExpressionRef target,
                           BinaryenExpressionRef* operands,
                           BinaryenIndex numOperands,
                           BinaryenType params,
                           BinaryenType results);

// LocalGet: Note the 'type' parameter. It might seem redundant, since the
//           local at that index must have a type. However, this API lets you
//           build code "top-down": create a node, then its parents, and so
//           on, and finally create the function at the end. (Note that in fact
//           you do not mention a function when creating ExpressionRefs, only
//           a module.) And since LocalGet is a leaf node, we need to be told
//           its type. (Other nodes detect their type either from their
//           type or their opcode, or failing that, their children. But
//           LocalGet has no children, it is where a "stream" of type info
//           begins.)
//           Note also that the index of a local can refer to a param or
//           a var, that is, either a parameter to the function or a variable
//           declared when you call BinaryenAddFunction. See BinaryenAddFunction
//           for more details.
BINARYEN_API BinaryenExpressionRef BinaryenLocalGet(BinaryenModuleRef module,
                                                    BinaryenIndex index,
                                                    BinaryenType type);
BINARYEN_API BinaryenExpressionRef BinaryenLocalSet(
  BinaryenModuleRef module, BinaryenIndex index, BinaryenExpressionRef value);
BINARYEN_API BinaryenExpressionRef BinaryenLocalTee(BinaryenModuleRef module,
                                                    BinaryenIndex index,
                                                    BinaryenExpressionRef value,
                                                    BinaryenType type);
BINARYEN_API BinaryenExpressionRef BinaryenGlobalGet(BinaryenModuleRef module,
                                                     const char* name,
                                                     BinaryenType type);
BINARYEN_API BinaryenExpressionRef BinaryenGlobalSet(
  BinaryenModuleRef module, const char* name, BinaryenExpressionRef value);
// Load: align can be 0, in which case it will be the natural alignment (equal
// to bytes)
BINARYEN_API BinaryenExpressionRef BinaryenLoad(BinaryenModuleRef module,
                                                uint32_t bytes,
                                                bool signed_,
                                                uint32_t offset,
                                                uint32_t align,
                                                BinaryenType type,
                                                BinaryenExpressionRef ptr,
                                                const char* memoryName);
// Store: align can be 0, in which case it will be the natural alignment (equal
// to bytes)
BINARYEN_API BinaryenExpressionRef BinaryenStore(BinaryenModuleRef module,
                                                 uint32_t bytes,
                                                 uint32_t offset,
                                                 uint32_t align,
                                                 BinaryenExpressionRef ptr,
                                                 BinaryenExpressionRef value,
                                                 BinaryenType type,
                                                 const char* memoryName);
BINARYEN_API BinaryenExpressionRef BinaryenConst(BinaryenModuleRef module,
                                                 struct BinaryenLiteral value);
BINARYEN_API BinaryenExpressionRef BinaryenUnary(BinaryenModuleRef module,
                                                 BinaryenOp op,
                                                 BinaryenExpressionRef value);
BINARYEN_API BinaryenExpressionRef BinaryenBinary(BinaryenModuleRef module,
                                                  BinaryenOp op,
                                                  BinaryenExpressionRef left,
                                                  BinaryenExpressionRef right);
BINARYEN_API BinaryenExpressionRef
BinaryenSelect(BinaryenModuleRef module,
               BinaryenExpressionRef condition,
               BinaryenExpressionRef ifTrue,
               BinaryenExpressionRef ifFalse,
               BinaryenType type);
BINARYEN_API BinaryenExpressionRef BinaryenDrop(BinaryenModuleRef module,
                                                BinaryenExpressionRef value);
// Return: value can be NULL
BINARYEN_API BinaryenExpressionRef BinaryenReturn(BinaryenModuleRef module,
                                                  BinaryenExpressionRef value);
BINARYEN_API BinaryenExpressionRef BinaryenMemorySize(BinaryenModuleRef module,
                                                      const char* memoryName,
                                                      bool memoryIs64);
BINARYEN_API BinaryenExpressionRef
BinaryenMemoryGrow(BinaryenModuleRef module,
                   BinaryenExpressionRef delta,
                   const char* memoryName,
                   bool memoryIs64);
BINARYEN_API BinaryenExpressionRef BinaryenNop(BinaryenModuleRef module);
BINARYEN_API BinaryenExpressionRef
BinaryenUnreachable(BinaryenModuleRef module);
BINARYEN_API BinaryenExpressionRef BinaryenAtomicLoad(BinaryenModuleRef module,
                                                      uint32_t bytes,
                                                      uint32_t offset,
                                                      BinaryenType type,
                                                      BinaryenExpressionRef ptr,
                                                      const char* memoryName);
BINARYEN_API BinaryenExpressionRef
BinaryenAtomicStore(BinaryenModuleRef module,
                    uint32_t bytes,
                    uint32_t offset,
                    BinaryenExpressionRef ptr,
                    BinaryenExpressionRef value,
                    BinaryenType type,
                    const char* memoryName);
BINARYEN_API BinaryenExpressionRef
BinaryenAtomicRMW(BinaryenModuleRef module,
                  BinaryenOp op,
                  BinaryenIndex bytes,
                  BinaryenIndex offset,
                  BinaryenExpressionRef ptr,
                  BinaryenExpressionRef value,
                  BinaryenType type,
                  const char* memoryName);
BINARYEN_API BinaryenExpressionRef
BinaryenAtomicCmpxchg(BinaryenModuleRef module,
                      BinaryenIndex bytes,
                      BinaryenIndex offset,
                      BinaryenExpressionRef ptr,
                      BinaryenExpressionRef expected,
                      BinaryenExpressionRef replacement,
                      BinaryenType type,
                      const char* memoryName);
BINARYEN_API BinaryenExpressionRef
BinaryenAtomicWait(BinaryenModuleRef module,
                   BinaryenExpressionRef ptr,
                   BinaryenExpressionRef expected,
                   BinaryenExpressionRef timeout,
                   BinaryenType type,
                   const char* memoryName);
BINARYEN_API BinaryenExpressionRef
BinaryenAtomicNotify(BinaryenModuleRef module,
                     BinaryenExpressionRef ptr,
                     BinaryenExpressionRef notifyCount,
                     const char* memoryName);
BINARYEN_API BinaryenExpressionRef
BinaryenAtomicFence(BinaryenModuleRef module);
BINARYEN_API BinaryenExpressionRef
BinaryenSIMDExtract(BinaryenModuleRef module,
                    BinaryenOp op,
                    BinaryenExpressionRef vec,
                    uint8_t index);
BINARYEN_API BinaryenExpressionRef
BinaryenSIMDReplace(BinaryenModuleRef module,
                    BinaryenOp op,
                    BinaryenExpressionRef vec,
                    uint8_t index,
                    BinaryenExpressionRef value);
BINARYEN_API BinaryenExpressionRef
BinaryenSIMDShuffle(BinaryenModuleRef module,
                    BinaryenExpressionRef left,
                    BinaryenExpressionRef right,
                    const uint8_t mask[16]);
BINARYEN_API BinaryenExpressionRef BinaryenSIMDTernary(BinaryenModuleRef module,
                                                       BinaryenOp op,
                                                       BinaryenExpressionRef a,
                                                       BinaryenExpressionRef b,
                                                       BinaryenExpressionRef c);
BINARYEN_API BinaryenExpressionRef
BinaryenSIMDShift(BinaryenModuleRef module,
                  BinaryenOp op,
                  BinaryenExpressionRef vec,
                  BinaryenExpressionRef shift);
BINARYEN_API BinaryenExpressionRef BinaryenSIMDLoad(BinaryenModuleRef module,
                                                    BinaryenOp op,
                                                    uint32_t offset,
                                                    uint32_t align,
                                                    BinaryenExpressionRef ptr,
                                                    const char* name);
BINARYEN_API BinaryenExpressionRef
BinaryenSIMDLoadStoreLane(BinaryenModuleRef module,
                          BinaryenOp op,
                          uint32_t offset,
                          uint32_t align,
                          uint8_t index,
                          BinaryenExpressionRef ptr,
                          BinaryenExpressionRef vec,
                          const char* memoryName);
BINARYEN_API BinaryenExpressionRef
BinaryenMemoryInit(BinaryenModuleRef module,
                   uint32_t segment,
                   BinaryenExpressionRef dest,
                   BinaryenExpressionRef offset,
                   BinaryenExpressionRef size,
                   const char* memoryName);
BINARYEN_API BinaryenExpressionRef BinaryenDataDrop(BinaryenModuleRef module,
                                                    uint32_t segment);
BINARYEN_API BinaryenExpressionRef
BinaryenMemoryCopy(BinaryenModuleRef module,
                   BinaryenExpressionRef dest,
                   BinaryenExpressionRef source,
                   BinaryenExpressionRef size,
                   const char* destMemory,
                   const char* sourceMemory);
BINARYEN_API BinaryenExpressionRef
BinaryenMemoryFill(BinaryenModuleRef module,
                   BinaryenExpressionRef dest,
                   BinaryenExpressionRef value,
                   BinaryenExpressionRef size,
                   const char* memoryName);
BINARYEN_API BinaryenExpressionRef BinaryenRefNull(BinaryenModuleRef module,
                                                   BinaryenType type);
BINARYEN_API BinaryenExpressionRef
BinaryenRefIsNull(BinaryenModuleRef module, BinaryenExpressionRef value);
BINARYEN_API BinaryenExpressionRef BinaryenRefAs(BinaryenModuleRef module,
                                                 BinaryenOp op,
                                                 BinaryenExpressionRef value);
BINARYEN_API BinaryenExpressionRef BinaryenRefFunc(BinaryenModuleRef module,
                                                   const char* func,
                                                   BinaryenType type);
BINARYEN_API BinaryenExpressionRef BinaryenRefEq(BinaryenModuleRef module,
                                                 BinaryenExpressionRef left,
                                                 BinaryenExpressionRef right);
BINARYEN_API BinaryenExpressionRef BinaryenTableGet(BinaryenModuleRef module,
                                                    const char* name,
                                                    BinaryenExpressionRef index,
                                                    BinaryenType type);
BINARYEN_API BinaryenExpressionRef
BinaryenTableSet(BinaryenModuleRef module,
                 const char* name,
                 BinaryenExpressionRef index,
                 BinaryenExpressionRef value);
BINARYEN_API BinaryenExpressionRef BinaryenTableSize(BinaryenModuleRef module,
                                                     const char* name);
BINARYEN_API BinaryenExpressionRef
BinaryenTableGrow(BinaryenModuleRef module,
                  const char* name,
                  BinaryenExpressionRef value,
                  BinaryenExpressionRef delta);
// Try: name can be NULL. delegateTarget should be NULL in try-catch.
BINARYEN_API BinaryenExpressionRef
BinaryenTry(BinaryenModuleRef module,
            const char* name,
            BinaryenExpressionRef body,
            const char** catchTags,
            BinaryenIndex numCatchTags,
            BinaryenExpressionRef* catchBodies,
            BinaryenIndex numCatchBodies,
            const char* delegateTarget);
BINARYEN_API BinaryenExpressionRef
BinaryenThrow(BinaryenModuleRef module,
              const char* tag,
              BinaryenExpressionRef* operands,
              BinaryenIndex numOperands);
BINARYEN_API BinaryenExpressionRef BinaryenRethrow(BinaryenModuleRef module,
                                                   const char* target);
BINARYEN_API BinaryenExpressionRef
BinaryenTupleMake(BinaryenModuleRef module,
                  BinaryenExpressionRef* operands,
                  BinaryenIndex numOperands);
BINARYEN_API BinaryenExpressionRef BinaryenTupleExtract(
  BinaryenModuleRef module, BinaryenExpressionRef tuple, BinaryenIndex index);
BINARYEN_API BinaryenExpressionRef BinaryenPop(BinaryenModuleRef module,
                                               BinaryenType type);
BINARYEN_API BinaryenExpressionRef BinaryenI31New(BinaryenModuleRef module,
                                                  BinaryenExpressionRef value);
BINARYEN_API BinaryenExpressionRef BinaryenI31Get(BinaryenModuleRef module,
                                                  BinaryenExpressionRef i31,
                                                  bool signed_);
BINARYEN_API BinaryenExpressionRef
BinaryenCallRef(BinaryenModuleRef module,
                BinaryenExpressionRef target,
                BinaryenExpressionRef* operands,
                BinaryenIndex numOperands,
                BinaryenType type,
                bool isReturn);
BINARYEN_API BinaryenExpressionRef BinaryenRefTest(BinaryenModuleRef module,
                                                   BinaryenExpressionRef ref,
                                                   BinaryenType castType);
BINARYEN_API BinaryenExpressionRef BinaryenRefCast(BinaryenModuleRef module,
                                                   BinaryenExpressionRef ref,
                                                   BinaryenType type);
BINARYEN_API BinaryenExpressionRef BinaryenBrOn(BinaryenModuleRef module,
                                                BinaryenOp op,
                                                const char* name,
                                                BinaryenExpressionRef ref,
                                                BinaryenType castType);
BINARYEN_API BinaryenExpressionRef
BinaryenStructNew(BinaryenModuleRef module,
                  BinaryenExpressionRef* operands,
                  BinaryenIndex numOperands,
                  BinaryenHeapType type);
BINARYEN_API BinaryenExpressionRef BinaryenStructGet(BinaryenModuleRef module,
                                                     BinaryenIndex index,
                                                     BinaryenExpressionRef ref,
                                                     BinaryenType type,
                                                     bool signed_);
BINARYEN_API BinaryenExpressionRef
BinaryenStructSet(BinaryenModuleRef module,
                  BinaryenIndex index,
                  BinaryenExpressionRef ref,
                  BinaryenExpressionRef value);
BINARYEN_API BinaryenExpressionRef BinaryenArrayNew(BinaryenModuleRef module,
                                                    BinaryenHeapType type,
                                                    BinaryenExpressionRef size,
                                                    BinaryenExpressionRef init);

// TODO: BinaryenArrayNewSeg

BINARYEN_API BinaryenExpressionRef
BinaryenArrayNewFixed(BinaryenModuleRef module,
                      BinaryenHeapType type,
                      BinaryenExpressionRef* values,
                      BinaryenIndex numValues);
BINARYEN_API BinaryenExpressionRef BinaryenArrayGet(BinaryenModuleRef module,
                                                    BinaryenExpressionRef ref,
                                                    BinaryenExpressionRef index,
                                                    BinaryenType type,
                                                    bool signed_);
BINARYEN_API BinaryenExpressionRef
BinaryenArraySet(BinaryenModuleRef module,
                 BinaryenExpressionRef ref,
                 BinaryenExpressionRef index,
                 BinaryenExpressionRef value);
BINARYEN_API BinaryenExpressionRef BinaryenArrayLen(BinaryenModuleRef module,
                                                    BinaryenExpressionRef ref);
BINARYEN_API BinaryenExpressionRef
BinaryenArrayCopy(BinaryenModuleRef module,
                  BinaryenExpressionRef destRef,
                  BinaryenExpressionRef destIndex,
                  BinaryenExpressionRef srcRef,
                  BinaryenExpressionRef srcIndex,
                  BinaryenExpressionRef length);
BINARYEN_API BinaryenExpressionRef
BinaryenStringNew(BinaryenModuleRef module,
                  BinaryenOp op,
                  BinaryenExpressionRef ptr,
                  BinaryenExpressionRef length,
                  BinaryenExpressionRef start,
                  BinaryenExpressionRef end,
                  bool try_);
BINARYEN_API BinaryenExpressionRef BinaryenStringConst(BinaryenModuleRef module,
                                                       const char* name);
BINARYEN_API BinaryenExpressionRef BinaryenStringMeasure(
  BinaryenModuleRef module, BinaryenOp op, BinaryenExpressionRef ref);
BINARYEN_API BinaryenExpressionRef
BinaryenStringEncode(BinaryenModuleRef module,
                     BinaryenOp op,
                     BinaryenExpressionRef ref,
                     BinaryenExpressionRef ptr,
                     BinaryenExpressionRef start);
BINARYEN_API BinaryenExpressionRef
BinaryenStringConcat(BinaryenModuleRef module,
                     BinaryenExpressionRef left,
                     BinaryenExpressionRef right);
BINARYEN_API BinaryenExpressionRef
BinaryenStringEq(BinaryenModuleRef module,
                 BinaryenOp op,
                 BinaryenExpressionRef left,
                 BinaryenExpressionRef right);
BINARYEN_API BinaryenExpressionRef BinaryenStringAs(BinaryenModuleRef module,
                                                    BinaryenOp op,
                                                    BinaryenExpressionRef ref);
BINARYEN_API BinaryenExpressionRef
BinaryenStringWTF8Advance(BinaryenModuleRef module,
                          BinaryenExpressionRef ref,
                          BinaryenExpressionRef pos,
                          BinaryenExpressionRef bytes);
BINARYEN_API BinaryenExpressionRef
BinaryenStringWTF16Get(BinaryenModuleRef module,
                       BinaryenExpressionRef ref,
                       BinaryenExpressionRef pos);
BINARYEN_API BinaryenExpressionRef
BinaryenStringIterNext(BinaryenModuleRef module, BinaryenExpressionRef ref);
BINARYEN_API BinaryenExpressionRef
BinaryenStringIterMove(BinaryenModuleRef module,
                       BinaryenOp op,
                       BinaryenExpressionRef ref,
                       BinaryenExpressionRef num);
BINARYEN_API BinaryenExpressionRef
BinaryenStringSliceWTF(BinaryenModuleRef module,
                       BinaryenOp op,
                       BinaryenExpressionRef ref,
                       BinaryenExpressionRef start,
                       BinaryenExpressionRef end);
BINARYEN_API BinaryenExpressionRef
BinaryenStringSliceIter(BinaryenModuleRef module,
                        BinaryenExpressionRef ref,
                        BinaryenExpressionRef num);

// Expression

// Gets the id (kind) of the given expression.
BINARYEN_API BinaryenExpressionId
BinaryenExpressionGetId(BinaryenExpressionRef expr);
// Gets the type of the given expression.
BINARYEN_API BinaryenType BinaryenExpressionGetType(BinaryenExpressionRef expr);
// Sets the type of the given expression.
BINARYEN_API void BinaryenExpressionSetType(BinaryenExpressionRef expr,
                                            BinaryenType type);
// Prints text format of the given expression to stdout.
BINARYEN_API void BinaryenExpressionPrint(BinaryenExpressionRef expr);
// Re-finalizes an expression after it has been modified.
BINARYEN_API void BinaryenExpressionFinalize(BinaryenExpressionRef expr);
// Makes a deep copy of the given expression.
BINARYEN_API BinaryenExpressionRef
BinaryenExpressionCopy(BinaryenExpressionRef expr, BinaryenModuleRef module);

// Block

// Gets the name (label) of a `block` expression.
BINARYEN_API const char* BinaryenBlockGetName(BinaryenExpressionRef expr);
// Sets the name (label) of a `block` expression.
BINARYEN_API void BinaryenBlockSetName(BinaryenExpressionRef expr,
                                       const char* name);
// Gets the number of child expressions of a `block` expression.
BINARYEN_API BinaryenIndex
BinaryenBlockGetNumChildren(BinaryenExpressionRef expr);
// Gets the child expression at the specified index of a `block` expression.
BINARYEN_API BinaryenExpressionRef
BinaryenBlockGetChildAt(BinaryenExpressionRef expr, BinaryenIndex index);
// Sets (replaces) the child expression at the specified index of a `block`
// expression.
BINARYEN_API void BinaryenBlockSetChildAt(BinaryenExpressionRef expr,
                                          BinaryenIndex index,
                                          BinaryenExpressionRef childExpr);
// Appends a child expression to a `block` expression, returning its insertion
// index.
BINARYEN_API BinaryenIndex BinaryenBlockAppendChild(
  BinaryenExpressionRef expr, BinaryenExpressionRef childExpr);
// Inserts a child expression at the specified index of a `block` expression,
// moving existing children including the one previously at that index one index
// up.
BINARYEN_API void BinaryenBlockInsertChildAt(BinaryenExpressionRef expr,
                                             BinaryenIndex index,
                                             BinaryenExpressionRef childExpr);
// Removes the child expression at the specified index of a `block` expression,
// moving all subsequent children one index down. Returns the child expression.
BINARYEN_API BinaryenExpressionRef
BinaryenBlockRemoveChildAt(BinaryenExpressionRef expr, BinaryenIndex index);

// If

// Gets the condition expression of an `if` expression.
BINARYEN_API BinaryenExpressionRef
BinaryenIfGetCondition(BinaryenExpressionRef expr);
// Sets the condition expression of an `if` expression.
BINARYEN_API void BinaryenIfSetCondition(BinaryenExpressionRef expr,
                                         BinaryenExpressionRef condExpr);
// Gets the ifTrue (then) expression of an `if` expression.
BINARYEN_API BinaryenExpressionRef
BinaryenIfGetIfTrue(BinaryenExpressionRef expr);
// Sets the ifTrue (then) expression of an `if` expression.
BINARYEN_API void BinaryenIfSetIfTrue(BinaryenExpressionRef expr,
                                      BinaryenExpressionRef ifTrueExpr);
// Gets the ifFalse (else) expression, if any, of an `if` expression.
BINARYEN_API BinaryenExpressionRef
BinaryenIfGetIfFalse(BinaryenExpressionRef expr);
// Sets the ifFalse (else) expression, if any, of an `if` expression.
BINARYEN_API void BinaryenIfSetIfFalse(BinaryenExpressionRef expr,
                                       BinaryenExpressionRef ifFalseExpr);

// Loop

// Gets the name (label) of a `loop` expression.
BINARYEN_API const char* BinaryenLoopGetName(BinaryenExpressionRef expr);
// Sets the name (label) of a `loop` expression.
BINARYEN_API void BinaryenLoopSetName(BinaryenExpressionRef expr,
                                      const char* name);
// Gets the body expression of a `loop` expression.
BINARYEN_API BinaryenExpressionRef
BinaryenLoopGetBody(BinaryenExpressionRef expr);
// Sets the body expression of a `loop` expression.
BINARYEN_API void BinaryenLoopSetBody(BinaryenExpressionRef expr,
                                      BinaryenExpressionRef bodyExpr);

// Break

// Gets the name (target label) of a `br` or `br_if` expression.
BINARYEN_API const char* BinaryenBreakGetName(BinaryenExpressionRef expr);
// Sets the name (target label) of a `br` or `br_if` expression.
BINARYEN_API void BinaryenBreakSetName(BinaryenExpressionRef expr,
                                       const char* name);
// Gets the condition expression, if any, of a `br_if` expression. No condition
// indicates a `br` expression.
BINARYEN_API BinaryenExpressionRef
BinaryenBreakGetCondition(BinaryenExpressionRef expr);
// Sets the condition expression, if any, of a `br_if` expression. No condition
// makes it a `br` expression.
BINARYEN_API void BinaryenBreakSetCondition(BinaryenExpressionRef expr,
                                            BinaryenExpressionRef condExpr);
// Gets the value expression, if any, of a `br` or `br_if` expression.
BINARYEN_API BinaryenExpressionRef
BinaryenBreakGetValue(BinaryenExpressionRef expr);
// Sets the value expression, if any, of a `br` or `br_if` expression.
BINARYEN_API void BinaryenBreakSetValue(BinaryenExpressionRef expr,
                                        BinaryenExpressionRef valueExpr);

// Switch

// Gets the number of names (target labels) of a `br_table` expression.
BINARYEN_API BinaryenIndex
BinaryenSwitchGetNumNames(BinaryenExpressionRef expr);
// Gets the name (target label) at the specified index of a `br_table`
// expression.
BINARYEN_API const char* BinaryenSwitchGetNameAt(BinaryenExpressionRef expr,
                                                 BinaryenIndex index);
// Sets the name (target label) at the specified index of a `br_table`
// expression.
BINARYEN_API void BinaryenSwitchSetNameAt(BinaryenExpressionRef expr,
                                          BinaryenIndex index,
                                          const char* name);
// Appends a name to a `br_table` expression, returning its insertion index.
BINARYEN_API BinaryenIndex BinaryenSwitchAppendName(BinaryenExpressionRef expr,
                                                    const char* name);
// Inserts a name at the specified index of a `br_table` expression, moving
// existing names including the one previously at that index one index up.
BINARYEN_API void BinaryenSwitchInsertNameAt(BinaryenExpressionRef expr,
                                             BinaryenIndex index,
                                             const char* name);
// Removes the name at the specified index of a `br_table` expres