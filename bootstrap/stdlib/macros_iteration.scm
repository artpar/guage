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
; - actor-spawn (dotimes)     - Repeat body n times
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
; (⊎ (trace :start) (do-work) (trace :end) :result)  ; -> :result
; ═══════════════════════════════════════════════════════════════

(macro-rules ⊎
  ; Single expression - just return it
  (($a) $a)
  ; Multiple expressions - eval first, then recurse
  (($a $rest ...) ((lambda (_) (⊎ $rest ...)) $a)))

; ═══════════════════════════════════════════════════════════════
; ⊲* (for-each) - Iterate with side effects
; ═══════════════════════════════════════════════════════════════
; Syntax: (⊲* func list)
; Applies func to each element for side effects
; Returns nil (unlike map which collects results)
;
; (⊲* (lambda (x) (trace x)) (cons :a (cons :b nil)))  ; prints :a, :b, returns nil
; ═══════════════════════════════════════════════════════════════

(macro-rules ⊲*
  (($f $lst)
   ((lambda (:⊲*-helper)
      (:⊲*-helper :⊲*-helper $lst))
    (lambda (:self :xs)
       (if (null? :xs)
          nil
          (⊎ ($f (car :xs))
             (:self :self (cdr :xs))))))))

; ═══════════════════════════════════════════════════════════════
; actor-spawn (dotimes) - Repeat body n times
; ═══════════════════════════════════════════════════════════════
; Syntax: (actor-spawn n body)
; Evaluates body n times, returns nil
;
; (actor-spawn #5 (trace :tick))  ; prints :tick 5 times
; ═══════════════════════════════════════════════════════════════

(macro-rules actor-spawn
  (($n $body)
   ((lambda (:⟳-helper)
      (:⟳-helper :⟳-helper $n))
    (lambda (:self :count)
       (if (<= :count #0)
          nil
          (⊎ $body
             (:self :self (- :count #1))))))))

; ═══════════════════════════════════════════════════════════════
; ⊎↦ (list-comp) - List comprehension
; ═══════════════════════════════════════════════════════════════
; Syntax: (⊎↦ body-expr (var list))
; Creates a new list by applying body-expr to each element
; Variable var is bound to each element in turn
;
; (⊎↦ (* :x #2) (:x (cons #1 (cons #2 nil))))  ; -> ⟨#2 ⟨#4 ∅⟩⟩
; ═══════════════════════════════════════════════════════════════

(macro-rules ⊎↦
  (($body ($var $lst))
   ((lambda (:⊎↦-helper)
      (:⊎↦-helper :⊎↦-helper $lst))
    (lambda (:self :xs)
       (if (null? :xs)
          nil
          (cons ((lambda ($var) $body) (car :xs))
              (:self :self (cdr :xs))))))))

; ═══════════════════════════════════════════════════════════════
; ⊎⊲ (filter-comp) - Filter comprehension
; ═══════════════════════════════════════════════════════════════
; Syntax: (⊎⊲ predicate (var list))
; Keeps elements where predicate is true
; Variable var is bound to each element in turn
;
; (⊎⊲ (> :x #3) (:x (cons #1 (cons #5 nil))))  ; -> ⟨#5 ∅⟩
; ═══════════════════════════════════════════════════════════════

(macro-rules ⊎⊲
  (($pred ($var $lst))
   ((lambda (:⊎⊲-helper)
      (:⊎⊲-helper :⊎⊲-helper $lst))
    (lambda (:self :xs)
       (if (null? :xs)
          nil
          (if ((lambda ($var) $pred) (car :xs))
             (cons (car :xs) (:self :self (cdr :xs)))
             (:self :self (cdr :xs))))))))

; ═══════════════════════════════════════════════════════════════
; ⟳← (reduce) - Fold with cleaner syntax
; ═══════════════════════════════════════════════════════════════
; Syntax: (⟳← body-expr init (var list))
; Folds list with body-expr, where :acc is accumulator
; body-expr has access to :acc (accumulator) and var (element)
;
; (⟳← (+ :acc :x) #0 (:x (cons #1 (cons #2 nil))))  ; -> #3
; ═══════════════════════════════════════════════════════════════

(macro-rules ⟳←
  (($body $init ($var $lst))
   ((lambda (:⟳←-helper)
      (:⟳←-helper :⟳←-helper $init $lst))
    (lambda (:self :acc :xs)
       (if (null? :xs)
          :acc
          (:self :self
                 ((lambda ($var) $body) (car :xs))
                 (cdr :xs)))))))

; ═══════════════════════════════════════════════════════════════
; Module Complete
; ═══════════════════════════════════════════════════════════════
; Iteration macros defined: 6
; - ⊎ (begin)       - UNLIMITED expressions
; - ⊲* (for-each)   - Side-effect iteration
; - actor-spawn (dotimes)     - Repeat n times
; - ⊎↦ (list-comp)  - List comprehension
; - ⊎⊲ (filter-comp) - Filter comprehension
; - ⟳← (reduce)     - Fold with syntax
; ═══════════════════════════════════════════════════════════════

"✓ 6 iteration macros loaded"
