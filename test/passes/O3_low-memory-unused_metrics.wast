;; zlib key deflate() function, with deflate_rle/deflate_huff inlining disabled
(module
 (type $0 (func (param i32 i32 i32) (result i32)))
 (type $1 (func (param i32 i32)))
 (type $2 (func (param i32 i32) (result i32)))
 (type $3 (func (param i32 i32 i32 i32)))
 (type $4 (func (param i32) (result i32)))
 (type $5 (func))
 (type $6 (func (param i32 i32 i32)))
 (type $7 (func (result i32)))
 (type $8 (func (param i32 i32 i32 i32) (result i32)))
 (type $9 (func (param i32 i32 i32 i32 i32 i32 i32 i32) (result i32)))
 (type $10 (func (param i32)))
 (type $11 (func (param i32 i32 i32 i32 i32 i32) (result i32)))
 (type $12 (func (param i64 i64) (result i32)))
 (type $13 (func (param i32 i64 i64 i32)))
 (type $14 (func (param i32 i32 i32 i32 i32) (result i32)))
 (type $15 (func (param i64 i32 i32) (result i32)))
 (type $16 (func (param i64 i32) (result i32)))
 (type $17 (func (param i32 i32 i32 i32 i32)))
 (type $18 (func (param i32 i64 i64 i32 i32 i32 i32) (result i32)))
 (type $19 (func (param i32 i64 i64 i64 i64)))
 (type $20 (func (param i64 i64 i64 i64) (result i32)))
 (type $21 (func (param i32 f64)))
 (type $22 (func (param i32 i32 i32) (result i32)))
 (type $23 (func (param i32 i32)))
 (type $24 (func (param i32 i32) (result i32)))
 (type $25 (func (param i32) (result i32)))
 (type $26 (func (result i32)))
 (type $27 (func (param i32)))
 (type $28 (func (param i32 i32 i32 i32) (result i32)))
 (type $29 (func (param i32 i32 i32)))
 (import "env" "memory" (memory $108 4096 4096))
 (import "env" "table" (table $timport$109 10 funcref))
 (import "env" "__assert_fail" (func $fimport$0 (param i32 i32 i32 i32)))
 (import "env" "__syscall54" (func $fimport$1 (param i32 i32) (result i32)))
 (import "env" "__syscall6" (func $fimport$2 (param i32 i32) (result i32)))
 (import "env" "__syscall140" (func $fimport$3 (param i32 i32) (result i32)))
 (import "env" "__syscall146" (func $fimport$4 (param i32 i32) (result i32)))
 (import "env" "sbrk" (func $fimport$5 (param i32) (result i32)))
 (import "env" "emscripten_memcpy_big" (func $fimport$6 (param i32 i32 i32) (result i32)))
 (import "env" "__wasm_call_ctors" (func $fimport$7))
 (import "env" "doit" (func $fimport$8 (param i32 i32 i32)))
 (import "env" "benchmark_main" (func $fimport$9 (param i32 i32) (result i32)))
 (import "env" "__original_main" (func $fimport$10 (result i32)))
 (import "env" "main" (func $fimport$11 (param i32 i32) (result i32)))
 (import "env" "compress" (func $fimport$12 (param i32 i32 i32 i32) (result i32)))
 (import "env" "compressBound" (func $fimport$13 (param i32) (result i32)))
 (import "env" "crc32" (func $fimport$14 (param i32 i32 i32) (result i32)))
 (import "env" "adler32" (func $fimport$15 (param i32 i32 i32) (result i32)))
 (import "env" "deflateInit_" (func $fimport$16 (param i32 i32 i32 i32) (result i32)))
 (import "env" "deflateInit2_" (func $fimport$17 (param i32 i32 i32 i32 i32 i32 i32 i32) (result i32)))
 (import "env" "deflateEnd" (func $fimport$18 (param i32) (result i32)))
 (import "env" "deflateReset" (func $fimport$19 (param i32) (result i32)))
 (import "env" "deflate_huff" (func $fimport$20 (param i32 i32) (result i32)))
 (import "env" "deflate_rle" (func $fimport$21 (param i32 i32) (result i32)))
 (import "env" "fill_window" (func $fimport$22 (param i32)))
 (import "env" "deflate_stored" (func $fimport$23 (param i32 i32) (result i32)))
 (import "env" "deflate_fast" (func $fimport$24 (param i32 i32) (result i32)))
 (import "env" "longest_match" (func $fimport$25 (param i32 i32) (result i32)))
 (import "env" "deflate_slow" (func $fimport$26 (param i32 i32) (result i32)))
 (import "env" "_tr_init" (func $fimport$27 (param i32)))
 (import "env" "init_block" (func $fimport$28 (param i32)))
 (import "env" "_tr_stored_block" (func $fimport$29 (param i32 i32 i32 i32)))
 (import "env" "_tr_align" (func $fimport$30 (param i32)))
 (import "env" "_tr_flush_block" (func $fimport$31 (param i32 i32 i32 i32)))
 (import "env" "build_tree" (func $fimport$32 (param i32 i32)))
 (import "env" "compress_block" (func $fimport$33 (param i32 i32 i32)))
 (import "env" "send_tree" (func $fimport$34 (param i32 i32 i32)))
 (import "env" "inflate_table" (func $fimport$35 (param i32 i32 i32 i32 i32 i32) (result i32)))
 (import "env" "inflate_fast" (func $fimport$36 (param i32 i32)))
 (import "env" "inflateInit2_" (func $fimport$37 (param i32 i32 i32 i32) (result i32)))
 (import "env" "inflateInit_" (func $fimport$38 (param i32 i32 i32) (result i32)))
 (import "env" "inflate" (func $fimport$39 (param i32 i32) (result i32)))
 (import "env" "updatewindow" (func $fimport$40 (param i32 i32) (result i32)))
 (import "env" "inflateEnd" (func $fimport$41 (param i32) (result i32)))
 (import "env" "uncompress" (func $fimport$42 (param i32 i32 i32 i32) (result i32)))
 (import "env" "zcalloc" (func $fimport$43 (param i32 i32 i32) (result i32)))
 (import "env" "zcfree" (func $fimport$44 (param i32 i32)))
 (import "env" "strcmp" (func $fimport$45 (param i32 i32) (result i32)))
 (import "env" "puts" (func $fimport$46 (param i32) (result i32)))
 (import "env" "strlen" (func $fimport$47 (param i32) (result i32)))
 (import "env" "fputs" (func $fimport$48 (param i32 i32) (result i32)))
 (import "env" "__towrite" (func $fimport$49 (param i32) (result i32)))
 (import "env" "__fwritex" (func $fimport$50 (param i32 i32 i32) (result i32)))
 (import "env" "fwrite" (func $fimport$51 (param i32 i32 i32 i32) (result i32)))
 (import "env" "__lockfile" (func $fimport$52 (param i32) (result i32)))
 (import "env" "__unlockfile" (func $fimport$53 (param i32)))
 (import "env" "__stdout_write" (func $fimport$54 (param i32 i32 i32) (result i32)))
 (import "env" "__errno_location" (func $fimport$55 (result i32)))
 (import "env" "__syscall_ret" (func $fimport$56 (param i32) (result i32)))
 (import "env" "dummy" (func $fimport$57 (param i32) (result i32)))
 (import "env" "__stdio_close" (func $fimport$58 (param i32) (result i32)))
 (import "env" "printf" (func $fimport$59 (param i32 i32) (result i32)))
 (import "env" "__overflow" (func $fimport$60 (param i32 i32) (result i32)))
 (import "env" "isdigit" (func $fimport$61 (param i32) (result i32)))
 (import "env" "memchr" (func $fimport$62 (param i32 i32 i32) (result i32)))
 (import "env" "pthread_self" (func $fimport$63 (result i32)))
 (import "env" "wcrtomb" (func $fimport$64 (param i32 i32 i32) (result i32)))
 (import "env" "__pthread_self" (func $fimport$65 (result i32)))
 (import "env" "wctomb" (func $fimport$66 (param i32 i32) (result i32)))
 (import "env" "__signbitl" (func $fimport$67 (param i64 i64) (result i32)))
 (import "env" "frexpl" (func $fimport$68 (param i32 i64 i64 i32)))
 (import "env" "vfprintf" (func $fimport$69 (param i32 i32 i32) (result i32)))
 (import "env" "printf_core" (func $fimport$70 (param i32 i32 i32 i32 i32) (result i32)))
 (import "env" "out" (func $fimport$71 (param i32 i32 i32)))
 (import "env" "getint" (func $fimport$72 (param i32) (result i32)))
 (import "env" "pop_arg" (func $fimport$73 (param i32 i32 i32)))
 (import "env" "fmt_x" (func $fimport$74 (param i64 i32 i32) (result i32)))
 (import "env" "fmt_o" (func $fimport$75 (param i64 i32) (result i32)))
 (import "env" "fmt_u" (func $fimport$76 (param i64 i32) (result i32)))
 (import "env" "pad" (func $fimport$77 (param i32 i32 i32 i32 i32)))
 (import "env" "fmt_fp" (func $fimport$78 (param i32 i64 i64 i32 i32 i32 i32) (result i32)))
 (import "env" "__stdio_seek" (func $fimport$79 (param i32 i32 i32) (result i32)))
 (import "env" "__stdio_write" (func $fimport$80 (param i32 i32 i32) (result i32)))
 (import "env" "malloc" (func $fimport$81 (param i32) (result i32)))
 (import "env" "free" (func $fimport$82 (param i32)))
 (import "env" "__addtf3" (func $fimport$83 (param i32 i64 i64 i64 i64)))
 (import "env" "__ashlti3" (func $fimport$84 (param i32 i64 i64 i32)))
 (import "env" "__unordtf2" (func $fimport$85 (param i64 i64 i64 i64) (result i32)))
 (import "env" "__eqtf2" (func $fimport$86 (param i64 i64 i64 i64) (result i32)))
 (import "env" "__netf2" (func $fimport$87 (param i64 i64 i64 i64) (result i32)))
 (import "env" "__extenddftf2" (func $fimport$88 (param i32 f64)))
 (import "env" "__fixtfsi" (func $fimport$89 (param i64 i64) (result i32)))
 (import "env" "__fixunstfsi" (func $fimport$90 (param i64 i64) (result i32)))
 (import "env" "__floatsitf" (func $fimport$91 (param i32 i32)))
 (import "env" "__floatunsitf" (func $fimport$92 (param i32 i32)))
 (import "env" "__lshrti3" (func $fimport$93 (param i32 i64 i64 i32)))
 (import "env" "__multf3" (func $fimport$94 (param i32 i64 i64 i64 i64)))
 (import "env" "__subtf3" (func $fimport$95 (param i32 i64 i64 i64 i64)))
 (import "env" "__fpclassifyl" (func $fimport$96 (param i64 i64) (result i32)))
 (import "env" "memset" (func $fimport$97 (param i32 i32 i32) (result i32)))
 (import "env" "memcpy" (func $fimport$98 (param i32 i32 i32) (result i32)))
 (import "env" "setThrew" (func $fimport$99 (param i32 i32)))
 (import "env" "stackSave" (func $fimport$100 (result i32)))
 (import "env" "stackAlloc" (func $fimport$101 (param i32) (result i32)))
 (import "env" "stackRestore" (func $fimport$102 (param i32)))
 (import "env" "__growWasmMemory" (func $fimport$103 (param i32) (result i32)))
 (import "env" "dynCall_iiii" (func $fimport$104 (param i32 i32 i32 i32) (result i32)))
 (import "env" "dynCall_vii" (func $fimport$105 (param i32 i32 i32)))
 (import "env" "dynCall_iii" (func $fimport$106 (param i32 i32 i32) (result i32)))
 (import "env" "dynCall_ii" (func $fimport$107 (param i32 i32) (result i32)))
 (global $global$0 (mut i32) (i32.const 5243904))
 (global $global$1 i32 (i32.const 5260880))
 (global $global$2 i32 (i32.const 17988))
 (export "deflate" (func $0))
 (func $0 (; 108 ;) (type $2) (param $0 i32) (param $1 i32) (result i32)
  (local $2 i32)
  (local $3 i32)
  (local $4 i32)
  (local $5 i32)
  (local $6 i32)
  (local $7 i32)
  (local $8 i32)
  (local $9 i32)
  (local $10 i32)
  (local.set $2
   (i32.const -2)
  )
  (block $label$1
   (br_if $label$1
    (i32.eqz
     (local.get $0)
    )
   )
   (br_if $label$1
    (i32.gt_u
     (local.get $1)
     (i32.const 5)
    )
   )
   (br_if $label$1
    (i32.eqz
     (local.tee $3
      (i32.load offset=28
       (local.get $0)
      )
     )
    )
   )
   (block $label$2
    (block $label$3
     (br_if $label$3
      (i32.eqz
       (i32.load offset=12
        (local.get $0)
       )
      )
     )
     (block $label$4
      (br_if $label$4
       (i32.load
        (local.get $0)
       )
      )
      (br_if $label$3
       (i32.load offset=4
        (local.get $0)
       )
      )
     )
     (local.set $2
      (i32.load offset=4
       (local.get $3)
      )
     )
     (br_if $label$2
      (i32.eq
       (local.get $1)
       (i32.const 4)
      )
     )
     (br_if $label$2
      (i32.ne
       (local.get $2)
       (i32.const 666)
      )
     )
    )
    (i32.store offset=24
     (local.get $0)
     (i32.load offset=16992
      (i32.const 0)
     )
    )
    (return
     (i32.const -2)
    )
   )
   (block $label$5
    (br_if $label$5
     (i32.load offset=16
      (local.get $0)
     )
    )
    (i32.store offset=24
     (local.get $0)
     (i32.load offset=17004
      (i32.const 0)
     )
    )
    (return
     (i32.const -5)
    )
   )
   (i32.store
    (local.get $3)
    (local.get $0)
   )
   (local.set $4
    (i32.load offset=40
     (local.get $3)
    )
   )
   (i32.store offset=40
    (local.get $3)
    (local.get $1)
   )
   (block $label$6
    (block $label$7
     (block $label$8
      (block $label$9
       (block $label$10
        (block $label$11
         (block $label$12
          (block $label$13
           (block $label$14
            (block $label$15
             (block $label$16
              (block $label$17
               (br_if $label$17
                (i32.ne
                 (local.get $2)
                 (i32.const 42)
                )
               )
               (block $label$18
                (br_if $label$18
                 (i32.ne
                  (i32.load offset=24
                   (local.get $3)
                  )
                  (i32.const 2)
                 )
                )
                (i32.store offset=48
                 (local.get $0)
                 (call $fimport$14
                  (i32.const 0)
                  (i32.const 0)
                  (i32.const 0)
                 )
                )
                (i32.store offset=20
                 (local.get $3)
                 (i32.add
                  (local.tee $2
                   (i32.load offset=20
                    (local.get $3)
                   )
                  )
                  (i32.const 1)
                 )
                )
                (i32.store8
                 (i32.add
                  (local.get $2)
                  (i32.load offset=8
                   (local.get $3)
                  )
                 )
                 (i32.const 31)
                )
                (i32.store offset=20
                 (local.get $3)
                 (i32.add
                  (local.tee $2
                   (i32.load offset=20
                    (local.get $3)
                   )
                  )
                  (i32.const 1)
                 )
                )
                (i32.store8
                 (i32.add
                  (local.get $2)
                  (i32.load offset=8
                   (local.get $3)
                  )
                 )
                 (i32.const 139)
                )
                (i32.store offset=20
                 (local.get $3)
                 (i32.add
                  (local.tee $2
                   (i32.load offset=20
                    (local.get $3)
                   )
                  )
                  (i32.const 1)
                 )
                )
                (i32.store8
                 (i32.add
                  (local.get $2)
                  (i32.load offset=8
                   (local.get $3)
                  )
                 )
                 (i32.const 8)
                )
                (block $label$19
                 (br_if $label$19
                  (local.tee $2
                   (i32.load offset=28
                    (local.get $3)
                   )
                  )
                 )
                 (i32.store offset=20
                  (local.get $3)
                  (i32.add
                   (local.tee $2
                    (i32.load offset=20
                     (local.get $3)
                    )
                   )
                   (i32.const 1)
                  )
                 )
                 (i32.store8
                  (i32.add
                   (local.get $2)
                   (i32.load offset=8
                    (local.get $3)
                   )
                  )
                  (i32.const 0)
                 )
                 (i32.store offset=20
                  (local.get $3)
                  (i32.add
                   (local.tee $2
                    (i32.load offset=20
                     (local.get $3)
                    )
                   )
                   (i32.const 1)
                  )
                 )
                 (i32.store8
                  (i32.add
                   (local.get $2)
                   (i32.load offset=8
                    (local.get $3)
                   )
                  )
                  (i32.const 0)
                 )
                 (i32.store offset=20
                  (local.get $3)
                  (i32.add
                   (local.tee $2
                    (i32.load offset=20
                     (local.get $3)
                    )
                   )
                   (i32.const 1)
                  )
                 )
                 (i32.store8
                  (i32.add
                   (local.get $2)
                   (i32.load offset=8
                    (local.get $3)
                   )
                  )
                  (i32.const 0)
                 )
                 (i32.store offset=20
                  (local.get $3)
                  (i32.add
                   (local.tee $2
                    (i32.load offset=20
                     (local.get $3)
                    )
                   )
                   (i32.const 1)
                  )
                 )
                 (i32.store8
                  (i32.add
                   (local.get $2)
                   (i32.load offset=8
                    (local.get $3)
                   )
                  )
                  (i32.const 0)
                 )
                 (i32.store offset=20
                  (local.get $3)
                  (i32.add
                   (local.tee $2
                    (i32.load offset=20
                     (local.get $3)
                    )
                   )
                   (i32.const 1)
                  )
                 )
                 (i32.store8
                  (i32.add
                   (local.get $2)
                   (i32.load offset=8
                    (local.get $3)
                   )
                  )
                  (i32.const 0)
                 )
                 (local.set $2
                  (i32.const 2)
                 )
                 (block $label$20
                  (br_if $label$20
                   (i32.eq
                    (local.tee $5
                     (i32.load offset=132
                      (local.get $3)
                     )
                    )
                    (i32.const 9)
                   )
                  )
                  (local.set $2
                   (select
                    (i32.const 4)
                    (i32.shl
                     (i32.gt_s
                      (i32.load offset=136
                       (local.get $3)
                      )
                      (i32.const 1)
                     )
                     (i32.const 2)
                    )
                    (i32.lt_s
                     (local.get $5)
                     (i32.const 2)
                    )
                   )
                  )
                 )
                 (i32.store offset=20
                  (local.get $3)
                  (i32.add
                   (local.tee $5
                    (i32.load offset=20
                     (local.get $3)
                    )
                   )
                   (i32.const 1)
                  )
                 )
                 (i32.store8
                  (i32.add
                   (local.get $5)
                   (i32.load offset=8
                    (local.get $3)
                   )
                  )
                  (local.get $2)
                 )
                 (i32.store offset=20
                  (local.get $3)
                  (i32.add
                   (local.tee $2
                    (i32.load offset=20
                     (local.get $3)
                    )
                   )
                   (i32.const 1)
                  )
                 )
                 (i32.store8
                  (i32.add
                   (local.get $2)
                   (i32.load offset=8
                    (local.get $3)
                   )
                  )
                  (i32.const 3)
                 )
                 (i32.store offset=4
                  (local.get $3)
                  (i32.const 113)
                 )
                 (br $label$6)
                )
                (local.set $5
                 (i32.load offset=36
                  (local.get $2)
                 )
                )
                (local.set $6
                 (i32.load offset=28
                  (local.get $2)
                 )
                )
                (local.set $7
                 (i32.load offset=16
                  (local.get $2)
                 )
                )
                (local.set $8
                 (i32.load offset=44
                  (local.get $2)
                 )
                )
                (local.set $9
                 (i32.load
                  (local.get $2)
                 )
                )
                (i32.store offset=20
                 (local.get $3)
                 (i32.add
                  (local.tee $10
                   (i32.load offset=20
                    (local.get $3)
                   )
                  )
                  (i32.const 1)
                 )
                )
                (local.set $2
                 (i32.const 2)
                )
                (i32.store8
                 (i32.add
                  (local.get $10)
                  (i32.load offset=8
                   (local.get $3)
                  )
                 )
                 (i32.or
                  (i32.or
                   (i32.or
                    (i32.or
                     (i32.shl
                      (i32.ne
                       (local.get $8)
                       (i32.const 0)
                      )
                      (i32.const 1)
                     )
                     (i32.ne
                      (local.get $9)
                      (i32.const 0)
                     )
                    )
                    (i32.shl
                     (i32.ne
                      (local.get $7)
                      (i32.const 0)
                     )
                     (i32.const 2)
                    )
                   )
                   (i32.shl
                    (i32.ne
                     (local.get $6)
                     (i32.const 0)
                    )
                    (i32.const 3)
                   )
                  )
                  (i32.shl
                   (i32.ne
                    (local.get $5)
                    (i32.const 0)
                   )
                   (i32.const 4)
                  )
                 )
                )
                (local.set $5
                 (i32.load offset=4
                  (i32.load offset=28
                   (local.get $3)
                  )
                 )
                )
                (i32.store offset=20
                 (local.get $3)
                 (i32.add
                  (local.tee $6
                   (i32.load offset=20
                    (local.get $3)
                   )
                  )
                  (i32.const 1)
                 )
                )
                (i32.store8
                 (i32.add
                  (local.get $6)
                  (i32.load offset=8
                   (local.get $3)
                  )
                 )
                 (local.get $5)
                )
                (local.set $5
                 (i32.load offset=4
                  (i32.load offset=28
                   (local.get $3)
                  )
                 )
                )
                (i32.store offset=20
                 (local.get $3)
                 (i32.add
                  (local.tee $6
                   (i32.load offset=20
                    (local.get $3)
                   )
                  )
                  (i32.const 1)
                 )
                )
                (i32.store8
                 (i32.add
                  (local.get $6)
                  (i32.load offset=8
                   (local.get $3)
                  )
                 )
                 (i32.shr_u
                  (local.get $5)
                  (i32.const 8)
                 )
                )
                (local.set $5
                 (i32.load16_u
                  (i32.add
                   (i32.load offset=28
                    (local.get $3)
                   )
                   (i32.const 6)
                  )
                 )
                )
                (i32.store offset=20
                 (local.get $3)
                 (i32.add
                  (local.tee $6
                   (i32.load offset=20
                    (local.get $3)
                   )
                  )
                  (i32.const 1)
                 )
                )
                (i32.store8
                 (i32.add
                  (local.get $6)
                  (i32.load offset=8
                   (local.get $3)
                  )
                 )
                 (local.get $5)
                )
                (local.set $5
                 (i32.load8_u
                  (i32.add
                   (i32.load offset=28
                    (local.get $3)
                   )
                   (i32.const 7)
                  )
                 )
                )
                (i32.store offset=20
                 (local.get $3)
                 (i32.add
                  (local.tee $6
                   (i32.load offset=20
                    (local.get $3)
                   )
                  )
                  (i32.const 1)
                 )
                )
                (i32.store8
                 (i32.add
                  (local.get $6)
                  (i32.load offset=8
                   (local.get $3)
                  )
                 )
                 (local.get $5)
                )
                (block $label$21
                 (br_if $label$21
                  (i32.eq
                   (local.tee $5
                    (i32.load offset=132
                     (local.get $3)
                    )
                   )
                   (i32.const 9)
                  )
                 )
                 (local.set $2
                  (select
                   (i32.const 4)
                   (i32.shl
                    (i32.gt_s
                     (i32.load offset=136
                      (local.get $3)
                     )
                     (i32.const 1)
                    )
                    (i32.const 2)
                   )
                   (i32.lt_s
                    (local.get $5)
                    (i32.const 2)
                   )
                  )
                 )
                )
                (i32.store offset=20
                 (local.get $3)
                 (i32.add
                  (local.tee $5
                   (i32.load offset=20
                    (local.get $3)
                   )
                  )
                  (i32.const 1)
                 )
                )
                (i32.store8
                 (i32.add
                  (local.get $5)
                  (i32.load offset=8
                   (local.get $3)
                  )
                 )
                 (local.get $2)
                )
                (local.set $2
                 (i32.load offset=12
                  (i32.load offset=28
                   (local.get $3)
                  )
                 )
                )
                (i32.store offset=20
                 (local.get $3)
                 (i32.add
                  (local.tee $5
                   (i32.load offset=20
                    (local.get $3)
                   )
                  )
                  (i32.const 1)
                 )
                )
                (i32.store8
                 (i32.add
                  (local.get $5)
                  (i32.load offset=8
                   (local.get $3)
                  )
                 )
                 (local.get $2)
                )
                (block $label$22
                 (br_if $label$22
                  (i32.eqz
                   (i32.load offset=16
                    (local.tee $2
                     (i32.load offset=28
                      (local.get $3)
                     )
                    )
                   )
                  )
                 )
                 (local.set $2
                  (i32.load offset=20
                   (local.get $2)
                  )
                 )
                 (i32.store offset=20
                  (local.get $3)
                  (i32.add
                   (local.tee $5
                    (i32.load offset=20
                     (local.get $3)
                    )
                   )
                   (i32.const 1)
                  )
                 )
                 (i32.store8
                  (i32.add
                   (local.get $5)
                   (i32.load offset=8
                    (local.get $3)
                   )
                  )
                  (local.get $2)
                 )
                 (local.set $2
                  (i32.load offset=20
                   (i32.load offset=28
                    (local.get $3)
                   )
                  )
                 )
                 (i32.store offset=20
                  (local.get $3)
                  (i32.add
                   (local.tee $5
                    (i32.load offset=20
                     (local.get $3)
                    )
                   )
                   (i32.const 1)
                  )
                 )
                 (i32.store8
                  (i32.add
                   (local.get $5)
                   (i32.load offset=8
                    (local.get $3)
                   )
                  )
                  (i32.shr_u
                   (local.get $2)
                   (i32.const 8)
                  )
                 )
                 (local.set $2
                  (i32.load offset=28
                   (local.get $3)
                  )
                 )
                )
                (block $label$23
                 (br_if $label$23
                  (i32.eqz
                   (i32.load offset=44
                    (local.get $2)
                   )
                  )
                 )
                 (i32.store offset=48
                  (local.get $0)
                  (call $fimport$14
                   (i32.load offset=48
                    (local.get $0)
                   )
                   (i32.load offset=8
                    (local.get $3)
                   )
                   (i32.load offset=20
                    (local.get $3)
                   )
                  )
      