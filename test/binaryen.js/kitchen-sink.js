// kitchen sink, tests the full API

function cleanInfo(info) {
  var ret = {};
  for (var x in info) {
    if (x !== 'value') {
      ret[x] = info[x];
    }
  }
  return ret;
}

var module;

// helpers

var v128_bytes = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16];

function makeInt32(x) {
  return module.i32.const(x);
}

function makeFloat32(x) {
  return module.f32.const(x);
}

function makeInt64(l, h) {
  return module.i64.const(l, h);
}

function makeFloat64(x) {
  return module.f64.const(x);
}

function makeVec128(i8s) {
  return module.v128.const(i8s)
}

function makeSomething() {
  return makeInt32(1337);
}

function makeDroppedInt32(x) {
  return module.drop(module.i32.const(x));
}

// tests

function test_types() {
  console.log("  // BinaryenTypeNone: " + binaryen.none);
  console.log("  //", binaryen.expandType(binaryen.none).join(","));

  console.log("  // BinaryenTypeUnreachable: " + binaryen.unreachable);
  console.log("  //", binaryen.expandType(binaryen.unreachable).join(","));

  console.log("  // BinaryenTypeInt32: " + binaryen.i32);
  console.log("  //", binaryen.expandType(binaryen.i32).join(","));

  console.log("  // BinaryenTypeInt64: " + binaryen.i64);
  console.log("  //", binaryen.expandType(binaryen.i64).join(","));

  console.log("  // BinaryenTypeFloat32: " + binaryen.f32);
  console.log("  //", binaryen.expandType(binaryen.f32).join(","));

  console.log("  // BinaryenTypeFloat64: " + binaryen.f64);
  console.log("  //", binaryen.expandType(binaryen.f64).join(","));

  console.log("  // BinaryenTypeVec128: " + binaryen.v128);
  console.log("  //", binaryen.expandType(binaryen.v128).join(","));

  console.log("  // BinaryenTypeAuto: " + binaryen.auto);

  var i32_pair = binaryen.createType([binaryen.i32, binaryen.i32]);
  console.log("  //", binaryen.expandType(i32_pair).join(","));

  var duplicate_pair = binaryen.createType([binaryen.i32, binaryen.i32]);
  console.log("  //", binaryen.expandType(duplicate_pair).join(","));

  assert(i32_pair == duplicate_pair);

  var f32_pair = binaryen.createType([binaryen.f32, binaryen.f32]);
  console.log("  //", binaryen.expandType(f32_pair).join(","));
}

function test_features() {
  console.log("Features.MVP: " + binaryen.Features.MVP);
  console.log("Features.Atomics: " + binaryen.Features.Atomics);
  console.log("Features.BulkMemory: " + binaryen.Features.BulkMemory);
  console.log("Features.MutableGlobals: " + binaryen.Features.MutableGlobals);
  console.log("Features.NontrappingFPToInt: " + binaryen.Features.NontrappingFPToInt);
  console.log("Features.SignExt: " + binaryen.Features.SignExt);
  console.log("Features.SIMD128: " + binaryen.Features.SIMD128);
  console.log("Features.ExceptionHandling: " + binaryen.Features.ExceptionHandling);
  console.log("Features.TailCall: " + binaryen.Features.TailCall);
  console.log("Features.ReferenceTypes: " + binaryen.Features.ReferenceTypes);
  console.log("Features.Multivalue: " + binaryen.Features.Multivalue);
  console.log("Features.GC: " + binaryen.Features.GC);
  console.log("Features.Memory64: " + binaryen.Features.Memory64);
  console.log("Features.RelaxedSIMD: " + binaryen.Features.RelaxedSIMD);
  console.log("Features.ExtendedConst: " + binaryen.Features.ExtendedConst);
  console.log("Features.Strings: " + binaryen.Features.Strings);
  console.log("Features.MultiMemories: " + binaryen.Features.MultiMemories);
  console.log("Features.All: " + binaryen.Features.All);
}

function test_ids() {
  console.log("InvalidId: " + binaryen.InvalidId);
  console.log("BlockId: " + binaryen.BlockId);
  console.log("IfId: " + binaryen.IfId);
  console.log("LoopId: " + binaryen.LoopId);
  console.log("BreakId: " + binaryen.BreakId);
  console.log("SwitchId: " + binaryen.SwitchId);
  console.log("CallId: " + binaryen.CallId);
  console.log("CallIndirectId: " + binaryen.CallIndirectId);
  console.log("LocalGetId: " + binaryen.LocalGetId);
  console.log("LocalSetId: " + binaryen.LocalSetId);
  console.log("GlobalGetId: " + binaryen.GlobalGetId);
  console.log("GlobalSetId: " + binaryen.GlobalSetId);
  console.log("LoadId: " + binaryen.LoadId);
  console.log("StoreId: " + binaryen.StoreId);
  console.log("ConstId: " + binaryen.ConstId);
  console.log("UnaryId: " + binaryen.UnaryId);
  console.log("BinaryId: " + binaryen.BinaryId);
  console.log("SelectId: " + binaryen.SelectId);
  console.log("DropId: " + binaryen.DropId);
  console.log("ReturnId: " + binaryen.ReturnId);
  console.log("MemorySizeId: " + binaryen.MemorySizeId);
  console.log("MemoryGrowId: " + binaryen.MemoryGrowId);
  console.log("NopId: " + binaryen.NopId);
  console.log("UnreachableId: " + binaryen.UnreachableId);
  console.log("AtomicCmpxchgId: " + binaryen.AtomicCmpxchgId);
  console.log("AtomicRMWId: " + binaryen.AtomicRMWId);
  console.log("AtomicWaitId: " + binaryen.AtomicWaitId);
  console.log("AtomicNotifyId: " + binaryen.AtomicNotifyId);
  console.log("SIMDExtractId: " + binaryen.SIMDExtractId);
  console.log("SIMDReplaceId: " + binaryen.SIMDReplaceId);
  console.log("SIMDShuffleId: " + binaryen.SIMDShuffleId);
  console.log("SIMDTernaryId: " + binaryen.SIMDTernaryId);
  console.log("SIMDShiftId: " + binaryen.SIMDShiftId);
  console.log("SIMDLoadId: " + binaryen.SIMDLoadId);
  console.log("SIMDLoadStoreLaneId: " + binaryen.SIMDLoadStoreLaneId);
  console.log("MemoryInitId: " + binaryen.MemoryInitId);
  console.log("DataDropId: " + binaryen.DataDropId);
  console.log("MemoryCopyId: " + binaryen.MemoryCopyId);
  console.log("MemoryFillId: " + binaryen.MemoryFillId);
  console.log("PopId: " + binaryen.PopId);
  console.log("RefNullId: " + binaryen.RefNullId);
  console.log("RefIsNullId: " + binaryen.RefIsNullId);
  console.log("RefFuncId: " + binaryen.RefFuncId);
  console.log("RefEqId: " + binaryen.RefEqId);
  console.log("TableGetId: " + binaryen.TableGetId);
  console.log("TableSetId: " + binaryen.TableSetId);
  console.log("TableSizeId: " + binaryen.TableSizeId);
  console.log("TableGrowId: " + binaryen.TableGrowId);
  console.log("TryId: " + binaryen.TryId);
  console.log("ThrowId: " + binaryen.ThrowId);
  console.log("RethrowId: " + binaryen.RethrowId);
  console.log("TupleMakeId: " + binaryen.TupleMakeId);
  console.log("TupleExtractId: " + binaryen.TupleExtractId);
  console.log("I31NewId: " + binaryen.I31NewId);
  console.log("I31GetId: " + binaryen.I31GetId);
  console.log("CallRefId: " + binaryen.CallRefId);
  console.log("RefTestId: " + binaryen.RefTestId);
  console.log("RefCastId: " + binaryen.RefCastId);
  console.log("BrOnId: " + binaryen.BrOnId);
  console.log("StructNewId: " + binaryen.StructNewId);
  console.log("StructGetId: " + binaryen.StructGetId);
  console.log("StructSetId: " + binaryen.StructSetId);
  console.log("ArrayNewId: " + binaryen.ArrayNewId);
  console.log("ArrayNewFixedId: " + binaryen.ArrayNewFixedId);
  console.log("ArrayGetId: " + binaryen.ArrayGetId);
  console.log("ArraySetId: " + binaryen.ArraySetId);
  console.log("ArrayLenId: " + binaryen.ArrayLenId);
  console.log("ArrayCopy: " + binaryen.ArrayCopyId);
  console.log("RefAs: " + binaryen.RefAsId);
  console.log("StringNew: " + binaryen.StringNewId);
  console.log("StringConst: " + binaryen.StringConstId);
  console.log("StringMeasure: " + binaryen.StringMeasureId);
  console.log("StringEncode: " + binaryen.StringEncodeId);
  console.log("StringConcat: " + binaryen.StringConcatId);
  console.log("StringEq: " + binaryen.StringEqId);
  console.log("StringAs: " + binaryen.StringAsId);
  console.log("StringWTF8Advance: " + binaryen.StringWTF8AdvanceId);
  console.log("StringWTF16Get: " + binaryen.StringWTF16GetId);
  console.log("StringIterNext: " + binaryen.StringIterNextId);
  console.log("StringIterMove: " + binaryen.StringIterMoveId);
  console.log("StringSliceWTF: " + binaryen.StringSliceWTFId);
  console.log("StringSliceIter: " + binaryen.StringSliceIterId);
}

function test_core() {

  // Module creation

  module = new binaryen.Module();
  // Memory
  module.setMemory(1, 256, "mem", [
    {
      passive: false,
      offset: module.i32.const(10),
      data: "hello, world".split('').map(function(x) { return x.charCodeAt(0) })
    },
    {
      passive: true,
      offset: null,
      data: "I am passive".split('').map(function(x) { return x.charCodeAt(0) })
    }
  ], true);

  // Create a tag
  var tag = module.addTag("a-tag", binaryen.i32, binaryen.none);

  // Literals and consts

  var constI32 = module.i32.const(1),
      constI64 = module.i64.const(2),
      constF32 = module.f32.const(3.14),
      constF64 = module.f64.const(2.1828),
      constF32Bits = module.f32.const_bits(0xffff1234),
      constF64Bits = module.f64.const_bits(0x5678abcd, 0xffff1234);

  var iIfF = binaryen.createType([binaryen.i32, binaryen.i64, binaryen.f32, binaryen.f64])

  var temp1 = makeInt32(1), temp2 = makeInt32(2), temp3 = makeInt32(3),
      temp4 = makeInt32(4), temp5 = makeInt32(5),
      temp6 = makeInt32(0), temp7 = makeInt32(1),
      temp8 = makeInt32(0), temp9 = makeInt32(1),
      temp10 = makeInt32(1), temp11 = makeInt32(3), temp12 = makeInt32(5),
      temp13 = makeInt32(10), temp14 = makeInt32(11),
      temp15 = makeInt32(110), temp16 = makeInt64(111);

  var valueList = [
    // Unary
    module.i32.clz(module.i32.const(-10)),
    module.i64.ctz(module.i64.const(-22, -1)),
    module.i32.popcnt(module.i32.const(-10)),
    module.f32.neg(module.f32.const(-33.612)),
    module.f64.abs(module.f64.const(-9005.841)),
    module.f32.ceil(module.f32.const(-33.612)),
    module.f64.floor(module.f64.const(-9005.841)),
    module.f32.trunc(module.f32.const(-33.612)),
    module.f32.nearest(module.f32.const(-33.612)),
    module.f64.sqrt(module.f64.const(-9005.841)),
    module.i32.eqz(module.i32.const(-10)),
    module.i64.extend_s(module.i32.const(-10)),
    module.i64.extend_u(module.i32.const(-10)),
    module.i32.wrap(module.i64.const(-22, -1)),
    module.i32.trunc_s.f32(module.f32.const(-33.612)),
    module.i64.trunc_s.f32(module.f32.const(-33.612)),
    module.i32.trunc_u.f32(module.f32.const(-33.612)),
    module.i64.trunc_u.f32(module.f32.const(-33.612)),
    module.i32.trunc_s.f64(module.f64.const(-9005.841)),
    module.i64.trunc_s.f64(module.f64.const(-9005.841)),
    module.i32.trunc_u.f64(module.f64.const(-9005.841)),
    module.i64.trunc_u.f64(module.f64.const(-9005.841)),
    module.i32.trunc_s_sat.f32(module.f32.const(-33.612)),
    module.i64.trunc_s_sat.f32(module.f32.const(-33.612)),
    module.i32.trunc_u_sat.f32(module.f32.const(-33.612)),
    module.i64.trunc_u_sat.f32(module.f32.const(-33.612)),
    module.i32.trunc_s_sat.f64(module.f64.const(-9005.841)),
    module.i64.trunc_s_sat.f64(module.f64.const(-9005.841)),
    module.i32.trunc_u_sat.f64(module.f64.const(-9005.841)),
    module.i64.trunc_u_sat.f64(module.f64.const(-9005.841)),
    module.i32.reinterpret(module.f32.const(-33.612)),
    module.i64.reinterpret(module.f64.const(-9005.841)),
    module.f32.convert_s.i32(module.i32.const(-10)),
    module.f64.convert_s.i32(module.i32.const(-10)),
    module.f32.convert_u.i32(module.i32.const(-10)),
    module.f64.convert_u.i32(module.i32.const(-10)),
    module.f32.convert_s.i64(module.i64.const(-22, -1)),
    module.f64.convert_s.i64(module.i64.const(-22, -1)),
    module.f32.convert_u.i64(module.i64.const(-22, -1)),
    module.f64.convert_u.i64(module.i64.const(-22, -1)),
    module.f64.promote(module.f32.const(-33.612)),
    module.f32.demote(module.f64.const(-9005.841)),
    module.f32.reinterpret(module.i32.const(-10)),
    module.f64.reinterpret(module.i64.const(-22, -1)),
    module.i8x16.splat(module.i32.const(42)),
    module.i16x8.splat(module.i32.const(42)),
    module.i32x4.splat(module.i32.const(42)),
    module.i64x2.splat(module.i64.const(123, 456)),
    module.f32x4.splat(module.f32.const(42.0)),
    module.f64x2.splat(module.f64.const(42.0)),
    module.v128.not(module.v128.const(v128_bytes)),
    module.v128.any_true(module.v128.const(v128_bytes)),
    module.i8x16.popcnt(module.v128.const(v128_bytes)),
    module.i8x16.abs(module.v128.const(v128_bytes)),
    module.i8x16.neg(module.v128.const(v128_bytes)),
    module.i8x16.all_true(module.v128.const(v128_bytes)),
    module.i8x16.bitmask(module.v128.const(v128_bytes)),
    module.i16x8.abs(module.v128.const(v128_bytes)),
    module.i16x8.neg(module.v128.const(v128_bytes)),
    module.i16x8.all_true(module.v128.const(v128_bytes)),
    module.i16x8.bitmask(module.v128.const(v128_bytes)),
    module.i16x8.extadd_pairwise_i8x16_s(module.v128.const(v128_bytes)),
    module.i16x8.extadd_pairwise_i8x16_u(module.v128.const(v128_bytes)),
    module.i32x4.abs(module.v128.const(v128_bytes)),
    module.i32x4.neg(module.v128.const(v128_bytes)),
    module.i32x4.all_true(module.v128.const(v128_bytes)),
    module.i32x4.bitmask(module.v128.const(v128_bytes)),
    module.i32x4.extadd_pairwise_i16x8_s(module.v128.const(v128_bytes)),
    module.i32x4.extadd_pairwise_i16x8_u(module.v128.const(v128_bytes)),
    module.i64x2.abs(module.v128.const(v128_bytes)),
    module.i64x2.neg(module.v128.const(v128_bytes)),
    module.i64x2.all_true(module.v128.const(v128_bytes)),
    module.i64x2.bitmask(module.v128.const(v128_bytes)),
    module.f32x4.abs(module.v128.const(v128_bytes)),
    module.f32x4.neg(module.v128.const(v128_bytes)),
    module.f32x4.sqrt(module.v128.const(v128_bytes)),
    module.f64x2.abs(module.v128.const(v128_bytes)),
    module.f64x2.neg(module.v128.const(v128_bytes)),
    module.f64x2.sqrt(module.v128.const(v128_bytes)),
    module.f64x2.convert_low_i32x4_s(module.v128.const(v128_bytes)),
    module.f64x2.convert_low_i32x4_u(module.v128.const(v128_bytes)),
    module.f64x2.promote_low_f32x4(module.v128.const(v128_bytes)),
    module.i32x4.trunc_sat_f32x4_s(module.v128.const(v128_bytes)),
    module.i32x4.trunc_sat_f32x4_u(module.v128.const(v128_bytes)),
    module.f32x4.convert_i32x4_s(module.v128.const(v128_bytes)),
    module.f32x4.convert_i32x4_u(module.v128.const(v128_bytes)),
    module.f32x4.demote_f64x2_zero(module.v128.const(v128_bytes)),
    module.i16x8.extend_low_i8x16_s(module.v128.const(v128_bytes)),
    module.i16x8.extend_high_i8x16_s(module.v128.const(v128_bytes)),
    module.i16x8.extend_low_i8x16_u(module.v128.const(v128_bytes)),
    module.i16x8.extend_high_i8x16_u(module.v128.const(v128_bytes)),
    module.i32x4.extend_low_i16x8_s(module.v128.const(v128_bytes)),
    module.i32x4.extend_high_i16x8_s(module.v128.const(v128_bytes)),
    module.i32x4.extend_low_i16x8_u(module.v128.const(v128_bytes)),
    module.i32x4.extend_high_i16x8_u(module.v128.const(v128_bytes)),
    module.i32x4.trunc_sat_f64x2_s_zero(module.v128.const(v128_bytes)),
    module.i32x4.trunc_sat_f64x2_u_zero(module.v128.const(v128_bytes)),
    module.i64x2.extend_low_i32x4_s(module.v128.const(v128_bytes)),
 