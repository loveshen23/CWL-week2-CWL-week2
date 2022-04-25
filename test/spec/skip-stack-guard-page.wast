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
    (local.set 0x086 (i64.load offset=0x086 align=1 (i32.const 0)))
    (local.set 0x087 (i64.load offset=0x087 align=1 (i32.const 0)))
    (local.set 0x088 (i64.load offset=0x088 align=1 (i32.const 0)))
    (local.set 0x089 (i64.load offset=0x089 align=1 (i32.const 0)))
    (local.set 0x08a (i64.load offset=0x08a align=1 (i32.const 0)))
    (local.set 0x08b (i64.load offset=0x08b align=1 (i32.const 0)))
    (local.set 0x08c (i64.load offset=0x08c align=1 (i32.const 0)))
    (local.set 0x08d (i64.load offset=0x08d align=1 (i32.const 0)))
    (local.set 0x08e (i64.load offset=0x08e align=1 (i32.const 0)))
    (local.set 0x08f (i64.load offset=0x08f align=1 (i32.const 0)))
    (local.set 0x090 (i64.load offset=0x090 align=1 (i32.const 0)))
    (local.set 0x091 (i64.load offset=0x091 align=1 (i32.const 0)))
    (local.set 0x092 (i64.load offset=0x092 align=1 (i32.const 0)))
    (local.set 0x093 (i64.load offset=0x093 align=1 (i32.const 0)))
    (local.set 0x094 (i64.load offset=0x094 align=1 (i32.const 0)))
    (local.set 0x095 (i64.load offset=0x095 align=1 (i32.const 0)))
    (local.set 0x096 (i64.load offset=0x096 align=1 (i32.const 0)))
    (local.set 0x097 (i64.load offset=0x097 align=1 (i32.const 0)))
    (local.set 0x098 (i64.load offset=0x098 align=1 (i32.const 0)))
    (local.set 0x099 (i64.load offset=0x099 align=1 (i32.const 0)))
    (local.set 0x09a (i64.load offset=0x09a align=1 (i32.const 0)))
    (local.set 0x09b (i64.load offset=0x09b align=1 (i32.const 0)))
    (local.set 0x09c (i64.load offset=0x09c align=1 (i32.const 0)))
    (local.set 0x09d (i64.load offset=0x09d align=1 (i32.const 0)))
    (local.set 0x09e (i64.load offset=0x09e align=1 (i32.const 0)))
    (local.set 0x09f (i64.load offset=0x09f align=1 (i32.const 0)))
    (local.set 0x0a0 (i64.load offset=0x0a0 align=1 (i32.const 0)))
    (local.set 0x0a1 (i64.load offset=0x0a1 align=1 (i32.const 0)))
    (local.set 0x0a2 (i64.load offset=0x0a2 align=1 (i32.const 0)))
    (local.set 0x0a3 (i64.load offset=0x0a3 align=1 (i32.const 0)))
    (local.set 0x0a4 (i64.load offset=0x0a4 align=1 (i32.const 0)))
    (local.set 0x0a5 (i64.load offset=0x0a5 align=1 (i32.const 0)))
    (local.set 0x0a6 (i64.load offset=0x0a6 align=1 (i32.const 0)))
    (local.set 0x0a7 (i64.load offset=0x0a7 align=1 (i32.const 0)))
    (local.set 0x0a8 (i64.load offset=0x0a8 align=1 (i32.const 0)))
    (local.set 0x0a9 (i64.load offset=0x0a9 align=1 (i32.const 0)))
    (local.set 0x0aa (i64.load offset=0x0aa align=1 (i32.const 0)))
    (local.set 0x0ab (i64.load offset=0x0ab align=1 (i32.const 0)))
    (local.set 0x0ac (i64.load offset=0x0ac align=1 (i32.const 0)))
    (local.set 0x0ad (i64.load offset=0x0ad align=1 (i32.const 0)))
    (local.set 0x0ae (i64.load offset=0x0ae align=1 (i32.const 0)))
    (local.set 0x0af (i64.load offset=0x0af align=1 (i32.const 0)))
    (local.set 0x0b0 (i64.load offset=0x0b0 align=1 (i32.const 0)))
    (local.set 0x0b1 (i64.load offset=0x0b1 align=1 (i32.const 0)))
    (local.set 0x0b2 (i64.load offset=0x0b2 align=1 (i32.const 0)))
    (local.set 0x0b3 (i64.load offset=0x0b3 align=1 (i32.const 0)))
    (local.set 0x0b4 (i64.load offset=0x0b4 align=1 (i32.const 0)))
    (local.set 0x0b5 (i64.load offset=0x0b5 align=1 (i32.const 0)))
    (local.set 0x0b6 (i64.load offset=0x0b6 align=1 (i32.const 0)))
    (local.set 0x0b7 (i64.load offset=0x0b7 align=1 (i32.const 0)))
    (local.set 0x0b8 (i64.load offset=0x0b8 align=1 (i32.const 0)))
    (local.set 0x0b9 (i64.load offset=0x0b9 align=1 (i32.const 0)))
    (local.set 0x0ba (i64.load offset=0x0ba align=1 (i32.const 0)))
    (local.set 0x0bb (i64.load offset=0x0bb align=1 (i32.const 0)))
    (local.set 0x0bc (i64.load offset=0x0bc align=1 (i32.const 0)))
    (local.set 0x0bd (i64.load offset=0x0bd align=1 (i32.const 0)))
    (local.set 0x0be (i64.load offset=0x0be align=1 (i32.const 0)))
    (local.set 0x0bf (i64.load offset=0x0bf align=1 (i32.const 0)))
    (local.set 0x0c0 (i64.load offset=0x0c0 align=1 (i32.const 0)))
    (local.set 0x0c1 (i64.load offset=0x0c1 align=1 (i32.const 0)))
    (local.set 0x0c2 (i64.load offset=0x0c2 align=1 (i32.const 0)))
    (local.set 0x0c3 (i64.load offset=0x0c3 align=1 (i32.const 0)))
    (local.set 0x0c4 (i64.load offset=0x0c4 align=1 (i32.const 0)))
    (local.set 0x0c5 (i64.load offset=0x0c5 align=1 (i32.const 0)))
    (local.set 0x0c6 (i64.load offset=0x0c6 align=1 (i32.const 0)))
    (local.set 0x0c7 (i64.load offset=0x0c7 align=1 (i32.const 0)))
    (local.set 0x0c8 (i64.load offset=0x0c8 align=1 (i32.const 0)))
    (local.set 0x0c9 (i64.load offset=0x0c9 align=1 (i32.const 0)))
    (local.set 0x0ca (i64.load offset=0x0ca align=1 (i32.const 0)))
    (local.set 0x0cb (i64.load offset=0x0cb align=1 (i32.const 0)))
    (local.set 0x0cc (i64.load offset=0x0cc align=1 (i32.const 0)))
    (local.set 0x0cd (i64.load offset=0x0cd align=1 (i32.const 0)))
    (local.set 0x0ce (i64.load offset=0x0ce align=1 (i32.const 0)))
    (local.set 0x0cf (i64.load offset=0x0cf align=1 (i32.const 0)))
    (local.set 0x0d0 (i64.load offset=0x0d0 align=1 (i32.const 0)))
    (local.set 0x0d1 (i64.load offset=0x0d1 align=1 (i32.const 0)))
    (local.set 0x0d2 (i64.load offset=0x0d2 align=1 (i32.const 0)))
    (local.set 0x0d3 (i64.load offset=0x0d3 align=1 (i32.const 0)))
    (local.set 0x0d4 (i64.load offset=0x0d4 align=1 (i32.const 0)))
    (local.set 0x0d5 (i64.load offset=0x0d5 align=1 (i32.const 0)))
    (local.set 0x0d6 (i64.load offset=0x0d6 align=1 (i32.const 0)))
    (local.set 0x0d7 (i64.load offset=0x0d7 align=1 (i32.const 0)))
    (local.set 0x0d8 (i64.load offset=0x0d8 align=1 (i32.const 0)))
    (local.set 0x0d9 (i64.load offset=0x0d9 align=1 (i32.const 0)))
    (local.set 0x0da (i64.load offset=0x0da align=1 (i32.const 0)))
    (local.set 0x0db (i64.load offset=0x0db align=1 (i32.const 0)))
    (local.set 0x0dc (i64.load offset=0x0dc align=1 (i32.const 0)))
    (local.set 0x0dd (i64.load offset=0x0dd align=1 (i32.const 0)))
    (local.set 0x0de (i64.load offset=0x0de align=1 (i32.const 0)))
    (local.set 0x0df (i64.load offset=0x0df align=1 (i32.const 0)))
    (local.set 0x0e0 (i64.load offset=0x0e0 align=1 (i32.const 0)))
    (local.set 0x0e1 (i64.load offset=0x0e1 align=1 (i32.const 0)))
    (local.set 0x0e2 (i64.load offset=0x0e2 align=1 (i32.const 0)))
    (local.set 0x0e3 (i64.load offset=0x0e3 align=1 (i32.const 0)))
    (local.set 0x0e4 (i64.load offset=0x0e4 align=1 (i32.const 0)))
    (local.set 0x0e5 (i64.load offset=0x0e5 align=1 (i32.const 0)))
    (local.set 0x0e6 (i64.load offset=0x0e6 align=1 (i32.const 0)))
    (local.set 0x0e7 (i64.load offset=0x0e7 align=1 (i32.const 0)))
    (local.set 0x0e8 (i64.load offset=0x0e8 align=1 (i32.const 0)))
    (local.set 0x0e9 (i64.load offset=0x0e9 align=1 (i32.const 0)))
    (local.set 0x0ea (i64.load offset=0x0ea align=1 (i32.const 0)))
    (local.set 0x0eb (i64.load offset=0x0eb align=1 (i32.const 0)))
    (local.set 0x0ec (i64.load offset=0x0ec align=1 (i32.const 0)))
    (local.set 0x0ed (i64.load offset=0x0ed align=1 (i32.const 0)))
    (local.set 0x0ee (i64.load offset=0x0ee align=1 (i32.const 0)))
    (local.set 0x0ef (i64.load offset=0x0ef align=1 (i32.const 0)))
    (local.set 0x0f0 (i64.load offset=0x0f0 align=1 (i32.const 0)))
    (local.set 0x0f1 (i64.load offset=0x0f1 align=1 (i32.const 0)))
    (local.set 0x0f2 (i64.load offset=0x0f2 align=1 (i32.const 0)))
    (local.set 0x0f3 (i64.load offset=0x0f3 align=1 (i32.const 0)))
    (local.set 0x0f4 (i64.load offset=0x0f4 align=1 (i32.const 0)))
    (local.set 0x0f5 (i64.load offset=0x0f5 align=1 (i32.const 0)))
    (local.set 0x0f6 (i64.load offset=0x0f6 align=1 (i32.const 0)))
    (local.set 0x0f7 (i64.load offset=0x0f7 align=1 (i32.const 0)))
    (local.set 0x0f8 (i64.load offset=0x0f8 align=1 (i32.const 0)))
    (local.set 0x0f9 (i64.load offset=0x0f9 align=1 (i32.const 0)))
    (local.set 0x0fa (i64.load offset=0x0fa align=1 (i32.const 0)))
    (local.set 0x0fb (i64.load offset=0x0fb align=1 (i32.const 0)))
    (local.set 0x0fc (i64.load offset=0x0fc align=1 (i32.const 0)))
    (local.set 0x0fd (i64.load offset=0x0fd align=1 (i32.const 0)))
    (local.set 0x0fe (i64.load offset=0x0fe align=1 (i32.const 0)))
    (local.set 0x0ff (i64.load offset=0x0ff align=1 (i32.const 0)))
    (local.set 0x100 (i64.load offset=0x100 align=1 (i32.const 0)))
    (local.set 0x101 (i64.load offset=0x101 align=1 (i32.const 0)))
    (local.set 0x102 (i64.load offset=0x102 align=1 (i32.const 0)))
    (local.set 0x103 (i64.load offset=0x103 align=1 (i32.const 0)))
    (local.set 0x104 (i64.load offset=0x104 align=1 (i32.const 0)))
    (local.set 0x105 (i64.load offset=0x105 align=1 (i32.const 0)))
    (local.set 0x106 (i64.load offset=0x106 align=1 (i32.const 0)))
    (local.set 0x107 (i64.load offset=0x107 align=1 (i32.const 0)))
    (local.set 0x108 (i64.load offset=0x108 align=1 (i32.const 0)))
    (local.set 0x109 (i64.load offset=0x109 align=1 (i32.const 0)))
    (local.set 0x10a (i64.load offset=0x10a align=1 (i32.const 0)))
    (local.set 0x10b (i64.load offset=0x10b align=1 (i32.const 0)))
    (local.set 0x10c (i64.load offset=0x10c align=1 (i32.const 0)))
    (local.set 0x10d (i64.load offset=0x10d align=1 (i32.const 0)))
    (local.set 0x10e (i64.load offset=0x10e align=1 (i32.const 0)))
    (local.set 0x10f (i64.load offset=0x10f align=1 (i32.const 0)))
    (local.set 0x110 (i64.load offset=0x110 align=1 (i32.const 0)))
    (local.set 0x111 (i64.load offset=0x111 align=1 (i32.const 0)))
    (local.set 0x112 (i64.load offset=0x112 align=1 (i32.const 0)))
    (local.set 0x113 (i64.load offset=0x113 align=1 (i32.const 0)))
    (local.set 0x114 (i64.load offset=0x114 align=1 (i32.const 0)))
    (local.set 0x115 (i64.load offset=0x115 align=1 (i32.const 0)))
    (local.set 0x116 (i64.load offset=0x116 align=1 (i32.const 0)))
    (local.set 0x117 (i64.load offset=0x117 align=1 (i32.const 0)))
    (local.set 0x118 (i64.load offset=0x118 align=1 (i32.const 0)))
    (local.set 0x119 (i64.load offset=0x119 align=1 (i32.const 0)))
    (local.set 0x11a (i64.load offset=0x11a align=1 (i32.const 0)))
    (local.set 0x11b (i64.load offset=0x11b align=1 (i32.const 0)))
    (local.set 0x11c (i64.load offset=0x11c align=1 (i32.const 0)))
    (local.set 0x11d (i64.load offset=0x11d align=1 (i32.const 0)))
    (local.set 0x11e (i64.load offset=0x11e align=1 (i32.const 0)))
    (local.set 0x11f (i64.load offset=0x11f align=1 (i32.const 0)))
    (local.set 0x120 (i64.load offset=0x120 align=1 (i32.const 0)))
    (local.set 0x121 (i64.load offset=0x121 align=1 (i32.const 0)))
    (local.set 0x122 (i64.load offset=0x122 align=1 (i32.const 0)))
    (local.set 0x123 (i64.load offset=0x123 align=1 (i32.const 0)))
    (local.set 0x124 (i64.load offset=0x124 align=1 (i32.const 0)))
    (local.set 0x125 (i64.load offset=0x125 align=1 (i32.const 0)))
    (local.set 0x126 (i64.load offset=0x126 align=1 (i32.const 0)))
    (local.set 0x127 (i64.load offset=0x127 align=1 (i32.const 0)))
    (local.set 0x128 (i64.load offset=0x128 align=1 (i32.const 0)))
    (local.set 0x129 (i64.load offset=0x129 align=1 (i32.const 0)))
    (local.set 0x12a (i64.load offset=0x12a align=1 (i32.const 0)))
    (local.set 0x12b (i64.load offset=0x12b align=1 (i32.const 0)))
    (local.set 0x12c (i64.load offset=0x12c align=1 (i32.const 0)))
    (local.set 0x12d (i64.load offset=0x12d align=1 (i32.const 0)))
    (local.set 0x12e (i64.load offset=0x12e align=1 (i32.const 0)))
    (local.set 0x12f (i64.load offset=0x12f align=1 (i32.const 0)))
    (local.set 0x130 (i64.load offset=0x130 align=1 (i32.const 0)))
    (local.set 0x131 (i64.load offset=0x131 align=1 (i32.const 0)))
    (local.set 0x132 (i64.load offset=0x132 align=1 (i32.const 0)))
    (local.set 0x133 (i64.load offset=0x133 align=1 (i32.const 0)))
    (local.set 0x134 (i64.load offset=0x134 align=1 (i32.const 0)))
    (local.set 0x135 (i64.load offset=0x135 align=1 (i32.const 0)))
    (local.set 0x136 (i64.load offset=0x136 align=1 (i32.const 0)))
    (local.set 0x137 (i64.load offset=0x137 align=1 (i32.const 0)))
    (local.set 0x138 (i64.load offset=0x138 align=1 (i32.const 0)))
    (local.set 0x139 (i64.load offset=0x139 align=1 (i32.const 0)))
    (local.set 0x13a (i64.load offset=0x13a align=1 (i32.const 0)))
    (local.set 0x13b (i64.load offset=0x13b align=1 (i32.const 0)))
    (local.set 0x13c (i64.load offset=0x13c align=1 (i32.const 0)))
    (local.set 0x13d (i64.load offset=0x13d align=1 (i32.const 0)))
    (local.set 0x13e (i64.load offset=0x13e align=1 (i32.const 0)))
    (local.set 0x13f (i64.load offset=0x13f align=1 (i32.const 0)))
    (local.set 0x140 (i64.load offset=0x140 align=1 (i32.const 0)))
    (local.set 0x141 (i64.load offset=0x141 align=1 (i32.const 0)))
    (local.set 0x142 (i64.load offset=0x142 align=1 (i32.const 0)))
    (local.set 0x143 (i64.load offset=0x143 align=1 (i32.const 0)))
    (local.set 0x144 (i64.load offset=0x144 align=1 (i32.const 0)))
    (local.set 0x145 (i64.load offset=0x145 align=1 (i32.const 0)))
    (local.set 0x146 (i64.load offset=0x146 align=1 (i32.const 0)))
    (local.set 0x147 (i64.load offset=0x147 align=1 (i32.const 0)))
    (local.set 0x148 (i64.load offset=0x148 align=1 (i32.const 0)))
    (local.set 0x149 (i64.load offset=0x149 align=1 (i32.const 0)))
    (local.set 0x14a (i64.load offset=0x14a align=1 (i32.const 0)))
    (local.set 0x14b (i64.load offset=0x14b align=1 (i32.const 0)))
    (local.set 0x14c (i64.load offset=0x14c align=1 (i32.const 0)))
    (local.set 0x14d (i64.load offset=0x14d align=1 (i32.const 0)))
    (local.set 0x14e (i64.load offset=0x14e align=1 (i32.const 0)))
    (local.set 0x14f (i64.load offset=0x14f align=1 (i32.const 0)))
    (local.set 0x150 (i64.load offset=0x150 align=1 (i32.const 0)))
    (local.set 0x151 (i64.load offset=0x151 align=1 (i32.const 0)))
    (local.set 0x152 (i64.load offset=0x152 align=1 (i32.const 0)))
    (local.set 0x153 (i64.load offset=0x153 align=1 (i32.const 0)))
    (local.set 0x154 (i64.load offset=0x154 align=1 (i32.const 0)))
    (local.set 0x155 (i64.load offset=0x155 align=1 (i32.const 0)))
    (local.set 0x156 (i64.load offset=0x156 align=1 (i32.const 0)))
    (local.set 0x157 (i64.load offset=0x157 align=1 (i32.const 0)))
    (local.set 0x158 (i64.load offset=0x158 align=1 (i32.const 0)))
    (local.set 0x159 (i64.load offset=0x159 align=1 (i32.const 0)))
    (local.set 0x15a (i64.load offset=0x15a align=1 (i32.const 0)))
    (local.set 0x15b (i64.load offset=0x15b align=1 (i32.const 0)))
    (local.set 0x15c (i64.load offset=0x15c align=1 (i32.const 0)))
    (local.set 0x15d (i64.load offset=0x15d align=1 (i32.const 0)))
    (local.set 0x15e (i64.load offset=0x15e align=1 (i32.const 0)))
    (local.set 0x15f (i64.load offset=0x15f align=1 (i32.const 0)))
    (local.set 0x160 (i64.load offset=0x160 align=1 (i32.const 0)))
    (local.set 0x161 (i64.load offset=0x161 align=1 (i32.const 0)))
    (local.set 0x162 (i64.load offset=0x162 align=1 (i32.const 0)))
    (local.set 0x163 (i64.load offset=0x163 align=1 (i32.const 0)))
    (local.set 0x164 (i64.load offset=0x164 align=1 (i32.const 0)))
    (local.set 0x165 (i64.load offset=0x165 align=1 (i32.const 0)))
    (local.set 0x166 (i64.load offset=0x166 align=1 (i32.const 0)))
    (local.set 0x167 (i64.load offset=0x167 align=1 (i32.const 0)))
    (local.set 0x168 (i64.load offset=0x168 align=1 (i32.const 0)))
    (local.set 0x169 (i64.load offset=0x169 align=1 (i32.const 0)))
    (local.set 0x16a (i64.load offset=0x16a align=1 (i32.const 0)))
    (local.set 0x16b (i64.load offset=0x16b align=1 (i32.const 0)))
    (local.set 0x16c (i64.load offset=0x16c align=1 (i32.const 0)))
    (local.set 0x16d (i64.load offset=0x16d align=1 (i32.const 0)))
    (local.set 0x16e (i64.load offset=0x16e align=1 (i32.const 0)))
    (local.set 0x16f (i64.load offset=0x16f align=1 (i32.const 0)))
    (local.set 0x170 (i64.load offset=0x170 align=1 (i32.const 0)))
    (local.set 0x171 (i64.load offset=0x171 align=1 (i32.const 0)))
    (local.set 0x172 (i64.load offset=0x172 align=1 (i32.const 0)))
    (local.set 0x173 (i64.load offset=0x173 align=1 (i32.const 0)))
    (local.set 0x174 (i64.load offset=0x174 align=1 (i32.const 0)))
    (local.set 0x175 (i64.load offset=0x175 align=1 (i32.const 0)))
    (local.set 0x176 (i64.load offset=0x176 align=1 (i32.const 0)))
    (local.set 0x177 (i64.load offset=0x177 align=1 (i32.const 0)))
    (local.set 0x178 (i64.load offset=0x178 align=1 (i32.const 0)))
    (local.set 0x179 (i64.load offset=0x179 align=1 (i32.const 0)))
    (local.set 0x17a (i64.load offset=0x17a align=1 (i32.const 0)))
    (local.set 0x17b (i64.load offset=0x17b align=1 (i32.const 0)))
    (local.set 0x17c (i64.load offset=0x17c align=1 (i32.const 0)))
    (local.set 0x17d (i64.load offset=0x17d align=1 (i32.const 0)))
    (local.set 0x17e (i64.load offset=0x17e align=1 (i32.const 0)))
    (local.set 0x17f (i64.load offset=0x17f align=1 (i32.const 0)))
    (local.set 0x180 (i64.load offset=0x180 align=1 (i32.const 0)))
    (local.set 0x181 (i64.load offset=0x181 align=1 (i32.const 0)))
    (local.set 0x182 (i64.load offset=0x182 align=1 (i32.const 0)))
    (local.set 0x183 (i64.load offset=0x183 align=1 (i32.const 0)))
    (local.set 0x184 (i64.load offset=0x184 align=1 (i32.const 0)))
    (local.set 0x185 (i64.load offset=0x185 align=1 (i32.const 0)))
    (local.set 0x186 (i64.load offset=0x186 align=1 (i32.const 0)))
    (local.set 0x187 (i64.load offset=0x187 align=1 (i32.const 0)))
    (local.set 0x188 (i64.load offset=0x188 align=1 (i32.const 0)))
    (local.set 0x189 (i64.load offset=0x189 align=1 (i32.const 0)))
    (local.set 0x18a (i64.load offset=0x18a align=1 (i32.const 0)))
    (local.set 0x18b (i64.load offset=0x18b align=1 (i32.const 0)))
    (local.set 0x18c (i64.load offset=0x18c align=1 (i32.const 0)))
    (local.set 0x18d (i64.load offset=0x18d align=1 (i32.const 0)))
    (local.set 0x18e (i64.load offset=0x18e align=1 (i32.const 0)))
    (local.set 0x18f (i64.load offset=0x18f align=1 (i32.const 0)))
    (local.set 0x190 (i64.load offset=0x190 align=1 (i32.const 0)))
    (local.set 0x191 (i64.load offset=0x191 align=1 (i32.const 0)))
    (local.set 0x192 (i64.load offset=0x192 align=1 (i32.const 0)))
    (local.set 0x193 (i64.load offset=0x193 align=1 (i32.const 0)))
    (local.set 0x194 (i64.load offset=0x194 align=1 (i32.const 0)))
    (local.set 0x195 (i64.load offset=0x195 align=1 (i32.const 0)))
    (local.set 0x196 (i64.load offset=0x196 align=1 (i32.const 0)))
    (local.set 0x197 (i64.load offset=0x197 align=1 (i32.const 0)))
    (local.set 0x198 (i64.load offset=0x198 align=1 (i32.const 0)))
    (local.set 0x199 (i64.load offset=0x199 align=1 (i32.const 0)))
    (local.set 0x19a (i64.load offset=0x19a align=1 (i32.const 0)))
    (local.set 0x19b (i64.load offset=0x19b align=1 (i32.const 0)))
    (local.set 0x19c (i64.load offset=0x19c align=1 (i32.const 0)))
    (local.set 0x19d (i64.load offset=0x19d align=1 (i32.const 0)))
    (local.set 0x19e (i64.load offset=0x19e align=1 (i32.const 0)))
    (local.set 0x19f (i64.load offset=0x19f align=1 (i32.const 0)))
    (local.set 0x1a0 (i64.load offset=0x1a0 align=1 (i32.const 0)))
    (local.set 0x1a1 (i64.load offset=0x1a1 align=1 (i32.const 0)))
    (local.set 0x1a2 (i64.load offset=0x1a2 align=1 (i32.const 0)))
    (local.set 0x1a3 (i64.load offset=0x1a3 align=1 (i32.const 0)))
    (local.set 0x1a4 (i64.load offset=0x1a4 align=1 (i32.const 0)))
    (local.set 0x1a5 (i64.load offset=0x1a5 align=1 (i32.const 0)))
    (local.set 0x1a6 (i64.load offset=0x1a6 align=1 (i32.const 0)))
    (local.set 0x1a7 (i64.load offset=0x1a7 align=1 (i32.const 0)))
    (local.set 0x1a8 (i64.load offset=0x1a8 align=1 (i32.const 0)))
    (local.set 0x1a9 (i64.load offset=0x1a9 align=1 (i32.const 0)))
    (local.set 0x1aa (i64.load offset=0x1aa align=1 (i32.const 0)))
    (local.set 0x1ab (i64.load offset=0x1ab align=1 (i32.const 0)))
    (local.set 0x1ac (i64.load offset=0x1ac align=1 (i32.const 0)))
    (local.set 0x1ad (i64.load offset=0x1ad align=1 (i32.const 0)))
    (local.set 0x1ae (i64.load offset=0x1ae align=1 (i32.const 0)))
    (local.set 0x1af (i64.load offset=0x1af align=1 (i32.const 0)))
    (local.set 0x1b0 (i64.load offset=0x1b0 align=1 (i32.const 0)))
    (local.set 0x1b1 (i64.load offset=0x1b1 align=1 (i32.const 0)))
    (local.set 0x1b2 (i64.load offset=0x1b2 align=1 (i32.const 0)))
    (local.set 0x1b3 (i64.load offset=0x1b3 align=1 (i32.const 0)))
    (local.set 0x1b4 (i64.load offset=0x1b4 align=1 (i32.const 0)))
    (local.set 0x1b5 (i64.load offset=0x1b5 align=1 (i32.const 0)))
    (local.set 0x1b6 (i64.load offset=0x1b6 align=1 (i32.const 0)))
    (local.set 0x1b7 (i64.load offset=0x1b7 align=1 (i32.const 0)))
    (local.set 0x1b8 (i64.load offset=0x1b8 align=1 (i32.const 0)))
    (local.set 0x1b9 (i64.load offset=0x1b9 align=1 (i32.const 0)))
    (local.set 0x1ba (i64.load offset=0x1ba align=1 (i32.const 0)))
    (local.set 0x1bb (i64.load offset=0x1bb align=1 (i32.const 0)))
    (local.set 0x1bc (i64.load offset=0x1bc align=1 (i32.const 0)))
    (local.set 0x1bd (i64.load offset=0x1bd align=1 (i32.const 0)))
    (local.set 0x1be (i64.load offset=0x1be align=1 (i32.const 0)))
    (local.set 0x1bf (i64.load offset=0x1bf align=1 (i32.const 0)))
    (local.set 0x1c0 (i64.load offset=0x1c0 align=1 (i32.const 0)))
    (local.set 0x1c1 (i64.load offset=0x1c1 align=1 (i32.const 0)))
    (local.set 0x1c2 (i64.load offset=0x1c2 align=1 (i32.const 0)))
    (local.set 0x1c3 (i64.load offset=0x1c3 align=1 (i32.const 0)))
    (local.set 0x1c4 (i64.load offset=0x1c4 align=1 (i32.const 0)))
    (local.set 0x1c5 (i64.load offset=0x1c5 align=1 (i32.const 0)))
    (local.set 0x1c6 (i64.load offset=0x1c6 align=1 (i32.const 0)))
    (local.set 0x1c7 (i64.load offset=0x1c7 align=1 (i32.const 0)))
    (local.set 0x1c8 (i64.load offset=0x1c8 align=1 (i32.const 0)))
    (local.set 0x1c9 (i64.load offset=0x1c9 align=1 (i32.const 0)))
    (local.set 0x1ca (i64.load offset=0x1ca align=1 (i32.const 0)))
    (local.set 0x1cb (i64.load offset=0x1cb align=1 (i32.const 0)))
    (local.set 0x1cc (i64.load offset=0x1cc align=1 (i32.const 0)))
    (local.set 0x1cd (i64.load offset=0x1cd align=1 (i32.const 0)))
    (local.set 0x1ce (i64.load offset=0x1ce align=1 (i32.const 0)))
    (local.set 0x1cf (i64.load offset=0x1cf align=1 (i32.const 0)))
    (local.set 0x1d0 (i64.load offset=0x1d0 align=1 (i32.const 0)))
    (local.set 0x1d1 (i64.load offset=0x1d1 align=1 (i32.const 0)))
    (local.set 0x1d2 (i64.load offset=0x1d2 align=1 (i32.const 0)))
    (local.set 0x1d3 (i64.load offset=0x1d3 align=1 (i32.const 0)))
    (local.set 0x1d4 (i64.load offset=0x1d4 align=1 (i32.const 0)))
    (local.set 0x1d5 (i64.load offset=0x1d5 align=1 (i32.const 0)))
    (local.set 0x1d6 (i64.load offset=0x1d6 align=1 (i32.const 0)))
    (local.set 0x1d7 (i64.load offset=0x1d7 align=1 (i32.const 0)))
    (local.set 0x1d8 (i64.load offset=0x1d8 align=1 (i32.const 0)))
    (local.set 0x1d9 (i64.load offset=0x1d9 align=1 (i32.const 0)))
    (local.set 0x1da (i64.load offset=0x1da align=1 (i32.const 0)))
    (local.set 0x1db (i64.load offset=0x1db align=1 (i32.const 0)))
    (local.set 0x1dc (i64.load offset=0x1dc align=1 (i32.const 0)))
    (local.set 0x1dd (i64.load offset=0x1dd align=1 (i32.const 0)))
    (local.set 0x1de (i64.load offset=0x1de align=1 (i32.const 0)))
    (local.set 0x1df (i64.load offset=0x1df align=1 (i32.const 0)))
    (local.set 0x1e0 (i64.load offset=0x1e0 align=1 (i32.const 0)))
    (local.set 0x1e1 (i64.load offset=0x1e1 align=1 (i32.const 0)))
    (local.set 0x1e2 (i64.load offset=0x1e2 align=1 (i32.const 0)))
    (local.set 0x1e3 (i64.load offset=0x1e3 align=1 (i32.const 0)))
    (local.