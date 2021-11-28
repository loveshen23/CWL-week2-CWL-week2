
;; NOTE: Assertions have been generated by update_lit_checks.py and should not be edited.
;; RUN: wasm-opt %s --optimize-instructions -S -o - | filecheck %s

(module
  ;; CHECK:      (func $duplicate-elimination (param $x i32)
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (i32.extend8_s
  ;; CHECK-NEXT:    (local.get $x)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (i32.extend16_s
  ;; CHECK-NEXT:    (local.get $x)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $duplicate-elimination (param $x i32)
    (drop (i32.extend8_s (i32.extend8_s (local.get $x))))
    (drop (i32.extend16_s (i32.extend16_s (local.get $x))))
  )

  ;; i64(x) << 56 >> 56   ==>   i64.extend8_s(x)
  ;; i64(x) << 48 >> 48   ==>   i64.extend16_s(x)
  ;; i64(x) << 32 >> 32   ==>   i64.extend32_s(x)
  ;; i64.extend_i32_s(i32.wrap_i64(x))   ==>   i64.extend32_s(x)

  ;; CHECK:      (func $i64-sign-extentions (param $x i64)
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (i64.extend8_s
  ;; CHECK-NEXT:    (local.get $x)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (i64.extend16_s
  ;; CHECK-NEXT:    (local.get $x)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (i64.extend32_s
  ;; CHECK-NEXT:    (local.get $x)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (i64.shr_s
  ;; CHECK-NEXT:    (i64.shl
  ;; CHECK-NEXT:     (local.get $x)
  ;; CHECK-NEXT:     (i64.const 16)
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:    (i64.const 16)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (i64.extend32_s
  ;; CHECK-NEXT:    (local.get $x)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $i64-sign-extentions (param $x i64)
    (drop (i64.shr_s (i64.shl (local.get $x) (i64.const 56)) (i64.const 56)))
    (drop (i64.shr_s (i64.shl (local.get $x) (i64.const 48)) (i64.const 48)))
    (drop (i64.shr_s (i64.shl (local.get $x) (i64.const 32)) (i64.const 32)))
    (drop (i64.shr_s (i64.shl (local.get $x) (i64.const 16)) (i64.const 16))) ;; skip
    (drop (i64.extend_i32_s (i32.wrap_i64 (local.get $x))))
  )

  ;; i32(x) << 24 >> 24   ==>   i32.extend8_s(x)
  ;; i32(x) << 16 >> 16   ==>   i32.extend16_s(x)

  ;; CHECK:      (func $i32-sign-extentions (param $x i32)
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (i32.extend8_s
  ;; CHECK-NEXT:    (local.get $x)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (i32.extend16_s
  ;; CHECK-NEXT:    (local.get $x)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (i32.shr_s
  ;; CHECK-NEXT:    (i32.shl
  ;; CHECK-NEXT:     (local.get $x)
  ;; CHECK-NEXT:     (i32.const 8)
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:    (i32.const 8)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (i32.and
  ;; CHECK-NEXT:    (local.get $x)
  ;; CHECK-NEXT:    (i32.const 255)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (i32.and
  ;; CHECK-NEXT:    (local.get $x)
  ;; CHECK-NEXT:    (i32.const 65535)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (i32.shr_s
  ;; CHECK-NEXT:    (i32.shl
  ;; CHECK-NEXT:     (local.get $x)
  ;; CHECK-NEXT:     (i32.const 16)
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:    (i32.const 24)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (i32.shr_s
  ;; CHECK-NEXT:    (i32.shl
  ;; CHECK-NEXT:     (local.get $x)
  ;; CHECK-NEXT:     (i32.const 24)
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:    (i32.const 16)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $i32-sign-extentions (param $x i32)
    (drop (i32.shr_s (i32.shl (local.get $x) (i32.const 24)) (i32.const 24)))
    (drop (i32.shr_s (i32.shl (local.get $x) (i32.const 16)) (i32.const 16)))

    (drop (i32.shr_s (i32.shl (local.get $x)  (i32.const 8))  (i32.const 8))) ;; skip
    (drop (i32.shr_u (i32.shl (local.get $x) (i32.const 24)) (i32.const 24))) ;; skip
    (drop (i32.shr_u (i32.shl (local.get $x) (i32.const 16)) (i32.const 16))) ;; skip
    (drop (i32.shr_s (i32.shl (local.get $x) (i32.const 16)) (i32.const 24))) ;; skip
    (drop (i32.shr_s (i32.shl (local.get $x) (i32.const 24)) (i32.const 16))) ;; skip
  )
)