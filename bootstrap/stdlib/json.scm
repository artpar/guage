; Guage Standard Library: JSON Parser/Serializer
; Day 147 — Pure Guage recursive descent parser and serializer
;
; Uses: HashMap (⊞), Vector (⟦⟧), String primitives (≈)

; ============================================================================
; JSON Serializer (Guage → JSON string)
; ============================================================================

; ⌂: Serialize a Guage value to JSON string
; ∈: α → ≈
; Ex: (json-serialize #42) → "42"
; Ex: (json-serialize "hello") → "\"hello\""
; Ex: (json-serialize (⊞ (⟨⟩ "a" #1))) → "{\"a\":1}"
(≔ json-serialize (λ (val)
  (? (∅? val) "null"
  (? (𝔹? val) (? val "true" "false")
  (? (ℕ? val) (≈ val)
  (? (≈? val) (json-serialize-string val)
  (? (:? val) (json-serialize-string (≈ val))
  (? (⊞? val) (json-serialize-object val)
  (? (⟦? val) (json-serialize-array val)
  (? (⟨⟩? val) (json-serialize-list val)
     (≈ val)))))))))))

; ⌂: Escape and quote a string for JSON
; ∈: ≈ → ≈
(≔ json-serialize-string (λ (s)
  (≈⊕ "\""
    (≈⊕ (json-escape-string s #0 (≈# s) "")
         "\""))))

; ⌂: Escape special characters in string
; ∈: ≈ → ℕ → ℕ → ≈ → ≈
(≔ json-escape-string (λ (s i len acc)
  (? (≥ i len) acc
     (⪢
       (≔ code (≈→# s i))
       (≔ escaped
         (? (≡ code #34) "\\\""
         (? (≡ code #92) "\\\\"
         (? (≡ code #10) "\\n"
         (? (≡ code #13) "\\r"
         (? (≡ code #9)  "\\t"
         (? (≡ code #8)  "\\b"
         (? (≡ code #12) "\\f"
         (? (< code #32) ""
            (≈⊂ s i (⊕ i #1)))))))))))
       (json-escape-string s (⊕ i #1) len (≈⊕ acc escaped))))))

; ⌂: Serialize HashMap to JSON object
; ∈: ⊞ → ≈
(≔ json-serialize-object (λ (m)
  (⪢
    (≔ keys (⊞⊙ m))
    (≈⊕ "{"
      (≈⊕ (json-serialize-pairs m keys #t)
           "}")))))

; ⌂: Serialize key-value pairs
; ∈: ⊞ → [≈] → 𝔹 → ≈
(≔ json-serialize-pairs (λ (m keys first)
  (? (∅? keys) ""
     (⪢
       (≔ k (◁ keys))
       (≔ v (⊞→ m k))
       (≔ key-str (? (≈? k) k (≈ k)))
       (≔ entry (≈⊕ (json-serialize-string key-str)
                   (≈⊕ ":" (json-serialize v))))
       (≔ prefix (? first "" ","))
       (≈⊕ prefix (≈⊕ entry (json-serialize-pairs m (▷ keys) #f)))))))

; ⌂: Serialize Vector to JSON array
; ∈: ⟦⟧ → ≈
(≔ json-serialize-array (λ (v)
  (≈⊕ "["
    (≈⊕ (json-serialize-vec-items v #0 (⟦# v))
         "]"))))

; ⌂: Serialize vector items
; ∈: ⟦⟧ → ℕ → ℕ → ≈
(≔ json-serialize-vec-items (λ (v i len)
  (? (≥ i len) ""
     (⪢
       (≔ item (json-serialize (⟦→ v i)))
       (≔ prefix (? (≡ i #0) "" ","))
       (≈⊕ prefix (≈⊕ item (json-serialize-vec-items v (⊕ i #1) len)))))))

; ⌂: Serialize list (pair chain) to JSON array
; ∈: [α] → ≈
(≔ json-serialize-list (λ (lst)
  (≈⊕ "["
    (≈⊕ (json-serialize-list-items lst #t)
         "]"))))

; ⌂: Serialize list items
; ∈: [α] → 𝔹 → ≈
(≔ json-serialize-list-items (λ (lst first)
  (? (∅? lst) ""
     (⪢
       (≔ item (json-serialize (◁ lst)))
       (≔ prefix (? first "" ","))
       (≈⊕ prefix (≈⊕ item (json-serialize-list-items (▷ lst) #f)))))))

; ============================================================================
; JSON Parser (JSON string → Guage values)
; ============================================================================

; ⌂: Parse JSON string into Guage values
; ∈: ≈ → α|⚠
; Ex: (json-parse "42") → #42
; Ex: (json-parse "\"hello\"") → "hello"
; Ex: (json-parse "{\"a\":1}") → ⊞{a→1}
; Ex: (json-parse "[1,2,3]") → ⟦1 2 3⟧
(≔ json-parse (λ (src)
  (⪢
    (≔ result (json-parse-value src #0))
    (? (⚠? result)
       result
       (◁ result)))))

; ⌂: Skip whitespace, return new index
; ∈: ≈ → ℕ → ℕ
(≔ json-skip-ws (λ (src i)
  (? (≥ i (≈# src)) i
     (⪢
       (≔ c (≈→# src i))
       (? (∨ (≡ c #32) (∨ (≡ c #9) (∨ (≡ c #10) (≡ c #13))))
          (json-skip-ws src (⊕ i #1))
          i)))))

; ⌂: Parse any JSON value
; ∈: ≈ → ℕ → ⟨α ℕ⟩|⚠
(≔ json-parse-value (λ (src i)
  (⪢
    (≔ pos (json-skip-ws src i))
    (? (≥ pos (≈# src))
       (⚠ :json-unexpected-end pos)
       (⪢
         (≔ c (≈→# src pos))
         (? (≡ c #34) (json-parse-string src pos)
         (? (≡ c #123) (json-parse-object src pos)
         (? (≡ c #91) (json-parse-array src pos)
         (? (≡ c #116) (json-parse-true src pos)
         (? (≡ c #102) (json-parse-false src pos)
         (? (≡ c #110) (json-parse-null src pos)
         (? (∨ (≡ c #45) (∧ (≥ c #48) (≤ c #57)))
            (json-parse-number src pos)
            (⚠ :json-unexpected-char (⟨⟩ pos c))))))))))))))

; ⌂: Parse JSON string literal
; ∈: ≈ → ℕ → ⟨≈ ℕ⟩|⚠
(≔ json-parse-string (λ (src i)
  (json-parse-string-chars src (⊕ i #1) "")))

; ⌂: Parse string characters
; ∈: ≈ → ℕ → ≈ → ⟨≈ ℕ⟩|⚠
(≔ json-parse-string-chars (λ (src i acc)
  (? (≥ i (≈# src))
     (⚠ :json-unterminated-string i)
     (⪢
       (≔ c (≈→# src i))
       (? (≡ c #34)  ; closing "
          (⟨⟩ acc (⊕ i #1))
          (? (≡ c #92) ; backslash
             (? (≥ (⊕ i #1) (≈# src))
                (⚠ :json-unterminated-escape i)
                (⪢
                  (≔ next (≈→# src (⊕ i #1)))
                  (≔ escaped
                    (? (≡ next #34) "\""
                    (? (≡ next #92) "\\"
                    (? (≡ next #110) "\n"
                    (? (≡ next #114) "\r"
                    (? (≡ next #116) "\t"
                    (? (≡ next #98) "\b"
                    (? (≡ next #102) "\f"
                    (? (≡ next #47) "/"
                       (⚠ :json-bad-escape next))))))))))
                  (? (⚠? escaped) escaped
                     (json-parse-string-chars src (⊕ i #2) (≈⊕ acc escaped)))))
             (json-parse-string-chars src (⊕ i #1) (≈⊕ acc (≈⊂ src i (⊕ i #1))))))))))

; ⌂: Parse JSON number
; ∈: ≈ → ℕ → ⟨ℕ ℕ⟩|⚠
(≔ json-parse-number (λ (src i)
  (⪢
    (≔ p0 i)
    ; optional minus
    (≔ p1 (? (≡ (≈→# src p0) #45) (⊕ p0 #1) p0))
    ; digits
    (≔ p2 (json-scan-digits src p1))
    ; optional decimal
    (≔ p3 (? (∧ (< p2 (≈# src)) (≡ (≈→# src p2) #46))
             (json-scan-digits src (⊕ p2 #1))
             p2))
    ; optional exponent
    (≔ p4 (? (∧ (< p3 (≈# src)) (∨ (≡ (≈→# src p3) #101) (≡ (≈→# src p3) #69)))
             (⪢
               (≔ e1 (⊕ p3 #1))
               (≔ e2 (? (∧ (< e1 (≈# src)) (∨ (≡ (≈→# src e1) #43) (≡ (≈→# src e1) #45)))
                        (⊕ e1 #1) e1))
               (json-scan-digits src e2))
             p3))
    (≔ numstr (≈⊂ src p0 p4))
    (≔ n (≈→ℕ numstr))
    (? (⚠? n) (⚠ :json-bad-number numstr)
       (⟨⟩ n p4)))))

; ⌂: Scan consecutive digits
; ∈: ≈ → ℕ → ℕ
(≔ json-scan-digits (λ (src i)
  (? (≥ i (≈# src)) i
     (⪢
       (≔ c (≈→# src i))
       (? (∧ (≥ c #48) (≤ c #57))
          (json-scan-digits src (⊕ i #1))
          i)))))

; ⌂: Parse "true"
; ∈: ≈ → ℕ → ⟨𝔹 ℕ⟩|⚠
(≔ json-parse-true (λ (src i)
  (? (≡ (≈⊂ src i (⊕ i #4)) "true")
     (⟨⟩ #t (⊕ i #4))
     (⚠ :json-expected-true i))))

; ⌂: Parse "false"
; ∈: ≈ → ℕ → ⟨𝔹 ℕ⟩|⚠
(≔ json-parse-false (λ (src i)
  (? (≡ (≈⊂ src i (⊕ i #5)) "false")
     (⟨⟩ #f (⊕ i #5))
     (⚠ :json-expected-false i))))

; ⌂: Parse "null"
; ∈: ≈ → ℕ → ⟨∅ ℕ⟩|⚠
(≔ json-parse-null (λ (src i)
  (? (≡ (≈⊂ src i (⊕ i #4)) "null")
     (⟨⟩ ∅ (⊕ i #4))
     (⚠ :json-expected-null i))))

; ⌂: Parse JSON object
; ∈: ≈ → ℕ → ⟨⊞ ℕ⟩|⚠
(≔ json-parse-object (λ (src i)
  (⪢
    (≔ pos (json-skip-ws src (⊕ i #1)))
    (≔ m (⊞))
    (? (∧ (< pos (≈# src)) (≡ (≈→# src pos) #125))
       (⟨⟩ m (⊕ pos #1))
       (json-parse-object-pairs src pos m)))))

; ⌂: Parse object key-value pairs
; ∈: ≈ → ℕ → ⊞ → ⟨⊞ ℕ⟩|⚠
(≔ json-parse-object-pairs (λ (src i m)
  (⪢
    (≔ key-result (json-parse-string src i))
    (? (⚠? key-result) key-result
       (⪢
         (≔ key (◁ key-result))
         (≔ p1 (json-skip-ws src (▷ key-result)))
         (? (∨ (≥ p1 (≈# src)) (¬ (≡ (≈→# src p1) #58)))
            (⚠ :json-expected-colon p1)
            (⪢
              (≔ p2 (⊕ p1 #1))
              (≔ val-result (json-parse-value src p2))
              (? (⚠? val-result) val-result
                 (⪢
                   (⊞← m key (◁ val-result))
                   (≔ p3 (json-skip-ws src (▷ val-result)))
                   (? (≥ p3 (≈# src))
                      (⚠ :json-unterminated-object p3)
                      (⪢
                        (≔ c (≈→# src p3))
                        (? (≡ c #125)
                           (⟨⟩ m (⊕ p3 #1))
                           (? (≡ c #44)
                              (json-parse-object-pairs src (json-skip-ws src (⊕ p3 #1)) m)
                              (⚠ :json-expected-comma-or-brace p3))))))))))))))

; ⌂: Parse JSON array
; ∈: ≈ → ℕ → ⟨⟦⟧ ℕ⟩|⚠
(≔ json-parse-array (λ (src i)
  (⪢
    (≔ pos (json-skip-ws src (⊕ i #1)))
    (≔ v (⟦⟧))
    (? (∧ (< pos (≈# src)) (≡ (≈→# src pos) #93))
       (⟨⟩ v (⊕ pos #1))
       (json-parse-array-items src pos v)))))

; ⌂: Parse array items
; ∈: ≈ → ℕ → ⟦⟧ → ⟨⟦⟧ ℕ⟩|⚠
(≔ json-parse-array-items (λ (src i v)
  (⪢
    (≔ result (json-parse-value src i))
    (? (⚠? result) result
       (⪢
         (⟦⊕ v (◁ result))
         (≔ p1 (json-skip-ws src (▷ result)))
         (? (≥ p1 (≈# src))
            (⚠ :json-unterminated-array p1)
            (⪢
              (≔ c (≈→# src p1))
              (? (≡ c #93)
                 (⟨⟩ v (⊕ p1 #1))
                 (? (≡ c #44)
                    (json-parse-array-items src (json-skip-ws src (⊕ p1 #1)) v)
                    (⚠ :json-expected-comma-or-bracket p1))))))))))

; ============================================================================
; Symbolic Aliases
; ============================================================================

(≔ ⊞⊳json json-parse)       ; parse JSON
(≔ ⊞→json json-serialize)   ; serialize to JSON

; ============================================================================
; Module complete - JSON parse/serialize
; ============================================================================
