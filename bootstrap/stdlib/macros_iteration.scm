; ═══════════════════════════════════════════════════════════════
; Guage Standard Library: Iteration Macros
; ═══════════════════════════════════════════════════════════════
; Status: NEW (Day 81)
; Created: 2026-01-29 (Day 81)
; Purpose: Iteration and sequencing constructs
;
; Symbols defined:
; - ⊎ (begin/progn) - Sequence expressions, return last
; - ⊲* (for-each)   - Iterate with side effects (returns nil)
; - ⟳ (dotimes)     - Repeat body n times
; - ⊎↦ (list-comp)  - List comprehension
; - ⊎⊲ (filter-comp) - Filter comprehension
; - ⟳← (reduce)     - Fold with cleaner syntax
; ═══════════════════════════════════════════════════════════════

; ═══════════════════════════════════════════════════════════════
; ⊎ (begin/progn) - Sequence expressions (UNLIMITED)
; ═══════════════════════════════════════════════════════════════
; Syntax: (⊎ expr1 expr2 ...)
; Evaluates all expressions in order, returns last result
; Essential for side-effecting code in expression context
;
; (⊎ (⟲ :start) (do-work) (⟲ :end) :result)  ; → :result
; ═══════════════════════════════════════════════════════════════

(⧉⊜ ⊎
  ; Single expression - just return it
  (($a) $a)
  ; Multiple expressions - eval first, then recurse
  (($a $rest ...) ((λ (_) (⊎ $rest ...)) $a)))

; ═══════════════════════════════════════════════════════════════
; ⊲* (for-each) - Iterate with side effects
; ═══════════════════════════════════════════════════════════════
; Syntax: (⊲* func list)
; Applies func to each element for side effects
; Returns ∅ (unlike map which collects results)
;
; (⊲* (λ (x) (⟲ x)) (⟨⟩ :a (⟨⟩ :b ∅)))  ; prints :a, :b, returns ∅
; ═══════════════════════════════════════════════════════════════

(⧉⊜ ⊲*
  (($f $lst)
   ((λ (:⊲*-helper)
      (:⊲*-helper :⊲*-helper $lst))
    (λ (:self :xs)
       (? (∅? :xs)
          ∅
          (⊎ ($f (◁ :xs))
             (:self :self (▷ :xs))))))))

; ═══════════════════════════════════════════════════════════════
; ⟳ (dotimes) - Repeat body n times
; ═══════════════════════════════════════════════════════════════
; Syntax: (⟳ n body)
; Evaluates body n times, returns ∅
;
; (⟳ #5 (⟲ :tick))  ; prints :tick 5 times
; ═══════════════════════════════════════════════════════════════

(⧉⊜ ⟳
  (($n $body)
   ((λ (:⟳-helper)
      (:⟳-helper :⟳-helper $n))
    (λ (:self :count)
       (? (≤ :count #0)
          ∅
          (⊎ $body
             (:self :self (⊖ :count #1))))))))

; ═══════════════════════════════════════════════════════════════
; ⊎↦ (list-comp) - List comprehension
; ═══════════════════════════════════════════════════════════════
; Syntax: (⊎↦ body-expr (var list))
; Creates a new list by applying body-expr to each element
; Variable var is bound to each element in turn
;
; (⊎↦ (⊗ :x #2) (:x (⟨⟩ #1 (⟨⟩ #2 ∅))))  ; → ⟨#2 ⟨#4 ∅⟩⟩
; ═══════════════════════════════════════════════════════════════

(⧉⊜ ⊎↦
  (($body ($var $lst))
   ((λ (:⊎↦-helper)
      (:⊎↦-helper :⊎↦-helper $lst))
    (λ (:self :xs)
       (? (∅? :xs)
          ∅
          (⟨⟩ ((λ ($var) $body) (◁ :xs))
              (:self :self (▷ :xs))))))))

; ═══════════════════════════════════════════════════════════════
; ⊎⊲ (filter-comp) - Filter comprehension
; ═══════════════════════════════════════════════════════════════
; Syntax: (⊎⊲ predicate (var list))
; Keeps elements where predicate is true
; Variable var is bound to each element in turn
;
; (⊎⊲ (> :x #3) (:x (⟨⟩ #1 (⟨⟩ #5 ∅))))  ; → ⟨#5 ∅⟩
; ═══════════════════════════════════════════════════════════════

(⧉⊜ ⊎⊲
  (($pred ($var $lst))
   ((λ (:⊎⊲-helper)
      (:⊎⊲-helper :⊎⊲-helper $lst))
    (λ (:self :xs)
       (? (∅? :xs)
          ∅
          (? ((λ ($var) $pred) (◁ :xs))
             (⟨⟩ (◁ :xs) (:self :self (▷ :xs)))
             (:self :self (▷ :xs))))))))

; ═══════════════════════════════════════════════════════════════
; ⟳← (reduce) - Fold with cleaner syntax
; ═══════════════════════════════════════════════════════════════
; Syntax: (⟳← body-expr init (var list))
; Folds list with body-expr, where :acc is accumulator
; body-expr has access to :acc (accumulator) and var (element)
;
; (⟳← (⊕ :acc :x) #0 (:x (⟨⟩ #1 (⟨⟩ #2 ∅))))  ; → #3
; ═══════════════════════════════════════════════════════════════

(⧉⊜ ⟳←
  (($body $init ($var $lst))
   ((λ (:⟳←-helper)
      (:⟳←-helper :⟳←-helper $init $lst))
    (λ (:self :acc :xs)
       (? (∅? :xs)
          :acc
          (:self :self
                 ((λ ($var) $body) (◁ :xs))
                 (▷ :xs)))))))

; ═══════════════════════════════════════════════════════════════
; Module Complete
; ═══════════════════════════════════════════════════════════════
; Iteration macros defined: 6
; - ⊎ (begin)       - UNLIMITED expressions
; - ⊲* (for-each)   - Side-effect iteration
; - ⟳ (dotimes)     - Repeat n times
; - ⊎↦ (list-comp)  - List comprehension
; - ⊎⊲ (filter-comp) - Filter comprehension
; - ⟳← (reduce)     - Fold with syntax
; ═══════════════════════════════════════════════════════════════

"✓ 6 iteration macros loaded"
