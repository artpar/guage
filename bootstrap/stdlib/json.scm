; Guage Standard Library: JSON Parser/Serializer
; Day 147 — Pure Guage recursive descent parser and serializer
;
; Uses: HashMap (hashmap), Vector (vector), String primitives (string)

; ============================================================================
; JSON Serializer (Guage -> JSON string)
; ============================================================================

; ⌂: Serialize a Guage value to JSON string
; ∈: α -> string
; Ex: (json-serialize #42) -> "42"
; Ex: (json-serialize "hello") -> "\"hello\""
; Ex: (json-serialize (hashmap (cons "a" #1))) -> "{\"a\":1}"
(define json-serialize (lambda (val)
  (if (null? val) "null"
  (if (boolean? val) (if val "true" "false")
  (if (number? val) (string val)
  (if (string? val) (json-serialize-string val)
  (if (symbol? val) (json-serialize-string (string val))
  (if (hashmap? val) (json-serialize-object val)
  (if (vector? val) (json-serialize-array val)
  (if (pair? val) (json-serialize-list val)
     (string val)))))))))))

; ⌂: Escape and quote a string for JSON
; ∈: string -> string
(define json-serialize-string (lambda (s)
  (string-append "\""
    (string-append (json-escape-string s #0 (string-length s) "")
         "\""))))

; ⌂: Escape special characters in string
; ∈: string -> ℕ -> ℕ -> string -> string
(define json-escape-string (lambda (s i len acc)
  (if (>= i len) acc
     (begin
       (define code (string-char-code s i))
       (define escaped
         (if (equal? code #34) "\\\""
         (if (equal? code #92) "\\\\"
         (if (equal? code #10) "\\n"
         (if (equal? code #13) "\\r"
         (if (equal? code #9)  "\\t"
         (if (equal? code #8)  "\\b"
         (if (equal? code #12) "\\f"
         (if (< code #32) ""
            (string-slice s i (+ i #1)))))))))))
       (json-escape-string s (+ i #1) len (string-append acc escaped))))))

; ⌂: Serialize HashMap to JSON object
; ∈: hashmap -> string
(define json-serialize-object (lambda (m)
  (begin
    (define keys (hashmap-keys m))
    (string-append "{"
      (string-append (json-serialize-pairs m keys #t)
           "}")))))

; ⌂: Serialize key-value pairs
; ∈: hashmap -> [≈] -> Bool -> string
(define json-serialize-pairs (lambda (m keys first)
  (if (null? keys) ""
     (begin
       (define k (car keys))
       (define v (hashmap-get m k))
       (define key-str (if (string? k) k (string k)))
       (define entry (string-append (json-serialize-string key-str)
                   (string-append ":" (json-serialize v))))
       (define prefix (if first "" ","))
       (string-append prefix (string-append entry (json-serialize-pairs m (cdr keys) #f)))))))

; ⌂: Serialize Vector to JSON array
; ∈: vector -> string
(define json-serialize-array (lambda (v)
  (string-append "["
    (string-append (json-serialize-vec-items v #0 (vector-length v))
         "]"))))

; ⌂: Serialize vector items
; ∈: vector -> ℕ -> ℕ -> string
(define json-serialize-vec-items (lambda (v i len)
  (if (>= i len) ""
     (begin
       (define item (json-serialize (vector-ref v i)))
       (define prefix (if (equal? i #0) "" ","))
       (string-append prefix (string-append item (json-serialize-vec-items v (+ i #1) len)))))))

; ⌂: Serialize list (pair chain) to JSON array
; ∈: [α] -> string
(define json-serialize-list (lambda (lst)
  (string-append "["
    (string-append (json-serialize-list-items lst #t)
         "]"))))

; ⌂: Serialize list items
; ∈: [α] -> Bool -> string
(define json-serialize-list-items (lambda (lst first)
  (if (null? lst) ""
     (begin
       (define item (json-serialize (car lst)))
       (define prefix (if first "" ","))
       (string-append prefix (string-append item (json-serialize-list-items (cdr lst) #f)))))))

; ============================================================================
; JSON Parser (JSON string -> Guage values)
; ============================================================================

; ⌂: Parse JSON string into Guage values
; ∈: string -> α|⚠
; Ex: (json-parse "42") -> #42
; Ex: (json-parse "\"hello\"") -> "hello"
; Ex: (json-parse "{\"a\":1}") -> ⊞{a→1}
; Ex: (json-parse "[1,2,3]") -> ⟦1 2 3⟧
(define json-parse (lambda (src)
  (begin
    (define result (json-parse-value src #0))
    (if (error? result)
       result
       (car result)))))

; ⌂: Skip whitespace, return new index
; ∈: string -> ℕ -> ℕ
(define json-skip-ws (lambda (src i)
  (if (>= i (string-length src)) i
     (begin
       (define c (string-char-code src i))
       (if (or (equal? c #32) (or (equal? c #9) (or (equal? c #10) (equal? c #13))))
          (json-skip-ws src (+ i #1))
          i)))))

; ⌂: Parse any JSON value
; ∈: string -> ℕ -> ⟨α ℕ⟩|⚠
(define json-parse-value (lambda (src i)
  (begin
    (define pos (json-skip-ws src i))
    (if (>= pos (string-length src))
       (error :json-unexpected-end pos)
       (begin
         (define c (string-char-code src pos))
         (if (equal? c #34) (json-parse-string src pos)
         (if (equal? c #123) (json-parse-object src pos)
         (if (equal? c #91) (json-parse-array src pos)
         (if (equal? c #116) (json-parse-true src pos)
         (if (equal? c #102) (json-parse-false src pos)
         (if (equal? c #110) (json-parse-null src pos)
         (if (or (equal? c #45) (and (>= c #48) (<= c #57)))
            (json-parse-number src pos)
            (error :json-unexpected-char (cons pos c))))))))))))))

; ⌂: Parse JSON string literal
; ∈: string -> ℕ -> ⟨≈ ℕ⟩|⚠
(define json-parse-string (lambda (src i)
  (json-parse-string-chars src (+ i #1) "")))

; ⌂: Parse string characters
; ∈: string -> ℕ -> string -> ⟨≈ ℕ⟩|⚠
(define json-parse-string-chars (lambda (src i acc)
  (if (>= i (string-length src))
     (error :json-unterminated-string i)
     (begin
       (define c (string-char-code src i))
       (if (equal? c #34)  ; closing "
          (cons acc (+ i #1))
          (if (equal? c #92) ; backslash
             (if (>= (+ i #1) (string-length src))
                (error :json-unterminated-escape i)
                (begin
                  (define next (string-char-code src (+ i #1)))
                  (define escaped
                    (if (equal? next #34) "\""
                    (if (equal? next #92) "\\"
                    (if (equal? next #110) "\n"
                    (if (equal? next #114) "\r"
                    (if (equal? next #116) "\t"
                    (if (equal? next #98) "\b"
                    (if (equal? next #102) "\f"
                    (if (equal? next #47) "/"
                       (error :json-bad-escape next))))))))))
                  (if (error? escaped) escaped
                     (json-parse-string-chars src (+ i #2) (string-append acc escaped)))))
             (json-parse-string-chars src (+ i #1) (string-append acc (string-slice src i (+ i #1))))))))))

; ⌂: Parse JSON number
; ∈: string -> ℕ -> ⟨ℕ ℕ⟩|⚠
(define json-parse-number (lambda (src i)
  (begin
    (define p0 i)
    ; optional minus
    (define p1 (if (equal? (string-char-code src p0) #45) (+ p0 #1) p0))
    ; digits
    (define p2 (json-scan-digits src p1))
    ; optional decimal
    (define p3 (if (and (< p2 (string-length src)) (equal? (string-char-code src p2) #46))
             (json-scan-digits src (+ p2 #1))
             p2))
    ; optional exponent
    (define p4 (if (and (< p3 (string-length src)) (or (equal? (string-char-code src p3) #101) (equal? (string-char-code src p3) #69)))
             (begin
               (define e1 (+ p3 #1))
               (define e2 (if (and (< e1 (string-length src)) (or (equal? (string-char-code src e1) #43) (equal? (string-char-code src e1) #45)))
                        (+ e1 #1) e1))
               (json-scan-digits src e2))
             p3))
    (define numstr (string-slice src p0 p4))
    (define n (string->number numstr))
    (if (error? n) (error :json-bad-number numstr)
       (cons n p4)))))

; ⌂: Scan consecutive digits
; ∈: string -> ℕ -> ℕ
(define json-scan-digits (lambda (src i)
  (if (>= i (string-length src)) i
     (begin
       (define c (string-char-code src i))
       (if (and (>= c #48) (<= c #57))
          (json-scan-digits src (+ i #1))
          i)))))

; ⌂: Parse "true"
; ∈: string -> ℕ -> ⟨𝔹 ℕ⟩|⚠
(define json-parse-true (lambda (src i)
  (if (equal? (string-slice src i (+ i #4)) "true")
     (cons #t (+ i #4))
     (error :json-expected-true i))))

; ⌂: Parse "false"
; ∈: string -> ℕ -> ⟨𝔹 ℕ⟩|⚠
(define json-parse-false (lambda (src i)
  (if (equal? (string-slice src i (+ i #5)) "false")
     (cons #f (+ i #5))
     (error :json-expected-false i))))

; ⌂: Parse "null"
; ∈: string -> ℕ -> ⟨∅ ℕ⟩|⚠
(define json-parse-null (lambda (src i)
  (if (equal? (string-slice src i (+ i #4)) "null")
     (cons nil (+ i #4))
     (error :json-expected-null i))))

; ⌂: Parse JSON object
; ∈: string -> ℕ -> ⟨⊞ ℕ⟩|⚠
(define json-parse-object (lambda (src i)
  (begin
    (define pos (json-skip-ws src (+ i #1)))
    (define m (hashmap))
    (if (and (< pos (string-length src)) (equal? (string-char-code src pos) #125))
       (cons m (+ pos #1))
       (json-parse-object-pairs src pos m)))))

; ⌂: Parse object key-value pairs
; ∈: string -> ℕ -> hashmap -> ⟨⊞ ℕ⟩|⚠
(define json-parse-object-pairs (lambda (src i m)
  (begin
    (define key-result (json-parse-string src i))
    (if (error? key-result) key-result
       (begin
         (define key (car key-result))
         (define p1 (json-skip-ws src (cdr key-result)))
         (if (or (>= p1 (string-length src)) (not (equal? (string-char-code src p1) #58)))
            (error :json-expected-colon p1)
            (begin
              (define p2 (+ p1 #1))
              (define val-result (json-parse-value src p2))
              (if (error? val-result) val-result
                 (begin
                   (hashmap-put m key (car val-result))
                   (define p3 (json-skip-ws src (cdr val-result)))
                   (if (>= p3 (string-length src))
                      (error :json-unterminated-object p3)
                      (begin
                        (define c (string-char-code src p3))
                        (if (equal? c #125)
                           (cons m (+ p3 #1))
                           (if (equal? c #44)
                              (json-parse-object-pairs src (json-skip-ws src (+ p3 #1)) m)
                              (error :json-expected-comma-or-brace p3))))))))))))))

; ⌂: Parse JSON array
; ∈: string -> ℕ -> ⟨⟦⟧ ℕ⟩|⚠
(define json-parse-array (lambda (src i)
  (begin
    (define pos (json-skip-ws src (+ i #1)))
    (define v (vector))
    (if (and (< pos (string-length src)) (equal? (string-char-code src pos) #93))
       (cons v (+ pos #1))
       (json-parse-array-items src pos v)))))

; ⌂: Parse array items
; ∈: string -> ℕ -> vector -> ⟨⟦⟧ ℕ⟩|⚠
(define json-parse-array-items (lambda (src i v)
  (begin
    (define result (json-parse-value src i))
    (if (error? result) result
       (begin
         (vector-push! v (car result))
         (define p1 (json-skip-ws src (cdr result)))
         (if (>= p1 (string-length src))
            (error :json-unterminated-array p1)
            (begin
              (define c (string-char-code src p1))
              (if (equal? c #93)
                 (cons v (+ p1 #1))
                 (if (equal? c #44)
                    (json-parse-array-items src (json-skip-ws src (+ p1 #1)) v)
                    (error :json-expected-comma-or-bracket p1))))))))))

; ============================================================================
; Symbolic Aliases
; ============================================================================

(define ⊞⊳json json-parse)       ; parse JSON
(define ⊞→json json-serialize)   ; serialize to JSON

; ============================================================================
; Module complete - JSON parse/serialize
; ============================================================================
