;; NOTE: Assertions have been generated by update_lit_checks.py --all-items and should not be edited.
;; NOTE: This test was ported using port_passes_tests_to_lit.py and could be cleaned up.

;; RUN: foreach %s %t wasm-opt --dealign --enable-memory64 -S -o - | filecheck %s

(module
 ;; CHECK:      (type $none_=>_none (func))

 ;; CHECK:      (memory $0 i64 1 1)
 (memory $0 i64 1 1)
 ;; CHECK:      (func $test
 ;; CHECK-NEXT:  (drop
 ;; CHECK-NEXT:   (i32.load align=1
 ;; CHECK-NEXT:    (i64.const 4)
 ;; CHECK-NEXT:   )
 ;; CHECK-NEXT:  )
 ;; CHECK-NEXT:  (drop
 ;; CHECK-NEXT:   (i32.load align=1
 ;; CHECK-NEXT:    (i64.const 8)
 ;; CHECK-NEXT:   )
 ;; CHECK-NEXT:  )
 ;; CHECK-NEXT:  (drop
 ;; CHECK-NEXT:   (i32.load align=1
 ;; CHECK-NEXT:    (i64.const 12)
 ;; CHECK-NEXT:   )
 ;; CHECK-NEXT:  )
 ;; CHECK-NEXT:  (i32.store align=1
 ;; CHECK-NEXT:   (i64.const 16)
 ;; CHECK-NEXT:   (i32.const 28)
 ;; CHECK-NEXT:  )
 ;; CHECK-NEXT:  (i32.store align=1
 ;; CHECK-NEXT:   (i64.const 20)
 ;; CHECK-NEXT:   (i32.const 32)
 ;; CHECK-NEXT:  )
 ;; CHECK-NEXT:  (i32.store align=1
 ;; CHECK-NEXT:   (i64.const 24)
 ;; CHECK-NEXT:   (i32.const 36)
 ;; CHECK-NEXT:  )
 ;; CHECK-NEXT: )
 (func $test
  (drop (i32.load         (i64.const 4)))
  (drop (i32.load align=1 (i64.const 8)))
  (drop (i32.load align=2 (i64.const 12)))
  (i32.store         (i64.const 16) (i32.const 28))
  (i32.store align=1 (i64.const 20) (i32.const 32))
  (i32.store align=2 (i64.const 24) (i32.const 36))
 )
)