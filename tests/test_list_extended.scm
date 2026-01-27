; Test suite for extended list operations
; Tests for: ⇶ ⊡ ⊳ ⊞ ⊟ ⊠

; ============================================================================
; Load required functions from stdlib/list.scm
; ============================================================================

; ⧺ :: [α] → [α] → [α]
; Append - concatenate two lists
(≔ ⧺ (λ (lst2) (λ (lst1)
  (((⊕→ (λ (a) (λ (b) (⟨⟩ a b)))) lst1) lst2))))

; ⊕→ :: (α → β → β) → [α] → β → β
; Fold-right - accumulate from right to left
(≔ ⊕→ (λ (ƒ) (λ (lst) (λ (acc)
  (? (∅? lst)
     acc
     ((ƒ (◁ lst)) (((⊕→ ƒ) (▷ lst)) acc)))))))

; ↦ :: (α → β) → [α] → [β]
; Map - transform each element using function
(≔ ↦ (λ (ƒ) (λ (lst)
  (? (∅? lst)
     ∅
     (⟨⟩ (ƒ (◁ lst)) ((↦ ƒ) (▷ lst)))))))

; ⊲ :: (α → 𝔹) → [α] → [α]
; Filter - keep only elements satisfying predicate
(≔ ⊲ (λ (pred) (λ (lst)
  (? (∅? lst)
     ∅
     (? (pred (◁ lst))
        (⟨⟩ (◁ lst) ((⊲ pred) (▷ lst)))
        ((⊲ pred) (▷ lst)))))))

; # :: [α] → ℕ
; Length - count elements
(≔ # (λ (lst)
  (((⊕← (λ (acc) (λ (_) (⊕ acc #1)))) #0) lst)))

; ⊕← :: (α → β → α) → α → [β] → α
; Fold-left - accumulate from left to right
(≔ ⊕← (λ (ƒ) (λ (acc) (λ (lst)
  (? (∅? lst)
     acc
     (((⊕← ƒ) ((ƒ acc) (◁ lst))) (▷ lst)))))))

; ============================================================================
; Load functions from stdlib/list_extended.scm
; ============================================================================

; ⇶ :: (α → 𝔹) → [α] → α | ∅
; Find - first element satisfying predicate (returns ∅ if not found)
(≔ ⇶ (λ (pred) (λ (lst)
  (? (∅? lst)
     ∅
     (? (pred (◁ lst))
        (◁ lst)
        ((⇶ pred) (▷ lst)))))))

; ⊡ :: ℕ → [α] → α | ∅
; Nth - get element at index (0-based, returns ∅ if out of bounds)
(≔ ⊡ (λ (idx) (λ (lst)
  (? (∅? lst)
     ∅
     (? (≡ idx #0)
        (◁ lst)
        ((⊡ (⊖ idx #1)) (▷ lst)))))))

; ⊳ :: (α → 𝔹) → [α] → ⟨[α] [α]⟩
; Partition - split into (satisfies, doesn't-satisfy) pair
(≔ ⊳ (λ (pred) (λ (lst)
  (? (∅? lst)
     (⟨⟩ ∅ ∅)
     (? (pred (◁ lst))
        ; Add to first list (satisfies)
        (⟨⟩ (⟨⟩ (◁ lst) (◁ ((⊳ pred) (▷ lst))))
            (▷ ((⊳ pred) (▷ lst))))
        ; Add to second list (doesn't satisfy)
        (⟨⟩ (◁ ((⊳ pred) (▷ lst)))
            (⟨⟩ (◁ lst) (▷ ((⊳ pred) (▷ lst))))))))))

; ⊞ :: [[α]] → [α]
; Concat - flatten list of lists into single list
(≔ ⊞ (λ (lst-of-lsts)
  (? (∅? lst-of-lsts)
     ∅
     ; Append first list to flattened rest
     ((⧺ (⊞ (▷ lst-of-lsts))) (◁ lst-of-lsts)))))

; ⊟ :: α → [α] → [α]
; Intersperse - insert separator between elements
(≔ ⊟ (λ (sep) (λ (lst)
  (? (∅? lst)
     ∅
     (? (∅? (▷ lst))
        ; Single element - no separator needed
        lst
        ; Multiple elements - add separator
        (⟨⟩ (◁ lst) (⟨⟩ sep ((⊟ sep) (▷ lst)))))))))

; ⊠ :: [α] → [β] → [⟨α β⟩]
; Cartesian - cartesian product of two lists
(≔ ⊠ (λ (lst2) (λ (lst1)
  (? (∅? lst1)
     ∅
     (? (∅? lst2)
        ∅
        ; For each element in lst1, pair with all elements in lst2
        ((⧺ ((⊠ lst2) (▷ lst1)))
            ((↦ (λ (y) (⟨⟩ (◁ lst1) y))) lst2)))))))

; ============================================================================
; ⇶ (find) - First element satisfying predicate
; ============================================================================

; Test 1: Find element that exists (first match)
(⊨ :find-exists-first
   #7
   ((⇶ (λ (x) (> x #5))) (⟨⟩ #3 (⟨⟩ #7 (⟨⟩ #9 ∅)))))

; Test 2: Find element that exists (later match)
(⊨ :find-exists-later
   #9
   ((⇶ (λ (x) (> x #8))) (⟨⟩ #3 (⟨⟩ #7 (⟨⟩ #9 ∅)))))

; Test 3: Find element that doesn't exist
(⊨ :find-not-found
   ∅
   ((⇶ (λ (x) (> x #10))) (⟨⟩ #3 (⟨⟩ #7 (⟨⟩ #9 ∅)))))

; Test 4: Find in empty list
(⊨ :find-empty
   ∅
   ((⇶ (λ (x) (> x #5))) ∅))

; Test 5: Find with equality
(⊨ :find-equal
   #5
   ((⇶ (λ (x) (≡ x #5))) (⟨⟩ #1 (⟨⟩ #5 (⟨⟩ #9 ∅)))))

; Test 6: Find first of many matches
(⊨ :find-first-match
   #7
   ((⇶ (λ (x) (> x #5))) (⟨⟩ #7 (⟨⟩ #8 (⟨⟩ #9 ∅)))))

; ============================================================================
; ⊡ (nth) - Get element at index
; ============================================================================

; Test 7: Get first element (index 0)
(⊨ :nth-first
   #1
   ((⊡ #0) (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅)))))

; Test 8: Get middle element
(⊨ :nth-middle
   #2
   ((⊡ #1) (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅)))))

; Test 9: Get last element
(⊨ :nth-last
   #3
   ((⊡ #2) (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅)))))

; Test 10: Index out of bounds
(⊨ :nth-out-of-bounds
   ∅
   ((⊡ #5) (⟨⟩ #1 (⟨⟩ #2 ∅))))

; Test 11: Nth in empty list
(⊨ :nth-empty
   ∅
   ((⊡ #0) ∅))

; Test 12: Nth with larger list
(⊨ :nth-larger
   #4
   ((⊡ #3) (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 (⟨⟩ #4 ∅))))))

; ============================================================================
; ⊳ (partition) - Split into two lists
; ============================================================================

; Test 13: Partition with some matches
(⊨ :partition-mixed
   #t
   (≟ (⟨⟩ (⟨⟩ #7 (⟨⟩ #9 ∅)) (⟨⟩ #3 (⟨⟩ #2 ∅)))
       ((⊳ (λ (x) (> x #5))) (⟨⟩ #3 (⟨⟩ #7 (⟨⟩ #2 (⟨⟩ #9 ∅)))))))

; Test 14: Partition with all matches
(⊨ :partition-all-match
   #t
   (≟ (⟨⟩ (⟨⟩ #7 (⟨⟩ #8 (⟨⟩ #9 ∅))) ∅)
       ((⊳ (λ (x) (> x #5))) (⟨⟩ #7 (⟨⟩ #8 (⟨⟩ #9 ∅))))))

; Test 15: Partition with no matches
(⊨ :partition-no-match
   #t
   (≟ (⟨⟩ ∅ (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅))))
       ((⊳ (λ (x) (> x #5))) (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅))))))

; Test 16: Partition empty list
(⊨ :partition-empty
   #t
   (≟ (⟨⟩ ∅ ∅)
       ((⊳ (λ (x) (> x #5))) ∅)))

; Test 17: Partition with even/odd
(⊨ :partition-even
   #t
   (≟ (⟨⟩ (⟨⟩ #2 (⟨⟩ #4 ∅)) (⟨⟩ #1 (⟨⟩ #3 ∅)))
       ((⊳ (λ (x) (≡ (% x #2) #0))) (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 (⟨⟩ #4 ∅)))))))

; ============================================================================
; ⊞ (concat) - Flatten list of lists
; ============================================================================

; Test 18: Concat two lists
(⊨ :concat-two
   #t
   (≟ (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 (⟨⟩ #4 ∅))))
       (⊞ (⟨⟩ (⟨⟩ #1 (⟨⟩ #2 ∅)) (⟨⟩ (⟨⟩ #3 (⟨⟩ #4 ∅)) ∅)))))

; Test 19: Concat three lists
(⊨ :concat-three
   #t
   (≟ (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 (⟨⟩ #4 (⟨⟩ #5 (⟨⟩ #6 ∅))))))
       (⊞ (⟨⟩ (⟨⟩ #1 (⟨⟩ #2 ∅))
           (⟨⟩ (⟨⟩ #3 (⟨⟩ #4 ∅))
               (⟨⟩ (⟨⟩ #5 (⟨⟩ #6 ∅)) ∅))))))

; Test 20: Concat with empty lists
(⊨ :concat-empties
   #t
   (≟ (⟨⟩ #1 (⟨⟩ #2 ∅))
       (⊞ (⟨⟩ (⟨⟩ #1 ∅) (⟨⟩ ∅ (⟨⟩ (⟨⟩ #2 ∅) ∅))))))

; Test 21: Concat empty list of lists
(⊨ :concat-empty
   ∅
   (⊞ ∅))

; Test 22: Concat single list
(⊨ :concat-single
   #t
   (≟ (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅)))
       (⊞ (⟨⟩ (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅))) ∅))))

; ============================================================================
; ⊟ (intersperse) - Insert separator between elements
; ============================================================================

; Test 23: Intersperse in normal list
(⊨ :intersperse-normal
   #t
   (≟ (⟨⟩ #1 (⟨⟩ #0 (⟨⟩ #2 (⟨⟩ #0 (⟨⟩ #3 ∅)))))
       ((⊟ #0) (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅))))))

; Test 24: Intersperse in two-element list
(⊨ :intersperse-two
   #t
   (≟ (⟨⟩ #1 (⟨⟩ #0 (⟨⟩ #2 ∅)))
       ((⊟ #0) (⟨⟩ #1 (⟨⟩ #2 ∅)))))

; Test 25: Intersperse in single-element list
(⊨ :intersperse-single
   #t
   (≟ (⟨⟩ #1 ∅)
       ((⊟ #0) (⟨⟩ #1 ∅))))

; Test 26: Intersperse in empty list
(⊨ :intersperse-empty
   ∅
   ((⊟ #0) ∅))

; Test 27: Intersperse with different separator
(⊨ :intersperse-negone
   #t
   (≟ (⟨⟩ #1 (⟨⟩ #-1 (⟨⟩ #2 (⟨⟩ #-1 (⟨⟩ #3 ∅)))))
       ((⊟ #-1) (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅))))))

; ============================================================================
; ⊠ (cartesian) - Cartesian product
; ============================================================================

; Test 28: Cartesian of two small lists
(⊨ :cartesian-2x2
   #t
   (≟ (⟨⟩ (⟨⟩ #1 #10) (⟨⟩ (⟨⟩ #1 #20) (⟨⟩ (⟨⟩ #2 #10) (⟨⟩ (⟨⟩ #2 #20) ∅))))
       ((⊠ (⟨⟩ #10 (⟨⟩ #20 ∅))) (⟨⟩ #1 (⟨⟩ #2 ∅)))))

; Test 29: Cartesian with single element lists
(⊨ :cartesian-1x1
   #t
   (≟ (⟨⟩ (⟨⟩ #1 #10) ∅)
       ((⊠ (⟨⟩ #10 ∅)) (⟨⟩ #1 ∅))))

; Test 30: Cartesian with empty first list
(⊨ :cartesian-empty-first
   ∅
   ((⊠ (⟨⟩ #10 (⟨⟩ #20 ∅))) ∅))

; Test 31: Cartesian with empty second list
(⊨ :cartesian-empty-second
   ∅
   ((⊠ ∅) (⟨⟩ #1 (⟨⟩ #2 ∅))))

; Test 32: Cartesian 3x2
(⊨ :cartesian-3x2
   #6
   (# ((⊠ (⟨⟩ #10 (⟨⟩ #20 ∅))) (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅))))))

; Test 33: Cartesian result count check
(⊨ :cartesian-count
   #12
   (# ((⊠ (⟨⟩ #10 (⟨⟩ #20 (⟨⟩ #30 ∅)))) (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 (⟨⟩ #4 ∅)))))))

; ============================================================================
; Integration Tests - Using multiple functions together
; ============================================================================

; Test 34: Find using nth
(⊨ :integration-find-nth
   #9
   ((⊡ #1) (◁ ((⊳ (λ (x) (> x #5))) (⟨⟩ #3 (⟨⟩ #7 (⟨⟩ #2 (⟨⟩ #9 ∅))))))))

; Test 35: Concat after partition
(⊨ :integration-concat-partition
   #4
   (# (⊞ (⟨⟩ (◁ ((⊳ (λ (x) (> x #5))) (⟨⟩ #3 (⟨⟩ #7 (⟨⟩ #2 (⟨⟩ #9 ∅))))))
           (⟨⟩ (▷ ((⊳ (λ (x) (> x #5))) (⟨⟩ #3 (⟨⟩ #7 (⟨⟩ #2 (⟨⟩ #9 ∅)))))) ∅)))))

; Test 36: Intersperse then filter
(⊨ :integration-intersperse-filter
   #3
   (# ((⊲ (λ (x) (≡ x #0))) ((⊟ #0) (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 (⟨⟩ #4 ∅))))))))

; Test 37: Find in cartesian product
(⊨ :integration-find-cartesian
   #t
   (≟ (⟨⟩ #2 #10)
       ((⇶ (λ (pair) (≡ (◁ pair) #2))) ((⊠ (⟨⟩ #10 (⟨⟩ #20 ∅))) (⟨⟩ #1 (⟨⟩ #2 ∅))))))

; Test 38: Complex integration - partition, nth, find
(⊨ :integration-complex
   #9
   ((⇶ (λ (x) (> x #8)))
    (◁ ((⊳ (λ (x) (> x #5))) (⟨⟩ #3 (⟨⟩ #7 (⟨⟩ #2 (⟨⟩ #9 (⟨⟩ #1 (⟨⟩ #8 ∅))))))))))

; ============================================================================
; Summary
; ============================================================================

; Total tests: 38
; ⇶ (find): 6 tests
; ⊡ (nth): 6 tests
; ⊳ (partition): 5 tests
; ⊞ (concat): 5 tests
; ⊟ (intersperse): 5 tests
; ⊠ (cartesian): 6 tests
; Integration: 5 tests
