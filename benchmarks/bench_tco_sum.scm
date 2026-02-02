;;; Benchmark: TCO sum accumulator(500000)
;;; Tests: tail-call optimization throughput

(≔ sum-acc (λ (n acc)
  (? (≡ n #0) acc
     (sum-acc (⊖ n #1) (⊕ acc n)))))

(⟲ (sum-acc #500000 #0))
