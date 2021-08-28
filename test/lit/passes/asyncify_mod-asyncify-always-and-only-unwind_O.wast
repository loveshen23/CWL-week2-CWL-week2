
;; NOTE: Assertions have been generated by update_lit_checks.py --all-items and should not be edited.
;; NOTE: This test was ported using port_passes_tests_to_lit.py and could be cleaned up.

;; RUN: foreach %s %t wasm-opt --asyncify --mod-asyncify-always-and-only-unwind -O -S -o - | filecheck %s

(module
  (memory 1 2)
  ;; CHECK:      (type $none_=>_none (func))

  ;; CHECK:      (type $i32_=>_none (func (param i32)))

  ;; CHECK:      (type $none_=>_i32 (func (result i32)))

  ;; CHECK:      (import "env" "import" (func $import))
  (import "env" "import" (func $import))
  (import "env" "import2" (func $import2 (result i32)))
  (import "env" "import3" (func $import3 (param i32)))
  ;; CHECK:      (global $__asyncify_state (mut i32) (i32.const 0))

  ;; CHECK:      (global $__asyncify_data (mut i32) (i32.const 0))

  ;; CHECK:      (memory $0 1 2)

  ;; CHECK:      (export "calls-import" (func $calls-import))
  (export "calls-import" (func $calls-import))
  ;; CHECK:      (export "calls-import2" (func $calls-import))
  (export "calls-import2" (func $calls-import))
  ;; CHECK:      (export "calls-import2-drop" (func $calls-import))
  (export "calls-import2-drop" (func $calls-import))
  ;; CHECK:      (export "calls-nothing" (func $calls-import))
  (export "calls-nothing" (func $calls-import))
  ;; CHECK:      (export "asyncify_start_unwind" (func $asyncify_start_unwind))

  ;; CHECK:      (export "asyncify_stop_unwind" (func $asyncify_stop_unwind))

  ;; CHECK:      (export "asyncify_start_rewind" (func $asyncify_start_rewind))

  ;; CHECK:      (export "asyncify_stop_rewind" (func $asyncify_stop_unwind))

  ;; CHECK:      (export "asyncify_get_state" (func $asyncify_get_state))

  ;; CHECK:      (func $calls-import (; has Stack IR ;)
  ;; CHECK-NEXT:  (local $0 i32)
  ;; CHECK-NEXT:  (call $import)
  ;; CHECK-NEXT:  (i32.store
  ;; CHECK-NEXT:   (i32.load
  ;; CHECK-NEXT:    (global.get $__asyncify_data)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:   (local.get $0)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (i32.store
  ;; CHECK-NEXT:   (global.get $__asyncify_data)
  ;; CHECK-NEXT:   (i32.add
  ;; CHECK-NEXT:    (i32.load
  ;; CHECK-NEXT:     (global.get $__asyncify_data)
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:    (i32.const 4)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $calls-import
    (call $import)
  )
  (func $calls-import2 (result i32)
    (local $temp i32)
    (local.set $temp (call $import2))
    (return (local.get $temp))
  )
  (func $calls-import2-drop
    (drop (call $import2))
  )
  (func $calls-nothing
    (drop (i32.eqz (i32.const 17)))
  )
)
;; CHECK:      (func $asyncify_start_unwind (; has Stack IR ;) (param $0 i32)
;; CHECK-NEXT:  (global.set $__asyncify_state
;; CHECK-NEXT:   (i32.const 1)
;; CHECK-NEXT:  )
;; CHECK-NEXT:  (global.set $__asyncify_data
;; CHECK-NEXT:   (local.get $0)
;; CHECK-NEXT:  )
;; CHECK-NEXT:  (if
;; CHECK-NEXT:   (i32.gt_u
;; CHECK-NEXT:    (i32.load
;; CHECK-NEXT:     (global.get $__asyncify_data)
;; CHECK-NEXT:    )
;; CHECK-NEXT:    (i32.load offset=4
;; CHECK-NEXT:     (global.get $__asyncify_data)
;; CHECK-NEXT:    )
;; CHECK-NEXT:   )
;; CHECK-NEXT:   (unreachable)
;; CHECK-NEXT:  )
;; CHECK-NEXT: )

;; CHECK:      (func $asyncify_stop_unwind (; has Stack IR ;)
;; CHECK-NEXT:  (global.set $__asyncify_state
;; CHECK-NEXT:   (i32.const 0)
;; CHECK-NEXT:  )
;; CHECK-NEXT:  (if
;; CHECK-NEXT:   (i32.gt_u
;; CHECK-NEXT:    (i32.load
;; CHECK-NEXT:     (global.get $__asyncify_data)
;; CHECK-NEXT:    )
;; CHECK-NEXT:    (i32.load offset=4
;; CHECK-NEXT:     (global.get $__asyncify_data)
;; CHECK-NEXT:    )
;; CHECK-NEXT:   )
;; CHECK-NEXT:   (unreachable)
;; CHECK-NEXT:  )
;; CHECK-NEXT: )

;; CHECK:      (func $asyncify_start_rewind (; has Stack IR ;) (param $0 i32)
;; CHECK-NEXT:  (global.set $__asyncify_state
;; CHECK-NEXT:   (i32.const 2)
;; CHECK-NEXT:  )
;; CHECK-NEXT:  (global.set $__asyncify_data
;; CHECK-NEXT:   (local.get $0)
;; CHECK-NEXT:  )
;; CHECK-NEXT:  (if
;; CHECK-NEXT:   (i32.gt_u
;; CHECK-NEXT:    (i32.load
;; CHECK-NEXT:     (global.get $__asyncify_data)
;; CHECK-NEXT:    )
;; CHECK-NEXT:    (i32.load offset=4
;; CHECK-NEXT:     (global.get $__asyncify_data)
;; CHECK-NEXT:    )
;; CHECK-NEXT:   )
;; CHECK-NEXT:   (unreachable)
;; CHECK-NEXT:  )
;; CHECK-NEXT: )

;; CHECK:      (func $asyncify_get_state (; has Stack IR ;) (result i32)
;; CHECK-NEXT:  (global.get $__asyncify_state)
;; CHECK-NEXT: )