; Guage Standard Library: Option and Result Types
; Error handling without exceptions - values as boundaries!

; ============================================================================
; Type Definitions
; ============================================================================

; Option type - represents optional values (may or may not exist)
; :None - no value present
; :Some - value present
(⊚≔ :Option (⌜ (:None)) (⌜ (:Some :value)))

; Result type - represents operations that can succeed or fail
; :Ok - success with value
; :Err - failure with error
(⊚≔ :Result (⌜ (:Ok :value)) (⌜ (:Err :error)))

; ============================================================================
; Option Constructors (for convenience)
; ============================================================================

; ⊙◇ :: α → Option α
; Some - wrap value in Some
(≔ ⊙◇ (λ (value)
  (⊚ :Option :Some value)))

; ⊙∅ :: Option α
; None - the none value
(≔ ⊙∅ (⊚ :Option :None))

; ============================================================================
; Result Constructors (for convenience)
; ============================================================================

; ⊙✓ :: α → Result α β
; Ok - wrap value in success
(≔ ⊙✓ (λ (value)
  (⊚ :Result :Ok value)))

; ⊙✗ :: β → Result α β
; Err - wrap error in failure
(≔ ⊙✗ (λ (error)
  (⊚ :Result :Err error)))

; ============================================================================
; Option Predicates
; ============================================================================

; ⊙? :: Option α → 𝔹
; Is-some - check if option contains a value
(≔ ⊙? (λ (opt) (⊚? opt :Option :Some)))

; ⊙∅? :: Option α → 𝔹
; Is-none - check if option is None
(≔ ⊙∅? (λ (opt) (⊚? opt :Option :None)))

; ============================================================================
; Result Predicates
; ============================================================================

; ⊙✓? :: Result α β → 𝔹
; Is-ok - check if result is success
(≔ ⊙✓? (λ (res) (⊚? res :Result :Ok)))

; ⊙✗? :: Result α β → 𝔹
; Is-err - check if result is failure
(≔ ⊙✗? (λ (res) (⊚? res :Result :Err)))

; ============================================================================
; Option Operations
; ============================================================================

; ⊙→ :: (α → β) → Option α → Option β
; Map-option - transform the value inside Some, None stays None
(≔ ⊙→ (λ (ƒ) (λ (opt)
  (? (⊙? opt)
     (⊙◇ (ƒ (⊚→ opt :value)))
     ⊙∅))))

; ⊙⊙ :: (α → Option β) → Option α → Option β
; Bind-option - chain optional operations, flatten nested Options
(≔ ⊙⊙ (λ (ƒ) (λ (opt)
  (? (⊙? opt)
     (ƒ (⊚→ opt :value))
     ⊙∅))))

; ⊙∨ :: α → Option α → α
; Or-else - provide default value for None
(≔ ⊙∨ (λ (default) (λ (opt)
  (? (⊙? opt)
     (⊚→ opt :value)
     default))))

; ⊙! :: Option α → α
; Unwrap - extract value or error if None
; WARNING: Unsafe! Use only when you know it's Some
(≔ ⊙! (λ (opt)
  (? (⊙? opt)
     (⊚→ opt :value)
     (⚠ :unwrap-none :attempted-to-unwrap-none))))

; ⊙⊕ :: Option α → Option α → Option α
; Or-option - return first Some, or None if both None
(≔ ⊙⊕ (λ (opt2) (λ (opt1)
  (? (⊙? opt1)
     opt1
     opt2))))

; ============================================================================
; Result Operations
; ============================================================================

; ⊙⇒ :: (α → β) → Result α γ → Result β γ
; Map-result - transform success value, error unchanged
(≔ ⊙⇒ (λ (ƒ) (λ (res)
  (? (⊙✓? res)
     (⊙✓ (ƒ (⊚→ res :value)))
     res))))

; ⊙⇐ :: (α → β) → Result γ α → Result γ β
; Map-error - transform error value, success unchanged
(≔ ⊙⇐ (λ (ƒ) (λ (res)
  (? (⊙✓? res)
     res
     (⊙✗ (ƒ (⊚→ res :error)))))))

; ⊙⊙⇒ :: (α → Result β γ) → Result α γ → Result β γ
; Bind-result - chain result operations, flatten nested Results
(≔ ⊙⊙⇒ (λ (ƒ) (λ (res)
  (? (⊙✓? res)
     (ƒ (⊚→ res :value))
     res))))

; ⊙‼ :: Result α β → α
; Unwrap-result - extract success value or error if Err
; WARNING: Unsafe! Use only when you know it's Ok
(≔ ⊙‼ (λ (res)
  (? (⊙✓? res)
     (⊚→ res :value)
     (⚠ :unwrap-error (⊚→ res :error)))))

; ⊙‼∨ :: α → Result α β → α
; Unwrap-or - extract success value or provide default
(≔ ⊙‼∨ (λ (default) (λ (res)
  (? (⊙✓? res)
     (⊚→ res :value)
     default))))

; ============================================================================
; Conversions
; ============================================================================

; ⊙→⊙ :: Option α → Result α :none
; Option-to-result - convert None to Err(:none), Some to Ok
(≔ ⊙→⊙ (λ (opt)
  (? (⊙? opt)
     (⊙✓ (⊚→ opt :value))
     (⊙✗ :none))))

; ⊙⊙→ :: Result α β → Option α
; Result-to-option - convert Err to None, Ok to Some
(≔ ⊙⊙→ (λ (res)
  (? (⊙✓? res)
     (⊙◇ (⊚→ res :value))
     ⊙∅)))
