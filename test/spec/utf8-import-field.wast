;;;;;; Invalid UTF-8 import field names

;;;; Continuation bytes not preceded by prefixes

;; encoding starts with (first) continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0b"                       ;; import section
    "\01"                          ;; length 1
    "\01\80"                       ;; "\80"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; encoding starts with (0x8f) continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0b"                       ;; import section
    "\01"                          ;; length 1
    "\01\8f"                       ;; "\8f"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; encoding starts with (0x90) continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0b"                       ;; import section
    "\01"                          ;; length 1
    "\01\90"                       ;; "\90"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; encoding starts with (0x9f) continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0b"                       ;; import section
    "\01"                          ;; length 1
    "\01\9f"                       ;; "\9f"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; encoding starts with (0xa0) continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0b"                       ;; import section
    "\01"                          ;; length 1
    "\01\a0"                       ;; "\a0"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; encoding starts with (last) continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0b"                       ;; import section
    "\01"                          ;; length 1
    "\01\bf"                       ;; "\bf"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;;;; 2-byte sequences

;; 2-byte sequence contains 3 bytes
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0d"                       ;; import section
    "\01"                          ;; length 1
    "\03\c2\80\80"                 ;; "\c2\80\80"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; 2-byte sequence contains 1 byte at end of string
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0b"                       ;; import section
    "\01"        