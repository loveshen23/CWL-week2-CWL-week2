;; NOTE: Assertions have been generated by update_lit_checks.py --all-items and should not be edited.
;; NOTE: This test was ported using port_passes_tests_to_lit.py and could be cleaned up.

;; RUN: foreach %s %t wasm-opt --coalesce-locals -S -o - | filecheck %s

(module
  (memory 10)
  ;; CHECK:      (type $2 (func))

  ;; CHECK:      (type $none_=>_i32 (func (result i32)))

  ;; CHECK:      (type $4 (func (param i32)))

  ;; CHECK:      (type $FUNCSIG$iii (func (param i32 i32) (result i32)))

  ;; CHECK:      (type $f64_i32_=>_i64 (func (param f64 i32) (result i64)))

  ;; CHECK:      (type $3 (func (param i32 f32)))

  ;; CHECK:      (type $FUNCSIG$iiii (func (param i32 i32 i32) (result i32)))
  (type $FUNCSIG$iiii (func (param i32 i32 i32) (result i32)))
  (type $FUNCSIG$iii (func (param i32 i32) (result i32)))
  (type $2 (func))
  (type $3 (func (param i32 f32)))
  (type $4 (func (param i32)))
  (import $_emscripten_autodebug_i32 "env" "_emscripten_autodebug_i32" (param i32 i32) (result i32))
  (import $get "env" "get" (result i32))
  (import $set "env" "set" (param i32))
  ;; CHECK:      (type $i32_=>_i32 (func (param i32) (result i32)))

  ;; CHECK:      (type $i32_i32_=>_none (func (param i32 i32)))

  ;; CHECK:      (type $none_=>_f64 (func (result f64)))

  ;; CHECK:      (import "env" "_emscripten_autodebug_i32" (func $_emscripten_autodebug_i32 (param i32 i32) (result i32)))

  ;; CHECK:      (import "env" "get" (func $get (result i32)))

  ;; CHECK:      (import "env" "set" (func $set (param i32)))

  ;; CHECK:      (memory $0 10)

  ;; CHECK:      (func $nothing-to-do
  ;; CHECK-NEXT:  (local $0 i32)
  ;; CHECK-NEXT:  (nop)
  ;; CHECK-NEXT: )
  (func $nothing-to-do (type $2)
    (local $x i32)
    (nop)
  )
  ;; CHECK:      (func $merge
  ;; CHECK-NEXT:  (local $0 i32)
  ;; CHECK-NEXT:  (nop)
  ;; CHECK-NEXT: )
  (func $merge (type $2)
    (local $x i32)
    (local $y i32)
    (nop)
  )
  ;; CHECK:      (func $leave-type
  ;; CHECK-NEXT:  (local $0 i32)
  ;; CHECK-NEXT:  (local $1 f32)
  ;; CHECK-NEXT:  (nop)
  ;; CHECK-NEXT: )
  (func $leave-type (type $2)
    (local $x i32)
    (local $y f32)
    (nop)
  )
  ;; CHECK:      (func $leave-interfere
  ;; CHECK-NEXT:  (local $0 i32)
  ;; CHECK-NEXT:  (local $1 i32)
  ;; CHECK-NEXT:  (local.set $0
  ;; CHECK-NEXT:   (i32.const 0)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (local.set $1
  ;; CHECK-NEXT:   (i32.const 1)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (local.get $0)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (local.get $1)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $leave-interfere (type $2)
    (local $x i32)
    (local $y i32)
    (local.set $x
      (i32.const 0)
    )
    (local.set $y
      (i32.const 1)
    )
    (drop
      (local.get $x)
    )
    (drop
      (local.get $y)
    )
  )
  ;; CHECK:      (func $almost-interfere
  ;; CHECK-NEXT:  (local $0 i32)
  ;; CHECK-NEXT:  (local.set $0
  ;; CHECK-NEXT:   (i32.const 0)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (local.get $0)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (local.set $0
  ;; CHECK-NEXT:   (i32.const 0)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (local.get $0)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $almost-interfere (type $2)
    (local $x i32)
    (local $y i32)
    (local.set $x
      (i32.const 0)
    )
    (drop
      (local.get $x)
    )
    (local.set $y
      (i32.const 0)
    )
    (drop
      (local.get $y)
    )
  )
  ;; CHECK:      (func $redundant-copy
  ;; CHECK-NEXT:  (local $0 i32)
  ;; CHECK-NEXT:  (local.set $0
  ;; CHECK-NEXT:   (i32.const 0)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (nop)
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (local.get $0)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $redundant-copy (type $2)
    (local $x i32)
    (local $y i32)
    (local.set $x
      (i32.const 0)
    )
    (local.set $y
      (local.get $x)
    )
    (drop
      (local.get $y)
    )
  )
  ;; CHECK:      (func $ineffective-store
  ;; CHECK-NEXT:  (local $0 i32)
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (i32.const 0)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (local.set $0
  ;; CHECK-NEXT:   (i32.const 0)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (local.get $0)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $ineffective-store (type $2)
    (local $x i32)
    (local.set $x
      (i32.const 0)
    )
    (local.set $x
      (i32.const 0)
    )
    (drop
      (local.get $x)
    )
  )
  ;; CHECK:      (func $block
  ;; CHECK-NEXT:  (local $0 i32)
  ;; CHECK-NEXT:  (block $block0
  ;; CHECK-NEXT:   (local.set $0
  ;; CHECK-NEXT:    (i32.const 0)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (local.get $0)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $block (type $2)
    (local $x i32)
    (block $block0
      (local.set $x
        (i32.const 0)
      )
    )
    (drop
      (local.get $x)
    )
  )
  ;; CHECK:      (func $see-both-sides
  ;; CHECK-NEXT:  (local $0 i32)
  ;; CHECK-NEXT:  (local $1 i32)
  ;; CHECK-NEXT:  (local.set $0
  ;; CHECK-NEXT:   (i32.const 0)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (block $block0
  ;; CHECK-NEXT:   (local.set $1
  ;; CHECK-NEXT:    (i32.const 1)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (local.get $0)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (local.get $1)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $see-both-sides (type $2)
    (local $x i32)
    (local $y i32)
    (local.set $x
      (i32.const 0)
    )
    (block $block0
      (local.set $y
        (i32.const 1)
      )
    )
    (drop
      (local.get $x)
    )
    (drop
      (local.get $y)
    )
  )
  ;; CHECK:      (func $see-br-and-ignore-dead
  ;; CHECK-NEXT:  (local $0 i32)
  ;; CHECK-NEXT:  (local.set $0
  ;; CHECK-NEXT:   (i32.const 0)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (block $block
  ;; CHECK-NEXT:   (br $block)
  ;; CHECK-NEXT:   (drop
  ;; CHECK-NEXT:    (i32.const 0)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:   (drop
  ;; CHECK-NEXT:    (i32.const 0)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:   (drop
  ;; CHECK-NEXT:    (i32.const -1)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (local.get $0)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $see-br-and-ignore-dead (type $2)
    (local $x i32)
    (local $y i32)
    (local.set $x
      (i32.const 0)
    )
    (block $block
      (br $block)
      (local.set $y
        (i32.const 0)
      )
      (drop
        (local.get $y)
      )
      (local.set $x
        (i32.const -1)
      )
    )
    (drop
      (local.get $x)
    )
  )
  ;; CHECK:      (func $see-block-body
  ;; CHECK-NEXT:  (local $0 i32)
  ;; CHECK-NEXT:  (local $1 i32)
  ;; CHECK-NEXT:  (local.set $0
  ;; CHECK-NEXT:   (i32.const 0)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (block $block
  ;; CHECK-NEXT:   (local.set $1
  ;; CHECK-NEXT:    (i32.const 1)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:   (drop
  ;; CHECK-NEXT:    (local.get $1)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:   (br $block)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (local.get $0)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $see-block-body (type $2)
    (local $x i32)
    (local $y i32)
    (local.set $x
      (i32.const 0)
    )
    (block $block
      (local.set $y
        (i32.const 1)
      )
      (drop
        (local.get $y)
      )
      (br $block)
    )
    (drop
      (local.get $x)
    )
  )
  ;; CHECK:      (func $zero-init
  ;; CHECK-NEXT:  (local $0 i32)
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (local.get $0)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (local.get $0)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $zero-init (type $2)
    (local $x i32)
    (local $y i32)
    (drop
      (local.get $x)
    )
    (drop
      (local.get $y)
    )
  )
  ;; CHECK:      (func $multi
  ;; CHECK-NEXT:  (local $0 i32)
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (local.get $0)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (local.get $0)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $multi (type $2)
    (local $x i32)
    (local $y i32)
    (local $z i32)
    (drop
      (local.get $y)
    )
    (drop
      (local.get $z)
    )
  )
  ;; CHECK:      (func $if-else
  ;; CHECK-NEXT:  (local $0 i32)
  ;; CHECK-NEXT:  (if
  ;; CHECK-NEXT:   (i32.const 0)
  ;; CHECK-NEXT:   (drop
  ;; CHECK-NEXT:    (local.get $0)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:   (drop
  ;; CHECK-NEXT:    (local.get $0)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $if-else (type $2)
    (local $x i32)
    (local $y i32)
    (if
      (i32.const 0)
      (drop
        (local.get $x)
      )
      (drop
        (local.get $y)
      )
    )
  )
  ;; CHECK:      (func $if-else-parallel
  ;; CHECK-NEXT:  (local $0 i32)
  ;; CHECK-NEXT:  (if
  ;; CHECK-NEXT:   (i32.const 0)
  ;; CHECK-NEXT:   (block $block1
  ;; CHECK-NEXT:    (local.set $0
  ;; CHECK-NEXT:     (i32.const 0)
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:    (drop
  ;; CHECK-NEXT:     (local.get $0)
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:   (block $block3
  ;; CHECK-NEXT:    (local.set $0
  ;; CHECK-NEXT:     (i32.const 1)
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:    (drop
  ;; CHECK-NEXT:     (local.get $0)
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $if-else-parallel (type $2)
    (local $x i32)
    (local $y i32)
    (if
      (i32.const 0)
      (block $block1
        (local.set $x
          (i32.const 0)
        )
        (drop
          (local.get $x)
        )
      )
      (block $block3
        (local.set $y
          (i32.const 1)
        )
        (drop
          (local.get $y)
        )
      )
    )
  )
  ;; CHECK:      (func $if-else-after
  ;; CHECK-NEXT:  (local $0 i32)
  ;; CHECK-NEXT:  (local $1 i32)
  ;; CHECK-NEXT:  (if
  ;; CHECK-NEXT:   (i32.const 0)
  ;; CHECK-NEXT:   (local.set $0
  ;; CHECK-NEXT:    (i32.const 0)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:   (local.set $1
  ;; CHECK-NEXT:    (i32.const 1)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (local.get $0)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (local.get $1)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $if-else-after (type $2)
    (local $x i32)
    (local $y i32)
    (if
      (i32.const 0)
      (local.set $x
        (i32.const 0)
      )
      (local.set $y
        (i32.const 1)
      )
    )
    (drop
      (local.get $x)
    )
    (drop
      (local.get $y)
    )
  )
  ;; CHECK:      (func $if-else-through
  ;; CHECK-NEXT:  (local $0 i32)
  ;; CHECK-NEXT:  (local $1 i32)
  ;; CHECK-NEXT:  (local.set $0
  ;; CHECK-NEXT:   (i32.const 0)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (local.set $1
  ;; CHECK-NEXT:   (i32.const 1)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (if
  ;; CHECK-NEXT:   (i32.const 0)
  ;; CHECK-NEXT:   (drop
  ;; CHECK-NEXT:    (i32.const 1)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:   (drop
  ;; CHECK