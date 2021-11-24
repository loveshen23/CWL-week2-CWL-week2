;; NOTE: Assertions have been generated by update_lit_checks.py and should not be edited.
;; RUN: wasm-opt %s --remove-unused-names --merge-blocks -all -S -o - \
;; RUN:   | filecheck %s
;;
;; --remove-unused-names lets --merge-blocks assume blocks without names have no
;; branch targets.

(module
 (type $anyref_=>_none (func (param anyref)))

 ;; CHECK:      (type $array (array (mut i32)))

 ;; CHECK:      (type $struct (struct (field (mut i32))))
 (type $struct (struct (field (mut i32))))

 (type $array (array (mut i32)))

 ;; CHECK:      (func $br_on_to_drop (type $none_=>_none)
 ;; CHECK-NEXT:  (nop)
 ;; CHECK-NEXT:  (drop
 ;; CHECK-NEXT:   (block $label$1 (result i31ref)
 ;; CHECK-NEXT:    (drop
 ;; CHECK-NEXT:     (br_on_i31 $label$1
 ;; CHECK-NEXT:      (ref.null none)
 ;; CHECK-NEXT:     )
 ;; CHECK-NEXT:    )
 ;; CHECK-NEXT:    (ref.null 