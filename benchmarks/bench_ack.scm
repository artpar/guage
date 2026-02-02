;;; Benchmark: Ackermann function ack(3,7)
;;; Tests: deep non-tail recursion, stack management

(≔ ack (λ (m n)
  (? (≡ m #0) (⊕ n #1)
     (? (≡ n #0) (ack (⊖ m #1) #1)
        (ack (⊖ m #1) (ack m (⊖ n #1)))))))

(⟲ (ack #3 #7))
