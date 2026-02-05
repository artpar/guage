;;; Result/Either Type - Railway-Oriented Programming
;;; Robust error handling with the Result ADT

;; =============================================================================
;; Type Definition
;; =============================================================================

;; Result type: Either error or success
;; Note: Variant definitions must be quoted!
(adt-define :Result (quote (:Err :error)) (quote (:Ok :value)))

;; =============================================================================
;; Constructors
;; =============================================================================

;; ok :: α -> Result α
;; Wrap a value in Ok (success)
(define ok (lambda (x)
  (adt-create :Result :Ok x)))

;; err :: α -> Result α
;; Wrap a value in Err (failure)
(define err (lambda (x)
  (adt-create :Result :Err x)))

;; =============================================================================
;; Predicates
;; =============================================================================

;; ok? :: Result α -> Bool
;; Check if Result is Ok
(define ok? (lambda (x)
  (adt? x :Result :Ok)))

;; err? :: Result α -> Bool
;; Check if Result is Err
(define err? (lambda (x)
  (adt? x :Result :Err)))

;; =============================================================================
;; Transformations
;; =============================================================================

;; map :: (α -> β) -> Result α -> Result β
;; Transform the Ok value, leave Err unchanged
(define map (lambda (f) (lambda (r)
  (if (ok? r)
     (ok (f (adt-get r :value)))
     r))))

;; map-err :: (α -> β) -> Result α -> Result α
;; Transform the Err value, leave Ok unchanged
(define map-err (lambda (f) (lambda (r)
  (if (err? r)
     (err (f (adt-get r :error)))
     r))))

;; flatmap :: (α -> Result β) -> Result α -> Result β
;; Monadic bind - chain operations that return Results
;; Also known as: bind, >>=, chain, andThen (in some languages)
(define flatmap (lambda (f) (lambda (r)
  (if (ok? r)
     (f (adt-get r :value))
     r))))

;; fold :: (α -> γ) -> (β -> γ) -> Result α -> γ
;; Eliminate Result - apply ok-fn to Ok, err-fn to Err
(define fold (lambda (ok-fn) (lambda (err-fn) (lambda (r)
  (if (ok? r)
     (ok-fn (adt-get r :value))
     (err-fn (adt-get r :error)))))))

;; =============================================================================
;; Extraction (Potentially Unsafe)
;; =============================================================================

;; unwrap :: Result α -> α | error
;; Extract Ok value or return error
;; UNSAFE: Caller must handle potential error
(define unwrap (lambda (r)
  (if (ok? r)
     (adt-get r :value)
     (error :unwrap-err (adt-get r :error)))))

;; unwrap-or :: α -> Result α -> α
;; Extract Ok value or return default
;; SAFE: Always returns a value
(define unwrap-or (lambda (default) (lambda (r)
  (if (ok? r)
     (adt-get r :value)
     default))))

;; unwrap-err :: Result α -> α | error
;; Extract Err value or return error
;; UNSAFE: For testing/debugging only
(define unwrap-err (lambda (r)
  (if (err? r)
     (adt-get r :error)
     (error :unwrap-ok (adt-get r :value)))))

;; =============================================================================
;; Combinators
;; =============================================================================

;; and-then :: Result α -> Result β -> Result β
;; Return second Result if first is Ok, otherwise first Err
;; Short-circuits on first error
(define and-then (lambda (r1) (lambda (r2)
  (if (ok? r1)
     r2
     r1))))

;; or-else :: Result α -> Result α -> Result α
;; Return first Result if Ok, otherwise try second
;; Fallback mechanism for errors
(define or-else (lambda (r1) (lambda (r2)
  (if (ok? r1)
     r1
     r2))))
