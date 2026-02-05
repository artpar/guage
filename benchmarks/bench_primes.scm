;;; Benchmark: Prime counting via trial division up to 10000
;;; Tests: mixed workload (arithmetic, branching, inner loops, TCO)

(define is-prime (lambda (n i)
  (if (> (* i i) n) #t
     (if (equal? (% n i) #0) #f
        (is-prime n (+ i #1))))))

(define count-primes (lambda (n limit count)
  (if (> n limit) count
     (if (is-prime n #2)
        (count-primes (+ n #1) limit (+ count #1))
        (count-primes (+ n #1) limit count)))))

(trace (count-primes #2 #10000 #0))
