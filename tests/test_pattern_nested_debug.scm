;; Debug nested pattern matching

(⊙≔ :Point :x :y)
(≔ p (⊙ :Point #3 #4))

;; Test 1: Simple match
(⊨ :test1 #7 (∇ p (⌜ (((⊙ :Point x y) (⊕ x y))))))

;; Test 2: Line with nested points
(⊙≔ :Line :start :end)
(≔ p1 (⊙ :Point #1 #2))
(≔ p2 (⊙ :Point #3 #4))
(≔ line (⊙ :Line p1 p2))

;; Print line to see structure
⟲ line

;; Test 3: Match line, extract first point
(⊨ :test3 #t (⚠? (∇ line (⌜ (((⊙ :Line s e) #t))))))

;; Test 4: Extract fields from line
(≔ result (∇ line (⌜ (((⊙ :Line s e) s)))))
⟲ result
