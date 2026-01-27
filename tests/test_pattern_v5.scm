; Pattern matching - correct clause structure

; Syntax: (∇ value (⟨⟩ (⟨⟩ (⌜ pattern) (⟨⟩ result ∅)) ...))
; Each clause is a pair where:
; - car is the quoted pattern
; - cdr is a list with one element (the result expression)

; Test 1: Wildcard
(⊨ :wild
   :ok
   (∇ #42 (⟨⟩ (⟨⟩ (⌜ _) (⟨⟩ :ok ∅)) ∅)))

; Test 2: Literal
(⊨ :lit
   :matched
   (∇ #42 (⟨⟩ (⟨⟩ (⌜ #42) (⟨⟩ :matched ∅)) ∅)))

; Test 3: Variable
(⊨ :var
   #42
   (∇ #42 (⟨⟩ (⟨⟩ (⌜ x) (⟨⟩ x ∅)) ∅)))

; Test 4: Variable in computation
(⊨ :var-calc
   #10
   (∇ #5 (⟨⟩ (⟨⟩ (⌜ n) (⟨⟩ (⊗ n #2) ∅)) ∅)))
