
;; Testing i64 shifts

(module
  (func $dummy)

  (func (export "shl_i64") (param $0 i64) (param $1 i64) (param $2 i64) (result i32)
    (i64.eq (i64.shl (local.get $0) (local.get $1)) (local.get $2)))
  (func (export "shr_i64") (param $0 i64) (param $1 i64) (param $2 i64) (result i32)
    (i64.eq (i64.shr_s (local.get $0) (local.get $1)) (local.get $2)))
)

(assert_return (invoke "shl_i64" (i32.const 0) (i32.const 0)
                                 (i32.const 0) (i32.const 0)
                                 (i32.const 0) (i32.const 0))
               (i32.const 1))
(assert_return (invoke "shl_i64" (i32.const 1) (i32.const 0)
                                 (i32.const 1) (i32.const 0)
                                 (i32.const 2) (i32.const 0))
               (i32.const 1))
(assert_return (invoke "shl_i64" (i32.const 1) (i32.const 0)
                                 (i32.const 0) (i32.const 0)
                                 (i32.const 1) (i32.const 0))
               (i32.const 1))
(assert_return (invoke "shl_i64" (i32.const 1) (i32.const 0)
                                 (i32.const 32) (i32.const 0)
                                 (i32.const 0) (i32.const 1))
               (i32.const 1))
(assert_return (invoke "shl_i64" (i32.const 1) (i32.const 0)
                                 (i32.const 63) (i32.const 0)
                                 (i32.const 0) (i32.const 2147483648))
               (i32.const 1))
(assert_return (invoke "shl_i64" (i32.const 1) (i32.const 0)
                                 (i32.const 64) (i32.const 0)
                                 (i32.const 1) (i32.const 0))
               (i32.const 1))

(assert_return (invoke "shr_i64" (i32.const 1) (i32.const 0)
                                 (i32.const 0) (i32.const 0)
                                 (i32.const 1) (i32.const 0))
               (i32.const 1))
(assert_return (invoke "shr_i64" (i32.const 1) (i32.const 0)
                                 (i32.const 32) (i32.const 0)
                                 (i32.const 0) (i32.const 0))
               (i32.const 1))
(assert_return (invoke "shr_i64" (i32.const 0) (i32.const 1)
                                 (i32.const 32) (i32.const 0)
                                 (i32.const 1) (i32.const 0))
               (i32.const 1))
(assert_return (invoke "shr_i64" (i32.const 0) (i32.const 2)
                                 (i32.const 33) (i32.const 0)
                                 (i32.const 1) (i32.const 0))
               (i32.const 1))
(assert_return (invoke "shr_i64" (i32.const 0) (i32.const 2147483648)
                                 (i32.const 32) (i32.const 0)
                                 (i32.const 2147483648) (i32.const -1))
               (i32.const 1))
(assert_return (invoke "shr_i64" (i32.const 0) (i32.const 2147483648)
                                 (i32.const 33) (i32.const 0)
                                 (i32.const 3221225472) (i32.const -1))
               (i32.const 1))
(assert_return (invoke "shr_i64" (i32.const 0) (i32.const 2147483648)
                                 (i32.const 63) (i32.const 0)
                                 (i32.const -1) (i32.const -1))
               (i32.const 1))
(assert_return (invoke "shr_i64" (i32.const 1) (i32.const 0)
                                 (i32.const 1) (i32.const 0)
                                 (i32.const 0) (i32.const 0))
               (i32.const 1))

(assert_return (invoke "shr_i64" (i32.const 1) (i32.const 0)
                                 (i32.const 2) (i32.const 0)
                                 (i32.const 0) (i32.const 0))
               (i32.const 1))
(assert_return (invoke "shr_i64" (i32.const 2) (i32.const 1)
                                 (i32.const 1) (i32.const 0)
                                 (i32.const -2147483647) (i32.const 0))
               (i32.const 1))
(assert_return (invoke "shr_i64" (i32.const 2) (i32.const 1)
                                 (i32.const 2) (i32.const 0)
                                 (i32.const 1073741824) (i32.const 0))
               (i32.const 1))