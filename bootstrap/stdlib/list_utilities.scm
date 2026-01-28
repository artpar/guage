; Guage Standard Library: List Utilities
; Additional list operations for data processing and transformation
; Pure symbolic names, explicit currying

; ============================================================================
; Conditional List Operations
; ============================================================================

; take-while :: (α → 𝔹) → [α] → [α]
; Take elements from list while predicate holds
; Ex: ((take-while (λ (x) (< x #5))) ⟨#1 ⟨#2 ⟨#7 ∅⟩⟩⟩) → ⟨#1 ⟨#2 ∅⟩⟩
(≔ take-while (λ (pred) (λ (lst)
  (? (∅? lst)
     ∅
     ((λ (head)
       (? (pred head)
          (⟨⟩ head ((take-while pred) (▷ lst)))
          ∅))
      (◁ lst))))))

; drop-while :: (α → 𝔹) → [α] → [α]
; Drop elements from list while predicate holds
; Ex: ((drop-while (λ (x) (< x #5))) ⟨#1 ⟨#2 ⟨#7 ∅⟩⟩⟩) → ⟨#7 ∅⟩
(≔ drop-while (λ (pred) (λ (lst)
  (? (∅? lst)
     ∅
     ((λ (head)
       (? (pred head)
          ((drop-while pred) (▷ lst))
          lst))
      (◁ lst))))))

; span :: (α → 𝔹) → [α] → ⟨[α] [α]⟩
; Split list at first element that fails predicate
; Returns pair of (elements-that-pass, rest)
; Ex: ((span (λ (x) (< x #5))) ⟨#1 ⟨#2 ⟨#7 ⟨#8 ∅⟩⟩⟩⟩) → ⟨⟨#1 ⟨#2 ∅⟩⟩ ⟨#7 ⟨#8 ∅⟩⟩⟩
(≔ span (λ (pred) (λ (lst)
  (⟨⟩ ((take-while pred) lst)
      ((drop-while pred) lst)))))

; break :: (α → 𝔹) → [α] → ⟨[α] [α]⟩
; Split list at first element that satisfies predicate
; Returns pair of (elements-before, rest-starting-with-match)
; Ex: ((break (λ (x) (≡ x #5))) ⟨#1 ⟨#2 ⟨#5 ⟨#8 ∅⟩⟩⟩⟩) → ⟨⟨#1 ⟨#2 ∅⟩⟩ ⟨#5 ⟨#8 ∅⟩⟩⟩
(≔ break (λ (pred) (λ (lst)
  ((span (λ (x) (¬ (pred x)))) lst))))

; ============================================================================
; List Transformation
; ============================================================================

; flatten :: [[α]] → [α]
; Flatten one level of nested list structure
; Ex: (flatten ⟨⟨#1 ⟨#2 ∅⟩⟩ ⟨⟨#3 ⟨#4 ∅⟩⟩ ∅⟩⟩) → ⟨#1 ⟨#2 ⟨#3 ⟨#4 ∅⟩⟩⟩⟩
(≔ flatten (λ (lst)
  (? (∅? lst)
     ∅
     (⧺ (◁ lst) (flatten (▷ lst))))))

; distinct :: [α] → [α]
; Remove duplicate elements (preserves first occurrence order)
; Uses O(n²) simple implementation with membership checking
; Ex: (distinct ⟨#1 ⟨#2 ⟨#1 ⟨#3 ⟨#2 ∅⟩⟩⟩⟩⟩) → ⟨#1 ⟨#2 ⟨#3 ∅⟩⟩⟩
(≔ distinct (λ (lst)
  (? (∅? lst)
     ∅
     ((λ (head)
       ((λ (tail-distinct)
         (? (∈ head tail-distinct)
            tail-distinct
            (⟨⟩ head tail-distinct)))
        (distinct (▷ lst))))
      (◁ lst)))))

; ============================================================================
; Safe Accessors
; ============================================================================

; nth-or :: [α] → ℕ → α → α
; Safe indexed access with default value
; Returns element at index n, or default if out of bounds
; Ex: (((nth-or ⟨#10 ⟨#20 ⟨#30 ∅⟩⟩⟩) #1) #999) → #20
; Ex: (((nth-or ⟨#10 ⟨#20 ∅⟩⟩) #5) #999) → #999
(≔ nth-or (λ (lst) (λ (n) (λ (default)
  (? (∅? lst)
     default
     (? (≡ n #0)
        (◁ lst)
        (((nth-or (▷ lst)) (⊖ n #1)) default)))))))

; head-or :: [α] → α → α
; Safe head with default value
; Ex: ((head-or ⟨#42 ∅⟩) #999) → #42
; Ex: ((head-or ∅) #999) → #999
(≔ head-or (λ (lst) (λ (default)
  (? (∅? lst) default (◁ lst)))))

; tail-or :: [α] → [α] → [α]
; Safe tail with default value
; Ex: ((tail-or ⟨#1 ⟨#2 ∅⟩⟩) ∅) → ⟨#2 ∅⟩
; Ex: ((tail-or ∅) ⟨#99 ∅⟩) → ⟨#99 ∅⟩
(≔ tail-or (λ (lst) (λ (default)
  (? (∅? lst) default (▷ lst)))))

; ============================================================================
; List Analysis
; ============================================================================

; all? :: (α → 𝔹) → [α] → 𝔹
; Check if all elements satisfy predicate (same as ∀ from list.scm)
; Provided here for completeness and discoverability
; Ex: ((all? (λ (x) (> x #0))) ⟨#1 ⟨#2 ⟨#3 ∅⟩⟩⟩) → #t
; Ex: ((all? (λ (x) (> x #0))) ⟨#1 ⟨#-2 ⟨#3 ∅⟩⟩⟩) → #f
(≔ all? (λ (pred) (λ (lst)
  (? (∅? lst)
     #t
     (∧ (pred (◁ lst))
        ((all? pred) (▷ lst)))))))

; any? :: (α → 𝔹) → [α] → 𝔹
; Check if any element satisfies predicate (same as ∃ from list.scm)
; Provided here for completeness and discoverability
; Ex: ((any? (λ (x) (> x #5))) ⟨#1 ⟨#2 ⟨#10 ∅⟩⟩⟩) → #t
; Ex: ((any? (λ (x) (> x #10))) ⟨#1 ⟨#2 ⟨#3 ∅⟩⟩⟩) → #f
(≔ any? (λ (pred) (λ (lst)
  (? (∅? lst)
     #f
     (∨ (pred (◁ lst))
        ((any? pred) (▷ lst)))))))

; none? :: (α → 𝔹) → [α] → 𝔹
; Check if no elements satisfy predicate
; Ex: ((none? (λ (x) (< x #0))) ⟨#1 ⟨#2 ⟨#3 ∅⟩⟩⟩) → #t
; Ex: ((none? (λ (x) (< x #0))) ⟨#-1 ⟨#2 ∅⟩⟩) → #f
(≔ none? (λ (pred) (λ (lst)
  (¬ ((any? pred) lst)))))

; ============================================================================
; List Building
; ============================================================================

; replicate-at :: α → ℕ → [α]
; Create list of n copies of value (alias for ⊚⊚ from list.scm)
; Provided here for discoverability
; Ex: ((replicate-at #42) #3) → ⟨#42 ⟨#42 ⟨#42 ∅⟩⟩⟩
(≔ replicate-at (λ (x) (λ (n)
  (? (≤ n #0)
     ∅
     (⟨⟩ x ((replicate-at x) (⊖ n #1)))))))

; cycle-at :: [α] → ℕ → [α]
; Repeat list n times (finite cycling)
; Ex: ((cycle-at ⟨#1 ⟨#2 ∅⟩⟩) #2) → ⟨#1 ⟨#2 ⟨#1 ⟨#2 ∅⟩⟩⟩⟩
(≔ cycle-at (λ (lst) (λ (n)
  (? (≤ n #0)
     ∅
     (⧺ lst ((cycle-at lst) (⊖ n #1)))))))

; ============================================================================
; Planned Future Functions (Complex)
; ============================================================================

; The following require more complex implementations or new infrastructure:
;
; group-by :: (α → β) → [α] → [(β, [α])]
; - Group elements by key function
; - Returns association list of (key, elements) pairs
; - Requires association list infrastructure or ADT
;
; sort-by :: (α → α → 𝔹) → [α] → [α]
; - Sort list using comparison function
; - Requires efficient sorting algorithm
; - O(n log n) complexity target
;
; group :: [α] → [[α]]
; - Group consecutive equal elements
; - Ex: (group ⟨#1 ⟨#1 ⟨#2 ⟨#2 ⟨#3 ∅⟩⟩⟩⟩⟩) → ⟨⟨#1 ⟨#1 ∅⟩⟩ ⟨⟨#2 ⟨#2 ∅⟩⟩ ⟨⟨#3 ∅⟩ ∅⟩⟩⟩
