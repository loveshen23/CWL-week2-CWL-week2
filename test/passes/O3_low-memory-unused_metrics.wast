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
 (import 