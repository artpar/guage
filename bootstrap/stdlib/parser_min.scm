; Minimal parser for testing
(load "stdlib/macros.scm")

; Character classification
(define ≈⊙space? (lambda (𝕔)
  (or-all (equal? 𝕔 (quote :space))
  (or-all (equal? 𝕔 (quote :tab))
  (or-all (equal? 𝕔 (quote :newline))
      (equal? 𝕔 (quote :return)))))))

; Token helpers
(define ≈⊙→token (lambda (𝕥 𝕧)
  (cons 𝕥 𝕧)))

(define ≈⊙token-type (lambda (𝕥)
  (car 𝕥)))

(define ≈⊙token-val (lambda (𝕥)
  (car (cdr 𝕥))))

; Skip whitespace
(define ≈⊙skip-ws (lambda (𝕤 𝕡)
  (if (>= 𝕡 (string-length 𝕤))
     𝕡
     (if (≈⊙space? (string-ref 𝕤 𝕡))
        (≈⊙skip-ws 𝕤 (+ 𝕡 #1))
        𝕡))))

; Test
(≈⊙skip-ws "  hello" #0)
