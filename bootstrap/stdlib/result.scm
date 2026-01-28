;;; Result/Either Type - Railway-Oriented Programming
;;; Robust error handling with the Result ADT

;; =============================================================================
;; Type Definition
;; =============================================================================

;; Result type: Either error or success
;; Note: Variant definitions must be quoted!
(⊚≔ :Result (⌜ (:Err :error)) (⌜ (:Ok :value)))

;; =============================================================================
;; Constructors
;; =============================================================================

;; ok :: α → Result α
;; Wrap a value in Ok (success)
(≔ ok (λ (x)
  (⊚ :Result :Ok x)))

;; err :: α → Result α
;; Wrap a value in Err (failure)
(≔ err (λ (x)
  (⊚ :Result :Err x)))

;; =============================================================================
;; Predicates
;; =============================================================================

;; ok? :: Result α → 𝔹
;; Check if Result is Ok
(≔ ok? (λ (x)
  (⊚? x :Result :Ok)))

;; err? :: Result α → 𝔹
;; Check if Result is Err
(≔ err? (λ (x)
  (⊚? x :Result :Err)))

;; =============================================================================
;; Transformations
;; =============================================================================

;; map :: (α → β) → Result α → Result β
;; Transform the Ok value, leave Err unchanged
(≔ map (λ (f) (λ (r)
  (? (ok? r)
     (ok (f (⊚→ r :value)))
     r))))

;; map-err :: (α → β) → Result α → Result α
;; Transform the Err value, leave Ok unchanged
(≔ map-err (λ (f) (λ (r)
  (? (err? r)
     (err (f (⊚→ r :error)))
     r))))

;; flatmap :: (α → Result β) → Result α → Result β
;; Monadic bind - chain operations that return Results
;; Also known as: bind, >>=, chain, andThen (in some languages)
(≔ flatmap (λ (f) (λ (r)
  (? (ok? r)
     (f (⊚→ r :value))
     r))))

;; fold :: (α → γ) → (β → γ) → Result α → γ
;; Eliminate Result - apply ok-fn to Ok, err-fn to Err
(≔ fold (λ (ok-fn) (λ (err-fn) (λ (r)
  (? (ok? r)
     (ok-fn (⊚→ r :value))
     (err-fn (⊚→ r :error)))))))

;; =============================================================================
;; Extraction (Potentially Unsafe)
;; =============================================================================

;; unwrap :: Result α → α | ⚠
;; Extract Ok value or return error
;; UNSAFE: Caller must handle potential error
(≔ unwrap (λ (r)
  (? (ok? r)
     (⊚→ r :value)
     (⚠ :unwrap-err (⊚→ r :error)))))

;; unwrap-or :: α → Result α → α
;; Extract Ok value or return default
;; SAFE: Always returns a value
(≔ unwrap-or (λ (default) (λ (r)
  (? (ok? r)
     (⊚→ r :value)
     default))))

;; unwrap-err :: Result α → α | ⚠
;; Extract Err value or return error
;; UNSAFE: For testing/debugging only
(≔ unwrap-err (λ (r)
  (? (err? r)
     (⊚→ r :error)
     (⚠ :unwrap-ok (⊚→ r :value)))))

;; =============================================================================
;; Combinators
;; =============================================================================

;; and-then :: Result α → Result β → Result β
;; Return second Result if first is Ok, otherwise first Err
;; Short-circuits on first error
(≔ and-then (λ (r1) (λ (r2)
  (? (ok? r1)
     r2
     r1))))

;; or-else :: Result α → Result α → Result α
;; Return first Result if Ok, otherwise try second
;; Fallback mechanism for errors
(≔ or-else (λ (r1) (λ (r2)
  (? (ok? r1)
     r1
     r2))))
