; Debug Option predicates

(⊚≔ :Option (⌜ (:None)) (⌜ (:Some :value)))
(≔ ⊙◇ (λ (value) (⊚ :Option :Some value)))
(≔ ⊙∅ (⊚ :Option :None))

; Test predicate definition
(≔ ⊙? (λ (opt) (∇ opt (⌜ (((⊚ :Option :Some _) #t) ((⊚ :Option :None) #f))))))

; Create test values
(≔ some-10 (⊙◇ #10))

; Test what the predicate returns
(⟲ (⊙? some-10))
(⟲ (⊙? ⊙∅))

; Also test direct pattern matching
(⟲ (∇ some-10 (⌜ (((⊚ :Option :Some _) :is-some) ((⊚ :Option :None) :is-none)))))
(⟲ (∇ ⊙∅ (⌜ (((⊚ :Option :Some _) :is-some) ((⊚ :Option :None) :is-none)))))
