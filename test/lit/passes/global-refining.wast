
;; NOTE: Assertions have been generated by update_lit_checks.py --all-items and should not be edited.

;; RUN: foreach %s %t wasm-opt --nominal --global-refining                -all -S -o - | filecheck %s
;; RUN: foreach %s %t wasm-opt --nominal --global-refining --closed-world -all -S -o - | filecheck %s --check-prefix=CLOSD

(module
  ;; Globals with no assignments aside from their initial values. The first is a
  ;; null, so we can optimize to a nullfuncref. The second is a ref.func which
  ;; lets us refine to the specific function type.
  ;; CHECK:      (type $foo_t (func))
  ;; CLOSD:      (type $foo_t (func))
  (type $foo_t (func))

  ;; CHECK:      (global $func-null-init (mut nullfuncref) (ref.null nofunc))
  ;; CLOSD:      (global $func-null-init (mut nullfuncref) (ref.null nofunc))
  (global $func-null-init (mut funcref) (ref.null $foo_t))
  ;; CHECK:      (global $func-func-init (mut (ref $foo_t)) (ref.func $foo))
  ;; CLOSD:      (global $func-func-init (mut (ref $foo_t)) (ref.func $foo))
  (global $func-func-init (mut funcref) (ref.func $foo))
  ;; CHECK:      (func $foo (type $foo_t)
  ;; CHECK-NEXT:  (nop)
  ;; CHECK-NEXT: )
  ;; CLOSD:      (func $foo (type $foo_t)
  ;; CLOSD-NEXT:  (nop)
  ;; CLOSD-NEXT: )
  (func $foo (type $foo_t))
)

(module
  ;; Globals with later assignments of null. The global with a function in its
  ;; init will update the null to allow it to refine.

  ;; CHECK:      (type $foo_t (func))
  ;; CLOSD:      (type $foo_t (func))
  (type $foo_t (func))

  ;; CHECK:      (global $func-null-init (mut nullfuncref) (ref.null nofunc))
  ;; CLOSD:      (global $func-null-init (mut nullfuncref) (ref.null nofunc))
  (global $func-null-init (mut funcref) (ref.null $foo_t))
  ;; CHECK:      (global $func-func-init (mut (ref null $foo_t)) (ref.func $foo))
  ;; CLOSD:      (global $func-func-init (mut (ref null $foo_t)) (ref.func $foo))
  (global $func-func-init (mut funcref) (ref.func $foo))

  ;; CHECK:      (func $foo (type $foo_t)
  ;; CHECK-NEXT:  (global.set $func-null-init
  ;; CHECK-NEXT:   (ref.null nofunc)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (global.set $func-func-init
  ;; CHECK-NEXT:   (ref.null nofunc)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  ;; CLOSD:      (func $foo (type $foo_t)
  ;; CLOSD-NEXT:  (global.set $func-null-init
  ;; CLOSD-NEXT:   (ref.null nofunc)
  ;; CLOSD-NEXT:  )
  ;; CLOSD-NEXT:  (global.set $func-func-init
  ;; CLOSD-NEXT:   (ref.null nofunc)
  ;; CLOSD-NEXT:  )
  ;; CLOSD-NEXT: )
  (func $foo (type $foo_t)
   (global.set $func-null-init (ref.null func))
   (global.set $func-func-init (ref.null $foo_t))
  )
)

(module
  ;; Globals with later assignments of something non-null. Both can be refined,
  ;; and the one with a non-null initial value can even become non-nullable.

  ;; CHECK:      (type $none_=>_none (func))

  ;; CHECK:      (global $func-null-init (mut (ref null $none_=>_none)) (ref.null nofunc))
  ;; CLOSD:      (type $none_=>_none (func))

  ;; CLOSD:      (global $func-null-init (mut (ref null $none_=>_none)) (ref.null nofunc))
  (global $func-null-init (mut funcref) (ref.null func))
  ;; CHECK:      (global $func-func-init (mut (ref $none_=>_none)) (ref.func $foo))
  ;; CLOSD:      (global $func-func-init (mut (ref $none_=>_none)) (ref.func $foo))
  (global $func-func-init (mut funcref) (ref.func $foo))

  ;; CHECK:      (elem declare func $foo)

  ;; CHECK:      (func $foo (type $none_=>_none)
  ;; CHECK-NEXT:  (global.set $func-null-init
  ;; CHECK-NEXT:   (ref.func $foo)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (global.set $func-func-init
  ;; CHECK-NEXT:   (ref.func $foo)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  ;; CLOSD:      (elem declare func $foo)

  ;; CLOSD:      (func $foo (type $none_=>_none)
  ;; CLOSD-NEXT:  (global.set $func-null-init
  ;; CLOSD-NEXT:   (ref.func $foo)
  ;; CLOSD-NEXT:  )
  ;; CLOSD-NEXT:  (global.set $func-func-init
  ;; CLOSD-NEXT:   (ref.func $foo)
  ;; CLOSD-NEXT:  )
  ;; CLOSD-NEXT: )
  (func $foo
   (global.set $func-null-init (ref.func $foo))
   (global.set $func-func-init (ref.func $foo))
  )
)

(module
  ;; A global with multiple later assignments. The refined type is more
  ;; specific than the original, but less than each of the non-null values.

  ;; CHECK:      (type $none_=>_none (func))

  ;; CHECK:      (type $struct (struct ))
  ;; CLOSD:      (type $none_=>_none (func))

  ;; CLOSD:      (type $struct (struct ))
  (type $struct (struct))
  (type $array (array i8))

  ;; CHECK:      (global $global (mut eqref) (ref.null none))
  ;; CLOSD:      (global $global (mut eqref) (ref.null none))
  (global $global (mut anyref) (ref.null any))

  ;; CHECK:      (func $foo (type $none_=>_none)
  ;; CHECK-NEXT:  (global.set $global
  ;; CHECK-NEXT:   (i31.new
  ;; CHECK-NEXT:    (i32.const 0)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (global.set $global
  ;; CHECK-NEXT:   (struct.new_default $struct)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (global.set $global
  ;; CHECK-NEXT:   (ref.null none)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (global.set $global
  ;; CHECK-NEXT:   (ref.null none)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (global.set $global
  ;; CHECK-NEXT:   (ref.null none)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  ;; CLOSD:      (func $foo (type $none_=>_none)
  ;; CLOSD-NEXT:  (global.set $global
  ;; CLOSD-NEXT:   (i31.new
  ;; CLOSD-NEXT:    (i32.const 0)
  ;; CLOSD-NEXT:   )
  ;; CLOSD-NEXT:  )
  ;; CLOSD-NEXT:  (global.set $global
  ;; CLOSD-NEXT:   (struct.new_default $struct)
  ;; CLOSD-NEXT:  )
  ;; CLOSD-NEXT:  (global.set $global
  ;; CLOSD-NEXT:   (ref.null none)
  ;; CLOSD-NEXT:  )
  ;; CLOSD-NEXT:  (global.set $global
  ;; CLOSD-NEXT:   (ref.null none)
  ;; CLOSD-NEXT:  )
  ;; CLOSD-NEXT:  (global.set $global
  ;; CLOSD-NEXT:   (ref.null none)
  ;; CLOSD-NEXT:  )
  ;; CLOSD-NEXT: )
  (func $foo
   (global.set $global (i31.new (i32.const 0)))
   (global.set $global (struct.new_default $struct))
   (global.set $global (ref.null eq))
   ;; These nulls will be updated.
   (global.set $global (ref.null i31))
   (global.set $global (ref.null $array))
  )
)

;; We can refine here, but as it is an export we only do so in open world.
(module
  ;; CHECK:      (type $none_=>_none (func))

  ;; CHECK:      (global $func-init (mut (ref $none_=>_none)) (ref.func $foo))
  ;; CLOSD:      (type $none_=>_none (func))

  ;; CLOSD:      (global $func-init (mut funcref) (ref.func $foo))
  (global $func-init (mut funcref) (ref.func $foo))

  ;; CHECK:      (export "global" (global $func-init))
  ;; CLOSD:      (export "global" (global $func-init))
  (export "global" (global $func-init))

  ;; CHECK:      (func $foo (type $none_=>_none)
  ;; CHECK-NEXT:  (nop)
  ;; CHECK-NEXT: )
  ;; CLOSD:      (func $foo (type $none_=>_none)
  ;; CLOSD-NEXT:  (nop)
  ;; CLOSD-NEXT: )
  (func $foo
    (nop)
  )
)