
(module
    (memory i64 1)

    (func $addr_limit (result i64)
      (i64.mul (memory.size) (i64.const 0x10000))
    )

    (func (export "store") (param $i i64) (param $v i32)
      (i32.store (i64.add (call $addr_limit) (local.get $i)) (local.get $v))
    )

    (func (export "load") (param $i i64) (result i32)
      (i32.load (i64.add (call $addr_limit) (local.get $i)))
    )

    (func (export "memory.grow") (param i64) (result i64)
      (memory.grow (local.get 0))
    )
)

(assert_return (invoke "store" (i64.const -4) (i32.const 42)))
(assert_return (invoke "load" (i64.const -4)) (i32.const 42))
(assert_trap (invoke "store" (i64.const -3) (i32.const 13)) "out of bounds memory access")
(assert_trap (invoke "load" (i64.const -3)) "out of bounds memory access")
(assert_trap (invoke "store" (i64.const -2) (i32.const 13)) "out of bounds memory access")
(assert_trap (invoke "load" (i64.const -2)) "out of bounds memory access")
(assert_trap (invoke "store" (i64.const -1) (i32.const 13)) "out of bounds memory access")
(assert_trap (invoke "load" (i64.const -1)) "out of bounds memory access")
(assert_trap (invoke "store" (i64.const 0) (i32.const 13)) "out of bounds memory access")
(assert_trap (invoke "load" (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "store" (i64.const 0x80000000) (i32.const 13)) "out of bounds memory access")
(assert_trap (invoke "load" (i64.const 0x80000000)) "out of bounds memory access")

(module
  (memory i64 1)
  (data (i64.const 0) "abcdefgh")
  (data (i64.const 0xfff8) "abcdefgh")

  (func (export "i32.load") (param $a i64) (result i32)
    (i32.load (local.get $a))
  )
  (func (export "i64.load") (param $a i64) (result i64)
    (i64.load (local.get $a))
  )
  (func (export "f32.load") (param $a i64) (result f32)
    (f32.load (local.get $a))
  )
  (func (export "f64.load") (param $a i64) (result f64)
    (f64.load (local.get $a))
  )
  (func (export "i32.load8_s") (param $a i64) (result i32)
    (i32.load8_s (local.get $a))
  )
  (func (export "i32.load8_u") (param $a i64) (result i32)
    (i32.load8_u (local.get $a))
  )
  (func (export "i32.load16_s") (param $a i64) (result i32)
    (i32.load16_s (local.get $a))
  )
  (func (export "i32.load16_u") (param $a i64) (result i32)
    (i32.load16_u (local.get $a))
  )
  (func (export "i64.load8_s") (param $a i64) (result i64)
    (i64.load8_s (local.get $a))
  )
  (func (export "i64.load8_u") (param $a i64) (result i64)
    (i64.load8_u (local.get $a))
  )
  (func (export "i64.load16_s") (param $a i64) (result i64)
    (i64.load16_s (local.get $a))
  )
  (func (export "i64.load16_u") (param $a i64) (result i64)
    (i64.load16_u (local.get $a))
  )
  (func (export "i64.load32_s") (param $a i64) (result i64)
    (i64.load32_s (local.get $a))
  )
  (func (export "i64.load32_u") (param $a i64) (result i64)
    (i64.load32_u (local.get $a))
  )
  (func (export "i32.store") (param $a i64) (param $v i32)
    (i32.store (local.get $a) (local.get $v))
  )
  (func (export "i64.store") (param $a i64) (param $v i64)
    (i64.store (local.get $a) (local.get $v))
  )
  (func (export "f32.store") (param $a i64) (param $v f32)
    (f32.store (local.get $a) (local.get $v))
  )
  (func (export "f64.store") (param $a i64) (param $v f64)
    (f64.store (local.get $a) (local.get $v))
  )
  (func (export "i32.store8") (param $a i64) (param $v i32)
    (i32.store8 (local.get $a) (local.get $v))
  )
  (func (export "i32.store16") (param $a i64) (param $v i32)
    (i32.store16 (local.get $a) (local.get $v))
  )
  (func (export "i64.store8") (param $a i64) (param $v i64)
    (i64.store8 (local.get $a) (local.get $v))
  )
  (func (export "i64.store16") (param $a i64) (param $v i64)
    (i64.store16 (local.get $a) (local.get $v))
  )
  (func (export "i64.store32") (param $a i64) (param $v i64)
    (i64.store32 (local.get $a) (local.get $v))
  )
)

(assert_trap (invoke "i32.store" (i64.const 0x10000) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.store" (i64.const 0xffff) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.store" (i64.const 0xfffe) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.store" (i64.const 0xfffd) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.store" (i64.const -1) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.store" (i64.const -2) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.store" (i64.const -3) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.store" (i64.const -4) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.store" (i64.const 0x10000) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.store" (i64.const 0xffff) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.store" (i64.const 0xfffe) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.store" (i64.const 0xfffd) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.store" (i64.const 0xfffc) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.store" (i64.const 0xfffb) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.store" (i64.const 0xfffa) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.store" (i64.const 0xfff9) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.store" (i64.const -1) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.store" (i64.const -2) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.store" (i64.const -3) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.store" (i64.const -4) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.store" (i64.const -5) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.store" (i64.const -6) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.store" (i64.const -7) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.store" (i64.const -8) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "f32.store" (i64.const 0x10000) (f32.const 0)) "out of bounds memory access")
(assert_trap (invoke "f32.store" (i64.const 0xffff) (f32.const 0)) "out of bounds memory access")
(assert_trap (invoke "f32.store" (i64.const 0xfffe) (f32.const 0)) "out of bounds memory access")
(assert_trap (invoke "f32.store" (i64.const 0xfffd) (f32.const 0)) "out of bounds memory access")
(assert_trap (invoke "f32.store" (i64.const -1) (f32.const 0)) "out of bounds memory access")
(assert_trap (invoke "f32.store" (i64.const -2) (f32.const 0)) "out of bounds memory access")
(assert_trap (invoke "f32.store" (i64.const -3) (f32.const 0)) "out of bounds memory access")
(assert_trap (invoke "f32.store" (i64.const -4) (f32.const 0)) "out of bounds memory access")
(assert_trap (invoke "f64.store" (i64.const 0x10000) (f64.const 0)) "out of bounds memory access")
(assert_trap (invoke "f64.store" (i64.const 0xffff) (f64.const 0)) "out of bounds memory access")
(assert_trap (invoke "f64.store" (i64.const 0xfffe) (f64.const 0)) "out of bounds memory access")
(assert_trap (invoke "f64.store" (i64.const 0xfffd) (f64.const 0)) "out of bounds memory access")
(assert_trap (invoke "f64.store" (i64.const 0xfffc) (f64.const 0)) "out of bounds memory access")
(assert_trap (invoke "f64.store" (i64.const 0xfffb) (f64.const 0)) "out of bounds memory access")
(assert_trap (invoke "f64.store" (i64.const 0xfffa) (f64.const 0)) "out of bounds memory access")
(assert_trap (invoke "f64.store" (i64.const 0xfff9) (f64.const 0)) "out of bounds memory access")
(assert_trap (invoke "f64.store" (i64.const -1) (f64.const 0)) "out of bounds memory access")
(assert_trap (invoke "f64.store" (i64.const -2) (f64.const 0)) "out of bounds memory access")
(assert_trap (invoke "f64.store" (i64.const -3) (f64.const 0)) "out of bounds memory access")
(assert_trap (invoke "f64.store" (i64.const -4) (f64.const 0)) "out of bounds memory access")
(assert_trap (invoke "f64.store" (i64.const -5) (f64.const 0)) "out of bounds memory access")
(assert_trap (invoke "f64.store" (i64.const -6) (f64.const 0)) "out of bounds memory access")
(assert_trap (invoke "f64.store" (i64.const -7) (f64.const 0)) "out of bounds memory access")
(assert_trap (invoke "f64.store" (i64.const -8) (f64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.store8" (i64.const 0x10000) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.store8" (i64.const -1) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.store16" (i64.const 0x10000) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.store16" (i64.const 0xffff) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.store16" (i64.const -1) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.store16" (i64.const -2) (i32.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.store8" (i64.const 0x10000) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.store8" (i64.const -1) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.store16" (i64.const 0x10000) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.store16" (i64.const 0xffff) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.store16" (i64.const -1) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.store16" (i64.const -2) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.store32" (i64.const 0x10000) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.store32" (i64.const 0xffff) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.store32" (i64.const 0xfffe) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.store32" (i64.const 0xfffd) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.store32" (i64.const -1) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.store32" (i64.const -2) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.store32" (i64.const -3) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i64.store32" (i64.const -4) (i64.const 0)) "out of bounds memory access")
(assert_trap (invoke "i32.load" (i64.const 0x10000)) "out of bounds memory access")
(assert_trap (invoke "i32.load" (i64.const 0xffff)) "out of bounds memory access")
(assert_trap (invoke "i32.load" (i64.const 0xfffe)) "out of bounds memory access")
(assert_trap (invoke "i32.load" (i64.const 0xfffd)) "out of bounds memory access")
(assert_trap (invoke "i32.load" (i64.const -1)) "out of bounds memory access")
(assert_trap (invoke "i32.load" (i64.const -2)) "out of bounds memory access")
(assert_trap (invoke "i32.load" (i64.const -3)) "out of bounds memory access")
(assert_trap (invoke "i32.load" (i64.const -4)) "out of bounds memory access")
(assert_trap (invoke "i64.load" (i64.const 0x10000)) "out of bounds memory access")
(assert_trap (invoke "i64.load" (i64.const 0xffff)) "out of bounds memory access")
(assert_trap (invoke "i64.load" (i64.const 0xfffe)) "out of bounds memory access")
(assert_trap (invoke "i64.load" (i64.const 0xfffd)) "out of bounds memory access")
(assert_trap (invoke "i64.load" (i64.const 0xfffc)) "out of bounds memory access")
(assert_trap (invoke "i64.load" (i64.const 0xfffb)) "out of bounds memory access")
(assert_trap (invoke "i64.load" (i64.const 0xfffa)) "out of bounds memory access")
(assert_trap (invoke "i64.load" (i64.const 0xfff9)) "out of bounds memory access")
(assert_trap (invoke "i64.load" (i64.const -1)) "out of bounds memory access")
(assert_trap (invoke "i64.load" (i64.const -2)) "out of bounds memory access")
(assert_trap (invoke "i64.load" (i64.const -3)) "out of bounds memory access")
(assert_trap (invoke "i64.load" (i64.const -4)) "out of bounds memory access")
(assert_trap (invoke "i64.load" (i64.const -5)) "out of bounds memory access")
(assert_trap (invoke "i64.load" (i64.const -6)) "out of bounds memory access")
(assert_trap (invoke "i64.load" (i64.const -7)) "out of bounds memory access")
(assert_trap (invoke "i64.load" (i64.const -8)) "out of bounds memory access")
(assert_trap (invoke "f32.load" (i64.const 0x10000)) "out of bounds memory access")
(assert_trap (invoke "f32.load" (i64.const 0xffff)) "out of bounds memory access")
(assert_trap (invoke "f32.load" (i64.const 0xfffe)) "out of bounds memory access")
(assert_trap (invoke "f32.load" (i64.const 0xfffd)) "out of bounds memory access")
(assert_trap (invoke "f32.load" (i64.const -1)) "out of bounds memory access")
(assert_trap (invoke "f32.load" (i64.const -2)) "out of bounds memory access")
(assert_trap (invoke "f32.load" (i64.const -3)) "out of bounds memory access")
(assert_trap (invoke "f32.load" (i64.const -4)) "out of bounds memory access")
(assert_trap (invoke "f64.load" (i64.const 0x10000)) "out of bounds memory access")
(assert_trap (invoke "f64.load" (i64.const 0xffff)) "out of bounds memory access")
(assert_trap (invoke "f64.load" (i64.const 0xfffe)) "out of bounds memory access")
(assert_trap (invoke "f64.load" (i64.const 0xfffd)) "out of bounds memory access")
(assert_trap (invoke "f64.load" (i64.const 0xfffc)) "out of bounds memory access")
(assert_trap (invoke "f64.load" (i64.const 0xfffb)) "out of bounds memory access")
(assert_trap (invoke "f64.load" (i64.const 0xfffa)) "out of bounds memory access")
(assert_trap (invoke "f64.load" (i64.const 0xfff9)) "out of bounds memory access")
(assert_trap (invoke "f64.load" (i64.const -1)) "out of bounds memory access")
(assert_trap (invoke "f64.load" (i64.const -2)) "out of bounds memory access")
(assert_trap (invoke "f64.load" (i64.const -3)) "out of bounds memory access")
(assert_trap (invoke "f64.load" (i64.const -4)) "out of bounds memory access")
(assert_trap (invoke "f64.load" (i64.const -5)) "out of bounds memory access")
(assert_trap (invoke "f64.load" (i64.const -6)) "out of bounds memory access")
(assert_trap (invoke "f64.load" (i64.const -7)) "out of bounds memory access")
(assert_trap (invoke "f64.load" (i64.const -8)) "out of bounds memory access")
(assert_trap (invoke "i32.load8_s" (i64.const 0x10000)) "out of bounds memory access")
(assert_trap (invoke "i32.load8_s" (i64.const -1)) "out of bounds memory access")
(assert_trap (invoke "i32.load8_u" (i64.const 0x10000)) "out of bounds memory access")
(assert_trap (invoke "i32.load8_u" (i64.const -1)) "out of bounds memory access")
(assert_trap (invoke "i32.load16_s" (i64.const 0x10000)) "out of bounds memory access")
(assert_trap (invoke "i32.load16_s" (i64.const 0xffff)) "out of bounds memory access")
(assert_trap (invoke "i32.load16_s" (i64.const -1)) "out of bounds memory access")
(assert_trap (invoke "i32.load16_s" (i64.const -2)) "out of bounds memory access")
(assert_trap (invoke "i32.load16_u" (i64.const 0x10000)) "out of bounds memory access")
(assert_trap (invoke "i32.load16_u" (i64.const 0xffff)) "out of bounds memory access")
(assert_trap (invoke "i32.load16_u" (i64.const -1)) "out of bounds memory access")
(assert_trap (invoke "i32.load16_u" (i64.const -2)) "out of bounds memory access")
(assert_trap (invoke "i64.load8_s" (i64.const 0x10000)) "out of bounds memory access")
(assert_trap (invoke "i64.load8_s" (i64.const -1)) "out of bounds memory access")
(assert_trap (invoke "i64.load8_u" (i64.const 0x10000)) "out of bounds memory access")
(assert_trap (invoke "i64.load8_u" (i64.const -1)) "out of bounds memory access")
(assert_trap (invoke "i64.load16_s" (i64.const 0x10000)) "out of bounds memory access")
(assert_trap (invoke "i64.load16_s" (i64.const 0xffff)) "out of bounds memory access")
(assert_trap (invoke "i64.load16_s" (i64.const -1)) "out of bounds memory access")
(assert_trap (invoke "i64.load16_s" (i64.const -2)) "out of bounds memory access")
(assert_trap (invoke "i64.load16_u" (i64.const 0x10000)) "out of bounds memory access")
(assert_trap (invoke "i64.load16_u" (i64.const 0xffff)) "out of bounds memory access")
(assert_trap (invoke "i64.load16_u" (i64.const -1)) "out of bounds memory access")
(assert_trap (invoke "i64.load16_u" (i64.const -2)) "out of bounds memory access")
(assert_trap (invoke "i64.load32_s" (i64.const 0x10000)) "out of bounds memory access")
(assert_trap (invoke "i64.load32_s" (i64.const 0xffff)) "out of bounds memory access")
(assert_trap (invoke "i64.load32_s" (i64.const 0xfffe)) "out of bounds memory access")
(assert_trap (invoke "i64.load32_s" (i64.const 0xfffd)) "out of bounds memory access")
(assert_trap (invoke "i64.load32_s" (i64.const -1)) "out of bounds memory access")
(assert_trap (invoke "i64.load32_s" (i64.const -2)) "out of bounds memory access")
(assert_trap (invoke "i64.load32_s" (i64.const -3)) "out of bounds memory access")
(assert_trap (invoke "i64.load32_s" (i64.const -4)) "out of bounds memory access")
(assert_trap (invoke "i64.load32_u" (i64.const 0x10000)) "out of bounds memory access")
(assert_trap (invoke "i64.load32_u" (i64.const 0xffff)) "out of bounds memory access")
(assert_trap (invoke "i64.load32_u" (i64.const 0xfffe)) "out of bounds memory access")
(assert_trap (invoke "i64.load32_u" (i64.const 0xfffd)) "out of bounds memory access")
(assert_trap (invoke "i64.load32_u" (i64.const -1)) "out of bounds memory access")
(assert_trap (invoke "i64.load32_u" (i64.const -2)) "out of bounds memory access")
(assert_trap (invoke "i64.load32_u" (i64.const -3)) "out of bounds memory access")
(assert_trap (invoke "i64.load32_u" (i64.const -4)) "out of bounds memory access")

;; No memory was changed
(assert_return (invoke "i64.load" (i64.const 0xfff8)) (i64.const 0x6867666564636261))
(assert_return (invoke "i64.load" (i64.const 0)) (i64.const 0x6867666564636261))