(module
  (memory 1 1)
  (func $consts
    (drop
      (i32.load (i32.const 0))
    )
    (drop
      (i32.load (i32.const 1))
    )
    (drop
      (i32.load (i32.const 1023))
    )
    (drop
      (i32.load (i32.const 1024))
    )
    (drop
      (i32.load offset=0 (i32.const 0))
    )
    (drop
      (i32.load offset=1 (i32.const 0))
    )
    (drop
      (i32.load offset=1023 (i32.const 0))
    )
    (drop
      (i32.load offset=1024 (i32.const 0))
    )
    (drop
      (i32.load offset=512 (i32.const 511))
    )
    (drop
      (i32.load offset=512 (i32.const 512))
    )
    (i32.store (i32.const 1) (i32.const 1))
  )
  (func $offsets (param $x i32)
    (drop
      (i32.load
        (i32.add
          (local.get $x)
          (i32.const 1)
        )
      )
    )
    (drop
      (i32.load
        (i32.add
          (local.get $x)
          (i32.const 8)
        )
      )
    )
    (drop
      (i32.load
        (i32.add
          (local.get $x)
          (i32.const 1023)
        )
      )
    )
    (drop
      (i32.load
        (i32.add
          (local.get $x)
          (i32.const 1024)
        )
      )
    )
    (drop
      (i32.load
        (i32.add
          (local.get $x)
          (i32.const 2048)
        )
      )
    )
    (drop
      (i32.load
        (i32.add
          (i32.const 4)
          (local.get $x)
        )
      )
    )
  )
  (func $load-off-2 (param $0 i32) (result i32)
    (i32.store offset=2
      (i32.add
        (i32.const 1)
        (i32.const 3)
      )
      (local.get $0)
    )
    (i32.store offset=2
      (i32.add
        (i32.const 3)
        (i32.const 1)
      )
      (local.get $0)
    )
    (i32.store offs