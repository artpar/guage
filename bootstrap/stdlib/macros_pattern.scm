; ═══════════════════════════════════════════════════════════════
; Guage Standard Library: Pattern-Based Macros
; ═══════════════════════════════════════════════════════════════
; Status: UPGRADED (Day 79)
; Created: 2026-01-29 (Day 76)
; Updated: 2026-01-29 (Day 79 - variadic ellipsis patterns)
; Purpose: Complex macros using pattern-based macro-rules system
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
; Returns nil if none match
;
; Uses ellipsis pattern for unlimited clauses (Day 79)
; Use #t as final condition for else.
; ═══════════════════════════════════════════════════════════════

(macro-rules ⇒*
  ; Zero clauses - nil
  (() nil)
  ; One clause
  ((($c $r)) (if $c $r nil))
  ; Multiple clauses - recursive
  ((($c $r) $rest ...) (if $c $r (⇒* $rest ...))))

; ═══════════════════════════════════════════════════════════════
; ≔⇊ (let*) - Sequential bindings (UNLIMITED BINDINGS)
; ═══════════════════════════════════════════════════════════════
; Syntax: (≔⇊ ((var1 val1) (var2 val2) ...) body)
; Each binding can reference previous bindings
; Expands to nested lambdas
;
; Uses ellipsis pattern for unlimited bindings (Day 79)
; ═══════════════════════════════════════════════════════════════

(macro-rules ≔⇊
  ; Zero bindings - just return body
  ((() $body) $body)
  ; One binding
  (((($v $e)) $body)
   ((lambda ($v) $body) $e))
  ; Multiple bindings - nest recursively
  (((($v $e) $rest ...) $body)
   ((lambda ($v) (≔⇊ ($rest ...) $body)) $e)))

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

(macro-rules ⇤
  ; Just :else - always match (base case for recursion)
  (($expr (:else $def)) $def)
  ; Single case, no more clauses
  (($expr ($v $r))
   ((lambda (:⇤tmp) (if (equal? :⇤tmp $v) $r nil)) $expr))
  ; Multiple cases - recursive, pass captured tmp to avoid re-evaluation
  (($expr ($v $r) $rest ...)
   ((lambda (:⇤tmp) (if (equal? :⇤tmp $v) $r (⇤ :⇤tmp $rest ...))) $expr)))

; ═══════════════════════════════════════════════════════════════
; Module Complete
; ═══════════════════════════════════════════════════════════════
; Pattern macros defined: 3
; - ⇒* (cond) - UNLIMITED clauses (Day 79)
; - ≔⇊ (let*) - UNLIMITED bindings (Day 79)
; - ⇤ (case) - UNLIMITED cases (Day 79)
; ═══════════════════════════════════════════════════════════════

"✓ 3 pattern-based stdlib macros loaded (variadic)"
