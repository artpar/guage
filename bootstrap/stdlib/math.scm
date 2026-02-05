; Guage Standard Library: Math Utilities
; Pure symbolic names, named parameters (converted to De Bruijn internally)

; ============================================================================
; List Aggregations
; ============================================================================

; ⊕⊕ :: [ℕ] -> ℕ
; Sum - sum of all numbers in list
(define ⊕⊕ (lambda (lst)
  (((fold-left (lambda (a) (lambda (b) (+ a b)))) #0) lst)))

; ⊗⊗ :: [ℕ] -> ℕ
; Product - product of all numbers in list
(define ⊗⊗ (lambda (lst)
  (((fold-left (lambda (a) (lambda (b) (* a b)))) #1) lst)))

; ============================================================================
; Binary Comparisons
; ============================================================================

; ↥ :: ℕ -> ℕ -> ℕ
; Max - maximum of two numbers
(define ↥ (lambda (b) (lambda (a)
  (if (> a b) a b))))

; ↧ :: ℕ -> ℕ -> ℕ
; Min - minimum of two numbers
(define ↧ (lambda (b) (lambda (a)
  (if (< a b) a b))))

; ============================================================================
; List Min/Max
; ============================================================================

; ↥↥ :: [ℕ] -> ℕ | nil
; Maximum - maximum value in list (returns nil if empty)
(define ↥↥ (lambda (lst)
  (if (null? lst)
     nil
     (((fold-left (lambda (a) (lambda (b) ((↥ b) a)))) (car lst)) (cdr lst)))))

; ↧↧ :: [ℕ] -> ℕ | nil
; Minimum - minimum value in list (returns nil if empty)
(define ↧↧ (lambda (lst)
  (if (null? lst)
     nil
     (((fold-left (lambda (a) (lambda (b) ((↧ b) a)))) (car lst)) (cdr lst)))))

; ============================================================================
; Examples
; ============================================================================

; Sum: (⊕⊕ (cons #1 (cons #2 (cons #3 nil))))  ; -> #6
; Product: (⊗⊗ (cons #2 (cons #3 (cons #4 nil))))  ; -> #24
; Max: ((↥ #5) #3)  ; -> #5
; Min: ((↧ #5) #3)  ; -> #3
; Maximum: (↥↥ (cons #3 (cons #7 (cons #2 (cons #9 nil)))))  ; -> #9
; Minimum: (↧↧ (cons #3 (cons #7 (cons #2 (cons #9 nil)))))  ; -> #2
