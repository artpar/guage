; ═══════════════════════════════════════════════════════════════
; Parser Tests - Day 39
; ═══════════════════════════════════════════════════════════════

; Load dependencies
(⋘ "stdlib/macros.scm")
(⋘ "stdlib/parser.scm")

; ───────────────────────────────────────────────────────────────
; Test Tokenization
; ───────────────────────────────────────────────────────────────

; Test 1: Tokenize simple number
(⊨ (⌜ :tokenize-number)
    #t
    (∧… (¬ (∅? (≈⊙tokenize "42")))
        (≡ (≈⊙token-type (◁ (≈⊙tokenize "42"))) (⌜ :number))))

; Test 2: Tokenize simple symbol
(⊨ (⌜ :tokenize-symbol)
    #t
    (∧… (¬ (∅? (≈⊙tokenize "foo")))
        (≡ (≈⊙token-type (◁ (≈⊙tokenize "foo"))) (⌜ :symbol))))

; Test 3: Tokenize list
(⊨ (⌜ :tokenize-list)
    #t
    (∧… (¬ (∅? (≈⊙tokenize "(foo)")))
        (≡ (≈⊙token-type (◁ (≈⊙tokenize "(foo)"))) (⌜ :lparen))))

; Test 4: Skip whitespace
(⊨ (⌜ :skip-whitespace)
    #t
    (∧… (¬ (∅? (≈⊙tokenize "  foo")))
        (≡ (≈⊙token-type (◁ (≈⊙tokenize "  foo"))) (⌜ :symbol))))

; Test 5: Skip comment
(⊨ (⌜ :skip-comment)
    #t
    (∧… (¬ (∅? (≈⊙tokenize "; comment\nfoo")))
        (≡ (≈⊙token-type (◁ (≈⊙tokenize "; comment\nfoo"))) (⌜ :symbol))))

; ───────────────────────────────────────────────────────────────
; Test Parsing
; ───────────────────────────────────────────────────────────────

; Test 6: Parse simple number
(⊨ (⌜ :parse-number)
    #t
    (≈? (≈⊙parse "42")))

; Test 7: Parse simple symbol
(⊨ (⌜ :parse-symbol)
    #t
    (≈? (≈⊙parse "foo")))

; Test 8: Parse empty list
(⊨ (⌜ :parse-empty-list)
    #t
    (∅? (≈⊙parse "()")))

; Test 9: Parse single-element list
(⊨ (⌜ :parse-single-list)
    #t
    (∧… (¬ (∅? (≈⊙parse "(foo)")))
        (≈? (◁ (≈⊙parse "(foo)")))))

; Test 10: Parse two-element list
(⊨ (⌜ :parse-two-list)
    #t
    (∧… (¬ (∅? (≈⊙parse "(foo bar)")))
        (¬ (∅? (▷ (≈⊙parse "(foo bar)"))))))

; ───────────────────────────────────────────────────────────────
; Integration Tests
; ───────────────────────────────────────────────────────────────

; Test 11: Parse nested list
(⊨ (⌜ :parse-nested)
    #t
    (¬ (∅? (≈⊙parse "((foo))"))))

; Test 12: Parse arithmetic expression
(⊨ (⌜ :parse-arithmetic)
    #t
    (¬ (∅? (≈⊙parse "(+ 1 2)"))))

; Test 13: Error on unclosed list
(⊨ (⌜ :error-unclosed)
    #t
    (⚠? (≈⊙parse "(foo")))

; Test 14: Error on unexpected rparen
(⊨ (⌜ :error-extra-rparen)
    #t
    (⚠? (≈⊙parse ")")))

; Test 15: Parse string literal
(⊨ (⌜ :parse-string)
    #t
    (≈? (≈⊙parse "\"hello\"")))

; ═══════════════════════════════════════════════════════════════
; End of Tests
; ═══════════════════════════════════════════════════════════════
