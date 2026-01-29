; ═══════════════════════════════════════════════════════════════
; Guage Standard Library: Pattern-Based Macros
; ═══════════════════════════════════════════════════════════════
; Status: NEW
; Created: 2026-01-29 (Day 76)
; Purpose: Complex macros using pattern-based ⧉⊜ system
;
; Symbols defined:
; - ⇒* (cond) - Multi-branch conditional
; - ≔⇊ (let*) - Sequential bindings
; - ⇤ (case) - Value dispatch
; ═══════════════════════════════════════════════════════════════

; ═══════════════════════════════════════════════════════════════
; ⇒* (cond) - Multi-branch conditional
; ═══════════════════════════════════════════════════════════════
; Syntax: (⇒* (cond1 result1) (cond2 result2) ...)
; Evaluates conditions in order, returns result of first true one
; Returns ∅ if none match
;
; Supports 1-5 clauses. Use #t as final condition for else.
; ═══════════════════════════════════════════════════════════════

(⧉⊜ ⇒*
  ; One clause
  ((($c1 $r1))
   (? $c1 $r1 ∅))
  ; Two clauses
  ((($c1 $r1) ($c2 $r2))
   (? $c1 $r1 (? $c2 $r2 ∅)))
  ; Three clauses
  ((($c1 $r1) ($c2 $r2) ($c3 $r3))
   (? $c1 $r1 (? $c2 $r2 (? $c3 $r3 ∅))))
  ; Four clauses
  ((($c1 $r1) ($c2 $r2) ($c3 $r3) ($c4 $r4))
   (? $c1 $r1 (? $c2 $r2 (? $c3 $r3 (? $c4 $r4 ∅)))))
  ; Five clauses
  ((($c1 $r1) ($c2 $r2) ($c3 $r3) ($c4 $r4) ($c5 $r5))
   (? $c1 $r1 (? $c2 $r2 (? $c3 $r3 (? $c4 $r4 (? $c5 $r5 ∅)))))))

; ═══════════════════════════════════════════════════════════════
; ≔⇊ (let*) - Sequential bindings
; ═══════════════════════════════════════════════════════════════
; Syntax: (≔⇊ ((var1 val1) (var2 val2) ...) body)
; Each binding can reference previous bindings
; Expands to nested lambdas
;
; Supports 1-4 bindings.
; ═══════════════════════════════════════════════════════════════

(⧉⊜ ≔⇊
  ; One binding
  (((($v1 $e1)) $body)
   ((λ ($v1) $body) $e1))
  ; Two bindings - second can reference first
  (((($v1 $e1) ($v2 $e2)) $body)
   ((λ ($v1) ((λ ($v2) $body) $e2)) $e1))
  ; Three bindings
  (((($v1 $e1) ($v2 $e2) ($v3 $e3)) $body)
   ((λ ($v1) ((λ ($v2) ((λ ($v3) $body) $e3)) $e2)) $e1))
  ; Four bindings
  (((($v1 $e1) ($v2 $e2) ($v3 $e3) ($v4 $e4)) $body)
   ((λ ($v1) ((λ ($v2) ((λ ($v3) ((λ ($v4) $body) $e4)) $e3)) $e2)) $e1)))

; ═══════════════════════════════════════════════════════════════
; ⇤ (case) - Value dispatch
; ═══════════════════════════════════════════════════════════════
; Syntax: (⇤ expr (val1 result1) (val2 result2) ... (:else default))
; Matches expr against literal values
; Uses :else keyword for default case
;
; Note: expr is evaluated once, then compared against each value
; Supports 2-5 cases (including :else)
; ═══════════════════════════════════════════════════════════════

(⧉⊜ ⇤
  ; Two cases (one match + else)
  (($expr ($v1 $r1) (:else $def))
   ((λ (:⇤tmp) (? (≡ :⇤tmp $v1) $r1 $def)) $expr))
  ; Two cases without else
  (($expr ($v1 $r1) ($v2 $r2))
   ((λ (:⇤tmp) (? (≡ :⇤tmp $v1) $r1 (? (≡ :⇤tmp $v2) $r2 ∅))) $expr))
  ; Three cases (two match + else)
  (($expr ($v1 $r1) ($v2 $r2) (:else $def))
   ((λ (:⇤tmp) (? (≡ :⇤tmp $v1) $r1 (? (≡ :⇤tmp $v2) $r2 $def))) $expr))
  ; Three cases without else
  (($expr ($v1 $r1) ($v2 $r2) ($v3 $r3))
   ((λ (:⇤tmp) (? (≡ :⇤tmp $v1) $r1 (? (≡ :⇤tmp $v2) $r2 (? (≡ :⇤tmp $v3) $r3 ∅)))) $expr))
  ; Four cases (three match + else)
  (($expr ($v1 $r1) ($v2 $r2) ($v3 $r3) (:else $def))
   ((λ (:⇤tmp) (? (≡ :⇤tmp $v1) $r1 (? (≡ :⇤tmp $v2) $r2 (? (≡ :⇤tmp $v3) $r3 $def)))) $expr)))

; ═══════════════════════════════════════════════════════════════
; Module Complete
; ═══════════════════════════════════════════════════════════════
; Pattern macros defined: 3
; - ⇒* (cond) - 5 arities
; - ≔⇊ (let*) - 4 arities
; - ⇤ (case) - 5 arities
; ═══════════════════════════════════════════════════════════════

"✓ 3 pattern-based stdlib macros loaded"
