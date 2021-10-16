
;; NOTE: Assertions have been generated by update_lit_checks.py --all-items and should not be edited.
;; NOTE: This test was ported using port_passes_tests_to_lit.py and could be cleaned up.

;; RUN: foreach %s %t wasm-opt --generate-i64-dyncalls -S -o - | filecheck %s

(module
 ;; CHECK:      (type $i32_=>_i64 (func (param i32) (result i64)))

 ;; CHECK:      (type $none_=>_i32 (func (result i32)))

 ;; CHECK:      (type $i32_i32_=>_i64 (func (param i32 i32) (result i64)))

 ;; CHECK:      (table $0 2 2 funcref)

 ;; CHECK:      (elem (i32.const 0) $f1 $f2)

 ;; CHECK:      (export "dynCall_ji" (func $dynCall_ji))

 ;; CHECK:      (func $f1 (result i32)
 ;; CHECK-NEXT:  (i32.const 1024)
 ;; CHECK-NEXT: )
 (func $f1 (result i32)
  (i32.const 1024)
 )
 ;; CHECK:      (func $f2 (param $0 i32) (result i64)
 ;; CHECK-NEXT:  (i64.const 42)
 ;; CHECK-NEXT: )
 (func $f2 (param i32) (result i64)
  (i64.const 42)
 )
 (table 2 2 funcref)
 (elem (i32.const 0) $f1 $f2)
)
;; CHECK:      (func $dynCall_ji (param $fptr i32) (param $0 i32) (result i64)
;; CHECK-NEXT:  (call_indirect (type $i32_=>_i64)
;; CHECK-NEXT:   (local.get $0)
;; CHECK-NEXT:   (local.get $fptr)
;; CHECK-NEXT:  )
;; CHECK-NEXT: )