; Exception Handling Macros for Guage
; Provides syntactic sugar for working with error error values
; Built on pattern-based macros (macro-rules)

; ============================================================================
; try-with (try-with) - Execute body, call handler if error
; ============================================================================
; ⌂: Execute expression, call handler function if result is error
; ∈: expr -> (error -> α) -> α
; Ex: (try-with (/ #1 #0) (lambda (e) :div-error)) -> :div-error
; Ex: (try-with (/ #6 #2) (lambda (e) :error)) -> #3

(macro-rules try-with
  (($body $handler)
   ((lambda (err-result)
      (if (error? err-result)
         ($handler err-result)
         err-result))
    $body)))

; ============================================================================
; try-or (try-or) - Execute with fallback default on error
; ============================================================================
; ⌂: Execute expression, return default value if error
; ∈: expr -> α -> α
; Ex: (try-or (/ #1 #0) #0) -> #0 (division error -> default)
; Ex: (try-or (/ #6 #2) #0) -> #3 (success)

(macro-rules try-or
  (($body $default)
   ((lambda (err-result)
      (if (error? err-result)
         $default
         err-result))
    $body)))

; ============================================================================
; ignore-errors (ignore-errors) - Execute, return nil on any error
; ============================================================================
; ⌂: Execute expression, return nil if error
; ∈: expr -> α | nil
; Ex: (ignore-errors (/ #1 #0)) -> nil
; Ex: (ignore-errors (/ #6 #2)) -> #3

(macro-rules ignore-errors
  (($body)
   ((lambda (err-result)
      (if (error? err-result)
         nil
         err-result))
    $body)))

; ============================================================================
; try (error-type?) - Check if error has specific type
; ============================================================================
; ⌂: Check if error value has specific error type
; ∈: α -> :symbol -> Bool
; Ex: (try (error :div-by-zero #0) :div-by-zero) -> #t
; Ex: (try (error :not-found "x") :div-by-zero) -> #f
; Ex: (try #42 :any) -> #f (not an error)

(define error-type-impl (lambda (val expected-type)
  (and (error? val)
     (equal? (error-type val) expected-type))))

(macro-rules try
  (($val $error-type)
   (error-type-impl $val $error-type)))

; ============================================================================
; error-data-safe (error-data) - Extract error data safely
; ============================================================================
; ⌂: Extract data from error, return nil if not an error
; ∈: α -> α | nil
; Ex: (error-data-safe (error :div-by-zero #0)) -> #0
; Ex: (error-data-safe #42) -> nil (not an error)

(define error-data-safe-impl (lambda (val)
  (if (error? val)
     (error-data val)
     nil)))

(macro-rules error-data-safe
  (($val)
   (error-data-safe-impl $val)))

; ============================================================================
; match-error (error-case) - Handle specific error types differently
; ============================================================================
; ⌂: Multi-clause error handling based on error type
; ∈: expr -> (error-type -> handler) ... -> α
; Ex: (match-error (/ #1 #0)
;        (:div-by-zero (lambda (d) :zero))
;        (:not-found (lambda (d) :missing))
;        (:else (lambda (d) :other)))

; Helper: Find matching error handler
(define match-error-find-handler (lambda (error-type clauses)
  (if (null? clauses)
     nil  ; No handler found
     ((lambda (clause)
        (if (equal? (car clause) error-type)
           (car (cdr clause))  ; Return handler
           (if (equal? (car clause) :else)
              (car (cdr clause))  ; :else matches anything
              (match-error-find-handler error-type (cdr clauses)))))
      (car clauses)))))

; Helper: Apply error handling
(define match-error-apply (lambda (result clauses)
  (if (error? result)
     ((lambda (handler)
        (if (null? handler)
           result  ; No handler, return error unchanged
           (handler (error-data result))))
      (match-error-find-handler (error-type result) clauses))
     result)))  ; Not an error, return as-is

; Note: Full variadic error-case requires runtime clause building
; For now, provide fixed arity versions

(macro-rules match-error
  ; 1 clause
  (($body (($type1 $handler1)))
   (match-error-apply $body (cons (cons $type1 (cons $handler1 nil)) nil)))
  ; 2 clauses
  (($body (($type1 $handler1)) (($type2 $handler2)))
   (match-error-apply $body (cons (cons $type1 (cons $handler1 nil))
                    (cons (cons $type2 (cons $handler2 nil)) nil))))
  ; 3 clauses
  (($body (($type1 $handler1)) (($type2 $handler2)) (($type3 $handler3)))
   (match-error-apply $body (cons (cons $type1 (cons $handler1 nil))
                    (cons (cons $type2 (cons $handler2 nil))
                    (cons (cons $type3 (cons $handler3 nil)) nil))))))

; ============================================================================
; trace-error (try-finally) - Execute with cleanup
; ============================================================================
; ⌂: Execute body, run cleanup regardless of success/error, return body result
; ∈: expr -> expr -> α
; Ex: (trace-error (/ #6 #2) (trace :cleanup)) -> #3 (prints :cleanup)
; Ex: (trace-error (/ #1 #0) (trace :cleanup)) -> ⚠:div-by-zero (prints :cleanup)

; Note: Cleanup is evaluated for side effects, body result is returned
(macro-rules trace-error
  (($body $cleanup)
   ((lambda (err-result)
      ((lambda (err-ignored)
         err-result)
       $cleanup))
    $body)))

; ============================================================================
; retry (retry) - Retry on error up to n times
; ============================================================================
; ⌂: Retry expression on error, up to max attempts
; ∈: ℕ -> expr -> α
; Ex: (retry #3 (may-fail)) - Try up to 3 times

; Helper: Retry implementation
(define retry-impl (lambda (n thunk)
  (if (equal? n #0)
     (error :retry-exhausted #0)
     ((lambda (result)
        (if (error? result)
           (retry-impl (- n #1) thunk)
           result))
      (thunk)))))

; Note: Body must be wrapped in thunk since macro expands before evaluation
(macro-rules retry
  (($n $body)
   (retry-impl $n (lambda () $body))))

; ============================================================================
; and-errors (all-succeed) - Execute all, fail if any fails
; ============================================================================
; ⌂: Execute expressions in sequence, return first error or last value
; ∈: expr ... -> α | error
; Ex: (and-errors (/ #6 #2) (+ #1 #1)) -> #2 (both succeed)
; Ex: (and-errors (/ #1 #0) (+ #1 #1)) -> ⚠:div-by-zero (first fails)

(macro-rules and-errors
  ; 1 expression
  (($e1)
   $e1)
  ; 2 expressions
  (($e1 $e2)
   ((lambda (err-r1)
      (if (error? err-r1)
         err-r1
         $e2))
    $e1))
  ; 3+ expressions using rest pattern
  (($e1 $rest ...)
   ((lambda (err-r1)
      (if (error? err-r1)
         err-r1
         (and-errors $rest ...)))
    $e1)))

; ============================================================================
; or-errors (first-success) - Return first successful result
; ============================================================================
; ⌂: Try expressions in sequence, return first non-error
; ∈: expr ... -> α | error
; Ex: (or-errors (/ #1 #0) (+ #1 #1)) -> #2 (first fails, second succeeds)
; Ex: (or-errors (/ #1 #0) (/ #1 #0)) -> ⚠:div-by-zero (all fail)

(macro-rules or-errors
  ; 1 expression
  (($e1)
   $e1)
  ; 2 expressions
  (($e1 $e2)
   ((lambda (err-r1)
      (if (error? err-r1)
         $e2
         err-r1))
    $e1))
  ; 3+ expressions using rest pattern
  (($e1 $rest ...)
   ((lambda (err-r1)
      (if (error? err-r1)
         (or-errors $rest ...)
         err-r1))
    $e1)))

; ============================================================================
; map-errors (map-errors) - Transform errors in list, keeping successes
; ============================================================================
; ⌂: Apply function to list, replacing errors with handler result
; ∈: (α -> β) -> (error -> β) -> [α] -> [β]

(define map-errors-impl (lambda (f handler lst)
  (if (null? lst)
     nil
     ((lambda (result)
        (cons (if (error? result) (handler result) result)
            (map-errors-impl f handler (cdr lst))))
      (f (car lst))))))

; ============================================================================
; Module complete - Exception handling utilities available
; ============================================================================
