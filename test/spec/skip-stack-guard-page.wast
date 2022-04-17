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
    (local i64) (local i64) (local i64) (local