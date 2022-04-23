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
    (local i64) (local i64) (local i64) (local i64) (local i64) (local i64) (local