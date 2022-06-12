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
 function dummy() {
  
 }
 
 function $1() {
  
 }
 
 function $2() {
  
 }
 
 function $3() {
  
 }
 
 function $4() {
  
 }
 
 function $5() {
  var $0 = 0;
  block : {
   $0 = 1;
   break block;
  }
  return $0 | 0;
 }
 
 function $6() {
  var i64toi32_i32$0 = 0, $0 = 0, $0$hi = 0;
  block : {
   i64toi32_i32$0 = 0;
   $0 = 2;
   $0$hi = i64toi32_i32$0;
   break block;
  }
  i64toi32_i32$0 = $0$hi;
  i64toi32_i32$HIGH_BITS = i64toi32_i32$0;
  return $0 | 0;
 }
 
 function $7() {
  var $0 = Math_fround(0);
  block : {
   $0 = Math_fround(3.0);
   break block;
  }
  return Math_fround($0);
 }
 
 function $8() {
  var $0 = 0.0;
  block : {
   $0 = 4.0;
   break block;
  }
  return +$0;
 }
 
 function $9() {
  
 }
 
 function $10() {
  block : {
   dummy();
   break block;
  }
 }
 
 function $11() {
  block : {
   dummy();
   break block;
  }
 }
 
 function $12() {
  var $0 = 0;
  block : {
   dummy();
   $0 = 2;
   break block;
  }
  return $0 | 0;
 }
 
 function $13() {
  var $0 = 0, $1_1 = 0, $3_1 = 0;
  block : {
   loop_in : while (1) {
    $0 = 3;
    break block;
   };
  }
  return $0 | 0;
 }
 
 function $14() {
  var $0 = 0, $1_1 = 0, $3_1 = 0;
  block : {
   loop_in : while (1) {
    dummy();
    $0 = 4;
    break block;
   };
  }
  return $0 | 0;
 }
 
 function $15() {
  var $0 = 0;
  block : {
   loop_in : while (1) {
    dummy();
    $0 = 5;
    break block;
   };
  }
  return $0 | 0;
 }
 
 function $16() {
  var $0 = 0;
  block : {
   $0 = 9;
   break block;
  }
  return $0 | 0;
 }
 
 function $17() {
  
 }
 
 function $18() {
  var $0 = 0;
  block : {
   $0 = 8;
   break block;
  }
  return $0 | 0;
 }
 
 function $19() {
  var $0 = 0;
  block : {
   $0 = 9;
   break block;
  }
  return $0 | 0;
 }
 
 function $20() {
  
 }
 
 function $21() {
  var $0 = 0;
  block : {
   $0 = 10;
   break block;
  }
  return $0 | 0;
 }
 
 function $22() {
  var $0 = 0;
  block : {
   $0 = 11;
   break block;
  }
  return $0 | 0;
 }
 
 function $23() {
  var i64toi32_i32$0 = 0, $0 = 0, $0$hi = 0;
  block : {
   i64toi32_i32$0 = 0;
   $0 = 7;
   $0$hi = i64toi32_i32$0;
   break block;
  }
  i64toi32_i32$0 = $0$hi;
  i64toi32_i32$HIGH_BITS = i64toi32_i32$0;
  return $0 | 0;
 }
 
 function $24() {
  var $0 = 0, $5_1 = 0;
  if_ : {
   $0 = 2;
   break if_;
  }
  return $0 | 0;
 }
 
 function $25($0, $1_1) {
  $0 = $0 | 0;
  $1_1 = $1_1 | 0;
  var $3_1 = 0, $7_1 = 0;
  block : {
   if ($0) {
    $3_1 = 3;
    break block;
   } else {
    $7_1 = $1_1
   }
   $3_1 = $7_1;
  }
  return $3_1 | 0;
 }
 
 function $26($0, $1_1) {
  $0 = $0 | 0;
  $1_1 = $1_1 | 0;
  var $6_1 = 0, $7_1 = 0;
  block : {
   if ($0) {
    $7_1 = $1_1
   } else {
    $6_1 = 4;
    break block;
   }
   $6_1 = $7_1;
  }
  return $6_1 | 0;
 }
 
 function $27($0, $1_1) {
  $0 = $0 | 0;
  $1_1 = $1_1 | 0;
  var $2_1 = 0, $3_1 = 0, $4_1 = 0;
  block : {
   $2_1 = 5;
   break block;
  }
  return $2_1 | 0;
 }
 
 function $28($0, $1_1) {
  $0 = $0 | 0;
  $1_1 = $1_1 | 0;
  var $2_1 = 0, $3_1 = 0, $4_1 = 0;
  block : {
   $2_1 = $0;
   $3_1 = 6;
   break block;
  }
  return $3_1 | 0;
 }
 
 function $29() {
  var $0 = 0;
  block : {
   $0 = 7;
   break block;
  }
  return $0 | 0;
 }
 
 function f($0, $1_1, $2_1) {
  $0 = $0 | 0;
  $1_1 = $1_1 | 0;
  $2_1 = $2_1 | 0;
  return -1 | 0;
 }
 
 function $31() {
  var $0 = 0;
  block : {
   $0 = 12;
   break block;
  }
  return $0 | 0;
 }
 
 function $32() {
  var $0 = 0;
  block : {
   $0 = 13;
   break block;
  }
  return $0 | 0;
 }
 
 function $33() {
  var $0 = 0;
  block : {
   $0 = 14;
   break block;
  }
  return $0 | 0;
 }
 
 function $34() {
  var $0 = 0;
  block : {
   $0 = 20;
   break block;
  }
  return $0 | 0;
 }
 
 function $35() {
  var $0 = 0;
  block : {
   $0 = 21;
   break block;
  }
  return $0 | 0;
 }
 
 function $36() {
  var $0 = 0;
  block : {
   $0 = 22;
   break block;
  }
  return $0 | 0;
 }
 
 function $37() {
  var $0 = 0;
  block : {
   $0 = 23;
   break block;
  }
  return $0 | 0;
 }
 
 function $38() {
  var $1_1 = 0;
  block : {
   $1_1 = 17;
   break block;
  }
  return $1_1 | 0;
 }
 
 function $39() {
  var $1_1 = 0;
  block : {
   $1_1 = 1;
   break block;
  }
  return $1_1 | 0;
 }
 
 function $40() {
  var $0 = 0;
  block : {
   $0 = 1;
   break block;
  }
  return $0 | 0;
 }
 
 function $41() {
  var $0 = Math_fround(0);
  block : {
   $0 = Math_fround(1.7000000476837158);
   break block;
  }
  return Math_fround($0);
 }
 
 function $42() {
  var i64toi32_i32$0 = 0, $0 = 0, $0$hi = 0;
  block : {
   i64toi32_i32$0 = 0;
   $0 = 30;
   $0$hi = i64toi32_i32$0;
   break block;
  }
  i64toi32_i32$0 = $0$hi;
  i64toi32_i32$HIGH_BITS = i64toi32_i32$0;
  return $0 | 0;
 }
 
 function $43() {
  var $0 = 0;
  block : {
   $0 = 30;
   break block;
  }
  return $0 | 0;
 }
 
 function $44() {
  var $0 = 0;
  block : {
   $0 = 31;
   break block;
  }
  return $0 | 0;
 }
 
 function $45() {
  var $0 = 0;
  block : {
   $0 = 32;
   break block;
  }
  return $0 | 0;
 }
 
 function $46() {
  var $0 = 0;
  block : {
   $0 = 33;
   break block;
  }
  return $0 | 0;
 }
 
 function $47() {
  var $0 = Math_fround(0);
  block : {
   $0 = Math_fround(3.4000000953674316);
   break block;
  }
  return Math_fround($0);
 }
 
 function $48() {
  var $0 = 0;
  block : {
   $0 = 3;
   break block;
  }
  return $0 | 0;
 }
 
 function $49() {
  var $0 = 0, $0$hi = 0, i64toi32_i32$1 = 0;
  block : {
   $0 = 45;
   $0$hi = 0;
   break block;
  }
  i64toi32_i32$1 = $0$hi;
  i64toi32_i32$HIGH_BITS = i64toi32_i32$1;
  return $0 | 0;
 }
 
 function $50() {
  var $0 = 0;
  block : {
   $0 = 44;
   break block;
  }
  return $0 | 0;
 }
 
 function $51() {
  var $0 = 0;
  block : {
   $0 = 43;
   break block;
  }
  return $0 | 0;
 }
 
 function $52() {
  var $0 = 0;
  block : {
   $0 = 42;
   break block;
  }
  return $0 | 0;
 }
 
 function $53() {
  var $0 = 0;
  block : {
   $0 = 41;
   break block;
  }
  return $0 | 0;
 }
 
 function $54() {
  var $0 = 0;
  block : {
   $0 = 40;
   break block;
  }
  return $0 | 0;
 }
 
 function $55() {
  var $0 = 0;
  block : {
   dummy();
   $0 = 8;
   break block;
  }
  return 1 + $0 | 0 | 0;
 }
 
 function $56() {
  var $0 = 0;
  block : {
   block0 : {
    $0 = 8;
    break block;
   }
  }
  return 1 + $0 | 0 | 0;
 }
 
 function $57() {
  var $0 = 0, $1_1 = 0;
  block : {
   $0 = 8;
   break block;
  }
  return 1 + $0 | 0 | 0;
 }
 
 function $58() {
  var $0 = 0;
  block : {
   $0 = 8;
   break block;
  }
  return 1 + $0 | 0 | 0;
 }
 
 function $59() {
  var $0 = 0;
  block : {
   $0 = 8;
   break block;
  }
  return 1 + $0 | 0 | 0;
 }
 
 function $60() {
  var $0 = 0;
  block : {
   $0 = 8;
   break block;
  }
  return 1 + $0 | 0 | 0;
 }
 
 function legalstub$6() {
  var i64toi32_i32$0 = 0, i64toi32_i32$4 = 0, i64toi32_i32$1 = 0, i64toi32_i32$3 = 0, $7_1 = 0, $0 = 0, $0$hi = 0, i64toi32_i32$2 = 0;
  i64toi32_i32$0 = $6() | 0;
  i64toi32_i32$1 = i64toi32_i32$HIGH_BITS;
  $0 = i64toi32_i32$0;
  $0$hi = i64toi32_i32$1;
  i64toi32_i32$2 = i64toi32_i32$0;
  i64toi32_i32$0 = 0;
  i64toi32_i32$3 = 32;
  i64toi32_i32$4 = i64toi32_i32$3 & 31 | 0;
  if (32 >>> 0 <= (i64toi32_i32$3 & 63 | 0) >>> 0) {
   i64toi32_i32$0 = 0;
   $7_1 = i64toi32_i32$1 >>> i64toi32_i32$4 | 0;
  } else {
   i64toi32_i32$0 = i64toi32_i32$1 >>> i64toi32_i32$4 | 0;
   $7_1 = (((1 << i64toi32_i32$4 | 0) - 1 | 0) & i64toi32_i32$1 | 0) << (32 - i64toi32_i32$4 | 0) | 0 | (i64toi32_i32$2 >>> i64toi32_i32$4 | 0) | 0;
  }
  setTempRet0($7_1 | 0);
  i64toi32_i32$0 = $0$hi;
  return $0 | 0;
 }
 
 function legalstub$23() {
  var i64toi32_i32$0 = 0, i64toi32_i32$4 = 0, i64toi32_i32$1 = 0, i64toi32_i32$3 = 0, $7_1 = 0, $0 = 0, $0$hi = 0, i64toi32_i32$2 = 0;
  i64toi32_i32$0 = $23() | 0;
  i64toi32_i32$1 = i64toi32_i32$HIGH_BITS;
  $0 = i64toi32_i32$0;
  $0$hi = i64toi32_i32$1;
  i64toi32_i32$2 = i64toi32_i32$0;
  i64toi32_i32$0 = 0;
  i64toi32_i32$3 = 32;
  i64toi32_i32$4 = i64toi32_i32$3 & 31 | 0;
  if (32 >>> 0 <= (i64toi32_i32$3 & 63 | 0) >>> 0) {
   i64toi32_i32$0 = 0;
   $7_1 = i64toi32_i32$1 >>> i64toi32_i32$4 | 0;
  } else {
   i64toi32_i32$0 = i64toi32_i32$1 >>> i64toi32_i32$4 | 0;
   $7_1 = (((1 << i64toi32_i32$4 | 0) - 1 | 0) & i64toi32_i32$1 | 0) << (32 - i64toi32_i32$4 | 0) | 0 | (i64toi32_i32$2 >>> i64toi32_i32$4 | 0) | 0;
  }
  setTempRet0($7_1 | 0);
  i64toi32_i32$0 = $0$hi;
  return $0 | 0;
 }
 
 function legalstub$42() {
  var i64toi32_i32$0 = 0, i64toi32_i32$4 = 0, i64toi32_i32$1 = 0, i64toi32_i32$3 = 0, $7_1 = 0, $0 = 0, $0$hi = 0, i64toi32_i32$2 = 0;
  i64toi32_i32$0 = $42() | 0;
  i64toi32_i32$1 = i64toi32_i32$HIGH_BITS;
  $0 = i64toi32_i32$0;
  $0$hi = i64toi32_i32$1;
  i64toi32_i32$2 = i64toi32_i32$0;
  i64toi32_i32$0 = 0;
  i64toi32_i32$3 = 32;
  i64toi32_i32$4 = i64toi32_i32$3 & 31 | 0;
  if (32 >>> 0 <= (i64toi32_i32$3 & 63 | 0) >>> 0) {
   i64toi32_i32$0 = 0;
   $7_1 = i64toi32_i32$1 >>> i64toi32_i32$4 | 0;
  } else {
   i64toi32_i32$0 = i64toi32_i32$1 >>> i64toi32_i32$4 | 0;
   $7_1 = (((1 << i64toi32_i32$4 | 0) - 1 | 0) & i64toi32_i32$1 | 0) << (32 - i64toi32_i32$4 | 0) | 0 | (i64toi32_i32$2 >>> i64toi32_i32$4 | 0) | 0;
  }
  setTempRet0($7_1 | 0);
  i64toi32_i32$0 = $0$hi;
  return $0 | 0;
 }
 
 function legalstub$49() {
  var i64toi32_i32$0 = 0, i64toi32_i32$4 = 0,