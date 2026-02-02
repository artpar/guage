;;; Benchmark: Prime counting via trial division up to 10000
;;; Tests: mixed workload (arithmetic, branching, inner loops, TCO)

(≔ is-prime (λ (n i)
  (? (> (⊗ i i) n) #t
     (? (≡ (% n i) #0) #f
        (is-prime n (⊕ i #1))))))

(≔ count-primes (λ (n limit count)
  (? (> n limit) count
     (? (is-prime n #2)
        (count-primes (⊕ n #1) limit (⊕ count #1))
        (count-primes (⊕ n #1) limit count)))))

(⟲ (count-primes #2 #10000 #0))
