; Test currying with symbols

; Simple identity function
(≔ id (λ (x) x))

; Test 1: Can we pass a symbol list to identity?
(⟲ (id (⟨⟩ :a (⟨⟩ :b ∅))))

; Simple get-first function
(≔ getfirst (λ (lst) (◁ lst)))

; Test 2: Can we get first of symbol list?
(⟲ (getfirst (⟨⟩ :a (⟨⟩ :b ∅))))

; Curried function
(≔ const (λ (x) (λ (y) x)))

; Test 3: Curried with symbols
(⟲ ((const #42) (⟨⟩ :a ∅)))

; Test 4: Check if this is a currying issue
(≔ ⊡-partial (⊡ #0))
(⟲ ⊡-partial)
