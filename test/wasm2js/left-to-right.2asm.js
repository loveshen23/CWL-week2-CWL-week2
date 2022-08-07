
  var bufferView;

  var scratchBuffer = new ArrayBuffer(16);
  var i32ScratchView = new Int32Array(scratchBuffer);
  var f32ScratchView = new Float32Array(scratchBuffer);
  var f64ScratchView = new Float64Array(scratchBuffer);
  
  function wasm2js_scratch_load_i32(index) {
    return i32ScratchView[index];
  }
      
  function wasm2js_scratch_store_i32(index, value) {
    i32ScratchView[index] = value;
  }
      
  function wasm2js_scratch_load_f64() {
    return f64ScratchView[0];
  }
      
  function wasm2js_scratch_store_f64(value) {
    f64ScratchView[0] = value;
  }
      
  function wasm2js_scratch_store_f32(value) {
    f32ScratchView[2] = value;
  }
      
function asmFunc(imports) {
 var buffer = new ArrayBuffer(65536);
 var HEAP8 = new Int8Array(buffer);
 var HEAP16 = new Int16Array(buffer);
 var HEAP32 = new Int32Array(buffer);
 var HEAPU8 = new Uint8Array(buffer);
 var HEAPU16 = new Uint16Array(buffer);
 var HEAPU32 = new Uint32Array(buffer);
 var HEAPF32 = new Float32Array(buffer);
 var HEAPF64 = new Float64Array(buffer);
 var Math_imul = Math.imul;
 var Math_fround = Math.fround;
 var Math_abs = Math.abs;
 var Math_clz32 = Math.clz32;
 var Math_min = Math.min;
 var Math_max = Math.max;
 var Math_floor = Math.floor;
 var Math_ceil = Math.ceil;
 var Math_trunc = Math.trunc;
 var Math_sqrt = Math.sqrt;
 var __wasm_intrinsics_temp_i64 = 0;
 var __wasm_intrinsics_temp_i64$hi = 0;
 var i64toi32_i32$HIGH_BITS = 0;
 function i32_t0($0, $1) {
  $0 = $0 | 0;
  $1 = $1 | 0;
  return -1 | 0;
 }
 
 function i32_t1($0, $1) {
  $0 = $0 | 0;
  $1 = $1 | 0;
  return -2 | 0;
 }
 
 function i64_t0($0, $0$hi, $1, $1$hi) {
  $0 = $0 | 0;
  $0$hi = $0$hi | 0;
  $1 = $1 | 0;
  $1$hi = $1$hi | 0;
  return -1 | 0;
 }
 
 function i64_t1($0, $0$hi, $1, $1$hi) {
  $0 = $0 | 0;
  $0$hi = $0$hi | 0;
  $1 = $1 | 0;
  $1$hi = $1$hi | 0;
  return -2 | 0;
 }
 
 function f32_t0($0, $1) {
  $0 = Math_fround($0);
  $1 = Math_fround($1);
  return -1 | 0;
 }
 
 function f32_t1($0, $1) {
  $0 = Math_fround($0);
  $1 = Math_fround($1);
  return -2 | 0;
 }
 
 function f64_t0($0, $1) {
  $0 = +$0;
  $1 = +$1;
  return -1 | 0;
 }
 
 function f64_t1($0, $1) {
  $0 = +$0;
  $1 = +$1;
  return -2 | 0;
 }
 
 function reset() {
  HEAP32[8 >> 2] = 0;
 }
 
 function bump() {
  HEAP8[11 >> 0] = HEAPU8[10 >> 0] | 0;
  HEAP8[10 >> 0] = HEAPU8[9 >> 0] | 0;
  HEAP8[9 >> 0] = HEAPU8[8 >> 0] | 0;
  HEAP8[8 >> 0] = -3;
 }
 
 function get() {
  return HEAP32[8 >> 2] | 0 | 0;
 }
 
 function i32_left() {
  bump();
  HEAP8[8 >> 0] = 1;
  return 0 | 0;
 }
 
 function i32_right() {
  bump();
  HEAP8[8 >> 0] = 2;
  return 1 | 0;
 }
 
 function i32_callee() {
  bump();
  HEAP8[8 >> 0] = 4;
  return 0 | 0;
 }
 
 function i32_bool() {
  bump();
  HEAP8[8 >> 0] = 5;
  return 0 | 0;
 }
 
 function i64_left() {
  var i64toi32_i32$0 = 0;
  bump();
  HEAP8[8 >> 0] = 1;
  i64toi32_i32$0 = 0;
  i64toi32_i32$HIGH_BITS = i64toi32_i32$0;
  return 0 | 0;
 }
 
 function i64_right() {
  var i64toi32_i32$0 = 0;
  bump();
  HEAP8[8 >> 0] = 2;
  i64toi32_i32$0 = 0;
  i64toi32_i32$HIGH_BITS = i64toi32_i32$0;
  return 1 | 0;
 }
 
 function i64_callee() {
  bump();
  HEAP8[8 >> 0] = 4;
  return 2 | 0;
 }
 
 function i64_bool() {
  bump();
  HEAP8[8 >> 0] = 5;
  return 0 | 0;
 }
 
 function f32_left() {
  bump();
  HEAP8[8 >> 0] = 1;
  return Math_fround(Math_fround(0.0));
 }
 
 function f32_right() {
  bump();
  HEAP8[8 >> 0] = 2;
  return Math_fround(Math_fround(1.0));
 }
 
 function f32_callee() {
  bump();
  HEAP8[8 >> 0] = 4;
  return 4 | 0;
 }
 
 function f32_bool() {
  bump();
  HEAP8[8 >> 0] = 5;
  return 0 | 0;
 }
 
 function f64_left() {
  bump();
  HEAP8[8 >> 0] = 1;
  return +(0.0);
 }
 
 function f64_right() {
  bump();
  HEAP8[8 >> 0] = 2;
  return +(1.0);
 }
 
 function f64_callee() {
  bump();
  HEAP8[8 >> 0] = 4;
  return 6 | 0;
 }
 
 function f64_bool() {
  bump();
  HEAP8[8 >> 0] = 5;
  return 0 | 0;
 }
 
 function i32_dummy($0, $1) {
  $0 = $0 | 0;
  $1 = $1 | 0;
 }
 
 function i64_dummy($0, $0$hi, $1, $1$hi) {
  $0 = $0 | 0;
  $0$hi = $0$hi | 0;
  $1 = $1 | 0;
  $1$hi = $1$hi | 0;
 }
 
 function f32_dummy($0, $1) {
  $0 = Math_fround($0);
  $1 = Math_fround($1);
 }
 
 function f64_dummy($0, $1) {
  $0 = +$0;
  $1 = +$1;
 }
 
 function $35() {
  reset();
  i32_left() | 0;
  i32_right() | 0;
  return get() | 0 | 0;
 }
 
 function $36() {
  reset();
  i32_left() | 0;
  i32_right() | 0;
  return get() | 0 | 0;
 }
 
 function $37() {
  reset();
  i32_left() | 0;
  i32_right() | 0;
  return get() | 0 | 0;
 }
 
 function $38() {
  reset();
  (i32_left() | 0 | 0) / (i32_right() | 0 | 0) | 0;
  return get() | 0 | 0;
 }
 
 function $39() {
  reset();
  ((i32_left() | 0) >>> 0) / ((i32_right() | 0) >>> 0) | 0;
  return get() | 0 | 0;
 }
 
 function $40() {
  reset();
  (i32_left() | 0 | 0) % (i32_right() | 0 | 0) | 0;
  return get() | 0 | 0;
 }
 
 function $41() {
  reset();
  ((i32_left() | 0) >>> 0) % ((i32_right() | 0) >>> 0) | 0;
  return get() | 0 | 0;
 }
 
 function $42() {
  reset();
  i32_left() | 0;
  i32_right() | 0;
  return get() | 0 | 0;
 }
 
 function $43() {
  reset();
  i32_left() | 0;
  i32_right() | 0;
  return get() | 0 | 0;
 }
 
 function $44() {
  reset();
  i32_left() | 0;
  i32_right() | 0;
  return get() | 0 | 0;
 }
 
 function $45() {
  reset();
  i32_left() | 0;
  i32_right() | 0;
  return get() | 0 | 0;
 }
 
 function $46() {
  reset();
  i32_left() | 0;
  i32_right() | 0;
  return get() | 0 | 0;
 }
 
 function $47() {
  reset();
  i32_left() | 0;
  i32_right() | 0;
  return get() | 0 | 0;
 }
 
 function $48() {
  reset();
  i32_left() | 0;
  i32_right() | 0;
  return get() | 0 | 0;
 }
 
 function $49() {
  reset();
  i32_left() | 0;
  i32_right() | 0;
  return get() | 0 | 0;
 }
 
 function $50() {
  reset();
  i32_left() | 0;
  i32_right() | 0;
  return get() | 0 | 0;
 }
 
 function $51() {
  reset();
  i32_left() | 0;
  i32_right() | 0;
  return get() | 0 | 0;
 }
 
 function $52() {
  reset();
  i32_left() | 0;
  i32_right() | 0;
  return get() | 0 | 0;
 }
 
 function $53() {
  reset();
  i32_left() | 0;
  i32_right() | 0;
  return get() | 0 | 0;
 }
 
 function $54() {
  reset();
  i32_left() | 0;
  i32_right() | 0;
  return get() | 0 | 0;
 }
 
 function $55() {
  reset();
  i32_left() | 0;
  i32_right() | 0;
  return get() | 0 | 0;
 }
 
 function $56() {
  reset();
  i32_left() | 0;
  i32_right() | 0;
  return get() | 0 | 0;
 }
 
 function $57() {
  reset();
  i32_left() | 0;
  i32_right() | 0;
  return get() | 0 | 0;
 }
 
 function $58() {
  var wasm2js_i32$0 = 0, wasm2js_i32$1 = 0;
  reset();
  (wasm2js_i32$0 = i32_left() | 0, wasm2js_i32$1 = i32_right() | 0), HEAP32[wasm2js_i32$0 >> 2] = wasm2js_i32$1;
  return get() | 0 | 0;
 }
 
 function $59() {
  var wasm2js_i32$0 = 0, wasm2js_i32$1 = 0;
  reset();
  (wasm2js_i32$0 = i32_left() | 0, wasm2js_i32$1 = i32_right() | 0), HEAP8[wasm2js_i32$0 >> 0] = wasm2js_i32$1;
  return get() | 0 | 0;
 }
 
 function $60() {
  var wasm2js_i32$0 = 0, wasm2js_i32$1 = 0;
  reset();
  (wasm2js_i32$0 = i32_left() | 0, wasm2js_i32$1 = i32_right() | 0), HEAP16[wasm2js_i32$0 >> 1] = wasm2js_i32$1;
  return get() | 0 | 0;
 }
 
 function $61() {
  reset();
  i32_dummy(i32_left() | 0 | 0, i32_right() | 0 | 0);
  return get() | 0 | 0;
 }
 
 function $62() {
  var wasm2js_i32$0 = 0, wasm2js_i32$1 = 0, wasm2js_i32$2 = 0;
  reset();
  ((wasm2js_i32$1 = i32_left() | 0, wasm2js_i32$2 = i32_right() | 0), wasm2js_i32$0 = i32_callee() | 0 | 0), FUNCTION_TABLE[wasm2js_i32$0](wasm2js_i32$1 | 0, wasm2js_i32$2 | 0) | 0;
  return get() | 0 | 0;
 }
 
 function $63() {
  reset();
  i32_left() | 0;
  i32_right() | 0;
  i32_bool() | 0;
  return get() | 0 | 0;
 }
 
 function $64() {
  var i64toi32_i32$0 = 0, i64toi32_i32$1 = 0, i64toi32_i32$3 = 0, i64toi32_i32$4 = 0, i64toi32_i32$5 = 0, $0 = 0, $0$hi = 0, $1 = 0, $1$hi = 0;
  reset();
  i64toi32_i32$0 = i64_left() | 0;
  i64toi32_i32$1 = i64toi32_i32$HIGH_BITS;
  $0 = i64toi32_i32$0;
  $0$hi = i64toi32_i32$1;
  i64toi32_i32$1 = i64_right() | 0;
  i64toi32_i32$0 = i64toi32_i32$HIGH_BITS;
  $1 = i64toi32_i32$1;
  $1$hi = i64toi32_i32$0;
  i64toi32_i32$0 = $0$hi;
  i64toi32_i32$1 = $1$hi;
  i64toi32_i32$3 = $1;
  i64toi32_i32$4 = $0 + i64toi32_i32$3 | 0;
  i64toi32_i32$5 = i64toi32_i32$0 + i64toi32_i32$1 | 0;
  if (i64toi32_i32$4 >>> 0 < i64toi32_i32$3 >>> 0) {
   i64toi32_i32$5 = i64toi32_i32$5 + 1 | 0
  }
  return get() | 0 | 0;
 }
 
 function $65() {
  var i64toi32_i32$0 = 0, i64toi32_i32$1 = 0, i64toi32_i32$2 = 0, i64toi32_i32$3 = 0, i64toi32_i32$5 = 0, $0 = 0, $0$hi = 0, $1 = 0, $1$hi = 0;
  reset();
  i64toi32_i32$0 = i64_left() | 0;
  i64toi32_i32$1 = i64toi32_i32$HIGH_BITS;
  $0 = i64toi32_i32$0;
  $0$hi = i64toi32_i32$1;
  i64toi32_i32$1 = i64_right() | 0;
  i64toi32_i32$0 = i64toi32_i32$HIGH_BITS;
  $1 = i64toi32_i32$1;
  $1$hi = i64toi32_i32$0;
  i64toi32_i32$0 = $0$hi;
  i64toi32_i32$2 = $0;
  i64toi32_i32$1 = $1$hi;
  i64toi32_i32$3 = $1;
  i64toi32_i32$5 = (i64toi32_i32$2 >>> 0 < i64toi32_i32$3 >>> 0) + i64toi32_i32$1 | 0;
  i64toi32_i32$5 = i64toi32_i32$0 - i64toi32_i32$5 | 0;
  return get() | 0 | 0;
 }
 
 function $66() {
  var i64toi32_i32$1 = 0, i64toi32_i32$0 = 0, $0 = 0, $0$hi = 0, $1 = 0, $1$hi = 0;
  reset();
  i64toi32_i32$0 = i64_left() | 0;
  i64toi32_i32$1 = i64toi32_i32$HIGH_BITS;
  $0 = i64toi32_i32$0;
  $0$hi = i64toi32_i32$1;
  i64toi32_i32$1 = i64_right() | 0;
  i64toi32_i32$0 = i64toi32_i32$HIGH_BITS;
  $1 = i64toi32_i32$1;
  $1$hi = i64toi32_i32$0;
  i64toi32_i32$0 = $0$hi;
  i64toi32_i32$1 = $1$hi;
  i64toi32_i32$1 = __wasm_i64_mul($0 | 0, i64toi32_i32$0 | 0, $1 | 0, i64toi32_i32$1 | 0) | 0;
  i64toi32_i32$0 = i64toi32_i32$HIGH_BITS;
  return get() | 0 | 0;
 }
 
 function $67() {
  var i64toi32_i32$1 = 0, i64toi32_i32$0 = 0, $0 = 0, $0$hi = 0, $1 = 0, $1$hi = 0;
  reset();
  i64toi32_i32$0 = i64_left() | 0;
  i64toi32_i32$1 = i64toi32_i32$HIGH_BITS;
  $0 = i64toi32_i32$0;
  $0$hi = i64toi32_i32$1;
  i64toi32_i32$1 = i64_right() | 0;
  i64toi32_i32$0 = i64toi32_i32$HIGH_BITS;
  $1 = i64toi32_i32$1;
  $1$hi = i64toi32_i32$0;
  i64toi32_i32$0 = $0$hi;
  i64toi32_i32$1 = $1$hi;
  i64toi32_i32$1 = __wasm_i64_sdiv($0 | 0, i64toi32_i32$0 | 0, $1 | 0, i64toi32_i32$1 | 0) | 0;
  i64toi32_i32$0 = i64toi32_i32$HIGH_BITS;
  return get() | 0 | 0;
 }
 
 function $68() {
  var i64toi32_i32$1 = 0, i64toi32_i32$0 = 0, $0 = 0, $0$hi = 0, $1 = 0, $1$hi = 0;
  reset();
  i64toi32_i32$0 = i64_left() | 0;
  i64toi32_i32$1 = i64toi32_i32$HIGH_BITS;
  $0 = i64toi32_i32$0;
  $0$hi = i64toi32_i32$1;
  i64toi32_i32$1 = i64_right() | 0;
  i64toi32_i32$0 = i64toi32_i32$HIGH_BITS;
  $1 = i64toi32_i32$1;
  $1$hi = i64toi32_i32$0;
  i64toi32_i32$0 = $0$hi;
  i64toi32_i32$1 = $1$hi;
  i64toi32_i32$1 = __wasm_i64_udiv($0 | 0, i64toi32_i32$0 | 0, $1 | 0, i64toi32_i32$1 | 0) | 0;
  i64toi32_i32$0 = i64toi32_i32$HIGH_BITS;
  return get() | 0 | 0;
 }
 
 function $69() {
  var i64toi32_i32$1 = 0, i64toi32_i32$0 = 0, $0 = 0, $0$hi = 0, $1 = 0, $1$hi = 0;
  reset();
  i64toi32_i32$0 = i64_left() | 0;
  i64toi32_i32$1 = i64toi32_i32$HIGH_BITS;
  $0 = i64toi32_i32$0;
  $0$hi = i64toi32_i32$1;
  i64toi32_i32$1 = i64_right() | 0;
  i64toi32_i32$0 = i64toi32_i32$HIGH_BITS;
  $1 = i64toi32_i32$1;
  $1$hi = i64toi32_i32$0;
  i64toi32_i32$0 = $0$hi;
  i64toi32_i32$1 = $1$hi;
  i64toi32_i32$1 = __wasm_i64_srem($0 | 0, i64toi32_i32$0 | 0, $1 | 0, i64toi32_i32$1 | 0) | 0;
  i64toi32_i32$0 = i64toi32_i32$HIGH_BITS;
  return get() | 0 | 0;
 }
 
 function $70() {
  var i64toi32_i32$1 = 0, i64toi32_i32$0 = 0, $0 = 0, $0$hi = 0, $1 = 0, $1$hi = 0;
  reset();
  i64toi32_i32$0 = i64_left() | 0;
  i64toi32_i32$1 = i64toi32_i32$HIGH_BITS;
  $0 = i64toi32_i32$0;
  $0$hi = i64toi32_i32$1;
  i64toi32_i32$1 = i64_right() | 0;
  i64toi32_i32$0 = i64toi32_i32$HIGH_BITS;
  $1 = i64toi32_i32$1;
  $1$hi = i64toi32_i32$0;
  i64toi32_i32$0 = $0$hi;
  i64toi32_i32$1 = $1$hi;
  i64toi32_i32$1 = __wasm_i64_urem($0 | 0, i64toi32_i32$0 | 0, $1 | 0, i6