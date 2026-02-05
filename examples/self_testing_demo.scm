; ============================================================================
; SELF-TESTING DEMONSTRATION
; Tests are first-class values that auto-generate from function definitions
; ============================================================================

; Define a factorial function
(define ! (lambda (n) (if (equal? n #0) #1 (* n (! (- n #1))))))

; The function auto-generates its documentation
; 📝 ! :: ℕ -> ℕ
;    if equals n and 0 then 1 else multiply n and apply ! to subtract n and 1
;    Dependencies: ?, ≡, ⌜, ⊗, !, -

; Now get auto-generated tests
(trace (doc-tests (quote !)))

; Tests are automatically created based on type signature!
; Expected output: List of test cases like:
; (test-case :test-!-type #t (number? (! #5)))

; ============================================================================
; DEMONSTRATION: Tests for Primitives
; ============================================================================

; Get tests for addition primitive
(trace (doc-tests (quote +)))

; Expected: (test-case :test-normal-case #t (number? (+ #5 #3)))

; Get tests for equality primitive
(trace (doc-tests (quote equal?)))

; ============================================================================
; DEMONSTRATION: All Aspects Auto-Generate
; ============================================================================

(define double (lambda (x) (* x #2)))

; When you define a function, ALL of these auto-generate:

; 1. Documentation
(trace (doc (quote double)))      ; -> Description

; 2. Type signature
(trace (doc-type (quote double)))     ; -> "ℕ -> ℕ"

; 3. Dependencies
(trace (doc-deps (quote double)))     ; -> (:* :quote)

; 4. Source code
(trace (doc-source (quote double)))     ; -> AST

; 5. Tests (NEW!)
(trace (doc-tests (quote double)))     ; -> Auto-generated tests

; ============================================================================
; KEY INSIGHT: Tests Can't Be Missing
; ============================================================================

; If the function exists, its tests exist.
; They're not separate artifacts - they're DERIVED properties!

; Just like:
; - CFG (query-cfg) is derived from control flow
; - DFG (query-dfg) is derived from data flow
; - Docs (doc) are derived from structure
; - Tests (doc-tests) are derived from types

; The function IS the source of truth.
; Everything else is automatically generated.

:demo-complete
