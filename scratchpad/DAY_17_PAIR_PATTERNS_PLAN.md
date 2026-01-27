; ============================================================
; Pattern Matching - Pair Patterns (Day 17)
; ============================================================
;
; Tests for pair pattern matching in ∇ (pattern match)
; Syntax: (∇ value (⌜ ((pattern₁ result₁) (pattern₂ result₂) ...)))
;
; Pair pattern: (⟨⟩ pat1 pat2)
; - Matches pairs (cons cells)
; - Recursively matches car and cdr
; - Binds variables from sub-patterns
;
; ============================================================

; ============================================================
; 1. Simple Pair Destructuring (5 tests)
; ============================================================

; Basic pair with two variables
(⊨ :pair-simple-vars
   #3
   (∇ (⟨⟩ #1 #2) (⌜ (((⟨⟩ x y) (⊕ x y))))))

; Pair with literal and variable
(⊨ :pair-literal-var
   #99
   (∇ (⟨⟩ #42 #99) (⌜ (((⟨⟩ #42 y) y)))))

; Pair with variable and literal
(⊨ :pair-var-literal
   #10
   (∇ (⟨⟩ #10 #5) (⌜ (((⟨⟩ x #5) x)))))

; Pair with wildcard and variable
(⊨ :pair-wildcard-var
   #7
   (∇ (⟨⟩ #3 #7) (⌜ (((⟨⟩ _ y) y)))))

; Pair with variable and wildcard
(⊨ :pair-var-wildcard
   #4
   (∇ (⟨⟩ #4 #9) (⌜ (((⟨⟩ x _) x)))))

; ============================================================
; 2. Pair Pattern Failures (3 tests)
; ============================================================

; Literal mismatch in first position
(⊨ :pair-fail-first
   #t
   (⚠? (∇ (⟨⟩ #1 #2) (⌜ (((⟨⟩ #42 y) y))))))

; Literal mismatch in second position
(⊨ :pair-fail-second
   #t
   (⚠? (∇ (⟨⟩ #1 #2) (⌜ (((⟨⟩ x #99) x))))))

; Pair pattern on non-pair value
(⊨ :pair-fail-not-pair
   #t
   (⚠? (∇ #42 (⌜ (((⟨⟩ x y) x))))))

; ============================================================
; 3. Nested Pairs (5 tests)
; ============================================================

; Left-nested pairs: ((a, b), c)
(⊨ :pair-nested-left
   #6
   (∇ (⟨⟩ (⟨⟩ #1 #2) #3) (⌜ (((⟨⟩ (⟨⟩ a b) c) (⊕ a (⊕ b c)))))))

; Right-nested pairs: (a, (b, c))
(⊨ :pair-nested-right
   #6
   (∇ (⟨⟩ #1 (⟨⟩ #2 #3)) (⌜ (((⟨⟩ a (⟨⟩ b c)) (⊕ a (⊕ b c)))))))

; Deep nesting: (((a, b), c), d)
(⊨ :pair-nested-deep
   #10
   (∇ (⟨⟩ (⟨⟩ (⟨⟩ #1 #2) #3) #4)
      (⌜ (((⟨⟩ (⟨⟩ (⟨⟩ a b) c) d) (⊕ (⊕ a b) (⊕ c d)))))))

; Mixed nested with literals
(⊨ :pair-nested-literals
   #5
   (∇ (⟨⟩ (⟨⟩ #1 #2) #3) (⌜ (((⟨⟩ (⟨⟩ #1 x) #3) (⊕ x #3))))))

; Nested with wildcards
(⊨ :pair-nested-wildcards
   #8
   (∇ (⟨⟩ (⟨⟩ #1 #2) #8) (⌜ (((⟨⟩ (⟨⟩ _ _) z) z)))))

; ============================================================
; 4. List Patterns (5 tests)
; ============================================================

; Single-element list: (x)
(⊨ :list-single
   #42
   (∇ (⟨⟩ #42 ∅) (⌜ (((⟨⟩ x ∅) x)))))

; Two-element list: (x, y)
(⊨ :list-two
   #7
   (∇ (⟨⟩ #3 (⟨⟩ #4 ∅)) (⌜ (((⟨⟩ x (⟨⟩ y ∅)) (⊕ x y))))))

; Three-element list: (x, y, z)
(⊨ :list-three
   #15
   (∇ (⟨⟩ #4 (⟨⟩ #5 (⟨⟩ #6 ∅)))
      (⌜ (((⟨⟩ x (⟨⟩ y (⟨⟩ z ∅))) (⊕ x (⊕ y z)))))))

; Extract head and tail: (h . t)
(⊨ :list-head-tail
   #3
   (∇ (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅))) (⌜ (((⟨⟩ h t) h)))))

; Check tail is a list
(⊨ :list-tail-check
   #t
   (∇ (⟨⟩ #1 (⟨⟩ #2 ∅)) (⌜ (((⟨⟩ h t) (⟨⟩? t))))))

; ============================================================
; 5. Computations with Pair Bindings (5 tests)
; ============================================================

; Sum of pair elements
(⊨ :pair-sum
   #15
   (∇ (⟨⟩ #7 #8) (⌜ (((⟨⟩ x y) (⊕ x y))))))

; Product of pair elements
(⊨ :pair-product
   #24
   (∇ (⟨⟩ #4 #6) (⌜ (((⟨⟩ x y) (⊗ x y))))))

; Comparison
(⊨ :pair-compare
   #f
   (∇ (⟨⟩ #10 #5) (⌜ (((⟨⟩ x y) (< x y))))))

; Nested computation
(⊨ :pair-nested-compute
   #100
   (∇ (⟨⟩ (⟨⟩ #5 #10) #2)
      (⌜ (((⟨⟩ (⟨⟩ a b) c) (⊗ (⊕ a b) c))))))

; Conditional with pair
(⊨ :pair-conditional
   #20
   (∇ (⟨⟩ #10 #20) (⌜ (((⟨⟩ x y) (? (> x y) x y))))))

; ============================================================
; 6. Multiple Clauses with Pairs (3 tests)
; ============================================================

; First clause matches
(⊨ :pair-multi-first
   #100
   (∇ (⟨⟩ #42 #58) (⌜ (((⟨⟩ #42 y) (⊕ #42 y)) ((⟨⟩ x y) (⊗ x y))))))

; Second clause matches
(⊨ :pair-multi-second
   #200
   (∇ (⟨⟩ #10 #20) (⌜ (((⟨⟩ #42 y) (⊕ #42 y)) ((⟨⟩ x y) (⊗ x y))))))

; No clause matches
(⊨ :pair-multi-none
   #t
   (⚠? (∇ (⟨⟩ #10 #20) (⌜ (((⟨⟩ #42 y) y) ((⟨⟩ #99 z) z))))))

; ============================================================
; 7. Edge Cases (3 tests)
; ============================================================

; Nil vs pair pattern
(⊨ :pair-edge-nil
   #t
   (⚠? (∇ ∅ (⌜ (((⟨⟩ x y) x))))))

; Atom vs pair pattern
(⊨ :pair-edge-atom
   #t
   (⚠? (∇ #42 (⌜ (((⟨⟩ x y) x))))))

; Boolean vs pair pattern
(⊨ :pair-edge-bool
   #t
   (⚠? (∇ #t (⌜ (((⟨⟩ x y) x))))))

; ============================================================
; Total: 29 tests
; ============================================================
