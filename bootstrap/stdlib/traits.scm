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

; ═══════════════════════════════════════════════════════════════
; Extended Trait Protocols (Day 143)
; Mappable, Foldable, Semigroup, Monoid, Filterable, Hashable
; ═══════════════════════════════════════════════════════════════

; ─── :Mappable ───
(⊧≔ :Mappable (⟨⟩ :map ∅))

(⊧⊕ :Pair   :Mappable (⟨⟩ (⟨⟩ :map (λ (f) (λ (coll) ((↦ f) coll)))) ∅))
(⊧⊕ :Nil    :Mappable (⟨⟩ (⟨⟩ :map (λ (f) (λ (coll) ∅))) ∅))
(⊧⊕ :Vector :Mappable (⟨⟩ (⟨⟩ :map (λ (f) (λ (v) (⟦↦ v f)))) ∅))

(≔ ⊧↦ (λ (f) (λ (coll) (((⊧→! coll :Mappable :map) f) coll))))

; ─── :Foldable ───
(⊧≔ :Foldable (⟨⟩ :fold-left (⟨⟩ :fold-right ∅)))

(⊧⊕ :Pair :Foldable
  (⟨⟩ (⟨⟩ :fold-left (λ (f) (λ (acc) (λ (lst) (((⊕← f) acc) lst)))))
  (⟨⟩ (⟨⟩ :fold-right (λ (f) (λ (lst) (λ (acc) (((⊕→ f) lst) acc))))) ∅)))
(⊧⊕ :Nil :Foldable
  (⟨⟩ (⟨⟩ :fold-left (λ (f) (λ (acc) (λ (lst) acc))))
  (⟨⟩ (⟨⟩ :fold-right (λ (f) (λ (lst) (λ (acc) acc)))) ∅)))

(≔ ⊧⊕← (λ (f) (λ (acc) (λ (coll) ((((⊧→! coll :Foldable :fold-left) f) acc) coll)))))
(≔ ⊧⊕→ (λ (f) (λ (coll) (λ (acc) ((((⊧→! coll :Foldable :fold-right) f) coll) acc)))))

; ─── :Semigroup ───
(⊧≔ :Semigroup (⟨⟩ :combine ∅))

(⊧⊕ :Number :Semigroup (⟨⟩ (⟨⟩ :combine (λ (a) (λ (b) (⊕ a b)))) ∅))
(⊧⊕ :String :Semigroup (⟨⟩ (⟨⟩ :combine (λ (a) (λ (b) (≈⊕ a b)))) ∅))
(⊧⊕ :Pair   :Semigroup (⟨⟩ (⟨⟩ :combine (λ (a) (λ (b) ((⧺ b) a)))) ∅))
(⊧⊕ :Nil    :Semigroup (⟨⟩ (⟨⟩ :combine (λ (a) (λ (b) b))) ∅))

(≔ ⊧⊕⊕ (λ (a) (λ (b) (((⊧→! a :Semigroup :combine) a) b))))

; ─── :Monoid ───
(⊧≔ :Monoid (⟨⟩ :empty ∅))

(⊧⊕ :Number :Monoid (⟨⟩ (⟨⟩ :empty (λ () #0)) ∅))
(⊧⊕ :String :Monoid (⟨⟩ (⟨⟩ :empty (λ () "")) ∅))
(⊧⊕ :Pair   :Monoid (⟨⟩ (⟨⟩ :empty (λ () ∅)) ∅))
(⊧⊕ :Nil    :Monoid (⟨⟩ (⟨⟩ :empty (λ () ∅)) ∅))

(≔ ⊧∅ (λ (x) ((⊧→! x :Monoid :empty))))

; Generic mconcat: fold using combine + empty
(≔ ⊧mconcat (λ (lst)
  (? (∅? lst) ∅
     (((⊧⊕← (λ (acc) (λ (x) ((⊧⊕⊕ acc) x)))) (⊧∅ (◁ lst))) lst))))

; ─── :Filterable ───
(⊧≔ :Filterable (⟨⟩ :filter ∅))

(⊧⊕ :Pair :Filterable (⟨⟩ (⟨⟩ :filter (λ (pred) (λ (coll) ((⊲ pred) coll)))) ∅))
(⊧⊕ :Nil  :Filterable (⟨⟩ (⟨⟩ :filter (λ (pred) (λ (coll) ∅))) ∅))

(≔ ⊧⊲ (λ (pred) (λ (coll) (((⊧→! coll :Filterable :filter) pred) coll))))

; ─── :Hashable ───
(⊧≔ :Hashable (⟨⟩ :hash ∅))

(⊧⊕ :Number :Hashable (⟨⟩ (⟨⟩ :hash (λ (x) x)) ∅))
(⊧⊕ :String :Hashable (⟨⟩ (⟨⟩ :hash (λ (x) (≈# x))) ∅))
(⊧⊕ :Symbol :Hashable (⟨⟩ (⟨⟩ :hash (λ (x) (≈# (≈ x)))) ∅))
