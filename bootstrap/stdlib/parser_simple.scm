; Simplified S-expression parser - proof of concept
; Avoids deep nested lambdas, handles only basic cases
(load "stdlib/macros.scm")

; ═══════════════════════════════════════════════════════════════
; Reuse character classification and tokenization from parser.scm
; ═══════════════════════════════════════════════════════════════

(load "stdlib/parser.scm")  ; Load tokenizer functions

; ═══════════════════════════════════════════════════════════════
; Simplified Parser - No nested lambdas beyond 1 level
; ═══════════════════════════════════════════════════════════════

; Helper: Get token type safely
(define ≈⊙get-type (lambda (𝕥𝕠𝕜𝕤)
  (if (null? 𝕥𝕠𝕜𝕤)
     (quote :eof)
     (≈⊙token-type (car 𝕥𝕠𝕜𝕤)))))

; Helper: Get token value safely
(define ≈⊙get-val (lambda (𝕥𝕠𝕜𝕤)
  (if (null? 𝕥𝕠𝕜𝕤)
     nil
     (≈⊙token-val (car 𝕥𝕠𝕜𝕤)))))

; Helper: Get remaining tokens
(define ≈⊙rest-tokens (lambda (𝕥𝕠𝕜𝕤)
  (if (null? 𝕥𝕠𝕜𝕤)
     nil
     (cdr 𝕥𝕠𝕜𝕤))))

; Parse one atom (number or symbol)
(define ≈⊙parse-atom (lambda (𝕥𝕠𝕜𝕤 𝕥𝕪𝕡𝕖 𝕧𝕒𝕝)
  (cons 𝕧𝕒𝕝 (≈⊙rest-tokens 𝕥𝕠𝕜𝕤))))

; Parse list - simplified version
(define ≈⊙parse-list-simple (lambda (𝕥𝕠𝕜𝕤 𝕒𝕔𝕔)
  (if (null? 𝕥𝕠𝕜𝕤)
     (error (quote :unclosed-list) nil)
     (if (equal? (≈⊙get-type 𝕥𝕠𝕜𝕤) (quote :rparen))
        ; Found closing paren - return accumulated list
        (cons 𝕒𝕔𝕔 (≈⊙rest-tokens 𝕥𝕠𝕜𝕤))
        ; Parse one element and continue
        (if (equal? (≈⊙get-type 𝕥𝕠𝕜𝕤) (quote :lparen))
           ; Nested list - recursive call
           (≈⊙parse-list-simple (≈⊙rest-tokens 𝕥𝕠𝕜𝕤) 𝕒𝕔𝕔)
           ; Atom - add to accumulator
           (≈⊙parse-list-simple
             (≈⊙rest-tokens 𝕥𝕠𝕜𝕤)
             (cons 𝕒𝕔𝕔 (≈⊙get-val 𝕥𝕠𝕜𝕤))))))))

; Simple parse-one - handles atoms and lists
(define ≈⊙parse-simple (lambda (𝕥𝕠𝕜𝕤)
  (if (null? 𝕥𝕠𝕜𝕤)
     (error (quote :eof) nil)
     (if (equal? (≈⊙get-type 𝕥𝕠𝕜𝕤) (quote :lparen))
        ; Parse list
        (≈⊙parse-list-simple (≈⊙rest-tokens 𝕥𝕠𝕜𝕤) nil)
        ; Parse atom
        (≈⊙parse-atom 𝕥𝕠𝕜𝕤 (≈⊙get-type 𝕥𝕠𝕜𝕤) (≈⊙get-val 𝕥𝕠𝕜𝕤))))))

; Top-level parse from string
(define ≈⊙parse-str (lambda (𝕤)
  (≈⊙parse-simple (≈⊙tokenize 𝕤))))

:parser-simple-loaded
