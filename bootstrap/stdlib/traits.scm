; ─── Core Trait Protocols ───
; Showable, Equatable, Comparable
; Uses FDT-backed dispatch (⊧→!) for ~5ns hot path

; ─── :Showable ───
(⊧≔ :Showable (⟨⟩ :show ∅))

(⊧⊕ :Number :Showable (⟨⟩ (⟨⟩ :show (λ (x) (≈ x))) ∅))
(⊧⊕ :Bool   :Showable (⟨⟩ (⟨⟩ :show (λ (x) (? x "#t" "#f"))) ∅))
(⊧⊕ :Symbol :Showable (⟨⟩ (⟨⟩ :show (λ (x) (≈ x))) ∅))
(⊧⊕ :Nil    :Showable (⟨⟩ (⟨⟩ :show (λ (x) "∅")) ∅))
(⊧⊕ :String :Showable (⟨⟩ (⟨⟩ :show (λ (x) x)) ∅))
(⊧⊕ :Error  :Showable (⟨⟩ (⟨⟩ :show (λ (x) (≈ x))) ∅))
(⊧⊕ :Pair   :Showable (⟨⟩ (⟨⟩ :show (λ (x) (≈ x))) ∅))
(⊧⊕ :Lambda :Showable (⟨⟩ (⟨⟩ :show (λ (x) "<λ>")) ∅))

; Convenience: uses ⊧→! (fused fast dispatch)
(≔ ⊧show (λ (x) ((⊧→! x :Showable :show) x)))

; ─── :Equatable ───
(⊧≔ :Equatable (⟨⟩ :equal? ∅))

(⊧⊕ :Number :Equatable (⟨⟩ (⟨⟩ :equal? (λ (a) (λ (b) (≡ a b)))) ∅))
(⊧⊕ :Bool   :Equatable (⟨⟩ (⟨⟩ :equal? (λ (a) (λ (b) (≡ a b)))) ∅))
(⊧⊕ :Symbol :Equatable (⟨⟩ (⟨⟩ :equal? (λ (a) (λ (b) (≡ a b)))) ∅))
(⊧⊕ :Nil    :Equatable (⟨⟩ (⟨⟩ :equal? (λ (a) (λ (b) (≡ a b)))) ∅))
(⊧⊕ :String :Equatable (⟨⟩ (⟨⟩ :equal? (λ (a) (λ (b) (≈≡ a b)))) ∅))

(≔ ⊧≡ (λ (a) (λ (b) (((⊧→! a :Equatable :equal?) a) b))))

; ─── :Comparable ───
(⊧≔ :Comparable (⟨⟩ :compare ∅))

(⊧⊕ :Number :Comparable
  (⟨⟩ (⟨⟩ :compare (λ (a) (λ (b) (? (< a b) :lt (? (≡ a b) :eq :gt))))) ∅))
(⊧⊕ :String :Comparable
  (⟨⟩ (⟨⟩ :compare (λ (a) (λ (b) (? (≈< a b) :lt (? (≈≡ a b) :eq :gt))))) ∅))

; Derived dispatchers
(≔ ⊧compare (λ (a) (λ (b) (((⊧→! a :Comparable :compare) a) b))))
(≔ ⊧<  (λ (a) (λ (b) (≡ ((⊧compare a) b) :lt))))
(≔ ⊧≤  (λ (a) (λ (b) (¬ (≡ ((⊧compare a) b) :gt)))))
(≔ ⊧>  (λ (a) (λ (b) (≡ ((⊧compare a) b) :gt))))
(≔ ⊧≥  (λ (a) (λ (b) (¬ (≡ ((⊧compare a) b) :lt)))))

; Generic sort using trait dispatch
(≔ ⊧sort (λ (lst)
  (? (∅? lst) ∅
    ((⊙sort→ (λ (a) (λ (b) ((⊧≤ a) b)))) lst))))
