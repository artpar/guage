; Benchmark: Sum of Squares
; Computes sum(i^2) for i = 1 to N

(define square (lambda (x) (* x x)))

(define sum-squares (lambda (n acc)
  (if (< n #1) acc
      (sum-squares (- n #1) (+ acc (square n))))))

; N=50000 by default - edit for different sizes
(trace (sum-squares #50000 #0))
