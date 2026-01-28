; Documentation Formatting Library
; Makes auto-generated documentation human-readable

; ≈⊙doc-simple :: :symbol → string
; Simple doc formatter that works
(≔ ≈⊙doc-simple (λ (sym)
  (≈⊕ "Symbol: "
  (≈⊕ (≈ sym)
  (≈⊕ "\nType: "
  (≈⊕ (≈ (⌂∈ sym))
  (≈⊕ "\nDescription: "
  (≈⊕ (≈ (⌂ sym))
  "\n"))))))))

; ≈⊙doc-format :: :symbol → string
; Format complete documentation for a symbol with box drawing
(≔ ≈⊙doc-format (λ (sym)
  (≈⊕ "┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n"
  (≈⊕ "┃ 📖 "
  (≈⊕ (≈ sym)
  (≈⊕ "\n┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫\n"
  (≈⊕ "┃ Type: "
  (≈⊕ (≈ (⌂∈ sym))
  (≈⊕ "\n┃\n┃ "
  (≈⊕ (≈ (⌂ sym))
  "\n┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n"))))))))))

; ≈⊙doc-deps :: :symbol → string
; Format dependencies nicely
(≔ ≈⊙doc-deps (λ (sym)
  (≈⊕ "Dependencies of "
  (≈⊕ (≈ sym)
  (≈⊕ ": "
  (≈⊕ (≈ (⌂≔ sym))
  "\n"))))))

; Test examples:
; (≈⊙doc-simple (⌜ ⊕))
; (≈⊙doc-format (⌜ ⊕))
; (≈⊙doc-deps (⌜ ⊕))
