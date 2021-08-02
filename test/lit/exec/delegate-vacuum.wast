
;; NOTE: Assertions have been generated by update_lit_checks.py --output=fuzz-exec and should not be edited.

;; RUN: wasm-opt %s --vacuum --fuzz-exec -all -q -o /dev/null 2>&1 | filecheck %s
;; Test the effect of vaccum on delegation. The delegate target must not
;; "escape" the current function scope and affect anything external, that is,
;; it must be cleared on function exit.

(module
 (tag $tag$0 (param i32))
 ;; CHECK:      [fuzz-exec] calling export-1
 ;; CHECK-NEXT: [exception thrown: tag$0 0]
 (func "export-1"
  (try
   (do
    (try
     (do
      (throw $tag$0
       (i32.const 0)
      )
     )
     ;; A delegation that leads to the caller. This sets the delegate field on
     ;; this function scope.
     (delegate 1)
    )
   )
   (catch_all
    (nop)
   )
  )
 )
 ;; CHECK:      [fuzz-exec] calling export-2
 ;; CHECK-NEXT: [trap unreachable]
 (func "export-2"
  (call $inner)
  (unreachable)
 )
 (func $inner
  ;; This inner function must not notice the delegate field that was set by
  ;; the call to the previous export (if it does notice it, it would delegate
  ;; to the caller or something else invalid, and the execution results would
  ;; differ, causing fuzz-exec to fail).
  (try
   (do
    (throw $tag$0
     (i32.const 0)
    )
   )
   (catch_all
    (nop)
   )
  )
 )
)
;; CHECK:      [fuzz-exec] calling export-1
;; CHECK-NEXT: [exception thrown: tag$0 0]

;; CHECK:      [fuzz-exec] calling export-2
;; CHECK-NEXT: [trap unreachable]
;; CHECK-NEXT: [fuzz-exec] comparing export-1
;; CHECK-NEXT: [fuzz-exec] comparing export-2