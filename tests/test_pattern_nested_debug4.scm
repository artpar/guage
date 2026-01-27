;; Isolate the binding issue

(⊙≔ :Point :x :y)
(⊙≔ :Line :start :end)
(≔ p1 (⊙ :Point #100 #200))
(≔ p2 (⊙ :Point #300 #400))
(≔ line (⊙ :Line p1 p2))

;; Test: Just add x1 + x2 (no y's)
(⊨ :test1
    #400
    (∇ line (⌜ (((⊙ :Line (⊙ :Point x1 _) (⊙ :Point x2 _))
                  (⊕ x1 x2))))))

;; Test: Add x2 + y2 (just second point)
(⊨ :test2
    #700
    (∇ line (⌜ (((⊙ :Line _ (⊙ :Point x2 y2))
                  (⊕ x2 y2))))))

;; Test: Add all four with parentheses
(⊨ :test3
    #1000
    (∇ line (⌜ (((⊙ :Line (⊙ :Point x1 y1) (⊙ :Point x2 y2))
                  (⊕ x1 (⊕ y1 (⊕ x2 y2))))))))
