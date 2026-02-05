;;; Benchmark: TCO sum accumulator(500000)
;;; Tests: tail-call optimization throughput

(define sum-acc (lambda (n acc)
  (if (equal? n #0) acc
     (sum-acc (- n #1) (+ acc n)))))

(trace (sum-acc #500000 #0))
