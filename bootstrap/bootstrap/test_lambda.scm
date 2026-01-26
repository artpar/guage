; Test 1: Identity Function
(≔ 𝕀 (λ (x) x))
(𝕀 5)

; Test 2: Const Function
(≔ 𝕂 (λ (x) (λ (y) x)))
((𝕂 10) 20)

; Test 3: Arithmetic in Lambda
(≔ ⊕1 (λ (x) (⊕ x 1)))
(⊕1 41)

; Test 4: Nested application
(≔ ⊗2 (λ (x) (⊗ x 2)))
(⊗2 21)
