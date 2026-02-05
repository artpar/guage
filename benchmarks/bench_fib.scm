;;; Benchmark: Naive recursive Fibonacci(28)
;;; Tests: function call overhead, tree recursion

(define fib (lambda (n)
  (if (< n #2) n
     (+ (fib (- n #1)) (fib (- n #2))))))

(trace (fib #28))
