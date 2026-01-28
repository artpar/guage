(⋘ "stdlib/macros.scm")
(⋘ "stdlib/parser.scm")

; Test single token
(≈⊙tokenize "42")

; Test multiple tokens
(≈⊙tokenize "(+ 1 2)")

; Test symbols
(≈⊙tokenize "hello world")
