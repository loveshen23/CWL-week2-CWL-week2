
(module
  (type $none_=>_i32 (func (result i32)))
  (import "env" "table" (table $timport 6 funcref))
  (func "foo-true" (param $x i32) (result i32)
    (call_indirect (type $none_=>_i32)
      (select
        (i32.const 1)
        (i32.const 0)
        (local.get $x)
      )
    )
  )
  (func "foo-false" (param $x i32) (result i32)
    (call_indirect (type $none_=>_i32)
      (select
        (i32.const 0)
        (i32.const 1)
        (local.get $x)
      )
    )
  )
)