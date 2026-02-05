; Guage Standard Library: Option and Result Types
; Error handling without exceptions - values as boundaries!

; ============================================================================
; Type Definitions
; ============================================================================

; Option type - represents optional values (may or may not exist)
; :None - no value present
; :Some - value present
(adt-define :Option (quote (:None)) (quote (:Some :value)))

; Result type - represents operations that can succeed or fail
; :Ok - success with value
; :Err - failure with error
(adt-define :Result (quote (:Ok :value)) (quote (:Err :error)))

; ============================================================================
; Option Constructors (for convenience)
; ============================================================================

; ⊙◇ :: α -> Option α
; Some - wrap value in Some
(define ⊙◇ (lambda (value)
  (adt-create :Option :Some value)))

; ⊙∅ :: Option α
; None - the none value
(define ⊙∅ (adt-create :Option :None))

; ============================================================================
; Result Constructors (for convenience)
; ============================================================================

; ⊙✓ :: α -> Result α β
; Ok - wrap value in success
(define ⊙✓ (lambda (value)
  (adt-create :Result :Ok value)))

; ⊙✗ :: β -> Result α β
; Err - wrap error in failure
(define ⊙✗ (lambda (error)
  (adt-create :Result :Err error)))

; ============================================================================
; Option Predicates
; ============================================================================

; struct? :: Option α -> Bool
; Is-some - check if option contains a value
(define struct? (lambda (opt) (adt? opt :Option :Some)))

; ⊙∅? :: Option α -> Bool
; Is-none - check if option is None
(define ⊙∅? (lambda (opt) (adt? opt :Option :None)))

; ============================================================================
; Result Predicates
; ============================================================================

; ⊙✓? :: Result α β -> Bool
; Is-ok - check if result is success
(define ⊙✓? (lambda (res) (adt? res :Result :Ok)))

; ⊙✗? :: Result α β -> Bool
; Is-err - check if result is failure
(define ⊙✗? (lambda (res) (adt? res :Result :Err)))

; ============================================================================
; Option Operations
; ============================================================================

; struct-get :: (α -> β) -> Option α -> Option β
; Map-option - transform the value inside Some, None stays None
(define struct-get (lambda (ƒ) (lambda (opt)
  (if (struct? opt)
     (⊙◇ (ƒ (adt-get opt :value)))
     ⊙∅))))

; ⊙⊙ :: (α -> Option β) -> Option α -> Option β
; Bind-option - chain optional operations, flatten nested Options
(define ⊙⊙ (lambda (ƒ) (lambda (opt)
  (if (struct? opt)
     (ƒ (adt-get opt :value))
     ⊙∅))))

; ⊙∨ :: α -> Option α -> α
; Or-else - provide default value for None
(define ⊙∨ (lambda (default) (lambda (opt)
  (if (struct? opt)
     (adt-get opt :value)
     default))))

; ⊙! :: Option α -> α
; Unwrap - extract value or error if None
; WARNING: Unsafe! Use only when you know it's Some
(define ⊙! (lambda (opt)
  (if (struct? opt)
     (adt-get opt :value)
     (error :unwrap-none :attempted-to-unwrap-none))))

; ⊙⊕ :: Option α -> Option α -> Option α
; Or-option - return first Some, or None if both None
(define ⊙⊕ (lambda (opt2) (lambda (opt1)
  (if (struct? opt1)
     opt1
     opt2))))

; ============================================================================
; Result Operations
; ============================================================================

; ⊙⇒ :: (α -> β) -> Result α γ -> Result β γ
; Map-result - transform success value, error unchanged
(define ⊙⇒ (lambda (ƒ) (lambda (res)
  (if (⊙✓? res)
     (⊙✓ (ƒ (adt-get res :value)))
     res))))

; ⊙⇐ :: (α -> β) -> Result γ α -> Result γ β
; Map-error - transform error value, success unchanged
(define ⊙⇐ (lambda (ƒ) (lambda (res)
  (if (⊙✓? res)
     res
     (⊙✗ (ƒ (adt-get res :error)))))))

; ⊙⊙⇒ :: (α -> Result β γ) -> Result α γ -> Result β γ
; Bind-result - chain result operations, flatten nested Results
(define ⊙⊙⇒ (lambda (ƒ) (lambda (res)
  (if (⊙✓? res)
     (ƒ (adt-get res :value))
     res))))

; ⊙‼ :: Result α β -> α
; Unwrap-result - extract success value or error if Err
; WARNING: Unsafe! Use only when you know it's Ok
(define ⊙‼ (lambda (res)
  (if (⊙✓? res)
     (adt-get res :value)
     (error :unwrap-error (adt-get res :error)))))

; ⊙‼∨ :: α -> Result α β -> α
; Unwrap-or - extract success value or provide default
(define ⊙‼∨ (lambda (default) (lambda (res)
  (if (⊙✓? res)
     (adt-get res :value)
     default))))

; ============================================================================
; Conversions
; ============================================================================

; ⊙→⊙ :: Option α -> Result α :none
; Option-to-result - convert None to Err(:none), Some to Ok
(define ⊙→⊙ (lambda (opt)
  (if (struct? opt)
     (⊙✓ (adt-get opt :value))
     (⊙✗ :none))))

; ⊙⊙→ :: Result α β -> Option α
; Result-to-option - convert Err to None, Ok to Some
(define ⊙⊙→ (lambda (res)
  (if (⊙✓? res)
     (⊙◇ (adt-get res :value))
     ⊙∅)))
