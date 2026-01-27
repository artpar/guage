;; Test suite for ⌞ (eval) primitive
;; Tests the metaprogramming foundation: evaluate quoted expressions

;; ============================================================
;; Phase 1: Self-Evaluating Forms
;; ============================================================

;; Numbers
(⊨ :eval-number-literal
   #42
   (⌞ (⌜ #42)))

(⊨ :eval-number-zero
   #0
   (⌞ (⌜ #0)))

(⊨ :eval-number-negative
   #-5
   (⌞ (⌜ #-5)))

;; Booleans
(⊨ :eval-bool-true
   #t
   (⌞ (⌜ #t)))

(⊨ :eval-bool-false
   #f
   (⌞ (⌜ #f)))

;; Nil
(⊨ :eval-nil
   ∅
   (⌞ (⌜ ∅)))

;; Keywords (self-evaluating symbols)
(⊨ :eval-keyword-simple
   :test
   (⌞ (⌜ :test)))

(⊨ :eval-keyword-type
   :Point
   (⌞ (⌜ :Point)))

(⊨ :eval-keyword-field
   :x
   (⌞ (⌜ :x)))

;; ============================================================
;; Phase 2: Primitive Operations
;; ============================================================

;; Arithmetic
(⊨ :eval-add
   #3
   (⌞ (⌜ (⊕ #1 #2))))

(⊨ :eval-multiply
   #12
   (⌞ (⌜ (⊗ #3 #4))))

(⊨ :eval-subtract
   #5
   (⌞ (⌜ (⊖ #10 #5))))

(⊨ :eval-divide
   #5
   (⌞ (⌜ (⊘ #10 #2))))

(⊨ :eval-modulo
   #1
   (⌞ (⌜ (% #10 #3))))

;; Comparisons
(⊨ :eval-equal
   #t
   (⌞ (⌜ (≡ #5 #5))))

(⊨ :eval-not-equal
   #t
   (⌞ (⌜ (≢ #5 #10))))

(⊨ :eval-less-than
   #t
   (⌞ (⌜ (< #5 #10))))

(⊨ :eval-greater-than
   #t
   (⌞ (⌜ (> #10 #5))))

(⊨ :eval-less-equal
   #t
   (⌞ (⌜ (≤ #5 #5))))

(⊨ :eval-greater-equal
   #t
   (⌞ (⌜ (≥ #10 #5))))

;; Logic
(⊨ :eval-and-true
   #t
   (⌞ (⌜ (∧ #t #t))))

(⊨ :eval-and-false
   #f
   (⌞ (⌜ (∧ #t #f))))

(⊨ :eval-or-true
   #t
   (⌞ (⌜ (∨ #f #t))))

(⊨ :eval-or-false
   #f
   (⌞ (⌜ (∨ #f #f))))

(⊨ :eval-not-true
   #f
   (⌞ (⌜ (¬ #t))))

(⊨ :eval-not-false
   #t
   (⌞ (⌜ (¬ #f))))

;; Lists
(⊨ :eval-cons
   #t
   (⟨⟩? (⌞ (⌜ (⟨⟩ #1 #2)))))

(⊨ :eval-car
   #1
   (⌞ (⌜ (◁ (⟨⟩ #1 #2)))))

(⊨ :eval-cdr
   #2
   (⌞ (⌜ (▷ (⟨⟩ #1 #2)))))

;; ============================================================
;; Phase 3: Conditionals
;; ============================================================

;; Simple conditionals
(⊨ :eval-if-true
   #1
   (⌞ (⌜ (? #t #1 #2))))

(⊨ :eval-if-false
   #2
   (⌞ (⌜ (? #f #1 #2))))

;; Conditionals with evaluation
(⊨ :eval-if-with-expr
   #42
   (⌞ (⌜ (? (≡ #1 #1) #42 #0))))

(⊨ :eval-if-with-comparison
   :yes
   (⌞ (⌜ (? (< #5 #10) :yes :no))))

(⊨ :eval-if-false-branch
   :no
   (⌞ (⌜ (? (> #5 #10) :yes :no))))

;; Nested conditionals
(⊨ :eval-nested-if
   #3
   (⌞ (⌜ (? #t (? #f #1 #3) #2))))

;; ============================================================
;; Phase 4: Variables
;; ============================================================

;; Define and eval variable
(≔ eval-test-x #42)
(⊨ :eval-variable-lookup
   #42
   (⌞ (⌜ eval-test-x)))

;; Eval with variable in expression
(⊨ :eval-variable-in-expr
   #43
   (⌞ (⌜ (⊕ eval-test-x #1))))

;; Multiple variables
(≔ eval-test-y #10)
(⊨ :eval-multiple-variables
   #420
   (⌞ (⌜ (⊗ eval-test-x eval-test-y))))

;; ============================================================
;; Phase 5: Lambdas and Application
;; ============================================================

;; Lambda returns a lambda (verify it's not nil and not an error)
(⊨ :eval-lambda-creation
   #t
   (¬ (∅? (⌞ (⌜ (λ (x) x))))))

;; Simple lambda application
(⊨ :eval-lambda-identity
   #5
   (⌞ (⌜ ((λ (x) x) #5))))

(⊨ :eval-lambda-add
   #6
   (⌞ (⌜ ((λ (x) (⊕ x #1)) #5))))

;; Lambda with multiple args
(⊨ :eval-lambda-two-args
   #15
   (⌞ (⌜ ((λ (x y) (⊕ x y)) #5 #10))))

;; Higher-order functions
(≔ eval-test-twice (λ (f) (λ (x) (f (f x)))))
(≔ eval-test-inc (λ (x) (⊕ x #1)))

(⊨ :eval-higher-order
   #7
   (⌞ (⌜ ((eval-test-twice eval-test-inc) #5))))

;; ============================================================
;; Phase 6: Nested Quote/Eval
;; ============================================================

;; Double quote/eval
(⊨ :eval-nested-simple
   #42
   (⌞ (⌞ (⌜ (⌜ #42)))))

;; Quote an eval expression
(⊨ :eval-quoted-eval
   #t
   (⟨⟩? (⌜ (⌞ (⌜ #42)))))

;; ============================================================
;; Phase 7: Error Handling
;; ============================================================

;; Undefined variable
(⊨ :eval-undefined-variable
   #t
   (⚠? (⌞ (⌜ undefined-variable-xyz))))

;; ============================================================
;; Phase 8: Auto-Generated Tests (Integration)
;; ============================================================

;; Generate tests for a primitive
(≔ eval-add-tests (⌂⊨ (⌜ ⊕)))

;; Verify tests were generated
(⊨ :eval-tests-generated
   #t
   (⟨⟩? eval-add-tests))

;; Execute first test (should pass)
(⊨ :eval-execute-generated-test
   #t
   (⌞ (◁ eval-add-tests)))

;; ============================================================
;; Summary
;; ============================================================

;; Print summary
(⟲ (⌜ :eval-tests-complete))
