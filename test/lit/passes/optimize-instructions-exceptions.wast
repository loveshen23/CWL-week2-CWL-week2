
;; NOTE: Assertions have been generated by update_lit_checks.py and should not be edited.
;; RUN: wasm-opt %s --optimize-instructions --enable-reference-types \
;; RUN:   --enable-exception-handling -S -o - | filecheck %s

(module
  ;; CHECK:      (func $test
  ;; CHECK-NEXT:  (if
  ;; CHECK-NEXT:   (try $try (result i32)
  ;; CHECK-NEXT:    (do
  ;; CHECK-NEXT:     (i32.const 123)
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:    (catch_all
  ;; CHECK-NEXT:     (i32.const 456)
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:   (nop)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $test
    (if
      (try (result i32)
        (do
          (i32.eqz
            (i32.eqz
              (i32.const 123)
            )
          )
        )
        (catch_all
          (i32.eqz
            (i32.eqz
              (i32.const 456)
            )
          )
        )
      )
      (nop)
    )
  )
)