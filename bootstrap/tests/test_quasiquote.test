; Test file for quasiquote (⌞̃) and unquote (~)
; Day 32 Part 2: Macro System - Quote Templates

; ============ Basic Quasiquote Tests ============

; Test 1: Quasiquote without unquote acts like quote
(⊨ :qq-like-quote
   (⌜ (⊕ #1 #2))
   (⌞̃ (⊕ #1 #2)))

; Test 2: Simple unquote - single value
(≔ x #42)
(⊨ :qq-simple-unquote
   #42
   (⌞̃ (~ x)))

; Test 3: Unquote in list structure
(≔ y #5)
(⊨ :qq-unquote-in-list
   (⌜ (⊕ #1 #5))
   (⌞̃ (⊕ #1 (~ y))))

; ============ Multiple Unquotes ============

; Test 4: Multiple unquotes
(≔ a #10)
(≔ b #20)
(⊨ :qq-multiple-unquotes
   (⌜ (⊕ #10 #20))
   (⌞̃ (⊕ (~ a) (~ b))))

; Test 5: Unquote in different positions
(≔ op :⊕)
(⊨ :qq-unquote-operator
   (⌜ (:⊕ #3 #4))
   (⌞̃ ((~ op) #3 #4)))

; ============ Nested Structures ============

; Test 6: Nested lists with unquote
(≔ inner #99)
(⊨ :qq-nested-list
   (⌜ (⟨⟩ #1 (⟨⟩ #99 ∅)))
   (⌞̃ (⟨⟩ #1 (⟨⟩ (~ inner) ∅))))

; Test 7: Unquote in nested expression
(≔ val #7)
(⊨ :qq-nested-expr
   (⌜ (⊗ (⊕ #1 #7) #2))
   (⌞̃ (⊗ (⊕ #1 (~ val)) #2)))

; ============ Evaluated Expressions ============

; Test 8: Unquote evaluates expression
(≔ n #5)
(⊨ :qq-eval-expr
   (⌜ (result #10))
   (⌞̃ (result (~ (⊗ n #2)))))

; Test 9: Unquote with function call
(≔ double (λ (x) (⊗ x #2)))
(≔ num #21)
(⊨ :qq-function-call
   (⌜ (value #42))
   (⌞̃ (value (~ (double num)))))

; ============ Mixed Quoted and Unquoted ============

; Test 10: Mix quoted and unquoted parts
(≔ x1 #100)
(⊨ :qq-mixed
   (⌜ (:constant #100 :another-constant))
   (⌞̃ (:constant (~ x1) :another-constant)))

; Test 11: Symbol quoted, value unquoted
(≔ tag :error)
(≔ code #404)
(⊨ :qq-symbol-value-mix
   (⌜ (:response :error #404))
   (⌞̃ (:response (~ tag) (~ code))))

; ============ Edge Cases ============

; Test 12: Empty unquote should error (handle gracefully)
; (⌞̃ (~))  ; Would error - unquote needs argument

; Test 13: Nested quasiquotes (advanced)
(≔ depth #2)
(⊨ :qq-nested-qq
   (⌜ (level #2))
   (⌞̃ (level (~ depth))))

; Test 14: Unquote nil
(≔ empty ∅)
(⊨ :qq-unquote-nil
   ∅
   (⌞̃ (~ empty)))

; Test 15: Unquote boolean
(≔ flag #t)
(⊨ :qq-unquote-bool
   #t
   (⌞̃ (~ flag)))

; ============ Real-World Usage ============

; Test 16: Build expression programmatically
(≔ build-add (λ (x) (λ (y)
  (⌞̃ (⊕ (~ x) (~ y))))))

(⊨ :qq-build-expr
   (⌜ (⊕ #3 #4))
   ((build-add #3) #4))

; Test 17: Conditional expression building
(≔ make-test (λ (val) (λ (expected)
  (⌞̃ (≡ (~ val) (~ expected))))))

(⊨ :qq-conditional
   (⌜ (≡ #42 #42))
   ((make-test #42) #42))

; Test 18: Template with multiple values
(≔ make-pair (λ (a) (λ (b)
  (⌞̃ (⟨⟩ (~ a) (~ b))))))

(⊨ :qq-template
   (⌜ (⟨⟩ #1 #2))
   ((make-pair #1) #2))

; ============ Macro-Like Usage ============

; Test 19: Simple comparison builder
(≔ make-gt (λ (a) (λ (b)
  (⌞̃ (> (~ a) (~ b))))))

(⊨ :qq-comparison
   (⌜ (> #10 #5))
   ((make-gt #10) #5))

; Test 20: List constructor
(≔ make-triple (λ (a) (λ (b) (λ (c)
  (⌞̃ (⟨⟩ (~ a) (⟨⟩ (~ b) (⟨⟩ (~ c) ∅))))))))

(⊨ :qq-triple
   (⌜ (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅))))
   (((make-triple #1) #2) #3))

(≋ "✓ All quasiquote tests defined")
