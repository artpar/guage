; Test helper module — defines some symbols, exports only some

(≔ pub-val #42)
(≔ priv-val #99)
(≔ pub-fn (λ (x) (⊕ x #1)))
(≔ priv-fn (λ (x) (⊗ x #2)))

; Declare exports: only pub-val and pub-fn are public
(⊞◇ "bootstrap/tests/test_module_helper.scm" (⌜ (:pub-val :pub-fn)))
