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
    "\01"                          ;; length 1
    "\01\c2"                       ;; "\c2"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; 2-byte sequence contains 1 byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0c"                       ;; import section
    "\01"                          ;; length 1
    "\02\c2\2e"                    ;; "\c2."
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;;;; 2-byte sequence contents

;; overlong encoding after 0xc0 prefix
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0c"                       ;; import section
    "\01"                          ;; length 1
    "\02\c0\80"                    ;; "\c0\80"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; overlong encoding after 0xc0 prefix
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0c"                       ;; import section
    "\01"                          ;; length 1
    "\02\c0\bf"                    ;; "\c0\bf"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; overlong encoding after 0xc1 prefix
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0c"                       ;; import section
    "\01"                          ;; length 1
    "\02\c1\80"                    ;; "\c1\80"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; overlong encoding after 0xc1 prefix
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0c"                       ;; import section
    "\01"                          ;; length 1
    "\02\c1\bf"                    ;; "\c1\bf"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; byte after (first) 2-byte prefix not a contination byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0c"                       ;; import section
    "\01"                          ;; length 1
    "\02\c2\00"                    ;; "\c2\00"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; byte after (first) 2-byte prefix not a continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0c"                       ;; import section
    "\01"                          ;; length 1
    "\02\c2\7f"                    ;; "\c2\7f"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; byte after (first) 2-byte prefix not a continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0c"                       ;; import section
    "\01"                          ;; length 1
    "\02\c2\c0"                    ;; "\c2\c0"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; byte after (first) 2-byte prefix not a continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0c"                       ;; import section
    "\01"                          ;; length 1
    "\02\c2\fd"                    ;; "\c2\fd"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; byte after (last) 2-byte prefix not a continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0c"                       ;; import section
    "\01"                          ;; length 1
    "\02\df\00"                    ;; "\df\00"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; byte after (last) 2-byte prefix not a continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0c"                       ;; import section
    "\01"                          ;; length 1
    "\02\df\7f"                    ;; "\df\7f"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; byte after (last) 2-byte prefix not a continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0c"                       ;; import section
    "\01"                          ;; length 1
    "\02\df\c0"                    ;; "\df\c0"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; byte after (last) 2-byte prefix not a continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0c"                       ;; import section
    "\01"                          ;; length 1
    "\02\df\fd"                    ;; "\df\fd"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;;;; 3-byte sequences

;; 3-byte sequence contains 4 bytes
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0e"                       ;; import section
    "\01"                          ;; length 1
    "\04\e1\80\80\80"              ;; "\e1\80\80\80"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; 3-byte sequence contains 2 bytes at end of string
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0c"                       ;; import section
    "\01"                          ;; length 1
    "\02\e1\80"                    ;; "\e1\80"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; 3-byte sequence contains 2 bytes
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0d"                       ;; import section
    "\01"                          ;; length 1
    "\03\e1\80\2e"                 ;; "\e1\80."
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; 3-byte sequence contains 1 byte at end of string
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0b"                       ;; import section
    "\01"                          ;; length 1
    "\01\e1"                       ;; "\e1"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; 3-byte sequence contains 1 byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0c"                       ;; import section
    "\01"                          ;; length 1
    "\02\e1\2e"                    ;; "\e1."
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;;;; 3-byte sequence contents

;; first byte after (0xe0) 3-byte prefix not a continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0d"                       ;; import section
    "\01"                          ;; length 1
    "\03\e0\00\a0"                 ;; "\e0\00\a0"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; first byte after (0xe0) 3-byte prefix not a continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0d"                       ;; import section
    "\01"                          ;; length 1
    "\03\e0\7f\a0"                 ;; "\e0\7f\a0"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; overlong encoding after 0xe0 prefix
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0d"                       ;; import section
    "\01"                          ;; length 1
    "\03\e0\80\80"                 ;; "\e0\80\80"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; overlong encoding after 0xe0 prefix
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0d"                       ;; import section
    "\01"                          ;; length 1
    "\03\e0\80\a0"                 ;; "\e0\80\a0"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; overlong encoding after 0xe0 prefix
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0d"                       ;; import section
    "\01"                          ;; length 1
    "\03\e0\9f\a0"                 ;; "\e0\9f\a0"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; overlong encoding after 0xe0 prefix
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0d"                       ;; import section
    "\01"                          ;; length 1
    "\03\e0\9f\bf"                 ;; "\e0\9f\bf"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; first byte after (0xe0) 3-byte prefix not a continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0d"                       ;; import section
    "\01"                          ;; length 1
    "\03\e0\c0\a0"                 ;; "\e0\c0\a0"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; first byte after (0xe0) 3-byte prefix not a continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0d"                       ;; import section
    "\01"                          ;; length 1
    "\03\e0\fd\a0"                 ;; "\e0\fd\a0"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; first byte after (first normal) 3-byte prefix not a continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0d"                       ;; import section
    "\01"                          ;; length 1
    "\03\e1\00\80"                 ;; "\e1\00\80"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; first byte after (first normal) 3-byte prefix not a continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0d"                       ;; import section
    "\01"                          ;; length 1
    "\03\e1\7f\80"                 ;; "\e1\7f\80"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; first byte after (first normal) 3-byte prefix not a continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0d"                       ;; import section
    "\01"                          ;; length 1
    "\03\e1\c0\80"                 ;; "\e1\c0\80"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; first byte after (first normal) 3-byte prefix not a continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0d"                       ;; import section
    "\01"                          ;; length 1
    "\03\e1\fd\80"                 ;; "\e1\fd\80"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; first byte after (normal) 3-byte prefix not a continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0d"                       ;; import section
    "\01"                          ;; length 1
    "\03\ec\00\80"                 ;; "\ec\00\80"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; first byte after (normal) 3-byte prefix not a continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0d"                       ;; import section
    "\01"                          ;; length 1
    "\03\ec\7f\80"                 ;; "\ec\7f\80"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; first byte after (normal) 3-byte prefix not a continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0d"                       ;; import section
    "\01"                          ;; length 1
    "\03\ec\c0\80"                 ;; "\ec\c0\80"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; first byte after (normal) 3-byte prefix not a continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0d"                       ;; import section
    "\01"                          ;; length 1
    "\03\ec\fd\80"                 ;; "\ec\fd\80"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; first byte after (0xed) 3-byte prefix not a continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0d"                       ;; import section
    "\01"                          ;; length 1
    "\03\ed\00\80"                 ;; "\ed\00\80"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; first byte after (0xed) 3-byte prefix not a continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0d"                       ;; import section
    "\01"                          ;; length 1
    "\03\ed\7f\80"                 ;; "\ed\7f\80"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; byte sequence reserved for UTF-16 surrogate half
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0d"                       ;; import section
    "\01"                          ;; length 1
    "\03\ed\a0\80"                 ;; "\ed\a0\80"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; byte sequence reserved for UTF-16 surrogate half
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0d"                       ;; import section
    "\01"                          ;; length 1
    "\03\ed\a0\bf"                 ;; "\ed\a0\bf"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; byte sequence reserved for UTF-16 surrogate half
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0d"                       ;; import section
    "\01"                          ;; length 1
    "\03\ed\bf\80"                 ;; "\ed\bf\80"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; byte sequence reserved for UTF-16 surrogate half
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0d"                       ;; import section
    "\01"                          ;; length 1
    "\03\ed\bf\bf"                 ;; "\ed\bf\bf"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; first byte after (0xed) 3-byte prefix not a continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0d"                       ;; import section
    "\01"                          ;; length 1
    "\03\ed\c0\80"                 ;; "\ed\c0\80"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; first byte after (0xed) 3-byte prefix not a continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0d"                       ;; import section
    "\01"                          ;; length 1
    "\03\ed\fd\80"                 ;; "\ed\fd\80"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; first byte after (normal) 3-byte prefix not a continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0d"                       ;; import section
    "\01"                          ;; length 1
    "\03\ee\00\80"                 ;; "\ee\00\80"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; first byte after (normal) 3-byte prefix not a continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0d"                       ;; import section
    "\01"                          ;; length 1
    "\03\ee\7f\80"                 ;; "\ee\7f\80"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; first byte after (normal) 3-byte prefix not a continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0d"                       ;; import section
    "\01"                          ;; length 1
    "\03\ee\c0\80"                 ;; "\ee\c0\80"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; first byte after (normal) 3-byte prefix not a continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0d"                       ;; import section
    "\01"                          ;; length 1
    "\03\ee\fd\80"                 ;; "\ee\fd\80"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; first byte after (last normal) 3-byte prefix not a continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0d"                       ;; import section
    "\01"                          ;; length 1
    "\03\ef\00\80"                 ;; "\ef\00\80"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; first byte after (last normal) 3-byte prefix not a continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0d"                       ;; import section
    "\01"                          ;; length 1
    "\03\ef\7f\80"                 ;; "\ef\7f\80"
    "\04\74\65\73\74"              ;; "test"
    "\03"                          ;; GlobalImport
    "\7f"                          ;; i32
    "\00"                          ;; immutable
  )
  "invalid UTF-8 encoding"
)

;; first byte after (last normal) 3-byte prefix not a continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\02\0d"                       ;; im