
(module
  (type $0 (func (result i32)))
  (import "env" "memory" (memory $0 i64 1 1))
  (func $foo (type $0) (result i32)
    (i32.load offset=13
      (i64.const 37)
    )
  )
)