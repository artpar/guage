; ═══════════════════════════════════════════════════════════════
; Guage Standard Library: Control Flow Macros
; ═══════════════════════════════════════════════════════════════
; Status: UPGRADED (Day 79)
; Created: 2026-01-29 (Day 77)
; Updated: 2026-01-29 (Day 79 - variadic ellipsis patterns)
; Purpose: Short-circuit logical operators and control flow
;
; Symbols defined:
; - ∧* (and*) - Short-circuit AND (unlimited args)
; - ∨* (or*)  - Short-circuit OR (unlimited args)
; - ⇒ (when)  - Conditional execution
; - ⇏ (unless) - Negative conditional
; ═══════════════════════════════════════════════════════════════

; ═══════════════════════════════════════════════════════════════
; ∧* (and*) - Short-circuit AND (UNLIMITED ARGS)
; ═══════════════════════════════════════════════════════════════
; Syntax: (∧* expr1 expr2 ...)
; Evaluates expressions left-to-right
; Returns first falsy value, or last value if all truthy
; Short-circuits: stops evaluating after first #f
;
; Lisp semantics: returns actual values, not just booleans
; (∧* #t #42) → #42
; (∧* #f anything) → #f (anything not evaluated)
;
; Uses ellipsis pattern for unlimited arity (Day 79)
; ═══════════════════════════════════════════════════════════════

(⧉⊜ ∧*
  ; Zero args - vacuous truth
  (() #t)
  ; Single arg - identity
  (($a) $a)
  ; Multiple args - short-circuit recursively
  (($a $rest ...) (? $a (∧* $rest ...) #f)))

; ═══════════════════════════════════════════════════════════════
; ∨* (or*) - Short-circuit OR (UNLIMITED ARGS)
; ═══════════════════════════════════════════════════════════════
; Syntax: (∨* expr1 expr2 ...)
; Evaluates expressions left-to-right
; Returns first truthy value, or last value if all falsy
; Short-circuits: stops evaluating after first truthy
;
; Lisp semantics: returns actual values, not just booleans
; (∨* #f #42) → #42
; (∨* #42 anything) → #42 (anything not evaluated)
;
; Uses ellipsis pattern for unlimited arity (Day 79)
; ═══════════════════════════════════════════════════════════════

(⧉⊜ ∨*
  ; Zero args - vacuous false
  (() #f)
  ; Single arg - identity
  (($a) $a)
  ; Multiple args - short-circuit recursively
  ; Use lambda to avoid double evaluation, check for "not #f" (Lisp semantics)
  (($a $rest ...) ((λ (:∨tmp) (? (≡ :∨tmp #f) (∨* $rest ...) :∨tmp)) $a)))

; ═══════════════════════════════════════════════════════════════
; ⇒ (when) - Conditional execution
; ═══════════════════════════════════════════════════════════════
; Syntax: (⇒ condition body)
; If condition is truthy, evaluates and returns body
; If condition is falsy, returns ∅ (nil)
; Body is not evaluated if condition is false
; ═══════════════════════════════════════════════════════════════

(⧉⊜ ⇒
  (($cond $body)
   (? $cond $body ∅)))

; ═══════════════════════════════════════════════════════════════
; ⇏ (unless) - Negative conditional
; ═══════════════════════════════════════════════════════════════
; Syntax: (⇏ condition body)
; If condition is falsy, evaluates and returns body
; If condition is truthy, returns ∅ (nil)
; Body is not evaluated if condition is true
; ═══════════════════════════════════════════════════════════════

(⧉⊜ ⇏
  (($cond $body)
   (? $cond ∅ $body)))

; ═══════════════════════════════════════════════════════════════
; Module Complete
; ═══════════════════════════════════════════════════════════════
; Control macros defined: 4
; - ∧* (and*) - UNLIMITED arity, short-circuit (Day 79)
; - ∨* (or*)  - UNLIMITED arity, short-circuit (Day 79)
; - ⇒ (when)  - 1 arity
; - ⇏ (unless) - 1 arity
; ═══════════════════════════════════════════════════════════════

"✓ 4 control flow macros loaded (variadic)"
