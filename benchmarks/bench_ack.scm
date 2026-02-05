;;; Benchmark: Ackermann function ack(3,7)
;;; Tests: deep non-tail recursion, stack management

(define ack (lambda (m n)
  (if (equal? m #0) (+ n #1)
     (if (equal? n #0) (ack (- m #1) #1)
        (ack (- m #1) (ack m (- n #1)))))))

(trace (ack #3 #7))
