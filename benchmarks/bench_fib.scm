;;; Benchmark: Naive recursive Fibonacci(28)
;;; Tests: function call overhead, tree recursion

(≔ fib (λ (n)
  (? (< n #2) n
     (⊕ (fib (⊖ n #1)) (fib (⊖ n #2))))))

(⟲ (fib #28))
