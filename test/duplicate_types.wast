
(module ;; tests duplicate types are named properly
 (type (func))
 (type (func))
 (type (func))
 (type (func (param i32)))
 (type $0 (func (param i32)))
 (type (func (param i32)))
 (type $b (func (param i32) (result f32)))
 (type (func (param i32) (result f32)))

 (func $f0 (param i32))
 (func $f1 (param i32) (result i32)
  (i32.const 0)
 )
)