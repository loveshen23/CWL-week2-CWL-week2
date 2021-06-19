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
  console.log("Feat