;; RUN: wasm-split %s --instrument -S -o - | filecheck %s

;; Check that the output round trips and validates as well
;; RUN: wasm-split %s --instrument -g -o %t
;; RUN: wasm-opt %t --print | filecheck %s

(module
  (import "env" "foo" (func $foo))
  (export "bar" (func $bar))
  (func $bar
    (call $foo)
  )
  (func $baz (param i32) (result i32)
    (local.get 0)
  )
)

;; Check that the counter and timestamps have been added
;; CHECK: (global $monotonic_counter (mut i32) (i32.const 0))
;; CHECK: (global $bar_timestamp (mut i32) (i32.const 0))
;; CHECK: (global $baz_timestamp (mut i32) (i32.const 0))

;; Check that a memory has been added
;; CHECK: (memory $0 1 1)

;; And the profiling function is exported
;; CHECK: (export "__write_profile" (func $__write_profile))

;; And the memory has been exported
;; CHECK: (export "profile-memory" (memory $0))

;; Ch