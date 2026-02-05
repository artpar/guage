; Enhanced Self-Testing Demo - Structure-Based Test Generation
; Shows how doc-tests now analyzes function structure to generate comprehensive tests

; ============ Example 1: Factorial (has recursion + conditional + zero comparison) ============
(define ! (lambda (n) (if (equal? n #0) #1 (* n (! (- n #1))))))

; Generate tests - should include:
; - Type conformance test
; - Branch test (conditional detected)
; - Base case test (recursion detected)
; - Recursive case test (recursion detected)
; - Zero edge test (zero comparison detected)
(trace (doc-tests (quote !)))

; ============ Example 2: Double (simple function - no special structure) ============
(define double (lambda (x) (* x #2)))

; Generate tests - should include:
; - Type conformance test only (no special structure)
(trace (doc-tests (quote double)))

; ============ Example 3: Fibonacci (recursion + conditional) ============
(define fib (lambda (n) (if (< n #2) n (+ (fib (- n #1)) (fib (- n #2))))))

; Generate tests - should include:
; - Type conformance test
; - Branch test (conditional detected)
; - Base case test (recursion detected)
; - Recursive case test (recursion detected)
(trace (doc-tests (quote fib)))

; ============ Example 4: Absolute value (conditional + zero comparison) ============
(define abs (lambda (x) (if (< x #0) (- #0 x) x)))

; Generate tests - should include:
; - Type conformance test
; - Branch test (conditional detected)
; - Zero edge test (zero comparison detected)
(trace (doc-tests (quote abs)))

; ============ Example 5: Test primitive with enhanced edge cases ============
; Arithmetic primitives now get zero edge case tests
(trace (doc-tests (quote +)))  ; Should have normal + zero edge cases

; ============ Verification ============
; Let's actually run some of the generated tests manually:

; Factorial tests:
(test-case :manual-test-!-base #t (equal? (! #0) #1))       ; Base case
(test-case :manual-test-!-recursive #t (equal? (! #5) #120)) ; Recursive case
(test-case :manual-test-!-type #t (number? (! #3)))         ; Type check

; Fibonacci tests:
(test-case :manual-test-fib-base #t (equal? (fib #0) #0))   ; Base case 0
(test-case :manual-test-fib-base2 #t (equal? (fib #1) #1))  ; Base case 1
(test-case :manual-test-fib-rec #t (equal? (fib #7) #13))   ; Recursive case

; Absolute value tests:
(test-case :manual-test-abs-pos #t (equal? (abs #5) #5))    ; Positive
(test-case :manual-test-abs-neg #t (equal? (abs #-5) #5))   ; Negative (Note: We don't have negative literals yet)
(test-case :manual-test-abs-zero #t (equal? (abs #0) #0))   ; Zero edge case

; Success!
:enhanced-self-testing-demo-complete
