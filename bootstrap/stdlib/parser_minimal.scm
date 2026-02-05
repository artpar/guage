; Minimal parser - absolute simplest version with no nested lambdas
(load "stdlib/macros.scm")
(load "stdlib/parser.scm")  ; Load tokenizer

; Test if tokenizer works
(define test-tokens (≈⊙tokenize "42"))

:minimal-parser-loaded
test-tokens
