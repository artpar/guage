; ============================================================
; Pattern Matching - Variable Patterns (Day 16)
; ============================================================
;
; Tests for variable binding patterns in ∇ (pattern match)
; Syntax: (∇ value (⌜ ((pattern₁ result₁) (pattern₂ result₂) ...)))
;
; Pattern types:
; - Variable: x, n, value (binds the matched value)
; - Can be used in result expression
;
; ============================================================

; Simple variable bindings
(⊨ :var-simple-number
   #42
   (∇ #42 (⌜ ((x x)))))

(⊨ :var-simple-bool
   #t
   (∇ #t (⌜ ((b b)))))

(⊨ :var-simple-symbol
   :hello
   (∇ :hello (⌜ ((s s)))))

(⊨ :var-simple-nil
   ∅
   (∇ ∅ (⌜ ((n n)))))

; Variable used in computation
(⊨ :var-double-number
   #10
   (∇ #5 (⌜ ((n (⊗ n #2))))))

(⊨ :var-increment
   #43
   (∇ #42 (⌜ ((n (⊕ n #1))))))

(⊨ :var-compare
   #t
   (∇ #10 (⌜ ((n (> n #5))))))

; Multiple clauses with variables
(⊨ :var-multi-first
   #100
   (∇ #50 (⌜ ((x (⊗ x #2)) (y (⊗ y #3))))))

(⊨ :var-multi-second
   #150
   (∇ #50 (⌜ ((#99 #0) (y (⊗ y #3))))))

; Variable with wildcard fallback
(⊨ :var-with-fallback-match
   #84
   (∇ #42 (⌜ ((x (⊗ x #2)) (_ #0)))))

; Variable with literal fallback
(⊨ :var-with-literal-fallback
   #10
   (∇ #5 (⌜ ((#42 #999) (n (⊗ n #2))))))

; Nested computation with variable
(⊨ :var-nested-calc
   #25
   (∇ #5 (⌜ ((n (⊗ n n))))))

; Variable binds pairs
(⊨ :var-binds-pair
   (⟨⟩ #1 #2)
   (∇ (⟨⟩ #1 #2) (⌜ ((p p)))))

; Variable binds error
(⊨ :var-binds-error
   #t
   (⚠? (∇ (⚠ :test #42) (⌜ ((e e))))))

; Different variable names
(⊨ :var-name-alpha
   #42
   (∇ #42 (⌜ ((alpha alpha)))))

(⊨ :var-name-value
   #42
   (∇ #42 (⌜ ((value value)))))

(⊨ :var-name-result
   #42
   (∇ #42 (⌜ ((result result)))))

; Edge case: single-char variable name
(⊨ :var-name-single-char
   #42
   (∇ #42 (⌜ ((z z)))))

; Variable should NOT match keywords
; (Keywords like :x should be literals, not variables)
(⊨ :keyword-not-variable-match
   :ok
   (∇ :x (⌜ ((:x :ok)))))

(⊨ :keyword-not-variable-nomatch
   #t
   (⚠? (∇ :y (⌜ ((:x :ok))))))

; Variable should work with nested expressions
(⊨ :var-nested-expr
   #7
   (∇ #3 (⌜ ((n (⊕ (⊗ n #2) #1))))))

; Zero special case
(⊨ :var-zero
   #0
   (∇ #0 (⌜ ((n n)))))

; Negative number
(⊨ :var-negative
   #-5
   (∇ #-5 (⌜ ((n n)))))

; Variable with conditional in result
(⊨ :var-with-conditional
   :positive
   (∇ #5 (⌜ ((n (? (> n #0) :positive :negative))))))

(⊨ :var-with-conditional-negative
   :negative
   (∇ #-5 (⌜ ((n (? (> n #0) :positive :negative))))))
