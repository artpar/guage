; Benchmark: Sum to N
; Computes sum(i) for i = 1 to N

(define sum-to (lambda (n acc)
  (if (< n #1) acc
      (sum-to (- n #1) (+ acc n)))))

; N=100000 by default
(trace (sum-to #100000 #0))
