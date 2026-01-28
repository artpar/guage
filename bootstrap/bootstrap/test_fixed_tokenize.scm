(⋘ "stdlib/macros.scm")
(⋘ "stdlib/parser.scm")

; Test tokenize-one first
(⟲ :tokenize-one (≈⊙tokenize-one "42" #0))

; Test tokenize
(⟲ :tokenize (≈⊙tokenize "42"))

; Test with multiple tokens
(⟲ :multi (≈⊙tokenize "(+ 1 2)"))
