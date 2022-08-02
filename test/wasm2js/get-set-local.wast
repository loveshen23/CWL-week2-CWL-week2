;; Tests for lowering local.get and local.set.

(module
  (func $dummy)

  (func (export "check_extend_ui32") (param $0 i32) (param $r i64) (result i32)
    (local $x i32)
    (local $result i64)
    (local $extend i64)
    (local.set $x (local.get $0))
    (local.set $result (local.get $r))
    (local.set $extend (i64.extend_i32_u (local.get $x)))
    (i64.eq (local.get $extend) (local.get $result)))
)

(assert_return (invoke "check_extend_ui32" (i32.const 0)
                                           (i32.const 0) (i32.const 0))
               (i32.const