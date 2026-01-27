; Test Suite: List Utilities
; Tests for stdlib/list_utilities.scm
; Simplified format with explicit definitions

; Load dependencies
(⋘ "../../stdlib/list.scm")
(⋘ "../../stdlib/list_utilities.scm")

; ============================================================================
; Helper Test Data
; ============================================================================

(≔ 𝕃1 (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅))))
(≔ 𝕃2 (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #5 (⟨⟩ #6 ∅)))))
(≔ 𝕃3 (⟨⟩ #10 (⟨⟩ #20 (⟨⟩ #30 ∅))))
(≔ 𝕃-dup (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #1 (⟨⟩ #3 (⟨⟩ #2 ∅))))))

; Predicates
(≔ lt3 (λ (x) (< x #3)))
(≔ lt5 (λ (x) (< x #5)))
(≔ gt0 (λ (x) (> x #0)))
(≔ gt5 (λ (x) (> x #5)))
(≔ eq5 (λ (x) (≡ x #5)))
(≔ even (λ (x) (≡ (% x #2) #0)))

; ============================================================================
; take-while Tests (5 tests)
; ============================================================================

(⊨ :take-while-basic
   (⟨⟩ #1 (⟨⟩ #2 ∅))
   ((take-while lt5) 𝕃2))

(⊨ :take-while-empty-list
   ∅
   ((take-while lt5) ∅))

(⊨ :take-while-none-match
   ∅
   ((take-while (λ (x) (< x #0))) 𝕃1))

(⊨ :take-while-all-match
   𝕃1
   ((take-while gt0) 𝕃1))

(⊨ :take-while-single
   (⟨⟩ #5 ∅)
   ((take-while (λ (x) #t)) (⟨⟩ #5 ∅)))

; ============================================================================
; drop-while Tests (5 tests)
; ============================================================================

(⊨ :drop-while-basic
   (⟨⟩ #5 (⟨⟩ #6 ∅))
   ((drop-while lt5) 𝕃2))

(⊨ :drop-while-empty-list
   ∅
   ((drop-while lt5) ∅))

(⊨ :drop-while-none-match
   𝕃1
   ((drop-while (λ (x) (< x #0))) 𝕃1))

(⊨ :drop-while-all-match
   ∅
   ((drop-while gt0) 𝕃1))

(⊨ :drop-while-single
   ∅
   ((drop-while (λ (x) #t)) (⟨⟩ #5 ∅)))

; ============================================================================
; span Tests (5 tests)
; ============================================================================

(≔ span-result1 ((span lt5) 𝕃2))
(⊨ :span-basic-first
   (⟨⟩ #1 (⟨⟩ #2 ∅))
   (◁ span-result1))

(⊨ :span-basic-second
   (⟨⟩ #5 (⟨⟩ #6 ∅))
   (▷ span-result1))

(≔ span-empty ((span lt5) ∅))
(⊨ :span-empty-first
   ∅
   (◁ span-empty))

(⊨ :span-empty-second
   ∅
   (▷ span-empty))

(≔ span-all ((span (λ (x) #t)) 𝕃1))
(⊨ :span-all-match
   𝕃1
   (◁ span-all))

; ============================================================================
; break Tests (4 tests)
; ============================================================================

(≔ break-result ((break eq5) 𝕃2))
(⊨ :break-basic-first
   (⟨⟩ #1 (⟨⟩ #2 ∅))
   (◁ break-result))

(⊨ :break-basic-second
   (⟨⟩ #5 (⟨⟩ #6 ∅))
   (▷ break-result))

(≔ break-never ((break (λ (x) #f)) 𝕃1))
(⊨ :break-never
   𝕃1
   (◁ break-never))

(≔ break-immediate ((break (λ (x) #t)) 𝕃1))
(⊨ :break-immediate
   ∅
   (◁ break-immediate))

; ============================================================================
; flatten Tests (5 tests)
; ============================================================================

(≔ nested (⟨⟩ (⟨⟩ #1 (⟨⟩ #2 ∅)) (⟨⟩ (⟨⟩ #3 (⟨⟩ #4 ∅)) ∅)))
(⊨ :flatten-basic
   (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 (⟨⟩ #4 ∅))))
   (flatten nested))

(⊨ :flatten-empty-outer
   ∅
   (flatten ∅))

(≔ empty-inner (⟨⟩ ∅ (⟨⟩ ∅ ∅)))
(⊨ :flatten-empty-inner
   ∅
   (flatten empty-inner))

(≔ single-inner (⟨⟩ (⟨⟩ #42 ∅) ∅))
(⊨ :flatten-single-inner
   (⟨⟩ #42 ∅)
   (flatten single-inner))

(≔ mixed-nested (⟨⟩ (⟨⟩ #1 ∅) (⟨⟩ (⟨⟩ #2 (⟨⟩ #3 ∅)) ∅)))
(⊨ :flatten-mixed
   (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅)))
   (flatten mixed-nested))

; ============================================================================
; distinct Tests (6 tests)
; ============================================================================

(⊨ :distinct-basic
   (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅)))
   (distinct 𝕃-dup))

(⊨ :distinct-no-duplicates
   𝕃1
   (distinct 𝕃1))

(≔ all-same (⟨⟩ #5 (⟨⟩ #5 (⟨⟩ #5 ∅))))
(⊨ :distinct-all-same
   (⟨⟩ #5 ∅)
   (distinct all-same))

(⊨ :distinct-empty
   ∅
   (distinct ∅))

(⊨ :distinct-single
   (⟨⟩ #42 ∅)
   (distinct (⟨⟩ #42 ∅)))

(≔ order-test (⟨⟩ #3 (⟨⟩ #1 (⟨⟩ #3 (⟨⟩ #2 (⟨⟩ #1 ∅))))))
(⊨ :distinct-preserves-order
   (⟨⟩ #3 (⟨⟩ #1 (⟨⟩ #2 ∅)))
   (distinct order-test))

; ============================================================================
; nth-or Tests (6 tests)
; ============================================================================

(⊨ :nth-or-first
   #10
   (((nth-or 𝕃3) #0) #999))

(⊨ :nth-or-middle
   #20
   (((nth-or 𝕃3) #1) #999))

(⊨ :nth-or-last
   #30
   (((nth-or 𝕃3) #2) #999))

(⊨ :nth-or-out-of-bounds
   #999
   (((nth-or 𝕃1) #5) #999))

(⊨ :nth-or-empty
   #999
   (((nth-or ∅) #0) #999))

; Note: Negative indices undefined behavior, but should return default
(⊨ :nth-or-negative-index
   #999
   (((nth-or 𝕃1) #-1) #999))

; ============================================================================
; head-or Tests (3 tests)
; ============================================================================

(⊨ :head-or-present
   #42
   ((head-or (⟨⟩ #42 (⟨⟩ #99 ∅))) #999))

(⊨ :head-or-empty
   #999
   ((head-or ∅) #999))

(⊨ :head-or-single
   #5
   ((head-or (⟨⟩ #5 ∅)) #999))

; ============================================================================
; tail-or Tests (3 tests)
; ============================================================================

(≔ default-tail (⟨⟩ #999 ∅))
(⊨ :tail-or-present
   (⟨⟩ #2 (⟨⟩ #3 ∅))
   ((tail-or 𝕃1) default-tail))

(⊨ :tail-or-empty
   default-tail
   ((tail-or ∅) default-tail))

(⊨ :tail-or-single
   ∅
   ((tail-or (⟨⟩ #1 ∅)) default-tail))

; ============================================================================
; all? Tests (4 tests)
; ============================================================================

(⊨ :all-true
   #t
   ((all? gt0) 𝕃1))

(≔ mixed-pos-neg (⟨⟩ #1 (⟨⟩ #-2 (⟨⟩ #3 ∅))))
(⊨ :all-false
   #f
   ((all? gt0) mixed-pos-neg))

(⊨ :all-empty
   #t
   ((all? (λ (x) #f)) ∅))

(⊨ :all-single-true
   #t
   ((all? eq5) (⟨⟩ #5 ∅)))

; ============================================================================
; any? Tests (4 tests)
; ============================================================================

(≔ with-10 (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #10 ∅))))
(⊨ :any-true
   #t
   ((any? gt5) with-10))

(⊨ :any-false
   #f
   ((any? (λ (x) (> x #10))) 𝕃1))

(⊨ :any-empty
   #f
   ((any? (λ (x) #t)) ∅))

(⊨ :any-single-true
   #t
   ((any? eq5) (⟨⟩ #5 ∅)))

; ============================================================================
; none? Tests (4 tests)
; ============================================================================

(⊨ :none-true
   #t
   ((none? (λ (x) (< x #0))) 𝕃1))

(≔ with-neg (⟨⟩ #-1 (⟨⟩ #2 ∅)))
(⊨ :none-false
   #f
   ((none? (λ (x) (< x #0))) with-neg))

(⊨ :none-empty
   #t
   ((none? (λ (x) #t)) ∅))

(⊨ :none-single-false
   #f
   ((none? eq5) (⟨⟩ #5 ∅)))

; ============================================================================
; replicate-at Tests (4 tests)
; ============================================================================

(⊨ :replicate-basic
   (⟨⟩ #42 (⟨⟩ #42 (⟨⟩ #42 ∅)))
   ((replicate-at #42) #3))

(⊨ :replicate-zero
   ∅
   ((replicate-at #42) #0))

(⊨ :replicate-one
   (⟨⟩ #5 ∅)
   ((replicate-at #5) #1))

(⊨ :replicate-negative
   ∅
   ((replicate-at #42) #-1))

; ============================================================================
; cycle-at Tests (5 tests)
; ============================================================================

(≔ cyc-input (⟨⟩ #1 (⟨⟩ #2 ∅)))
(⊨ :cycle-basic
   (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #1 (⟨⟩ #2 ∅))))
   ((cycle-at cyc-input) #2))

(⊨ :cycle-zero
   ∅
   ((cycle-at cyc-input) #0))

(⊨ :cycle-one
   cyc-input
   ((cycle-at cyc-input) #1))

(⊨ :cycle-single-element
   (⟨⟩ #5 (⟨⟩ #5 (⟨⟩ #5 ∅)))
   ((cycle-at (⟨⟩ #5 ∅)) #3))

(⊨ :cycle-negative
   ∅
   ((cycle-at cyc-input) #-1))

; ============================================================================
; Integration Tests (5 tests)
; ============================================================================

(≔ take-result ((take-while lt5) 𝕃2))
(≔ drop-result ((drop-while lt5) 𝕃2))
(⊨ :integration-take-drop
   𝕃2
   (⧺ take-result drop-result))

(≔ double-list (map (λ (x) ((replicate-at x) #2)) 𝕃1))
(⊨ :integration-flatten-replicate
   (⟨⟩ #1 (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #2 (⟨⟩ #3 (⟨⟩ #3 ∅))))))
   (flatten double-list))

(≔ nested-dups (⟨⟩ (⟨⟩ #1 (⟨⟩ #2 ∅)) (⟨⟩ (⟨⟩ #2 (⟨⟩ #3 ∅)) (⟨⟩ (⟨⟩ #1 ∅) ∅))))
(⊨ :integration-distinct-flatten
   (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅)))
   (distinct (flatten nested-dups)))

(≔ even-dups (⟨⟩ #2 (⟨⟩ #2 (⟨⟩ #4 (⟨⟩ #4 (⟨⟩ #5 ∅))))))
(≔ filtered (filter even even-dups))
(⊨ :integration-filter-distinct
   (⟨⟩ #2 (⟨⟩ #4 ∅))
   (distinct filtered))

(≔ test-combo (⟨⟩ #1 (⟨⟩ #3 (⟨⟩ #7 ∅))))
(≔ not-all-lt5 (¬ ((all? lt5) test-combo)))
(≔ has-gt5 ((any? gt5) test-combo))
(≔ no-negative ((none? (λ (x) (< x #0))) test-combo))
(⊨ :integration-all-any-none
   #t
   (∧ not-all-lt5 (∧ has-gt5 no-negative)))

; ============================================================================
; Summary
; ============================================================================

; Total tests: 71
; All tests use explicit definitions for clarity
; Functions tested: 13 (take-while, drop-while, span, break, flatten, distinct,
;                       nth-or, head-or, tail-or, all?, any?, none?,
;                       replicate-at, cycle-at)
