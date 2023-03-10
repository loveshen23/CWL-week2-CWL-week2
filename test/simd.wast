
(module
 (memory 1 1)
 (func $v128.load (param $0 i32) (result v128)
  (v128.load offset=0 align=16
    (local.get $0)
  )
 )
 (func $v128.load8x8_s (param $0 i32) (result v128)
  (v128.load8x8_s
   (local.get $0)
  )
 )
 (func $v128.load8x8_u (param $0 i32) (result v128)
  (v128.load8x8_u
   (local.get $0)
  )
 )
 (func $v128.load16x4_s (param $0 i32) (result v128)
  (v128.load16x4_s
   (local.get $0)
  )
 )
 (func $v128.load16x4_u (param $0 i32) (result v128)
  (v128.load16x4_u
   (local.get $0)
  )
 )
 (func $v128.load32x2_s (param $0 i32) (result v128)
  (v128.load32x2_s
   (local.get $0)
  )
 )
 (func $v128.load32x2_u (param $0 i32) (result v128)
  (v128.load32x2_u
   (local.get $0)
  )
 )
 (func $v128.load8_splat (param $0 i32) (result v128)
  (v128.load8_splat
   (local.get $0)
  )
 )
 (func $v128.load16_splat (param $0 i32) (result v128)
  (v128.load16_splat
   (local.get $0)
  )
 )
 (func $v128.load32_splat (param $0 i32) (result v128)
  (v128.load32_splat
   (local.get $0)
  )
 )
 (func $v128.load64_splat (param $0 i32) (result v128)
  (v128.load64_splat
   (local.get $0)
  )
 )
 (func $v128.store (param $0 i32) (param $1 v128)
  (v128.store offset=0 align=16
   (local.get $0)
   (local.get $1)
  )
 )
 (func $v128.const.i8x16 (result v128)
  (v128.const i8x16 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16)
 )
 (func $v128.const.i16x8 (result v128)
  (v128.const i16x8 1 2 3 4 5 6 7 8)
 )
 (func $v128.const.i32x4 (result v128)
  (v128.const i32x4 1 2 3 4)
 )
 (func $v128.const.i64x2 (result v128)
  (v128.const i64x2 1 2)
 )
 (func $v128.const.f32x4 (result v128)
  (v128.const f32x4 1.0 2 3 4)
 )
 (func $v128.const.f64x2 (result v128)
  (v128.const f64x2 1.0 2)
 )
 (func $i8x16.shuffle (param $0 v128) (param $1 v128) (result v128)
  (i8x16.shuffle 0 17 2 19 4 21 6 23 8 25 10 27 12 29 14 31
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i8x16.swizzle (param $0 v128) (param $1 v128) (result v128)
  (i8x16.swizzle
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i8x16.splat (param $0 i32) (result v128)
  (i8x16.splat
   (local.get $0)
  )
 )
 (func $i16x8.splat (param $0 i32) (result v128)
  (i16x8.splat
   (local.get $0)
  )
 )
 (func $f32x4.splat (param $0 f32) (result v128)
  (f32x4.splat
   (local.get $0)
  )
 )
 (func $f64x2.splat (param $0 f64) (result v128)
  (f64x2.splat
   (local.get $0)
  )
 )
 (func $i8x16.extract_lane_s (param $0 v128) (result i32)
  (i8x16.extract_lane_s 0
   (local.get $0)
  )
 )
 (func $i8x16.extract_lane_u (param $0 v128) (result i32)
  (i8x16.extract_lane_u 0
   (local.get $0)
  )
 )
 (func $i8x16.replace_lane (param $0 v128) (param $1 i32) (result v128)
  (i8x16.replace_lane 0
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i16x8.extract_lane_s (param $0 v128) (result i32)
  (i16x8.extract_lane_s 0
   (local.get $0)
  )
 )
 (func $i16x8.extract_lane_u (param $0 v128) (result i32)
  (i16x8.extract_lane_u 0
   (local.get $0)
  )
 )
 (func $i16x8.replace_lane (param $0 v128) (param $1 i32) (result v128)
  (i16x8.replace_lane 0
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i32x4.extract_lane (param $0 v128) (result i32)
  (i32x4.extract_lane 0
   (local.get $0)
  )
 )
 (func $i32x4.replace_lane (param $0 v128) (param $1 i32) (result v128)
  (i32x4.replace_lane 0
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i64x2.extract_lane (param $0 v128) (result i64)
  (i64x2.extract_lane 0
   (local.get $0)
  )
 )
 (func $i64x2.replace_lane (param $0 v128) (param $1 i64) (result v128)
  (i64x2.replace_lane 0
   (local.get $0)
   (local.get $1)
  )
 )
 (func $f32x4.extract_lane (param $0 v128) (result f32)
  (f32x4.extract_lane 0
   (local.get $0)
  )
 )
 (func $f32x4.replace_lane (param $0 v128) (param $1 f32) (result v128)
  (f32x4.replace_lane 0
   (local.get $0)
   (local.get $1)
  )
 )
 (func $f64x2.extract_lane (param $0 v128) (result f64)
  (f64x2.extract_lane 0
   (local.get $0)
  )
 )
 (func $f64x2.replace_lane (param $0 v128) (param $1 f64) (result v128)
  (f64x2.replace_lane 0
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i8x16.eq (param $0 v128) (param $1 v128) (result v128)
  (i8x16.eq
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i8x16.ne (param $0 v128) (param $1 v128) (result v128)
  (i8x16.ne
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i8x16.lt_s (param $0 v128) (param $1 v128) (result v128)
  (i8x16.lt_s
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i8x16.lt_u (param $0 v128) (param $1 v128) (result v128)
  (i8x16.lt_u
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i8x16.gt_s (param $0 v128) (param $1 v128) (result v128)
  (i8x16.gt_s
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i8x16.gt_u (param $0 v128) (param $1 v128) (result v128)
  (i8x16.gt_u
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i8x16.le_s (param $0 v128) (param $1 v128) (result v128)
  (i8x16.le_s
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i8x16.le_u (param $0 v128) (param $1 v128) (result v128)
  (i8x16.le_u
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i8x16.ge_s (param $0 v128) (param $1 v128) (result v128)
  (i8x16.ge_s
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i8x16.ge_u (param $0 v128) (param $1 v128) (result v128)
  (i8x16.ge_u
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i16x8.eq (param $0 v128) (param $1 v128) (result v128)
  (i16x8.eq
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i16x8.ne (param $0 v128) (param $1 v128) (result v128)
  (i16x8.ne
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i16x8.lt_s (param $0 v128) (param $1 v128) (result v128)
  (i16x8.lt_s
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i16x8.lt_u (param $0 v128) (param $1 v128) (result v128)
  (i16x8.lt_u
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i16x8.gt_s (param $0 v128) (param $1 v128) (result v128)
  (i16x8.gt_s
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i16x8.gt_u (param $0 v128) (param $1 v128) (result v128)
  (i16x8.gt_u
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i16x8.le_s (param $0 v128) (param $1 v128) (result v128)
  (i16x8.le_s
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i16x8.le_u (param $0 v128) (param $1 v128) (result v128)
  (i16x8.le_u
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i16x8.ge_s (param $0 v128) (param $1 v128) (result v128)
  (i16x8.ge_s
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i16x8.ge_u (param $0 v128) (param $1 v128) (result v128)
  (i16x8.ge_u
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i32x4.eq (param $0 v128) (param $1 v128) (result v128)
  (i32x4.eq
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i32x4.ne (param $0 v128) (param $1 v128) (result v128)
  (i32x4.ne
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i32x4.lt_s (param $0 v128) (param $1 v128) (result v128)
  (i32x4.lt_s
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i32x4.lt_u (param $0 v128) (param $1 v128) (result v128)
  (i32x4.lt_u
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i32x4.gt_s (param $0 v128) (param $1 v128) (result v128)
  (i32x4.gt_s
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i32x4.gt_u (param $0 v128) (param $1 v128) (result v128)
  (i32x4.gt_u
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i32x4.le_s (param $0 v128) (param $1 v128) (result v128)
  (i32x4.le_s
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i32x4.le_u (param $0 v128) (param $1 v128) (result v128)
  (i32x4.le_u
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i32x4.ge_s (param $0 v128) (param $1 v128) (result v128)
  (i32x4.ge_s
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i32x4.ge_u (param $0 v128) (param $1 v128) (result v128)
  (i32x4.ge_u
   (local.get $0)
   (local.get $1)
  )
 )
 (func $f32x4.eq (param $0 v128) (param $1 v128) (result v128)
  (f32x4.eq
   (local.get $0)
   (local.get $1)
  )
 )
 (func $f32x4.ne (param $0 v128) (param $1 v128) (result v128)
  (f32x4.ne
   (local.get $0)
   (local.get $1)
  )
 )
 (func $f32x4.lt (param $0 v128) (param $1 v128) (result v128)
  (f32x4.lt
   (local.get $0)
   (local.get $1)
  )
 )
 (func $f32x4.gt (param $0 v128) (param $1 v128) (result v128)
  (f32x4.gt
   (local.get $0)
   (local.get $1)
  )
 )
 (func $f32x4.le (param $0 v128) (param $1 v128) (result v128)
  (f32x4.le
   (local.get $0)
   (local.get $1)
  )
 )
 (func $f32x4.ge (param $0 v128) (param $1 v128) (result v128)
  (f32x4.ge
   (local.get $0)
   (local.get $1)
  )
 )
 (func $f64x2.eq (param $0 v128) (param $1 v128) (result v128)
  (f64x2.eq
   (local.get $0)
   (local.get $1)
  )
 )
 (func $f64x2.ne (param $0 v128) (param $1 v128) (result v128)
  (f64x2.ne
   (local.get $0)
   (local.get $1)
  )
 )
 (func $f64x2.lt (param $0 v128) (param $1 v128) (result v128)
  (f64x2.lt
   (local.get $0)
   (local.get $1)
  )
 )
 (func $f64x2.gt (param $0 v128) (param $1 v128) (result v128)
  (f64x2.gt
   (local.get $0)
   (local.get $1)
  )
 )
 (func $f64x2.le (param $0 v128) (param $1 v128) (result v128)
  (f64x2.le
   (local.get $0)
   (local.get $1)
  )
 )
 (func $f64x2.ge (param $0 v128) (param $1 v128) (result v128)
  (f64x2.ge
   (local.get $0)
   (local.get $1)
  )
 )
 (func $v128.not (param $0 v128) (result v128)
  (v128.not
   (local.get $0)
  )
 )
 (func $v128.and (param $0 v128) (param $1 v128) (result v128)
  (v128.and
   (local.get $0)
   (local.get $1)
  )
 )
 (func $v128.andnot (param $0 v128) (param $1 v128) (result v128)
  (v128.andnot
   (local.get $0)
   (local.get $1)
  )
 )
 (func $v128.or (param $0 v128) (param $1 v128) (result v128)
  (v128.or
   (local.get $0)
   (local.get $1)
  )
 )
 (func $v128.xor (param $0 v128) (param $1 v128) (result v128)
  (v128.xor
   (local.get $0)
   (local.get $1)
  )
 )
 (func $v128.bitselect (param $0 v128) (param $1 v128) (param $2 v128) (result v128)
  (v128.bitselect
   (local.get $0)
   (local.get $1)
   (local.get $2)
  )
 )
 (func $v128.any_true (param $0 v128) (result i32)
  (v128.any_true
   (local.get $0)
  )
 )
 (func $v128.load8_lane (param $0 i32) (param $1 v128) (result v128)
  (v128.load8_lane 0
   (local.get $0)
   (local.get $1)
  )
 )
 (func $v128.load16_lane (param $0 i32) (param $1 v128) (result v128)
  (v128.load16_lane 0
   (local.get $0)
   (local.get $1)
  )
 )
 (func $v128.load32_lane (param $0 i32) (param $1 v128) (result v128)
  (v128.load32_lane 0
   (local.get $0)
   (local.get $1)
  )
 )
 (func $v128.load64_lane (param $0 i32) (param $1 v128) (result v128)
  (v128.load64_lane 0
   (local.get $0)
   (local.get $1)
  )
 )
 (func $v128.load64_lane_align (param $0 i32) (param $1 v128) (result v128)
  (v128.load64_lane align=1 0
   (local.get $0)
   (local.get $1)
  )
 )
 (func $v128.load64_lane_offset (param $0 i32) (param $1 v128) (result v128)
  (v128.load64_lane offset=32 0
   (local.get $0)
   (local.get $1)
  )
 )
 (func $v128.load64_lane_align_offset (param $0 i32) (param $1 v128) (result v128)
  (v128.load64_lane align=1 offset=32 0
   (local.get $0)
   (local.get $1)
  )
 )
 (func $v128.store8_lane (param $0 i32) (param $1 v128)
  (v128.store8_lane 0
   (local.get $0)
   (local.get $1)
  )
 )
 (func $v128.store16_lane (param $0 i32) (param $1 v128)
  (v128.store16_lane 0
   (local.get $0)
   (local.get $1)
  )
 )
 (func $v128.store32_lane (param $0 i32) (param $1 v128)
  (v128.store32_lane 0
   (local.get $0)
   (local.get $1)
  )
 )
 (func $v128.store64_lane (param $0 i32) (param $1 v128)
  (v128.store64_lane 0
   (local.get $0)
   (local.get $1)
  )
 )
 (func $v128.store64_lane_align (param $0 i32) (param $1 v128)
  (v128.store64_lane align=1 0
   (local.get $0)
   (local.get $1)
  )
 )
 (func $v128.store64_lane_offset (param $0 i32) (param $1 v128)
  (v128.store64_lane offset=32 0
   (local.get $0)
   (local.get $1)
  )
 )
 (func $v128.store64_lane_align_offset (param $0 i32) (param $1 v128)
  (v128.store64_lane align=1 offset=32 0
   (local.get $0)
   (local.get $1)
  )
 )
 (func $v128.load32_zero (param $0 i32) (result v128)
  (v128.load32_zero
   (local.get $0)
  )
 )
 (func $v128.load64_zero (param $0 i32) (result v128)
  (v128.load64_zero
   (local.get $0)
  )
 )
  (func $f32x4.demote_f64x2_zero (param $0 v128) (result v128)
  (f32x4.demote_f64x2_zero
   (local.get $0)
  )
 )
 (func $f64x2.promote_low_f32x4 (param $0 v128) (result v128)
  (f64x2.promote_low_f32x4
   (local.get $0)
  )
 )
 (func $i8x16.abs (param $0 v128) (result v128)
  (i8x16.abs
   (local.get $0)
  )
 )
 (func $i8x16.neg (param $0 v128) (result v128)
  (i8x16.neg
   (local.get $0)
  )
 )
 (func $i8x16.popcnt (param $0 v128) (result v128)
  (i8x16.popcnt
   (local.get $0)
  )
 )
 (func $i8x16.all_true (param $0 v128) (result i32)
  (i8x16.all_true
   (local.get $0)
  )
 )
 (func $i8x16.bitmask (param $0 v128) (result i32)
  (i8x16.bitmask
   (local.get $0)
  )
 )
 (func $i8x16.narrow_i16x8_s (param $0 v128) (param $1 v128) (result v128)
  (i8x16.narrow_i16x8_s
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i8x16.narrow_i16x8_u (param $0 v128) (param $1 v128) (result v128)
  (i8x16.narrow_i16x8_u
   (local.get $0)
   (local.get $1)
  )
 )
 (func $f32x4.ceil (param $0 v128) (result v128)
  (f32x4.ceil
   (local.get $0)
  )
 )
 (func $f32x4.floor (param $0 v128) (result v128)
  (f32x4.floor
   (local.get $0)
  )
 )
 (func $f32x4.trunc (param $0 v128) (result v128)
  (f32x4.trunc
   (local.get $0)
  )
 )
 (func $f32x4.nearest (param $0 v128) (result v128)
  (f32x4.nearest
   (local.get $0)
  )
 )
 (func $i8x16.shl (param $0 v128) (param $1 i32) (result v128)
  (i8x16.shl
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i8x16.shr_s (param $0 v128) (param $1 i32) (result v128)
  (i8x16.shr_s
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i8x16.shr_u (param $0 v128) (param $1 i32) (result v128)
  (i8x16.shr_u
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i8x16.add (param $0 v128) (param $1 v128) (result v128)
  (i8x16.add
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i8x16.add_sat_s (param $0 v128) (param $1 v128) (result v128)
  (i8x16.add_sat_s
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i8x16.add_sat_u (param $0 v128) (param $1 v128) (result v128)
  (i8x16.add_sat_u
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i8x16.sub (param $0 v128) (param $1 v128) (result v128)
  (i8x16.sub
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i8x16.sub_sat_s (param $0 v128) (param $1 v128) (result v128)
  (i8x16.sub_sat_s
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i8x16.sub_sat_u (param $0 v128) (param $1 v128) (result v128)
  (i8x16.sub_sat_u
   (local.get $0)
   (local.get $1)
  )
 )
 (func $f64x2.ceil (param $0 v128) (result v128)
  (f64x2.ceil
   (local.get $0)
  )
 )
 (func $f64x2.floor (param $0 v128) (result v128)
  (f64x2.floor
   (local.get $0)
  )
 )
 (func $i8x16.min_s (param $0 v128) (param $1 v128) (result v128)
  (i8x16.min_s
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i8x16.min_u (param $0 v128) (param $1 v128) (result v128)
  (i8x16.min_u
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i8x16.max_s (param $0 v128) (param $1 v128) (result v128)
  (i8x16.max_s
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i8x16.max_u (param $0 v128) (param $1 v128) (result v128)
  (i8x16.max_u
   (local.get $0)
   (local.get $1)
  )
 )
 (func $f64x2.trunc (param $0 v128) (result v128)
  (f64x2.trunc
   (local.get $0)
  )
 )
 (func $i8x16.avgr_u (param $0 v128) (param $1 v128) (result v128)
  (i8x16.avgr_u
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i16x8.extadd_pairwise_i8x16_s (param $0 v128) (result v128)
  (i16x8.extadd_pairwise_i8x16_s
   (local.get $0)
  )
 )
 (func $i16x8.extadd_pairwise_i8x16_u (param $0 v128) (result v128)
  (i16x8.extadd_pairwise_i8x16_u
   (local.get $0)
  )
 )
 (func $i32x4.extadd_pairwise_i16x8_s (param $0 v128) (result v128)
  (i32x4.extadd_pairwise_i16x8_s
   (local.get $0)
  )
 )
 (func $i32x4.extadd_pairwise_i16x8_u (param $0 v128) (result v128)
  (i32x4.extadd_pairwise_i16x8_u
   (local.get $0)
  )
 )
 (func $i16x8.abs (param $0 v128) (result v128)
  (i16x8.abs
   (local.get $0)
  )
 )
 (func $i16x8.neg (param $0 v128) (result v128)
  (i16x8.neg
   (local.get $0)
  )
 )
 (func $i16x8.q15mulr_sat_s (param $0 v128) (param $1 v128) (result v128)
  (i16x8.q15mulr_sat_s
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i16x8.all_true (param $0 v128) (result i32)
  (i16x8.all_true
   (local.get $0)
  )
 )
 (func $i16x8.bitmask (param $0 v128) (result i32)
  (i16x8.bitmask
   (local.get $0)
  )
 )
 (func $i16x8.narrow_i32x4_s (param $0 v128) (param $1 v128) (result v128)
  (i16x8.narrow_i32x4_s
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i16x8.narrow_i32x4_u (param $0 v128) (param $1 v128) (result v128)
  (i16x8.narrow_i32x4_u
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i16x8.extend_low_i8x16_s (param $0 v128) (result v128)
  (i16x8.extend_low_i8x16_s
   (local.get $0)
  )
 )
 (func $i16x8.extend_high_i8x16_s (param $0 v128) (result v128)
  (i16x8.extend_high_i8x16_s
   (local.get $0)
  )
 )
 (func $i16x8.extend_low_i8x16_u (param $0 v128) (result v128)
  (i16x8.extend_low_i8x16_u
   (local.get $0)
  )
 )
 (func $i16x8.extend_high_i8x16_u (param $0 v128) (result v128)
  (i16x8.extend_high_i8x16_u
   (local.get $0)
  )
 )
(func $i16x8.shl (param $0 v128) (param $1 i32) (result v128)
  (i16x8.shl
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i16x8.shr_s (param $0 v128) (param $1 i32) (result v128)
  (i16x8.shr_s
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i16x8.shr_u (param $0 v128) (param $1 i32) (result v128)
  (i16x8.shr_u
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i16x8.add (param $0 v128) (param $1 v128) (result v128)
  (i16x8.add
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i16x8.add_sat_s (param $0 v128) (param $1 v128) (result v128)
  (i16x8.add_sat_s
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i16x8.add_sat_u (param $0 v128) (param $1 v128) (result v128)
  (i16x8.add_sat_u
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i16x8.sub (param $0 v128) (param $1 v128) (result v128)
  (i16x8.sub
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i16x8.sub_sat_s (param $0 v128) (param $1 v128) (result v128)
  (i16x8.sub_sat_s
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i16x8.sub_sat_u (param $0 v128) (param $1 v128) (result v128)
  (i16x8.sub_sat_u
   (local.get $0)
   (local.get $1)
  )
 )
 (func $f64x2.nearest (param $0 v128) (result v128)
  (f64x2.nearest
   (local.get $0)
  )
 )
 (func $i16x8.mul (param $0 v128) (param $1 v128) (result v128)
  (i16x8.mul
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i16x8.min_s (param $0 v128) (param $1 v128) (result v128)
  (i16x8.min_s
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i16x8.min_u (param $0 v128) (param $1 v128) (result v128)
  (i16x8.min_u
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i16x8.max_s (param $0 v128) (param $1 v128) (result v128)
  (i16x8.max_s
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i16x8.max_u (param $0 v128) (param $1 v128) (result v128)
  (i16x8.max_u
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i16x8.avgr_u (param $0 v128) (param $1 v128) (result v128)
  (i16x8.avgr_u
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i16x8.extmul_low_i8x16_s (param $0 v128) (param $1 v128) (result v128)
  (i16x8.extmul_low_i8x16_s
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i16x8.extmul_high_i8x16_s (param $0 v128) (param $1 v128) (result v128)
  (i16x8.extmul_high_i8x16_s
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i16x8.extmul_low_i8x16_u (param $0 v128) (param $1 v128) (result v128)
  (i16x8.extmul_low_i8x16_u
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i16x8.extmul_high_i8x16_u (param $0 v128) (param $1 v128) (result v128)
  (i16x8.extmul_high_i8x16_u
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i32x4.abs (param $0 v128) (result v128)
  (i32x4.abs
   (local.get $0)
  )
 )
 (func $i32x4.neg (param $0 v128) (result v128)
  (i32x4.neg
   (local.get $0)
  )
 )
 (func $i32x4.all_true (param $0 v128) (result i32)
  (i32x4.all_true
   (local.get $0)
  )
 )
 (func $i32x4.bitmask (param $0 v128) (result i32)
  (i32x4.bitmask
   (local.get $0)
  )
 )
 (func $i32x4.extend_low_i16x8_s (param $0 v128) (result v128)
  (i32x4.extend_low_i16x8_s
   (local.get $0)
  )
 )
 (func $i32x4.extend_high_i16x8_s (param $0 v128) (result v128)
  (i32x4.extend_high_i16x8_s
   (local.get $0)
  )
 )
 (func $i32x4.extend_low_i16x8_u (param $0 v128) (result v128)
  (i32x4.extend_low_i16x8_u
   (local.get $0)
  )
 )
 (func $i32x4.extend_high_i16x8_u (param $0 v128) (result v128)
  (i32x4.extend_high_i16x8_u
   (local.get $0)
  )
 )
 (func $i32x4.shl (param $0 v128) (param $1 i32) (result v128)
  (i32x4.shl
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i32x4.shr_s (param $0 v128) (param $1 i32) (result v128)
  (i32x4.shr_s
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i32x4.shr_u (param $0 v128) (param $1 i32) (result v128)
  (i32x4.shr_u
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i32x4.add (param $0 v128) (param $1 v128) (result v128)
  (i32x4.add
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i32x4.sub (param $0 v128) (param $1 v128) (result v128)
  (i32x4.sub
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i32x4.mul (param $0 v128) (param $1 v128) (result v128)
  (i32x4.mul
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i32x4.min_s (param $0 v128) (param $1 v128) (result v128)
  (i32x4.min_s
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i32x4.min_u (param $0 v128) (param $1 v128) (result v128)
  (i32x4.min_u
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i32x4.max_s (param $0 v128) (param $1 v128) (result v128)
  (i32x4.max_s
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i32x4.max_u (param $0 v128) (param $1 v128) (result v128)
  (i32x4.max_u
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i32x4.dot_i16x8_s (param $0 v128) (param $1 v128) (result v128)
  (i32x4.dot_i16x8_s
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i32x4.extmul_low_i16x8_s (param $0 v128) (param $1 v128) (result v128)
  (i32x4.extmul_low_i16x8_s
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i32x4.extmul_high_i16x8_s (param $0 v128) (param $1 v128) (result v128)
  (i32x4.extmul_high_i16x8_s
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i32x4.extmul_low_i16x8_u (param $0 v128) (param $1 v128) (result v128)
  (i32x4.extmul_low_i16x8_u
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i32x4.extmul_high_i16x8_u (param $0 v128) (param $1 v128) (result v128)
  (i32x4.extmul_high_i16x8_u
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i64x2.abs (param $0 v128) (result v128)
  (i64x2.abs
   (local.get $0)
  )
 )
 (func $i64x2.neg (param $0 v128) (result v128)
  (i64x2.neg
   (local.get $0)
  )
 )
 (func $i64x2.all_true (param $0 v128) (result i32)
  (i64x2.all_true
   (local.get $0)
  )
 )
 (func $i64x2.bitmask (param $0 v128) (result i32)
  (i64x2.bitmask
   (local.get $0)
  )
 )
 (func $i64x2.extend_low_i32x4_s (param $0 v128) (result v128)
  (i64x2.extend_low_i32x4_s
   (local.get $0)
  )
 )
 (func $i64x2.extend_high_i32x4_s (param $0 v128) (result v128)
  (i64x2.extend_high_i32x4_s
   (local.get $0)
  )
 )
 (func $i64x2.extend_low_i32x4_u (param $0 v128) (result v128)
  (i64x2.extend_low_i32x4_u
   (local.get $0)
  )
 )
 (func $i64x2.extend_high_i32x4_u (param $0 v128) (result v128)
  (i64x2.extend_high_i32x4_u
   (local.get $0)
  )
 )
 (func $i64x2.shl (param $0 v128) (param $1 i32) (result v128)
  (i64x2.shl
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i64x2.shr_s (param $0 v128) (param $1 i32) (result v128)
  (i64x2.shr_s
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i64x2.shr_u (param $0 v128) (param $1 i32) (result v128)
  (i64x2.shr_u
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i64x2.add (param $0 v128) (param $1 v128) (result v128)
  (i64x2.add
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i64x2.sub (param $0 v128) (param $1 v128) (result v128)
  (i64x2.sub
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i64x2.mul (param $0 v128) (param $1 v128) (result v128)
  (i64x2.mul
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i64x2.eq (param $0 v128) (param $1 v128) (result v128)
  (i64x2.eq
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i64x2.ne (param $0 v128) (param $1 v128) (result v128)
  (i64x2.ne
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i64x2.lt_s (param $0 v128) (param $1 v128) (result v128)
  (i64x2.lt_s
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i64x2.gt_s (param $0 v128) (param $1 v128) (result v128)
  (i64x2.gt_s
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i64x2.le_s (param $0 v128) (param $1 v128) (result v128)
  (i64x2.le_s
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i64x2.ge_s (param $0 v128) (param $1 v128) (result v128)
  (i64x2.ge_s
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i64x2.extmul_low_i32x4_s (param $0 v128) (param $1 v128) (result v128)
  (i64x2.extmul_low_i32x4_s
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i64x2.extmul_high_i32x4_s (param $0 v128) (param $1 v128) (result v128)
  (i64x2.extmul_high_i32x4_s
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i64x2.extmul_low_i32x4_u (param $0 v128) (param $1 v128) (result v128)
  (i64x2.extmul_low_i32x4_u
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i64x2.extmul_high_i32x4_u (param $0 v128) (param $1 v128) (result v128)
  (i64x2.extmul_high_i32x4_u
   (local.get $0)
   (local.get $1)
  )
 )
 (func $f32x4.abs (param $0 v128) (result v128)
  (f32x4.abs
   (local.get $0)
  )
 )
 (func $f32x4.neg (param $0 v128) (result v128)
  (f32x4.neg
   (local.get $0)
  )
 )
 (func $f32x4.sqrt (param $0 v128) (result v128)
  (f32x4.sqrt
   (local.get $0)
  )
 )
 (func $f32x4.add (param $0 v128) (param $1 v128) (result v128)
  (f32x4.add
   (local.get $0)
   (local.get $1)
  )
 )
 (func $f32x4.sub (param $0 v128) (param $1 v128) (result v128)
  (f32x4.sub
   (local.get $0)
   (local.get $1)
  )
 )
 (func $f32x4.mul (param $0 v128) (param $1 v128) (result v128)
  (f32x4.mul
   (local.get $0)
   (local.get $1)
  )
 )
 (func $f32x4.div (param $0 v128) (param $1 v128) (result v128)
  (f32x4.div
   (local.get $0)
   (local.get $1)
  )
 )
 (func $f32x4.min (param $0 v128) (param $1 v128) (result v128)
  (f32x4.min
   (local.get $0)
   (local.get $1)
  )
 )
 (func $f32x4.max (param $0 v128) (param $1 v128) (result v128)
  (f32x4.max
   (local.get $0)
   (local.get $1)
  )
 )
 (func $f32x4.pmin (param $0 v128) (param $1 v128) (result v128)
  (f32x4.pmin
   (local.get $0)
   (local.get $1)
  )
 )
 (func $f32x4.pmax (param $0 v128) (param $1 v128) (result v128)
  (f32x4.pmax
   (local.get $0)
   (local.get $1)
  )
 )
 (func $f64x2.abs (param $0 v128) (result v128)
  (f64x2.abs
   (local.get $0)
  )
 )
 (func $f64x2.neg (param $0 v128) (result v128)
  (f64x2.neg
   (local.get $0)
  )
 )
 (func $f64x2.sqrt (param $0 v128) (result v128)
  (f64x2.sqrt
   (local.get $0)
  )
 )
 (func $f64x2.add (param $0 v128) (param $1 v128) (result v128)
  (f64x2.add
   (local.get $0)
   (local.get $1)
  )
 )
 (func $f64x2.sub (param $0 v128) (param $1 v128) (result v128)
  (f64x2.sub
   (local.get $0)
   (local.get $1)
  )
 )
 (func $f64x2.mul (param $0 v128) (param $1 v128) (result v128)
  (f64x2.mul
   (local.get $0)
   (local.get $1)
  )
 )
 (func $f64x2.div (param $0 v128) (param $1 v128) (result v128)
  (f64x2.div
   (local.get $0)
   (local.get $1)
  )
 )
 (func $f64x2.min (param $0 v128) (param $1 v128) (result v128)
  (f64x2.min
   (local.get $0)
   (local.get $1)
  )
 )
 (func $f64x2.max (param $0 v128) (param $1 v128) (result v128)
  (f64x2.max
   (local.get $0)
   (local.get $1)
  )
 )
 (func $f64x2.pmin (param $0 v128) (param $1 v128) (result v128)
  (f64x2.pmin
   (local.get $0)
   (local.get $1)
  )
 )
 (func $f64x2.pmax (param $0 v128) (param $1 v128) (result v128)
  (f64x2.pmax
   (local.get $0)
   (local.get $1)
  )
 )
 (func $i32x4.trunc_sat_f32x4_s (param $0 v128) (result v128)
  (i32x4.trunc_sat_f32x4_s
   (local.get $0)
  )
 )
 (func $i32x4.trunc_sat_f32x4_u (param $0 v128) (result v128)
  (i32x4.trunc_sat_f32x4_u
   (local.get $0)
  )
 )
 (func $f32x4.convert_i32x4_s (param $0 v128) (result v128)
  (f32x4.convert_i32x4_s
   (local.get $0)
  )
 )
 (func $f32x4.convert_i32x4_u (param $0 v128) (result v128)
  (f32x4.convert_i32x4_u
   (local.get $0)
  )
 )
 (func $i32x4.trunc_sat_f64x2_s_zero (param $0 v128) (result v128)
  (i32x4.trunc_sat_f64x2_s_zero
   (local.get $0)
  )
 )
 (func $i32x4.trunc_sat_f64x2_u_zero (param $0 v128) (result v128)
  (i32x4.trunc_sat_f64x2_u_zero
   (local.get $0)
  )
 )
 (func $f64x2.convert_low_i32x4_s (param $0 v128) (result v128)
  (f64x2.convert_low_i32x4_s
   (local.get $0)
  )
 )
 (func $f64x2.convert_low_i32x4_u (param $0 v128) (result v128)
  (f64x2.convert_low_i32x4_u
   (local.get $0)
  )
 )
)