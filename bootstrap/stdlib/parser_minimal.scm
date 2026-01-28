; Minimal parser - absolute simplest version with no nested lambdas
(⋘ "stdlib/macros.scm")
(⋘ "stdlib/parser.scm")  ; Load tokenizer

; Test if tokenizer works
(≔ test-tokens (≈⊙tokenize "42"))

:minimal-parser-loaded
test-tokens
