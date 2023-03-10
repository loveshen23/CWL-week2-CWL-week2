
(module
 (memory $0 256 256)
 (export "add" (func $add))
 (export "no_return" (func $no-return)) ;; note exported name is slightly different
 (export "types" (func $types))
 (export "types2" (func $types2))
 (export "types3" (func $types3))
 (func $add (param $x i32) (param $y i32) (result i32)
  (i32.add
   (local.get $x)
   (local.get $y)
  )
 )
 (func $unexported (param $x i32) (param $y i32) (result i32)
  (i32.add
   (local.get $x)
   (local.get $y)
  )
 )
 (func $no-return (param $x i32)
  (drop
   (i32.add
    (local.get $x)
    (local.get $x)
   )
  )
 )
 (func $types (param $x i32) (param $y i64) (param $z f32) (param $w f64)
  (nop)
 )
 (func $types2 (param $x i32) (param $z f32) (param $w f64)
  (nop)
 )
 (func $types3 (param $x i32) (param $z f32) (param $w f64) (result i64)
  (i64.const 1)
 )
)