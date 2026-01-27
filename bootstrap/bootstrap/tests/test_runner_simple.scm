; Simplified Test Runner - Test execution logic only

; Helper functions
(≔ length (λ (lst)
  (? (∅? lst)
     #0
     (⊕ #1 (length (▷ lst))))))

; Execute a single test
(≔ execute-test (λ (test)
  (? (∅? test)
     ⟨:error :empty-test⟩
     (≔ test-sym (◁ test))
     (≔ name (◁ (▷ test)))
     (≔ expected (◁ (▷ (▷ test))))
     (≔ actual (◁ (▷ (▷ (▷ test)))))
     (? (≟ expected actual)
        ⟨:pass name⟩
        ⟨:fail name expected actual⟩))))

; Test with single primitive
⟲ :testing-single-primitive
(≔ add-tests (⌂⊨ (⌜ ⊕)))
⟲ :tests-generated
⟲ add-tests
(≔ result (execute-test (◁ add-tests)))
⟲ :test-result
result
