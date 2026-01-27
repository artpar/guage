; ============================================================================
; Test Module: Math Utilities
; ============================================================================
;
; A simple test module that defines some math functions
; Used to test module symbol tracking
;

; Define some basic math functions

(≔ square (λ (x) (⊗ x x)))

(≔ cube (λ (x) (⊗ x (⊗ x x))))

(≔ add-one (λ (x) (⊕ x #1)))

(≔ double (λ (x) (⊗ x #2)))

; That's all for this test module
