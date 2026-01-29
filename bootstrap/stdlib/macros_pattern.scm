; ═══════════════════════════════════════════════════════════════
; Guage Standard Library: Pattern-Based Macros
; ═══════════════════════════════════════════════════════════════
; Status: UPGRADED (Day 79)
; Created: 2026-01-29 (Day 76)
; Updated: 2026-01-29 (Day 79 - variadic ellipsis patterns)
; Purpose: Complex macros using pattern-based ⧉⊜ system
;
; Symbols defined:
; - ⇒* (cond) - Multi-branch conditional (unlimited clauses)
; - ≔⇊ (let*) - Sequential bindings (unlimited bindings)
; - ⇤ (case) - Value dispatch (unlimited cases)
; ═══════════════════════════════════════════════════════════════

; ═══════════════════════════════════════════════════════════════
; ⇒* (cond) - Multi-branch conditional (UNLIMITED CLAUSES)
; ═══════════════════════════════════════════════════════════════
; Syntax: (⇒* (cond1 result1) (cond2 result2) ...)
; Evaluates conditions in order, returns result of first true one
; Returns ∅ if none match
;
; Uses ellipsis pattern for unlimited clauses (Day 79)
; Use #t as final condition for else.
; ═══════════════════════════════════════════════════════════════

(⧉⊜ ⇒*
  ; Zero clauses - nil
  (() ∅)
  ; One clause
  ((($c $r)) (? $c $r ∅))
  ; Multiple clauses - recursive
  ((($c $r) $rest ...) (? $c $r (⇒* $rest ...))))

; ═══════════════════════════════════════════════════════════════
; ≔⇊ (let*) - Sequential bindings (UNLIMITED BINDINGS)
; ═══════════════════════════════════════════════════════════════
; Syntax: (≔⇊ ((var1 val1) (var2 val2) ...) body)
; Each binding can reference previous bindings
; Expands to nested lambdas
;
; Uses ellipsis pattern for unlimited bindings (Day 79)
; ═══════════════════════════════════════════════════════════════

(⧉⊜ ≔⇊
  ; Zero bindings - just return body
  ((() $body) $body)
  ; One binding
  (((($v $e)) $body)
   ((λ ($v) $body) $e))
  ; Multiple bindings - nest recursively
  (((($v $e) $rest ...) $body)
   ((λ ($v) (≔⇊ ($rest ...) $body)) $e)))

; ═══════════════════════════════════════════════════════════════
; ⇤ (case) - Value dispatch (UNLIMITED CASES)
; ═══════════════════════════════════════════════════════════════
; Syntax: (⇤ expr (val1 result1) (val2 result2) ... (:else default))
; Matches expr against literal values
; Uses :else keyword for default case
;
; Note: expr is evaluated once, then compared against each value
; Uses ellipsis pattern for unlimited cases (Day 79)
; ═══════════════════════════════════════════════════════════════

(⧉⊜ ⇤
  ; Just :else - always match (base case for recursion)
  (($expr (:else $def)) $def)
  ; Single case, no more clauses
  (($expr ($v $r))
   ((λ (:⇤tmp) (? (≡ :⇤tmp $v) $r ∅)) $expr))
  ; Multiple cases - recursive, pass captured tmp to avoid re-evaluation
  (($expr ($v $r) $rest ...)
   ((λ (:⇤tmp) (? (≡ :⇤tmp $v) $r (⇤ :⇤tmp $rest ...))) $expr)))

; ═══════════════════════════════════════════════════════════════
; Module Complete
; ═══════════════════════════════════════════════════════════════
; Pattern macros defined: 3
; - ⇒* (cond) - UNLIMITED clauses (Day 79)
; - ≔⇊ (let*) - UNLIMITED bindings (Day 79)
; - ⇤ (case) - UNLIMITED cases (Day 79)
; ═══════════════════════════════════════════════════════════════

"✓ 3 pattern-based stdlib macros loaded (variadic)"
