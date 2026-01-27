; Test suite for math utilities
; Tests for: ⊕⊕ ⊗⊗ ↥ ↧ ↥↥ ↧↧

; ============================================================================
; Load required functions
; ============================================================================

; ⊕← :: (α → β → α) → α → [β] → α
; Fold-left - accumulate from left to right
(≔ ⊕← (λ (ƒ) (λ (acc) (λ (lst)
  (? (∅? lst)
     acc
     (((⊕← ƒ) ((ƒ acc) (◁ lst))) (▷ lst)))))))

; ============================================================================
; Load math functions
; ============================================================================

; ⊕⊕ :: [ℕ] → ℕ
; Sum - sum of all numbers in list
(≔ ⊕⊕ (λ (lst)
  (((⊕← (λ (a) (λ (b) (⊕ a b)))) #0) lst)))

; ⊗⊗ :: [ℕ] → ℕ
; Product - product of all numbers in list
(≔ ⊗⊗ (λ (lst)
  (((⊕← (λ (a) (λ (b) (⊗ a b)))) #1) lst)))

; ↥ :: ℕ → ℕ → ℕ
; Max - maximum of two numbers
(≔ ↥ (λ (b) (λ (a)
  (? (> a b) a b))))

; ↧ :: ℕ → ℕ → ℕ
; Min - minimum of two numbers
(≔ ↧ (λ (b) (λ (a)
  (? (< a b) a b))))

; ↥↥ :: [ℕ] → ℕ | ∅
; Maximum - maximum value in list (returns ∅ if empty)
(≔ ↥↥ (λ (lst)
  (? (∅? lst)
     ∅
     (((⊕← (λ (a) (λ (b) ((↥ b) a)))) (◁ lst)) (▷ lst)))))

; ↧↧ :: [ℕ] → ℕ | ∅
; Minimum - minimum value in list (returns ∅ if empty)
(≔ ↧↧ (λ (lst)
  (? (∅? lst)
     ∅
     (((⊕← (λ (a) (λ (b) ((↧ b) a)))) (◁ lst)) (▷ lst)))))

; ============================================================================
; Test ⊕⊕ (sum)
; ============================================================================

; Test 1: Sum of small list
(⊨ :sum-small
   #6
   (⊕⊕ (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅)))))

; Test 2: Sum of single element
(⊨ :sum-single
   #42
   (⊕⊕ (⟨⟩ #42 ∅)))

; Test 3: Sum of empty list
(⊨ :sum-empty
   #0
   (⊕⊕ ∅))

; Test 4: Sum with zeros
(⊨ :sum-zeros
   #10
   (⊕⊕ (⟨⟩ #0 (⟨⟩ #5 (⟨⟩ #0 (⟨⟩ #5 ∅))))))

; Test 5: Sum of larger list
(⊨ :sum-larger
   #55
   (⊕⊕ (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 (⟨⟩ #4 (⟨⟩ #5 (⟨⟩ #6 (⟨⟩ #7 (⟨⟩ #8 (⟨⟩ #9 (⟨⟩ #10 ∅))))))))))))

; ============================================================================
; Test ⊗⊗ (product)
; ============================================================================

; Test 6: Product of small list
(⊨ :product-small
   #24
   (⊗⊗ (⟨⟩ #2 (⟨⟩ #3 (⟨⟩ #4 ∅)))))

; Test 7: Product of single element
(⊨ :product-single
   #7
   (⊗⊗ (⟨⟩ #7 ∅)))

; Test 8: Product of empty list
(⊨ :product-empty
   #1
   (⊗⊗ ∅))

; Test 9: Product with zero
(⊨ :product-zero
   #0
   (⊗⊗ (⟨⟩ #5 (⟨⟩ #0 (⟨⟩ #3 ∅)))))

; Test 10: Product with one
(⊨ :product-one
   #6
   (⊗⊗ (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅)))))

; ============================================================================
; Test ↥ (max - binary)
; ============================================================================

; Test 11: Max where first is larger
(⊨ :max-first-larger
   #10
   ((↥ #5) #10))

; Test 12: Max where second is larger
(⊨ :max-second-larger
   #10
   ((↥ #10) #5))

; Test 13: Max where equal
(⊨ :max-equal
   #7
   ((↥ #7) #7))

; Test 14: Max with zero
(⊨ :max-zero
   #5
   ((↥ #0) #5))

; Test 15: Max both zero
(⊨ :max-both-zero
   #0
   ((↥ #0) #0))

; ============================================================================
; Test ↧ (min - binary)
; ============================================================================

; Test 16: Min where first is smaller
(⊨ :min-first-smaller
   #5
   ((↧ #10) #5))

; Test 17: Min where second is smaller
(⊨ :min-second-smaller
   #5
   ((↧ #5) #10))

; Test 18: Min where equal
(⊨ :min-equal
   #7
   ((↧ #7) #7))

; Test 19: Min with zero
(⊨ :min-zero
   #0
   ((↧ #5) #0))

; Test 20: Min both zero
(⊨ :min-both-zero
   #0
   ((↧ #0) #0))

; ============================================================================
; Test ↥↥ (maximum of list)
; ============================================================================

; Test 21: Maximum of list
(⊨ :maximum-normal
   #9
   (↥↥ (⟨⟩ #3 (⟨⟩ #7 (⟨⟩ #2 (⟨⟩ #9 (⟨⟩ #1 ∅)))))))

; Test 22: Maximum of single element
(⊨ :maximum-single
   #42
   (↥↥ (⟨⟩ #42 ∅)))

; Test 23: Maximum of empty list
(⊨ :maximum-empty
   ∅
   (↥↥ ∅))

; Test 24: Maximum with duplicates
(⊨ :maximum-duplicates
   #5
   (↥↥ (⟨⟩ #5 (⟨⟩ #3 (⟨⟩ #5 (⟨⟩ #1 ∅))))))

; Test 25: Maximum at start
(⊨ :maximum-at-start
   #10
   (↥↥ (⟨⟩ #10 (⟨⟩ #5 (⟨⟩ #3 ∅)))))

; Test 26: Maximum at end
(⊨ :maximum-at-end
   #10
   (↥↥ (⟨⟩ #3 (⟨⟩ #5 (⟨⟩ #10 ∅)))))

; ============================================================================
; Test ↧↧ (minimum of list)
; ============================================================================

; Test 27: Minimum of list
(⊨ :minimum-normal
   #1
   (↧↧ (⟨⟩ #3 (⟨⟩ #7 (⟨⟩ #2 (⟨⟩ #9 (⟨⟩ #1 ∅)))))))

; Test 28: Minimum of single element
(⊨ :minimum-single
   #42
   (↧↧ (⟨⟩ #42 ∅)))

; Test 29: Minimum of empty list
(⊨ :minimum-empty
   ∅
   (↧↧ ∅))

; Test 30: Minimum with duplicates
(⊨ :minimum-duplicates
   #1
   (↧↧ (⟨⟩ #5 (⟨⟩ #1 (⟨⟩ #5 (⟨⟩ #1 ∅))))))

; Test 31: Minimum at start
(⊨ :minimum-at-start
   #2
   (↧↧ (⟨⟩ #2 (⟨⟩ #5 (⟨⟩ #10 ∅)))))

; Test 32: Minimum at end
(⊨ :minimum-at-end
   #2
   (↧↧ (⟨⟩ #10 (⟨⟩ #5 (⟨⟩ #2 ∅)))))

; ============================================================================
; Integration Tests
; ============================================================================

; Test 33: Sum and product relationship
(⊨ :integration-sum-product
   #t
   (> (⊗⊗ (⟨⟩ #2 (⟨⟩ #3 (⟨⟩ #4 ∅))))
      (⊕⊕ (⟨⟩ #2 (⟨⟩ #3 (⟨⟩ #4 ∅))))))

; Test 34: Max of list equals maximum
(⊨ :integration-max-maximum
   #t
   (≡ ((↥ ((↥ #5) #3)) #7)
      (↥↥ (⟨⟩ #3 (⟨⟩ #5 (⟨⟩ #7 ∅))))))

; Test 35: Min of list equals minimum
(⊨ :integration-min-minimum
   #t
   (≡ ((↧ ((↧ #5) #7)) #3)
      (↧↧ (⟨⟩ #3 (⟨⟩ #5 (⟨⟩ #7 ∅))))))

; Test 36: Sum of range 1-10
(⊨ :integration-sum-range
   #55
   (⊕⊕ (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 (⟨⟩ #4 (⟨⟩ #5 (⟨⟩ #6 (⟨⟩ #7 (⟨⟩ #8 (⟨⟩ #9 (⟨⟩ #10 ∅))))))))))))

; ============================================================================
; Summary
; ============================================================================

; Total tests: 36
; ⊕⊕ (sum): 5 tests
; ⊗⊗ (product): 5 tests
; ↥ (max): 5 tests
; ↧ (min): 5 tests
; ↥↥ (maximum): 6 tests
; ↧↧ (minimum): 6 tests
; Integration: 4 tests
