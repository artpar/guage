; Debug variable patterns

; First, test that quote works
(⊨ :quote-works :x (⌜ x))

; Test pattern match with literal
(⊨ :literal-match :ok (∇ #42 (⟨⟩ (⟨⟩ (⌜ #42) (⟨⟩ :ok ∅)) ∅)))

; Test wildcard
(⊨ :wildcard-match :ok (∇ #42 (⟨⟩ (⟨⟩ (⌜ _) (⟨⟩ :ok ∅)) ∅)))

; Test variable pattern - simplest case
; Pattern: (⌜ x) should evaluate to symbol x
; Result: x should be looked up
(⊨ :var-pattern #42 (∇ #42 (⟨⟩ (⟨⟩ (⌜ x) (⟨⟩ x ∅)) ∅)))
