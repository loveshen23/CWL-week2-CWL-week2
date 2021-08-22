;; NOTE: Assertions have been generated by update_lit_checks.py --all-items and should not be edited.

;; RUN: wasm-opt --enable-memory64 --asyncify %s -S -o - | filecheck %s

(module
  ;; CHECK:      (type $i32_i32_=>_none (func (param i32 i32)))

  ;; CHECK:      (type $f (func (param i32)))
  (type $f (func (param i32)))
  (memory i64 1 2)
  ;; CHECK:      (type $none_=>_none (func))

  ;; CHECK:      (type $i64_=>_none (func (param i64)))

  ;; CHECK:      (type $none_=>_i32 (func (result i32)))

  ;; CHECK:      (import "env" "import" (func $import))
  (import "env" "import" (func $import))
  ;; CHECK:      (import "env" "import2" (func $import2 (param i32)))
  (import "env" "import2" (func $import2 (param i32)))
  (table funcref (elem $liveness2 $liveness2))
  ;; CHECK:      (global $__asyncify_state (mut i32) (i32.const 0))

  ;; CHECK:      (global $__asyncify_data (mut i64) (i64.const 0))

  ;; CHECK:      (memory $0 i64 1 2)

  ;; CHECK:      (table $0 2 2 funcref)

  ;; CHECK:      (elem (i32.const 0) $liveness2 $liveness2)

  ;; CHECK:      (export "asyncify_start_unwind" (func $asyncify_start_unwind))

  ;; CHECK:      (export "asyncify_stop_unwind" (func $asyncify_stop_unwind))

  ;; CHECK:      (export "asyncify_start_rewind" (func $asyncify_start_rewind))

  ;; CHECK:      (export "asyncify_stop_rewind" (func $asyncify_stop_rewind))

  ;; CHECK:      (export "asyncify_get_state" (func $asyncify_get_state))

  ;; CHECK:      (func $liveness1 (param $live0 i32) (param $dead0 i32)
  ;; CHECK-NEXT:  (local $live1 i32)
  ;; CHECK-NEXT:  (local $dead1 i32)
  ;; CHECK-NEXT:  (local $4 i32)
  ;; CHECK-NEXT:  (local $5 i32)
  ;; CHECK-NEXT:  (local $6 i32)
  ;; CHECK-NEXT:  (local $7 i32)
  ;; CHECK-NEXT:  (local $8 i32)
  ;; CHECK-NEXT:  (local $9 i32)
  ;; CHECK-NEXT:  (local $10 i64)
  ;; CHECK-NEXT:  (local $11 i64)
  ;; CHECK-NEXT:  (if
  ;; CHECK-NEXT:   (i32.eq
  ;; CHECK-NEXT:    (global.get $__asyncify_state)
  ;; CHECK-NEXT:    (i32.const 2)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:   (block
  ;; CHECK-NEXT:    (i64.store
  ;; CHECK-NEXT:     (global.get $__asyncify_data)
  ;; CHECK-NEXT:     (i64.add
  ;; CHECK-NEXT:      (i64.load
  ;; CHECK-NEXT:       (global.get $__asyncify_data)
  ;; CHECK-NEXT:      )
  ;; CHECK-NEXT:      (i64.const -8)
  ;; CHECK-NEXT:     )
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:    (local.set $10
  ;; CHECK-NEXT:     (i64.load
  ;; CHECK-NEXT:      (global.get $__asyncify_data)
  ;; CHECK-NEXT:     )
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:    (local.set $live0
  ;; CHECK-NEXT:     (i32.load
  ;; CHECK-NEXT:      (local.get $10)
  ;; CHECK-NEXT:     )
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:    (local.set $live1
  ;; CHECK-NEXT:     (i32.load offset=4
  ;; CHECK-NEXT:      (local.get $10)
  ;; CHECK-NEXT:     )
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (local.set $8
  ;; CHECK-NEXT:   (block $__asyncify_unwind (result i32)
  ;; CHECK-NEXT:    (block
  ;; CHECK-NEXT:     (block
  ;; CHECK-NEXT:      (if
  ;; CHECK-NEXT:       (i32.eq
  ;; CHECK-NEXT:        (global.get $__asyncify_state)
  ;; CHECK-NEXT:        (i32.const 2)
  ;; CHECK-NEXT:       )
  ;; CHECK-NEXT:       (block
  ;; CHECK-NEXT:        (i64.store
  ;; CHECK-NEXT:         (global.get $__asyncify_data)
  ;; CHECK-NEXT:         (i64.add
  ;; CHECK-NEXT:          (i64.load
  ;; CHECK-NEXT:           (global.get $__asyncify_data)
  ;; CHECK-NEXT:          )
  ;; CHECK-NEXT:          (i64.const -4)
  ;; CHECK-NEXT:         )
  ;; CHECK-NEXT:        )
  ;; CHECK-NEXT:        (local.set $9
  ;; CHECK-NEXT:         (i32.load
  ;; CHECK-NEXT:          (i64.load
  ;; CHECK-NEXT:           (global.get $__asyncify_data)
  ;; CHECK-NEXT:          )
  ;; CHECK-NEXT:         )
  ;; CHECK-NEXT:        )
  ;; CHECK-NEXT:       )
  ;; CHECK-NEXT:      )
  ;; CHECK-NEXT:      (block
  ;; CHECK-NEXT:       (if
  ;; CHECK-NEXT:        (i32.eq
  ;; CHECK-NEXT:         (global.get $__asyncify_state)
  ;; CHECK-NEXT:         (i32.const 0)
  ;; CHECK-NEXT:        )
  ;; CHECK-NEXT:        (block
  ;; CHECK-NEXT:         (local.set $4
  ;; CHECK-NEXT:          (local.get $dead0)
  ;; CHECK-NEXT:         )
  ;; CHECK-NEXT:         (drop
  ;; CHECK-NEXT:          (local.get $4)
  ;; CHECK-NEXT:         )
  ;; CHECK-NEXT:         (local.set $5
  ;; CHECK-NEXT:          (local.get $dead1)
  ;; CHECK-NEXT:         )
  ;; CHECK-NEXT:         (drop
  ;; CHECK-NEXT:          (local.get $5)
  ;; CHECK-NEXT:         )
  ;; CHECK-NEXT:        )
  ;; CHECK-NEXT:       )
  ;; CHECK-NEXT:       (nop)
  ;; CHECK-NEXT:       (nop)
  ;; CHECK-NEXT:       (nop)
  ;; CHECK-NEXT:       (if
  ;; CHECK-NEXT:        (if (result i32)
  ;; CHECK-NEXT:         (i32.eq
  ;; CHECK-NEXT:          (global.get $__asyncify_state)
  ;; CHECK-NEXT:          (i32.const 0)
  ;; CHECK-NEXT:         )
  ;; CHECK-NEXT:         (i32.const 1)
  ;; CHECK-NEXT:         (i32.eq
  ;; CHECK-NEXT:          (local.get $9)
  ;; CHECK-NEXT:          (i32.const 0)
  ;; CHECK-NEXT:         )
  ;; CHECK-NEXT:        )
  ;; CHECK-NEXT:        (block
  ;; CHECK-NEXT:         (call $import)
  ;; CHECK-NEXT:         (if
  ;; CHECK-NEXT:          (i32.eq
  ;; CHECK-NEXT:           (global.get $__asyncify_state)
  ;; CHECK-NEXT:           (i32.const 1)
  ;; CHECK-NEXT:          )
  ;; CHECK-NEXT:          (br $__asyncify_unwind
  ;; CHECK-NEXT:           (i32.const 0)
  ;; CHECK-NEXT:          )
  ;; CHECK-NEXT:         )
  ;; CHECK-NEXT:        )
  ;; CHECK-NEXT:       )
  ;; CHECK-NEXT:       (if
  ;; CHECK-NEXT:        (i32.eq
  ;; CHECK-NEXT:         (global.get $__asyncify_state)
  ;; CHECK-NEXT:         (i32.const 0)
  ;; CHECK-NEXT:        )
  ;; CHECK-NEXT:        (block
  ;; CHECK-NEXT:         (local.set $6
  ;; CHECK-NEXT:          (local.get $live0)
  ;; CHECK-NEXT:         )
  ;; CHECK-NEXT:         (drop
  ;; CHECK-NEXT:          (local.get $6)
  ;; CHECK-NEXT:         )
  ;; CHECK-NEXT:         (local.set $7
  ;; CHECK-NEXT:          (local.get $live1)
  ;; CHECK-NEXT:         )
  ;; CHECK-NEXT:         (drop
  ;; CHECK-NEXT:          (local.get $7)
  ;; CHECK-NEXT:         )
  ;; CHECK-NEXT:        )
  ;; CHECK-NEXT:       )
  ;; CHECK-NEXT:       (nop)
  ;; CHECK-NEXT:       (nop)
  ;; CHECK-NEXT:       (nop)
  ;; CHECK-NEXT:      )
  ;; CHECK-NEXT:     )
  ;; CHECK-NEXT:     (return)
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (block
  ;; CHECK-NEXT:   (i32.store
  ;; CHECK-NEXT:    (i64.load
  ;; CHECK-NEXT:     (global.get $__asyncify_data)
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:    (local.get $8)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:   (i64.store
  ;; CHECK-NEXT:    (global.get $__asyncify_data)
  ;; CHECK-NEXT:    (i64.add
  ;; CHECK-NEXT:     (i64.load
  ;; CHECK-NEXT:      (global.get $__asyncify_data)
  ;; CHECK-NEXT:     )
  ;; CHECK-NEXT:     (i64.const 4)
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (block
  ;; CHECK-NEXT:   (local.set $11
  ;; CHECK-NEXT:    (i64.load
  ;; CHECK-NEXT:     (global.get $__asyncify_data)
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:   (i32.store
  ;; CHECK-NEXT:    (local.get $11)
  ;; CHECK-NEXT:    (local.get $live0)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:   (i32.store offset=4
  ;; CHECK-NEXT:    (local.get $11)
  ;; CHECK-NEXT:    (local.get $live1)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:   (i64.store
  ;; CHECK-NEXT:    (global.get $__asyncify_data)
  ;; CHECK-NEXT:    (i64.add
  ;; CHECK-NEXT:     (i64.load
  ;; CHECK-NEXT:      (global.get $__asyncify_data)
  ;; CHECK-NEXT:     )
  ;; CHECK-NEXT:     (i64.const 8)
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $liveness1 (param $live0 i32) (param $dead0 i32)
    (local $live1 i32)
    (local $dead1 i32)
    (drop (local.get $dead0))
    (drop (local.get $dead1))
    (call $import)
    (drop (local.get $live0))
    (drop (local.get $live1))
  )
  ;; CHECK:      (func $liveness2 (param $live0 i32) (param $dead0 i32)
  ;; CHECK-NEXT:  (local $live1 i32)
  ;; CHECK-NEXT:  (local $dead1 i32)
  ;; CHECK-NEXT:  (local $4 i32)
  ;; CHECK-NEXT:  (local $5 i32)
  ;; CHECK-NEXT:  (local $6 i32)
  ;; CHECK-NEXT:  (local $7 i32)
  ;; CHECK-NEXT:  (local $8 i32)
  ;; CHECK-NEXT:  (local $9 i32)
  ;; CHECK-NEXT:  (local $10 i64)
  ;; CHECK-NEXT:  (local $11 i64)
  ;; CHECK-NEXT:  (if
  ;; CHECK-NEXT:   (i32.eq
  ;; CHECK-NEXT:    (global.get $__asyncify_state)
  ;; CHECK-NEXT:    (i32.const 2)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:   (block
  ;; CHECK-NEXT:    (i64.store
  ;; CHECK-NEXT:     (global.get $__asyncify_data)
  ;; CHECK-NEXT:     (i64.add
  ;; CHECK-NEXT:      (i64.load
  ;; CHECK-NEXT:       (global.get $__asyncify_data)
  ;; CHECK-NEXT:      )
  ;; CHECK-NEXT:      (i64.const -8)
  ;; CHECK-NEXT:     )
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:    (local.set $10
  ;; CHECK-NEXT:     (i64.load
  ;; CHECK-NEXT:      (global.get $__asyncify_data)
  ;; CHECK-NEXT:     )
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:    (local.set $live0
  ;; CHECK-NEXT:     (i32.load
  ;; CHECK-NEXT:      (local.get $10)
  ;; CHECK-NEXT:     )
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:    (local.set $live1
  ;; CHECK-NEXT:     (i32.load offset=4
  ;; CHECK-NEXT:      (local.get $10)
  ;; CHECK-NEXT:     )
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (local.set $8
  ;; CHECK-NEXT:   (block $__asyncify_unwind (result i32)
  ;; CHECK-NEXT:    (block
  ;; CHECK-NEXT:     (block
  ;; CHECK-NEXT:      (if
  ;; CHECK-NEXT:       (i32.eq
  ;; CHECK-NEXT:        (global.get $__asyncify_state)
  ;; CHECK-NEXT:        (i32.const 2)
  ;; CHECK-NEXT:       )
  ;; CHECK-NEXT:       (block
  ;; CHECK-NEXT:        (i64.store
  ;; CHECK-NEXT:         (global.get $__asyncify_data)
  ;; CHECK-NEXT:         (i64.add
  ;; CHECK-NEXT:          (i64.load
  ;; CHECK-NEXT:           (global.get $__asyncify_data)
  ;; CHECK-NEXT:          )
  ;; CHECK-NEXT:          (i64.const -4)
  ;; CHECK-NEXT:         )
  ;; CHECK-NEXT:        )
  ;; CHECK-NEXT:        (local.set $9
  ;; CHECK-NEXT:         (i32.load
  ;; CHECK-NEXT:          (i64.load
  ;; CHECK-NEXT:           (global.get $__asyncify_data)
  ;; CHECK-NEXT:          )
  ;; CHECK-NEXT:         )
  ;; CHECK-NEXT:        )
  ;; CHECK-NEXT:       )
  ;; CHECK-NEXT:      )
  ;; CHECK-NEXT:      (block
  ;; CHECK-NEXT:       (if
  ;; CHECK-NEXT:        (i32.eq
  ;; CHECK-NEXT:         (global.get $__asyncify_state)
  ;; CHECK-NEXT:         (i32.const 0)
  ;; CHECK-NEXT:        )
  ;; CHECK-NEXT:        (block
  ;; CHECK-NEXT:         (local.set $4
  ;; CHECK-NEXT:          (local.get $dead0)
  ;; CHECK-NEXT:         )
  ;; CHECK-NEXT:         (drop
  ;; CHECK-NEXT:          (local.get $4)
  ;; CHECK-NEXT:         )
  ;; CHECK-NEXT:         (local.set $5
  ;; CHECK-NEXT:          (local.get $dead1)
  ;; CHECK-NEXT:         )
  ;; CHECK-NEXT:         (drop
  ;; CHECK-NEXT:          (local.get $5)
  ;; CHECK-NEXT:         )
  ;; CHECK-NEXT:        )
  ;; CHECK-NEXT:       )
  ;; CHECK-NEXT:       (nop)
  ;; CHECK-NEXT:       (nop)
  ;; CHECK-NEXT:       (nop)
  ;; CHECK-NEXT:       (if
  ;; CHECK-NEXT:        (if (result i32)
  ;; CHECK-NEXT:         (i32.eq
  ;; CHECK-NEXT:          (global.get $__asyncify_state)
  ;; CHECK-NEXT:          (i32.const 0)
  ;; CHECK-NEXT:         )
  ;; CHECK-NEXT:         (i32.const 1)
  ;; CHECK-NEXT:         (i32.eq
  ;; CHECK-NEXT:          (local.get $9)
  ;; CHECK-NEXT:          (i32.const 0)
  ;; CHECK-NEXT:         )
  ;; CHECK-NEXT:        )
  ;; CHECK-NEXT:        (block
  ;; CHECK-NEXT:         (call $import)
  ;; CHECK-NEXT:         (if
  ;; CHECK-NEXT:          (i32.eq
  ;; CHECK-NEXT:           (global.get $__asyncify_state)
  ;; CHECK-NEXT:           (i32.const 1)
  ;; CHECK-NEXT:          )
  ;; CHECK-NEXT:          (br $__asyncify_unwind
  ;; CHECK-NEXT:           (i32.const 0)
  ;; CHECK-NEXT:          )
  ;; CHECK-NEXT:         )
  ;; CHECK-NEXT:        )
  ;; CHECK-NEXT:       )
  ;; CHECK-NEXT:       (if
  ;; CHECK-NEXT:        (i32.eq
  ;; CHECK-NEXT:         (global.get $__asyncify_state)
  ;; CHECK-NEXT:         (i32.const 0)
  ;; CHECK-NEXT:        )
  ;; CHECK-NEXT:        (block
  ;; CHECK-NEXT:         (local.set $6
  ;; CHECK-NEXT:          (local.get $live0)
  ;; CHECK-NEXT:         )
  ;; CHECK-NEXT:         (drop
  ;; CHECK-NEXT:          (local.get $6)
  ;; CHECK-NEXT:         )
  ;; CHECK-NEXT:         (local.set $7
  ;; CHECK-NEXT:          (local.get $live1)
  ;; CHECK-NEXT:         )
  ;; CHECK-NEXT:         (drop
  ;; CHECK-NEXT:          (local.get $7)
  ;; CHECK-NEXT:         )
  ;; CHECK-NEXT:        )
  ;; CHECK-NEXT:       )
  ;; CHECK-NEXT:       (nop)
  ;; CHECK-NEXT:       (nop)
  ;; CHECK-NEXT:       (nop)
  ;; CHECK-NEXT:      )
  ;; CHECK-NEXT:     )
  ;; CHECK-NEXT:     (return)
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (block
  ;; CHECK-NEXT:   (i32.store
  ;; CHECK-NEXT:    (i64.load
  ;; CHECK-NEXT:     (global.get $__asyncify_data)
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:    (local.get $8)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:   (i64.store
  ;; CHECK-NEXT:    (global.get $__asyncify_data)
  ;; CHECK-NEXT:    (i64.add
  ;; CHECK-NEXT:     (i64.load
  ;; CHECK-NEXT:      (global.get $__asyncify_data)
  ;; CHECK-NEXT:     )
  ;; CHECK-NEXT:     (i64.const 4)
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (block
  ;; CHECK-NEXT:   (local.set $11
  ;; CHECK-NEXT:    (i64.load
  ;; CHECK-NEXT:     (global.get $__asyncify_data)
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:   (i32.store
  ;; CHECK-NEXT:    (local.get $11)
  ;; CHECK-NEXT:    (local.get $live0)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:   (i32.store offset=4
  ;; CHECK-NEXT:    (local.get $11)
  ;; CHECK-NEXT:    (local.get $live1)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:   (i64.store
  ;; CHECK-NEXT:    (global.get $__asyncify_data)
  ;; CHECK-NEXT:    (i64.add
  ;; CHECK-NEXT:     (i64.load
  ;; CHECK-NEXT:      (global.get $__asyncify_data)
  ;; CHECK-NEXT:     )
  ;; CHECK-NEXT:     (i64.const 8)
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $liveness2 (param $live0 i32) (param $dead0 i32)
    (local $live1 i32)
    (local $dead1 i32)
    (drop (local.get $dead0))
    (drop (local.get $dead1))
    (call $import)
    (drop (local.get $live0))
    (drop (local.get $live1))
  )
  ;; CHECK:      (func $liveness3 (param $live0 i32) (param $dead0 i32)
  ;; CHECK-NEXT:  (local $live1 i32)
  ;; CHECK-NEXT:  (local $dead1 i32)
  ;; CHECK-NEXT:  (local $4 i32)
  ;; CHECK-NEXT:  (local $5 i32)
  ;; CHECK-NEXT:  (local $6 i32)
  ;; CHECK-NEXT:  (local $7 i32)
  ;; CHECK-NEXT:  (local $8 i64)
  ;; CHECK-NEXT:  (local $9 i64)
  ;; CHECK-NEXT:  (if
  ;; CHECK-NEXT:   (i32.eq
  ;; CHECK-NEXT:    (global.get $__asyncify_state)
  ;; CHECK-NEXT:    (i32.const 2)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:   (block
  ;; CHECK-NEXT:    (i64.store
  ;; CHECK-NEXT:     (global.get $__asyncify_data)
  ;; CHECK-NEXT:     (i64.add
  ;; CHECK-NEXT:      (i64.load
  ;; CHECK-NEXT:       (global.get $__asyncify_data)
  ;; CHECK-NEXT:      )
  ;; CHECK-NEXT:      (i64.const -8)
  ;; CHECK-NEXT:     )
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:    (local.set $8
  ;; CHECK-NEXT:     (i64.load
  ;; CHECK-NEXT:      (global.get $__asyncify_data)
  ;; CHECK-NEXT:     )
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:    (local.set $live0
  ;; CHECK-NEXT:     (i32.load
  ;; CHECK-NEXT:      (local.get $8)
  ;; CHECK-NEXT:     )
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:    (local.set $live1
  ;; CHECK-NEXT:     (i32.load offset=4
  ;; CHECK-NEXT:      (local.get $8)
  ;; CHECK-NEXT:     )
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (local.set $6
  ;; CHECK-NEXT:   (block $__asyncify_unwind (result i32)
  ;; CHECK-NEXT:    (block
  ;; CHECK-NEXT:     (block
  ;; CHECK-NEXT:      (if
  ;; CHECK-NEXT:       (i32.eq
  ;; CHECK-NEXT:        (global.get $__asyncify_state)
  ;; CHECK-NEXT:        (i32.const 2)
  ;; CHECK-NEXT:       )
  ;; CHECK-NEXT:       (block
  ;; CHECK-NEXT:        (i64.store
  ;; CHECK-NEXT:         (global.get $__asyncify_data)
  ;; CHECK-NEXT:         (i64.add
  ;; CHECK-NEXT:          (i64.load
  ;; CHECK-NEXT:           (global.get $__asyncify_data)
  ;; CHECK-NEXT:          )
  ;; CHECK-NEXT:          (i64.const -4)
  ;; CHECK-NEXT:         )
  ;; CHECK-NEXT:        )
  ;; CHECK-NEXT:        (local.set $7
  ;; CHECK-NEXT:         (i32.load
  ;; CHECK-NEXT:          (i64.load
  ;; CHECK-NEXT:           (global.get $__asyncify_data)
  ;; CHECK-NEXT:          )
  ;; CHECK-NEXT:         )
  ;; CHECK-NEXT:        )
  ;; CHECK-NEXT:       )
  ;; CHECK-NEXT:      )
  ;; CHECK-NEXT:      (block
  ;; CHECK-NEXT:       (if
  ;; CHECK-NEXT:        (if (result i32)
  ;; CHECK-NEXT:         (i32.eq
  ;; CHECK-NEXT:          (global.get $__asyncify_state)
  ;; CHECK-NEXT:          (i32.const 0)
  ;; CHECK-NEXT:         )
  ;; CHECK-NEXT:         (i32.const 1)
  ;; CHECK-NEXT:         (i32.eq
  ;; CHECK-NEXT:          (local.get $7)
  ;; CHECK-NEXT:          (i32.const 0)
  ;; CHECK-NEXT:         )
  ;; CHECK-NEXT:        )
  ;; CHECK-NEXT:        (block
  ;; CHECK-NEXT:         (call $import)
  ;; CHECK-NEXT:         (if
  ;; CHECK-NEXT:          (i32.eq
  ;; CHECK-NEXT:           (global.get $__asyncify_state)
  ;; CHECK-NEXT:           (i32.const 1)
  ;; CHECK-NEXT:          )
  ;; CHECK-NEXT:          (br $__asyncify_unwind
  ;; CHECK-NEXT:           (i32.const 0)
  ;; CHECK-NEXT:          )
  ;; CHECK-NEXT:         )
  ;; CHECK-NEXT:        )
  ;; CHECK-NEXT:       )
  ;; CHECK-NEXT:       (if
  ;; CHECK-NEXT:        (i32.eq
  ;; CHECK-NEXT:         (global.get $__asyncify_state)
  ;; CHECK-NEXT:         (i32.const 0)
  ;; CHECK-NEXT:        )
  ;; CHECK-NEXT:        (block
  ;; CHECK-NEXT:         (local.set $4
  ;; CHECK-NEXT:          (local.get $live0)
  ;; CHECK-NEXT:         )
  ;; CHECK-NEXT:         (drop
  ;; CHECK-NEXT:          (local.get $4)
  ;; CHECK-NEXT:         )
  ;; CHECK-NEXT:        )
  ;; CHECK-NEXT:       )
  ;; CHECK-NEXT:       (nop)
  ;; CHECK-NEXT:       (if
  ;; CHECK-NEXT:        (if (result i32)
  ;; CHECK-NEXT:         (i32.eq
  ;; CHECK-NEXT:          (global.get $__asyncify_state)
  ;; CHECK-NEXT:          (i32.const 0)
  ;; CHECK-NEXT:         )
  ;; CHECK-NEXT:         (i32.const 1)
  ;; CHECK-NEXT:         (i32.eq
  ;; CHECK-NEXT:          (local.get $7)
  ;; CHECK-NEXT:          (i32.const 1)
  ;; CHECK-NEXT:         )
  ;; CHECK-NEXT:        )
  ;; CHECK-NEXT:        (block
  ;; CHECK-NEXT:         (call $import)
  ;; CHECK-NEXT:         (if
  ;; CHECK-NEXT:          (i32.eq
  ;; CHECK-NEXT:           (global.get $__asyncify_state)
  ;; CHECK-NEXT:           (i32.const 1)
  ;; CHECK-NEXT:          )
  ;; CHECK-NEXT:          (br $__asyncify_unwind
  ;; CHECK-NEXT:           (i32.const 1)
  ;; CHECK-NEXT:          )
  ;; CHECK-NEXT:         )
  ;; CHECK-NEXT:        )
  ;; CHECK-NEXT:       )
  ;; CHECK-NEXT:       (if
  ;; CHECK-NEXT:        (i32.eq
  ;; CHECK-NEXT:         (global.get $__asyncify_state)
  ;; CHECK-NEXT:         (i32.const 0)
  ;; CHECK-NEXT:        )
  ;; CHECK-NEXT:        (block
  ;; CHECK-NEXT:         (local.set $5
  ;; CHECK-NEXT:          (local.get $live1)
  ;; CHECK-NEXT:         )
  ;; CHECK-NEXT:         (drop
  ;; CHECK-NEXT:          (local.get $5)
  ;; CHECK-NEXT:         )
  ;; CHECK-NEXT:        )
  ;; CHECK-NEXT:       )
  ;; CHECK-NEXT:       (nop)
  ;; CHECK-NEXT:      )
  ;; CHECK-NEXT:     )
  ;; CHECK-NEXT:     (return)
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (block
  ;; CHECK-NEXT:   (i32.store
  ;; CHECK-NEXT:    (i64.load
  ;; CHECK-NEXT:     (global.get $__asyncify_data)
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:    (local.get $6)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:   (i64.store
  ;; CHECK-NEXT:    (global.get $__asyncify_data)
  ;; CHECK-NEXT:    (i64.add
  ;; CHECK-NEXT:     (i64.load
  ;; CHECK-NEXT:      (global.get $__asyncify_data)
  ;; CHECK-NEXT:     )
  ;; CHECK-NEXT:     (i64.const 4)
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (block
  ;; CHECK-NEXT:   (local.set $9
  ;; CHECK-NEXT:    (i64.load
  ;; CHECK-NEXT:     (global.get $__asyncify_data)
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:   (i32.store
  ;; CHECK-NEXT:    (local.get $9)
  ;; CHECK-NEXT:    (local.get $live0)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:   (i32.store offset=4
  ;; CHECK-NEXT:    (local.get $9)
  ;; CHECK-NEXT:    (local.get $live1)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:   (i64.store
  ;; CHECK-NEXT:    (global.get $__asyncify_data)
  ;; CHECK-NEXT:    (i64.add
  ;; CHECK-NEXT:     (i64.load
  ;; CHECK-NEXT:      (global.get $__asyncify_data)
  ;; CHECK-NEXT:     )
  ;; CHECK-NEXT:     (i64.const 8)
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $liveness3 (param $live0 i32) (param $dead0 i32)
    (local $live1 i32)
    (local $dead1 i32)
    (call $import)
    (drop (local.get $live0))
    (call $import)
    (drop (local.get $live1))
  )
  ;; CHECK:      (func $liveness4 (param $live0 i32) (param $dead0 i32)
  ;; CHECK-NEXT:  (local $2 i32)
  ;; CHECK-NEXT:  (local $3 i32)
  ;; CHECK-NEXT:  (local $4 i32)
  ;; CHECK-NEXT:  (local $5 i64)
  ;; CHECK-NEXT:  (local $6 i64)
  ;; CHECK-NEXT:  (if
  ;; CHECK-NEXT:   (i32.eq
  ;; CHECK-NEXT:    (global.get $__asyncify_state)
  ;; CHECK-NEXT:    (i32.const 2)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:   (block
  ;; CHECK-NEXT:    (i64.store
  ;; CHECK-NEXT:     (global.get $__asyncify_data)
  ;; CHECK-NEXT:     (i64.add
  ;; CHECK-NEXT:      (i64.load
  ;; CHECK-NEXT:       (global.get $__asyncify_data)
  ;; CHECK-NEXT:      )
  ;; CHECK-NEXT:      (i64.const -4)
  ;; CHECK-NEXT:     )
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:    (local.set $5
  ;; CHECK-NEXT:     (i64.load
  ;; CHECK-NEXT:      (global.get $__asyncify_data)
  ;; CHECK-NEXT:     )
  ;; CHECK-NEXT:    )
  ;; CHECK-NE