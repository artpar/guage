; String Library for Guage
; Provides higher-level string manipulation utilities
; Built on primitive operations: ≈, ≈⊕, ≈#, ≈→, ≈⊂, ≈≡, ≈<
; Day 121: Core operations now delegate to SIMD-accelerated C primitives

; ============================================================================
; Aliases: Map long names to C primitive symbols
; ============================================================================

; Search (Tier 1 — SIMD-accelerated)
(≔ string-index-of ≈⊳)
(≔ string-rfind ≈⊲)
(≔ string-contains? ≈∈?)
(≔ string-starts-with? ≈⊲?)
(≔ string-ends-with? ≈⊳?)
(≔ string-count ≈⊳#)

; Transform (Tier 2)
(≔ string-reverse ≈⇄)
(≔ string-repeat ≈⊛)
(≔ string-replace ≈⇔)
(≔ string-replacen ≈⇔#)

; Trim (Tier 3 — SIMD whitespace scan)
(≔ string-trim-left ≈⊏)
(≔ string-trim-right ≈⊐)
(≔ string-trim ≈⊏⊐)

; Split (Tier 4 — SIMD delimiter scan)
(≔ string-split ≈÷)
(≔ string-splitn ≈÷#)
(≔ string-fields ≈÷⊔)

; Pad (Tier 5)
(≔ string-pad-left ≈⊏⊕)
(≔ string-pad-right ≈⊐⊕)

; Strip (Tier 6)
(≔ string-strip-prefix ≈⊏⊖)
(≔ string-strip-suffix ≈⊐⊖)

; ============================================================================
; Core: Join (kept — no C primitive needed, pure Scheme is fine)
; ============================================================================

; ⌂: Join list of strings with delimiter
; ∈: [≈] → ≈ → ≈
; Ex: (string-join ⟨"a" ⟨"b" ⟨"c" ∅⟩⟩⟩ ",") → "a,b,c"
; Ex: (string-join ⟨"hello" ⟨"world" ∅⟩⟩ " ") → "hello world"
(≔ string-join (λ (lst delim)
  (? (∅? lst)
     ""  ; Empty list → empty string
     (? (∅? (▷ lst))
        (◁ lst)  ; Single element → just that element
        ; Multiple elements → join with delimiter
        (≈⊕ (◁ lst) (≈⊕ delim (string-join (▷ lst) delim)))))))

; Alias: ≈⊗
(≔ ≈⊗ string-join)

; ============================================================================
; Extended: Split by newlines (thin wrapper)
; ============================================================================

; ⌂: Split string by newlines
; ∈: ≈ → [≈]
; Ex: (string-split-lines "a\nb\nc") → ⟨"a" ⟨"b" ⟨"c" ∅⟩⟩⟩
(≔ string-split-lines (λ (s)
  (≈÷ s "\n")))

; Alias: ≈÷⊳
(≔ ≈÷⊳ string-split-lines)

; ============================================================================
; Case conversion (delegates to C primitives)
; ============================================================================

; ⌂: Convert single-char symbol to uppercase (ASCII a-z only)
; ∈: :char → :char
(≔ char-to-upper (λ (c)
  ((λ (code)
    (? (∧ (≥ code #97) (≤ code #122))
       (≈→ (#→≈ (⊖ code #32)) #0)
       c))
   (≈→# (≈ c) #0))))

; ⌂: Convert single-char symbol to lowercase (ASCII A-Z only)
; ∈: :char → :char
(≔ char-to-lower (λ (c)
  ((λ (code)
    (? (∧ (≥ code #65) (≤ code #90))
       (≈→ (#→≈ (⊕ code #32)) #0)
       c))
   (≈→# (≈ c) #0))))

; ⌂: Convert string to uppercase (ASCII only)
; ∈: ≈ → ≈
; Delegates to C-side ≈↑ primitive for single-pass efficiency
(≔ string-upcase ≈↑)

; ⌂: Convert string to lowercase (ASCII only)
; ∈: ≈ → ≈
; Delegates to C-side ≈↓ primitive for single-pass efficiency
(≔ string-downcase ≈↓)

; ============================================================================
; Module complete - Core string utilities available
; ============================================================================
