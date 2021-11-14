
;; NOTE: Assertions have been generated by update_lit_checks.py --all-items and should not be edited.
;; NOTE: This test was ported using port_test.py and could be cleaned up.

;; Check that legalize-js-interface-exported-helpers ends up using the
;; existing __set_temp_ret/__get_temp_ret helpers.
;; RUN: wasm-opt %s --legalize-js-interface --pass-arg=legalize-js-interface-exported-helpers -S -o - | filecheck %s

(module
  ;; CHECK:      (type $none_=>_i32 (func (result i32)))

  ;; CHECK:      (type $none_=>_i64 (func (result i64)))

  ;; CHECK:      (type $i32_=>_none (func (param i32)))

  ;; CHECK:      (import "env" "imported" (func $legalimport$imported (result i32)))

  ;; CHECK:      (export "get_i64" (func $legalstub$get_i64))
  (export "get_i64" (func $get_i64))
  (import "env" "imported" (func $imported (result i64)))
  (export "__set_temp_ret" (func $__set_temp_ret))
  (export "__get_temp_ret" (func $__get_temp_ret))
  ;; CHECK:      (func $get_i64 (result i64)
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (call $legalfunc$imported)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (i64.const 0)
  ;; CHECK-NEXT: )
  (func $get_i64 (result i64)
    (drop (call $imported))
    (i64.const 0)
  )
  ;; CHECK:      (func $__set_temp_ret (param $0 i32)
  ;; CHECK-NEXT:  (nop)
  ;; CHECK-NEXT: )
  (func $__set_temp_ret (param i32))
  ;; CHECK:      (func $__get_temp_ret (result i32)
  ;; CHECK-NEXT:  (i32.const 0)
  ;; CHECK-NEXT: )
  (func $__get_temp_ret (result i32)
    (i32.const 0)
  )
)
;; CHECK:      (func $legalstub$get_i64 (result i32)
;; CHECK-NEXT:  (local $0 i64)
;; CHECK-NEXT:  (local.set $0
;; CHECK-NEXT:   (call $get_i64)
;; CHECK-NEXT:  )
;; CHECK-NEXT:  (call $__set_temp_ret
;; CHECK-NEXT:   (i32.wrap_i64
;; CHECK-NEXT:    (i64.shr_u
;; CHECK-NEXT:     (local.get $0)
;; CHECK-NEXT:     (i64.const 32)
;; CHECK-NEXT:    )
;; CHECK-NEXT:   )
;; CHECK-NEXT:  )
;; CHECK-NEXT:  (i32.wrap_i64
;; CHECK-NEXT:   (local.get $0)
;; CHECK-NEXT:  )
;; CHECK-NEXT: )

;; CHECK:      (func $legalfunc$imported (result i64)
;; CHECK-NEXT:  (i64.or
;; CHECK-NEXT:   (i64.extend_i32_u
;; CHECK-NEXT:    (call $legalimport$imported)
;; CHECK-NEXT:   )
;; CHECK-NEXT:   (i64.shl
;; CHECK-NEXT:    (i64.extend_i32_u
;; CHECK-NEXT:     (call $__get_temp_ret)
;; CHECK-NEXT:    )
;; CHECK-NEXT:    (i64.const 32)
;; CHECK-NEXT:   )
;; CHECK-NEXT:  )
;; CHECK-NEXT: )