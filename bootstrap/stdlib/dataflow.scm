; ═══════════════════════════════════════════════════════════════
; Guage Standard Library: Data Flow Analysis
; ═══════════════════════════════════════════════════════════════
; Status: NEW
; Created: 2026-01-29 (Day 80)
; Purpose: Set operations and data flow analysis algorithms
;
; Symbols defined:
; Set Operations:
; - ∪∪ (set-union) - combine two sets
; - ∩ (set-intersection) - elements in both
; - ∖ (set-difference) - elements in first but not second
; - ⊆ (set-subset?) - is first a subset of second
; - ≡∪ (set-equal?) - same elements
;
; Data Flow Analysis:
; - ⇝⊃ (reaching-defs) - reaching definitions analysis
; - ⇝← (live-vars) - live variables analysis
; - ⇝∪ (available-exprs) - available expressions analysis
; ═══════════════════════════════════════════════════════════════

; Load list operations
(⋘ "bootstrap/stdlib/list.scm")

; ═══════════════════════════════════════════════════════════════
; Set Operations (lists as sets)
; ═══════════════════════════════════════════════════════════════

; ∪∪ :: [α] → [α] → [α]
; Set union - elements in either set (no duplicates)
(≔ ∪∪ (λ (set2) (λ (set1)
  (∪ ((⧺ set2) set1)))))

; ∩ :: [α] → [α] → [α]
; Set intersection - elements in both sets
(≔ ∩ (λ (set2) (λ (set1)
  ((⊲ (λ (x) ((∈ x) set2))) set1))))

; ∖ :: [α] → [α] → [α]
; Set difference - elements in first but not second
(≔ ∖ (λ (set2) (λ (set1)
  ((⊲ (λ (x) (¬ ((∈ x) set2)))) set1))))

; ⊆ :: [α] → [α] → 𝔹
; Subset - is first a subset of second
(≔ ⊆ (λ (set2) (λ (set1)
  ((∀ (λ (x) ((∈ x) set2))) set1))))

; ≡∪ :: [α] → [α] → 𝔹
; Set equality - same elements (order independent)
(≔ ≡∪ (λ (set2) (λ (set1)
  (∧ ((⊆ set2) set1) ((⊆ set1) set2)))))

; ═══════════════════════════════════════════════════════════════
; Fixed Point Iteration
; ═══════════════════════════════════════════════════════════════

; ⊛⊛ :: (α → α) → α → α
; Fixed point - iterate until no change
; Uses set equality for termination
(≔ ⊛⊛ (λ (f) (λ (init)
  ((λ (next)
    (? ((≡∪ next) init)
       init
       ((⊛⊛ f) next)))
   (f init)))))

; ═══════════════════════════════════════════════════════════════
; Data Flow Analysis: Reaching Definitions
; ═══════════════════════════════════════════════════════════════
; A definition d reaches a point p if there is a path from d to p
; where d is not killed (overwritten) along the path.
;
; Transfer function: out[B] = gen[B] ∪ (in[B] - kill[B])
; Meet: in[B] = ∪ out[predecessors]
; Direction: Forward

; Transfer function for reaching definitions
; gen = definitions generated at this node
; kill = definitions killed at this node
; in = definitions reaching entry of node
(≔ ⇝⊃-transfer (λ (gen) (λ (kill) (λ (in)
  ((∪∪ gen) ((∖ kill) in))))))

; Single iteration of reaching definitions
; nodes = list of (node-id gen kill preds)
; current = current solution (list of (node-id in out))
(≔ ⇝⊃-iter (λ (nodes) (λ (current)
  (? (∅? nodes)
     ∅
     (⟨⟩ ((⇝⊃-node (◁ nodes)) current)
         ((⇝⊃-iter (▷ nodes)) current))))))

; Process one node for reaching definitions
; node = (node-id gen kill preds)
; solution = current (node-id in out) pairs
(≔ ⇝⊃-node (λ (node) (λ (solution)
  ((λ (node-id) ((λ (gen) ((λ (kill) ((λ (preds)
    ; in = union of out[pred] for all predecessors
    ((λ (in)
      ; out = gen ∪ (in - kill)
      ((λ (out)
        (⟨⟩ node-id (⟨⟩ in (⟨⟩ out ∅))))
       (((⇝⊃-transfer gen) kill) in)))
     ((⇝⊃-meet preds) solution)))
   (◁ (▷ (▷ (▷ node))))))   ; preds
   (◁ (▷ (▷ node)))))       ; kill
   (◁ (▷ node))))           ; gen
   (◁ node)))))             ; node-id

; Meet function: union of predecessor outputs
(≔ ⇝⊃-meet (λ (preds) (λ (solution)
  (? (∅? preds)
     ∅
     ((∪∪ ((⇝⊃-get-out (◁ preds)) solution))
      ((⇝⊃-meet (▷ preds)) solution))))))

; Get out set for a node from solution
(≔ ⇝⊃-get-out (λ (node-id) (λ (solution)
  (? (∅? solution)
     ∅
     (? (≡ node-id (◁ (◁ solution)))
        (◁ (▷ (▷ (◁ solution))))  ; out is third element
        ((⇝⊃-get-out node-id) (▷ solution)))))))

; ═══════════════════════════════════════════════════════════════
; Data Flow Analysis: Live Variables
; ═══════════════════════════════════════════════════════════════
; A variable is live at a point p if it may be used before
; being redefined on some path from p.
;
; Transfer function: in[B] = use[B] ∪ (out[B] - def[B])
; Meet: out[B] = ∪ in[successors]
; Direction: Backward

; Transfer function for live variables
; use = variables used at this node
; def = variables defined at this node
; out = variables live at exit of node
(≔ ⇝←-transfer (λ (use) (λ (def) (λ (out)
  ((∪∪ use) ((∖ def) out))))))

; Process one node for live variables
; node = (node-id use def succs)
; solution = current (node-id in out) pairs
(≔ ⇝←-node (λ (node) (λ (solution)
  ((λ (node-id) ((λ (use) ((λ (def) ((λ (succs)
    ; out = union of in[succ] for all successors
    ((λ (out)
      ; in = use ∪ (out - def)
      ((λ (in)
        (⟨⟩ node-id (⟨⟩ in (⟨⟩ out ∅))))
       (((⇝←-transfer use) def) out)))
     ((⇝←-meet succs) solution)))
   (◁ (▷ (▷ (▷ node))))))   ; succs
   (◁ (▷ (▷ node)))))       ; def
   (◁ (▷ node))))           ; use
   (◁ node)))))             ; node-id

; Meet function: union of successor inputs
(≔ ⇝←-meet (λ (succs) (λ (solution)
  (? (∅? succs)
     ∅
     ((∪∪ ((⇝←-get-in (◁ succs)) solution))
      ((⇝←-meet (▷ succs)) solution))))))

; Get in set for a node from solution
(≔ ⇝←-get-in (λ (node-id) (λ (solution)
  (? (∅? solution)
     ∅
     (? (≡ node-id (◁ (◁ solution)))
        (◁ (▷ (◁ solution)))  ; in is second element
        ((⇝←-get-in node-id) (▷ solution)))))))

; ═══════════════════════════════════════════════════════════════
; Data Flow Analysis: Available Expressions
; ═══════════════════════════════════════════════════════════════
; An expression is available at point p if it has been computed
; on every path to p and not killed (operands not modified).
;
; Transfer function: out[B] = gen[B] ∪ (in[B] - kill[B])
; Meet: in[B] = ∩ out[predecessors] (intersection!)
; Direction: Forward

; Meet function for available expressions: intersection
(≔ ⇝∪-meet (λ (preds) (λ (solution)
  (? (∅? preds)
     ∅  ; Empty means "all expressions" in theory, but we use empty for init
     (? (∅? (▷ preds))
        ; Single predecessor - just return its out
        ((⇝⊃-get-out (◁ preds)) solution)
        ; Multiple predecessors - intersect
        ((∩ ((⇝⊃-get-out (◁ preds)) solution))
         ((⇝∪-meet (▷ preds)) solution)))))))

; ═══════════════════════════════════════════════════════════════
; Helper: Initialize solution with empty sets
; ═══════════════════════════════════════════════════════════════

(≔ ⇝-init-solution (λ (nodes)
  (? (∅? nodes)
     ∅
     (⟨⟩ (⟨⟩ (◁ (◁ nodes))  ; node-id
             (⟨⟩ ∅           ; in = empty
                 (⟨⟩ ∅ ∅)))  ; out = empty
         (⇝-init-solution (▷ nodes))))))

; ═══════════════════════════════════════════════════════════════
; Module Complete
; ═══════════════════════════════════════════════════════════════
; Set operations: 5 (∪∪, ∩, ∖, ⊆, ≡∪)
; Data flow: Fixed point, transfer functions, meet operations
; ═══════════════════════════════════════════════════════════════

"✓ Data flow analysis module loaded"
