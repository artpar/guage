; Enhanced Test Generation Library
; Generates property-based and example-based tests from type signatures

; Type-directed test value generation
; ≈⊙testgen-value :: :type-name -> value
; Generate test value for a given type
(define ≈⊙testgen-value (lambda (type)
  (match type (quote (
    (:ℕ #42)           ; Default number
    (:Bool #t)            ; Default boolean
    (:symbol :test)    ; Default symbol
    (_ #0)             ; Fallback
  )))))

; ≈⊙testgen-boundary-ℕ :: () -> [ℕ]
; Generate boundary values for numbers
(define ≈⊙testgen-boundary-ℕ (lambda
  (cons #0 (cons #1 (cons #-1 (cons #100 (cons #-100 nil)))))))

; ≈⊙testgen-examples-ℕ :: () -> [ℕ]
; Generate example values for numbers
(define ≈⊙testgen-examples-ℕ (lambda
  (cons #5 (cons #10 (cons #42 (cons #99 nil))))))

; Property test generators

; ≈⊙testgen-identity :: :op -> :type -> test
; Generate identity property test: op(x, identity) = x
(define ≈⊙testgen-identity (lambda (op) (lambda (id-val) (lambda (test-val)
  (cons :test-case (cons :test-identity-property
    (cons #t (cons (cons :equal? (cons (cons op (cons test-val (cons id-val nil))) (cons test-val nil))) nil))))))))

; ≈⊙testgen-commutative :: :op -> ℕ -> ℕ -> test
; Generate commutativity test: op(x, y) = op(y, x)
(define ≈⊙testgen-commutative (lambda (op) (lambda (x) (lambda (y)
  (cons :test-case (cons :test-commutative-property
    (cons #t (cons (cons :equal?
      (cons (cons op (cons x (cons y nil)))
      (cons (cons op (cons y (cons x nil))) nil))) nil))))))))

; ≈⊙testgen-associative :: :op -> ℕ -> ℕ -> ℕ -> test
; Generate associativity test: op(op(x, y), z) = op(x, op(y, z))
(define ≈⊙testgen-associative (lambda (op) (lambda (x) (lambda (y) (lambda (z)
  (cons :test-case (cons :test-associative-property
    (cons #t (cons (cons :equal?
      (cons (cons op (cons (cons op (cons x (cons y nil))) (cons z nil)))
      (cons (cons op (cons x (cons (cons op (cons y (cons z nil))) nil))) nil))) nil))))))))))

; ≈⊙testgen-idempotent :: :op -> α -> test
; Generate idempotency test: op(op(x)) = op(x)
(define ≈⊙testgen-idempotent (lambda (op) (lambda (x)
  (cons :test-case (cons :test-idempotent-property
    (cons #t (cons (cons :equal?
      (cons (cons op (cons (cons op (cons x nil)) nil))
      (cons (cons op (cons x nil)) nil))) nil)))))))

; ≈⊙testgen-inverse :: :op-forward -> :op-backward -> α -> test
; Generate inverse property test: backward(forward(x)) = x
(define ≈⊙testgen-inverse (lambda (fwd) (lambda (bwd) (lambda (x)
  (cons :test-case (cons :test-inverse-property
    (cons #t (cons (cons :equal?
      (cons (cons bwd (cons (cons fwd (cons x nil)) nil))
      (cons x nil))) nil))))))))

; Specific operator test suites

; ≈⊙testgen-arithmetic-op :: :op -> [test]
; Generate comprehensive tests for arithmetic operator
(define ≈⊙testgen-arithmetic-op (lambda (op)
  ; Test with boundary values
  (cons ((≈⊙testgen-commutative op) #0 #5)
  (cons ((≈⊙testgen-commutative op) #10 #20)
  (cons (((≈⊙testgen-associative op) #1) #2) #3)
  nil)))))

; ≈⊙testgen-predicate :: :pred -> [test]
; Generate tests for predicates (functions returning Bool)
(define ≈⊙testgen-predicate (lambda (pred)
  ; Test that result is boolean
  (cons (cons :test-case (cons :test-returns-boolean
    (cons #t (cons (cons :boolean? (cons (cons pred (cons #42 nil)) nil)) nil))))
  nil)))

; Examples:
; ((≈⊙testgen-identity (quote +)) #0 #42)  ; Test + identity
; ((≈⊙testgen-commutative (quote +)) #3 #7)  ; Test + commutativity
; (((≈⊙testgen-associative (quote +)) #1) #2) #3)  ; Test + associativity
