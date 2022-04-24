;; This tests that the stack overflow guard page can't be skipped by a function with more than a page of locals.
(module
  (memory 1)
  (export "test-guard-page-skip" (func $test-guard-page-skip))

  (func $test-guard-page-skip
    (param $depth i32)
    (if (i32.eq (local.get $depth) (i32.const 0))
      (then (call $function-with-many-locals))
      (else (call $test-guard-page-skip (i32.sub (local.get $depth) (i32.const 1))))
    )
  )

  (func $function-with-many-locals

    ;; 1056 i64 = 8448 bytes of locals
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x000-0x007
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x008-0x00f
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x010-0x017
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x018-0x01f
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x020-0x027
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x028-0x02f
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x030-0x037
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x038-0x03f
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x040-0x047
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x048-0x04f
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x050-0x057
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x058-0x05f
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x060-0x067
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x068-0x06f
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x070-0x077
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x078-0x07f
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x080-0x087
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x088-0x08f
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x090-0x097
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x098-0x09f
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x0a0-0x0a7
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x0a8-0x0af
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x0b0-0x0b7
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x0b8-0x0bf
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x0c0-0x0c7
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x0c8-0x0cf
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x0d0-0x0d7
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x0d8-0x0df
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x0e0-0x0e7
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x0e8-0x0ef
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x0f0-0x0f7
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x0f8-0x0ff

    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x100-0x107
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x108-0x10f
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x110-0x117
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x118-0x11f
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x120-0x127
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x128-0x12f
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x130-0x137
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x138-0x13f
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x140-0x147
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x148-0x14f
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x150-0x157
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x158-0x15f
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x160-0x167
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x168-0x16f
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x170-0x177
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x178-0x17f
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x180-0x187
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x188-0x18f
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x190-0x197
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x198-0x19f
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x1a0-0x1a7
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x1a8-0x1af
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x1b0-0x1b7
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x1b8-0x1bf
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x1c0-0x1c7
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x1c8-0x1cf
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x1d0-0x1d7
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x1d8-0x1df
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x1e0-0x1e7
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x1e8-0x1ef
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x1f0-0x1f7
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x1f8-0x1ff

    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x200-0x207
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x208-0x20f
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x210-0x217
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x218-0x21f
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x220-0x227
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x228-0x22f
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x230-0x237
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x238-0x23f
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x240-0x247
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x248-0x24f
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x250-0x257
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x258-0x25f
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x260-0x267
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x268-0x26f
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x270-0x277
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x278-0x27f
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x280-0x287
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x288-0x28f
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x290-0x297
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x298-0x29f
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x2a0-0x2a7
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x2a8-0x2af
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x2b0-0x2b7
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x2b8-0x2bf
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x2c0-0x2c7
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x2c8-0x2cf
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x2d0-0x2d7
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x2d8-0x2df
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x2e0-0x2e7
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x2e8-0x2ef
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x2f0-0x2f7
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x2f8-0x2ff

    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x300-0x307
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x308-0x30f
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x310-0x317
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x318-0x31f
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x320-0x327
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x328-0x32f
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x330-0x337
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x338-0x33f
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x340-0x347
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x348-0x34f
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x350-0x357
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x358-0x35f
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x360-0x367
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x368-0x36f
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x370-0x377
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x378-0x37f
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x380-0x387
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x388-0x38f
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x390-0x397
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x398-0x39f
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x3a0-0x3a7
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x3a8-0x3af
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x3b0-0x3b7
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x3b8-0x3bf
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x3c0-0x3c7
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x3c8-0x3cf
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x3d0-0x3d7
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x3d8-0x3df
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x3e0-0x3e7
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x3e8-0x3ef
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x3f0-0x3f7
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x3f8-0x3ff

    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x400-0x407
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x408-0x40f
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x410-0x417
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) ;; 0x418-0x41f

    ;; recurse first to try to make the callee access the stack below the space allocated for the locals before the locals themselves have been initialized.
    (call $function-with-many-locals)

    ;; load from memory into the locals
    (local.set 0x000 (i64.load offset=0x000 align=1 (i32.const 0)))
    (local.set 0x001 (i64.load offset=0x001 align=1 (i32.const 0)))
    (local.set 0x002 (i64.load offset=0x002 align=1 (i32.const 0)))
    (local.set 0x003 (i64.load offset=0x003 align=1 (i32.const 0)))
    (local.set 0x004 (i64.load offset=0x004 align=1 (i32.const 0)))
    (local.set 0x005 (i64.load offset=0x005 align=1 (i32.const 0)))
    (local.set 0x006 (i64.load offset=0x006 align=1 (i32.const 0)))
    (local.set 0x007 (i64.load offset=0x007 align=1 (i32.const 0)))
    (local.set 0x008 (i64.load offset=0x008 align=1 (i32.const 0)))
    (local.set 0x009 (i64.load offset=0x009 align=1 (i32.const 0)))
    (local.set 0x00a (i64.load offset=0x00a align=1 (i32.const 0)))
    (local.set 0x00b (i64.load offset=0x00b align=1 (i32.const 0)))
    (local.set 0x00c (i64.load offset=0x00c align=1 (i32.const 0)))
    (local.set 0x00d (i64.load offset=0x00d align=1 (i32.const 0)))
    (local.set 0x00e (i64.load offset=0x00e align=1 (i32.const 0)))
    (local.set 0x00f (i64.load offset=0x00f align=1 (i32.const 0)))
    (local.set 0x010 (i64.load offset=0x010 align=1 (i32.const 0)))
    (local.set 0x011 (i64.load offset=0x011 align=1 (i32.const 0)))
    (local.set 0x012 (i64.load offset=0x012 align=1 (i32.const 0)))
    (local.set 0x013 (i64.load offset=0x013 align=1 (i32.const 0)))
    (local.set 0x014 (i64.load offset=0x014 align=1 (i32.const 0)))
    (local.set 0x015 (i64.load offset=0x015 align=1 (i32.const 0)))
    (local.set 0x016 (i64.load offset=0x016 align=1 (i32.const 0)))
    (local.set 0x017 (i64.load offset=0x017 align=1 (i32.const 0)))
    (local.set 0x018 (i64.load offset=0x018 align=1 (i32.const 0)))
    (local.set 0x019 (i64.load offset=0x019 align=1 (i32.const 0)))
    (local.set 0x01a (i64.load offset=0x01a align=1 (i32.const 0)))
    (local.set 0x01b (i64.load offset=0x01b align=1 (i32.const 0)))
    (local.set 0x01c (i64.load offset=0x01c align=1 (i32.const 0)))
    (local.set 0x01d (i64.load offset=0x01d align=1 (i32.const 0)))
    (local.set 0x01e (i64.load offset=0x01e align=1 (i32.const 0)))
    (local.set 0x01f (i64.load offset=0x01f align=1 (i32.const 0)))
    (local.set 0x020 (i64.load offset=0x020 align=1 (i32.const 0)))
    (local.set 0x021 (i64.load offset=0x021 align=1 (i32.const 0)))
    (local.set 0x022 (i64.load offset=0x022 align=1 (i32.const 0)))
    (local.set 0x023 (i64.load offset=0x023 align=1 (i32.const 0)))
    (local.set 0x024 (i64.load offset=0x024 align=1 (i32.const 0)))
    (local.set 0x025 (i64.load offset=0x025 align=1 (i32.const 0)))
    (local.set 0x026 (i64.load offset=0x026 align=1 (i32.const 0)))
    (local.set 0x027 (i64.load offset=0x027 align=1 (i32.const 0)))
    (local.set 0x028 (i64.load offset=0x028 align=1 (i32.const 0)))
    (local.set 0x029 (i64.load offset=0x029 align=1 (i32.const 0)))
    (local.set 0x02a (i64.load offset=0x02a align=1 (i32.const 0)))
    (local.set 0x02b (i64.load offset=0x02b align=1 (i32.const 0)))
    (local.set 0x02c (i64.load offset=0x02c align=1 (i32.const 0)))
    (local.set 0x02d (i64.load offset=0x02d align=1 (i32.const 0)))
    (local.set 0x02e (i64.load offset=0x02e align=1 (i32.const 0)))
    (local.set 0x02f (i64.load offset=0x02f align=1 (i32.const 0)))
    (local.set 0x030 (i64.load offset=0x030 align=1 (i32.const 0)))
    (local.set 0x031 (i64.load offset=0x031 align=1 (i32.const 0)))
    (local.set 0x032 (i64.load offset=0x032 align=1 (i32.const 0)))
    (local.set 0x033 (i64.load offset=0x033 align=1 (i32.const 0)))
    (local.set 0x034 (i64.load offset=0x034 align=1 (i32.const 0)))
    (local.set 0x035 (i64.load offset=0x035 align=1 (i32.const 0)))
    (local.set 0x036 (i64.load offset=0x036 align=1 (i32.const 0)))
    (local.set 0x037 (i64.load offset=0x037 align=1 (i32.const 0)))
    (local.set 0x038 (i64.load offset=0x038 align=1 (i32.const 0)))
    (local.set 0x039 (i64.load offset=0x039 align=1 (i32.const 0)))
    (local.set 0x03a (i64.load offset=0x03a align=1 (i32.const 0)))
    (local.set 0x03b (i64.load offset=0x03b align=1 (i32.const 0)))
    (local.set 0x03c (i64.load offset=0x03c align=1 (i32.const 0)))
    (local.set 0x03d (i64.load offset=0x03d align=1 (i32.const 0)))
    (local.set 0x03e (i64.load offset=0x03e align=1 (i32.const 0)))
    (local.set 0x03f (i64.load offset=0x03f align=1 (i32.const 0)))
    (local.set 0x040 (i64.load offset=0x040 align=1 (i32.const 0)))
    (local.set 0x041 (i64.load offset=0x041 align=1 (i32.const 0)))
    (local.set 0x042 (i64.load offset=0x042 align=1 (i32.const 0)))
    (local.set 0x043 (i64.load offset=0x043 align=1 (i32.const 0)))
    (local.set 0x044 (i64.load offset=0x044 align=1 (i32.const 0)))
    (local.set 0x045 (i64.load offset=0x045 align=1 (i32.const 0)))
    (local.set 0x046 (i64.load offset=0x046 align=1 (i32.const 0)))
    (local.set 0x047 (i64.load offset=0x047 align=1 (i32.const 0)))
    (local.set 0x048 (i64.load offset=0x048 align=1 (i32.const 0)))
    (local.set 0x049 (i64.load offset=0x049 align=1 (i32.const 0)))
    (local.set 0x04a (i64.load offset=0x04a align=1 (i32.const 0)))
    (local.set 0x04b (i64.load offset=0x04b align=1 (i32.const 0)))
    (local.set 0x04c (i64.load offset=0x04c align=1 (i32.const 0)))
    (local.set 0x04d (i64.load offset=0x04d align=1 (i32.const 0)))
    (local.set 0x04e (i64.load offset=0x04e align=1 (i32.const 0)))
    (local.set 0x04f (i64.load offset=0x04f align=1 (i32.const 0)))
    (local.set 0x050 (i64.load offset=0x050 align=1 (i32.const 0)))
    (local.set 0x051 (i64.load offset=0x051 align=1 (i32.const 0)))
    (local.set 0x052 (i64.load offset=0x052 align=1 (i32.const 0)))
    (local.set 0x053 (i64.load offset=0x053 align=1 (i32.const 0)))
    (local.set 0x054 (i64.load offset=0x054 align=1 (i32.const 0)))
    (local.set 0x055 (i64.load offset=0x055 align=1 (i32.const 0)))
    (local.set 0x056 (i64.load offset=0x056 align=1 (i32.const 0)))
    (local.set 0x057 (i64.load offset=0x057 align=1 (i32.const 0)))
    (local.set 0x058 (i64.load offset=0x058 align=1 (i32.const 0)))
    (local.set 0x059 (i64.load offset=0x059 align=1 (i32.const 0)))
    (local.set 0x05a (i64.load offset=0x05a align=1 (i32.const 0)))
    (local.set 0x05b (i64.load offset=0x05b align=1 (i32.const 0)))
    (local.set 0x05c (i64.load offset=0x05c align=1 (i32.const 0)))
    (local.set 0x05d (i64.load offset=0x05d align=1 (i32.const 0)))
    (local.set 0x05e (i64.load offset=0x05e align=1 (i32.const 0)))
    (local.set 0x05f (i64.load offset=0x05f align=1 (i32.const 0)))
    (local.set 0x060 (i64.load offset=0x060 align=1 (i32.const 0)))
    (local.set 0x061 (i64.load offset=0x061 align=1 (i32.const 0)))
    (local.set 0x062 (i64.load offset=0x062 align=1 (i32.const 0)))
    (local.set 0x063 (i64.load offset=0x063 align=1 (i32.const 0)))
    (local.set 0x064 (i64.load offset=0x064 align=1 (i32.const 0)))
    (local.set 0x065 (i64.load offset=0x065 align=1 (i32.const 0)))
    (local.set 0x066 (i64.load offset=0x066 align=1 (i32.const 0)))
    (local.set 0x067 (i64.load offset=0x067 align=1 (i32.const 0)))
    (local.set 0x068 (i64.load offset=0x068 align=1 (i32.const 0)))
    (local.set 0x069 (i64.load offset=0x069 align=1 (i32.const 0)))
    (local.set 0x06a (i64.load offset=0x06a align=1 (i32.const 0)))
    (local.set 0x06b (i64.load offset=0x06b align=1 (i32.const 0)))
    (local.set 0x06c (i64.load offset=0x06c align=1 (i32.const 0)))
    (local.set 0x06d (i64.load offset=0x06d align=1 (i32.const 0)))
    (local.set 0x06e (i64.load offset=0x06e align=1 (i32.const 0)))
    (local.set 0x06f (i64.load offset=0x06f align=1 (i32.const 0)))
    (local.set 0x070 (i64.load offset=0x070 align=1 (i32.const 0)))
    (local.set 0x071 (i64.load offset=0x071 align=1 (i32.const 0)))
    (local.set 0x072 (i64.load offset=0x072 align=1 (i32.const 0)))
    (local.set 0x073 (i64.load offset=0x073 align=1 (i32.const 0)))
    (local.set 0x074 (i64.load offset=0x074 align=1 (i32.const 0)))
    (local.set 0x075 (i64.load offset=0x075 align=1 (i32.const 0)))
    (local.set 0x076 (i64.load offset=0x076 align=1 (i32.const 0)))
    (local.set 0x077 (i64.load offset=0x077 align=1 (i32.const 0)))
    (local.set 0x078 (i64.load offset=0x078 align=1 (i32.const 0)))
    (local.set 0x079 (i64.load offset=0x079 align=1 (i32.const 0)))
    (local.set 0x07a (i64.load offset=0x07a align=1 (i32.const 0)))
    (local.set 0x07b (i64.load offset=0x07b align=1 (i32.const 0)))
    (local.set 0x07c (i64.load offset=0x07c align=1 (i32.const 0)))
    (local.set 0x07d (i64.load offset=0x07d align=1 (i32.const 0)))
    (local.set 0x07e (i64.load offset=0x07e align=1 (i32.const 0)))
    (local.set 0x07f (i64.load offset=0x07f align=1 (i32.const 0)))
    (local.set 0x080 (i64.load offset=0x080 align=1 (i32.const 0)))
    (local.set 0x081 (i64.load offset=0x081 align=1 (i32.const 0)))
    (local.set 0x082 (i64.load offset=0x082 align=1 (i32.const 0)))
    (local.set 0x083 (i64.load offset=0x083 align=1 (i32.const 0)))
    (local.set 0x084 (i64.load offset=0x084 align=1 (i32.const 0)))
    (local.set 0x085 (i64.load offset=0x085 align=1 (i32.const 0)))
    (local.set 0x086 (i64.load offset=0x