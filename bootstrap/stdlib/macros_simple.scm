; ═══════════════════════════════════════════════════════════════
; Guage Standard Library: Macros (Simplified Test)
; ═══════════════════════════════════════════════════════════════
; Testing with simple ASCII names first to isolate the issue
; ═══════════════════════════════════════════════════════════════

; Test 1: Simple unless macro with ASCII names
(macro unless (c b) (quasiquote-tilde (if (~ c) nil (~ b))))

; Test 2: Simple let macro with ASCII names
(macro mylet (v val body) (quasiquote-tilde ((lambda (v) (~ body)) (~ val))))

"✓ Macros loaded"
