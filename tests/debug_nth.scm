; Debug nth function

; ⊡ :: ℕ → [α] → α | ∅
(≔ ⊡ (λ (idx) (λ (lst)
  (? (∅? lst)
     ∅
     (? (≡ idx #0)
        (◁ lst)
        ((⊡ (⊖ idx #1)) (▷ lst)))))))

; Test 1: Simple number list
(⟲ ((⊡ #0) (⟨⟩ #1 (⟨⟩ #2 ∅))))

; Test 2: Get second element
(⟲ ((⊡ #1) (⟨⟩ #1 (⟨⟩ #2 ∅))))

; Test 3: Symbol list - FIRST element
(⟲ ((⊡ #0) (⟨⟩ :a (⟨⟩ :b ∅))))

; Test 4: Symbol list - SECOND element
(⟲ ((⊡ #1) (⟨⟩ :a (⟨⟩ :b ∅))))
