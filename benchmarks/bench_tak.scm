;;; Benchmark: Takeuchi function tak(21,14,7)
;;; Tests: multi-argument calls, branching, deep recursion

(≔ tak (λ (x y z)
  (? (≤ x y) z
     (tak (tak (⊖ x #1) y z)
          (tak (⊖ y #1) z x)
          (tak (⊖ z #1) x y)))))

(⟲ (tak #21 #14 #7))
