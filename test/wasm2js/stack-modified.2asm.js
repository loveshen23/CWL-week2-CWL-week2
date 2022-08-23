import * as env from 'env';

function asmFunc(imports) {
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
 var env = imports.env;
 var setTempRet0 = env.setTempRet0;
 var i64toi32_i32$HIGH_BITS = 0;
 function $0(var$0, var$0$hi) {
  var$0 = var$0 | 0;
  var$0$hi = var$0$hi | 0;
  var i64toi32_i32$0 = 0, i64toi32_i32$2 = 0, i64toi32_i32$3 = 0, var$1$hi = 0, i64toi32_i32$5 = 0, var$1 = 0, var$2$hi = 0, i64toi32_i32$1 = 0, var$2 = 0;
  i64toi32_i32$0 = var$0$hi;
  var$1 = var$0;
  var$1$hi = i64toi32_i32$0;
  i64toi32_i32$0 = 0;
  var$2 = 1;
  var$2$hi = i64toi32_i32$0;
  label$1 : {
   label$2 : while (1) {
    i64toi32_i32$0 = var$1$hi;
    i64toi32_i32$2 = var$1;
    i64toi32_i32$1 = 0;
    i64toi32_i32$3 = 0;
    if ((i64toi32_i32$2 | 0) == (i64toi32_i32$3 | 0) & (i64toi32_i32$0 | 0) == (i64toi32_i32$1 | 0) | 0) {
     break label$1
    } else {
     label$5 : {
      i64toi32_i32$2 = var$1$hi;
      i64toi32_i32$2 = var$2$hi;
      i64toi32_i32$2 = var$1$hi;
      i64toi32_i32$0 = var$2$hi;
      i64toi32_i32$0 = __wasm_i64_mul(var$1 | 0, i64toi32_i32$2 | 0, var$2 | 0, i64toi32_i32$0 | 0) | 0;
      i64toi32_i32$2 = i64toi32_i32$HIGH_BITS;
      var$2 = i64toi32_i32$0;
      var$2$hi = i64toi32_i32$2;
      i64toi32_i32$2 = var$1$hi;
      i64toi32_i32$3 = var$1;
      i64toi32_i32$0 = 0;
      i64toi32_i32$1 = 1;
      i64toi32_i32$5 = (i64toi32_i32$3 >>> 0 < i64toi32_i32$1 >>> 0) + i64toi32_i32$0 | 0;
      i64toi32_i32$5 = i64toi32_i32$2 - i64toi32_i32$5 | 0;
      var$1 = i64toi32_i32$3 - i64toi32_i32$1 | 0;
      var$1$hi = i64toi32_i32$5;
     }
    }
    continue label$2;
   };
  }
  i64toi32_i32$5 = var$2$hi;
  i64toi32_i32$3 = var$2;
  i64toi32_i32$HIGH_BITS = i64toi32_i32$5;
  return i64toi32_i32$3 | 0;
 }
 
 function $1(var$0, var$0$hi) {
  var$0 = var$0 | 0;
  var$0$hi = var$0$hi | 0;
  var i64toi32_i32$0 = 0, i64toi32_i32$2 = 0, i64toi32_i32$3 = 0, var$1$hi = 0, i64toi32_i32$5 = 0, var$1 = 0, var$2$hi = 0, i64toi32_i32$1 = 0, var$2 = 0;
  i64toi32_i32$0 = var$0$hi;
  var$1 = var$0;
  var$1$hi = i64toi32_i32$0;
  i64toi32_i32$0 = 0;
  var$2 = 1;
  var$2$hi = i64toi32_i32$0;
  label$1 : {
   label$2 : while (1) {
    i64toi32_i32$0 = var$1$hi;
    i64toi32_i32$2 = var$1;
    i64toi32_i32$1 = 0;
    i64toi32_i32$3 = 0;
    if ((i64toi32_i32$2 | 0) == (i64toi32_i32$3 | 0) & (i64toi32_i32$0 | 0) == (i64toi32_i32$1 | 0) | 0) {
     break label$1
    } else {
     i64toi32_i32$2 = var$1$hi;
     i64toi32_i32$2 = var$2$hi;
     i64toi32_i32$2 = var$1$hi;
     i64toi32_i32$0 = var$2$hi;
     i64toi32_i32$0 = __wasm_i64_mul(var$1 | 0, i64toi32_i32$2 | 0, var$2 | 0, i64toi32_i32$0 | 0) | 0;
     i64toi32_i32$2 = i64toi32_i32$HIGH_BITS;
     var$2 = i64toi32_i32$0;
     var$2$hi = i64toi32_i32$2;
     i64toi32_i32$2 = var$1$hi;
     i64toi32_i32$3 = var$1;
     i64toi32_i32$0 = 0;
     i64toi32_i32$1 = 1;
     i64toi32_i32$5 = (i64toi32_i32$3 >>> 0 < i64toi32_i32$1 >>> 0) + i64toi32_i32$0 | 0;
     i64toi32_i32$5 = i64toi32_i32$2 - i64toi32_i32$5 | 0;
     var$1 = i64toi32_i32$3 - i64toi32_i32$1 | 0;
     var$1$hi = i64toi32_i32$5;
    }
    continue label$2;
   };
  }
  i64toi32_i32$5 = var$2$hi;
  i64toi32_i32$3 = var$2;
  i64toi32_i32$HIGH_BITS = i64toi32_i32$5;
  return i64toi32_i32$3 | 0;
 }
 
 function $2(var$0, var$0$hi) {
  var$0 = var$0 | 0;
  var$0$hi = var$0$hi | 0;
  var i64toi32_i32$0 = 0, i64toi32_i32$2 = 0, i64toi32_i32$3 = 0, var$1$hi = 0, i64toi32_i32$5 = 0, var$1 = 0, var$2$hi = 0, i64toi32_i32$1 = 0, var$2 = 0;
  i64toi32_i32$0 = var$0$hi;
  var$1 = var$0;
  var$1$hi = i64toi32_i32$0;
  i64toi32_i32$0 = 0;
  var$2 = 1;
  var$2$hi = i64toi32_i32$0;
  label$1 : {
   label$2 : while (1) {
    i64toi32_i32$0 = var$1$hi;
    i64toi32_i32$2 = var$1;
    i64toi32_i32$1 = 0;
    i64toi32_i32$3 = 0;
    if ((i64toi32_i32$2 | 0) == (i64toi32_i32$3 | 0) & (i64toi32_i32$0 | 0) == (i64toi32_i32$1 | 0) | 0) {
     break label$1
    } else {
     i64toi32_i32$2 = var$1$hi;
     i64toi32_i32$2 = var$2$hi;
     i64toi32_i32$2 = var$1$hi;
     i64toi32_i32$0 = var$2$hi;
     i64toi32_i32$0 = __wasm_i64_mul(var$1 | 0, i64toi32_i32$2 | 0, var$2 | 0, i64toi32_i32$0 | 0) | 0;
     i64toi32_i32$2 = i64toi32_i32$HIGH_BITS;
     var$2 = i64toi32_i32$0;
     var$2$hi = i64toi32_i32$2;
     i64toi32_i32$2 = var$1$hi;
     i64toi32_i32$3 = var$1;
     i64toi32_i32$0 = 0;
     i64toi32_i32$1 = 1;
     i64toi32_i32$5 = (i64toi32_i32$3 >>> 0 < i64toi32_i32$1 >>> 0) + i64toi32_i32$0 | 0;
     i64toi32_i32$5 = i64toi32_i32$2 - i64toi32_i32$5 | 0;
     var$1 = i64toi32_i32$3 - i64toi32_i32$1 | 0;
     var$1$hi = i64toi32_i32$5;
    }
    continue label$2;
   };
  }
  i64toi32_i32$5 = var$2$hi;
  i64toi32_i32$3 = var$2;
  i64toi32_i32$HIGH_BITS = i64toi32_i32$5;
  return i64toi32_i32$3 | 0;
 }
 
 function $3(var$0, var$0$hi) {
  var$0 = var$0 | 0;
  var$0$hi = var$0$hi | 0;
  var i64toi32_i32$0 = 0, i64toi32_i32$2 = 0, i64toi32_i32$3 = 0, var$1$hi = 0, i64toi32_i32$5 = 0, var$1 = 0, var$2$hi = 0, i64toi32_i32$1 = 0, var$2 = 0;
  i64toi32_i32$0 = var$0$hi;
  var$1 = var$0;
  var$1$hi = i64toi32_i32$0;
  i64toi32_i32$0 = 0;
  var$2 = 1;
  var$2$hi = i64toi32_i32$0;
  label$1 : {
   label$2 : while (1) {
    i64toi32_i32$0 = var$1$hi;
    i64toi32_i32$2 = var$1;
    i64toi32_i32$1 = 0;
    i64toi32_i32$3 = 0;
    if ((i64toi32_i32$2 | 0) == (i64toi32_i32$3 | 0) & (i64toi32_i32$0 | 0) == (i64toi32_i32$1 | 0) | 0) {
     break label$1
    } else {
     i64toi32_i32$2 = var$1$hi;
     i64toi32_i32$2 = var$2$hi;
     i64toi32_i32$2 = var$1$hi;
     i64toi32_i32$0 = var$2$hi;
     i64toi32_i32$0 = __wasm_i64_mul(var$1 | 0, i64toi32_i32$2 | 0, var$2 | 0, i64toi32_i32$0 | 0) | 0;
     i64toi32_i32$2 = i64toi32_i32$HIGH_BITS;
     var$2 = i64toi32_i32$0;
     var$2$hi = i64toi32_i32$2;
     i64toi32_i32$2 = var$1$hi;
     i64toi32_i32$3 = var$1;
     i64toi32_i32$0 = 0;
     i64toi32_i32$1 = 1;
     i64toi32_i32$5 = (i64toi32_i32$3 >>> 0 < i64toi32_i32$1 >>> 0) + i64toi32_i32$0 | 0;
     i64toi32_i32$5 = i64toi32_i32$2 - i64toi32_i32$5 | 0;
     var$1 = i64toi32_i32$3 - i64toi32_i32$1 | 0;
     var$1$hi = i64toi32_i32$5;
    }
    continue label$2;
   };
  }
  i64toi32_i32$5 = var$2$hi;
  i64toi32_i32$3 = var$2;
  i64toi32_i32$HIGH_BITS = i64toi32_i32$5;
  return i64toi32_i32$3 | 0;
 }
 
 function $4(var$0, var$0$hi) {
  var$0 = var$0 | 0;
  var$0$hi = var$0$hi | 0;
  var i64toi32_i32$0 = 0, i64toi32_i32$2 = 0, i64toi32_i32$3 = 0, var$1$hi = 0, i64toi32_i32$5 = 0, var$1 = 0, var$2$hi = 0, i64toi32_i32$1 = 0, var$2 = 0;
  i64toi32_i32$0 = var$0$hi;
  var$1 = var$0;
  var$1$hi = i64toi32_i32$0;
  i64toi32_i32$0 = 0;
  var$2 = 1;
  var$2$hi = i64toi32_i32$0;
  label$1 : {
   label$2 : while (1) {
    i64toi32_i32$0 = var$1$hi;
    i64toi32_i32$2 = var$1;
    i64toi32_i32$1 = 0;
    i64toi32_i32$3 = 0;
    if ((i64toi32_i32$2 | 0) == (i64toi32_i32$3 | 0) & (i64toi32_i32$0 | 0) == (i64toi32_i32$1 | 0) | 0) {
     break label$1
    } else {
     i64toi32_i32$2 = var$1$hi;
     i64toi32_i32$2 = var$2$hi;
     i64toi32_i32$2 = var$1$hi;
     i64toi32_i32$0 = var$2$hi;
     i64toi32_i32$0 = __wasm_i64_mul(var$1 | 0, i64toi32_i32$2 | 0, var$2 | 0, i64toi32_i32$0 | 0) | 0;
     i64toi32_i32$2 = i64toi32_i32$HIGH_BITS;
     var$2 = i64toi32_i32$0;
     var$2$hi = i64toi32_i32$2;
     i64toi32_i32$2 = va