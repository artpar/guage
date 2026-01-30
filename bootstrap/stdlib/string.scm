; String Library for Guage
; Provides higher-level string manipulation utilities
; Built on primitive operations: ≈, ≈⊕, ≈#, ≈→, ≈⊂, ≈≡, ≈<

; ============================================================================
; Helper: Character predicates
; ============================================================================

(≔ char-is-space? (λ (c)
  (∨ (≡ c (≈→ " " #0))
  (∨ (≡ c (≈→ "\t" #0))
  (∨ (≡ c (≈→ "\n" #0))
     (≡ c (≈→ "\r" #0)))))))

; ============================================================================
; Core: Split
; ============================================================================

; Helper: Find next delimiter position in string
(≔ string-split-find-delim (λ (s pos delim delim-len)
  (? (> (⊕ pos delim-len) (≈# s))
     ∅  ; Past end of string
     (? (≈≡ (≈⊂ s pos (⊕ pos delim-len)) delim)
        pos  ; Found delimiter
        (string-split-find-delim s (⊕ pos #1) delim delim-len)))))

; Helper: Split recursively collecting parts
(≔ string-split-helper (λ (s start delim delim-len acc)
  ((λ (delim-pos)
     (? (∅? delim-pos)
        ; No more delimiters - add rest of string (reversed, so prepend)
        (⟨⟩ (≈⊂ s start (≈# s)) acc)
        ; Found delimiter - extract part and continue
        (string-split-helper s (⊕ delim-pos delim-len) delim delim-len
                            (⟨⟩ (≈⊂ s start delim-pos) acc))))
   (string-split-find-delim s start delim delim-len))))

; Helper: Split string into individual characters
(≔ string-split-chars (λ (s pos acc)
  (? (≥ pos (≈# s))
     acc
     (string-split-chars s (⊕ pos #1)
                        (⟨⟩ (≈ (≈→ s pos)) acc)))))

; Helper: Reverse a list
(≔ string-split-reverse (λ (lst acc)
  (? (∅? lst)
     acc
     (string-split-reverse (▷ lst) (⟨⟩ (◁ lst) acc)))))

; ⌂: Split string by delimiter into list of strings
; ∈: ≈ → ≈ → [≈]
; Ex: (string-split "a,b,c" ",") → ⟨"a" ⟨"b" ⟨"c" ∅⟩⟩⟩
; Ex: (string-split "hello" "") → ⟨"h" ⟨"e" ⟨"l" ⟨"l" ⟨"o" ∅⟩⟩⟩⟩⟩
(≔ string-split (λ (str delim)
  (? (≈∅? delim)
     ; Empty delimiter - split into characters
     (string-split-reverse (string-split-chars str #0 ∅) ∅)
     ; Normal delimiter split
     (? (≈∅? str)
        ∅  ; Empty string → empty list
        (string-split-reverse
          (string-split-helper str #0 delim (≈# delim) ∅)
          ∅)))))

; Alias: ≈÷
(≔ ≈÷ string-split)

; ============================================================================
; Core: Join
; ============================================================================

; ⌂: Join list of strings with delimiter
; ∈: [≈] → ≈ → ≈
; Ex: (string-join ⟨"a" ⟨"b" ⟨"c" ∅⟩⟩⟩ ",") → "a,b,c"
; Ex: (string-join ⟨"hello" ⟨"world" ∅⟩⟩ " ") → "hello world"
(≔ string-join (λ (lst delim)
  (? (∅? lst)
     ""  ; Empty list → empty string
     (? (∅? (▷ lst))
        (◁ lst)  ; Single element → just that element
        ; Multiple elements → join with delimiter
        (≈⊕ (◁ lst) (≈⊕ delim (string-join (▷ lst) delim)))))))

; Alias: ≈⊗
(≔ ≈⊗ string-join)

; ============================================================================
; Core: Trim
; ============================================================================

(≔ string-trim-left-helper (λ (str pos)
  (? (≥ pos (≈# str))
     ""  ; All whitespace
     (? (char-is-space? (≈→ str pos))
        (string-trim-left-helper str (⊕ pos #1))  ; Skip whitespace
        (≈⊂ str pos (≈# str))))))  ; Return rest

(≔ string-trim-left (λ (s)
  (string-trim-left-helper s #0)))

(≔ string-trim-right-helper (λ (str pos)
  (? (< pos #0)
     ""  ; All whitespace
     (? (char-is-space? (≈→ str pos))
        (string-trim-right-helper str (⊖ pos #1))  ; Skip whitespace
        (≈⊂ str #0 (⊕ pos #1))))))  ; Return up to here

(≔ string-trim-right (λ (s)
  (string-trim-right-helper s (⊖ (≈# s) #1))))

(≔ string-trim (λ (s)
  (string-trim-right (string-trim-left s))))

; Alias: ≈⊏⊐
(≔ ≈⊏⊐ string-trim)

; ============================================================================
; Core: Contains
; ============================================================================

(≔ string-contains-search (λ (haystack needle needle-len max-pos pos)
  (? (> pos max-pos)
     #f  ; Not found
     (? (≈≡ (≈⊂ haystack pos (⊕ pos needle-len)) needle)
        #t  ; Found it!
        (string-contains-search haystack needle needle-len max-pos (⊕ pos #1))))))

(≔ string-contains? (λ (haystack needle)
  (? (≈∅? needle)
     #t  ; Empty string is always contained
     ((λ (needle-len)
        ((λ (max-pos)
           (string-contains-search haystack needle needle-len max-pos #0))
         (⊖ (≈# haystack) needle-len)))
      (≈# needle)))))

; Alias: ≈∈?
(≔ ≈∈? string-contains?)

; ============================================================================
; Core: Replace
; ============================================================================

(≔ string-replace-helper (λ (str old new old-len str-len pos acc)
  (? (≥ pos str-len)
     acc  ; Done - return accumulated result
     (? (∧ (≤ (⊕ pos old-len) str-len)
           (≈≡ (≈⊂ str pos (⊕ pos old-len)) old))
        ; Found match - replace and skip past it
        (string-replace-helper str old new old-len str-len (⊕ pos old-len) (≈⊕ acc new))
        ; No match - copy one character and continue
        (string-replace-helper str old new old-len str-len (⊕ pos #1) (≈⊕ acc (≈ (≈→ str pos))))))))

(≔ string-replace (λ (str old new)
  (? (≈∅? old)
     str  ; Can't replace empty string
     ((λ (old-len)
        ((λ (str-len)
           (string-replace-helper str old new old-len str-len #0 ""))
         (≈# str)))
      (≈# old)))))

; Alias: ≈⇔
(≔ ≈⇔ string-replace)

; ============================================================================
; Extended: Split variants
; ============================================================================

; ⌂: Split string by newlines
; ∈: ≈ → [≈]
; Ex: (string-split-lines "a\nb\nc") → ⟨"a" ⟨"b" ⟨"c" ∅⟩⟩⟩
(≔ string-split-lines (λ (s)
  (string-split s "\n")))

; Alias: ≈÷⊳
(≔ ≈÷⊳ string-split-lines)

; ============================================================================
; Extended: Index-of
; ============================================================================

(≔ string-index-of-search (λ (haystack needle needle-len max-pos pos)
  (? (> pos max-pos)
     ∅  ; Not found
     (? (≈≡ (≈⊂ haystack pos (⊕ pos needle-len)) needle)
        pos  ; Found it!
        (string-index-of-search haystack needle needle-len max-pos (⊕ pos #1))))))

(≔ string-index-of (λ (haystack needle)
  (? (≈∅? needle)
     #0  ; Empty string found at position 0
     ((λ (needle-len)
        ((λ (max-pos)
           (string-index-of-search haystack needle needle-len max-pos #0))
         (⊖ (≈# haystack) needle-len)))
      (≈# needle)))))

; Alias: ≈⊳
(≔ ≈⊳ string-index-of)

; ============================================================================
; Extended: Case conversion (basic ASCII)
; ============================================================================

; ⌂: Convert single-char symbol to uppercase (ASCII a-z only)
; ∈: :char → :char
(≔ char-to-upper (λ (c)
  ((λ (code)
    (? (∧ (≥ code #97) (≤ code #122))
       (≈→ (#→≈ (⊖ code #32)) #0)
       c))
   (≈→# (≈ c) #0))))

; ⌂: Convert single-char symbol to lowercase (ASCII A-Z only)
; ∈: :char → :char
(≔ char-to-lower (λ (c)
  ((λ (code)
    (? (∧ (≥ code #65) (≤ code #90))
       (≈→ (#→≈ (⊕ code #32)) #0)
       c))
   (≈→# (≈ c) #0))))

; ⌂: Convert string to uppercase (ASCII only)
; ∈: ≈ → ≈
; Delegates to C-side ≈↑ primitive for single-pass efficiency
(≔ string-upcase ≈↑)

; ⌂: Convert string to lowercase (ASCII only)
; ∈: ≈ → ≈
; Delegates to C-side ≈↓ primitive for single-pass efficiency
(≔ string-downcase ≈↓)

; ============================================================================
; Module complete - Core string utilities available
; ============================================================================
