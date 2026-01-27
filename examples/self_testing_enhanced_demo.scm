; Enhanced Self-Testing Demo - Structure-Based Test Generation
; Shows how ⌂⊨ now analyzes function structure to generate comprehensive tests

; ============ Example 1: Factorial (has recursion + conditional + zero comparison) ============
(≔ ! (λ (n) (? (≡ n #0) #1 (⊗ n (! (⊖ n #1))))))

; Generate tests - should include:
; - Type conformance test
; - Branch test (conditional detected)
; - Base case test (recursion detected)
; - Recursive case test (recursion detected)
; - Zero edge test (zero comparison detected)
(⟲ (⌂⊨ (⌜ !)))

; ============ Example 2: Double (simple function - no special structure) ============
(≔ double (λ (x) (⊗ x #2)))

; Generate tests - should include:
; - Type conformance test only (no special structure)
(⟲ (⌂⊨ (⌜ double)))

; ============ Example 3: Fibonacci (recursion + conditional) ============
(≔ fib (λ (n) (? (< n #2) n (⊕ (fib (⊖ n #1)) (fib (⊖ n #2))))))

; Generate tests - should include:
; - Type conformance test
; - Branch test (conditional detected)
; - Base case test (recursion detected)
; - Recursive case test (recursion detected)
(⟲ (⌂⊨ (⌜ fib)))

; ============ Example 4: Absolute value (conditional + zero comparison) ============
(≔ abs (λ (x) (? (< x #0) (⊖ #0 x) x)))

; Generate tests - should include:
; - Type conformance test
; - Branch test (conditional detected)
; - Zero edge test (zero comparison detected)
(⟲ (⌂⊨ (⌜ abs)))

; ============ Example 5: Test primitive with enhanced edge cases ============
; Arithmetic primitives now get zero edge case tests
(⟲ (⌂⊨ (⌜ ⊕)))  ; Should have normal + zero edge cases

; ============ Verification ============
; Let's actually run some of the generated tests manually:

; Factorial tests:
(⊨ :manual-test-!-base #t (≡ (! #0) #1))       ; Base case
(⊨ :manual-test-!-recursive #t (≡ (! #5) #120)) ; Recursive case
(⊨ :manual-test-!-type #t (ℕ? (! #3)))         ; Type check

; Fibonacci tests:
(⊨ :manual-test-fib-base #t (≡ (fib #0) #0))   ; Base case 0
(⊨ :manual-test-fib-base2 #t (≡ (fib #1) #1))  ; Base case 1
(⊨ :manual-test-fib-rec #t (≡ (fib #7) #13))   ; Recursive case

; Absolute value tests:
(⊨ :manual-test-abs-pos #t (≡ (abs #5) #5))    ; Positive
(⊨ :manual-test-abs-neg #t (≡ (abs #-5) #5))   ; Negative (Note: We don't have negative literals yet)
(⊨ :manual-test-abs-zero #t (≡ (abs #0) #0))   ; Zero edge case

; Success!
:enhanced-self-testing-demo-complete
