;; NOTE: Assertions have been generated by update_lit_checks.py --all-items and should not be edited.
;; RUN: foreach %s %t wasm-opt --nominal --gto --closed-world -all -S -o - | filecheck %s
;; (remove-unused-names is added to test fallthrough values without a block
;; name getting in the way)

(module
  ;; The struct here has three fields, and the second of them has no struct.set
  ;; which means we can make it immutable.

  ;; CHECK:      (type $struct (struct (field (mut funcref)) (field funcref) (field (mut funcref))))
  (type $struct (struct (field (mut funcref)) (field (mut funcref)) (field (mut funcref))))

  ;; CHECK:      (type $two-params (func (param (ref $struct) (ref $struct))))
  (type $two-params (func (param (ref $struct)) (param (ref $struct))))

  ;; Test that we update tag types properly.
  (table 0 funcref)

  ;; CHECK:      (type $ref|$struct|_=>_none (func (param (ref $struct))))

  ;; CHECK:      (type $none_=>_ref?|$struct| (func (result (ref null $struct))))

  ;; CHECK:      (type $ref?|$struct|_=>_none (func (param (ref null $struct))))

  ;; CHECK:      (table $0 0 funcref)

  ;; CHECK:      (elem declare func $func-two-params)

  ;; CHECK:      (tag $tag (param (ref $struct)))
  (tag $tag (param (ref $struct)))

  ;; CHECK:      (func $func (type $ref|$struct|_=>_none) (param $x (ref $struct))
  ;; CHECK-NEXT:  (local $temp (ref null $struct))
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (struct.new $struct
  ;; CHECK-NEXT:    (ref.null nofunc)
  ;; CHECK-NEXT:    (ref.null nofunc)
  ;; CHECK-NEXT:    (ref.null nofunc)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (struct.set $struct 0
  ;; CHECK-NEXT:   (local.get $x)
  ;; CHECK-NEXT:   (ref.null nofunc)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (struct.set $struct 2
  ;; CHECK-NEXT:   (local.get $x)
  ;; CHECK-NEXT:   (ref.null nofunc)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (local.set $temp
  ;; CHECK-NEXT:   (local.get $x)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (struct.get $struct 0
  ;; CHECK-NEXT:    (local.get $x)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (struct.get $struct 1
  ;; CHECK-NEXT:    (local.get $x)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $func (param $x (ref $struct))
    (local $temp (ref null $struct))
    ;; The presence of a struct.new does not prevent this optimization: we just
    ;; care about writes using struct.set.
    (drop
      (struct.new $struct
        (ref.null func)
        (ref.null func)
        (ref.null func)
      )
    )
    (struct.set $struct 0
      (local.get $x)
      (ref.null func)
    )
    (struct.set $struct 2
      (local.get $x)
      (ref.null func)
    )
    ;; Test that local types remain valid after our work (otherwise, we'd get a
    ;; validation error).
    (local.set $temp
      (local.get $x)
    )
    ;; Test that struct.get types remain valid after our work.
    (drop
      (struct.get $struct 0
        (local.get $x)
      )
    )
    (drop
      (struct.get $struct 1
        (local.get $x)
      )
    )
  )

  ;; CHECK:      (func $foo (type $none_=>_ref?|$struct|) (result (ref null $struct))
  ;; CHECK-NEXT:  (try $try
  ;; CHECK-NEXT:   (do
  ;; CHECK-NEXT:    (nop)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:   (catch $tag
  ;; CHECK-NEXT:    (return
  ;; CHECK-NEXT:     (pop (ref $struct))
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (ref.null none)
  ;; CHECK-NEXT: )
  (func $foo (result (ref null $struct))
    ;; Use a tag so that we test proper updating of its type after making
    ;; changes.
    (try
      (do
        (nop)
      )
      (catch $tag
        (return
          (pop (ref $struct))
        )
      )
    )
    (ref.null $struct)
  )

  ;; CHECK:      (func $func-two-params (type $two-params) (param $x (ref $struct)) (param $y (ref $struct))
  ;; CHECK-NEXT:  (local $z (ref null $two-params))
  ;; CHECK-NEXT:  (local.set $z
  ;; CHECK-NEXT:   (ref.func $func-two-params)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (call_indirect $0 (type $two-params)
  ;; CHECK-NEXT:   (local.get $x)
  ;; CHECK-NEXT:   (local.get $y)
  ;; CHECK-NEXT:   (i32.const 0)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $func-two-params (param $x (ref $struct)) (param $y (ref $struct))
    ;; This function has two params, which means a tuple type is used for its
    ;; signature, which we must also update. To verify the update is correct,
    ;; assign it to a local.
    (local $z (ref null $two-params))
    (local.set $z
      (ref.func $func-two-params)
    )
    ;; Also check that a call_indirect still validates after the rewriting.
    (call_indirect (type $two-params)
     (local.get $x)
     (local.get $y)
     (i32.const 0)
    )
  )

  ;; CHECK:      (func $field-keepalive (type $ref?|$struct|_=>_none) (param $struct (ref null $struct))
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (struct.get $struct 2
  ;; CHECK-NEXT:    (local.get $struct)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $field-keepalive (param $struct (ref null $struct))
    ;; --gto will remove fields that are not read from, so add reads to any
    ;; that don't already have them.
    (drop (struct.get $struct 2 (local.get $struct)))
  )
)

(module
  ;; Test recursion between structs where we only modify one. Specifically $B
  ;; has no writes to either of its fields.

  ;; CHECK:      (type $A (struct (field (mut (ref null $B))) (field (mut i32))))
  (type $A (struct (field (mut (ref null $B))) (field (mut i32)) ))
  ;; CHECK:      (type $B (struct (field (ref null $A)) (field f64)))
  (type $B (struct (field (mut (ref null $A))) (field (mut f64)) ))

  ;; CHECK:      (type $ref|$A|_=>_none (func (param (ref $A))))

  ;; CHECK:      (type $ref?|$A|_ref?|$B|_=>_none (func (param (ref null $A) (ref null $B))))

  ;; CHECK:      (func $func (type $ref|$A|_=>_none) (param $x (ref $A))
  ;; CHECK-NEXT:  (struct.set $A 0
  ;; CHECK-NEXT:   (local.get $x)
  ;; CHECK-NEXT:   (ref.null none)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (struct.set $A 1
  ;; CHECK-NEXT:   (local.get $x)
  ;; CHECK-NEXT:   (i32.const 20)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $func (param $x (ref $A))
    (struct.set $A 0
      (local.get $x)
      (ref.null $B)
    )
    (struct.set $A 1
      (local.get $x)
      (i32.const 20)
    )
  )

  ;; CHECK:      (func $field-keepalive (type $ref?|$A|_ref?|$B|_=>_none) (param $A (ref null $A)) (param $B (ref null $B))
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (struct.get $A 0
  ;; CHECK-NEXT:    (local.get $A)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (struct.get $A 1
  ;; CHECK-NEXT:    (local.get $A)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (struct.get $B 0
  ;; CHECK-NEXT:    (local.get $B)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (struct.get $B 1
  ;; CHECK-NEXT:    (local.get $B)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $field-keepalive (param $A (ref null $A)) (param $B (ref null $B))
    (drop (struct.get $A 0 (local.get $A)))
    (drop (struct.get $A 1 (local.get $A)))
    (drop (struct.get $B 0 (local.get $B)))
    (drop (struct.get $B 1 (local.get $B)))
  )
)

(module
  ;; As before, but flipped so that $A's fields can become immutable.

  ;; CHECK:      (type $B (struct (field (mut (ref null $A))) (field (mut f64))))
  (type $B (struct (field (mut (ref null $A))) (field (mut f64)) ))

  ;; CHECK:      (type $A (struct (field (ref null $B)) (field i32)))
  (type $A (struct (field (mut (ref null $B))) (field (mut i32)) ))

  ;; CHECK:      (type $ref|$B|_=>_none (func (param (ref $B))))

  ;; CHECK:      (type $ref?|$A|_ref?|$B|_=>_none (func (param (ref null $A) (ref null $B))))

  ;; CHECK:      (func $func (type $ref|$B|_=>_none) (param $x (ref $B))
  ;; CHECK-NEXT:  (struct.set $B 0
  ;; CHECK-NEXT:   (local.get $x)
  ;; CHECK-NEXT:   (ref.null none)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (struct.set $B 1
  ;; CHECK-NEXT:   (local.get $x)
  ;; CHECK-NEXT:   (f64.const 3.14159)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $func (param $x (ref $B))
    (struct.set $B 0
      (local.get $x)
      (ref.null $A)
    )
    (struct.set $B 1
      (local.get $x)
      (f64.const 3.14159)
    )
  )

  ;; CHECK:      (func $field-keepalive (type $ref?|$A|_ref?|$B|_=>_none) (param $A (ref null $A)) (param $B (ref null $B))
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (struct.get $A 0
  ;; CHECK-NEXT:    (local.get $A)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (struct.get $A 1
  ;; CHECK-NEXT:    (local.get $A)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (struct.get $B 0
  ;; CHECK-NEXT:    (local.get $B)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (struct.get $B 1
  ;; CHECK-NEXT:    (local.get $B)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $field-keepalive (param $A (ref null $A)) (param $B (ref null $B))
    (drop (struct.get $A 0 (local.get $A)))
    (drop (struct.get $A 1 (local.get $A)))
    (drop (struct.get $B 0 (local.get $B)))
    (drop (struct.get $B 1 (local.get $B)))
  )
)

(module
  ;; As before, but now one field in each can become immutable.

  ;; CHECK:      (type $A (struct (field (mut (ref null $B))) (field i32)))

  ;; CHECK:      (type $B (struct (field (ref null $A)) (field (mut f64))))
  (type $B (struct (field (mut (ref null $A))) (field (mut f64)) ))

  (type $A (struct (field (mut (ref null $B))) (field (mut i32)) ))

  ;; CHECK:      (type $ref|$A|_ref|$B|_=>_none (func (param (ref $A) (ref $B))))

  ;; CHECK:      (type $ref?|$A|_ref?|$B|_=>_none (func (param (ref null $A) (ref null $B))))

  ;; CHECK:      (func $func (type $ref|$A|_ref|$B|_=>_none) (param $x (ref $A)) (param $y (ref $B))
  ;; CHECK-NEXT:  (struct.set $A 0
  ;; CHECK-NEXT:   (local.get $x)
  ;; CHECK-NEXT:   (ref.null none)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (struct.set $B 1
  ;; CHECK-NEXT:   (local.get $y)
  ;; CHECK-NEXT:   (f64.const 3.14159)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $func (param $x (ref $A)) (param $y (ref $B))
    (struct.set $A 0
      (local.get $x)
      (ref.null $B)
    )
    (struct.set $B 1
      (local.get $y)
      (f64.const 3.14159)
    )
  )

  ;; CHECK:      (func $field-keepalive (type $ref?|$A|_ref?|$B|_=>_none) (param $A (ref null $A)) (param $B (ref null $B))
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (struct.get $A 0
  ;; CHECK-NEXT:    (local.get $A)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (struct.get $A 1
  ;; CHECK-NEXT:    (local.get $A)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (struct.get $B 0
  ;; CHECK-NEXT:    (local.get $B)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (struct.get $B 1
  ;; CHECK-NEXT:    (local.get $B)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $field-keepalive (param $A (ref null $A)) (param $B (ref null $B))
    (drop (struct.get $A 0 (local.get $A)))
    (drop (struct.get $A 1 (local.get $A)))
    (drop (struct.get $B 0 (local.get $B)))
    (drop (struct.get $B 1 (local.get $B)))
  )
)

(module
  ;; Field #0 is already immutable.
  ;; Field #1 is mutable and can become so.
  ;; Field #2 is mutable and must remain so.

  ;; CHECK:      (type $struct (struct (field i32) (field i32) (field (mut i32))))
  (type $struct (struct (field i32) (field (mut i32)) (field (mut i32))))

  ;; CHECK:      (type $ref|$struct|_=>_none (func (param (ref $struct))))

  ;; CHECK:      (type $ref?|$struct|_=>_none (func (param (ref null $struct))))

  ;; CHECK:      (func $func (type $ref|$struct|_=>_none) (param $x (ref $struct))
  ;; CHECK-NEXT:  (struct.set $struct 2
  ;; CHECK-NEXT:   (local.get $x)
  ;; CHECK-NEXT:   (i32.const 1)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $func (param $x (ref $struct))
    (struct.set $struct 2
      (local.get $x)
      (i32.const 1)
    )
  )

  ;; CHECK:      (func $field-keepalive (type $ref?|$struct|_=>_none) (param $struct (ref null $struct))
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (struct.get $struct 0
  ;; CHECK-NEXT:    (local.get $struct)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (struct.get $struct 1
  ;; CHECK-NEXT:    (local.get $struct)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (struct.get $struct 2
  ;; CHECK-NEXT:    (local.get $struct)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $field-keepalive (param $struct (ref null $struct))
    (drop (struct.get $struct 0 (local.get $struct)))
    (drop (struct.get $struct 1 (local.get $struct)))
    (drop (struct.get $struct 2 (local.get $struct)))
  )
)

(module
  ;; Subtyping. Without a write in either supertype or subtype, we can
  ;; optimize the field to be immutable.

  ;; CHECK:      (type $super (struct (field i32)))
  (type $super (struct (field (mut i32))))
  ;; CHECK:      (type $sub (struct_subtype (field i32) $super))
  (type $sub (struct_subtype (field (mut i32)) $super))

  ;; CHECK:      (type $none_=>_none (func))

  ;; CHECK:      (type $ref?|$super|_ref?|$sub|_=>_none (func (param (ref null $super) (ref null $sub))))

  ;; CHECK:      (func $func (type $none_=>_none)
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (struct.new $super
  ;; CHECK-NEXT:    (i32.const 1)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (struct.new $sub
  ;; CHECK-NEXT:    (i32.const 1)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $func
    ;; The presence of struct.new do not prevent us optimizing
    (drop
      (struct.new $super
        (i32.const 1)
      )
    )
    (drop
      (struct.new $sub
        (i32.const 1)
      )
    )
  )

  ;; CHECK:      (func $field-keepalive (type $ref?|$super|_ref?|$sub|_=>_none) (param $super (ref null $super)) (param $sub (ref null $sub))
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (struct.get $super 0
  ;; CHECK-NEXT:    (local.get $super)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (struct.get $sub 0
  ;; CHECK-NEXT:    (local.get $sub)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $field-keepalive (param $super (ref null $super)) (param $sub (ref null $sub))
    (drop (struct.get $super 0 (local.get $super)))
    (drop (struct.get $sub 0 (local.get $sub)))
  )
)

(module
  ;; As above, but add a write in the super, which prevents optimization.

  ;; CHECK:      (type $super (struct (field (mut i32))))
  (type $super (struct (field (mut i32))))
  ;; CHECK:      (type $sub (struct_subtype (field (mut i32)) $super))
  (type $sub (struct_subtype (field (mut i32)) $super))

  ;; CHECK:      (type $ref|$super|_=>_none (func (param (ref $super))))

  ;; CHECK:      (type $ref?|$super|_ref?|$sub|_=>_none (func (param (ref null $super) (ref null $sub))))

  ;; CHECK:      (func $func (type $ref|$super|_=>_none) (param $x (ref $super))
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (struct.new $super
  ;; CHECK-NEXT:    (i32.const 1)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (struct.new $sub
  ;; CHECK-NEXT:    (i32.const 1)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (struct.set $super 0
  ;; CHECK-NEXT:   (local.get $x)
  ;; CHECK-NEXT:   (i32.const 2)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $func (param $x (ref $super))
    ;; The presence of struct.new do not prevent us optimizing
    (drop
      (struct.new $super
        (i32.const 1)
      )
    )
    (drop
      (struct.new $sub
        (i32.const 1)
      )
    )
    (struct.set $super 0
      (local.get $x)
      (i32.const 2)
    )
  )

  ;; CHECK:      (func $field-keepalive (type $ref?|$super|_ref?|$sub|_=>_none) (param $super (ref null $super)) (param $sub (ref null $sub))
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (struct.get $super 0
  ;; CHECK-NEXT:    (local.get $super)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (struct.get $sub 0
  ;; CHECK-NEXT:    (local.get $sub)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $field-keepalive (param $super (ref null $super)) (param $sub (ref null $sub))
    (drop (struct.get $super 0 (local.get $super)))
    (drop (struct.get $sub 0 (local.get $sub)))
  )
)

(module
  ;; As above, but add a write in the sub, which prevents optimization.


  ;; CHECK:      (type $super (struct (field (mut i32))))
  (type $super (struct (field (mut i32))))
  ;; CHECK:      (type $sub (struct_subtype (field (mut i32)) $super))
  (type $sub (struct_subtype (field (mut i32)) $super))

  ;; CHECK:      (type $ref|$sub|_=>_none (func (param (ref $sub))))

  ;; CHECK:      (type $ref?|$super|_ref?|$sub|_=>_none (func (param (ref null $super) (ref null $sub))))

  ;; CHECK:      (func $func (type $ref|$sub|_=>_none) (param $x (ref $sub))
  ;; CHECK-NEXT:  (struct.set $sub 0
  ;; CHECK-NEXT:   (local.get $x)
  ;; CHECK-NEXT:   (i32.const 2)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $func (param $x (ref $sub))
    (struct.set $sub 0
      (local.get $x)
      (i32.const 2)
    )
  )

  ;; CHECK:      (func $field-keepalive (type $ref?|$super|_ref?|$sub|_=>_none) (param $super (ref null $super)) (param $sub (ref null $sub))
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (struct.get $super 0
  ;; CHECK-NEXT:    (local.get $super)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (struct.get $sub 0
  ;; CHECK-NEXT:    (local.get $sub)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $field-keepalive (param $super (ref null $super)) (param $sub (ref null $sub))
    (drop (struct.get $super 0 (local.get $super)))
    (drop (struct.get $sub 0 (local.get $sub)))
  )
)