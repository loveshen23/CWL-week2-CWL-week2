;; NOTE: Assertions have been generated by update_lit_checks.py --all-items and should not be edited.

;; Check that string types are emitted properly in the binary format.
;;
;; runs --precompute in order to verify no problems occur in the optimizer's
;; invocation of the interpreter.

;; RUN: wasm-opt %s --enable-strings --enable-reference-types --enable-gc --roundtrip --precompute -S -o - | filecheck %s

;; Check that we can roundtrip through the text format as well.

;; RUN: wasm-opt %s -all -S -o - | wasm-opt -all --precompute -S -o - | filecheck %s

(module
  (memory 10 10)

  ;; CHECK:      (type $stringref_=>_none (func (param stringref)))

  ;; CHECK:      (type $stringref_stringref_=>_none (func (param stringref stringref)))

  ;; CHECK:      (type $stringref_stringview_wtf8_stringview_wtf16_stringview_iter_=>_none (func (param stringref stringview_wtf8 stringview_wtf16 stringview_iter)))

  ;; CHECK:      (type $array (array (mut i8)))
  (type $array (array_subtype (mut i8) data))
  ;; CHECK:      (type $array16 (array (mut i16)))
  (type $array16 (array_subtype (mut i16) data))

  ;; CHECK:      (type $stringref_stringview_wtf8_stringview_wtf16_stringview_iter_stringref_stringview_wtf8_stringview_wtf16_stringview_iter_ref|string|_ref|stringview_wtf8|_ref|stringview_wtf16|_ref|stringview_iter|_=>_none (func (param stringref stringview_wtf8 stringview_wtf16 stringview_iter stringref stringview_wtf8 stringview_wtf16 stringview_iter (ref string) (ref stringview_wtf8) (ref stringview_wtf16) (ref stringview_iter))))

  ;; CHECK:      (type $ref|string|_=>_none (func (param (ref string))))

  ;; CHECK:      (type $stringview_wtf16_=>_none (func (param stringview_wtf16)))

  ;; CHECK:      (type $ref|$array|_ref|$array16|_=>_none (func (param (ref $array) (ref $array16))))

  ;; CHECK:      (type $stringref_ref|$array|_ref|$array16|_=>_none (func (param stringref (ref $array) (ref $array16))))

  ;; CHECK:      (type $none_=>_none (func))

  ;; CHECK:      (type $ref|$array|_=>_none (func (param (ref $array))))

  ;; CHECK:      (type $stringref_=>_i32 (func (param stringref) (result i32)))

  ;; CHECK:      (global $string-const stringref (string.const "string in a global \01\ff\00\t\t\n\n\r\r\"\"\'\'\\\\"))
  (global $string-const stringref (string.const "string in a global \01\ff\00\t\09\n\0a\r\0d\"\22\'\27\\\5c"))

  ;; CHECK:      (memory $0 10 10)

  ;; CHECK:      (func $string.new (type $stringref_stringview_wtf8_stringview_wtf16_stringview_iter_stringref_stringview_wtf8_stringview_wtf16_stringview_iter_ref|string|_ref|stringview_wtf8|_ref|stringview_wtf16|_ref|stringview_iter|_=>_none) (param $a stringref) (param $b stringview_wtf8) (param $c stringview_wtf16) (param $d stringview_iter) (param $e stringref) (param $f stringview_wtf8) (param $g stringview_wtf16) (param $h stringview_iter) (param $i (ref string)) (param $j (ref stringview_wtf8)) (param $k (ref stringview_wtf16)) (param $l (ref stringview_iter))
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (string.new_wtf8 utf8
  ;; CHECK-NEXT:    (i32.const 1)
  ;; CHECK-NEXT:    (i32.const 2)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (string.new_wtf8 wtf8
  ;; CHECK-NEXT:    (i32.const 3)
  ;; CHECK-NEXT:    (i32.const 4)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (string.new_wtf8 replace
  ;; CHECK-NEXT:    (i32.const 5)
  ;; CHECK-NEXT:    (i32.const 6)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (string.new_wtf16
  ;; CHECK-NEXT:    (i32.const 7)
  ;; CHECK-NEXT:    (i32.const 8)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $string.new
    (param $a stringref)
    (param $b stringview_wtf8)
    (param $c stringview_wtf16)
    (param $d stringview_iter)
    (param $e (ref null string))
    (param $f (ref null stringview_wtf8))
    (param $g (ref null stringview_wtf16))
    (param $h (ref null stringview_iter))
    (param $i (ref string))
    (param $j (ref stringview_wtf8))
    (param $k (ref stringview_wtf16))
    (param $l (ref stringview_iter))
    (drop
      (string.new_wtf8 utf8
        (i32.const 1)
        (i32.const 2)
      )
    )
    (drop
      (string.new_wtf8 wtf8
        (i32.const 3)
        (i32.const 4)
      )
    )
    (drop
      (string.new_wtf8 replace
        (i32.const 5)
        (i32.const 6)
      )
    )
    (drop
      (string.new_wtf16
        (i32.const 7)
        (i32.const 8)
      )
    )
  )

  ;; CHECK:      (func $string.const (type $ref|string|_=>_none) (param $param (ref string))
  ;; CHECK-NEXT:  (call $string.const
  ;; CHECK-NEXT:   (string.const "foo")
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (call $string.const
  ;; CHECK-NEXT:   (string.const "foo")
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (call $string.const
  ;; CHECK-NEXT:   (string.const "bar")
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $string.const (param $param (ref string))
    ;; Use calls to avoid precompute removing dropped constants.
    (call $string.const
      (string.const "foo")
    )
    (call $string.const
      (string.const "foo") ;; intentionally repeat the previous one
    )
    (call $string.const
      (string.const "bar")
    )
  )

  ;; CHECK:      (func $string.measure (type $stringref_=>_none) (param $ref stringref)
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (i32.eqz
  ;; CHECK-NEXT:    (string.measure_wtf8 wtf8
  ;; CHECK-NEXT:     (local.get $ref)
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (string.measure_wtf8 utf8
  ;; CHECK-NEXT:    (local.get $ref)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (string.measure_wtf16
  ;; CHECK-NEXT:    (local.get $ref)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $string.measure (param $ref stringref)
    (drop
      (i32.eqz ;; validate the output is i32
        (string.measure_wtf8 wtf8
          (local.get $ref)
        )
      )
    )
    (drop
      (string.measure_wtf8 utf8
        (local.get $ref)
      )
    )
    (drop
      (string.measure_wtf16
        (local.get $ref)
      )
    )
  )

  ;; CHECK:      (func $string.encode (type $stringref_=>_none) (param $ref stringref)
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (i32.eqz
  ;; CHECK-NEXT:    (string.encode_wtf8 wtf8
  ;; CHECK-NEXT:     (local.get $ref)
  ;; CHECK-NEXT:     (i32.const 10)
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (string.encode_wtf8 utf8
  ;; CHECK-NEXT:    (local.get $ref)
  ;; CHECK-