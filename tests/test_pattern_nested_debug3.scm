;; Debug nested pattern matching - clearer values

(⊙≔ :Point :x :y)
(⊙≔ :Line :start :end)
(≔ p1 (⊙ :Point #100 #200))
(≔ p2 (⊙ :Point #300 #400))
(≔ line (⊙ :Line p1 p2))

;; Test 1: Extract first point's x
(⊨ :test1
    #100
    (∇ line (⌜ (((⊙ :Line (⊙ :Point x1 y1) e) x1)))))

;; Test 2: Extract first point's y
(⊨ :test2
    #200
    (∇ line (⌜ (((⊙ :Line (⊙ :Point x1 y1) e) y1)))))

;; Test 3: Extract both points
(⊨ :test3
    #700
    (∇ line (⌜ (((⊙ :Line (⊙ :Point x1 y1) (⊙ :Point x2 y2))
                  (⊕ (⊕ x1 y1) (⊕ x2 y2)))))))
