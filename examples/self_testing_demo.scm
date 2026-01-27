; ============================================================================
; SELF-TESTING DEMONSTRATION
; Tests are first-class values that auto-generate from function definitions
; ============================================================================

; Define a factorial function
(≔ ! (λ (n) (? (≡ n #0) #1 (⊗ n (! (⊖ n #1))))))

; The function auto-generates its documentation
; 📝 ! :: ℕ → ℕ
;    if equals n and 0 then 1 else multiply n and apply ! to subtract n and 1
;    Dependencies: ?, ≡, ⌜, ⊗, !, ⊖

; Now get auto-generated tests
(⟲ (⌂⊨ (⌜ !)))

; Tests are automatically created based on type signature!
; Expected output: List of test cases like:
; (⊨ :test-!-type #t (ℕ? (! #5)))

; ============================================================================
; DEMONSTRATION: Tests for Primitives
; ============================================================================

; Get tests for addition primitive
(⟲ (⌂⊨ (⌜ ⊕)))

; Expected: (⊨ :test-normal-case #t (ℕ? (⊕ #5 #3)))

; Get tests for equality primitive
(⟲ (⌂⊨ (⌜ ≡)))

; ============================================================================
; DEMONSTRATION: All Aspects Auto-Generate
; ============================================================================

(≔ double (λ (x) (⊗ x #2)))

; When you define a function, ALL of these auto-generate:

; 1. Documentation
(⟲ (⌂ (⌜ double)))      ; → Description

; 2. Type signature
(⟲ (⌂∈ (⌜ double)))     ; → "ℕ → ℕ"

; 3. Dependencies
(⟲ (⌂≔ (⌜ double)))     ; → (:⊗ :⌜)

; 4. Source code
(⟲ (⌂⊛ (⌜ double)))     ; → AST

; 5. Tests (NEW!)
(⟲ (⌂⊨ (⌜ double)))     ; → Auto-generated tests

; ============================================================================
; KEY INSIGHT: Tests Can't Be Missing
; ============================================================================

; If the function exists, its tests exist.
; They're not separate artifacts - they're DERIVED properties!

; Just like:
; - CFG (⌂⟿) is derived from control flow
; - DFG (⌂⇝) is derived from data flow
; - Docs (⌂) are derived from structure
; - Tests (⌂⊨) are derived from types

; The function IS the source of truth.
; Everything else is automatically generated.

:demo-complete
