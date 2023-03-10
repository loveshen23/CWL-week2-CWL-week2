
(module
  (type $FUNCSIG$vf (func (param f32)))
  (type $FUNCSIG$v (func))
  (type $FUNCSIG$id (func (param f64) (result i32)))
  (type $FUNCSIG$ddd (func (param f64 f64) (result f64)))
  (type $4 (func (result f64)))
  (type $5 (func (result i32)))
  (type $6 (func (param i32) (result i32)))
  (type $7 (func (param f64) (result f64)))
  (type $8 (func (result i64)))
  (type $9 (func (param i32 i64)))
  (import "env" "_emscripten_asm_const_vi" (func $_emscripten_asm_const_vi))
  (import "asm2wasm" "f64-to-int" (func $f64-to-int (param f64) (result i32)))
  (import "asm2wasm" "f64-rem" (func $f64-rem (param f64 f64) (result f64)))
  (table 10 funcref)
  (elem (i32.const 0) $z $big_negative $z $z $w $w $importedDoubles $w $z $cneg)
  (memory $0 4096 4096)
  (data (i32.const 1026) "\14\00")
  (export "big_negative" (func $big_negative))
  (func $big_negative (type $FUNCSIG$v)
    (local $temp f64)
    (block $block0
      (local.set $temp
        (f64.const -2147483648)
      )
      (local.set $temp
        (f64.const -2147483648)
      )
      (local.set $temp
        (f64.const -21474836480)
      )
      (local.set $temp
        (f64.const 0.039625)
      )
      (local.set $temp
        (f64.const -0.039625)
      )
    )
  )
  (func $importedDoubles (type $4) (result f64)
    (local $temp f64)
    (block $topmost (result f64)
      (local.set $temp
        (f64.add
          (f64.add
            (f64.add
              (f64.load
                (i32.const 8)
              )
              (f64.load
                (i32.const 16)
              )
            )
            (f64.neg
              (f64.load
                (i32.const 16)
              )
            )
          )
          (f64.neg
            (f64.load
              (i32.const 8)
            )
          )
        )
      )
      (if
        (i32.gt_s
          (i32.load
            (i32.const 24)
          )
          (i32.const 0)
        )
        (br $topmost
          (f64.const -3.4)
        )
      )
      (if
        (f64.gt
          (f64.load
            (i32.const 32)
          )
          (f64.const 0)
        )
        (br $topmost
          (f64.const 5.6)
        )
      )
      (f64.const 1.2)
    )
  )
  (func $doubleCompares (type $FUNCSIG$ddd) (param $x f64) (param $y f64) (result f64)
    (local $t f64)
    (local $Int f64)
    (local $Double i32)
    (block $topmost (result f64)
      (if
        (f64.gt
          (local.get $x)
          (f64.const 0)
        )
        (br $topmost
          (f64.const 1.2)
        )
      )
      (if
        (f64.gt
          (local.get $Int)
          (f64.const 0)
        )
        (br $topmost
          (f64.const -3.4)
        )
      )
      (if
        (i32.gt_s
          (local.get $Double)
          (i32.const 0)
        )
        (br $topmost
          (f64.const 5.6)
        )
      )
      (if
        (f64.lt
          (local.get $x)
          (local.get $y)
        )
        (br $topmost
          (local.get $x)
        )
      )
      (local.get $y)
    )
  )
  (func $intOps (type $5) (result i32)
    (local $x i32)
    (i32.eq
      (local.get $x)
      (i32.const 0)
    )
  )
  (func $hexLiterals (type $FUNCSIG$v)
    (drop
      (i32.add
        (i32.add
          (i32.const 0)
          (i32.const 313249263)
        )
        (i32.const -19088752)
      )
    )
  )
  (func $conversions (type $FUNCSIG$v)
    (local $i i32)
    (local $d f64)
    (block $block0
      (local.set $i
        (call $f64-to-int
          (local.get $d)
        )
      )
      (local.set $d
        (f64.convert_i32_s
          (local.get $i)
        )
      )
      (local.set $d
        (f64.convert_i32_u
          (i32.shr_u
            (local.get $i)
            (i32.const 0)
          )
        )
      )
    )
  )
  (func $seq (type $FUNCSIG$v)
    (local $J f64)
    (local.set $J
      (f64.sub
        (block $block0 (result f64)
          (drop
            (f64.const 0.1)
          )
          (f64.const 5.1)
        )
        (block $block1 (result f64)
          (drop
            (f64.const 3.2)
          )
          (f64.const 4.2)
        )
      )
    )
  )
  (func $switcher (type $6) (param $x i32) (result i32)
    (block $topmost (result i32)
      (block $switch$0
        (block $switch-default$3
          (block $switch-case$2
            (block $switch-case$1
              (br_table $switch-case$1 $switch-case$2 $switch-default$3
                (i32.sub
                  (local.get $x)
                  (i32.const 1)
                )
              )
            )
            (br $topmost
              (i32.const 1)
            )
          )
          (br $topmost
            (i32.const 2)
          )
        )
        (nop)
      )
      (block $switch$4
        (block $switch-default$7
          (block $switch-case$6
            (block $switch-case$5
              (br_table $switch-case$6 $switch-default$7 $switch-default$7 $switch-default$7 $switch-default$7 $switch-default$7 $switch-default$7 $switch-case$5 $switch-default$7
                (i32.sub
                  (local.get $x)
                  (i32.const 5)
                )
              )
            )
            (br $topmost
              (i32.const 121)
            )
          )
          (br $topmost
            (i32.const 51)
          )
        )
        (nop)
      )
      (block $label$break$Lout
        (block $switch-default$16
          (block $switch-case$15
            (block $switch-case$12
              (block $switch-case$9
                (block $switch-case$8
                  (br_table $switch-case$15 $switch-default$16 $switch-default$16 $switch-case$12 $switch-default$16 $switch-default$16 $switch-default$16 $switch-default$16 $switch-case$9 $switch-default$16 $switch-case$8 $switch-default$16
                    (i32.sub
                      (local.get $x)
                      (i32.const 2)
                    )
                  )
                )
                (br $label$break$Lout)
              )
              (br $label$break$Lout)
            )
            (block $while-out$10
              (loop $while-in$11
                (block $block1
                  (br $while-out$10)
                  (br $while-in$11)
                )
              )
              (br $label$break$Lout)
            )
          )
          (block $while-out$13
            (loop $while-in$14
              (block $block3
                (br $label$break$Lout)
                (br $while-in$14)
              )
            )
            (br $label$break$Lout)
          )
        )
        (nop)
      )
      (i32.const 0)
    )
  )
  (func $blocker (type $FUNCSIG$v)
    (block $label$break$L
      (br $label$break$L)
    )
  )
  (func $frem (type $4) (result f64)
    (call $f64-rem
      (f64.const 5.5)
      (f64.const 1.2)
    )
  )
  (func $big_uint_div_u (type $5) (result i32)
    (local $x i32)
    (block $topmost (result i32)
      (local.set $x
        (i32.and
          (i32.div_u
            (i32.const -1)
            (i32.const 2)
          )
          (i32.const -1)
        )
      )
      (local.get $x)
    )
  )
  (func $fr (type $FUNCSIG$vf) (param $x f32)
    (local $y f32)
    (local $z f64)
    (block $block0
      (drop
        (f32.demote_f64
          (local.get $z)
        )
      )
      (drop
        (local.get $y)
      )
      (drop
        (f32.const 5)
      )
      (drop
        (f32.const 0)
      )
      (drop
        (f32.const 5)
      )
      (drop
        (f32.const 0)
      )
    )
  )
  (func $negZero (type $4) (result f64)
    (f64.const -0)
  )
  (func $abs (type $FUNCSIG$v)
    (local $x i32)
    (local $y f64)
    (local $z f32)
    (local $asm2wasm_i32_temp i32)
    (block $block0
      (local.set $x
        (block $block1 (result i32)
          (local.set $asm2wasm_i32_temp
            (i32.const 0)
          )
          (select
            (i32.sub
              (i32.const 0)
              (local.get $asm2wasm_i32_temp)
            )
            (local.get $asm2wasm_i32_temp)
            (i32.lt_s
              (local.get $asm2wasm_i32_temp)
              (i32.const 0)
            )
          )
        )
      )
      (local.set $y
        (f64.abs
          (f64.const 0)
        )
      )
      (local.set $z
        (f32.abs
          (f32.const 0)
        )
      )
    )
  )
  (func $neg (type $FUNCSIG$v)
    (local $x f32)
    (block $block0
      (local.set $x
        (f32.neg
          (local.get $x)
        )
      )
      (call_indirect (type $FUNCSIG$vf)
        (local.get $x)
        (i32.add
          (i32.and
            (i32.const 1)
            (i32.const 7)
          )
          (i32.const 8)
        )
      )
    )
  )
  (func $cneg (type $FUNCSIG$vf) (param $x f32)
    (call_indirect (type $FUNCSIG$vf)
      (local.get $x)
      (i32.add
        (i32.and
          (i32.const 1)
          (i32.const 7)
        )
        (i32.const 8)
      )
    )
  )
  (func $___syscall_ret (type $FUNCSIG$v)
    (local $$0 i32)
    (drop
      (i32.gt_u
        (i32.shr_u
          (local.get $$0)
          (i32.const 0)
        )
        (i32.const -4096)
      )
    )
  )
  (func $z (type $FUNCSIG$v)
    (nop)
  )
  (func $w (type $FUNCSIG$v)
    (nop)
  )
  (func $block_and_after (type $5) (result i32)
    (block $waka
      (drop
        (i32.const 1)
      )
      (br $waka)
    )
    (i32.const 0)
  )
  (func $loop-roundtrip (type $7) (param $0 f64) (result f64)
    (loop $loop-in1 (result f64)
      (drop
        (local.get $0)
      )
      (local.get $0)
    )
  )
  (func $big-i64 (type $8) (result i64)
    (i64.const -9218868437227405313)
  )
  (func $i64-store32 (type $9) (param $0 i32) (param $1 i64)
    (i64.store32
      (local.get $0)
      (local.get $1)
    )
  )
  (func $return-unreachable (result i32)
    (return (i32.const 1))
  )
  (func $unreachable-block (result i32)
    (f64.abs
      (block ;; note no type - valid in binaryen IR, in wasm must be i32
        (drop (i32.const 1))
        (return (i32.const 2))
      )
    )
  )
  (func $unreachable-block-toplevel (result i32)
    (block ;; note no type - valid in binaryen IR, in wasm must be i32
      (drop (i32.const 1))
      (return (i32.const 2))
    )
  )
  (func $unreachable-block0 (result i32)
    (f64.abs
      (block ;; note no type - valid in binaryen IR, in wasm must be i32
        (return (i32.const 2))
      )
    )
  )
  (func $unreachable-block0-toplevel (result i32)
    (block ;; note no type - valid in binaryen IR, in wasm must be i32
      (return (i32.const 2))
    )
  )
  (func $unreachable-block-with-br (result i32)
    (block $block ;; unreachable type due to last element having that type, but the block is exitable
      (drop (i32.const 1))
      (br $block)
    )
    (i32.const 1)
  )
  (func $unreachable-if (result i32)
    (f64.abs
      (if ;; note no type - valid in binaryen IR, in wasm must be i32
        (i32.const 3)
        (return (i32.const 2))
        (return (i32.const 1))
      )
    )
  )
  (func $unreachable-if-toplevel (result i32)
    (if ;; note no type - valid in binaryen IR, in wasm must be i32
      (i32.const 3)
      (return (i32.const 2))
      (return (i32.const 1))
    )
  )
  (func $unreachable-loop (result i32)
    (f64.abs
      (loop ;; note no type - valid in binaryen IR, in wasm must be i32
        (nop)
        (return (i32.const 1))
      )
    )
  )
  (func $unreachable-loop0 (result i32)
    (f64.abs
      (loop ;; note no type - valid in binaryen IR, in wasm must be i32
        (return (i32.const 1))
      )
    )
  )
  (func $unreachable-loop-toplevel (result i32)
    (loop ;; note no type - valid in binaryen IR, in wasm must be i32
      (nop)
      (return (i32.const 1))
    )
  )
  (func $unreachable-loop0-toplevel (result i32)
    (loop ;; note no type - valid in binaryen IR, in wasm must be i32
      (return (i32.const 1))
    )
  )
  (func $unreachable-ifs
    (if (unreachable) (nop))
    (if (unreachable) (unreachable))
    (if (unreachable) (nop) (nop))
    (if (unreachable) (unreachable) (nop))
    (if (unreachable) (nop) (unreachable))
    (if (unreachable) (unreachable) (unreachable))
    ;;
    (if (i32.const 1) (unreachable) (nop))
    (if (i32.const 1) (nop) (unreachable))
    (if (i32.const 1) (unreachable) (unreachable))
  )
  (func $unreachable-if-arm
    (if
      (i32.const 1)
      (block
        (nop)
      )
      (block
        (unreachable)
        (drop
          (i32.const 1)
        )
      )
    )
  )
  (func $local-to-stack (param $x i32) (result i32)
    (local $temp i32)
    (local.set $temp (call $local-to-stack (i32.const 1))) ;; this set could just be on the stack
    (drop (call $local-to-stack (i32.const 2)))
    (local.get $temp)
  )
  (func $local-to-stack-1 (param $x i32) (result i32)
    (local $temp i32)
    (local.set $temp (call $local-to-stack (i32.const 1)))
    (drop (call $local-to-stack (i32.const 2)))
    (i32.eqz
      (local.get $temp)
    )
  )
  (func $local-to-stack-1b (param $x i32) (result i32)
    (local $temp i32)
    (local.set $temp (call $local-to-stack (i32.const 1)))
    (drop (call $local-to-stack (i32.const 2)))
    (i32.add
      (local.get $temp)
      (i32.const 3)
    )
  )
  (func $local-to-stack-1c-no (param $x i32) (result i32)
    (local $temp i32)
    (local.set $temp (call $local-to-stack (i32.const 1)))
    (drop (call $local-to-stack (i32.const 2)))
    (i32.add
      (i32.const 3) ;; this is in the way
      (local.get $temp)
    )
  )
  (func $local-to-stack-2-no (param $x i32) (result i32)
    (local $temp i32)
    (local.set $temp (call $local-to-stack (i32.const 1)))
    (drop (call $local-to-stack (i32.const 2)))
    (i32.add
      (local.get $temp)
      (local.get $temp) ;; a second use - so cannot stack it
    )
  )
  (func $local-to-stack-3-no (param $x i32) (result i32)
    (local $temp i32)
    (if (i32.const 1)
      (local.set $temp (call $local-to-stack (i32.const 1)))
      (local.set $temp (call $local-to-stack (i32.const 2))) ;; two sets for that get
    )
    (drop (call $local-to-stack (i32.const 3)))
    (local.get $temp)
  )
  (func $local-to-stack-multi-4 (param $x i32) (result i32)
    (local $temp1 i32)
    (local $temp2 i32)
    (local.set $temp1 (call $local-to-stack-multi-4 (i32.const 1)))
    (drop (call $local-to-stack-multi-4 (i32.const 2)))
    (drop (local.get $temp1))
    (local.set $temp1 (call $local-to-stack-multi-4 (i32.const 3))) ;; same local, used later
    (drop (call $local-to-stack-multi-4 (i32.const 4)))
    (local.get $temp1)
  )
  (func $local-to-stack-multi-5 (param $x i32) (result i32)
    (local $temp1 i32)
    (local $temp2 i32)
    (local.set $temp1 (call $local-to-stack-multi-4 (i32.const 1)))
    (drop (call $local-to-stack-multi-4 (i32.const 2)))
    (drop (local.get $temp1))
    (local.set $temp2 (call $local-to-stack-multi-4 (i32.const 3))) ;; different local, used later
    (drop (call $local-to-stack-multi-4 (i32.const 4)))
    (local.get $temp2)
  )
  (func $local-to-stack-multi-6-justone (param $x i32) (result i32)
    (local $temp1 i32)
    (local $temp2 i32)
    (local.set $temp1 (call $local-to-stack-multi-4 (i32.const 1)))
    (drop (call $local-to-stack-multi-4 (i32.const 2)))
    (drop (local.get $temp1))
    (local.set $temp2 (call $local-to-stack-multi-4 (i32.const 3))) ;; different local, used later
    (drop (call $local-to-stack-multi-4 (i32.const 4)))
    (i32.add
      (local.get $temp2)
      (local.get $temp2)
    )
  )
  (func $local-to-stack-multi-7-justone (param $x i32) (result i32)
    (local $temp1 i32)
    (local $temp2 i32)
    (local.set $temp1 (call $local-to-stack-multi-4 (i32.const 1)))
    (drop (call $local-to-stack-multi-4 (i32.const 2)))
    (drop
      (i32.add
        (local.get $temp1)
        (local.get $temp1)
      )
    )
    (local.set $temp2 (call $local-to-stack-multi-4 (i32.const 3))) ;; different local, used later
    (drop (call $local-to-stack-multi-4 (i32.const 4)))
    (local.get $temp2)
  )
  (func $local-to-stack-overlapping-multi-8-no (param $x i32) (result i32)
    (local $temp1 i32)
    (local $temp2 i32)
    (local.set $temp1 (call $local-to-stack-multi-4 (i32.const 1)))
    (local.set $temp2 (call $local-to-stack-multi-4 (i32.const 1)))
    (drop (call $local-to-stack-multi-4 (i32.const 3)))
    (i32.add
      (local.get $temp2) ;; the timing
      (local.get $temp1) ;; it sucks
    )
  )
  (func $local-to-stack-overlapping-multi-9-yes (param $x i32) (result i32)
    (local $temp1 i32)
    (local $temp2 i32)
    (local.set $temp1 (call $local-to-stack-multi-4 (i32.const 1)))
    (local.set $temp2 (call $local-to-stack-multi-4 (i32.const 1)))
    (drop (call $local-to-stack-multi-4 (i32.const 3)))
    (i32.add
      (local.get $temp1) ;; the stars align
      (local.get $temp2) ;; and a time presents itself
    )
  )
  (func $local-to-stack-through-control-flow
    (local $temp1 i32)
    (local $temp2 i32)
    (local.set $temp2 (call $local-to-stack-multi-4 (i32.const 0)))
    (local.set $temp1 (call $local-to-stack-multi-4 (i32.const 1)))
    (if (i32.const 0) (nop))
    (drop (local.get $temp1))
    (local.set $temp1 (call $local-to-stack-multi-4 (i32.const 2)))
    (block $block (br $block))
    (drop (local.get $temp1))
    (drop (local.get $temp2))
  )
  (func $local-to-stack-in-control-flow
    (local $temp1 i32)
    (if (i32.const 0)
      (block
        (local.set $temp1 (call $local-to-stack-multi-4 (i32.const 0)))
        (drop (local.get $temp1))
      )
      (block
        (local.set $temp1 (call $local-to-stack-multi-4 (i32.const 1)))
        (drop (local.get $temp1))
      )
    )
  )
  (func $remove-block (param $x i32) (result i32)
   (local $temp i32)
   (i32.add
    (call $remove-block (i32.const 0))
    (i32.eqz
     (block (result i32) ;; after we use the stack instead of the local, we can remove this block
      (local.set $temp (call $remove-block (i32.const 1)))
      (drop (call $remove-block (i32.const 2)))
      (local.get $temp)
     )
    )
   )
  )
)