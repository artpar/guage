;; Test three field bindings

(⊙≔ :Triple :a :b :c)
(≔ t (⊙ :Triple #1 #2 #3))

;; Test: All three variables
(⊨ :test-all-three #6
    (∇ t (⌜ (((⊙ :Triple a b c) (⊕ a (⊕ b c)))))))
