; Documentation Formatting Library
; Makes auto-generated documentation human-readable

; ≈⊙doc-simple :: :symbol -> string
; Simple doc formatter that works
(define ≈⊙doc-simple (lambda (sym)
  (string-append "Symbol: "
  (string-append (string sym)
  (string-append "\nType: "
  (string-append (string (doc-type sym))
  (string-append "\nDescription: "
  (string-append (string (doc sym))
  "\n"))))))))

; ≈⊙doc-format :: :symbol -> string
; Format complete documentation for a symbol with box drawing
(define ≈⊙doc-format (lambda (sym)
  (string-append "┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n"
  (string-append "┃ doc-generate "
  (string-append (string sym)
  (string-append "\n┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫\n"
  (string-append "┃ Type: "
  (string-append (string (doc-type sym))
  (string-append "\n┃\n┃ "
  (string-append (string (doc sym))
  "\n┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n"))))))))))

; ≈⊙doc-deps :: :symbol -> string
; Format dependencies nicely
(define ≈⊙doc-deps (lambda (sym)
  (string-append "Dependencies of "
  (string-append (string sym)
  (string-append ": "
  (string-append (string (doc-deps sym))
  "\n"))))))

; Test examples:
; (≈⊙doc-simple (quote +))
; (≈⊙doc-format (quote +))
; (≈⊙doc-deps (quote +))
