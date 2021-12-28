(module
  (memory 0)
  (type $0 (func))
  (func $erase (type $0)
    (nop)
  )
  (func $other (type $0)
    (nop)
  )
)
(module
  (memory 0)
  (type $0 (func))
  (func $keep2 (type $0)
    (drop
      (i32.const 0)
    )
  )
  (func $other (type $0)
    (nop)
  )
)
(module
  (memory 0)
  (type $0 (func))
  (func $erase (type $0)
    (drop
      (i32.const 0)
    )
  )
  (func $other (type $0)
    (drop
      (i32.const 0)
    )
  )
)
(module
  (memory 0)
  (type $0 (func))
  (func $keep2 (type $0)
    (drop
      (i32.const 0)
    )
  )
  (func $other (type $0)
    (drop
      (i32.const 1)
    )
  )
)
(module
  (memory 0)
  (start $other)
  (type $0 (func))
  (export "keep2" $keep2)
  (export "other" $other)
  (table 3 3 funcref)
  (elem (i32.const 0) $keep2 $other $caller)
  (func $keep2 (type $0)
    (nop)
  )
  (func $other (type $0)
    (nop)
  )
  (func $caller (type $0)
    (call $keep2)
    (call $other)
  )
)
(module
  (memory 0)
  (type $0 (func))
  (func $keep2-after-two-passes (type $0)
    (nop)
  )
  (func $other (type $0)
    (nop)
  )
  (func $keep-caller (type $0)
    (call $keep2-after-two-passes)
  )
  (func $other-caller (type $0)
    (call $other)
  )
)
(module
  (memory 0)
  (type $0 (func))
  (func $keep-4 (type $0)
    (nop)
  )
  (func $other (type $0)
    (unreachable)
  )
  (func $keep-caller (type $0)
    (call $keep-4)
  )
  (func $other-caller (type $0)
    (call $other)
  )
)
(module
  (memory 0)
  (type $T (func (result i32)))
  (type $S (func (result i32)))
  (type $2 (func))
  (type $3 (func (param i32)))
  (func $keep4-similar-but-func-sig-differs (type $2)
    (drop
      (i32.const 0)
    )
  )
  (func $other1 (type $3) (param $i i32)
    (drop
      (i32.const 0)
    )
  )
  (func $other2 (type $T) (result i32)
    (i32.const 0)
  )
  (func $other3 (type $S) (result i32)
    (i32.const 0)
  )
)
(module
  (memory 0)
  (type $S (func (result i32)))
  (type $1 (func (param i32)))
  (func $keep2-similar-but-func-sig-differs (type $1) (param $i i32)
    (drop
      (i32.const 0)
    )
  )
  (func $other1 (type $1) (param $i i32)
    (drop
      (i32.const 0)
    )
  )
  (func $other2 (type $S) (result i32)
    (i32.const 0)
  )
  (func $other3 (type $S) (result i32)
    (i32.const 0)
  )
)
(module
  (memory 0)
  (type $0 (func))
  (func $keep2 (type $0)
    (nop)
  )
  (func $other (type $0)
    (nop)
    (nop)
  )
)
(module
  (memory 0)
  (type $0 (func))
  (func $erase (type $0)
    (block $block0
    )
  )
  (func $other (type $0)
    (block $block0
    )
  )
)
(module
  (memory 0)
  (type $0 (func))
  (func $keep2 (type $0)
    (block $block0
    )
  )
  (func $other (type $0)
    (block $block0
      (nop)
    )
  )
)
(module
  (memory 0)
  (type $0 (func))
  (func $erase (type $0)
    (block $block0
      (nop)
    )
  )
  (func $other (type $0)
    (block $block0
      (nop)
    )
  )
)
(module
  (memory 0)
  (type $0 (func))
  (func $keep2 (type $0)
    (block $block0
      (nop)
    )
  )
  (func $other (type $0)
    (block $block0
      (nop)
      (unreachable)
    )
  )
)
(module
  (memory 0)
  (type $0 (func))
  (func $keep2 (type $0)
    (block $block0
      (nop)
    )
  )
  (func $other (type $0)
    (block $block0
      (unreachable)
    )
  )
)
(module
  (memory 0)
  (type $0 (func))
  (func $erase-since-block-names-do-not-matter (type $0)
    (block $foo
    )
  )
  (func $other (type $0)
    (block $bar
    )
  )
)
(module
  (memory 0)
  (type $0 (func))
  (func $erase-since-block-names-do-not-matter (type $0)
    (block $foo
      (br $foo)
      (br_table $foo $foo
        (i32.const 0)
      )
    )
  )
  (func $other (type $0)
    (block $bar
      (br $bar)
      (br_table $bar $bar
        (i32.const 0)
      )
    )
  )
)
(module
  (memory 0)
  (type $0 (func))
  (func $keep2 (type $0)
    (block $foo
      (block
        (drop
          (i32.const 0)
        )
        (br $foo)
      )
    )
  )
  (func $other (type $0)
    (block $bar
      (block
        (drop
          (i32.const 1)
        )
        (br $bar)
      )
    )
  )
)
(module
  (memory 0)
  (type $0 (func))
  (func $keep2 (type $0)
    (block $foo
      (br_if $foo
        (i32.const 0)
      )
    )
  )
  (func $other (type $0)
    (block $bar
      (br_if $bar
        (i32.const 1)
      )
    )
  )
)
(module
  (memory 0)
  (type $0 (func))
  (func $erase (type $0)
    (block $foo
      (br_if $foo
        (i32.const 0)
      )
    )
  )
  (func $other (type $0)
    (block $bar
      (br_if $bar
        (i32.const 0)
      )
    )
  )
)
(module
  (memory 0)
  (type $0 (func))
  (func $keep2 (type $0)
    (block $foo
      (br_table $foo $foo
        (i32.const 0)
      )
    )
  )
  (func $other (type $0)
    (block $bar
      (br_table $bar $bar
        (i32.const 1)
      )
    )
  )
)
(module
  (memory 0)
  (type $0 (func))
  (func $erase (type $0)
    (loop $bar
      (nop)
    )
  )
  (func $other (type $0)
    (loop $sjc
      (nop)
    )
  )
)
(module
  (memory 0)
  (type $0 (func))
  (func $keep2 (type $0)
    (drop
      (block $foo (result i32)
        (br_table $foo $foo
          (i32.const 0)
          (i32.const 0)
        )
      )
    )
  )
  (func $other (type $0)
    (drop
      (block $bar (result i32)
        (br_table $bar $bar
          (i32.const 0)
          (i32.const 1)
        )
      )
    )
  )
)
(module
  (memory 0)
  (type $0 (func))
  (func $keep2 (type $0)
    (block $foo
      (block $bar
        (br_table $foo $bar
          (i32.const 0)
        )
      )
    )
  )
  (func $other (type $0)
    (block $bar
      (block $foo
        (br_table $bar $foo
          (i32.const 0)
        )
      )
    )
  )
)
(module
  (memory 0)
  (type $0 (func))
  (func $erase (type $0)
    (block $foo
      (block $bar
        (br_table $foo $bar
          (i32.const 0)
        )
      )
    )
  )
  (func $other (type $0)
    (block $bar
      (block $foo
        (br_table $foo $bar
          (i32.const 0)
        )
      )
    )
  )
)
(module
  (memory 0)
  (type $0 (func))
  (func $erase (type $0)
    (call $erase)
  )
  (func $other (type $0)
    (call $erase)
  )
)
(module
  (memory 0)
  (type $0 (func))
  (func $keep2-but-in-theory-we-could-erase (type $0)
    (call $keep2-but-in-theory-we-could-erase)
  )
  (func $other (type $0)
    (call $other)
  )
)
(module
  (memory 0)
  (type $FUNCSIG$v (func))
  (import $i "env" "i")
  (import $j "env" "j")
  (func $erase (type $FUNCSIG$v)
    (call $i)
  )
  (func $other (type $FUNCSIG$v)
    (call $i)
  )
)
(module
  (memory 0)
  (type $FUNCSIG$v (func))
  (import $i "env" "i")
  (import $j "env" "j")
  (func $keep2 (type $FUNCSIG$v)
    (call $i)
  )
  (func $other (type $FUNCSIG$v)
    (call $j)
  )
)
(module
  (memory 0)
  (type $T (func))
  (table 2 2 funcref)
  (elem (i32.const 0)  $erase $other)
  (func $erase (type $T)
    (call_indirect (type $T)
      (i32.const 0)
    )
  )
  (func $other (type $T)
    (call_indirect (type $T)
      (i32.const 0)
    )
  )
)
(module
  (memory 0)
  (type $T (func))
  (table 2 2 funcref)
  (elem (i32.const 0)  $keep2 $other)
  (func $keep2 (type $T)
    (call_indirect (type $T)
      (i32.const 0)
    )
  )
  (func $other (type $T)
    (call_indirect (type $T)
      (i32.const 1)
    )
  )
)
(module
  (memory 0)
  (type $T (func))
  (type $S (func))
  (table 2 2 funcref)
  (elem (i32.const 0)  $keep2 $other)
  (func $keep2 (type $T)
    (call_indirect (type $T)
      (i32.const 0)
    )
  )
  (func $other (type $T)
    (call_indirect (type $S)
      (i32.const 0)
    )
  )
)
(module
  (memory 0)
  (type $0 (func))
  (func $erase-even-locals-with-different-names (type $0)
    (local $i i32)
    (drop
      (local.get $i)
    )
  )
  (func $other (type $0)
    (local $j i32)
    (drop
      (local.get $j)
    )
  )
)
(module
  (memory 0)
  (type $0 (func))
  (func $keep2 (type $0)
    (local $i i32)
    (drop
      (local.get $i)
    )
  )
  (func $other (type $0)
    (local $j i64)
    (drop
      (local.get $j)
    )
  )
)
(module
  (memory 0)
  (type $0 (func))
  (func $erase-even-locals-with-different-names (type $0)
    (local $i i32)
    (local.set $i
      (i32.const 0)
    )
  )
  (func $other (type $0)
    (local $j i32)
    (local.set $j
      (i32.const 0)
    )
  )
)
(module
  (memory 0)
  (type $0 (func))
  (func $keep2 (type $0)
    (local $i i32)
    (local.set $i
      (i32.const 0)
    )
  )
  (func $other (type $0)
    (local $j i64)
    (local.set $j
      (i64.const 0)
    )
  )
)
(module
  (memory 0)
  (type $0 (func))
  (func $keep2 (type $0)
    (local $i i32)
    (local.set $i
      (i32.const 0)
    )
  )
  (func $other (type $0)
    (local $j i32)
    (local.set $j
      (i32.const 1)
    )
  )
)
(module
  (memory 10)
  (type $0 (func))
  (func $erase (type $0)
    (drop
      (i32.load
        (i32.const 0)
      )
    )
    (drop
      (i32.load16_s offset=3 align=2
        (i32.const 0)
      )
    )
  )
  (func $other (type $0)
    (drop
      (i32.load
        (i32.const 0)
      )
    )
    (drop
      (i32.load16_s offset=3 align=2
        (i32.const 0)
      )
    )
  )
)
(module
  (memory 10)
  (type $0 (func))
  (func $keep2 (type $0)
    (drop
      (i32.load offset=3
        (i32.const 0)
      )
    )
  )
  (func $other (type $0)
    (drop
      (i32.load16_s offset=3 align=2
        (i32.const 0)
      )
    )
  )
)
(module
  (memory 10)
  (type $0 (func))
  (func $keep2 (type $0)
    (drop
      (i32.load16_s offset=3
        (i32.const 0)
      )
    )
  )
  (func $other (type $0)
    (drop
      (i32.load16_s offset=3 align=1
        (i32.const 0)
      )
    )
  )
)
(module
  (memory 10)
  (type $0 (func))
  (func $keep2 (type $0)
    (drop
      (i32.load16_s align=2
        (i32.const 0)
      )
    )
  )
  (func $other (type $0)
    (drop
      (i32.load16_s offset=3 align=2
        (i32.const 0)
      )
    )
  )
)
(module
  (memory 10)
  (type $0 (func))
  (func $keep2 (type $0)
    (drop
      (i32.load16_s offset=3 align=2
        (i32.const 0)
      )
    )
  )
  (func $other (type $0)
    (drop
      (i32.load16_s offset=3 align=2
        (i32.const 1)
      )
    )
  )
)
(module
  (memory 10)
  (type $0 (func))
  (func $keep2 (type $0)
    (drop
      (i32.load16_u offset=3 align=2
        (i32.const 0)
      )
    )
  )
  (func $other (type $0)
    (drop
      (i32.load16_s offset=3 align=2
        (i32.const 0)
      )
    )
  )
)
(module
  (memory 10)
  (type $0 (func))
  (func $erase (type $0)
    (i32.store
      (i32.const 0)
      (i32.const 100)
    )
    (i32.store16 offset=3 align=2
      (i32.const 0)
      (i32.const 100)
    )
  )
  (func $other (type $0)
    (i32.store
      (i32.const 0)
      (i32.const 100)
    )
    (i32.store16 offset=3 align=2
      (i32.const 0)
      (i32.const 100)
    )
  )
)
(module
  (memory 10)
  (type $0 (func))
  (func $keep2 (type $0)
    (i32.store offset=3
      (i32.const 0)
      (i32.const 100)
    )
  )
  (func $other (type $0)
    (i32.store16 offset=3 align=2
      (i32.const 0)
      (i32.const 100)
    )
  )
)
(module
  (memory 10)
  (type $0 (func))
  (func $keep2 (type $0)
    (i32.store16 offset=3
      (i32.const 0)
      (i32.const 100)
    )
  )
  (func $other (type $0)
    (i32.store16 offset=3 align=1
      (i32.const 0)
      (i32.const 100)
    )
  )
)
(module
  (memory 10)
  (type $0 (func))
  (func $keep2 (type $0)
    (i32.store16 align=2
      (i32.const 0)
      (i32.const 100)
    )
  )
  (func $other (type $0)
    (i32.store16 offset=3 align=2
      (i32.const 0)
      (i32.const 100)
    )
  )
)
(module
  (memory 10)
  (type $0 (func))
  (func $keep2 (type $0)
    (i32.store16 offset=3 align=2
      (i32.const 0)
      (i32.const 100)
    )
  )
  (func $other (type $0)
    (i32.store16 offset=3 align=2
      (i32.const 1)
      (i32.const 100)
    )
  )
)
(module
  (memory 10)
  (type $0 (func))
  (func $keep2 (type $0)
    (i32.store16 offset=3 align=2
      (i32.const 0)
      (i32.const 100)
    )
  )
  (func $other (type $0)
    (i32.store16 offset=3 align=2