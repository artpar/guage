; Enhanced Test Generation Library
; Generates property-based and example-based tests from type signatures

; Type-directed test value generation
; ≈⊙testgen-value :: :type-name → value
; Generate test value for a given type
(≔ ≈⊙testgen-value (λ (type)
  (∇ type (⌜ (
    (:ℕ #42)           ; Default number
    (:𝔹 #t)            ; Default boolean
    (:symbol :test)    ; Default symbol
    (_ #0)             ; Fallback
  )))))

; ≈⊙testgen-boundary-ℕ :: () → [ℕ]
; Generate boundary values for numbers
(≔ ≈⊙testgen-boundary-ℕ (λ
  (⟨⟩ #0 (⟨⟩ #1 (⟨⟩ #-1 (⟨⟩ #100 (⟨⟩ #-100 ∅)))))))

; ≈⊙testgen-examples-ℕ :: () → [ℕ]
; Generate example values for numbers
(≔ ≈⊙testgen-examples-ℕ (λ
  (⟨⟩ #5 (⟨⟩ #10 (⟨⟩ #42 (⟨⟩ #99 ∅))))))

; Property test generators

; ≈⊙testgen-identity :: :op → :type → test
; Generate identity property test: op(x, identity) = x
(≔ ≈⊙testgen-identity (λ (op) (λ (id-val) (λ (test-val)
  (⟨⟩ :⊨ (⟨⟩ :test-identity-property
    (⟨⟩ #t (⟨⟩ (⟨⟩ :≡ (⟨⟩ (⟨⟩ op (⟨⟩ test-val (⟨⟩ id-val ∅))) (⟨⟩ test-val ∅))) ∅))))))))

; ≈⊙testgen-commutative :: :op → ℕ → ℕ → test
; Generate commutativity test: op(x, y) = op(y, x)
(≔ ≈⊙testgen-commutative (λ (op) (λ (x) (λ (y)
  (⟨⟩ :⊨ (⟨⟩ :test-commutative-property
    (⟨⟩ #t (⟨⟩ (⟨⟩ :≡
      (⟨⟩ (⟨⟩ op (⟨⟩ x (⟨⟩ y ∅)))
      (⟨⟩ (⟨⟩ op (⟨⟩ y (⟨⟩ x ∅))) ∅))) ∅))))))))

; ≈⊙testgen-associative :: :op → ℕ → ℕ → ℕ → test
; Generate associativity test: op(op(x, y), z) = op(x, op(y, z))
(≔ ≈⊙testgen-associative (λ (op) (λ (x) (λ (y) (λ (z)
  (⟨⟩ :⊨ (⟨⟩ :test-associative-property
    (⟨⟩ #t (⟨⟩ (⟨⟩ :≡
      (⟨⟩ (⟨⟩ op (⟨⟩ (⟨⟩ op (⟨⟩ x (⟨⟩ y ∅))) (⟨⟩ z ∅)))
      (⟨⟩ (⟨⟩ op (⟨⟩ x (⟨⟩ (⟨⟩ op (⟨⟩ y (⟨⟩ z ∅))) ∅))) ∅))) ∅))))))))))

; ≈⊙testgen-idempotent :: :op → α → test
; Generate idempotency test: op(op(x)) = op(x)
(≔ ≈⊙testgen-idempotent (λ (op) (λ (x)
  (⟨⟩ :⊨ (⟨⟩ :test-idempotent-property
    (⟨⟩ #t (⟨⟩ (⟨⟩ :≡
      (⟨⟩ (⟨⟩ op (⟨⟩ (⟨⟩ op (⟨⟩ x ∅)) ∅))
      (⟨⟩ (⟨⟩ op (⟨⟩ x ∅)) ∅))) ∅)))))))

; ≈⊙testgen-inverse :: :op-forward → :op-backward → α → test
; Generate inverse property test: backward(forward(x)) = x
(≔ ≈⊙testgen-inverse (λ (fwd) (λ (bwd) (λ (x)
  (⟨⟩ :⊨ (⟨⟩ :test-inverse-property
    (⟨⟩ #t (⟨⟩ (⟨⟩ :≡
      (⟨⟩ (⟨⟩ bwd (⟨⟩ (⟨⟩ fwd (⟨⟩ x ∅)) ∅))
      (⟨⟩ x ∅))) ∅))))))))

; Specific operator test suites

; ≈⊙testgen-arithmetic-op :: :op → [test]
; Generate comprehensive tests for arithmetic operator
(≔ ≈⊙testgen-arithmetic-op (λ (op)
  ; Test with boundary values
  (⟨⟩ ((≈⊙testgen-commutative op) #0 #5)
  (⟨⟩ ((≈⊙testgen-commutative op) #10 #20)
  (⟨⟩ (((≈⊙testgen-associative op) #1) #2) #3)
  ∅)))))

; ≈⊙testgen-predicate :: :pred → [test]
; Generate tests for predicates (functions returning 𝔹)
(≔ ≈⊙testgen-predicate (λ (pred)
  ; Test that result is boolean
  (⟨⟩ (⟨⟩ :⊨ (⟨⟩ :test-returns-boolean
    (⟨⟩ #t (⟨⟩ (⟨⟩ :𝔹? (⟨⟩ (⟨⟩ pred (⟨⟩ #42 ∅)) ∅)) ∅))))
  ∅)))

; Examples:
; ((≈⊙testgen-identity (⌜ ⊕)) #0 #42)  ; Test ⊕ identity
; ((≈⊙testgen-commutative (⌜ ⊕)) #3 #7)  ; Test ⊕ commutativity
; (((≈⊙testgen-associative (⌜ ⊕)) #1) #2) #3)  ; Test ⊕ associativity
