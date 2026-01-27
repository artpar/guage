; Pattern matching - both pattern and result quoted

; Syntax: (∇ value (⟨⟩ (⟨⟩ (⌜ pattern) (⌜ result)) ∅))
; The ⟨⟩ calls ARE evaluated (build pairs)
; The (⌜ ...) parts are NOT evaluated (remain quoted)

; Test 1: Wildcard
(⊨ :wild
   :ok
   (∇ #42 (⟨⟩ (⟨⟩ (⌜ _) (⌜ :ok)) ∅)))

; Test 2: Literal
(⊨ :lit
   :matched
   (∇ #42 (⟨⟩ (⟨⟩ (⌜ #42) (⌜ :matched)) ∅)))

; Test 3: Variable - result uses the variable
(⊨ :var
   #42
   (∇ #42 (⟨⟩ (⟨⟩ (⌜ x) (⌜ x)) ∅)))
