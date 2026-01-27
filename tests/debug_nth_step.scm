; Debug nth step by step

(≔ ⊡ (λ (idx) (λ (lst)
  (? (∅? lst)
     ∅
     (? (≡ idx #0)
        (◁ lst)
        ((⊡ (⊖ idx #1)) (▷ lst)))))))

; Step 1: Create a symbol list
(≔ symlst (⟨⟩ :a (⟨⟩ :b (⟨⟩ :c ∅))))
(⟲ symlst)

; Step 2: Test with index 0 (should return first element)
(⟲ ((⊡ #0) symlst))

; Step 3: If that works, test with index 1
;(⟲ ((⊡ #1) symlst))
