; Enhanced Auto-Documentation Tests
; Tests for improved documentation formatting and presentation

; Load documentation formatter
(⋘ "stdlib/doc_format.scm")

; Define test functions
(≔ inc (λ (x) (⊕ x #1)))
(≔ double (λ (n) (⊗ n #2)))
(≔ square (λ (n) (⊗ n n)))

; Test 1: Format documentation for primitive
⟲ :test-1-primitive-doc
(≋ (≈⊙doc-format (⌜ ⊕)))

; Test 2: Format documentation for user function
⟲ :test-2-user-function-doc
(≋ (≈⊙doc-format (⌜ inc)))

; Test 3: Batch format multiple functions
⟲ :test-3-batch-doc
(≋ (≈⊙doc-list (⟨⟩ (⌜ inc) (⟨⟩ (⌜ double) (⟨⟩ (⌜ square) ∅)))))

; Test 4: Description extraction
⟲ :test-4-description
(⊨ :desc-is-symbol #t (:? (⌂ (⌜ inc))))

; Test 5: Type signature extraction
⟲ :test-5-type-sig
(⊨ :type-is-symbol #t (:? (⌂∈ (⌜ inc))))

; Test 6: Dependencies extraction
⟲ :test-6-dependencies
(⊨ :deps-is-list #t (⟨⟩? (⌂≔ (⌜ inc))))

; Test 7: Dependencies are symbols
⟲ :test-7-dep-symbols
(⊨ :first-dep-is-symbol #t (:? (◁ (⌂≔ (⌜ inc)))))

; Test 8: Multiple functions have different docs
⟲ :test-8-different-docs
(⊨ :different-descriptions #t (≢ (⌂ (⌜ inc)) (⌂ (⌜ double))))

; Test 9: Type signatures are correct
⟲ :test-9-type-correct
(⊨ :inc-type-correct #t (≡ (⌂∈ (⌜ inc)) :ℕ → ℕ))

; Test 10: Documentation is self-describing
⟲ :test-10-self-doc
(⊨ :doc-has-doc #t (:? (⌂ (⌜ ⌂))))

:all-tests-complete
