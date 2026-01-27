; Test passing number list to lambda

; Direct - this should work
((λ (x) x) (⟨⟩ #1 (⟨⟩ #2 ∅)))

; Get head - should return #1
((λ (lst) (◁ lst)) (⟨⟩ #1 (⟨⟩ #2 ∅)))
