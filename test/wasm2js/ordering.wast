(module
 (type $0 (func (param i32) (result i32)))
 (import "env" "table" (table $timport$9 7 funcref))
 (elem (i32.const 1) $foo $bar $baz)
 (export "main" (func $main))
 (func $main
  (drop
   (call_indirect (type $0)
    (i32.cons