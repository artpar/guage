; Simple pattern matching test with quoted clauses

; Test 1: Wildcard pattern (simplest)
; Syntax: (∇ value (quote ((pattern . result) ...)))
(⊨ :wildcard-quoted
   :ok
   (∇ #42 (⌜ (((_ . :ok))))))

; Test 2: Literal pattern
(⊨ :literal-quoted
   :matched
   (∇ #42 (⌜ (((#42 . :matched))))))

; Test 3: Variable pattern
(⊨ :variable-quoted
   #42
   (∇ #42 (⌜ (((x . x))))))
