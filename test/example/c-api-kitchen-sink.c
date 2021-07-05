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

  Binarye