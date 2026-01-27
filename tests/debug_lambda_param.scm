; Test passing symbol list to lambda

; Direct call
((λ (x) x) (⟨⟩ :a (⟨⟩ :b ∅)))

; Get head through lambda
((λ (lst) (◁ lst)) (⟨⟩ :a (⟨⟩ :b ∅)))
