; Consistency Audit - Check all 55 functional primitives
; Testing that all primitives are accessible and have documentation

; ========== Arithmetic (9) ==========
⟲ :checking-arithmetic
(⌂ (⌜ ⊕))
(⌂ (⌜ ⊖))
(⌂ (⌜ ⊗))
(⌂ (⌜ ⊘))
(⌂ (⌜ %))
(⌂ (⌜ <))
(⌂ (⌜ >))
(⌂ (⌜ ≤))
(⌂ (⌜ ≥))

; ========== Logic & Comparison (5) ==========
⟲ :checking-logic
(⌂ (⌜ ≡))
(⌂ (⌜ ≢))
(⌂ (⌜ ∧))
(⌂ (⌜ ∨))
(⌂ (⌜ ¬))

; ========== Type Predicates (6) ==========
⟲ :checking-type-predicates
(⌂ (⌜ ℕ?))
(⌂ (⌜ 𝔹?))
(⌂ (⌜ :?))
(⌂ (⌜ ∅?))
(⌂ (⌜ ⟨⟩?))
(⌂ (⌜ #?))

; ========== Lists (3) ==========
⟲ :checking-lists
(⌂ (⌜ ⟨⟩))
(⌂ (⌜ ◁))
(⌂ (⌜ ▷))

; ========== Metaprogramming (1) ==========
⟲ :checking-meta
(⌂ (⌜ ⌜))

; ========== Debug/Error (4) ==========
⟲ :checking-debug
(⌂ (⌜ ⚠))
(⌂ (⌜ ⚠?))
(⌂ (⌜ ⊢))
(⌂ (⌜ ⟲))

; ========== Introspection (2) ==========
⟲ :checking-introspection
(⌂ (⌜ ⧉))
(⌂ (⌜ ⊛))

; ========== Testing (2) ==========
⟲ :checking-testing
(⌂ (⌜ ≟))
(⌂ (⌜ ⊨))

; ========== Documentation (5) ==========
⟲ :checking-documentation
(⌂ (⌜ ⌂))
(⌂ (⌜ ⌂∈))
(⌂ (⌜ ⌂≔))
(⌂ (⌜ ⌂⊛))
(⌂ (⌜ ⌂⊨))

; ========== CFG/DFG (2) ==========
⟲ :checking-cfg-dfg
(⌂ (⌜ ⌂⟿))
(⌂ (⌜ ⌂⇝))

; ========== Structures - Leaf (5) ==========
⟲ :checking-structures-leaf
(⌂ (⌜ ⊙≔))
(⌂ (⌜ ⊙))
(⌂ (⌜ ⊙→))
(⌂ (⌜ ⊙←))
(⌂ (⌜ ⊙?))

; ========== Structures - Node (4) ==========
⟲ :checking-structures-node
(⌂ (⌜ ⊚≔))
(⌂ (⌜ ⊚))
(⌂ (⌜ ⊚→))
(⌂ (⌜ ⊚?))

; ========== Graphs (6) ==========
⟲ :checking-graphs
(⌂ (⌜ ⊝≔))
(⌂ (⌜ ⊝))
(⌂ (⌜ ⊝⊕))
(⌂ (⌜ ⊝⊗))
(⌂ (⌜ ⊝→))
(⌂ (⌜ ⊝?))

⟲ :consistency-check-complete
:all-55-primitives-accessible
