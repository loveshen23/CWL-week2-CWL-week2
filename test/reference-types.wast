(module
  (type $sig_eqref (func (param eqref)))
  (type $sig_funcref (func (param funcref)))
  (type $sig_anyref (func (param anyref)))

  (func $take_eqref (param eqref))
  (func $take_funcref (param funcref))
  (func $take_anyref (param anyref))
  (func $foo)

  (table funcref (elem $take_eqref $take_funcref $take_anyref))
  (elem declare func $ref-taken-but-not-in-table)

  (import "env" "import_func" (func $import_func (param eqref) (result funcref)))
  (import "env" "import_global" (global $import_global eqref))
  (export "export_func" (func $import_func (param eqref) (result funcref)))
  (export "export_global" (global $import_global))

  ;; Test global initializer expressions
  (global $global_eqref (mut eqref) (ref.null eq))
  (global $global_funcref (mut funcref) (ref.null func))
  (global $global_funcref_func (mut funcref) (ref.func $foo))
  (global $global_anyref (mut anyref) (ref.null any))

  ;; Test subtype relationship in global initializer expressions
  (global $global_anyref2 (mut anyref) (ref.null eq))

  (tag $e-i32 (param i32))

  (func $test
    (local $local_eqref eqref)
    (local $local_funcref funcref)
    (local $local_anyref anyref)

    ;; Test types for local.get/set
    (local.set $local_eqref (local.get $local_eqref))
    (local.set $local_eqref (global.get $global_eqref))
    (local.set $local_eqref (ref.null eq))
    (local.set $local_funcref (local.get $local_funcref))
    (local.set $local_funcref (global.get $global_funcref))
    (local.set $local_funcref (ref.null func))
    (local.set $local_funcref (ref.func $foo))
    (local.set $local_anyref (local.get $local_anyref))
    (local.set $local_anyref (global.get $global_anyref))
    (local.set $local_anyref (ref.null any))

    ;; Test subtype relationship for local.set
    (local.set $local_anyref (local.get $local_eqref))
    (local.set $local_anyref (global.get $global_eqref))
    (local.set $local_anyref (ref.null eq))

    ;; Test types for global.get/set
    (global.set $global_eqref (global.get $global_eqref))
    (global.set $global_eqref (local.get $local_eqref))
    (global.set $global_eqref (ref.null eq))
    (global.set $global_funcref (global.get $global_funcref))
    (global.set $global_funcref (local.get $local_funcref))
    (global.set $global_funcref (ref.null func))
    (global.set $global_funcref (ref.func $foo))
    (global.set $global_anyref (global.get $global_anyref))
    (global.set $global_anyref (local.get $local_anyref))
    (global.set $global_anyref (ref.null any))

    ;; Test subtype relationship for global.set
    (global.set $global_anyref (global.get $global_eqref))
    (global.set $global_anyref (local.get $local_eqref))
    (global.set $global_anyref (ref.null eq))

    ;; Test function call params
    (call $take_eqref (local.get $local_eqref))
    (call $take_eqref (global.get $global_eqref))
    (call $take_eqref (ref.null eq))
    (call $take_funcref (local.get $local_funcref))
    (call $take_funcref (global.get $global_funcref))
    (call $take_funcref (ref.null func))
    (call $take_funcref (ref.func $foo))
    (call $take_anyref (local.get $local_anyref))
    (call $take_anyref (global.get $global_anyref))
    (call $take_anyref (ref.null any))

    ;; Test subtype relationship for function call params
    (call $take_anyref (local.get $local_eqref))
    (call $take_anyref (global.get $global_eqref))
    (call $take_anyref (ref.null eq))

    ;; Test call_indirect params
    (call_indirect (type $sig_eqref) (local.get $local_eqref) (i32.const 0))
    (call_indirect (type $sig_eqref) (global.get $global_eqref) (i32.const 0))
    (call_indirect (type $sig_eqref) (ref.null eq) (i32.const 0))
    (call_indirect (type $sig_funcref) (local.get $local_funcref) (i32.const 1))
    (call_indirect (type $sig_funcref) (global.get $global_funcref) (i32.const 1))
    (call_indirect (type $sig_funcref) (ref.null func) (i32.const 1))
    (call_indirect (type $sig_funcref) (ref.func $foo) (i32.const 1))
    (call_indirect (type $sig_anyref) (local.get $local_anyref) (i32.const 3))
    (call_indirect (type $sig_anyref) (global.get $global_anyref) (i32.const 3))
    (call_indirect (type $sig_anyref) (ref.null any) (i32.const 3))

    ;; Test subtype relationship for call_indirect params
    (call_indirect (type $sig_anyref) (local.get $local_eqref) (i32.const 3))
    (call_indirect (type $sig_anyref) (global.get $global_eqref) (i32.const 3))
    (call_indirect (type $sig_anyref) (ref.null eq) (i32.const 3))

    ;; Test block return type
    (drop
      (block (result eqref)
        (br_if 0 (local.get $local_eqref) (i32.const 1))
      )
    )
    (drop
      (block (result eqref)
        (br_if 0 (global.get $global_eqref) (i32.const 1))
      )
    )
    (drop
      (block (result eqref)
        (br_if 0 (ref.null eq) (i32.const 1))
      )
    )
    (drop
      (block (result funcref)
        (br_if 0 (local.get $local_funcref) (i32.const 1))
      )
    )
    (drop
      (block (result funcref)
        (br_if 0 (global.get $global_funcref) (i32.const 1))
      )
    )
    (drop
      (block (result funcref)
        (br_if 0 (ref.null func) (i32.const 1))
      )
    )
    (drop
      (block (result funcref)
        (br_if 0 (ref.func $foo) (i32.const 1))
      )
    )
    (drop
      (block (result anyref)
        (br_if 0 (local.get $local_anyref) (i32.const 1))
      )
    )
    (drop
      (block (result anyref)
        (br_if 0 (global.get $global_anyref) (i32.const 1))
      )
    )
    (drop
      (block (result anyref)
        (br_if 0 (ref.null any) (i32.const 1))
      )
    )

    ;; Test subtype relationship for block return type
    (drop
      (block (result anyref)
        (br_if 0 (local.get $local_eqref) (i32.const 1))
      )
    )
    (drop
      (block (result anyref)
        (br_if 0 (ref.null eq) (i32.const 1))
      )
    )

    ;; Test loop return type
    (drop
      (loop (result eqref)
        (local.get $local_eqref)
      )
    )
    (drop
      (loop (result eqref)
        (global.get $global_eqref)
      )
    )
    (drop
      (loop (result eqref)
        (ref.null eq)
      )
    )
    (drop
      (loop (result funcref)
        (local.get $local_funcref)
      )
    )
    (drop
      (loop (result funcref)
        (global.get $global_funcref)
      )
    )
    (drop
      (loop (result funcref)
        (ref.n