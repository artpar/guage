; Test Console I/O Primitives (≋, ≋≈, ≋←)
; Day 24 - Phase 1: Console I/O

; ============ ≋ (print) Tests ============

; Test print with string
(⊨ :print-string #t
   (? (≈? (≋ "hello")) #t #f))

; Test print with number
(⊨ :print-number #t
   (? (ℕ? (≋ #42)) #t #f))

; Test print with boolean
(⊨ :print-bool #t
   (? (𝔹? (≋ #t)) #t #f))

; Test print with symbol
(⊨ :print-symbol #t
   (? (:? (≋ :test)) #t #f))

; Test print with nil
(⊨ :print-nil #t
   (? (∅? (≋ ∅)) #t #f))

; Test print returns value (identity)
(⊨ :print-returns-value "test"
   (≋ "test"))

(⊨ :print-returns-number #5
   (≋ #5))

; ============ ≋≈ (print-str) Tests ============

; Test print-str with string
(⊨ :print-str-basic "hello"
   (≋≈ "hello"))

; Test print-str empty string
(⊨ :print-str-empty ""
   (≋≈ ""))

; Test print-str returns value
(⊨ :print-str-returns "world"
   (≋≈ "world"))

; Test print-str error on non-string (error)
(⊨ :print-str-error-number #t
   (⚠? (≋≈ #42)))

; Test print-str error on non-string (symbol)
(⊨ :print-str-error-symbol #t
   (⚠? (≋≈ :test)))

; ============ Integration Tests ============

; Test chained printing
(⊨ :chain-print "done"
   (≋≈ (≋≈ (≋≈ "done"))))

; Test print and return
(⊨ :print-and-compute #84
   (⊗ (≋ #42) #2))

; Test string operations with print
(⊨ :print-concat "helloworld"
   (≋≈ (≈⊕ "hello" "world")))

; Test conditional printing
(⊨ :print-conditional "yes"
   (? #t (≋≈ "yes") (≋≈ "no")))

; ============ Summary ============
; Total: 18 tests
; - ≋ (print): 7 tests
; - ≋≈ (print-str): 5 tests
; - Integration: 6 tests
