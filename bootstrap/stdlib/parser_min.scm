; Minimal parser for testing
(⋘ "stdlib/macros.scm")

; Character classification
(≔ ≈⊙space? (λ (𝕔)
  (∨… (≡ 𝕔 (⌜ :space))
  (∨… (≡ 𝕔 (⌜ :tab))
  (∨… (≡ 𝕔 (⌜ :newline))
      (≡ 𝕔 (⌜ :return)))))))

; Token helpers
(≔ ≈⊙→token (λ (𝕥 𝕧)
  (⟨⟩ 𝕥 𝕧)))

(≔ ≈⊙token-type (λ (𝕥)
  (◁ 𝕥)))

(≔ ≈⊙token-val (λ (𝕥)
  (◁ (▷ 𝕥))))

; Skip whitespace
(≔ ≈⊙skip-ws (λ (𝕤 𝕡)
  (? (≥ 𝕡 (≈# 𝕤))
     𝕡
     (? (≈⊙space? (≈→ 𝕤 𝕡))
        (≈⊙skip-ws 𝕤 (⊕ 𝕡 #1))
        𝕡))))

; Test
(≈⊙skip-ws "  hello" #0)
