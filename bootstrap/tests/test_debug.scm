; Debug test structure

; Generate tests
(≔ tests (⌂⊨ (⌜ ⊕)))

; Show structure
⟲ :all-tests
tests

⟲ :first-test
(≔ first (◁ tests))
first

⟲ :test-symbol
(◁ first)

⟲ :rest
(▷ first)

⟲ :name
(◁ (▷ first))

⟲ :expected
(◁ (▷ (▷ first)))

⟲ :actual
(◁ (▷ (▷ (▷ first))))
