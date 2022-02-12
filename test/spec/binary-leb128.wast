;; Unsigned LEB128 can have non-minimal length
(module binary
  "\00asm" "\01\00\00\00"
  "\05\04\01"                          ;; Memory section with 1 entry
  "\00\82\00"                          ;; no max, minimum 2
)
(module binary
  "\00asm" "\01\00\00\00"
  "\05\07\01"                          ;; Memory section with 1 entry
  "\00\82\80\80\80\00"                 ;; no max, minimum 2
)
(module binary
  "\00asm" "\01\00\00\00"
  "\05\06\01"                          ;; Memory section with 1 entry
  "\01\82\00"                          ;; minimum 2
  "\82\00"                             ;; max 2
)
(module binary
  "\00asm" "\01\00\00\00"
  "\05\09\01"                          ;; Memory section with 1 entry
  "\01\82\00"                          ;; minimum 2
  "\82\80\80\80\00"                    ;; max 2
)
(module binary
  "\00asm" "\01\00\00\00"
  "\05\03\01"                          ;; Memory section with 1 entry
  "\00\00"                             ;; no max, minimum 0
  "\0b\07\01"                          ;; Data section with 1 entry
  "\80\00"                             ;; Memory index 0, encoded with 2 bytes
  "\41\00\0b\00"                       ;; (i32.const 0) with contents ""
)
(module binary
  "\00asm" "\01\00\00\00"
  "\04\04\01"                          ;; Table section with 1 entry
  "\70\00\00"                          ;; no max, minimum 0, funcref
  "\09\07\01"                          ;; Element section with 1 entry
  "\80\00"                             ;; Table index 0, encoded with 2 bytes
  "\41\00\0b\00"                       ;; (i32.const 0) with no elements
)
(module binary
  "\00asm" "\01\00\00\00"
  "\00"                                ;; custom section
  "\8a\00"                             ;; section size 10, encoded with 2 bytes
  "\01"                                ;; name byte count
  "1"                                  ;; name
  "23456789"                           ;; sequence of bytes
)
(module binary
  "\00asm" "\01\00\00\00"
  "\00"                                ;; custom section
  "\0b"                                ;; section size
  "\88\00"                             ;; name byte count 8, encoded with 2 bytes
  "12345678"                           ;; name
  "9"                                  ;; sequence of bytes
)
(module binary
  "\00asm" "\01\00\00\00"
  "\01\08\01"                          ;; type section
  "\60"                                ;; func type
  "\82\00"                             ;; num params 2, encoded with 2 bytes
  "\7f\7e"                             ;; param type
  "\01"                                ;; num results
  "\7f"                                ;; result type
)
(module binary
  "\00asm" "\01\00\00\00"
  "\01\08\01"                          ;; type section
  "\60"                                ;; func type
  "\02"                                ;; num params
  "\7f\7e"                             ;; param type
  "\81\00"                             ;; num results 1, encoded with 2 bytes
  "\7f"                                ;; result type
)
(module binary
  "\00asm" "\01\00\00\00"
  "\01\05\01"                          ;; type section
  "\60\01\7f\00"                       ;; function type
  "\02\17\01"                          ;; import section
  "\88\00"                             ;; module name length 8, encoded with 2 bytes
  "\73\70\65\63\74\65\73\74"           ;; module name
  "\09"                                ;; entity name length
  "\70\72\69\6e\74\5f\69\33\32"        ;; entity name
  "\00"                                ;; import kind
  "\00"                                ;; import signature index
)
(module binary
  "\00asm" "\01\00\00\00"
  "\01\05\01"                          ;; type section
  "\60\01\7f\00"                       ;; function type
  "\02\17\01"                          ;; import section
  "\08"                                ;; module name length
  "\73\70\65\63\74\65\73\74"           ;; module name
  "\89\00"                             ;; entity name length 9, encoded with 2 bytes
  "\70\72\69\6e\74\5f\69\33\32"        ;; entity name
  "\00"                                ;; import kind
  "\00"                                ;; import signature index
)
(module binary
  "\00asm" "\01\00\00\00"
  "\01\05\01"                          ;; type section
  "\60\01\7f\00"                       ;; function type
  "\02\17\01"                          ;; import section
  "\08"                                ;; module name length
  "\73\70\65\63\74\65\73\74"           ;; module name
  "\09"                                ;; entity name length 9
  "\70\72\69\6e\74\5f\69\33\32"        ;; entity name
  "\00"                                ;; import kind
  "\80\00"                             ;; import signature index, encoded with 2 bytes
)
(module binary
  "\00asm" "\01\00\00\00"
  "\01\04\01"                          ;; type section
  "\60\00\00"                          ;; function type
  "\03\03\01"                          ;; function section
  "\80\00"                             ;; function 0 signature index, encoded with 2 bytes
  "\0a\04\01"                          ;; code section
  "\02\00\0b"                          ;; function body
)
(module binary
  "\00asm" "\01\00\00\00"
  "\01\04\01"                          ;; type section
  "\60\00\00"                          ;; fun type
  "\03\02\01\00"                       ;; function section
  "\07\07\01"                          ;; export section
  "\82\00"                             ;; string length 2, encoded with 2 bytes
  "\66\31"                             ;; export name f1
  "\00"                                ;; export kind
  "\00"                                ;; export func index
  "\0a\04\01"                          ;; code section
  "\02\00\0b"                          ;; function body
)
(module binary
  "\00asm" "\01\00\00\00"
  "\01\04\01"                          ;; type section
  "\60\00\00"                          ;; fun type
  "\03\02\01\00"                       ;; function section
  "\07\07\01"                          ;; export section
  "\02"                                ;; string length 2
  "\66\31"                             ;; export name f1
  "\00"                                ;; export kind
  "\80\00"                             ;; export func index, encoded with 2 bytes
  "\0a\04\01"                          ;; code section
  "\02\00\0b"                          ;; function body
)
(module binary
  "\00asm" "\01\00\00\00"
  "\01\04\01"                          ;; type section
  "\60\00\00"                          ;; fun type
  "\03\02\01\00"                       ;; function section
  "\0a"                                ;; code section
  "\05"                                ;; section size
  "\81\00"                             ;; num functions, encoded with 2 bytes
  "\02\00\0b"                          ;; function body
)

;; Signed LEB128 can have non-minimal length
(module binary
  "\00asm" "\01\00\00\00"
  "\06\07\01"                          ;; Global section with 1 entry
  "\7f\00"                             ;; i32, immutable
  "\41\80\00"                          ;; i32.const 0
  "\0b"                                ;; end
)
(module binary
  "\00asm" "\01\00\00\00"
  "\06\07\01"                          ;; Global section with 1 entry
  "\7f\00"                             ;; i32, immutable
  "\41\ff\7f"                          ;; i32.const -1
  "\0b"                                ;; end
)
(module binary
  "\00asm" "\01\00\00\00"
  "\06\0a\01"                          ;; Global section with 1 entry
  "\7f\00"                             ;; i32, immutable
  "\41\80\80\80\80\00"                 ;; i32.const 0
  "\0b"                                ;; end
)
(module binary
  "\00asm" "\01\00\00\00"
  "\06\0a\01"                          ;; Global section with 1 entry
  "\7f\00"                             ;; i32, immutable
  "\41\ff\ff\ff\ff\7f"                 ;; i32.const -1
  "\0b"                                ;; end
)

(module binary
  "\00asm" "\01\00\00\00"
  "\06\07\01"                          ;; Global section with 1 entry
  "\7e\00"                             ;; i64, immutable
  "\42\80\00"                          ;; i64.const 0 with unused bits set
  "\0b"                                ;; end
)
(module binary
  "\00asm" "\01\00\00\00"
  "\06\07\01"                          ;; Global section with 1 entry
  "\7e\00"                             ;; i64, immutable
  "\42\ff\7f"                          ;; i64.const -1 with unused bits unset
  "\0b"                                ;; end
)
(module binary
  "\00asm" "\01\00\00\00"
  "\06\0f\01"                          ;; Global section with 1 entry
  "\7e\00"                             ;; i64, immutable
  "\42\80\80\80\80\80\80\80\80\80\00"  ;; i64.const 0 with unused bits set
  "\0b"                                ;; end
)
(module binary
  "\00asm" "\01\00\00\00"
  "\06\0f\01"                          ;; Global section with 1 entry
  "\7e\00"                             ;; i64, immutable
  "\42\ff\ff\ff\ff\ff\ff\ff\ff\ff\7f"  ;; i64.const -1 with unused bits unset
  "\0b"                                ;; end
)

;; Unsigned LEB128 must not be overlong
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\05\08\01"                          ;; Memory section with 1 entry
    "\00\82\80\80\80\80\00"              ;; no max, minimum 2 with one byte too many
  )
  "integer representation too long"
)
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\05\0a\01"                          ;; Memory section with 1 entry
    "\01\82\00"                          ;; minimum 2
    "\82\80\80\80\80\00"                 ;; max 2 with one byte too many
  )
  "integer representation too long"
)
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\05\03\01"                          ;; Memory section with 1 entry
    "\00\00"                             ;; no max, minimum 0
    "\0b\0b\01"                          ;; Data section with 1 entry
    "\80\80\80\80\80\00"                 ;; Memory index 0 with one byte too many
    "\41\00\0b\00"                       ;; (i32.const 0) with contents ""
  )
  "integer representation too long"
)
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\04\04\01"                          ;; Table section with 1 entry
    "\70\00\00"                          ;; no max, minimum 0, funcref
    "\09\0b\01"                          ;; Element section with 1 entry
    "\80\80\80\80\80\00"                 ;; Table index 0 with one byte too many
    "\41\00\0b\00"                       ;; (i32.const 0) with no elements
  )
  "integer representation too long"
)
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\00"                                ;; custom section
    "\83\80\80\80\80\00"                 ;; section size 3 with one byte too many
    "\01"                                ;; name byte count
    "1"                                  ;; name
    "2"                                  ;; sequence of bytes
  )
  "integer representation too long"
)
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\00"                                ;; custom section
    "\0A"                                ;; section size
    "\83\80\80\80\80\00"                 ;; name byte count 3 with one byte too many
    "123"                                ;; name
    "4"                                  ;; sequence of bytes
  )
  "integer representation too long"
)
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\01\0c\01"                          ;; type section
    "\60"                                ;; func type
    "\82\80\80\80\80\00"                 ;; num params 2 with one byte too many
    "\7f\7e"              