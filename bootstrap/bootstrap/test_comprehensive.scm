; Comprehensive Test Suite for Guage

; Basic Lambda Tests
(≔ id (λ (x) x))
(⟲ (id 42))

(≔ const (λ (x) (λ (y) x)))
(⟲ ((const 10) 20))

(≔ add1 (λ (x) (⊕ x 1)))
(⟲ (add1 99))

; Introspection Tests
(⟲ (⊙ 42))
(⟲ (⊙ id))
(⟲ (⧉ id))
(⟲ (⧉ const))

; Error Handling Tests
(≔ safe-div (λ (x y)
  (? (≡ y #0)
     (⚠ :div-by-zero y)
     (⊘ x y))))

(⟲ (safe-div 10 2))
(⟲ (safe-div 10 #0))

; Assertions
(⟲ (⊢ #t :ok))
(⟲ (⊢ (≡ (⊕ 2 2) #4) :math-works))

; Deep Equality
(⟲ (≟ #42 #42))
(⟲ (≟ (⟨⟩ 1 2) (⟨⟩ 1 2)))
