; Test Runner - Execute Auto-Generated Tests
; Collects tests from all primitives and user functions, runs them, reports results

; ============ Helper Functions ============

; Flatten nested list into single list
(≔ flatten (λ (lst)
  (? (∅? lst)
     ∅
     (? (⟨⟩? (◁ lst))
        (append (flatten (◁ lst)) (flatten (▷ lst)))
        (⟨⟩ (◁ lst) (flatten (▷ lst)))))))

; Append two lists
(≔ append (λ (l1 l2)
  (? (∅? l1)
     l2
     (⟨⟩ (◁ l1) (append (▷ l1) l2)))))

; Count elements in list
(≔ length (λ (lst)
  (? (∅? lst)
     #0
     (⊕ #1 (length (▷ lst))))))

; Execute a single test (test is: (⊨ :name expected actual))
; Returns: (pass :name) or (fail :name expected actual)
(≔ execute-test (λ (test)
  (? (∅? test)
     ⟨:error :empty-test⟩
     ; Extract test components
     (≔ name (◁ (▷ test)))               ; Get :name
     ; For now, just return the test structure
     ; (actual execution would evaluate 'actual' and compare)
     ⟨:pending name⟩))))

; Count test results by status
(≔ count-status (λ (results status)
  (? (∅? results)
     #0
     (? (≡ (◁ (◁ results)) status)
        (⊕ #1 (count-status (▷ results) status))
        (count-status (▷ results) status)))))

; ============ Primitive Test Collection ============

; Core arithmetic primitives
(≔ arithmetic-tests (λ ()
  (append (⌂⊨ (⌜ ⊕))
  (append (⌂⊨ (⌜ ⊖))
  (append (⌂⊨ (⌜ ⊗))
  (append (⌂⊨ (⌜ ⊘))
  (append (⌂⊨ (⌜ %))
  ∅)))))))

; Comparison primitives
(≔ comparison-tests (λ ()
  (append (⌂⊨ (⌜ ≡))
  (append (⌂⊨ (⌜ ≢))
  (append (⌂⊨ (⌜ <))
  (append (⌂⊨ (⌜ >))
  (append (⌂⊨ (⌜ ≤))
  (append (⌂⊨ (⌜ ≥))
  ∅))))))))

; Logic primitives
(≔ logic-tests (λ ()
  (append (⌂⊨ (⌜ ∧))
  (append (⌂⊨ (⌜ ∨))
  (append (⌂⊨ (⌜ ¬))
  ∅)))))

; Type predicates
(≔ type-predicate-tests (λ ()
  (append (⌂⊨ (⌜ ℕ?))
  (append (⌂⊨ (⌜ 𝔹?))
  (append (⌂⊨ (⌜ :?))
  (append (⌂⊨ (⌜ ∅?))
  (append (⌂⊨ (⌜ ⟨⟩?))
  (append (⌂⊨ (⌜ #?))
  ∅))))))))

; ============ Main Test Runner ============

; Collect all primitive tests
(≔ all-primitive-tests (λ ()
  (append (arithmetic-tests)
  (append (comparison-tests)
  (append (logic-tests)
  (append (type-predicate-tests)
  ∅))))))

; Run all tests and report
(≔ run-all-tests (λ ()
  (≔ tests (all-primitive-tests))
  (≔ total (length tests))
  ; For now, just report how many tests we collected
  ⟨:total total :tests tests⟩))

; ============ Execute Test Run ============

; Run the test suite
⟲ :starting-test-run
(≔ results (run-all-tests))
⟲ :test-run-complete
results
