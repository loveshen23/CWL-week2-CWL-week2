;; NOTE: Assertions have been generated by update_lit_checks.py and should not be edited.
;; RUN: wasm-opt %s -all --dae -S -o - | filecheck %s
;; RUN: wasm-opt %s -all --dae --nominal -S -o - | filecheck %s --check-prefix NOMNL

(module

 ;; CHECK:      (type ${} (struct ))
 ;; NOMNL:      (type ${} (struct ))
 (type ${} (struct))

 ;; CHECK:      (type ${i32} (struct_subtype (field i32) ${}))
 ;; NOMNL:      (type ${i32} (struct_subtype (field i32) ${}))
 (type ${i32} (struct_subtype (field i32) ${}))

 ;; CHECK:      (type ${i32_i64} (struct_subtype (field i32) (field i64) ${i32}))

 ;; CHECK:      (type ${i32_f32} (struct_subtype (field i32) (field f32) ${i32}))

 ;; CHECK:      (type ${f64} (struct_subtype (field f64) ${}))
 ;; NOMNL:      (type ${i32_i64} (struct_subtype (field i32) (field i64) ${i32}))

 ;; NOMNL:      (type ${i32_f32} (struct_subtype (field i32) (field f32) ${i32}))

 ;; NOMNL:      (type ${f64} (struct_subtype (field f64) ${}))
 (type ${f64} (struct_subtype (field f64) ${}))

(type ${i32_i64} (struct_subtype (field i32) (field i64) ${i32}))

 (type ${i32_f32} (struct_subtype (field i32) (field f32) ${i32}))

 ;; CHECK:      (func $call-various-params-no (type $none_=>_none)
 ;; CHECK-NEXT:  (call $various-params-no
 ;; CHECK-NEXT:   (call $get_{})
 ;; CHECK-NEXT:   (call $get_{i32})
 ;; CHECK-NEXT:  )
 ;; CHECK-NEXT:  (call $various-params-no
 ;; CHECK-NEXT:   (call $get_{i32})
 ;; CHECK-NEXT:   (call $get_{f64})
 ;; CHECK-NEXT:  )
 ;; CHECK-NEXT: )
 ;; NOMNL:      (func $call-various-params-no (type $none_=>_none)
 ;; NOMNL-NEXT:  (call $various-params-no
 ;; NOMNL-NEXT:   (call $get_{})
 ;; NOMNL-NEXT:   (call $get_{i32})
 ;; NOMNL-NEXT:  )
 ;; NOMNL-NEXT:  (call $various-params-no
 ;; NOMNL-NEXT:   (call $get_{i32})
 ;; NOMNL-NEXT:   (call $get_{f64})
 ;; NOMNL-NEXT:  )
 ;; NOMNL-NEXT: )
 (func $call-various-params-no
  ;; The first argument gets {} and {i32}; the second {i32} and {f64}; none of
  ;; those pairs can be optimized. Note that we do not pass in all nulls, as
  ;; all nulls are identical and we could do other optimization work due to
  ;; that.
  (call $various-params-no
   (call $get_{})
   (call $get_{i32})
  )
  (call $various-params-no
   (call $get_{i32})
   (call $get_{f64})
  )
 )
 ;; This function is called in ways that do not allow us to alter the types of
 ;; its parameters (see last function).
 ;; CHECK:      (func $various-params-no (type $ref?|${}|_ref?|${}|_=>_none) (param $x (ref null ${})) (param $y (ref null ${}))
 ;; CHECK-NEXT:  (drop
 ;; CHECK-NEXT:   (local.get $x)
 ;; CHECK-NEXT:  )
 ;; CHECK-NEXT:  (drop
 ;; CHECK-NEXT:   (local.get $y)
 ;; CHECK-NEXT:  )
 ;; CHECK-NEXT: )
 ;; NOMNL:      (func $various-params-no (type $ref?|${}|_ref?|${}|_=>_none) (param $x (ref null ${})) (param $y (ref null ${}))
 ;; NOMNL-NEXT:  (drop
 ;; NOMNL-NEXT:   (local.get $x)
 ;; NOMNL-NEXT:  )
 ;; NOMNL-NEXT:  (drop
 ;; NOMNL-NEXT:   (local.get $y)
 ;; NOMNL-NEXT:  )
 ;; NOMNL-NEXT: )
 (func $various-params-no (param $x (ref null ${})) (param $y (ref null ${}))
  ;; "Use" the locals to avoid other optimizations kicking in.
  (drop (local.get $x))
  (drop (local.get $y))
 )

 ;; CHECK:      (func $get_{} (type $none_=>_ref?|${}|) (result (ref null ${}))
 ;; CHECK-NEXT:  (unreachable)
 ;; CHECK-NEXT: )
 ;; NOMNL:      (func $get_{} (type $none_=>_ref?|${}|) (result (ref nul