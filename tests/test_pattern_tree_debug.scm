;; Debug tree pattern crash

(⊚≔ :Tree (⌜ (:Leaf :value)) (⌜ (:Node :left :value :right)))
(≔ leaf5 (⊚ :Tree :Leaf #5))
(≔ leaf10 (⊚ :Tree :Leaf #10))
(≔ leaf15 (⊚ :Tree :Leaf #15))
(≔ tree (⊚ :Tree :Node leaf5 #10 leaf15))

;; Test: Extract just lv
(⊨ :test-lv #5
    (∇ tree (⌜ (((⊚ :Tree :Node (⊚ :Tree :Leaf lv) v rv) lv)))))

;; Test: Extract just v
(⊨ :test-v #10
    (∇ tree (⌜ (((⊚ :Tree :Node l v r) v)))))

;; Test: Extract just rv
(⊨ :test-rv #15
    (∇ tree (⌜ (((⊚ :Tree :Node l v (⊚ :Tree :Leaf rv)) rv)))))

;; Test: Add lv + rv
(⊨ :test-both #20
    (∇ tree (⌜ (((⊚ :Tree :Node (⊚ :Tree :Leaf lv) v (⊚ :Tree :Leaf rv))
                  (⊕ lv rv))))))
