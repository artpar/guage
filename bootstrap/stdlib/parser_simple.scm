; Simplified S-expression parser - proof of concept
; Avoids deep nested lambdas, handles only basic cases
(⋘ "stdlib/macros.scm")

; ═══════════════════════════════════════════════════════════════
; Reuse character classification and tokenization from parser.scm
; ═══════════════════════════════════════════════════════════════

(⋘ "stdlib/parser.scm")  ; Load tokenizer functions

; ═══════════════════════════════════════════════════════════════
; Simplified Parser - No nested lambdas beyond 1 level
; ═══════════════════════════════════════════════════════════════

; Helper: Get token type safely
(≔ ≈⊙get-type (λ (𝕥𝕠𝕜𝕤)
  (? (∅? 𝕥𝕠𝕜𝕤)
     (⌜ :eof)
     (≈⊙token-type (◁ 𝕥𝕠𝕜𝕤)))))

; Helper: Get token value safely
(≔ ≈⊙get-val (λ (𝕥𝕠𝕜𝕤)
  (? (∅? 𝕥𝕠𝕜𝕤)
     ∅
     (≈⊙token-val (◁ 𝕥𝕠𝕜𝕤)))))

; Helper: Get remaining tokens
(≔ ≈⊙rest-tokens (λ (𝕥𝕠𝕜𝕤)
  (? (∅? 𝕥𝕠𝕜𝕤)
     ∅
     (▷ 𝕥𝕠𝕜𝕤))))

; Parse one atom (number or symbol)
(≔ ≈⊙parse-atom (λ (𝕥𝕠𝕜𝕤 𝕥𝕪𝕡𝕖 𝕧𝕒𝕝)
  (⟨⟩ 𝕧𝕒𝕝 (≈⊙rest-tokens 𝕥𝕠𝕜𝕤))))

; Parse list - simplified version
(≔ ≈⊙parse-list-simple (λ (𝕥𝕠𝕜𝕤 𝕒𝕔𝕔)
  (? (∅? 𝕥𝕠𝕜𝕤)
     (⚠ (⌜ :unclosed-list) ∅)
     (? (≡ (≈⊙get-type 𝕥𝕠𝕜𝕤) (⌜ :rparen))
        ; Found closing paren - return accumulated list
        (⟨⟩ 𝕒𝕔𝕔 (≈⊙rest-tokens 𝕥𝕠𝕜𝕤))
        ; Parse one element and continue
        (? (≡ (≈⊙get-type 𝕥𝕠𝕜𝕤) (⌜ :lparen))
           ; Nested list - recursive call
           (≈⊙parse-list-simple (≈⊙rest-tokens 𝕥𝕠𝕜𝕤) 𝕒𝕔𝕔)
           ; Atom - add to accumulator
           (≈⊙parse-list-simple
             (≈⊙rest-tokens 𝕥𝕠𝕜𝕤)
             (⟨⟩ 𝕒𝕔𝕔 (≈⊙get-val 𝕥𝕠𝕜𝕤))))))))

; Simple parse-one - handles atoms and lists
(≔ ≈⊙parse-simple (λ (𝕥𝕠𝕜𝕤)
  (? (∅? 𝕥𝕠𝕜𝕤)
     (⚠ (⌜ :eof) ∅)
     (? (≡ (≈⊙get-type 𝕥𝕠𝕜𝕤) (⌜ :lparen))
        ; Parse list
        (≈⊙parse-list-simple (≈⊙rest-tokens 𝕥𝕠𝕜𝕤) ∅)
        ; Parse atom
        (≈⊙parse-atom 𝕥𝕠𝕜𝕤 (≈⊙get-type 𝕥𝕠𝕜𝕤) (≈⊙get-val 𝕥𝕠𝕜𝕤))))))

; Top-level parse from string
(≔ ≈⊙parse-str (λ (𝕤)
  (≈⊙parse-simple (≈⊙tokenize 𝕤))))

:parser-simple-loaded
