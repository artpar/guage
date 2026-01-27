; Run Option/Result tests
; Load library first, then run tests

; Load the Option/Result library
(⌞ (⌜ (⌜ stdlib/option.scm)))

; Note: In a real module system, we'd use (import stdlib/option)
; For now, we'll manually evaluate the library file content

; Since we can't easily load files yet, we'll define types inline for testing
; This will be replaced with proper module loading later

; Type definitions (from option.scm)
(⊚≔ :Option (⌜ (:None)) (⌜ (:Some :value)))
(⊚≔ :Result (⌜ (:Ok :value)) (⌜ (:Err :error)))

; Load test definitions from tests/stdlib_option.test
; (In practice, this would be done via test runner)
