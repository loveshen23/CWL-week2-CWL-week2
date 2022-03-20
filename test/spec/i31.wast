(module
  (func (export "new") (param $i i32) (result (ref i31))
    (i31.new (local.get $i))
  )

  (func (export "get_u") (param $i i32) (result i32)
    (i31.get_u (i31.new (local.get $i)))
  )
  (func (export "get_s") (param $i i32) (result i32)
    (i31.get_s (i31.new (local.get $i)))
  )
)

(assert_return (invoke "get_u" (i32.const 0)) (i32.const 0))
(assert_return (invoke "get_u" (i32.const 100)) (i32.const 100))
(assert_return (invoke "get_u" (i32.const -1)) (i32.const 0x7fffffff))
(assert_return (invoke "get_u" (i32.const 0x3fffffff)) (i32.const 0x3fffffff))
(assert_return (invoke "get_u" (i32.const 0x40000000)) (i32.const 0x40000000))
(assert_return (invoke "get_u" (i32.const 0x7fffffff)) (i32.const 0x7fffffff))
(assert_return (invoke "get_u" (i32.const 0xaaaaaaaa)) (i32.const 0x2aaaaaaa))
(assert_return (invoke "get_u" (i32.const 0xcaaaaaaa)) (i32.const 0x4aaaaaaa))

(assert_return (invoke "get_s" (i32.const 0)) (i32.const 0))
(assert_return (invoke "get_s" (i32.const 100)) (i32.const 100))
(assert_return (invoke "get_s" (i32.const -1)) (i32.const -1))
(assert_return (invoke "get_s" (i32.const 0x3fffffff)) (i32.const 0x3fffffff))
(assert_return (invoke "get_s" (i32.const 0x40000000)) (i32.const -0x40000000))
(assert_return (invoke "get_s" (i32.const 0x7ffff