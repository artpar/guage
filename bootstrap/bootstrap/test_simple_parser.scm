(⋘ "stdlib/parser_simple.scm")

; Test parsing a number
:test-number
(≈⊙parse-str "42")

; Test parsing a symbol
:test-symbol
(≈⊙parse-str "hello")

; Test simple list
:test-list
(≈⊙parse-str "(+ 1 2)")
