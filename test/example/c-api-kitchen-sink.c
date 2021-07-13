// We always need asserts here
#ifdef NDEBUG
#undef NDEBUG
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <binaryen-c.h>

// kitchen sink, tests the full API

// helpers

static const uint8_t v128_bytes[] = {
  1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

BinaryenExpressionRef
makeUnary(BinaryenModuleRef module, BinaryenOp op, BinaryenType inputType) {
  if (inputType == BinaryenTypeInt32())
    return BinaryenUnary(
      module, op, BinaryenConst(module, BinaryenLiteralInt32(-10)));
  if (inputType == BinaryenTypeInt64())
    return BinaryenUnary(
      module, op, BinaryenConst(module, BinaryenLiteralInt64(-22)));
  if (inputType == BinaryenTypeFloat32())
    return BinaryenUnary(
      module, op, BinaryenConst(module, BinaryenLiteralFloat32(-33.612f)));
  if (inputType == BinaryenTypeFloat64())
    return BinaryenUnary(
      module, op, BinaryenConst(module, BinaryenLiteralFloat64(-9005.841)));
  if (inputType == BinaryenTypeVec128())
    return BinaryenUnary(
      module, op, BinaryenConst(module, BinaryenLiteralVec128(v128_bytes)));
  abort();
}

BinaryenExpressionRef
makeBinary(BinaryenModuleRef module, BinaryenOp op, BinaryenType type) {
  if (type == BinaryenTypeInt32()) {
    // use temp vars to ensure optimization doesn't change the order of
    // operation in our trace recording
    BinaryenExpressionRef temp =
      BinaryenConst(module, BinaryenLiteralInt32(-11));
    return BinaryenBinary(
      module, op, BinaryenConst(module, BinaryenLiteralInt32(-10)), temp);
  }
  if (type == BinaryenTypeInt64()) {
    BinaryenExpressionRef temp =
      BinaryenConst(module, BinaryenLiteralInt64(-23));
    return BinaryenBinary(
      module, op, BinaryenConst(module, BinaryenLiteralInt64(-22)), temp);
  }
  if (type == BinaryenTypeFloat32()) {
    BinaryenExpressionRef temp =
      BinaryenConst(module, BinaryenLiteralFloat32(-62.5f));
    return BinaryenBinary(
      module,
      op,
      BinaryenConst(module, BinaryenLiteralFloat32(-33.612f)),
      temp);
  }
  if (type == BinaryenTypeFloat64()) {
    BinaryenExpressionRef temp =
      BinaryenConst(module, BinaryenLiteralFloat64(-9007.333));
    return BinaryenBinary(
      module,
      op,
      BinaryenConst(module, BinaryenLiteralFloat64(-9005.841)),
      temp);
  }
  if (type == BinaryenTypeVec128()) {
    BinaryenExpressionRef temp =
      BinaryenConst(module, BinaryenLiteralVec128(v128_bytes));
    return BinaryenBinary(
      module,
      op,
      BinaryenConst(module, BinaryenLiteralVec128(v128_bytes)),
      temp);
  }
  abort();
}

BinaryenExpressionRef makeInt32(BinaryenModuleRef module, int x) {
  return BinaryenConst(module, BinaryenLiteralInt32(x));
}

BinaryenExpressionRef makeFloat32(BinaryenModuleRef module, float x) {
  return BinaryenConst(module, BinaryenLiteralFloat32(x));
}

BinaryenExpressionRef makeInt64(BinaryenModuleRef module, int64_t x) {
  return BinaryenConst(module, BinaryenLiteralInt64(x));
}

BinaryenExpressionRef makeFloat64(BinaryenModuleRef module, double x) {
  return BinaryenConst(module, BinaryenLiteralFloat64(x));
}

BinaryenExpressionRef makeVec128(BinaryenModuleRef module,
                                 uint8_t const* bytes) {
  return BinaryenConst(module, BinaryenLiteralVec128(bytes));
}

BinaryenExpressionRef makeSomething(BinaryenModuleRef module) {
  return makeInt32(module, 1337);
}

BinaryenExpressionRef makeDroppedInt32(BinaryenModuleRef module, int x) {
  return BinaryenDrop(module, BinaryenConst(module, BinaryenLiteralInt32(x)));
}

BinaryenExpressionRef makeSIMDExtract(BinaryenModuleRef module, BinaryenOp op) {
  return BinaryenSIMDExtract(module, op, makeVec128(module, v128_bytes), 0);
}

BinaryenExpressionRef
makeSIMDReplace(BinaryenModuleRef module, BinaryenOp op, BinaryenType type) {
  BinaryenExpressionRef val;
  if (type == BinaryenTypeInt32()) {
    val = makeInt32(module, 42);
  }
  if (type == BinaryenTypeInt64()) {
    val = makeInt64(module, 42);
  }
  if (type == BinaryenTypeFloat32()) {
    val = makeFloat32(module, 42.);
  }
  if (type == BinaryenTypeFloat64()) {
    val = makeFloat64(module, 42.);
  }
  if (!val) {
    abort();
  }
  return BinaryenSIMDReplace(
    module, op, makeVec128(module, v128_bytes), 0, val);
}

BinaryenExpressionRef makeSIMDShuffle(BinaryenModuleRef module) {
  BinaryenExpressionRef left = makeVec128(module, v128_bytes);
  BinaryenExpressionRef right = makeVec128(module, v128_bytes);
  return BinaryenSIMDShuffle(module, left, right, (uint8_t[16]){});
}

BinaryenExpressionRef makeSIMDTernary(BinaryenModuleRef module, BinaryenOp op) {
  BinaryenExpressionRef a = makeVec128(module, v128_bytes);
  BinaryenExpressionRef b = makeVec128(module, v128_bytes);
  BinaryenExpressionRef c = makeVec128(module, v128_bytes);
  return BinaryenSIMDTernary(module, op, a, b, c);
}

BinaryenExpressionRef makeSIMDShift(BinaryenModuleRef module, BinaryenOp op) {
  BinaryenExpressionRef vec = makeVec128(module, v128_bytes);
  return BinaryenSIMDShift(module, op, vec, makeInt32(module, 1));
}

BinaryenExpressionRef makeMemoryInit(BinaryenModuleRef module) {
  BinaryenExpressionRef dest = makeInt32(module, 1024);
  BinaryenExpressionRef offset = makeInt32(module, 0);
  BinaryenExpressionRef size = makeInt32(module, 12);
  return BinaryenMemoryInit(module, 0, dest, offset, size, "0");
};

BinaryenExpressionRef makeDataDrop(BinaryenModuleRef module) {
  return BinaryenDataDrop(module, 0);
};

BinaryenExpressionRef makeMemoryCopy(BinaryenModuleRef module) {
  BinaryenExpressionRef dest = makeInt32(module, 2048);
  BinaryenExpressionRef source = makeInt32(module, 1024);
  BinaryenExpressionRef size = makeInt32(module, 12);
  return BinaryenMemoryCopy(module, dest, source, size, "0", "0");
};

BinaryenExpressionRef makeMemoryFill(BinaryenModuleRef module) {
  BinaryenExpressionRef dest = makeInt32(module, 0);
  BinaryenExpressionRef value = makeInt32(module, 42);
  BinaryenExpressionRef size = makeInt32(module, 1024);
  return BinaryenMemoryFill(module, dest, value, size, "0");
};

// tests

void test_types() {
  BinaryenType valueType = 0xdeadbeef;

  BinaryenType none = BinaryenTypeNone();
  printf("BinaryenTypeNone: %zd\n", none);
  assert(BinaryenTypeArity(none) == 0);
  BinaryenTypeExpand(none, &valueType);
  assert(valueType == 0xdeadbeef);

  BinaryenType unreachable = BinaryenTypeUnreachable();
  printf("BinaryenTypeUnreachable: %zd\n", unreachable);
  assert(BinaryenTypeArity(unreachable) == 1);
  BinaryenTypeExpand(unreachable, &valueType);
  assert(valueType == unreachable);

  BinaryenType i32 = BinaryenTypeInt32();
  printf("BinaryenTypeInt32: %zd\n", i32);
  assert(BinaryenTypeArity(i32) == 1);
  BinaryenTypeExpand(i32, &valueType);
  assert(valueType == i32);

  BinaryenType i64 = BinaryenTypeInt64();
  printf("BinaryenTypeInt64: %zd\n", i64);
  assert(BinaryenTypeArity(i64) == 1);
  BinaryenTypeExpand(i64, &valueType);
  assert(valueType == i64);

  BinaryenType f32 = BinaryenTypeFloat32();
  printf("BinaryenTypeFloat32: %zd\n", f32);
  assert(BinaryenTypeArity(f32) == 1);
  BinaryenTypeExpand(f32, &valueType);
  assert(valueType == f32);

  BinaryenType f64 = BinaryenTypeFloat64();
  printf("BinaryenTypeFloat64: %zd\n", f64);
  assert(BinaryenTypeArity(f64) == 1);
  BinaryenTypeExpand(f64, &valueType);
  assert(valueType == f64);

  BinaryenType v128 = BinaryenTypeVec128();
  printf("BinaryenTypeVec128: %zd\n", v128);
  assert(BinaryenTypeArity(v128) == 1);
  BinaryenTypeExpand(v128, &valueType);
  assert(valueType == v128);

  BinaryenType funcref = BinaryenTypeFuncref();
  printf("BinaryenTypeFuncref: (ptr)\n");
  assert(funcref == BinaryenTypeFuncref());
  assert(BinaryenTypeArity(funcref) == 1);
  BinaryenTypeExpand(funcref, &valueType);
  assert(valueType == funcref);

  BinaryenType externref = BinaryenTypeExternref();
  printf("BinaryenTypeExternref: (ptr)\n");
  assert(externref == BinaryenTypeExternref());
  assert(BinaryenTypeArity(externref) == 1);
  BinaryenTypeExpand(externref, &valueType);
  assert(valueType == externref);

  BinaryenType anyref = BinaryenTypeAnyref();
  printf("BinaryenTypeAnyref: (ptr)\n");
  assert(anyref == BinaryenTypeAnyref());
  assert(BinaryenTypeArity(anyref) == 1);
  BinaryenTypeExpand(anyref, &valueType);
  assert(valueType == anyref);

  BinaryenType eqref = BinaryenTypeEqref();
  printf("BinaryenTypeEqref: (ptr)\n");
  assert(eqref == BinaryenTypeEqref());
  assert(BinaryenTypeArity(eqref) == 1);
  BinaryenTypeExpand(eqref, &valueType);
  assert(valueType == eqref);

  BinaryenType i31ref = BinaryenTypeI31ref();
  printf("BinaryenTypeI31ref: (ptr)\n");
  assert(i31ref == BinaryenTypeI31ref());
  assert(BinaryenTypeArity(i31ref) == 1);
  BinaryenTypeExpand(i31ref, &valueType);
  assert(valueType == i31ref);

  BinaryenType structref = BinaryenTypeStructref();
  printf("BinaryenTypeStructref: (ptr)\n");
  assert(structref == BinaryenTypeStructref());
  assert(BinaryenTypeArity(structref) == 1);
  BinaryenTypeExpand(structref, &valueType);
  assert(valueType == structref);

  BinaryenType arrayref = BinaryenTypeArrayref();
  printf("BinaryenTypeArrayref: (ptr)\n");
  assert(arrayref == BinaryenTypeArrayref());
  assert(BinaryenTypeArity(arrayref) == 1);
  BinaryenTypeExpand(arrayref, &valueType);
  assert(valueType == arrayref);

  BinaryenType stringref = BinaryenTypeStringref();
  printf("BinaryenTypeStringref: (ptr)\n");
  assert(BinaryenTypeArity(stringref) == 1);
  BinaryenTypeExpand(stringref, &valueType);
  assert(valueType == stringref);

  BinaryenType stringview_wtf8_ = BinaryenTypeStringviewWTF8();
  printf("BinaryenTypeStringviewWTF8: (ptr)\n");
  assert(BinaryenTypeArity(stringview_wtf8_) == 1);
  BinaryenTypeExpand(stringview_wtf8_, &valueType);
  assert(valueType == stringview_wtf8_);

  BinaryenType stringview_wtf16_ = BinaryenTypeStringviewWTF16();
  printf("BinaryenTypeStringviewWTF16: (ptr)\n");
  assert(BinaryenTypeArity(stringview_wtf16_) == 1);
  BinaryenTypeExpand(stringview_wtf16_, &valueType);
  assert(valueType == stringview_wtf16_);

  BinaryenType stringview_iter_ = BinaryenTypeStringviewIter();
  printf("BinaryenTypeStringviewIter: (ptr)\n");
  assert(BinaryenTypeArity(stringview_iter_) == 1);
  BinaryenTypeExpand(stringview_iter_, &valueType);
  assert(valueType == stringview_iter_);

  BinaryenType nullref = BinaryenTypeNullref();
  printf("BinaryenTypeNullref: (ptr)\n");
  assert(BinaryenTypeArity(nullref) == 1);
  BinaryenTypeExpand(nullref, &valueType);
  assert(valueType == nullref);

  BinaryenType nullexternref = BinaryenTypeNullExternref();
  printf("BinaryenTypeNullExternref: (ptr)\n");
  assert(BinaryenTypeArity(nullexternref) == 1);
  BinaryenTypeExpand(nullexternref, &valueType);
  assert(valueType == nullexternref);

  BinaryenType nullfuncref = BinaryenTypeNullFuncref();
  printf("BinaryenTypeNullFuncref: (ptr)\n");
  assert(BinaryenTypeArity(nullfuncref) == 1);
  BinaryenTypeExpand(nullfuncref, &valueType);
  assert(valueType == nullfuncref);

  printf("BinaryenTypeAuto: %zd\n", BinaryenTypeAuto());

  BinaryenType pair[] = {i32, i32};

  BinaryenType i32_pair = BinaryenTypeCreate(pair, 2);
  assert(BinaryenTypeArity(i32_pair) == 2);
  pair[0] = pair[1] = none;
  BinaryenTypeExpand(i32_pair, pair);
  assert(pair[0] == i32 && pair[1] == i32);

  BinaryenType duplicate_pair = BinaryenTypeCreate(pair, 2);
  assert(duplicate_pair == i32_pair);

  pair[0] = pair[1] = f32;
  BinaryenType float_pair = BinaryenTypeCreate(pair, 2);
  assert(float_pair != i32_pair);

  BinaryenPackedType notPacked = BinaryenPackedTypeNotPacked();
  printf("BinaryenPackedTypeNotPacked: %d\n", notPacked);
  BinaryenPackedType i8 = BinaryenPackedTypeInt8();
  printf("BinaryenPackedTypeInt8: %d\n", i8);
  BinaryenPackedType i16 = BinaryenPackedTypeInt16();
  printf("BinaryenPackedTypeInt16: %d\n", i16);

  printf("BinaryenHeapTypeExt: %zd\n", BinaryenHeapTypeExt());
  printf("BinaryenHeapTypeFunc: %zd\n", BinaryenHeapTypeFunc());
  printf("BinaryenHeapTypeAny: %zd\n", BinaryenHeapTypeAny());
  printf("BinaryenHeapTypeEq: %zd\n", BinaryenHeapTypeEq());
  printf("BinaryenHeapTypeI31: %zd\n", BinaryenHeapTypeI31());
  printf("BinaryenHeapTypeStruct: %zd\n", BinaryenHeapTypeStruct());
  printf("BinaryenHeapTypeArray: %zd\n", BinaryenHeapTypeArray());
  printf("BinaryenHeapTypeString: %zd\n", BinaryenHeapTypeString());
  printf("BinaryenHeapTypeStringviewWTF8: %zd\n",
         BinaryenHeapTypeStringviewWTF8());
  printf("BinaryenHeapTypeStringviewWTF16: %zd\n",
         BinaryenHeapTypeStringviewWTF16());
  printf("BinaryenHeapTypeStringviewIter: %zd\n",
         BinaryenHeapTypeStringviewIter());
  printf("BinaryenHeapTypeNone: %zd\n", BinaryenHeapTypeNone());
  printf("BinaryenHeapTypeNoext: %zd\n", BinaryenHeapTypeNoext());
  printf("BinaryenHeapTypeNofunc: %zd\n", BinaryenHeapTypeNofunc());

  assert(!BinaryenHeapTypeIsBottom(BinaryenHeapTypeExt()));
  assert(BinaryenHeapTypeIsBottom(BinaryenHeapTypeNoext()));
  assert(BinaryenHeapTypeGetBottom(BinaryenHeapTypeExt()) ==
         BinaryenHeapTypeNoext());

  BinaryenHeapType eq = BinaryenTypeGetHeapType(eqref);
  assert(eq == BinaryenHeapTypeEq());
  BinaryenType ref_null_eq = BinaryenTypeFromHeapType(eq, true);
  assert(BinaryenTypeGetHeapType(ref_null_eq) == eq);
  assert(BinaryenTypeIsNullable(ref_null_eq));
  BinaryenType ref_eq = BinaryenTypeFromHeapType(eq, false);
  assert(ref_eq != ref_null_eq);
  assert(BinaryenTypeGetHeapType(ref_eq) == eq);
  assert(!BinaryenTypeIsNullable(ref_eq));
}

void test_features() {
  printf("BinaryenFeatureMVP: %d\n", BinaryenFeatureMVP());
  printf("BinaryenFeatureAtomics: %d\n", BinaryenFeatureAtomics());
  printf("BinaryenFeatureBulkMemory: %d\n", BinaryenFeatureBulkMemory());
  printf("BinaryenFeatureMutableGlobals: %d\n",
         BinaryenFeatureMutableGlobals());
  printf("BinaryenFeatureNontrappingFPToInt: %d\n",
         BinaryenFeatureNontrappingFPToInt());
  printf("BinaryenFeatureSignExt: %d\n", BinaryenFeatureSignExt());
  printf("BinaryenFeatureSIMD128: %d\n", BinaryenFeatureSIMD128());
  printf("BinaryenFeatureExceptionHandling: %d\n",
         BinaryenFeatureExceptionHandling());
  printf("BinaryenFeatureTailCall: %d\n", BinaryenFeatureTailCall());
  printf("BinaryenFeatureReferenceTypes: %d\n",
         BinaryenFeatureReferenceTypes());
  printf("BinaryenFeatureMultivalue: %d\n", BinaryenFeatureMultivalue());
  printf("BinaryenFeatureGC: %d\n", BinaryenFeatureGC());
  printf("BinaryenFeatureMemory64: %d\n", BinaryenFeatureMemory64());
  printf("BinaryenFeatureRelaxedSIMD: %d\n", BinaryenFeatureRelaxedSIMD());
  printf("BinaryenFeatureExtendedConst: %d\n", BinaryenFeatureExtendedConst());
  printf("BinaryenFeatureStrings: %d\n", BinaryenFeatureStrings());
  printf("BinaryenFeatureAll: %d\n", BinaryenFeatureAll());
}

void test_core() {

  // Module creation

  BinaryenModuleRef module = BinaryenModuleCreate();

  // Literals and consts

  BinaryenExpressionRef
    constI32 = BinaryenConst(module, BinaryenLiteralInt32(1)),
    constI64 = BinaryenConst(module, BinaryenLiteralInt64(2)),
    constF32 = BinaryenConst(module, BinaryenLiteralFloat32(3.14f)),
    constF64 = BinaryenConst(module, BinaryenLiteralFloat64(2.1828)),
    constF32Bits =
      BinaryenConst(module, BinaryenLiteralFloat32Bits(0xffff1234)),
    constF64Bits =
      BinaryenConst(module, BinaryenLiteralFloat64Bits(0xffff12345678abcdLL)),
    constV128 = BinaryenConst(module, BinaryenLiteralVec128(v128_bytes));

  const char* switchValueNames[] = {"the-value"};
  const char* switchBodyNames[] = {"the-nothing"};

  BinaryenExpressionRef callOperands2[] = {makeInt32(module, 13),
                                           makeFloat64(module, 3.7)};
  BinaryenExpressionRef callOperands4[] = {makeInt32(module, 13),
                                           makeInt64(module, 37),
                                           makeFloat32(module, 1.3f),
                                           makeFloat64(module, 3.7)};
  BinaryenExpressionRef callOperands4b[] = {makeInt32(module, 13),
                                            makeInt64(module, 37),
                                            makeFloat32(module, 1.3f),
                                            makeFloat64(module, 3.7)};
  BinaryenExpressionRef tupleElements4a[] = {makeInt32(module, 13),
                                             makeInt64(module, 37),
                                             makeFloat32(module, 1.3f),
                                             makeFloat64(module, 3.7)};
  BinaryenExpressionRef tupleElements4b[] = {makeInt32(module, 13),
                                             makeInt64(module, 37),
                                             makeFloat32(module, 1.3f),
                                             makeFloat64(module, 3.7)};

  BinaryenType iIfF_[4] = {BinaryenTypeInt32(),
                           BinaryenTypeInt64(),
                           BinaryenTypeFloat32(),
                           BinaryenTypeFloat64()};
  BinaryenType iIfF = BinaryenTypeCreate(iIfF_, 4);

  BinaryenExpressionRef temp1 = makeInt32(module, 1),
                        temp2 = makeInt32(module, 2),
                        temp3 = makeInt32(module, 3),
                        temp4 = makeInt32(module, 4),
                        temp5 = makeInt32(module, 5),
                        temp6 = makeInt32(module, 0),
                        temp7 = makeInt32(module, 1),
                        temp8 = makeInt32(module, 0),
                        temp9 = makeInt32(module, 1),
                        temp10 = makeInt32(module, 1),
                        temp11 = makeInt32(module, 3),
                        temp12 = makeInt32(module, 5),
                        temp13 = makeInt32(module, 10),
                        temp14 = makeInt32(module, 11),
                        temp15 = makeInt32(module, 110),
                        temp16 = makeInt64(module, 111);
  BinaryenExpressionRef externrefExpr =
    BinaryenRefNull(module, BinaryenTypeNullExternref());
  BinaryenExpressionRef funcrefExpr =
    BinaryenRefNull(module, BinaryenTypeNullFuncref());
  funcrefExpr =
    BinaryenRefFunc(module, "kitchen()sinker", BinaryenTypeFuncref());
  BinaryenExpressionRef i31refExpr =
    BinaryenI31New(module, makeInt32(module, 1));

  // Tags
  BinaryenAddTag(module, "a-tag", BinaryenTypeInt32(), BinaryenTypeNone());

  BinaryenAddTable(module, "tab", 0, 100, BinaryenTypeFuncref());

  // Exception handling

  // (try
  //   (do
  //     (throw $a-tag (i32.const 0))
  //   )
  //   (catch $a-tag
  //     (drop (i32 pop))
  //   )
  //   (catch_all)
  // )
  BinaryenExpressionRef tryBody = BinaryenThrow(
    module, "a-tag", (BinaryenExpressionRef[]){makeInt32(module, 0)}, 1);
  BinaryenExpressionRef catchBody =
    BinaryenDrop(module, BinaryenPop(module, BinaryenTypeInt32()));
  BinaryenExpressionRef catchAllBody = BinaryenNop(module);
  const char* catchTags[] = {"a-tag"};
  BinaryenExpressionRef catchBodies[] = {catchBody, catchAllBody};
  const char* emptyCatchTags[] = {};
  BinaryenExpressionRef emptyCatchBodies[] = {};
  BinaryenExpressionRef nopCatchBody[] = {BinaryenNop(module)};

  BinaryenType i32 = BinaryenTypeInt32();
  BinaryenType i64 = BinaryenTypeInt64();
  BinaryenType f32 = BinaryenTypeFloat32();
  BinaryenType f64 = BinaryenTypeFloat64();
  BinaryenType v128 = BinaryenTypeVec128();
  BinaryenType i8Array;
  BinaryenType i16Array;
  BinaryenType i32Struct;
  {
    TypeBuilderRef tb = TypeBuilderCreate(3);
    TypeBuilderSetArrayType(
      tb, 0, BinaryenTypeInt32(), BinaryenPackedTypeInt8(), true);
    TypeBuilderSetArrayType(
      tb, 1, BinaryenTypeInt32(), BinaryenPackedTypeInt16(), true);
    TypeBuilderSetStructType(
      tb,
      2,
      (BinaryenType[]){BinaryenTypeInt32()},
      (BinaryenPackedType[]){BinaryenPackedTypeNotPacked()},
      (bool[]){true},
      1);
    BinaryenHeapType builtHeapTypes[3];
    TypeBuilderBuildAndDispose(tb, (BinaryenHeapType*)&builtHeapTypes, 0, 0);
    i8Array = BinaryenTypeFromHeapType(builtHeapTypes[0], true);
    i16Array = BinaryenTypeFromHeapType(builtHeapTypes[1], true);
    i32Struct = BinaryenTypeFromHeapType(builtHeapTypes[2], true);
  }

  // Memory. Add it before creating any memory-using instructions.

  const char* segments[] = {"hello, world", "I am passive"};
  bool segmentPassive[] = {false, true};
  BinaryenExpressionRef segmentOffsets[] = {
    BinaryenConst(module, BinaryenLiteralInt32(10)), NULL};
  BinaryenIndex segmentSizes[] = {12, 12};
  BinaryenSetMemory(module,
                    1,
                    256,
                    "mem",
                    segments,
                    segmentPassive,
                    segmentOffsets,
                    segmentSizes,
                    2,
                    1,
                    0,
                    "0");

  BinaryenExpressionRef valueList[] = {
    // Unary
    makeUnary(module, BinaryenClzInt32(), i32),
    makeUnary(module, BinaryenCtzInt64(), i64),
    makeUnary(module, BinaryenPopcntInt32(), i32),
    makeUnary(module, BinaryenNegFloat32(), f32),
    makeUnary(module, BinaryenAbsFloat64(), f64),
    makeUnary(module, BinaryenCeilFloat32(), f32),
    makeUnary(module, BinaryenFloorFloat64(), f64),
    makeUnary(module, BinaryenTruncFloat32(), f32),
    makeUnary(module, BinaryenNearestFloat32(), f32),
    makeUnary(module, BinaryenSqrtFloat64(), f64),
    makeUnary(module, BinaryenEqZInt32(), i32),
    makeUnary(module, BinaryenExtendSInt32(), i32),
    makeUnary(module, BinaryenExtendUInt32(), i32),
    makeUnary(module, BinaryenWrapInt64(), i64),
    makeUnary(module, BinaryenTruncSFloat32ToInt32(), f32),
    makeUnary(module, BinaryenTruncSFloat32ToInt64(), f32),
    makeUnary(module, BinaryenTruncUFloat32ToInt32(), f32),
    makeUnary(module, BinaryenTruncUFloat32ToInt64(), f32),
    makeUnary(module, BinaryenTruncSFloat64ToInt32(), f64),
    makeUnary(module, BinaryenTruncSFloat64ToInt64(), f64),
    makeUnary(module, BinaryenTruncUFloat64ToInt32(), f64),
    makeUnary(module, BinaryenTruncUFloat64ToInt64(), f64),
    makeUnary(module, BinaryenTruncSatSFloat32ToInt32(), f32),
    makeUnary(module, BinaryenTruncSatSFloat32ToInt64(), f32),
    makeUnary(module, BinaryenTruncSatUFloat32ToInt32(), f32),
    makeUnary(module, BinaryenTruncSatUFloat32ToInt64(), f32),
    makeUnary(module, BinaryenTruncSatSFloat64ToInt32(), f64),
    makeUnary(module, BinaryenTruncSatSFloat64ToInt64(), f64),
    makeUnary(module, BinaryenTruncSatUFloat64ToInt32(), f64),
    makeUnary(module, BinaryenTruncSatUFloat64ToInt64(), f64),
    makeUnary(module, BinaryenReinterpretFloat32(), f32),
    makeUnary(module, BinaryenReinterpretFloat64(), f64),
    makeUnary(module, BinaryenConvertSInt32ToFloat32(), i32),
    makeUnary(module, BinaryenConvertSInt32ToFloat64(), i32),
    makeUnary(module, BinaryenConvertUInt32ToFloat32(), i32),
    makeUnary(module, BinaryenConvertUInt32ToFloat64(), i32),
    makeUnary(module, BinaryenConvertSInt64ToFloat32(), i64),
    makeUnary(module, BinaryenConvertSInt64ToFloat64(), i64),
    makeUnary(module, BinaryenConvertUInt64ToFloat32(), i64),
    makeUnary(module, BinaryenConvertUInt64ToFloat64(), i64),
    makeUnary(module, BinaryenPromoteFloat32(), f32),
    makeUnary(module, BinaryenDemoteFloat64(), f64),
    makeUnary(module, BinaryenReinterpretInt32(), i32),
    makeUnary(module, BinaryenReinterpretInt64(), i64),
    makeUnary(module, BinaryenSplatVecI8x16(), i32),
    makeUnary(module, BinaryenSplatVecI16x8(), i32),
    makeUnary(module, BinaryenSplatVecI32x4(), i32),
    makeUnary(module, BinaryenSplatVecI64x2(), i64),
    makeUnary(module, BinaryenSplatVecF32x4(), f32),
    makeUnary(module, BinaryenSplatVecF64x2(), f64),
    makeUnary(module, BinaryenNotVec128(), v128),
    makeUnary(module, BinaryenAnyTrueVec128(), v128),
    makeUnary(module, BinaryenPopcntVecI8x16(), v128),
    makeUnary(module, BinaryenAbsVecI8x16(), v128),
    makeUnary(module, BinaryenNegVecI8x16(), v128),
    makeUnary(module, BinaryenAllTrueVecI8x16(), v128),
    makeUnary(module, BinaryenBitmaskVecI8x16(), v128),
    makeUnary(module, BinaryenAbsVecI16x8(), v128),
    makeUnary(module, BinaryenNegVecI16x8(), v128),
    makeUnary(module, BinaryenAllTrueVecI16x8(), v128),
    makeUnary(module, BinaryenBitmaskVecI16x8(), v128),
    makeUnary(module, BinaryenAbsVecI32x4(), v128),
    makeUnary(module, BinaryenNegVecI32x4(), v128),
    makeUnary(module, BinaryenAllTrueVecI32x4(), v128),
    makeUnary(module, BinaryenBitmaskVecI32x4(), v128),
    makeUnary(module, BinaryenAbsVecI64x2(), v128),
    makeUnary(module, BinaryenNegVecI64x2(), v128),
    makeUnary(module, BinaryenAllTrueVecI64x2(), v128),
    makeUnary(module, BinaryenBitmaskVecI64x2(), v128),
    makeUnary(module, BinaryenAbsVecF32x4(), v128),
    makeUnary(module, BinaryenNegVecF32x4(), v128),
    makeUnary(module, BinaryenSqrtVecF32x4(), v128),
    makeUnary(module, BinaryenAbsVecF64x2(), v128),
    makeUnary(module, BinaryenNegVecF64x2(), v128),
    makeUnary(module, BinaryenSqrtVecF64x2(), v128),
    makeUnary(module, BinaryenTruncSatSVecF32x4ToVecI32x4(), v128),
    makeUnary(module, BinaryenTruncSatUVecF32x4ToVecI32x4(), v128),
    makeUnary(module, BinaryenConvertSVecI32x4ToVecF32x4(), v128),
    makeUnary(module, BinaryenConvertUVecI32x4ToVecF32x4(), v128),
    makeUnary(module, BinaryenExtendLowSVecI8x16ToVecI16x8(), v128),
    makeUnary(module, BinaryenExtendHighSVecI8x16ToVecI16x8(), v128),
    makeUnary(module, BinaryenExtendLowUVecI8x16ToVecI16x8(), v128),
    makeUnary(module, BinaryenExtendHighUVecI8x16ToVecI16x8(), v128),
    makeUnary(module, BinaryenExtendLowSVecI16x8ToVecI32x4(), v128),
    makeUnary(module, BinaryenExtendHighSVecI16x8ToVecI32x4(), v128),
    makeUnary(module, BinaryenExtendLowUVecI16x8ToVecI32x4(), v128),
    makeUnary(module, BinaryenExtendHighUVecI16x8ToVecI32x4(), v128),
    makeUnary(module, BinaryenExtendLowSVecI32x4ToVecI64x2(), v128),
    makeUnary(module, BinaryenExtendHighSVecI32x4ToVecI64x2(), v128),
    makeUnary(module, BinaryenExtendLowUVecI32x4ToVecI64x2(), v128),
    makeUnary(module, BinaryenExtendHighUVecI32x4ToVecI64x2(), v128),
    makeUnary(module, BinaryenConvertLowSVecI32x4ToVecF64x2(), v128),
    makeUnary(module, BinaryenConvertLowUVecI32x4ToVecF64x2(), v128),
    makeUnary(module, BinaryenTruncSatZeroSVecF64x2ToVecI32x4(), v128),
    makeUnary(module, BinaryenTruncSatZeroUVecF64x2ToVecI32x4(), v128),
    makeUnary(module, BinaryenDemoteZeroVecF64x2ToVecF32x4(), v128),
    makeUnary(module, BinaryenPromoteLowVecF32x4ToVecF64x2(), v128),
    makeUnary(module, BinaryenRelaxedTruncSVecF32x4ToVecI32x4(), v128),
    makeUnary(module, BinaryenRelaxedTruncUVecF32x4ToVecI32x4(), v128),
    makeUnary(module, BinaryenRelaxedTruncZeroSVecF64x2ToVecI32x4(), v128),
    makeUnary(module, BinaryenRelaxedTruncZeroUVecF64x2ToVecI32x4(), v128),
    // Binary
    makeBinary(module, BinaryenAddInt32(), i32),
    makeBinary(module, BinaryenSubFloat64(), f64),
    makeBinary(module, BinaryenDivSInt32(), i32),
    makeBinary(module, BinaryenDivUInt64(), i64),
    makeBinary(module, BinaryenRemSInt64(), i64),
    makeBinary(module, BinaryenRemUInt32(), i32),
    makeBinary(module, BinaryenAndInt32(), i32),
    makeBinary(module, BinaryenOrInt64(), i64),
    makeBinary(module, BinaryenXorInt32(), i32),
    makeBinary(module, BinaryenShlInt64(), i64),
    makeBinary(module, BinaryenShrUInt64(), i64),
    makeBinary(module, BinaryenShrSInt32(), i32),
    makeBinary(module, BinaryenRotLInt32(), i32),
    makeBinary(module, BinaryenRotRInt64(), i64),
    makeBinary(module, BinaryenDivFloat32(), f32),
    makeBinary(module, BinaryenCopySignFloat64(), f64),
    makeBinary(module, BinaryenMinFloat32(), f32),
    makeBinary(module, BinaryenMaxFloat64(), f64),
    makeBinary(module, BinaryenEqInt32(), i32),
    makeBinary(module, BinaryenNeFloat32(), f32),
    makeBinary(module, BinaryenLtSInt32(), i32),
    makeBinary(module, BinaryenLtUInt64(), i64),
    makeBinary(module, BinaryenLeSInt64(), i64),
    makeBinary(module, BinaryenLeUInt32(), i32),
    makeBinary(module, BinaryenGtSInt64(), i64),
    makeBinary(module, BinaryenGtUInt32(), i32),
    makeBinary(module, BinaryenGeSInt32(), i32),
    makeBinary(module, BinaryenGeUInt64(), i64),
    makeBinary(module, BinaryenLtFloat32(), f32),
    makeBinary(module, BinaryenLeFloat64(), f64),
    makeBinary(module, BinaryenGtFloat64(), f64),
    makeBinary(module, BinaryenGeFloat32(), f32),
    makeBinary(module, BinaryenEqVecI8x16(), v128),
    makeBinary(module, BinaryenNeVecI8x16(), v128),
    makeBinary(module, BinaryenLtSVecI8x16(), v128),
    makeBinary(module, BinaryenLtUVecI8x16(), v128),
    makeBinary(module, BinaryenGtSVecI8x16(), v128),
    makeBinary(module, BinaryenGtUVecI8x16(), v128),
    makeBinary(module, BinaryenLeSVecI8x16(), v128),
    makeBinary(module, BinaryenLeUVecI8x16(), v128),
    makeBinary(module, BinaryenGeSVecI8x16(), v128),
    makeBinary(module, BinaryenGeUVecI8x16(), v128),
    makeBinary(module, BinaryenEqVecI16x8(), v128),
    makeBinary(module, BinaryenNeVecI16x8(), v128),
    makeBinary(module, BinaryenLtSVecI16x8(), v128),
    makeBinary(module, BinaryenLtUVecI16x8(), v128),
    makeBinary(module, BinaryenGtSVecI16x8(), v128),
    makeBinary(module, BinaryenGtUVecI16x8(), v128),
    makeBinary(module, BinaryenLeSVecI16x8(), v128),
    makeBinary(module, BinaryenLeUVecI16x8(), v128),
    makeBinary(module, BinaryenGeSVecI16x8(), v128),
    makeBinary(module, BinaryenGeUVecI16x8(), v128),
    makeBinary(module, BinaryenEqVecI32x4(), v128),
    makeBinary(module, BinaryenNeVecI32x4(), v128),
    makeBinary(module, BinaryenLtSVecI32x4(), v128),
    makeBinary(module, BinaryenLtUVecI32x4(), v128),
    makeBinary(module, BinaryenGtSVecI32x4(), v128),
    makeBinary(module, BinaryenGtUVecI32x4(), v128),
    makeBinary(module, BinaryenLeSVecI32x4(), v128),
    makeBinary(module, BinaryenLeUVecI32x4(), v128),
    makeBinary(module, BinaryenGeSVecI32x4(), v128),
    makeBinary(module, BinaryenGeUVecI32x4(), v128),
    makeBinary(module, BinaryenEqVecI64x2(), v128),
    makeBinary(module, BinaryenNeVecI64x2(), v128),
    makeBinary(module, BinaryenLtSVecI64x2(), v128),
    makeBinary(module, BinaryenGtSVecI64x2(), v128),
    makeBinary(module, BinaryenLeSVecI64x2(), v128),
    makeBinary(module, BinaryenGeSVecI64x2(), v128),
    makeBinary(module, BinaryenEqVecF32x4(), v128),
    makeBinary(module, BinaryenNeVecF32x4(), v128),
    makeBinary(module, BinaryenLtVecF32x4(), v128)