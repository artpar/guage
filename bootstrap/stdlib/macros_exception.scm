; Exception Handling Macros for Guage
; Provides syntactic sugar for working with ⚠ error values
; Built on pattern-based macros (⧉⊜)

; ============================================================================
; ⚡ (try-with) - Execute body, call handler if error
; ============================================================================
; ⌂: Execute expression, call handler function if result is error
; ∈: expr → (⚠ → α) → α
; Ex: (⚡ (⊘ #1 #0) (λ (e) :div-error)) → :div-error
; Ex: (⚡ (⊘ #6 #2) (λ (e) :error)) → #3

(⧉⊜ ⚡
  (($body $handler)
   ((λ (⚠⊙result)
      (? (⚠? ⚠⊙result)
         ($handler ⚠⊙result)
         ⚠⊙result))
    $body)))

; ============================================================================
; ⚡⊳ (try-or) - Execute with fallback default on error
; ============================================================================
; ⌂: Execute expression, return default value if error
; ∈: expr → α → α
; Ex: (⚡⊳ (⊘ #1 #0) #0) → #0 (division error → default)
; Ex: (⚡⊳ (⊘ #6 #2) #0) → #3 (success)

(⧉⊜ ⚡⊳
  (($body $default)
   ((λ (⚠⊙result)
      (? (⚠? ⚠⊙result)
         $default
         ⚠⊙result))
    $body)))

; ============================================================================
; ⚡∅ (ignore-errors) - Execute, return nil on any error
; ============================================================================
; ⌂: Execute expression, return nil if error
; ∈: expr → α | ∅
; Ex: (⚡∅ (⊘ #1 #0)) → ∅
; Ex: (⚡∅ (⊘ #6 #2)) → #3

(⧉⊜ ⚡∅
  (($body)
   ((λ (⚠⊙result)
      (? (⚠? ⚠⊙result)
         ∅
         ⚠⊙result))
    $body)))

; ============================================================================
; ⚡? (error-type?) - Check if error has specific type
; ============================================================================
; ⌂: Check if error value has specific error type
; ∈: α → :symbol → 𝔹
; Ex: (⚡? (⚠ :div-by-zero #0) :div-by-zero) → #t
; Ex: (⚡? (⚠ :not-found "x") :div-by-zero) → #f
; Ex: (⚡? #42 :any) → #f (not an error)

(≔ ⚡?-impl (λ (val error-type)
  (∧ (⚠? val)
     (≡ (⚠⊙ val) error-type))))

(⧉⊜ ⚡?
  (($val $error-type)
   (⚡?-impl $val $error-type)))

; ============================================================================
; ⚡⊙ (error-data) - Extract error data safely
; ============================================================================
; ⌂: Extract data from error, return nil if not an error
; ∈: α → α | ∅
; Ex: (⚡⊙ (⚠ :div-by-zero #0)) → #0
; Ex: (⚡⊙ #42) → ∅ (not an error)

(≔ ⚡⊙-impl (λ (val)
  (? (⚠? val)
     (⚠→ val)
     ∅)))

(⧉⊜ ⚡⊙
  (($val)
   (⚡⊙-impl $val)))

; ============================================================================
; ⚡⇒ (error-case) - Handle specific error types differently
; ============================================================================
; ⌂: Multi-clause error handling based on error type
; ∈: expr → (error-type → handler) ... → α
; Ex: (⚡⇒ (⊘ #1 #0)
;        (:div-by-zero (λ (d) :zero))
;        (:not-found (λ (d) :missing))
;        (:else (λ (d) :other)))

; Helper: Find matching error handler
(≔ ⚡⇒-find-handler (λ (error-type clauses)
  (? (∅? clauses)
     ∅  ; No handler found
     ((λ (clause)
        (? (≡ (◁ clause) error-type)
           (◁ (▷ clause))  ; Return handler
           (? (≡ (◁ clause) :else)
              (◁ (▷ clause))  ; :else matches anything
              (⚡⇒-find-handler error-type (▷ clauses)))))
      (◁ clauses)))))

; Helper: Apply error handling
(≔ ⚡⇒-apply (λ (result clauses)
  (? (⚠? result)
     ((λ (handler)
        (? (∅? handler)
           result  ; No handler, return error unchanged
           (handler (⚠→ result))))
      (⚡⇒-find-handler (⚠⊙ result) clauses))
     result)))  ; Not an error, return as-is

; Note: Full variadic error-case requires runtime clause building
; For now, provide fixed arity versions

(⧉⊜ ⚡⇒
  ; 1 clause
  (($body (($type1 $handler1)))
   (⚡⇒-apply $body (⟨⟩ (⟨⟩ $type1 (⟨⟩ $handler1 ∅)) ∅)))
  ; 2 clauses
  (($body (($type1 $handler1)) (($type2 $handler2)))
   (⚡⇒-apply $body (⟨⟩ (⟨⟩ $type1 (⟨⟩ $handler1 ∅))
                    (⟨⟩ (⟨⟩ $type2 (⟨⟩ $handler2 ∅)) ∅))))
  ; 3 clauses
  (($body (($type1 $handler1)) (($type2 $handler2)) (($type3 $handler3)))
   (⚡⇒-apply $body (⟨⟩ (⟨⟩ $type1 (⟨⟩ $handler1 ∅))
                    (⟨⟩ (⟨⟩ $type2 (⟨⟩ $handler2 ∅))
                    (⟨⟩ (⟨⟩ $type3 (⟨⟩ $handler3 ∅)) ∅))))))

; ============================================================================
; ⚡⟲ (try-finally) - Execute with cleanup
; ============================================================================
; ⌂: Execute body, run cleanup regardless of success/error, return body result
; ∈: expr → expr → α
; Ex: (⚡⟲ (⊘ #6 #2) (⟲ :cleanup)) → #3 (prints :cleanup)
; Ex: (⚡⟲ (⊘ #1 #0) (⟲ :cleanup)) → ⚠:div-by-zero (prints :cleanup)

; Note: Cleanup is evaluated for side effects, body result is returned
(⧉⊜ ⚡⟲
  (($body $cleanup)
   ((λ (⚠⊙result)
      ((λ (⚠⊙ignored)
         ⚠⊙result)
       $cleanup))
    $body)))

; ============================================================================
; ⚡↺ (retry) - Retry on error up to n times
; ============================================================================
; ⌂: Retry expression on error, up to max attempts
; ∈: ℕ → expr → α
; Ex: (⚡↺ #3 (may-fail)) - Try up to 3 times

; Helper: Retry implementation
(≔ ⚡↺-impl (λ (n thunk)
  (? (≡ n #0)
     (⚠ :retry-exhausted #0)
     ((λ (result)
        (? (⚠? result)
           (⚡↺-impl (⊖ n #1) thunk)
           result))
      (thunk)))))

; Note: Body must be wrapped in thunk since macro expands before evaluation
(⧉⊜ ⚡↺
  (($n $body)
   (⚡↺-impl $n (λ () $body))))

; ============================================================================
; ⚡∧ (all-succeed) - Execute all, fail if any fails
; ============================================================================
; ⌂: Execute expressions in sequence, return first error or last value
; ∈: expr ... → α | ⚠
; Ex: (⚡∧ (⊘ #6 #2) (⊕ #1 #1)) → #2 (both succeed)
; Ex: (⚡∧ (⊘ #1 #0) (⊕ #1 #1)) → ⚠:div-by-zero (first fails)

(⧉⊜ ⚡∧
  ; 1 expression
  (($e1)
   $e1)
  ; 2 expressions
  (($e1 $e2)
   ((λ (⚠⊙r1)
      (? (⚠? ⚠⊙r1)
         ⚠⊙r1
         $e2))
    $e1))
  ; 3+ expressions using rest pattern
  (($e1 $rest ...)
   ((λ (⚠⊙r1)
      (? (⚠? ⚠⊙r1)
         ⚠⊙r1
         (⚡∧ $rest ...)))
    $e1)))

; ============================================================================
; ⚡∨ (first-success) - Return first successful result
; ============================================================================
; ⌂: Try expressions in sequence, return first non-error
; ∈: expr ... → α | ⚠
; Ex: (⚡∨ (⊘ #1 #0) (⊕ #1 #1)) → #2 (first fails, second succeeds)
; Ex: (⚡∨ (⊘ #1 #0) (⊘ #1 #0)) → ⚠:div-by-zero (all fail)

(⧉⊜ ⚡∨
  ; 1 expression
  (($e1)
   $e1)
  ; 2 expressions
  (($e1 $e2)
   ((λ (⚠⊙r1)
      (? (⚠? ⚠⊙r1)
         $e2
         ⚠⊙r1))
    $e1))
  ; 3+ expressions using rest pattern
  (($e1 $rest ...)
   ((λ (⚠⊙r1)
      (? (⚠? ⚠⊙r1)
         (⚡∨ $rest ...)
         ⚠⊙r1))
    $e1)))

; ============================================================================
; ⚡↦ (map-errors) - Transform errors in list, keeping successes
; ============================================================================
; ⌂: Apply function to list, replacing errors with handler result
; ∈: (α → β) → (⚠ → β) → [α] → [β]

(≔ ⚡↦-impl (λ (f handler lst)
  (? (∅? lst)
     ∅
     ((λ (result)
        (⟨⟩ (? (⚠? result) (handler result) result)
            (⚡↦-impl f handler (▷ lst))))
      (f (◁ lst))))))

; ============================================================================
; Module complete - Exception handling utilities available
; ============================================================================
