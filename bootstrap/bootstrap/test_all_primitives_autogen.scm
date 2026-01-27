;; Test Auto-Test Generation Coverage
;; Check how many primitives generate non-empty tests

;; Sample primitives across all categories
(≔ test-prims (⌜ ⟨
  ;; Data structure
  ⟨⟩ ◁ ▷

  ;; Metaprogramming
  ⌜ ⌞

  ;; Pattern matching
  ∇

  ;; Equality
  ≡ ≢ ≟

  ;; Logic
  ∧ ∨ ¬

  ;; Arithmetic
  ⊕ ⊖ ⊗ ⊘ %

  ;; Comparison
  < > ≤ ≥

  ;; Type predicates
  ℕ? 𝔹? :? ∅? ⟨⟩? #? ⚠?

  ;; Error handling
  ⚠ ⊢

  ;; Debugging
  ⟲ ⧉ ⊛

  ;; Testing
  ⊨

  ;; Documentation
  ⌂ ⌂∈ ⌂≔ ⌂⊛ ⌂⊨
⟩⌝))

;; Count how many generate non-empty tests
(≔ count-tests (λ (prims)
  (? (∅? prims)
     #0
     (? (∅? (⌂⊨ (⌜ (◁ prims))))
        (count-tests (▷ prims))
        (⊕ #1 (count-tests (▷ prims)))))))

;; Run count
(count-tests test-prims)
