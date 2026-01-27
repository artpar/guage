; Guage Standard Library: String Manipulation
; Pure symbolic names, named parameters (converted to De Bruijn internally)

; NOTE: This is a simplified first version focusing on the most useful functions
; Complex functions like split will be added once the implementation patterns are clearer

; ============================================================================
; Helper Functions
; ============================================================================

; ≈⊙? :: :symbol → 𝔹
; Check if character (as symbol) is whitespace
(≔ ≈⊙? (λ (ch)
  (∨ (∨ (≡ ch :  )  ; space (note: colon followed by space)
        (≡ ch :\t))  ; tab
     (∨ (≡ ch :\n)   ; newline
        (≡ ch :\r))))) ; carriage return

; ============================================================================
; String Manipulation Functions
; ============================================================================

; ≈⊠ :: [≈] → ≈ → ≈
; Join list of strings with delimiter
(≔ ≈⊠ (λ (lst) (λ (delim)
  (? (∅? lst)
     ""
     ; Use lambda to bind first and rest
     ((λ (first)
       ((λ (rest)
         (? (∅? rest)
            first
            (((⊕← (λ (acc) (λ (s) (≈⊕ (≈⊕ acc delim) s)))) first) rest)))
        (▷ lst)))
      (◁ lst))))))

; ≈⊲ :: ≈ → ≈
; Trim whitespace from both ends of string (basic implementation)
; Note: For now, just removes leading/trailing spaces
; Full whitespace trimming requires character-by-character scanning
(≔ ≈⊲ (λ (s)
  ; For now, identity function - proper implementation complex
  ; Will be enhanced when character iteration patterns are clearer
  s))

; ≈⊗ :: ≈ → ℕ → ≈
; Repeat string n times
(≔ ≈⊗ (λ (s) (λ (n)
  (? (≤ n #0)
     ""
     (≈⊕ s ((≈⊗ s) (⊖ n #1)))))))

; ============================================================================
; String Predicates
; ============================================================================

; ≈⊃→ :: ≈ → ≈ → ℕ → 𝔹
; Helper: Check if substring exists at position i
(≔ ≈⊃→ (λ (s) (λ (sub) (λ (i)
  (? (> (⊕ i (≈# sub)) (≈# s))
     #f
     (? (≈≡ (≈⊂ s i (⊕ i (≈# sub))) sub)
        #t
        (((≈⊃→ s) sub) (⊕ i #1))))))))

; ≈⊃ :: ≈ → ≈ → 𝔹
; Check if string contains substring
(≔ ≈⊃ (λ (s) (λ (sub)
  (? (≈∅? sub)
     #t
     (? (> (≈# sub) (≈# s))
        #f
        (((≈⊃→ s) sub) #0))))))

; ============================================================================
; Future Functions (TODO)
; ============================================================================

; The following functions require more complex character-by-character processing
; and will be added once the implementation patterns are more established:
;
; ≈⊞ :: ≈ → :symbol → [≈]  (split - complex recursion)
; ≈⊳ :: ≈ → ≈  (trim-left - needs char iteration)
; ≈⊴ :: ≈ → ≈  (trim-right - needs char iteration)
; ≈↑ :: ≈ → ≈  (uppercase - needs char arithmetic)
; ≈↓ :: ≈ → ≈  (lowercase - needs char arithmetic)
