
;; NOTE: Assertions have been generated by update_lit_checks.py and should not be edited.
;; RUN: wasm-opt %s -all --inlining-optimizing -S -o - | filecheck %s

(module
 ;; CHECK:      (type $none_=>_none (func))
 (type $none_=>_none (func))
 (type $none_=>_i32 (func (result i32)))
 ;; CHECK:      (func $0 (type $none_=>_none)
 ;; CHECK-NEXT:  (nop)
 ;; CHECK-NEXT: )
 (func $0
  (nop)
 )
 ;; CHECK:      (func $1 (type $none_=>_none)
 ;; CHECK-NEXT:  (drop
 ;; CHECK-NEXT:   (block ;; (replaces something unreachable we can't emit)
 ;; CHECK-NEXT:    (drop
 ;; CHECK-NEXT:     (unreachable)
 ;; CHECK-NEXT:    )
 ;; CHECK-NEXT:    (unreachable)
 ;; CHECK-NEXT:   )
 ;; CHECK-NEXT:  )
 ;; CHECK-NEXT: )
 (func $1
  ;; $0 will be inlined into here. We will then optimize this function - but
  ;; we do so *without* optimizing $0 (as inlining-optimizing only optimizes
  ;; where it inlines, for efficiency). As part of the optimiziations, we will
  ;; try to precompute the cast here, which will try to look up $0. We should
  ;; not hit an assertion, rather we should skip precomputing it, the same as if
  ;; we were optimizing $1 before $0 were added to the module. (In fact, we will
  ;; be able to see that the cast cannot succeed, and will optimize it into an
  ;; unreachable.)
  (call $0)
  (drop
   (call_ref $none_=>_i32
    (ref.cast $none_=>_i32
     (ref.func $0)
    )
   )
  )
 )
)