;; Test two vs three bindings

(⊙≔ :Pair :a :b)
(≔ pair (⊙ :Pair #1 #2))

;; Test: Two variables - should work
(⊨ :test-two #3
    (∇ pair (⌜ (((⊙ :Pair a b) (⊕ a b))))))

(⊙≔ :Triple :a :b :c)
(≔ t (⊙ :Triple #10 #20 #30))

;; Test: Three variables - might crash
(⊨ :test-three #60
    (∇ t (⌜ (((⊙ :Triple a b c) (⊕ a (⊕ b c)))))))
