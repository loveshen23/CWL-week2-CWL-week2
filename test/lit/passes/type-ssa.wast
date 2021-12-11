;; NOTE: Assertions have been generated by update_lit_checks.py --all-items and should not be edited.
;; RUN: foreach %s %t wasm-opt           --type-ssa -all -S -o - | filecheck %s
;; RUN: foreach %s %t wasm-opt --nominal --type-ssa -all -S -o - | filecheck %s --check-prefix NOMNL

;; Test in both isorecursive and nominal modes to make sure we create the new
;; types properly in both.

;; Every struct.new here should get a new type.
(module
  ;; CHECK:      (type $struct (struct (field i32)))
  ;; NOMNL:      (type $struct (struct (field i32)))
  (type $struct (struct_subtype (field i32) data))

  ;; CHECK:      (type $none_=>_none (func))

  ;; CHECK:      (rec
  ;; CHECK-NEXT:  (type $struct$1 (struct_subtype (field i32) $struct))

  ;; CHECK:       (type $struct$2 (struct_subtype (field i32) $struct))

  ;; CHECK:       (type $struct$3 (struct_subtype (field i32) $struct))

  ;; CHECK:       (type $struct$4 (struct_subtype (field i32) $struct))

  ;; CHECK:       (type $struct$5 (struct_subtype (field i32) $struct))

  ;; CHECK:      (global $g (ref $struct) (struct.new $struct$4
  ;; CHECK-NEXT:  (i32.const 42)
  ;; CHECK-NEXT: ))
  ;; NOMNL:      (type $none_=>_none (func))

  ;; NOMNL:      (type $struct$4 (struct_subtype (field i32) $struct))

  ;; NOMNL:      (type $struct$5 (struct_subtype (field i32) $struct))

  ;; NOMNL:      (type $struct$1 (struct_subtype (field i32) $struct))

  ;; NOMNL:      (type $struct$2 (struct_subtype (field i32) $struct))

  ;; NOMNL:      (type $struct$3 (struct_subtype (field i32) $struct))

  ;; NOMNL:      (global $g (ref $struct) (struct.new $struct$4
  ;; NOMNL-NEXT:  (i32.const 42)
  ;; NOMNL-NEXT: ))
  (global $g (ref $struct) (struct.new $struct
    (i32.const 42)
  ))

  ;; CHECK:      (global $h (ref $struct) (struct.new $struct$5
  ;; CHECK-NEXT:  (i32.const 42)
  ;; CHECK-NEXT: ))
  ;; NOMNL:      (global $h (ref $struct) (struct.new $struct$5
  ;; NOMNL-NEXT:  (i32.const 42)
  ;; NOMNL-NEXT: ))
  (global $h (ref $struct) (struct.new $struct
    (i32.const 42)
  ))

  ;; CHECK:      (func $foo (type $none_=>_none)
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (struct.new_default $struct$1)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (struct.new $struct$2
  ;; CHECK-NEXT:    (i32.const 10)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  ;; NOMNL:      (func $foo (type $none_=>_none)
  ;; NOMNL-NEXT:  (drop
  ;; NOMNL-NEXT:   (struct.new_default $struct$1)
  ;; NOMNL-NEXT:  )
  ;; NOMNL-NEXT:  (drop
  ;; NOMNL-NEXT:   (struct.new $struct$2
  ;; NOMNL-NEXT:    (i32.const 10)
  ;; NOMNL-NEXT:   )
  ;; NOMNL-NEXT:  )
  ;; NOMNL-NEXT: )
  (func $foo
    (drop
      (struct.new_default $struct)
    )
    (drop
      (struct.new $struct
        (i32.const 10)
      )
    )
  )

  ;; CHECK:      (func $another-func (type $none_=>_none)
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (struct.new $struct$3
  ;; CHECK-NEXT:    (i32.const 100)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  ;; NOMNL:      (func $another-func (type $none_=>_none)
  ;; NOMNL-NEXT:  (drop
  ;; NOMNL-NEXT:   (struct.new $struct$3
  ;; NOMNL-NEXT:    (i32.const 100)
  ;; NOMNL-NEXT:   )
  ;; NOMNL-NEXT:  )
  ;; NOMNL-NEXT: )
  (func $another-func
    (drop
      (struct.new $struct
        (i32.const 100)
      )
    )
  )
)

;; Some of these are uninteresting and should not get a new type.
(module
  ;; CHECK:      (type $anyref_arrayref_=>_none (func (param anyref arrayref)))

  ;; CHECK:      (type $struct (struct (field anyref)))
  ;; NOMNL:      (type $anyref_arrayref_=>_none (func (param anyref arrayref)))

  ;; NOMNL:      (type $struct (struct (field anyref)))
  (type $struct (struct_subtype (field (ref null any)) data))

  ;; CHECK:      (rec
  ;; CHECK-NEXT:  (type $struct$1 (struct_subtype (field anyref) $struct))

  ;; CHECK:       (type $struct$2 (struct_subtype (field anyref) $struct))

  ;; CHECK:       (type $struct$3 (struct_subtype (field anyref) $struct))

  ;; CHECK:      (func $foo (type $anyref_arrayref_=>_none) (param $any anyref) (param $array arrayref)
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (struct.new_default $struct$1)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (struct.new $struct$2
  ;; CHECK-NEXT:    (ref.null none)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (struct.new $struct
  ;; CHECK-NEXT:    (local.get $any)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (struct.new $struct$3
  ;; CHECK-NEXT:    (local.get $array)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (block ;; (replaces something unreachable we can't emit)
  ;; CHECK-NEXT:    (drop
  ;; CHECK-NEXT:     (unreachable)
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:    (unreachable)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  ;; NOMNL:      (type $struct$1 (struct_subtype (field anyref) $struct))

  ;; NOMNL:      (type $struct$2 (struct_subtype (field anyref) $struct))

  ;; NOMNL:      (type $struct$3 (struct_subtype (field anyref) $struct))

  ;; NOMNL:      (func $foo (type $anyref_arrayref_=>_none) (param $any anyref) (param $array arrayref)
  ;; NOMNL-NEXT:  (drop
  ;; NOMNL-NEXT:   (struct.new_default $struct$1)
  ;; NOMNL-NEXT:  )
  ;; NOMNL-NEXT:  (drop
  ;; NOMNL-NEXT:   (struct.new $struct$2
  ;; NOMNL-NEXT:    (ref.null none)
  ;; NOMNL-NEXT:   )
  ;; NOMNL-NEXT:  )
  ;; NOMNL-NEXT:  (drop
  ;; NOMNL-NEXT:   (struct.new $struct
  ;; NOMNL-NEXT:    (local.get $any)
  ;; NOMNL-NEXT:   )
  ;; NOMNL-NEXT:  )
  ;; NOMNL-NEXT:  (drop
  ;; NOMNL-NEXT:   (struct.new $struct$3
  ;; NOMNL-NEXT:    (local.get $array)
  ;; NOMNL-NEXT:   )
  ;; NOMNL-NEXT:  )
  ;; NOMNL-NEXT:  (drop
  ;; NOMNL-NEXT:   (block ;; (replaces something unreachable we can't emit)
  ;; NOMNL-NEXT:    (drop
  ;; NOMNL-NEXT:     (unreachable)
  ;; NOMNL-NEXT:    )
  ;; NOMNL-NEXT:    (unreachable)
  ;; NOMNL-NEXT:   )
  ;; NOMNL-NEXT:  )
  ;; NOMNL-NEXT: )
  (func $foo (param $any (ref null any)) (param $array (ref null array))
    ;; A null is interesting.
    (drop
      (struct.new_default $struct)
    )
    (drop
      (struct.new $struct
        (ref.null none)
      )
    )
    ;; An unknown value of the same type is uninteresting.
    (drop
      (struct.new $struct
        (local.get $any)
      )
    )
    ;; But a more refined type piques our interest.
    (drop
      (struct.new $struct
        (local.get $array)
      )
    )
    ;; An unreachable is boring.
    (drop
      (struct.new $struct
        (unreachable)
      )
    )
  )
)

(module
  ;; CHECK:      (type $array (array (mut anyref)))
  ;; NOMNL:      (type $array (array (mut anyref)))
  (type $array (array (mut (ref null any))))

  ;; CHECK:      (type $ref|i31|_anyref_=>_none (func (param (ref i31) anyref)))

  ;; CHECK:      (type $array-func (array (mut funcref)))
  ;; NOMNL:      (type $ref|i31|_anyref_=>_none (func (param (ref i31) anyref)))

  ;; NOMNL:      (type $array$1 (array_subtype (mut anyref) $array))

  ;; NOMNL:      (type $array$2 (array_subtype (mut anyref) $array))

  ;; NOMNL:      (type $array$3 (array_subtype (mut anyref) $array))

  ;; NOMNL:      (type $none_=>_none (func))

  ;; NOMNL:      (type $array-func (array (mut funcref)))
  (type $array-func (array (mut funcref)))


  ;; CHECK:      (rec
  ;; CHECK-NEXT:  (type $array$1 (array_subtype (mut anyref) $array))

  ;; CHECK:       (type $array$2 (array_subtype (mut anyref) $array))

  ;; CHECK:       (type $array$3 (array_subtype (mut anyref) $array))

  ;; CHECK:       (type $array-func$4 (array_subtype (mut funcref) $array-func))

  ;; CHECK:       (type $array$5 (array_subtype (mut anyref) $array))

  ;; CHECK:       (type $array$6 (array_subtype (mut anyref) $array))

  ;; CHECK:      (type $none_=>_none (func))

  ;; CHECK:      (elem func $array.new)
  ;; NOMNL:      (type $array-func$4 (array_subtype (mut funcref) $array-func))

  ;; NOMNL:      (type $array$5 (array_subtype (mut anyref) $array))

  ;; NOMNL:      (type $array$6 (array_subtype (mut anyref) $array))

  ;; NOMNL:      (elem func $array.new)
  (elem func $array.new)

  ;; CHECK:      (func $array.new (type $ref|i31|_anyref_=>_none) (param $refined (ref i31)) (param $null-any anyref)
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (array.new_default $array$1
  ;; CHECK-NEXT:    (i32.const 5)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (array.new $array$2
  ;; CHECK-NEXT:    (ref.null none)
  ;; CHECK-NEXT:    (i32.const 5)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (array.new $array$3
  ;; CHECK-NEXT:    (local.get $refined)
  ;; CHECK-NEXT:    (i32.const 5)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (array.new $array
  ;; CHECK-NEXT:    (local.get $null-any)
  ;; CHECK-NEXT:    (i32.const 5)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  ;; NOMNL:      (func $array.new (type $ref|i31|_anyref_=>_none) (param $refined (ref i31)) (param $null-any anyref)
  ;; NOMNL-NEXT:  (drop
  ;; NOMNL-NEXT:   (array.new_default $array$1
  ;; NOMNL-NEXT:    (i32.const 5)
  ;; NOMNL-NEXT:   )
  ;; NOMNL-NEXT:  )
  ;; NOMNL-NEXT:  (drop
  ;; NOMNL-NEXT:   (array.new $array$2
  ;; NOMNL-NEXT:    (ref.null none)
  ;; NOMNL-NEXT:    (i32.const 5)
  ;; NOMNL-NEXT:   )
  ;; NOMNL-NEXT:  )
  ;; NOMNL-NEXT:  (drop
  ;; NOMNL-NEXT:   (array.new $array$3
  ;; NOMNL-NEXT:    (local.get $refined)
  ;; NOMNL-NEXT:    (i32.const 5)
  ;; NOMNL-NEXT:   )
  ;; NOMNL-NEXT:  )
  ;; NOMNL-NEXT:  (drop
  ;; NOMNL-NEXT:   (array.new $array
  ;; NOMNL-NEXT:    (local.get $null-any)
  ;; NOMNL-NEXT:    (i32.const 5)
  ;; NOMNL-NEXT:   )
  ;; NOMNL-NEXT:  )
  ;; NOMNL-NEXT: )
  (func $array.new (param $refined (ref i31)) (param $null-any (ref null any))
    ;; Default null, an interesting value, so we get a new type.
    (drop
      (array.new_default $array
        (i32.const 5)
      )
    )
    ;; Given null, also interesting.
    (drop
      (array.new $array
        (ref.null none)
        (i32.const 5)
      )
    )
    ;; More refined type, interesting.
    (drop
      (array.new $array
        (local.get $refined)
        (i32.const 5)
      )
    )
    ;; Same type as declared - boring, no new type.
    (drop
      (array.new $array
        (local.get $null-any)
        (i32.const 5)
      )
    )
  )

  ;; CHECK:      (func $array.new_seg (type $none_=>_none)
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (array.new_elem $array-func$4 0
  ;; CHECK-NEXT:    (i32.const 0)
  ;; CHECK-NEXT:    (i32.const 3)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  ;; NOMNL:      (func $array.new_seg (type $none_=>_none)
  ;; NOMNL-NEXT:  (drop
  ;; NOMNL-NEXT:   (array.new_elem $array-func$4 0
  ;; NOMNL-NEXT:    (i32.const 0)
  ;; NOMNL-NEXT:    (i32.const 3)
  ;; NOMNL-NEXT:   )
  ;; NOMNL-NEXT:  )
  ;; NOMNL-NEXT: )
  (func $array.new_seg
    ;; We consider all new_elem to be interesting as we don't look at the elem
    ;; data yet.
    (drop
      (array.new_elem $array-func 0
        (i32.const 0)
        (i32.const 3)
      )
    )
  )

  ;; CHECK:      (func $array.new_fixed (type $ref|i31|_anyref_=>_none) (param $refined (ref i31)) (param $null-any anyref)
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (array.new_fixed $array$5
  ;; CHECK-NEXT:    (ref.null none)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (array.new_fixed $array$6
  ;; CHECK-NEXT:    (local.get $refined)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (array.new_fixed $array
  ;; CHECK-NEXT:    (local.get $null-any)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (array.new_fixed $array
  ;; CHECK-NEXT:    (local.get $refined)
  ;; CHECK-NEXT:    (local.get $null-any)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  ;; NOMNL:      (func $array.new_fixed (type $ref|i31|_anyref_=>_none) (param $refined (ref i31)) (param $null-any anyref)
  ;; NOMNL-NEXT:  (drop
  ;; NOMNL-NEXT:   (array.new_fixed $array$5
  ;; NOMNL-NEXT:    (ref.null none)
  ;; NOMNL-NEXT:   )
  ;; NOMNL-NEXT:  )
  ;; NOMNL-NEXT:  (drop
  ;; NOMNL-NEXT:   (array.new_fixed $array$6
  ;; NOMNL-NEXT:    (local.get $refined)
  ;; NOMNL-NEXT:   )
  ;; NOMNL-NEXT:  )
  ;; NOMNL-NEXT:  (drop
  ;; NOMNL-NEXT:   (array.new_fixed $array
  ;; NOMNL-NEXT:    (local.get $null-any)
  ;; NOMNL-NEXT:   )
  ;; NOMNL-NEXT:  )
  ;; NOMNL-NEXT:  (drop
  ;; NOMNL-NEXT:   (array.new_fixed $array
  ;; NOMNL-NEXT:    (local.get $refined)
  ;; NOMNL-NEXT:    (local.get $null-any)
  ;; NOMNL-NEXT:   )
  ;; NOMNL-NEXT:  )
  ;; NOMNL-NEXT: )
  (func $array.new_fixed (param $refined (ref i31)) (param $null-any (ref null any))
    ;; Null, interesting, so we get a new type.
    (drop
      (array.new_fixed $array
        (ref.null none)
      )
    )
    ;; More refined type, interesting.
    (drop
      (array.new_fixed $array
        (local.get $refined)
      )
    )
    ;; Same type as declared - boring, no new type.
    (drop
      (array.new_fixed $array
        (local.get $null-any)
      )
    )
    ;; Mixture of boring and interesting => boring (since we infer a single type
    ;; for the entire array).
    (drop
      (array.new_fixed $array
        (local.get $refined)
        (local.get $null-any)
      )
    )
  )
)
