;; Test that optimizers don't do redundant-load, store-to-load, or dead-store
;; optimizations when there are interfering stores, even of different types
;; and