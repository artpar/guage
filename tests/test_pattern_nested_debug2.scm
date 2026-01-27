;; Debug nested pattern matching - part 2

(⊙≔ :Point :x :y)
(⊙≔ :Line :start :end)
(≔ p1 (⊙ :Point #1 #2))
(≔ p2 (⊙ :Point #3 #4))
(≔ line (⊙ :Line p1 p2))

;; Test 1: Extract fields as simple variables
(⊨ :test1
    #t
    (≡ (∇ line (⌜ (((⊙ :Line s e) s)))) p1))

;; Test 2: Nested pattern matching
(⊨ :test2
    #3
    (∇ line (⌜ (((⊙ :Line (⊙ :Point x1 y1) e) x1)))))

;; Test 3: Both nested
(⊨ :test3
    #8
    (∇ line (⌜ (((⊙ :Line (⊙ :Point x1 y1) (⊙ :Point x2 y2))
                  (⊕ (⊕ x1 y1) (⊕ x2 y2)))))))
