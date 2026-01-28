; Test parser step by step
(⋘ "stdlib/macros.scm")

; Phase 1 - Character classification
(≔ ≈⊙space? (λ (𝕔)
  (∨… (≡ 𝕔 (⌜ :space))
  (∨… (≡ 𝕔 (⌜ :tab))
  (∨… (≡ 𝕔 (⌜ :newline))
      (≡ 𝕔 (⌜ :return)))))))

; Phase 2 - Token helpers
(≔ ≈⊙→token (λ (𝕥 𝕧)
  (⟨⟩ 𝕥 𝕧)))

(≔ ≈⊙token-type (λ (𝕥)
  (◁ 𝕥)))

; Test it
(≈⊙→token (⌜ :number) "42")
