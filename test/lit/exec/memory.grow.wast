;; NOTE: Assertions have been generated by update_lit_checks.py --output=fuzz-exec and should not be edited.

;; RUN: wasm-opt %s -all --fuzz-exec-before -q -o /dev/null 2>&1 | filecheck %s

(module
  (memory $0 1)

  ;; CHECK:      [fuzz-exec] calling grow_twice
  ;; CHECK-NEXT: [fuzz-exec] note result: grow_twice => 3
  (func "grow_twice" (result i32)
    ;; The nested grow will increase the size from 1 to 3, and return the old
    ;; size, 1. Then the outer grow will grow by that amount 1, from 3 to 4.
    (memory.grow
      (memory.grow
        (i32.const 2)
      )
    )
  )

  ;; CHECK:      [fuzz-exec] calling measure
  ;; CHECK-NEXT: [fuzz-exec] note result: measure => 4
  (func "measure" (export "measure") (result i32)
    ;; This should return the final size, 4.
    (memory.size)
  )
)
