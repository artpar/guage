; Enhanced Auto-Test Generation Tests
; Tests for property-based test generation

; Load test generator library
(⋘ "stdlib/testgen.scm")

⟲ :starting-enhanced-testgen-tests

; Test 1: Identity property for addition
⟲ :test-1-identity
(≔ test1 (((≈⊙testgen-identity (⌜ ⊕)) #0) #42))
(⊨ :identity-test-generated #t (⟨⟩? test1))

; Test 2: Commutative property for addition
⟲ :test-2-commutative
(≔ test2 (((≈⊙testgen-commutative (⌜ ⊕)) #3) #7))
(⊨ :commutative-test-generated #t (⟨⟩? test2))

; Test 3: Associative property for addition
⟲ :test-3-associative
(≔ test3 ((((≈⊙testgen-associative (⌜ ⊕)) #1) #2) #3))
(⊨ :associative-test-generated #t (⟨⟩? test3))

; Test 4: Generate boundary values for numbers
⟲ :test-4-boundary-values
(≔ boundaries (≈⊙testgen-boundary-ℕ))
(⊨ :has-zero #t (⟨⟩? boundaries))
(⊨ :first-is-zero #t (≡ (◁ boundaries) #0))

; Test 5: Generate example values
⟲ :test-5-example-values
(≔ examples (≈⊙testgen-examples-ℕ))
(⊨ :has-examples #t (⟨⟩? examples))

; Test 6: Idempotent property
⟲ :test-6-idempotent
(≔ test6 ((≈⊙testgen-idempotent (⌜ ¬)) #t))
(⊨ :idempotent-test-generated #t (⟨⟩? test6))

; Test 7: Test value generation
⟲ :test-7-test-values
(⊨ :gen-number #t (≡ (≈⊙testgen-value :ℕ) #42))
(⊨ :gen-boolean #t (≡ (≈⊙testgen-value :𝔹) #t))
(⊨ :gen-symbol #t (≡ (≈⊙testgen-value :symbol) :test))

; Test 8: Arithmetic op test suite
⟲ :test-8-arithmetic-suite
(≔ suite (≈⊙testgen-arithmetic-op (⌜ ⊕)))
(⊨ :suite-generated #t (⟨⟩? suite))

; Test 9: Predicate test suite
⟲ :test-9-predicate-suite
(≔ pred-suite (≈⊙testgen-predicate (⌜ ℕ?)))
(⊨ :predicate-suite-generated #t (⟨⟩? pred-suite))

; Test 10: Multiple property tests
⟲ :test-10-multiple-properties
(≔ add-tests (⟨⟩ (((≈⊙testgen-identity (⌜ ⊕)) #0) #10)
              (⟨⟩ (((≈⊙testgen-commutative (⌜ ⊕)) #5) #8)
              ∅)))
(⊨ :multiple-tests-generated #t (⟨⟩? add-tests))
(⊨ :has-two-tests #t (⟨⟩? (▷ add-tests)))

:all-enhanced-testgen-tests-complete
