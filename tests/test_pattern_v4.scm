; Pattern matching - pattern quoted, result NOT quoted

; Syntax: (∇ value (⟨⟩ (⟨⟩ (⌜ pattern) result-expr) ∅))
; Pattern is quoted (data)
; Result is code (will be evaluated in extended environment)

; Test 1: Wildcard
(⊨ :wild
   :ok
   (∇ #42 (⟨⟩ (⟨⟩ (⌜ _) :ok) ∅)))

; Test 2: Literal
(⊨ :lit
   :matched
   (∇ #42 (⟨⟩ (⟨⟩ (⌜ #42) :matched) ∅)))

; Test 3: Variable - result references the variable
(⊨ :var
   #42
   (∇ #42 (⟨⟩ (⟨⟩ (⌜ x) x) ∅)))

; Test 4: Variable in computation
(⊨ :var-calc
   #10
   (∇ #5 (⟨⟩ (⟨⟩ (⌜ n) (⊗ n #2)) ∅)))
