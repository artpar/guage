; Debug symbol lists

; Test 1: Can we construct a symbol list?
(⟲ (⟨⟩ :a (⟨⟩ :b ∅)))

; Test 2: Can we get the head?
(⟲ (◁ (⟨⟩ :a (⟨⟩ :b ∅))))

; Test 3: Can we get the tail?
(⟲ (▷ (⟨⟩ :a (⟨⟩ :b ∅))))

; Test 4: Can we get head of tail?
(⟲ (◁ (▷ (⟨⟩ :a (⟨⟩ :b ∅)))))
