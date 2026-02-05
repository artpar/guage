; ═══════════════════════════════════════════════════════════════
; Guage Standard Library: S-Expression Parser
; ═══════════════════════════════════════════════════════════════
; Status: CURRENT
; Created: 2026-01-27 (Day 39)
; Purpose: Parse S-expressions written in Guage - Step toward self-hosting
;
; All names are PURELY SYMBOLIC - no English words!
; This parser converts strings to nested list structures (AST)
; ═══════════════════════════════════════════════════════════════

; ═══════════════════════════════════════════════════════════════
; Phase 1: Character Classification
; ═══════════════════════════════════════════════════════════════

; ≈⊙space? :: :symbol -> Bool
; Check if character symbol is whitespace
(define ≈⊙space? (lambda (𝕔)
  (or (equal? 𝕔 (string-ref " " #0))
  (or (equal? 𝕔 (string-ref "\t" #0))
  (or (equal? 𝕔 (string-ref "\n" #0))
      (equal? 𝕔 (string-ref "\r" #0)))))))

; ≈⊙digit? :: :symbol -> Bool
; Check if character symbol is a digit (0-9)
(define ≈⊙digit? (lambda (𝕔)
  (or (equal? 𝕔 (string-ref "0" #0))
  (or (equal? 𝕔 (string-ref "1" #0))
  (or (equal? 𝕔 (string-ref "2" #0))
  (or (equal? 𝕔 (string-ref "3" #0))
  (or (equal? 𝕔 (string-ref "4" #0))
  (or (equal? 𝕔 (string-ref "5" #0))
  (or (equal? 𝕔 (string-ref "6" #0))
  (or (equal? 𝕔 (string-ref "7" #0))
  (or (equal? 𝕔 (string-ref "8" #0))
      (equal? 𝕔 (string-ref "9" #0)))))))))))))

; ≈⊙paren? :: :symbol -> Bool
; Check if character is parenthesis
(define ≈⊙paren? (lambda (𝕔)
  (or (equal? 𝕔 (string-ref "(" #0))
      (equal? 𝕔 (string-ref ")" #0)))))

; ≈⊙special? :: :symbol -> Bool
; Check if character is special delimiter
(define ≈⊙special? (lambda (𝕔)
  (or (≈⊙space? 𝕔)
  (or (≈⊙paren? 𝕔)
  (or (equal? 𝕔 (string-ref "\"" #0))
  (or (equal? 𝕔 (string-ref "'" #0))
      (equal? 𝕔 (string-ref ";" #0))))))))

; ═══════════════════════════════════════════════════════════════
; Phase 2: Tokenization Helpers
; ═══════════════════════════════════════════════════════════════

; ≈⊙→token :: :symbol -> α -> token
; Create token structure: ⟨type value⟩
(define ≈⊙→token (lambda (𝕥 𝕧)
  (cons 𝕥 𝕧)))

; ≈⊙token-type :: token -> :symbol
; Get token type
(define ≈⊙token-type (lambda (𝕥)
  (car 𝕥)))

; ≈⊙token-val :: token -> α
; Get token value
(define ≈⊙token-val (lambda (𝕥)
  (cdr 𝕥)))

; ≈⊙skip-ws :: string -> ℕ -> ℕ
; Skip whitespace, return new position
(define ≈⊙skip-ws (lambda (𝕤 𝕡)
  (if (>= 𝕡 (string-length 𝕤))
     𝕡
     (if (≈⊙space? (string-ref 𝕤 𝕡))
        (≈⊙skip-ws 𝕤 (+ 𝕡 #1))
        𝕡))))

; ≈⊙skip-comment :: string -> ℕ -> ℕ
; Skip comment (from ; to newline), return new position
(define ≈⊙skip-comment (lambda (𝕤 𝕡)
  (if (>= 𝕡 (string-length 𝕤))
     𝕡
     (if (equal? (string-ref 𝕤 𝕡) (string-ref "\n" #0))
        (+ 𝕡 #1)
        (≈⊙skip-comment 𝕤 (+ 𝕡 #1))))))

; ═══════════════════════════════════════════════════════════════
; Phase 3: Token Reading
; ═══════════════════════════════════════════════════════════════

; ≈⊙read-number :: string -> ℕ -> ℕ -> string
; Read number characters until delimiter
(define ≈⊙read-number (lambda (𝕤 𝕡 𝕤𝕥𝕒𝕣𝕥)
  (if (>= 𝕡 (string-length 𝕤))
     (string-slice 𝕤 𝕤𝕥𝕒𝕣𝕥 𝕡)
     (if (≈⊙special? (string-ref 𝕤 𝕡))
        (string-slice 𝕤 𝕤𝕥𝕒𝕣𝕥 𝕡)
        (≈⊙read-number 𝕤 (+ 𝕡 #1) 𝕤𝕥𝕒𝕣𝕥)))))

; ≈⊙read-symbol :: string -> ℕ -> ℕ -> string
; Read symbol characters until delimiter
(define ≈⊙read-symbol (lambda (𝕤 𝕡 𝕤𝕥𝕒𝕣𝕥)
  (if (>= 𝕡 (string-length 𝕤))
     (string-slice 𝕤 𝕤𝕥𝕒𝕣𝕥 𝕡)
     (if (≈⊙special? (string-ref 𝕤 𝕡))
        (string-slice 𝕤 𝕤𝕥𝕒𝕣𝕥 𝕡)
        (≈⊙read-symbol 𝕤 (+ 𝕡 #1) 𝕤𝕥𝕒𝕣𝕥)))))

; ≈⊙read-string :: string -> ℕ -> ℕ -> string
; Read string characters until closing quote
(define ≈⊙read-string (lambda (𝕤 𝕡 𝕤𝕥𝕒𝕣𝕥)
  (if (>= 𝕡 (string-length 𝕤))
     (error (quote :unclosed-string) (string-slice 𝕤 𝕤𝕥𝕒𝕣𝕥 𝕡))
     (if (equal? (string-ref 𝕤 𝕡) (string-ref "\"" #0))
        (string-slice 𝕤 𝕤𝕥𝕒𝕣𝕥 𝕡)
        (≈⊙read-string 𝕤 (+ 𝕡 #1) 𝕤𝕥𝕒𝕣𝕥)))))

; ═══════════════════════════════════════════════════════════════
; Phase 4: Tokenizer
; ═══════════════════════════════════════════════════════════════

; ≈⊙tokenize-one :: string -> ℕ -> ⟨token ℕ⟩ | nil
; Read one token from position, return ⟨token new-position⟩ or nil at EOF
(define ≈⊙tokenize-one (lambda (𝕤 𝕡)
  ; Skip whitespace using lambda binding
  ((lambda (𝕡′)
    ; Check if at end
    (if (>= 𝕡′ (string-length 𝕤))
       nil

       ; Check for comment
       (if (equal? (string-ref 𝕤 𝕡′) (string-ref ";" #0))
          (≈⊙tokenize-one 𝕤 (≈⊙skip-comment 𝕤 𝕡′))

          ; Check for left paren
          (if (equal? (string-ref 𝕤 𝕡′) (string-ref "(" #0))
             (cons (≈⊙→token (quote :lparen) (string-ref "(" #0)) (+ 𝕡′ #1))

             ; Check for right paren
             (if (equal? (string-ref 𝕤 𝕡′) (string-ref ")" #0))
                (cons (≈⊙→token (quote :rparen) (string-ref ")" #0)) (+ 𝕡′ #1))

                ; Check for quote
                (if (equal? (string-ref 𝕤 𝕡′) (string-ref "'" #0))
                   (cons (≈⊙→token (quote :quote) (string-ref "'" #0)) (+ 𝕡′ #1))

                   ; Check for string
                   (if (equal? (string-ref 𝕤 𝕡′) (string-ref "\"" #0))
                      ((lambda (𝕤𝕥𝕣)
                        (if (error? 𝕤𝕥𝕣)
                           (cons 𝕤𝕥𝕣 𝕡′)  ; Return error
                           (cons (≈⊙→token (quote :string) 𝕤𝕥𝕣)
                               (+ (+ 𝕡′ #1) (+ (string-length 𝕤𝕥𝕣) #1)))))
                       (≈⊙read-string 𝕤 (+ 𝕡′ #1) (+ 𝕡′ #1)))

                      ; Check for number (digit or -)
                      (if (or (≈⊙digit? (string-ref 𝕤 𝕡′))
                             (equal? (string-ref 𝕤 𝕡′) (string-ref "-" #0)))
                         ((lambda (𝕟𝕦𝕞)
                           (cons (≈⊙→token (quote :number) 𝕟𝕦𝕞) (+ 𝕡′ (string-length 𝕟𝕦𝕞))))
                          (≈⊙read-number 𝕤 𝕡′ 𝕡′))

                         ; Must be symbol
                         ((lambda (𝕤𝕪𝕞)
                           (cons (≈⊙→token (quote :symbol) 𝕤𝕪𝕞) (+ 𝕡′ (string-length 𝕤𝕪𝕞))))
                          (≈⊙read-symbol 𝕤 𝕡′ 𝕡′))))))))))
   (≈⊙skip-ws 𝕤 𝕡))))

; ≈⊙tokenize :: string -> [token] | error
; Tokenize entire string into list of tokens
(define ≈⊙tokenize (lambda (𝕤)
  (≈⊙tokenize-loop 𝕤 #0)))

; Tokenize loop - calls tokenize-one repeatedly (no nested lambda, calls tokenize-one 3x)
(define ≈⊙tokenize-loop (lambda (𝕤 𝕡)
  (if (>= 𝕡 (string-length 𝕤))
     nil  ; At end of string
     ; Check if tokenize-one returns nil
     (if (null? (≈⊙tokenize-one 𝕤 𝕡))
        nil
        ; Check if token is error
        (if (error? (car (≈⊙tokenize-one 𝕤 𝕡)))
           (car (≈⊙tokenize-one 𝕤 𝕡))  ; Return the error
           ; Build list: cons token onto recursive call
           (cons (car (≈⊙tokenize-one 𝕤 𝕡))
               (≈⊙tokenize-loop 𝕤 (cdr (≈⊙tokenize-one 𝕤 𝕡)))))))))

; ═══════════════════════════════════════════════════════════════
; Phase 5: Parser
; ═══════════════════════════════════════════════════════════════

; ≈⊙parse-one :: [token] -> ⟨expr [token]⟩ | error
; Parse one expression from token list
; Returns ⟨parsed-expr remaining-tokens⟩
(define ≈⊙parse-one (lambda (𝕥𝕠𝕜𝕤)
  (if (null? 𝕥𝕠𝕜𝕤)
     (error (quote :unexpected-eof) nil)

     ; Use nested lambda bindings for local variables
     ((lambda (𝕥𝕠𝕜)
       ((lambda (𝕥𝕪𝕡𝕖)
         ((lambda (𝕧𝕒𝕝)
           ; Check token type
           (if (equal? 𝕥𝕪𝕡𝕖 (quote :number))
              ; TODO: Convert string to number (for now return string)
              (cons 𝕧𝕒𝕝 (cdr 𝕥𝕠𝕜𝕤))

              (if (equal? 𝕥𝕪𝕡𝕖 (quote :string))
                 (cons 𝕧𝕒𝕝 (cdr 𝕥𝕠𝕜𝕤))

                 (if (equal? 𝕥𝕪𝕡𝕖 (quote :symbol))
                    ; Convert string to symbol
                    ; TODO: Proper symbol creation (for now return string)
                    (cons 𝕧𝕒𝕝 (cdr 𝕥𝕠𝕜𝕤))

                    (if (equal? 𝕥𝕪𝕡𝕖 (quote :quote))
                       ; Parse quoted expression: ' -> (quote ...)
                       ((lambda (𝕢𝕦𝕠𝕥𝕖𝕕)
                         (if (error? 𝕢𝕦𝕠𝕥𝕖𝕕)
                            𝕢𝕦𝕠𝕥𝕖𝕕
                            (cons (cons (quote quote) (cons (car 𝕢𝕦𝕠𝕥𝕖𝕕) nil))
                                (cdr 𝕢𝕦𝕠𝕥𝕖𝕕))))
                        (≈⊙parse-one (cdr 𝕥𝕠𝕜𝕤)))

                       (if (equal? 𝕥𝕪𝕡𝕖 (quote :lparen))
                          ; Parse list until rparen
                          (≈⊙parse-list (cdr 𝕥𝕠𝕜𝕤))

                          (if (equal? 𝕥𝕪𝕡𝕖 (quote :rparen))
                             (error (quote :unexpected-rparen) 𝕧𝕒𝕝)
                             (error (quote :unknown-token-type) 𝕥𝕪𝕡𝕖))))))))
          (≈⊙token-val 𝕥𝕠𝕜)))
        (≈⊙token-type 𝕥𝕠𝕜)))
      (car 𝕥𝕠𝕜𝕤)))))

; ≈⊙parse-list :: [token] -> ⟨list [token]⟩ | error
; Parse list elements until rparen
; Returns ⟨list-expr remaining-tokens⟩
(define ≈⊙parse-list (lambda (𝕥𝕠𝕜𝕤)
  (if (null? 𝕥𝕠𝕜𝕤)
     (error (quote :unclosed-list) nil)

     ((lambda (𝕥𝕠𝕜)
       ((lambda (𝕥𝕪𝕡𝕖)
         ; Check for closing paren
         (if (equal? 𝕥𝕪𝕡𝕖 (quote :rparen))
            (cons nil (cdr 𝕥𝕠𝕜𝕤))

            ; Parse one element
            ((lambda (𝕖𝕝𝕖𝕞)
              (if (error? 𝕖𝕝𝕖𝕞)
                 𝕖𝕝𝕖𝕞

                 ; Parse rest of list
                 ((lambda (𝕣𝕖𝕤𝕥)
                   (if (error? 𝕣𝕖𝕤𝕥)
                      𝕣𝕖𝕤𝕥
                      (cons (cons (car 𝕖𝕝𝕖𝕞) (car 𝕣𝕖𝕤𝕥))
                          (cdr 𝕣𝕖𝕤𝕥))))
                  (≈⊙parse-list (cdr 𝕖𝕝𝕖𝕞)))))
             (≈⊙parse-one 𝕥𝕠𝕜𝕤))))
        (≈⊙token-type 𝕥𝕠𝕜)))
      (car 𝕥𝕠𝕜𝕤)))))

; ≈⊙parse :: string -> expr | error
; Parse string into S-expression
; Example: (≈⊙parse "(+ 1 2)") -> ⟨"+" ⟨"1" ⟨"2" ∅⟩⟩⟩
(define ≈⊙parse (lambda (𝕤)
  ((lambda (𝕥𝕠𝕜𝕤)
    (if (error? 𝕥𝕠𝕜𝕤)
       𝕥𝕠𝕜𝕤
       ((lambda (𝕣𝕖𝕤)
         (if (error? 𝕣𝕖𝕤)
            𝕣𝕖𝕤
            (car 𝕣𝕖𝕤)))
        (≈⊙parse-one 𝕥𝕠𝕜𝕤))))
   (≈⊙tokenize 𝕤))))

; ═══════════════════════════════════════════════════════════════
; End of Parser
; ═══════════════════════════════════════════════════════════════
