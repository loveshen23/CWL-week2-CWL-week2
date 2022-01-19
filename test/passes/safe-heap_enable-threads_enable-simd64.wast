(module
 (memory (shared i64 100 100))
 (func $loads
  (drop (i32.load (i64.const 1)))
  (drop (i32.atomic.load (i64.co