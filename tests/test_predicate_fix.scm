; Test predicate fix - force evaluation before ∇

(⊚≔ :Option (⌜ (:None)) (⌜ (:Some :value)))
(≔ ⊙◇ (λ (value) (⊚ :Option :Some value)))
(≔ ⊙∅ (⊚ :Option :None))

; BROKEN: ∇ doesn't evaluate its first argument
; (≔ ⊙?-broken (λ (opt) (∇ opt (⌜ (((⊚ :Option :Some _) #t) ((⊚ :Option :None) #f))))))

; FIXED: Use immediately-invoked lambda to force evaluation
(≔ ⊙? (λ (opt) ((λ (v) (∇ v (⌜ (((⊚ :Option :Some _) #t) ((⊚ :Option :None) #f))))) opt)))

; Test
(≔ some-10 (⊙◇ #10))
(⟲ (⊙? some-10))
(⟲ (⊙? ⊙∅))

; Alternative: use conditional instead of pattern matching
(≔ ⊙?-alt (λ (opt) (⊚? opt :Option :Some)))
(⟲ (⊙?-alt some-10))
(⟲ (⊙?-alt ⊙∅))
