(⋘ "stdlib/macros.scm")
(⋘ "stdlib/parser.scm")

; Test tokenize-loop directly with a manual result
(≔ manual-result (⟨⟩ (⟨⟩ (⌜ :number) "42") #2))
:manual-result
manual-result

; Test if we can call tokenize-loop
(≈⊙tokenize-loop "42 abc" #2)
