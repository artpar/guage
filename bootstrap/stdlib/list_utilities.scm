; Guage Standard Library: List Utilities
; Additional list operations for data processing and transformation
; Pure symbolic names, explicit currying

; ============================================================================
; Conditional List Operations
; ============================================================================

; take-while :: (α -> Bool) -> [α] -> [α]
; Take elements from list while predicate holds
; Ex: ((take-while (lambda (x) (< x #5))) ⟨#1 ⟨#2 ⟨#7 ∅⟩⟩⟩) -> ⟨#1 ⟨#2 ∅⟩⟩
(define take-while (lambda (pred) (lambda (lst)
  (if (null? lst)
     nil
     ((lambda (head)
       (if (pred head)
          (cons head ((take-while pred) (cdr lst)))
          nil))
      (car lst))))))

; drop-while :: (α -> Bool) -> [α] -> [α]
; Drop elements from list while predicate holds
; Ex: ((drop-while (lambda (x) (< x #5))) ⟨#1 ⟨#2 ⟨#7 ∅⟩⟩⟩) -> ⟨#7 ∅⟩
(define drop-while (lambda (pred) (lambda (lst)
  (if (null? lst)
     nil
     ((lambda (head)
       (if (pred head)
          ((drop-while pred) (cdr lst))
          lst))
      (car lst))))))

; span :: (α -> Bool) -> [α] -> ⟨[α] [α]⟩
; Split list at first element that fails predicate
; Returns pair of (elements-that-pass, rest)
; Ex: ((span (lambda (x) (< x #5))) ⟨#1 ⟨#2 ⟨#7 ⟨#8 ∅⟩⟩⟩⟩) -> ⟨⟨#1 ⟨#2 ∅⟩⟩ ⟨#7 ⟨#8 ∅⟩⟩⟩
(define span (lambda (pred) (lambda (lst)
  (cons ((take-while pred) lst)
      ((drop-while pred) lst)))))

; break :: (α -> Bool) -> [α] -> ⟨[α] [α]⟩
; Split list at first element that satisfies predicate
; Returns pair of (elements-before, rest-starting-with-match)
; Ex: ((break (lambda (x) (equal? x #5))) ⟨#1 ⟨#2 ⟨#5 ⟨#8 ∅⟩⟩⟩⟩) -> ⟨⟨#1 ⟨#2 ∅⟩⟩ ⟨#5 ⟨#8 ∅⟩⟩⟩
(define break (lambda (pred) (lambda (lst)
  ((span (lambda (x) (not (pred x)))) lst))))

; ============================================================================
; List Transformation
; ============================================================================

; flatten :: [[α]] -> [α]
; Flatten one level of nested list structure
; Ex: (flatten ⟨⟨#1 ⟨#2 ∅⟩⟩ ⟨⟨#3 ⟨#4 ∅⟩⟩ ∅⟩⟩) -> ⟨#1 ⟨#2 ⟨#3 ⟨#4 ∅⟩⟩⟩⟩
(define flatten (lambda (lst)
  (if (null? lst)
     nil
     (⧺ (car lst) (flatten (cdr lst))))))

; distinct :: [α] -> [α]
; Remove duplicate elements (preserves first occurrence order)
; Uses O(n²) simple implementation with membership checking
; Ex: (distinct ⟨#1 ⟨#2 ⟨#1 ⟨#3 ⟨#2 ∅⟩⟩⟩⟩⟩) -> ⟨#1 ⟨#2 ⟨#3 ∅⟩⟩⟩
(define distinct (lambda (lst)
  (if (null? lst)
     nil
     ((lambda (head)
       ((lambda (tail-distinct)
         (if ((∋ head) tail-distinct)
            tail-distinct
            (cons head tail-distinct)))
        (distinct (cdr lst))))
      (car lst)))))

; ============================================================================
; Safe Accessors
; ============================================================================

; nth-or :: [α] -> ℕ -> α -> α
; Safe indexed access with default value
; Returns element at index n, or default if out of bounds
; Ex: (((nth-or ⟨#10 ⟨#20 ⟨#30 ∅⟩⟩⟩) #1) #999) -> #20
; Ex: (((nth-or ⟨#10 ⟨#20 ∅⟩⟩) #5) #999) -> #999
(define nth-or (lambda (lst) (lambda (n) (lambda (default)
  (if (null? lst)
     default
     (if (equal? n #0)
        (car lst)
        (((nth-or (cdr lst)) (- n #1)) default)))))))

; head-or :: [α] -> α -> α
; Safe head with default value
; Ex: ((head-or ⟨#42 ∅⟩) #999) -> #42
; Ex: ((head-or nil) #999) -> #999
(define head-or (lambda (lst) (lambda (default)
  (if (null? lst) default (car lst)))))

; tail-or :: [α] -> [α] -> [α]
; Safe tail with default value
; Ex: ((tail-or ⟨#1 ⟨#2 ∅⟩⟩) nil) -> ⟨#2 ∅⟩
; Ex: ((tail-or nil) ⟨#99 ∅⟩) -> ⟨#99 ∅⟩
(define tail-or (lambda (lst) (lambda (default)
  (if (null? lst) default (cdr lst)))))

; ============================================================================
; List Analysis
; ============================================================================

; all? :: (α -> Bool) -> [α] -> Bool
; Check if all elements satisfy predicate (same as ∀ from list.scm)
; Provided here for completeness and discoverability
; Ex: ((all? (lambda (x) (> x #0))) ⟨#1 ⟨#2 ⟨#3 ∅⟩⟩⟩) -> #t
; Ex: ((all? (lambda (x) (> x #0))) ⟨#1 ⟨#-2 ⟨#3 ∅⟩⟩⟩) -> #f
(define all? (lambda (pred) (lambda (lst)
  (if (null? lst)
     #t
     (and (pred (car lst))
        ((all? pred) (cdr lst)))))))

; any? :: (α -> Bool) -> [α] -> Bool
; Check if any element satisfies predicate (same as ∃ from list.scm)
; Provided here for completeness and discoverability
; Ex: ((any? (lambda (x) (> x #5))) ⟨#1 ⟨#2 ⟨#10 ∅⟩⟩⟩) -> #t
; Ex: ((any? (lambda (x) (> x #10))) ⟨#1 ⟨#2 ⟨#3 ∅⟩⟩⟩) -> #f
(define any? (lambda (pred) (lambda (lst)
  (if (null? lst)
     #f
     (or (pred (car lst))
        ((any? pred) (cdr lst)))))))

; none? :: (α -> Bool) -> [α] -> Bool
; Check if no elements satisfy predicate
; Ex: ((none? (lambda (x) (< x #0))) ⟨#1 ⟨#2 ⟨#3 ∅⟩⟩⟩) -> #t
; Ex: ((none? (lambda (x) (< x #0))) ⟨#-1 ⟨#2 ∅⟩⟩) -> #f
(define none? (lambda (pred) (lambda (lst)
  (not ((any? pred) lst)))))

; ============================================================================
; List Building
; ============================================================================

; replicate-at :: α -> ℕ -> [α]
; Create list of n copies of value (alias for ⊚⊚ from list.scm)
; Provided here for discoverability
; Ex: ((replicate-at #42) #3) -> ⟨#42 ⟨#42 ⟨#42 ∅⟩⟩⟩
(define replicate-at (lambda (x) (lambda (n)
  (if (<= n #0)
     nil
     (cons x ((replicate-at x) (- n #1)))))))

; cycle-at :: [α] -> ℕ -> [α]
; Repeat list n times (finite cycling)
; Ex: ((cycle-at ⟨#1 ⟨#2 ∅⟩⟩) #2) -> ⟨#1 ⟨#2 ⟨#1 ⟨#2 ∅⟩⟩⟩⟩
(define cycle-at (lambda (lst) (lambda (n)
  (if (<= n #0)
     nil
     (⧺ lst ((cycle-at lst) (- n #1)))))))

; ============================================================================
; Planned Future Functions (Complex)
; ============================================================================

; The following require more complex implementations or new infrastructure:
;
; group-by :: (α -> β) -> [α] -> [(β, [α])]
; - Group elements by key function
; - Returns association list of (key, elements) pairs
; - Requires association list infrastructure or ADT
;
; sort-by :: (α -> α -> Bool) -> [α] -> [α]
; - Sort list using comparison function
; - Requires efficient sorting algorithm
; - O(n log n) complexity target
;
; group :: [α] -> [[α]]
; - Group consecutive equal elements
; - Ex: (group ⟨#1 ⟨#1 ⟨#2 ⟨#2 ⟨#3 ∅⟩⟩⟩⟩⟩) -> ⟨⟨#1 ⟨#1 ∅⟩⟩ ⟨⟨#2 ⟨#2 ∅⟩⟩ ⟨⟨#3 ∅⟩ ∅⟩⟩⟩
