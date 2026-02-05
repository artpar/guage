; ═══════════════════════════════════════════════════════════════
; Guage Standard Library: Sorting Algorithms
; ═══════════════════════════════════════════════════════════════

; Copy from old file up to line 159
; ═══════════════════════════════════════════════════════════════
; Guage Standard Library: Sorting Algorithms
; ═══════════════════════════════════════════════════════════════
;
; Provides multiple sorting algorithms with different trade-offs:
; - Bubble sort: Simple, O(n²), stable
; - Insertion sort: Good for small/nearly-sorted, O(n²), stable
; - Merge sort: Guaranteed O(n log n), stable, uses extra space
; - Quicksort: Average O(n log n), unstable, in-place
;
; Plus utilities for custom comparators and sorting checks.
;
; ═══════════════════════════════════════════════════════════════

; ────────────────────────────────────────────────────────────────
; COMPARATORS
; ────────────────────────────────────────────────────────────────

; Default numeric comparator: a <= b
(define ⊙≤ (lambda (a) (lambda (b) (<= a b))))

; Reverse comparator: b <= a
(define ⊙≥ (lambda (a) (lambda (b) (>= a b))))

; ────────────────────────────────────────────────────────────────
; UTILITY FUNCTIONS
; ────────────────────────────────────────────────────────────────

; Check if list is sorted according to comparator
; ⊙⊢→ : (α -> α -> Bool) -> [α] -> Bool
(define ⊙⊢→ (lambda (cmp) (lambda (lst)
  (if (null? lst)
     #t
     (if (null? (cdr lst))
        #t
        (if ((cmp (car lst)) (car (cdr lst)))
           ((⊙⊢→ cmp) (cdr lst))
           #f))))))

; Check if list is sorted (ascending)
; ⊙⊢ : [ℕ] -> Bool
(define ⊙⊢ (⊙⊢→ ⊙≤))

; ────────────────────────────────────────────────────────────────
; BUBBLE SORT
; ────────────────────────────────────────────────────────────────

(define ⊙bubble-pass (lambda (cmp) (lambda (lst)
  (if (null? lst)
     nil
     (if (null? (cdr lst))
        lst
        (if ((cmp (car lst)) (car (cdr lst)))
           (cons (car lst) ((⊙bubble-pass cmp) (cdr lst)))
           (cons (car (cdr lst)) ((⊙bubble-pass cmp) (cons (car lst) (cdr (cdr lst)))))))))))

(define ⊙bubble→ (lambda (cmp) (lambda (lst)
  (if (null? lst)
     nil
     (if ((⊙⊢→ cmp) lst)
        lst
        ((⊙bubble→ cmp) ((⊙bubble-pass cmp) lst)))))))

(define ⊙bubble (⊙bubble→ ⊙≤))

; ────────────────────────────────────────────────────────────────
; INSERTION SORT
; ────────────────────────────────────────────────────────────────

(define ⊙insert-sorted (lambda (cmp) (lambda (x) (lambda (lst)
  (if (null? lst)
     (cons x nil)
     (if ((cmp x) (car lst))
        (cons x lst)
        (cons (car lst) (((⊙insert-sorted cmp) x) (cdr lst)))))))))

(define ⊙insertion→ (lambda (cmp) (lambda (lst)
  (if (null? lst)
     nil
     (((⊙insert-sorted cmp) (car lst)) ((⊙insertion→ cmp) (cdr lst)))))))

(define ⊙insertion (⊙insertion→ ⊙≤))

; ────────────────────────────────────────────────────────────────
; MERGE SORT
; ────────────────────────────────────────────────────────────────

; Merge two sorted lists
(define ⊙merge (lambda (cmp) (lambda (lst1) (lambda (lst2)
  (if (null? lst1)
     lst2
     (if (null? lst2)
        lst1
        (if ((cmp (car lst1)) (car lst2))
           (cons (car lst1) (((⊙merge cmp) (cdr lst1)) lst2))
           (cons (car lst2) (((⊙merge cmp) lst1) (cdr lst2))))))))))

; Reverse a list
(define ⊙rev-helper (lambda (acc) (lambda (lst)
  (if (null? lst)
     acc
     ((⊙rev-helper (cons (car lst) acc)) (cdr lst))))))

(define ⊙rev (lambda (lst)
  ((⊙rev-helper nil) lst)))

; Split list in half
(define ⊙split-helper (lambda (prefix) (lambda (slow) (lambda (fast)
  (if (null? fast)
     (cons prefix slow)
     (if (null? (cdr fast))
        (cons prefix slow)
        (((⊙split-helper (cons (car slow) prefix)) (cdr slow)) (cdr (cdr fast)))))))))

(define ⊙split (lambda (lst)
  (((⊙split-helper nil) lst) lst)))

; Merge sort with custom comparator
(define ⊙mergesort→ (lambda (cmp) (lambda (lst)
  (if (null? lst)
     nil
     (if (null? (cdr lst))
        lst
        ((lambda (halves)
           ((lambda (left)
              ((lambda (right)
                 (((⊙merge cmp) ((⊙mergesort→ cmp) left)) ((⊙mergesort→ cmp) right)))
               (cdr halves)))
            (⊙rev (car halves))))
         (⊙split lst)))))))

(define ⊙mergesort (⊙mergesort→ ⊙≤))

; ────────────────────────────────────────────────────────────────
; QUICKSORT
; ────────────────────────────────────────────────────────────────

; Helper to append two lists
(define ⊙append (lambda (lst1) (lambda (lst2)
  (if (null? lst1)
     lst2
     (cons (car lst1) ((⊙append (cdr lst1)) lst2))))))

; Helper to concatenate three lists
(define ⊙concat-three (lambda (a) (lambda (b) (lambda (c)
  ((⊙append ((⊙append a) b)) c)))))

; Partition list around pivot
(define ⊙partition-helper (lambda (cmp) (lambda (pivot) (lambda (lst) (lambda (less) (lambda (greater)
  (if (null? lst)
     (cons (⊙rev less) (⊙rev greater))
     (if ((cmp (car lst)) pivot)
        (((((⊙partition-helper cmp) pivot) (cdr lst)) (cons (car lst) less)) greater)
        (((((⊙partition-helper cmp) pivot) (cdr lst)) less) (cons (car lst) greater))))))))))

(define ⊙partition (lambda (cmp) (lambda (pivot) (lambda (lst)
  (((((⊙partition-helper cmp) pivot) lst) nil) nil)))))

; Quicksort with custom comparator
; Quicksort with custom comparator
(define ⊙quicksort→ (lambda (cmp) (lambda (lst)
  (if (null? lst)
     nil
     (if (null? (cdr lst))
        lst
        ((lambda (pivot)
           ((lambda (parts)
              ((lambda (less)
                 ((lambda (greater)
                    (((⊙concat-three ((⊙quicksort→ cmp) less)) (cons pivot nil)) ((⊙quicksort→ cmp) greater)))
                  (cdr parts)))
               (car parts)))
            (((⊙partition cmp) pivot) (cdr lst))))
         (car lst)))))))

(define ⊙quicksort (⊙quicksort→ ⊙≤))

; ────────────────────────────────────────────────────────────────
; DEFAULT SORT
; ────────────────────────────────────────────────────────────────

(define ⊙sort→ ⊙mergesort→)
(define ⊙sort ⊙mergesort)

; ────────────────────────────────────────────────────────────────
; HIGHER-ORDER SORTING
; ────────────────────────────────────────────────────────────────

(define ⊙sortby→ (lambda (key-fn) (lambda (cmp) (lambda (lst)
  ((⊙sort→ (lambda (a) (lambda (b) ((cmp (key-fn a)) (key-fn b))))) lst)))))

(define ⊙sortby (lambda (key-fn) ((⊙sortby→ key-fn) ⊙≤)))

; ═══════════════════════════════════════════════════════════════
; END OF SORT LIBRARY
; ═══════════════════════════════════════════════════════════════
