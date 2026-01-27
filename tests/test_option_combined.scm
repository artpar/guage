; Combined Option/Result library + tests
; Load library inline, then run tests

; ============================================================================
; LIBRARY: Option and Result Types
; ============================================================================

(⊚≔ :Option (⌜ (:None)) (⌜ (:Some :value)))
(⊚≔ :Result (⌜ (:Ok :value)) (⌜ (:Err :error)))

(≔ ⊙◇ (λ (value) (⊚ :Option :Some value)))
(≔ ⊙∅ (⊚ :Option :None))
(≔ ⊙✓ (λ (value) (⊚ :Result :Ok value)))
(≔ ⊙✗ (λ (error) (⊚ :Result :Err error)))

(≔ ⊙? (λ (opt) (⊚? opt :Option :Some)))
(≔ ⊙∅? (λ (opt) (⊚? opt :Option :None)))
(≔ ⊙✓? (λ (res) (⊚? res :Result :Ok)))
(≔ ⊙✗? (λ (res) (⊚? res :Result :Err)))

(≔ ⊙→ (λ (ƒ) (λ (opt) (? (⊙? opt) (⊙◇ (ƒ (⊚→ opt :value))) ⊙∅))))
(≔ ⊙⊙ (λ (ƒ) (λ (opt) (? (⊙? opt) (ƒ (⊚→ opt :value)) ⊙∅))))
(≔ ⊙∨ (λ (default) (λ (opt) (? (⊙? opt) (⊚→ opt :value) default))))
(≔ ⊙! (λ (opt) (? (⊙? opt) (⊚→ opt :value) (⚠ :unwrap-none :attempted-to-unwrap-none))))
(≔ ⊙⊕ (λ (opt2) (λ (opt1) (? (⊙? opt1) opt1 opt2))))

(≔ ⊙⇒ (λ (ƒ) (λ (res) (? (⊙✓? res) (⊙✓ (ƒ (⊚→ res :value))) res))))
(≔ ⊙⇐ (λ (ƒ) (λ (res) (? (⊙✓? res) res (⊙✗ (ƒ (⊚→ res :error)))))))
(≔ ⊙⊙⇒ (λ (ƒ) (λ (res) (? (⊙✓? res) (ƒ (⊚→ res :value)) res))))
(≔ ⊙‼ (λ (res) (? (⊙✓? res) (⊚→ res :value) (⚠ :unwrap-error (⊚→ res :error)))))
(≔ ⊙‼∨ (λ (default) (λ (res) (? (⊙✓? res) (⊚→ res :value) default))))

(≔ ⊙→⊙ (λ (opt) (? (⊙? opt) (⊙✓ (⊚→ opt :value)) (⊙✗ :none))))
(≔ ⊙⊙→ (λ (res) (? (⊙✓? res) (⊙◇ (⊚→ res :value)) ⊙∅)))

; ============================================================================
; TESTS: Option Constructors
; ============================================================================

(≔ some-42 (⊙◇ #42))
(⊨ :some-creates-value #t (⊚? some-42 :Option :Some))
(⊨ :none-is-none #t (⊚? ⊙∅ :Option :None))
(⊨ :some-value-correct #42 (⊚→ some-42 :value))

(≔ some-true (⊙◇ #t))
(⊨ :some-bool #t (⊚→ some-true :value))

; ============================================================================
; TESTS: Result Constructors
; ============================================================================

(≔ ok-99 (⊙✓ #99))
(⊨ :ok-creates-success #t (⊚? ok-99 :Result :Ok))

(≔ err-msg (⊙✗ :bad-input))
(⊨ :err-creates-failure #t (⊚? err-msg :Result :Err))
(⊨ :ok-value-correct #99 (⊚→ ok-99 :value))
(⊨ :err-error-correct #t (≡ :bad-input (⊚→ err-msg :error)))

; ============================================================================
; TESTS: Option Predicates
; ============================================================================

(⊨ :is-some-on-some #t (⊙? (⊙◇ #10)))
(⊨ :is-some-on-none #f (⊙? ⊙∅))
(⊨ :is-none-on-none #t (⊙∅? ⊙∅))
(⊨ :is-none-on-some #f (⊙∅? (⊙◇ #5)))

; ============================================================================
; TESTS: Result Predicates
; ============================================================================

(⊨ :is-ok-on-ok #t (⊙✓? (⊙✓ #100)))
(⊨ :is-ok-on-err #f (⊙✓? (⊙✗ :error)))
(⊨ :is-err-on-err #t (⊙✗? (⊙✗ :failed)))
(⊨ :is-err-on-ok #f (⊙✗? (⊙✓ #50)))

; ============================================================================
; TESTS: Option Map
; ============================================================================

(≔ double (λ (x) (⊗ x #2)))
(≔ some-5 (⊙◇ #5))
(≔ mapped ((⊙→ double) some-5))
(⊨ :map-some-result #10 (⊚→ mapped :value))

(≔ mapped-none ((⊙→ double) ⊙∅))
(⊨ :map-none-stays-none #t (⊙∅? mapped-none))

(≔ inc (λ (x) (⊕ x #1)))
(≔ mapped-inc ((⊙→ inc) (⊙◇ #10)))
(⊨ :map-increment #11 (⊚→ mapped-inc :value))
(⊨ :map-preserves-some #t (⊙? mapped-inc))

; ============================================================================
; TESTS: Option Bind
; ============================================================================

(≔ safe-div (λ (x) (? (≡ x #0) ⊙∅ (⊙◇ (⊘ #100 x)))))
(≔ bound ((⊙⊙ safe-div) (⊙◇ #10)))
(⊨ :bind-some-to-some #10 (⊚→ bound :value))

(≔ bound-none ((⊙⊙ safe-div) (⊙◇ #0)))
(⊨ :bind-some-to-none #t (⊙∅? bound-none))

(≔ bound-none2 ((⊙⊙ safe-div) ⊙∅))
(⊨ :bind-none-stays-none #t (⊙∅? bound-none2))

(≔ nested-opt (⊙◇ #20))
(≔ flattened ((⊙⊙ (λ (x) (⊙◇ (⊕ x #1)))) nested-opt))
(⊨ :bind-flattens #21 (⊚→ flattened :value))

; ============================================================================
; TESTS: Option Or-Else
; ============================================================================

(≔ val ((⊙∨ #99) (⊙◇ #42)))
(⊨ :or-else-some-value #42 val)

(≔ val-default ((⊙∨ #99) ⊙∅))
(⊨ :or-else-none-default #99 val-default)

(≔ preserved ((⊙∨ #0) (⊙◇ #7)))
(⊨ :or-else-preserves-some #7 preserved)
(⊨ :or-else-different-default #-1 ((⊙∨ #-1) ⊙∅))

; ============================================================================
; TESTS: Option Unwrap
; ============================================================================

(⊨ :unwrap-some-extracts #42 (⊙! (⊙◇ #42)))

(≔ unwrap-err (⊙! ⊙∅))
(⊨ :unwrap-none-is-error #t (⚠? unwrap-err))

; ============================================================================
; TESTS: Option Or
; ============================================================================

(≔ or-first ((⊙⊕ (⊙◇ #20)) (⊙◇ #10)))
(⊨ :or-first-some #10 (⊚→ or-first :value))

(≔ or-second ((⊙⊕ (⊙◇ #30)) ⊙∅))
(⊨ :or-second-some #30 (⊚→ or-second :value))

(≔ or-both-none ((⊙⊕ ⊙∅) ⊙∅))
(⊨ :or-both-none #t (⊙∅? or-both-none))

; ============================================================================
; TESTS: Result Map
; ============================================================================

(≔ ok-10 (⊙✓ #10))
(≔ mapped-ok ((⊙⇒ double) ok-10))
(⊨ :map-ok-transforms #20 (⊚→ mapped-ok :value))

(≔ err-test (⊙✗ :error))
(≔ mapped-err ((⊙⇒ double) err-test))
(⊨ :map-err-preserved #t (⊙✗? mapped-err))

(≔ err-to-upper (λ (e) :UPPER))
(≔ mapped-err-val ((⊙⇐ err-to-upper) (⊙✗ :lower)))
(⊨ :map-error-transforms #t (≡ :UPPER (⊚→ mapped-err-val :error)))

(≔ mapped-ok-unchanged ((⊙⇐ err-to-upper) (⊙✓ #50)))
(⊨ :map-error-preserves-ok #50 (⊚→ mapped-ok-unchanged :value))

; ============================================================================
; TESTS: Result Bind
; ============================================================================

(≔ validate (λ (x) (? (> x #0) (⊙✓ (⊗ x #2)) (⊙✗ :negative))))
(≔ bound-ok ((⊙⊙⇒ validate) (⊙✓ #5)))
(⊨ :bind-ok-to-ok #10 (⊚→ bound-ok :value))

(≔ bound-err ((⊙⊙⇒ validate) (⊙✓ #-1)))
(⊨ :bind-ok-to-err #t (⊙✗? bound-err))

(≔ bound-err2 ((⊙⊙⇒ validate) (⊙✗ :initial-error)))
(⊨ :bind-err-stays-err #t (⊙✗? bound-err2))
(⊨ :bind-preserves-error #t (≡ :initial-error (⊚→ bound-err2 :error)))

; ============================================================================
; TESTS: Result Unwrap
; ============================================================================

(⊨ :unwrap-ok-extracts #100 (⊙‼ (⊙✓ #100)))

(≔ unwrap-res-err (⊙‼ (⊙✗ :failed)))
(⊨ :unwrap-err-is-error #t (⚠? unwrap-res-err))
(⊨ :unwrap-or-ok-value #25 ((⊙‼∨ #0) (⊙✓ #25)))

; ============================================================================
; TESTS: Result Unwrap-Or
; ============================================================================

(⊨ :unwrap-or-err-default #99 ((⊙‼∨ #99) (⊙✗ :error)))
(⊨ :unwrap-or-preserves #15 ((⊙‼∨ #-1) (⊙✓ #15)))

; ============================================================================
; TESTS: Conversions
; ============================================================================

(≔ opt-some (⊙◇ #77))
(≔ res-from-some (⊙→⊙ opt-some))
(⊨ :some-to-ok #77 (⊚→ res-from-some :value))

(≔ res-from-none (⊙→⊙ ⊙∅))
(⊨ :none-to-err #t (⊙✗? res-from-none))

(≔ res-ok (⊙✓ #88))
(≔ opt-from-ok (⊙⊙→ res-ok))
(⊨ :ok-to-some #88 (⊚→ opt-from-ok :value))

(≔ opt-from-err (⊙⊙→ (⊙✗ :error)))
(⊨ :err-to-none #t (⊙∅? opt-from-err))

; ============================================================================
; TESTS: Real-world Usage
; ============================================================================

(≔ safe-divide (λ (a) (λ (b) (? (≡ b #0) ⊙∅ (⊙◇ (⊘ a b))))))
(⊨ :safe-div-valid #5 ((⊙∨ #0) ((safe-divide #10) #2)))
(⊨ :safe-div-zero #-1 ((⊙∨ #-1) ((safe-divide #10) #0)))

(≔ half-if-even (λ (x) (? (≡ (% x #2) #0) (⊙◇ (⊘ x #2)) ⊙∅)))
(≔ chain-result ((⊙⊙ half-if-even) (⊙◇ #10)))
(⊨ :chain-even-success #5 (⊚→ chain-result :value))

(≔ validate-positive (λ (x) (? (> x #0) (⊙✓ x) (⊙✗ :must-be-positive))))
(⊨ :validate-positive-ok #5 (⊚→ ((⊙⊙⇒ validate-positive) (⊙✓ #5)) :value))
(⊨ :validate-positive-err #t (⊙✗? ((⊙⊙⇒ validate-positive) (⊙✓ #-1))))
