; ═══════════════════════════════════════════════════════════════
; Guage Standard Library: Macros (Simplified Test)
; ═══════════════════════════════════════════════════════════════
; Testing with simple ASCII names first to isolate the issue
; ═══════════════════════════════════════════════════════════════

; Test 1: Simple unless macro with ASCII names
(⧉ unless (c b) (⌞̃ (? (~ c) ∅ (~ b))))

; Test 2: Simple let macro with ASCII names
(⧉ mylet (v val body) (⌞̃ ((λ (v) (~ body)) (~ val))))

"✓ Macros loaded"
