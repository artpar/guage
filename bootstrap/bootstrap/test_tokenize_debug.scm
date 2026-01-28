; Debug tokenize issue
(⋘ "stdlib/macros.scm")
(⋘ "stdlib/parser.scm")

; Test what tokenize-one returns
(≔ result (≈⊙tokenize-one "42" #0))
(⟲ :result result)

; Check structure
(⟲ :is-pair? (¬ (∅? result)))
(⟲ :first (◁ result))
(⟲ :second (◁ (▷ result)))

; Check if first element is a token
(≔ token (◁ result))
(⟲ :token token)
(⟲ :token-type (≈⊙token-type token))
(⟲ :token-val (≈⊙token-val token))
