; Test lambda bindings
(⋘ "stdlib/macros.scm")

; Simple test
(≔ test1 (λ (x)
  ((λ (y) y)
   (⟨⟩ x #42))))

(test1 #1)

; Test with car
(≔ test2 (λ (x)
  ((λ (y) (◁ y))
   (⟨⟩ x #42))))

(test2 #1)

; Test tokenize-one structure
(⋘ "stdlib/parser.scm")
(≔ r (≈⊙tokenize-one "42" #0))
:got-result
(∅? r)
(◁ r)
