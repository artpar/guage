; ═══════════════════════════════════════════════════════════════
; Guage Standard Library: Macros
; ═══════════════════════════════════════════════════════════════
; Status: CURRENT
; Created: 2026-01-27 (Day 34)
; Purpose: Core macros for control flow, bindings, and functional programming
;
; All names are PURELY SYMBOLIC - no English words!
; Parameter naming: Single-character Unicode mathematical letters only
; ═══════════════════════════════════════════════════════════════

; ═══════════════════════════════════════════════════════════════
; Phase 1: Control Flow Macros
; ═══════════════════════════════════════════════════════════════

; unless - Unless (inverse conditional)
; If condition is false, execute body, else return nil
; Usage: (unless condition body)
; Example: (unless (< #5 #3) #42)  ; -> #42 (since condition is false)
(macro unless (𝕩 𝕪)
  (quasiquote-tilde (if (~ 𝕩) nil (~ 𝕪))))

; and-all - Short-circuit logical AND (binary for now)
; Evaluates arguments left to right, returns first false or last true
; Usage: (and-all expr₁ expr₂)
; Example: (and-all (> 𝕩 #0) (< 𝕩 #10))
(macro and-all (𝕩 𝕪)
  (quasiquote-tilde (if (~ 𝕩) (~ 𝕪) #f)))

; or-all - Short-circuit logical OR (binary for now)
; Evaluates arguments left to right, returns first true or last false
; Usage: (or-all expr₁ expr₂)
; Example: (or-all (< 𝕩 #0) (> 𝕩 #100))
(macro or-all (𝕩 𝕪)
  (quasiquote-tilde (if (~ 𝕩) #t (~ 𝕪))))

; thread-first - Thread-first pipeline
; Threads value through a function
; Usage: (thread-first value function)
; Example: (thread-first #5 (lambda (𝕩) (+ 𝕩 #1)))  ; -> 6
(macro thread-first (𝕩 𝕗)
  (quasiquote-tilde ((~ 𝕗) (~ 𝕩))))

; ═══════════════════════════════════════════════════════════════
; Phase 2: Binding Macros
; ═══════════════════════════════════════════════════════════════

; let-local - Local binding (let)
; Creates local scope with single binding
; Usage: (let-local 𝕩 value body)
; Note: Must use 𝕩 as variable name in body
; Example: (let-local 𝕩 #42 (+ 𝕩 #1))  ; -> #43
(macro let-local (𝕧 𝕨 𝕓)
  (quasiquote-tilde ((lambda (𝕩) (~ 𝕓)) (~ 𝕨))))

; letrec-local - Recursive bindings (letrec) - LIMITATION!
; Creates local scope where binding can reference itself
; Usage: (letrec-local 𝕗 value body)
; Note: Must use 𝕗 as variable name in body
;
; LIMITATION: Simple macro expansion cannot create true recursive bindings.
; The function cannot reference itself by name within its own definition.
; For recursive functions, use define (global definition) instead.
;
; This macro is kept for API completeness but has limited utility.
(macro letrec-local (𝕧 𝕨 𝕓)
  (quasiquote-tilde ((lambda (𝕗) (~ 𝕓)) (~ 𝕨))))

; ═══════════════════════════════════════════════════════════════
; Phase 3: Functional Macros
; ═══════════════════════════════════════════════════════════════

; compose - Function composition (standard mathematical composition)
; Composes two functions: (compose 𝕗 𝕘) = λx. 𝕗(𝕘(x))
; Usage: (compose fn₁ fn₂)
; Example: ((compose (lambda (𝕩) (* 𝕩 #2)) (lambda (𝕩) (+ 𝕩 #1))) #5)  ; -> 12
(macro compose (𝕗 𝕘)
  (quasiquote-tilde (lambda (𝕩) ((~ 𝕗) ((~ 𝕘) 𝕩)))))

; partial - Partial application (freeze left argument)
; Creates new function with first argument fixed
; Usage: (partial fn arg)
; Example: ((partial + #10) #5)  ; -> 15
(macro partial (𝕗 𝕩)
  (quasiquote-tilde (lambda (𝕪) ((~ 𝕗) (~ 𝕩) 𝕪))))

; flip - Flip arguments (swap parameter order)
; Swaps first two arguments of a binary function
; Usage: (flip fn)
; Example: ((flip -) #5 #10)  ; -> 5 (normally - #5 #10 = -5)
(macro flip (𝕗)
  (quasiquote-tilde (lambda (𝕩 𝕪) ((~ 𝕗) 𝕪 𝕩))))

; ═══════════════════════════════════════════════════════════════
; Module Complete
; ═══════════════════════════════════════════════════════════════
; Macros defined: 8
; - Control flow: 4 (?¬, ∧…, ∨…, thread-first)
; - Bindings: 2 (≔↓, letrec-local)
; - Functional: 3 (∘, ⊰, flip)
;
; Note: ⇒* (cond) and ≔⇊ (let*) omitted for now - they require more
; complex list processing that needs additional helper functions
; ═══════════════════════════════════════════════════════════════

"✓ 8 stdlib macros loaded"
