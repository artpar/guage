; ═══════════════════════════════════════════════════════════════
; Test: Single-character Unicode mathematical letters
; ═══════════════════════════════════════════════════════════════

; Test with single Unicode mathematical letters
(macro unless (𝕩 𝕪) (quasiquote-tilde (if (~ 𝕩) nil (~ 𝕪))))

; Test with more single Unicode letters
(macro let-local (𝕧 𝕨 𝕓) (quasiquote-tilde ((lambda (𝕧) (~ 𝕓)) (~ 𝕨))))

"✓ Unicode macros loaded"
