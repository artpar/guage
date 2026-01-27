; Pattern matching with proper ⟨⟩ syntax

; Each clause is (⟨⟩ pattern (⟨⟩ result ∅))
; The whole clause list is quoted

; Test 1: Wildcard
(⊨ :wild
   :ok
   (∇ #42 (⌜ (⟨⟩ (⟨⟩ _ (⟨⟩ :ok ∅)) ∅))))

; Test 2: Literal
(⊨ :lit
   :matched
   (∇ #42 (⌜ (⟨⟩ (⟨⟩ #42 (⟨⟩ :matched ∅)) ∅))))

; Test 3: Variable
(⊨ :var
   #42
   (∇ #42 (⌜ (⟨⟩ (⟨⟩ x (⟨⟩ x ∅)) ∅))))
