; Guage Standard Library: List Operations
; Pure symbolic names, named parameters (converted to De Bruijn internally)

; ============================================================================
; Core List Operations
; ============================================================================

; list-map :: (α -> β) -> [α] -> [β]
; Map - transform each element using function
(define list-map (lambda (ƒ) (lambda (lst)
  (if (null? lst)
     nil
     (cons (ƒ (car lst)) ((list-map ƒ) (cdr lst)))))))

; list-filter :: (α -> Bool) -> [α] -> [α]
; Filter - keep only elements satisfying predicate
(define list-filter (lambda (pred) (lambda (lst)
  (if (null? lst)
     nil
     (if (pred (car lst))
        (cons (car lst) ((list-filter pred) (cdr lst)))
        ((list-filter pred) (cdr lst)))))))

; fold-left :: (α -> β -> α) -> α -> [β] -> α
; Fold-left - accumulate from left to right
(define fold-left (lambda (ƒ) (lambda (acc) (lambda (lst)
  (if (null? lst)
     acc
     (((fold-left ƒ) ((ƒ acc) (car lst))) (cdr lst)))))))

; fold-right :: (α -> β -> β) -> [α] -> β -> β
; Fold-right - accumulate from right to left
(define fold-right (lambda (ƒ) (lambda (lst) (lambda (acc)
  (if (null? lst)
     acc
     ((ƒ (car lst)) (((fold-right ƒ) (cdr lst)) acc)))))))

; ============================================================================
; List Utilities
; ============================================================================

; # :: [α] -> ℕ
; Length - count elements
(define # (lambda (lst)
  (((fold-left (lambda (acc) (lambda (_) (+ acc #1)))) #0) lst)))

; ⧺ :: [α] -> [α] -> [α]
; Append - concatenate two lists
(define ⧺ (lambda (lst2) (lambda (lst1)
  (((fold-right (lambda (x) (lambda (acc) (cons x acc)))) lst1) lst2))))

; ⇄ :: [α] -> [α]
; Reverse - reverse order
(define ⇄ (lambda (lst)
  (((fold-left (lambda (acc) (lambda (x) (cons x acc)))) nil) lst)))

; ============================================================================
; List Slicing
; ============================================================================

; ↑ :: ℕ -> [α] -> [α]
; Take - first n elements
(define ↑ (lambda (n) (lambda (lst)
  (if (equal? n #0)
     nil
     (if (null? lst)
        nil
        (cons (car lst) ((↑ (- n #1)) (cdr lst))))))))

; ↓ :: ℕ -> [α] -> [α]
; Drop - skip first n elements
(define ↓ (lambda (n) (lambda (lst)
  (if (equal? n #0)
     lst
     (if (null? lst)
        nil
        ((↓ (- n #1)) (cdr lst)))))))

; ============================================================================
; List Combinators
; ============================================================================

; ⊼ :: [α] -> [β] -> [⟨α β⟩]
; Zip - pair corresponding elements
(define ⊼ (lambda (lst2) (lambda (lst1)
  (if (null? lst1)
     nil
     (if (null? lst2)
        nil
        (cons (cons (car lst1) (car lst2)) ((⊼ (cdr lst2)) (cdr lst1))))))))

; ∃ :: (α -> Bool) -> [α] -> Bool
; Exists (any) - test if any element satisfies predicate
(define ∃ (lambda (pred) (lambda (lst)
  (if (null? lst)
     #f
     (if (pred (car lst))
        #t
        ((∃ pred) (cdr lst)))))))

; ∀ :: (α -> Bool) -> [α] -> Bool
; Forall (all) - test if all elements satisfy predicate
(define ∀ (lambda (pred) (lambda (lst)
  (if (null? lst)
     #t
     (if (pred (car lst))
        ((∀ pred) (cdr lst))
        #f)))))

; ============================================================================
; List Search
; ============================================================================

; ∋ :: α -> [α] -> Bool
; Contains (element of) - test membership
; Note: type-decl is reserved for type annotation (type-decl name type)
(define ∋ (lambda (elem) (lambda (lst)
  ((∃ (lambda (x) (equal? elem x))) lst))))

; ============================================================================
; List Building
; ============================================================================

; range :: ℕ -> ℕ -> [ℕ]
; Range - numbers from start to end (exclusive)
(define range (lambda (end) (lambda (start)
  (if (>= start end)
     nil
     (cons start ((range end) (+ start #1)))))))

; ⊚⊚ :: ℕ -> α -> [α]
; Replicate - n copies of value
(define ⊚⊚ (lambda (val) (lambda (n)
  (if (equal? n #0)
     nil
     (cons val ((⊚⊚ val) (- n #1)))))))

; ============================================================================
; Advanced List Transformation
; ============================================================================

; ⊽ :: [⟨α β⟩] -> ⟨[α] [β]⟩
; Unzip - split paired list into two lists
(define ⊽ (lambda (pairs)
  ((lambda (fsts) ((lambda (snds)
    (cons fsts snds))
    ((list-map cdr) pairs)))
   ((list-map car) pairs))))

; ⊤⊥ :: [[α]] -> [[α]]
; Transpose - rotate matrix of lists
(define ⊤⊥ (lambda (matrix)
  (if (null? matrix)
     nil
     (if (null? (car matrix))
        nil
        (cons ((list-map car) matrix)
            (⊤⊥ ((list-map cdr) matrix)))))))

; Helper for flatten
(define ⊟-helper (lambda (acc) (lambda (sublist) ((⧺ sublist) acc))))

; deque :: [[α]] -> [α]
; Flatten - deep list flattening (one level)
(define deque (lambda (lst)
  (((fold-left ⊟-helper) nil) lst)))

; ↦⊟ :: (α -> [β]) -> [α] -> [β]
; Flat-map - map function then flatten
(define ↦⊟ (lambda (ƒ) (lambda (lst)
  (deque ((list-map ƒ) lst)))))

; ============================================================================
; Conditional List Operations
; ============================================================================

; ↑? :: (α -> Bool) -> [α] -> [α]
; Take-while - take elements while predicate true
(define ↑? (lambda (pred) (lambda (lst)
  (if (null? lst)
     nil
     (if (pred (car lst))
        (cons (car lst) ((↑? pred) (cdr lst)))
        nil)))))

; ↓? :: (α -> Bool) -> [α] -> [α]
; Drop-while - drop elements while predicate true
(define ↓? (lambda (pred) (lambda (lst)
  (if (null? lst)
     nil
     (if (pred (car lst))
        ((↓? pred) (cdr lst))
        lst)))))

; ⊠ :: (α -> Bool) -> [α] -> ⟨[α] [α]⟩
; Partition - split into [true, false] by predicate
(define ⊠ (lambda (pred) (lambda (lst)
  ((lambda (trues) ((lambda (falses)
    (cons trues falses))
    ((list-filter (lambda (x) (not (pred x)))) lst)))
   ((list-filter pred) lst)))))

; Helper for group-by: insert into association list
(define ⊡-insert (lambda (k) (lambda (v) (lambda (alist)
  (if (null? alist)
     (cons (cons k (cons v nil)) nil)
     (if (equal? k (car (car alist)))
        (cons (cons k (cons v (cdr (car alist))))
            (cdr alist))
        (cons (car alist) (((⊡-insert k) v) (cdr alist)))))))))

; apply-primitive :: (α -> β) -> [α] -> [⟨β [α]⟩]
; Group-by - group elements by key function
(define apply-primitive (lambda (keyfn) (lambda (lst)
  (((fold-left (lambda (acc) (lambda (x)
    ((lambda (k) (((⊡-insert k) x) acc))
     (keyfn x)))))
   nil)
   lst))))

; ============================================================================
; List Manipulation
; ============================================================================

; Helper for interleave
(define ⋈-helper (lambda (l1) (lambda (l2)
  (if (null? l1)
     l2
     (if (null? l2)
        l1
        (cons (car l1) (cons (car l2) ((⋈-helper (cdr l1)) (cdr l2)))))))))

; ⋈ :: [α] -> [α] -> [α]
; Interleave - merge lists alternating elements
(define ⋈ ⋈-helper)

; ∪ :: [α] -> [α]
; Deduplicate - remove duplicates (keeps first occurrence)
(define ∪ (lambda (lst)
  (((fold-left (lambda (acc) (lambda (x)
    (if ((∋ x) acc)
       acc
       ((⧺ (cons x nil)) acc)))))
   nil)
   lst)))

; generic-param :: (α -> Bool) -> [α] -> α | nil
; Find - first element matching predicate (or nil)
(define generic-param (lambda (pred) (lambda (lst)
  (if (null? lst)
     nil
     (if (pred (car lst))
        (car lst)
        ((generic-param pred) (cdr lst)))))))

; Helper for index-of
(define ⊳#-helper (lambda (elem) (lambda (lst) (lambda (idx)
  (if (null? lst)
     nil
     (if (equal? elem (car lst))
        idx
        (((⊳#-helper elem) (cdr lst)) (+ idx #1))))))))

; ⊳# :: α -> [α] -> ℕ | nil
; Index-of - position of first matching element (or nil)
(define ⊳# (lambda (elem) (lambda (lst)
  (((⊳#-helper elem) lst) #0))))

; ============================================================================
; Sorting
; ============================================================================

; Comparison wrappers (primitives need wrapping for currying)
(define <′ (lambda (a) (lambda (b) (< a b))))
(define >′ (lambda (a) (lambda (b) (> a b))))
(define ≤′ (lambda (a) (lambda (b) (<= a b))))
(define ≥′ (lambda (a) (lambda (b) (>= a b))))

; Helper for merge (merge sort)
(define ⊴-merge (lambda (cmp) (lambda (l1) (lambda (l2)
  (if (null? l1)
     l2
     (if (null? l2)
        l1
        (if ((cmp (car l2)) (car l1))
           (cons (car l2) (((⊴-merge cmp) l1) (cdr l2)))
           (cons (car l1) (((⊴-merge cmp) (cdr l1)) l2)))))))))

; Helper for merge sort
(define ⊴-sort (lambda (cmp) (lambda (lst)
  (if (null? lst)
     nil
     (if (null? (cdr lst))
        lst
        ((lambda (mid) ((lambda (left) ((lambda (right)
          (((⊴-merge cmp) ((⊴-sort cmp) left)) ((⊴-sort cmp) right)))
         ((↓ mid) lst)))
         ((↑ mid) lst)))
        (quotient (# lst) #2)))))))

; ⊴ :: (α -> α -> Bool) -> [α] -> [α]
; Sort - sort with comparison function (merge sort)
(define ⊴ ⊴-sort)

; ⊴< :: (α -> β) -> [α] -> [α]
; Sort-by - sort by key function (using <)
(define ⊴< (lambda (keyfn) (lambda (lst)
  ((⊴ (lambda (a) (lambda (b) (< (keyfn a) (keyfn b))))) lst))))

; ============================================================================
; Examples
; ============================================================================

; Sum: (fold-left + #0 (cons #1 (cons #2 (cons #3 nil))))  ; -> #6
; Product: (fold-left * #1 (cons #2 (cons #3 (cons #4 nil))))  ; -> #24
; Map double: (list-map (lambda (x) (* x #2)) (cons #1 (cons #2 (cons #3 nil))))
; Filter >5: (list-filter (lambda (x) (> x #5)) (cons #3 (cons #7 (cons #2 (cons #9 nil)))))
; Reverse: (⇄ (cons #1 (cons #2 (cons #3 nil))))
; Range: (range #5 #1)  ; -> (cons #1 (cons #2 (cons #3 (cons #4 nil))))
; Length: (# (cons #1 (cons #2 (cons #3 nil))))  ; -> #3

; Advanced examples:
; Unzip: (⊽ (cons (cons #1 #2) (cons (cons #3 #4) nil)))  ; -> ⟨⟨#1 ⟨#3 ∅⟩⟩ ⟨#2 ⟨#4 ∅⟩⟩⟩
; Transpose: (⊤⊥ (cons (cons #1 (cons #2 nil)) (cons (cons #3 (cons #4 nil)) nil)))  ; -> ⟨⟨#1 ⟨#3 ∅⟩⟩ ⟨#2 ⟨#4 ∅⟩⟩⟩
; Flatten: (deque (cons (cons #1 (cons #2 nil)) (cons (cons #3 nil) nil)))  ; -> ⟨#1 ⟨#2 ⟨#3 ∅⟩⟩⟩
; Take-while: (↑? (lambda (x) (< x #5)) (cons #1 (cons #2 (cons #7 nil))))  ; -> ⟨#1 ⟨#2 ∅⟩⟩
; Partition: (⊠ (lambda (x) (equal? (% x #2) #0)) (cons #1 (cons #2 (cons #3 (cons #4 nil)))))  ; -> ⟨⟨#2 ⟨#4 ∅⟩⟩ ⟨#1 ⟨#3 ∅⟩⟩⟩
; Interleave: (⋈ (cons #1 (cons #2 nil)) (cons #3 (cons #4 nil)))  ; -> ⟨#1 ⟨#3 ⟨#2 ⟨#4 ∅⟩⟩⟩⟩
; Sort: (⊴ < (cons #3 (cons #1 (cons #4 (cons #2 nil)))))  ; -> ⟨#1 ⟨#2 ⟨#3 ⟨#4 ∅⟩⟩⟩⟩
