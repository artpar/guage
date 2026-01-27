;;;; Day 30: Module System Integration Tests
;;;; End-to-end validation of complete module system

;; Test 1: Real stdlib usage - list operations
;; Load and use actual standard library modules
(⋘ "../../stdlib/list.scm")
(≔ test-list (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅))))
(≔ doubled ((↦ (λ (x) (⊗ x #2))) test-list))
(⊢ (≡ (◁ doubled) #2)
   "Stdlib list map should work after loading")

;; Test 2: Real stdlib usage - option type
;; Verify Option type works as expected
(⋘ "../../stdlib/option.scm")
(≔ some-val (Some #42))
(≔ extracted (Option-unwrap some-val #0))
(⊢ (≡ extracted #42)
   "Stdlib Option type should work after loading")

;; Test 3: App with multiple modules
;; Simulate a multi-module application
(≋⊲ "../../tests/fixtures/integ_utils.scm" "(≔ triple (λ (n) (⊗ n #3)))")
(≋⊲ "../../tests/fixtures/integ_app.scm" "(⋘ \"../../tests/fixtures/integ_utils.scm\") (≔ main (λ (x) (triple x)))")
(⋘ "../../tests/fixtures/integ_app.scm")
(⊢ (≡ (main #7) #21)
   "Multi-module app should work correctly")

;; Test 4: Module dependency chain
;; Verify dependencies are tracked through chain
(≔ app-deps (⌂⊚→ "../../tests/fixtures/integ_app.scm"))
(≔ has-utils (λ (lst)
  (? (∅? lst)
     #f
     (? (≈≡ (◁ lst) "../../tests/fixtures/integ_utils.scm")
        #t
        (has-utils (▷ lst))))))
(⊢ (has-utils app-deps)
   "Module dependency chain should be tracked")

;; Test 5: Symbol provenance across modules
;; Verify we can trace where symbols come from
(≔ triple-source (⌂⊚ :triple))
(⊢ (≈≡ triple-source "../../tests/fixtures/integ_utils.scm")
   "Symbol provenance should work across modules")

;; Test 6: Selective import validation across modules
;; Verify selective import works with loaded modules
(≔ import-result (⋖ "../../tests/fixtures/integ_utils.scm" (⟨⟩ :triple ∅)))
(⊢ (≡ import-result :ok)
   "Selective import should validate symbols from loaded modules")

;; Test 7: Module redefinition (conflict)
;; When same symbol defined in multiple modules, last one wins
(≋⊲ "../../tests/fixtures/conflict_a.scm" "(≔ value (λ (x) (⊕ x #1)))")
(≋⊲ "../../tests/fixtures/conflict_b.scm" "(≔ value (λ (x) (⊕ x #2)))")
(⋘ "../../tests/fixtures/conflict_a.scm")
(≔ result-a (value #10))
(⋘ "../../tests/fixtures/conflict_b.scm")
(≔ result-b (value #10))
(⊢ (∧ (≡ result-a #11) (≡ result-b #12))
   "Module loading order should determine which definition wins")

;; Test 8: Empty module
;; Empty modules should load without error
(≋⊲ "../../tests/fixtures/empty_module.scm" "")
(≔ empty-result (⋘ "../../tests/fixtures/empty_module.scm"))
(⊢ (¬ (⚠? empty-result))
   "Empty module should load successfully")

;; Test 9: Module with only comments
;; Modules with only comments should load
(≋⊲ "../../tests/fixtures/comments_only.scm" "; This is a comment\n; Another comment")
(≔ comments-result (⋘ "../../tests/fixtures/comments_only.scm"))
(⊢ (¬ (⚠? comments-result))
   "Module with only comments should load successfully")

;; Test 10: Complex dependency graph
;; Create and verify complex multi-module system
(≋⊲ "../../tests/fixtures/math_base.scm" "(≔ add (λ (x y) (⊕ x y)))")
(≋⊲ "../../tests/fixtures/math_extended.scm" "(⋘ \"../../tests/fixtures/math_base.scm\") (≔ add3 (λ (x y z) (add (add x y) z)))")
(≋⊲ "../../tests/fixtures/calculator.scm" "(⋘ \"../../tests/fixtures/math_extended.scm\") (≔ sum-list (λ (lst) (? (∅? (▷ lst)) (◁ lst) (add (◁ lst) (sum-list (▷ lst))))))")

; Load full system
(⋘ "../../tests/fixtures/calculator.scm")

; Test functionality
(≔ calc-result (sum-list (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅)))))
(⊢ (≡ calc-result #6)
   "Complex multi-module system should work correctly")

; Verify dependency tracking
(≔ calc-deps (⌂⊚→ "../../tests/fixtures/calculator.scm"))
(≔ has-math-ext (λ (lst)
  (? (∅? lst)
     #f
     (? (≈≡ (◁ lst) "../../tests/fixtures/math_extended.scm")
        #t
        (has-math-ext (▷ lst))))))
(⊢ (has-math-ext calc-deps)
   "Complex system should track dependencies correctly")
