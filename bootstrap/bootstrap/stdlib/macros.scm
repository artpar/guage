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

; ?¬ - Unless (inverse conditional)
; If condition is false, execute body, else return nil
; Usage: (?¬ condition body)
; Example: (?¬ (< #5 #3) #42)  ; → #42 (since condition is false)
(⧉ ?¬ (𝕩 𝕪)
  (⌞̃ (? (~ 𝕩) ∅ (~ 𝕪))))

; ∧… - Short-circuit logical AND (binary for now)
; Evaluates arguments left to right, returns first false or last true
; Usage: (∧… expr₁ expr₂)
; Example: (∧… (> 𝕩 #0) (< 𝕩 #10))
(⧉ ∧… (𝕩 𝕪)
  (⌞̃ (? (~ 𝕩) (~ 𝕪) #f)))

; ∨… - Short-circuit logical OR (binary for now)
; Evaluates arguments left to right, returns first true or last false
; Usage: (∨… expr₁ expr₂)
; Example: (∨… (< 𝕩 #0) (> 𝕩 #100))
(⧉ ∨… (𝕩 𝕪)
  (⌞̃ (? (~ 𝕩) #t (~ 𝕪))))

; ⊳→ - Thread-first pipeline
; Threads value through a function
; Usage: (⊳→ value function)
; Example: (⊳→ #5 (λ (𝕩) (⊕ 𝕩 #1)))  ; → 6
(⧉ ⊳→ (𝕩 𝕗)
  (⌞̃ ((~ 𝕗) (~ 𝕩))))

; ═══════════════════════════════════════════════════════════════
; Phase 2: Binding Macros
; ═══════════════════════════════════════════════════════════════

; ≔↓ - Local binding (let)
; Creates local scope with single binding
; Usage: (≔↓ 𝕩 value body)
; Note: Must use 𝕩 as variable name in body
; Example: (≔↓ 𝕩 #42 (⊕ 𝕩 #1))  ; → #43
(⧉ ≔↓ (𝕧 𝕨 𝕓)
  (⌞̃ ((λ (𝕩) (~ 𝕓)) (~ 𝕨))))

; ≔↻ - Recursive bindings (letrec) - LIMITATION!
; Creates local scope where binding can reference itself
; Usage: (≔↻ 𝕗 value body)
; Note: Must use 𝕗 as variable name in body
;
; LIMITATION: Simple macro expansion cannot create true recursive bindings.
; The function cannot reference itself by name within its own definition.
; For recursive functions, use ≔ (global definition) instead.
;
; This macro is kept for API completeness but has limited utility.
(⧉ ≔↻ (𝕧 𝕨 𝕓)
  (⌞̃ ((λ (𝕗) (~ 𝕓)) (~ 𝕨))))

; ═══════════════════════════════════════════════════════════════
; Phase 3: Functional Macros
; ═══════════════════════════════════════════════════════════════

; ∘ - Function composition (standard mathematical composition)
; Composes two functions: (∘ 𝕗 𝕘) = λx. 𝕗(𝕘(x))
; Usage: (∘ fn₁ fn₂)
; Example: ((∘ (λ (𝕩) (⊗ 𝕩 #2)) (λ (𝕩) (⊕ 𝕩 #1))) #5)  ; → 12
(⧉ ∘ (𝕗 𝕘)
  (⌞̃ (λ (𝕩) ((~ 𝕗) ((~ 𝕘) 𝕩)))))

; ⊰ - Partial application (freeze left argument)
; Creates new function with first argument fixed
; Usage: (⊰ fn arg)
; Example: ((⊰ ⊕ #10) #5)  ; → 15
(⧉ ⊰ (𝕗 𝕩)
  (⌞̃ (λ (𝕪) ((~ 𝕗) (~ 𝕩) 𝕪))))

; ↔ - Flip arguments (swap parameter order)
; Swaps first two arguments of a binary function
; Usage: (↔ fn)
; Example: ((↔ ⊖) #5 #10)  ; → 5 (normally ⊖ #5 #10 = -5)
(⧉ ↔ (𝕗)
  (⌞̃ (λ (𝕩 𝕪) ((~ 𝕗) 𝕪 𝕩))))

; ═══════════════════════════════════════════════════════════════
; Module Complete
; ═══════════════════════════════════════════════════════════════
; Macros defined: 8
; - Control flow: 4 (?¬, ∧…, ∨…, ⊳→)
; - Bindings: 2 (≔↓, ≔↻)
; - Functional: 3 (∘, ⊰, ↔)
;
; Note: ⇒* (cond) and ≔⇊ (let*) omitted for now - they require more
; complex list processing that needs additional helper functions
; ═══════════════════════════════════════════════════════════════

"✓ 8 stdlib macros loaded"
