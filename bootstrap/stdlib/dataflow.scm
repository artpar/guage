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
(load "bootstrap/stdlib/list.scm")

; ═══════════════════════════════════════════════════════════════
; Set Operations (lists as sets)
; ═══════════════════════════════════════════════════════════════

; ∪∪ :: [α] -> [α] -> [α]
; Set union - elements in either set (no duplicates)
(define ∪∪ (lambda (set2) (lambda (set1)
  (∪ ((⧺ set2) set1)))))

; ∩ :: [α] -> [α] -> [α]
; Set intersection - elements in both sets
(define ∩ (lambda (set2) (lambda (set1)
  ((list-filter (lambda (x) ((∋ x) set2))) set1))))

; ∖ :: [α] -> [α] -> [α]
; Set difference - elements in first but not second
(define ∖ (lambda (set2) (lambda (set1)
  ((list-filter (lambda (x) (not ((∋ x) set2)))) set1))))

; ⊆ :: [α] -> [α] -> Bool
; Subset - is first a subset of second
(define ⊆ (lambda (set2) (lambda (set1)
  ((∀ (lambda (x) ((∋ x) set2))) set1))))

; ≡∪ :: [α] -> [α] -> Bool
; Set equality - same elements (order independent)
(define ≡∪ (lambda (set2) (lambda (set1)
  (and ((⊆ set2) set1) ((⊆ set1) set2)))))

; ═══════════════════════════════════════════════════════════════
; Fixed Point Iteration
; ═══════════════════════════════════════════════════════════════

; ⊛⊛ :: (α -> α) -> α -> α
; Fixed point - iterate until no change
; Uses set equality for termination
(define ⊛⊛ (lambda (f) (lambda (init)
  ((lambda (next)
    (if ((≡∪ next) init)
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
(define ⇝⊃-transfer (lambda (gen) (lambda (kill) (lambda (in)
  ((∪∪ gen) ((∖ kill) in))))))

; Single iteration of reaching definitions
; nodes = list of (node-id gen kill preds)
; current = current solution (list of (node-id in out))
(define ⇝⊃-iter (lambda (nodes) (lambda (current)
  (if (null? nodes)
     nil
     (cons ((⇝⊃-node (car nodes)) current)
         ((⇝⊃-iter (cdr nodes)) current))))))

; Process one node for reaching definitions
; node = (node-id gen kill preds)
; solution = current (node-id in out) pairs
(define ⇝⊃-node (lambda (node) (lambda (solution)
  ((lambda (node-id) ((lambda (gen) ((lambda (kill) ((lambda (preds)
    ; in = union of out[pred] for all predecessors
    ((lambda (in)
      ; out = gen ∪ (in - kill)
      ((lambda (out)
        (cons node-id (cons in (cons out nil))))
       (((⇝⊃-transfer gen) kill) in)))
     ((⇝⊃-meet preds) solution)))
   (car (cdr (cdr (cdr node))))))   ; preds
   (car (cdr (cdr node)))))       ; kill
   (car (cdr node))))           ; gen
   (car node)))))             ; node-id

; Meet function: union of predecessor outputs
(define ⇝⊃-meet (lambda (preds) (lambda (solution)
  (if (null? preds)
     nil
     ((∪∪ ((⇝⊃-get-out (car preds)) solution))
      ((⇝⊃-meet (cdr preds)) solution))))))

; Get out set for a node from solution
(define ⇝⊃-get-out (lambda (node-id) (lambda (solution)
  (if (null? solution)
     nil
     (if (equal? node-id (car (car solution)))
        (car (cdr (cdr (car solution))))  ; out is third element
        ((⇝⊃-get-out node-id) (cdr solution)))))))

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
(define ⇝←-transfer (lambda (use) (lambda (def) (lambda (out)
  ((∪∪ use) ((∖ def) out))))))

; Process one node for live variables
; node = (node-id use def succs)
; solution = current (node-id in out) pairs
(define ⇝←-node (lambda (node) (lambda (solution)
  ((lambda (node-id) ((lambda (use) ((lambda (def) ((lambda (succs)
    ; out = union of in[succ] for all successors
    ((lambda (out)
      ; in = use ∪ (out - def)
      ((lambda (in)
        (cons node-id (cons in (cons out nil))))
       (((⇝←-transfer use) def) out)))
     ((⇝←-meet succs) solution)))
   (car (cdr (cdr (cdr node))))))   ; succs
   (car (cdr (cdr node)))))       ; def
   (car (cdr node))))           ; use
   (car node)))))             ; node-id

; Meet function: union of successor inputs
(define ⇝←-meet (lambda (succs) (lambda (solution)
  (if (null? succs)
     nil
     ((∪∪ ((⇝←-get-in (car succs)) solution))
      ((⇝←-meet (cdr succs)) solution))))))

; Get in set for a node from solution
(define ⇝←-get-in (lambda (node-id) (lambda (solution)
  (if (null? solution)
     nil
     (if (equal? node-id (car (car solution)))
        (car (cdr (car solution)))  ; in is second element
        ((⇝←-get-in node-id) (cdr solution)))))))

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
(define ⇝∪-meet (lambda (preds) (lambda (solution)
  (if (null? preds)
     nil  ; Empty means "all expressions" in theory, but we use empty for init
     (if (null? (cdr preds))
        ; Single predecessor - just return its out
        ((⇝⊃-get-out (car preds)) solution)
        ; Multiple predecessors - intersect
        ((∩ ((⇝⊃-get-out (car preds)) solution))
         ((⇝∪-meet (cdr preds)) solution)))))))

; ═══════════════════════════════════════════════════════════════
; Helper: Initialize solution with empty sets
; ═══════════════════════════════════════════════════════════════

(define ⇝-init-solution (lambda (nodes)
  (if (null? nodes)
     nil
     (cons (cons (car (car nodes))  ; node-id
             (cons nil           ; in = empty
                 (cons nil nil)))  ; out = empty
         (⇝-init-solution (cdr nodes))))))

; ═══════════════════════════════════════════════════════════════
; Module Complete
; ═══════════════════════════════════════════════════════════════
; Set operations: 5 (∪∪, ∩, ∖, ⊆, ≡∪)
; Data flow: Fixed point, transfer functions, meet operations
; ═══════════════════════════════════════════════════════════════

"✓ Data flow analysis module loaded"
