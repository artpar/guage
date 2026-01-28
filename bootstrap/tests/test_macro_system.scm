; Test suite for macro system (Day 33)
; =========================================

; Test 1: Simple macro definition
; ⧉ should return the macro name
(⊨ :macro-def-simple :when (⧉ when (c b) (⌞̃ (? (~ c) (~ b) ∅))))

; Test 2: Macro usage - when with true condition
(⧉ when (condition body) (⌞̃ (? (~ condition) (~ body) ∅)))
(⊨ :when-true #42 (when #t #42))

; Test 3: Macro usage - when with false condition
(⊨ :when-false ∅ (when #f #42))

; Test 4: Macro with computation
(⊨ :when-with-computation #10 (when #t (⊕ #5 #5)))

; Test 5: Let-style binding macro
(⧉ let (var value body) (⌞̃ ((λ (x) (~ body)) (~ value))))
(⊨ :let-binding #43 (let x #42 (⊕ x #1)))

; Test 6: Let with complex computation
(⊨ :let-complex #15 (let x (⊕ #5 #5) (⊕ x #5)))

; Test 7: Unless macro (opposite of when)
(⧉ unless (condition body) (⌞̃ (? (~ condition) ∅ (~ body))))
(⊨ :unless-false #42 (unless #f #42))

; Test 8: Unless with true condition
(⊨ :unless-true ∅ (unless #t #42))

; Test 9: Multi-parameter macro
(⧉ add3 (a b c) (⌞̃ (⊕ (~ a) (⊕ (~ b) (~ c)))))
(⊨ :add3 #6 (add3 #1 #2 #3))

; Test 10: Macro with nested operations
(⧉ double-sum (x y) (⌞̃ (⊗ #2 (⊕ (~ x) (~ y)))))
(⊨ :double-sum #20 (double-sum #5 #5))

; Test 11: Nested macro calls - macro uses result of another macro
(⧉ twice (expr) (⌞̃ (⊕ (~ expr) (~ expr))))
(⊨ :nested-twice #84 (twice (twice #21)))

; Test 12: Macro calling other macros
(⧉ if-positive (x then else) (⌞̃ (? (> (~ x) #0) (~ then) (~ else))))
(⊨ :macro-calls-macro #5 (if-positive #3 #5 #0))
(⊨ :macro-calls-macro-neg #0 (if-positive (⊖ #0 #3) #5 #0))

; Test 13: Macro with conditional expansion
(⧉ safe-div (x y) (⌞̃ (? (≡ (~ y) #0) (⚠ :div-by-zero ∅) (⊘ (~ x) (~ y)))))
(⊨ :safe-div-normal #5 (safe-div #10 #2))
(⊨ :safe-div-error #t (⚠? (safe-div #10 #0)))

; Test 14: Macro redefining macro
(⧉ test-redef (x) (⌞̃ (⊕ (~ x) #1)))
(⊨ :test-redef-v1 #6 (test-redef #5))
(⧉ test-redef (x) (⌞̃ (⊗ (~ x) #2)))
(⊨ :test-redef-v2 #10 (test-redef #5))

; Test 15: Macro with multiple expansions in sequence
(⧉ inc (x) (⌞̃ (⊕ (~ x) #1)))
(⊨ :inc-chain #7 (inc (inc (inc #4))))

; ===== Additional advanced tests =====

; Test 16: Macro generating comparisons
(⧉ between (x lo hi) (⌞̃ (∧ (> (~ x) (~ lo)) (< (~ x) (~ hi)))))
(⊨ :between-true #t (between #5 #0 #10))
(⊨ :between-false #f (between #15 #0 #10))

; Test 17: Macro for pair operations
(⧉ first-of (pair) (⌞̃ (◁ (~ pair))))
(⧉ second-of (pair) (⌞̃ (▷ (~ pair))))
(⊨ :first-of #1 (first-of (⟨⟩ #1 #2)))
(⊨ :second-of #2 (second-of (⟨⟩ #1 #2)))

; Test 18: Macro with side effects (printing)
(⧉ debug (x) (⌞̃ (⟲ (~ x))))
(⊨ :debug-returns-value #42 (debug #42))

; Test 19: Macro with assertion
(⧉ assert-positive (x) (⌞̃ (⊢ (> (~ x) #0) :not-positive)))
(⊨ :assert-positive-ok #t (assert-positive #5))
(⊨ :assert-positive-fail #t (⚠? (assert-positive (⊖ #0 #5))))

; Test 20: Complex macro - conditional binding
(⧉ when-let (var test then else)
  (⌞̃ (? (~ test)
        ((λ (x) (~ then)) (~ test))
        (~ else))))
(⊨ :when-let-true #11 (when-let x #t (⊕ #10 #1) #99))

; Summary
(≋ "✓ 20 macro system tests")
