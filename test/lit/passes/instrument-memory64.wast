;; NOTE: Assertions have been generated by update_lit_checks.py --all-items and should not be edited.
;; NOTE: This test was ported using port_passes_tests_to_lit.py and could be cleaned up.

;; RUN: foreach %s %t wasm-opt --instrument-memory --enable-memory64 -S -o - | filecheck %s

(module
  (memory i64 256 256)
  ;; CHECK:      (type $1 (func))
  (type $1 (func))
  ;; CHECK:      (type $i32_i32_i64_i64_=>_i64 (func (param i32 i32 i64 i64) (result i64)))

  ;; CHECK:      (type $i32_i32_=>_i32 (func (param i32 i32) (result i32)))

  ;; CHECK:      (type $i32_i64_=>_i64 (func (param i32 i64) (result i64)))

  ;; CHECK:      (type $i32_f32_=>_f32 (func (param i32 f32) (result f32)))

  ;; CHECK:      (type $i32_f64_=>_f64 (func (param i32 f64) (result f64)))

  ;; CHECK:      (import "env" "load_ptr" (func $load_ptr (param i32 i32 i64 i64) (result i64)))

  ;; CHECK:      (import "env" "load_val_i32" (func $load_val_i32 (param i32 i32) (result i32)))

  ;; CHECK:      (import "env" "load_val_i64" (func $load_val_i64 (param i32 i64) (result i64)))

  ;; CHECK:      (import "env" "load_val_f32" (func $load_val_f32 (param i32 f32) (result f32)))

  ;; CHECK:      (import "env" "load_val_f64" (func $load_val_f64 (param i32 f64) (result f64)))

  ;; CHECK:      (import "env" "store_ptr" (func $store_ptr (param i32 i32 i64 i64) (result i64)))

  ;; CHECK:      (import "env" "store_val_i32" (func $store_val_i32 (param i32 i32) (result i32)))

  ;; CHECK:      (import "env" "store_val_i64" (func $store_val_i64 (param i32 i64) (result i64)))

  ;; CHECK:      (import "env" "store_val_f32" (func $store_val_f32 (param i32 f32) (result f32)))

  ;; CHECK:      (import "env" "store_val_f64" (func $store_val_f64 (param i32 f64) (result f64)))

  ;; CHECK:      (memory $0 i64 256 256)

  ;; CHECK:      (func $A
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (call $load_val_i32
  ;; CHECK-NEXT:    (i32.const 1)
  ;; CHECK-NEXT:    (i32.load8_s
  ;; CHECK-NEXT:     (call $load_ptr
  ;; CHECK-NEXT:      (i32.const 1)
  ;; CHECK-NEXT:      (i32.const 1)
  ;; CHECK-NEXT:      (i64.const 0)
  ;; CHECK-NEXT:      (i64.const 0)
  ;; CHECK-NEXT:     )
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (call $load_val_i32
  ;; CHECK-NEXT:    (i32.const 2)
  ;; CHECK-NEXT:    (i32.load8_u
  ;; CHECK-NEXT:     (call $load_ptr
  ;; CHECK-NEXT:      (i32.const 2)
  ;; CHECK-NEXT:      (i32.const 1)
  ;; CHECK-NEXT:      (i64.const 0)
  ;; CHECK-NEXT:      (i64.const 0)
  ;; CHECK-NEXT:     )
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (call $load_val_i32
  ;; CHECK-NEXT:    (i32.const 3)
  ;; CHECK-NEXT:    (i32.load16_s
  ;; CHECK-NEXT:     (call $load_ptr
  ;; CHECK-NEXT:      (i32.const 3)
  ;; CHECK-NEXT:      (i32.const 2)
  ;; CHECK-NEXT:      (i64.const 0)
  ;; CHECK-NEXT:      (i64.const 0)
  ;; CHECK-NEXT:     )
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (call $load_val_i32
  ;; CHECK-NEXT:    (i32.const 4)
  ;; CHECK-NEXT:    (i32.load16_u
  ;; CHECK-NEXT:     (call $load_ptr
  ;; CHECK-NEXT:      (i32.const 4)
  ;; CHECK-NEXT:      (i32.const 2)
  ;; CHECK-NEXT:      (i64.const 0)
  ;; CHECK-NEXT:      (i64.const 0)
  ;; CHECK-NEXT:     )
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (call $load_val_i32
  ;; CHECK-NEXT:    (i32.const 5)
  ;; CHECK-NEXT:    (i32.load
  ;; CHECK-NEXT:     (call $load_ptr
  ;; CHECK-NEXT:      (i32.const 5)
  ;; CHECK-NEXT:      (i32.const 4)
  ;; CHECK-NEXT:      (i64.const 0)
  ;; CHECK-NEXT:      (i64.const 0)
  ;; CHECK-NEXT:     )
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (call $load_val_i64
  ;; CHECK-NEXT:    (i32.const 6)
  ;; CHECK-NEXT:    (i64.load8_s
  ;; CHECK-NEXT:     (call $load_ptr
  ;; CHECK-NEXT:      (i32.const 6)
  ;; CHECK-NEXT:      (i32.const 1)
  ;; CHECK-NEXT:      (i64.const 0)
  ;; CHECK-NEXT:      (i64.const 0)
  ;; CHECK-NEXT:     )
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (call $load_val_i64
  ;; CHECK-NEXT:    (i32.const 7)
  ;; CHECK-NEXT:    (i64.load8_u
  ;; CHECK-NEXT:     (call $load_ptr
  ;; CHECK-NEXT:      (i32.const 7)
  ;; CHECK-NEXT:      (i32.const 1)
  ;; CHECK-NEXT:      (i64.const 0)
  ;; CHECK-NEXT:      (i64.const 0)
  ;; CHECK-NEXT:     )
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (call $load_val_i64
  ;; CHECK-NEXT:    (i32.const 8)
  ;; CHECK-NEXT:    (i64.load16_s
  ;; CHECK-NEXT:     (call $load_ptr
  ;; CHECK-NEXT:      (i32.const 8)
  ;; CHECK-NEXT:      (i32.const 2)
  ;; CHECK-NEXT:      (i64.const 0)
  ;; CHECK-NEXT:      (i64.const 0)
  ;; CHECK-NEXT:     )
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (call $load_val_i64
  ;; CHECK-NEXT:    (i32.const 9)
  ;; CHECK-NEXT:    (i64.load16_u
  ;; CHECK-NEXT:     (call $load_ptr
  ;; CHECK-NEXT:      (i32.const 9)
  ;; CHECK-NEXT:      (i32.const 2)
  ;; CHECK-NEXT:      (i64.const 0)
  ;; CHECK-NEXT:      (i64.const 0)
  ;; CHECK-NEXT:     )
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (call $load_val_i64
  ;; CHECK-NEXT:    (i32.const 10)
  ;; CHECK-NEXT:    (i64.load32_s
  ;; CHECK-NEXT:     (call $load_ptr
  ;; CHECK-NEXT:      (i32.const 10)
  ;; CHECK-NEXT:      (i32.const 4)
  ;; CHECK-NEXT:      (i64.const 0)
  ;; CHECK-NEXT:      (i64.const 0)
  ;; CHECK-NEXT:     )
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (call $load_val_i64
  ;; CHECK-NEXT:    (i32.const 11)
  ;; CHECK-NEXT:    (i64.load32_u
  ;; CHECK-NEXT:     (call $load_ptr
  ;; CHECK-NEXT:      (i32.const 11)
  ;; CHECK-NEXT:      (i32.const 4)
  ;; CHECK-NEXT:      (i64.const 0)
  ;; CHECK-NEXT:      (i64.const 0)
  ;; CHECK-NEXT:     )
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (call $load_val_i64
  ;; CHECK-NEXT:    (i32.const 12)
  ;; CHECK-NEXT:    (i64.load
  ;; CHECK-NEXT:     (call $load_ptr
  ;; CHECK-NEXT:      (i32.const 12)
  ;; CHECK-NEXT:      (i32.const 8)
  ;; CHECK-NEXT:      (i64.const 0)
  ;; CHECK-NEXT:      (i64.const 0)
  ;; CHECK-NEXT:     )
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (call $load_val_f32
  ;; CHECK-NEXT:    (i32.const 13)
  ;; CHECK-NEXT:    (f32.load
  ;; CHECK-NEXT:     (call $load_ptr
  ;; CHECK-NEXT:      (i32.const 13)
  ;; CHECK-NEXT:      (i32.const 4)
  ;; CHECK-NEXT:      (i64.const 0)
  ;; CHECK-NEXT:      (i64.const 0)
  ;; CHECK-NEXT:     )
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (call $load_val_f64
  ;; CHECK-NEXT:    (i32.const 14)
  ;; CHECK-NEXT:    (f64.load
  ;; CHECK-NEXT:     (call $load_ptr
  ;; CHECK-NEXT:      (i32.const 14)
  ;; CHECK-NEXT:      (i32.const 8)
  ;; CHECK-NEXT:      (i64.const 0)
  ;; CHECK-NEXT:      (i64.const 0)
  ;; CHECK-NEXT:     )
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: 