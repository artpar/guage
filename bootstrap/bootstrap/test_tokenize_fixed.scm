(⋘ "stdlib/macros.scm")
(⋘ "stdlib/parser.scm")

; Test tokenize-one
(≈⊙tokenize-one "42" #0)

; Test tokenize
(≈⊙tokenize "42")

; Test with multiple tokens
(≈⊙tokenize "(+ 1 2)")

; Test with symbols
(≈⊙tokenize "hello world")
