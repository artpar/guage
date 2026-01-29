; ═══════════════════════════════════════════════════════════════
; Guage Standard Library: Control Flow Macros
; ═══════════════════════════════════════════════════════════════
; Status: NEW
; Created: 2026-01-29 (Day 77)
; Purpose: Short-circuit logical operators and control flow
;
; Symbols defined:
; - ∧* (and*) - Short-circuit AND (1-4 args)
; - ∨* (or*)  - Short-circuit OR (1-4 args)
; - ⇒ (when)  - Conditional execution
; - ⇏ (unless) - Negative conditional
; ═══════════════════════════════════════════════════════════════

; ═══════════════════════════════════════════════════════════════
; ∧* (and*) - Short-circuit AND
; ═══════════════════════════════════════════════════════════════
; Syntax: (∧* expr1 expr2 ...)
; Evaluates expressions left-to-right
; Returns first falsy value, or last value if all truthy
; Short-circuits: stops evaluating after first #f
;
; Lisp semantics: returns actual values, not just booleans
; (∧* #t #42) → #42
; (∧* #f anything) → #f (anything not evaluated)
; ═══════════════════════════════════════════════════════════════

(⧉⊜ ∧*
  ; Single arg - identity
  (($a)
   $a)
  ; Two args - short-circuit
  (($a $b)
   (? $a $b #f))
  ; Three args
  (($a $b $c)
   (? $a (? $b $c #f) #f))
  ; Four args
  (($a $b $c $d)
   (? $a (? $b (? $c $d #f) #f) #f)))

; ═══════════════════════════════════════════════════════════════
; ∨* (or*) - Short-circuit OR
; ═══════════════════════════════════════════════════════════════
; Syntax: (∨* expr1 expr2 ...)
; Evaluates expressions left-to-right
; Returns first truthy value, or last value if all falsy
; Short-circuits: stops evaluating after first truthy
;
; Lisp semantics: returns actual values, not just booleans
; (∨* #f #42) → #42
; (∨* #42 anything) → #42 (anything not evaluated)
; ═══════════════════════════════════════════════════════════════

(⧉⊜ ∨*
  ; Single arg - identity
  (($a)
   $a)
  ; Two args - short-circuit
  ; Need to avoid double evaluation of $a
  ; Test for "not #f" rather than "truthy" (Lisp semantics)
  ; (? (≡ val #f) else val) - if val is #f, go else; otherwise return val
  (($a $b)
   ((λ (:∨tmp) (? (≡ :∨tmp #f) $b :∨tmp)) $a))
  ; Three args
  (($a $b $c)
   ((λ (:∨tmp) (? (≡ :∨tmp #f)
     ((λ (:∨tmp2) (? (≡ :∨tmp2 #f) $c :∨tmp2)) $b)
     :∨tmp)) $a))
  ; Four args
  (($a $b $c $d)
   ((λ (:∨tmp) (? (≡ :∨tmp #f)
     ((λ (:∨tmp2) (? (≡ :∨tmp2 #f)
       ((λ (:∨tmp3) (? (≡ :∨tmp3 #f) $d :∨tmp3)) $c)
       :∨tmp2)) $b)
     :∨tmp)) $a)))

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
; - ∧* (and*) - 4 arities, short-circuit
; - ∨* (or*)  - 4 arities, short-circuit
; - ⇒ (when)  - 1 arity
; - ⇏ (unless) - 1 arity
; ═══════════════════════════════════════════════════════════════

"✓ 4 control flow macros loaded"
