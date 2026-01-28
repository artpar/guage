(⋘ "stdlib/macros.scm")
(⋘ "stdlib/parser.scm")

; Test parsing a number
:parsing-number
(≈⊙parse "42")

; Test parsing a symbol
:parsing-symbol
(≈⊙parse "hello")

; Test parsing a simple list
:parsing-list
(≈⊙parse "(+ 1 2)")

; Test parsing nested list
:parsing-nested
(≈⊙parse "(foo (bar baz))")
