;;;;;; Invalid UTF-8 custom section names

;;;; Continuation bytes not preceded by prefixes

;; encoding starts with (first) continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\00\02"                       ;; custom section
    "\01\80"                       ;; "\80"
  )
  "invalid UTF-8 encoding"
)

;; encoding starts with (0x8f) continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\00\02"                       ;; custom section
    "\01\8f"                       ;; "\8f"
  )
  "invalid UTF-8 encoding"
)

;; encoding starts with (0x90) continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\00\02"                       ;; custom section
    "\01\90"                       ;; "\90"
  )
  "invalid UTF-8 encoding"
)

;; encoding starts with (0x9f) continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\00\02"                       ;; custom section
    "\01\9f"                       ;; "\9f"
  )
  "invalid UTF-8 encoding"
)

;; encoding starts with (0xa0) continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\00\02"                       ;; custom section
    "\01\a0"                       ;; "\a0"
  )
  "invalid UTF-8 encoding"
)

;; encoding starts with (last) continuation byte
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\00\02"                       ;; custom section
    "\01\bf"                       ;; "\bf"
  )
  "invalid UTF-8 encoding"
)

;;;; 2-byte sequences

;; 2-byte sequence contains 3 bytes
(assert_malformed
  (module binary
    "\00asm" "\01\00\00\00"
    "\00\04"                       ;;