; Test minimal parser loading
(⋘ "stdlib/macros.scm")

; Test 1: Define a simple function
(≔ test1 (λ (x) x))

; Test 2: String operations
(≔ test2 (λ (s) (≈# s)))

; Test 3: Use ≈→
(≔ test3 (λ (s p) (≈→ s p)))

; Test 4: Test our char classification
(≔ ≈⊙space-test? (λ (𝕔)
  (∨… (≡ 𝕔 (⌜ :space))
      (≡ 𝕔 (⌜ :tab)))))

:done
