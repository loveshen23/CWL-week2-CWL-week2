(module
  ;; Statement switch
  (func (export "stmt") (param $i i32) (result i32)
    (local $j i32)
    (local.set $j (i32.const 100))
    (block $switch
      (block $7
        (block $default
          (block $6
            (block $5
              (block $4
                (block $3
                  (block $2
                    (block $1
                      (block $0
                        (br_table $0 $1 $2 $3 $4 $5 $6 $7 $default
                          (local.get $i)
                        )
                      ) ;; 0
                      (return (local.get $i))
                    ) ;; 1
                    (nop)
                    ;; fallthrough
                  ) ;; 2
                 