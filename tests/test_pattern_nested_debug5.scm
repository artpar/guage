;; Try to see what's bound

(⊙≔ :Point :x :y)
(⊙≔ :Line :start :end)
(≔ p1 (⊙ :Point #100 #200))
(≔ p2 (⊙ :Point #300 #400))
(≔ line (⊙ :Line p1 p2))

;; Test: Return x1 alone
(⊨ :test-x1 #100
    (∇ line (⌜ (((⊙ :Line (⊙ :Point x1 y1) (⊙ :Point x2 y2)) x1)))))

;; Test: Return y1 alone
(⊨ :test-y1 #200
    (∇ line (⌜ (((⊙ :Line (⊙ :Point x1 y1) (⊙ :Point x2 y2)) y1)))))

;; Test: Return x2 alone
(⊨ :test-x2 #300
    (∇ line (⌜ (((⊙ :Line (⊙ :Point x1 y1) (⊙ :Point x2 y2)) x2)))))

;; Test: Return y2 alone
(⊨ :test-y2 #400
    (∇ line (⌜ (((⊙ :Line (⊙ :Point x1 y1) (⊙ :Point x2 y2)) y2)))))

;; Test: Add x1 + y1 (first pair)
(⊨ :test-add-first #300
    (∇ line (⌜ (((⊙ :Line (⊙ :Point x1 y1) (⊙ :Point x2 y2)) (⊕ x1 y1))))))

;; Test: Add x2 + y2 (second pair)
(⊨ :test-add-second #700
    (∇ line (⌜ (((⊙ :Line (⊙ :Point x1 y1) (⊙ :Point x2 y2)) (⊕ x2 y2))))))

;; Test: Add x1 + x2 (across pairs)
(⊨ :test-add-xs #400
    (∇ line (⌜ (((⊙ :Line (⊙ :Point x1 y1) (⊙ :Point x2 y2)) (⊕ x1 x2))))))

;; Test: Add y1 + y2 (across pairs)
(⊨ :test-add-ys #600
    (∇ line (⌜ (((⊙ :Line (⊙ :Point x1 y1) (⊙ :Point x2 y2)) (⊕ y1 y2))))))

;; Test: Add three: x1 + y1 + x2
(⊨ :test-add-three #600
    (∇ line (⌜ (((⊙ :Line (⊙ :Point x1 y1) (⊙ :Point x2 y2)) (⊕ (⊕ x1 y1) x2))))))
