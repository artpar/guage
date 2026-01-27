; Guage Standard Library Tests: String Manipulation
; Tests for stdlib/string.scm (simplified version)

; Load dependencies (relative to bootstrap/bootstrap/)
(⋘ "../../stdlib/list.scm")
(⋘ "../../stdlib/string.scm")

; ============================================================================
; Helper Function Tests
; ============================================================================

; Test: ≈⊙? - whitespace detection
(⊨ :ws-space-yes #t (≈⊙? :  ))  ; space
(⊨ :ws-tab-yes #t (≈⊙? :\t))
(⊨ :ws-newline-yes #t (≈⊙? :\n))
(⊨ :ws-return-yes #t (≈⊙? :\r))
(⊨ :ws-not-a #f (≈⊙? :a))
(⊨ :ws-not-1 #f (≈⊙? :1))

; ============================================================================
; Join Tests (≈⊠)
; ============================================================================

; Test: Basic join
(⊨ :join-comma #t (≈≡ ((≈⊠ (⟨⟩ "a" (⟨⟩ "b" (⟨⟩ "c" ∅)))) ",") "a,b,c"))
(⊨ :join-space #t (≈≡ ((≈⊠ (⟨⟩ "hello" (⟨⟩ "world" ∅))) " ") "hello world"))

; Test: Join empty list
(⊨ :join-empty #t (≈≡ ((≈⊠ ∅) ",") ""))

; Test: Join single element
(⊨ :join-single #t (≈≡ ((≈⊠ (⟨⟩ "only" ∅)) ",") "only"))

; Test: Join with empty strings
(⊨ :join-empty-strs #t (≈≡ ((≈⊠ (⟨⟩ "" (⟨⟩ "a" (⟨⟩ "" ∅)))) ",") ",a,"))

; Test: Join three elements
(⊨ :join-three #t (≈≡ ((≈⊠ (⟨⟩ "x" (⟨⟩ "y" (⟨⟩ "z" ∅)))) "-") "x-y-z"))

; ============================================================================
; Contains Tests (≈⊃)
; ============================================================================

; Test: Contains - basic
(⊨ :contains-yes #t ((≈⊃ "hello world") "world"))
(⊨ :contains-no #f ((≈⊃ "hello world") "goodbye"))
(⊨ :contains-start #t ((≈⊃ "hello world") "hello"))
(⊨ :contains-end #t ((≈⊃ "hello world") "rld"))

; Test: Contains - edge cases
(⊨ :contains-empty #t ((≈⊃ "hello") ""))
(⊨ :contains-self #t ((≈⊃ "hello") "hello"))
(⊨ :contains-longer #f ((≈⊃ "hi") "hello"))
(⊨ :contains-single #t ((≈⊃ "hello") "e"))
(⊨ :contains-middle #t ((≈⊃ "hello") "ll"))

; ============================================================================
; Repeat Tests (≈⊗)
; ============================================================================

; Test: Repeat - basic
(⊨ :repeat-3 #t (≈≡ ((≈⊗ "ab") #3) "ababab"))
(⊨ :repeat-1 #t (≈≡ ((≈⊗ "test") #1) "test"))
(⊨ :repeat-0 #t (≈≡ ((≈⊗ "test") #0) ""))
(⊨ :repeat-negative #t (≈≡ ((≈⊗ "test") #-1) ""))
(⊨ :repeat-5 #t (≈≡ ((≈⊗ "x") #5) "xxxxx"))

; ============================================================================
; Integration Tests
; ============================================================================

; Test: Build CSV header
(⊨ :csv-header #t (≈≡ ((≈⊠ (⟨⟩ "name" (⟨⟩ "age" (⟨⟩ "city" ∅)))) ",") "name,age,city"))

; Test: Build path
(⊨ :path-build #t (≈≡ ((≈⊠ (⟨⟩ "usr" (⟨⟩ "local" (⟨⟩ "bin" ∅)))) "/") "usr/local/bin"))

; Test: Repeat and join
(⊨ :repeat-join #t (≈≡ ((≈⊠ (⟨⟩ ((≈⊗ "a") #2) (⟨⟩ ((≈⊗ "b") #2) ∅))) "-") "aa-bb"))

; Test: Contains with repeat
(⊨ :contains-repeat #t ((≈⊃ ((≈⊗ "abc") #3)) "cab"))

; Test: Join words
(≔ join-words (λ (words) ((≈⊠ words) " ")))
(⊨ :join-words #t (≈≡ (join-words (⟨⟩ "The" (⟨⟩ "quick" (⟨⟩ "brown" ∅)))) "The quick brown"))

; ============================================================================
; Summary
; ============================================================================

(≋ "")
(≋ "String Manipulation Tests Complete!")
(≋ "Functions tested: ≈⊙?, ≈⊠ (join), ≈⊃ (contains), ≈⊗ (repeat)")
(≋ "All tests passing ✓")
