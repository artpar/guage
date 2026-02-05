; String Library for Guage
; Provides higher-level string manipulation utilities
; Built on primitive operations: ≈, ≈⊕, ≈#, ≈→, ≈⊂, ≈≡, string<?
; Day 121: Core operations now delegate to SIMD-accelerated C primitives

; ============================================================================
; Aliases: Map long names to C primitive symbols
; ============================================================================

; Search (Tier 1 — SIMD-accelerated)
(define string-index-of string-find)
(define string-rfind string-rfind)
(define string-contains? string-contains?)
(define string-starts-with? string-starts-with?)
(define string-ends-with? string-ends-with?)
(define string-count string-count)

; Transform (Tier 2)
(define string-reverse string-reverse)
(define string-repeat string-repeat)
(define string-replace string-replace)
(define string-replacen string-replace-n)

; Trim (Tier 3 — SIMD whitespace scan)
(define string-trim-left string-trim-left)
(define string-trim-right string-trim-right)
(define string-trim string-trim)

; Split (Tier 4 — SIMD delimiter scan)
(define string-split string-split)
(define string-splitn string-split-n)
(define string-fields string-fields)

; Pad (Tier 5)
(define string-pad-left string-pad-left)
(define string-pad-right string-pad-right)

; Strip (Tier 6)
(define string-strip-prefix string-strip-prefix)
(define string-strip-suffix string-strip-suffix)

; ============================================================================
; Core: Join (kept — no C primitive needed, pure Scheme is fine)
; ============================================================================

; ⌂: Join list of strings with delimiter
; ∈: [≈] -> string -> string
; Ex: (string-join ⟨"a" ⟨"b" ⟨"c" ∅⟩⟩⟩ ",") -> "a,b,c"
; Ex: (string-join ⟨"hello" ⟨"world" ∅⟩⟩ " ") -> "hello world"
(define string-join (lambda (lst delim)
  (if (null? lst)
     ""  ; Empty list -> empty string
     (if (null? (cdr lst))
        (car lst)  ; Single element -> just that element
        ; Multiple elements -> join with delimiter
        (string-append (car lst) (string-append delim (string-join (cdr lst) delim)))))))

; Alias: ≈⊗
(define ≈⊗ string-join)

; ============================================================================
; Extended: Split by newlines (thin wrapper)
; ============================================================================

; ⌂: Split string by newlines
; ∈: string -> [≈]
; Ex: (string-split-lines "a\nb\nc") -> ⟨"a" ⟨"b" ⟨"c" ∅⟩⟩⟩
(define string-split-lines (lambda (s)
  (string-split s "\n")))

; Alias: ≈÷⊳
(define ≈÷⊳ string-split-lines)

; ============================================================================
; Case conversion (delegates to C primitives)
; ============================================================================

; ⌂: Convert single-char symbol to uppercase (ASCII a-z only)
; ∈: :char -> :char
(define char-to-upper (lambda (c)
  ((lambda (code)
    (if (and (>= code #97) (<= code #122))
       (string-ref (code->char (- code #32)) #0)
       c))
   (string-char-code (string c) #0))))

; ⌂: Convert single-char symbol to lowercase (ASCII A-Z only)
; ∈: :char -> :char
(define char-to-lower (lambda (c)
  ((lambda (code)
    (if (and (>= code #65) (<= code #90))
       (string-ref (code->char (+ code #32)) #0)
       c))
   (string-char-code (string c) #0))))

; ⌂: Convert string to uppercase (ASCII only)
; ∈: string -> string
; Delegates to C-side string-upcase primitive for single-pass efficiency
(define string-upcase string-upcase)

; ⌂: Convert string to lowercase (ASCII only)
; ∈: string -> string
; Delegates to C-side string-downcase primitive for single-pass efficiency
(define string-downcase string-downcase)

; ============================================================================
; Module complete - Core string utilities available
; ============================================================================
