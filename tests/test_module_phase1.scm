; test_module_phase1.scm - Phase 1: Module as Value
; Only tests basic module structure (no stdlib dependencies)

; ============================================================================
; Setup: Create a test module file
; ============================================================================

(≔ test-code "(≔ double (λ (n) (⊗ n #2)))\n(≔ triple (λ (n) (⊗ n #3)))")
(≋⊲ "test_simple_mod.scm" test-code)

; ============================================================================
; PHASE 1A: Load returns module structure
; ============================================================================

; Load file - should return Module structure (not evaluation result)
(≔ mod (⋘ "test_simple_mod.scm"))

; Test 1: Module is a structure
(⊨ :mod-is-struct #t (⊙? mod))

; Test 2: Module has :path field
(⊨ :has-path-field #t (⊙? mod :path))

; Test 3: Path is correct
(⊨ :path-correct "test_simple_mod.scm" (⊙→ mod :path))

; ============================================================================
; PHASE 1B: Module contains definitions
; ============================================================================

; Test 4: Module has :defs field
(⊨ :has-defs-field #t (⊙? mod :defs))

; Get defs list
(≔ defs (⊙→ mod :defs))

; Test 5: Defs is a list (pair)
(⊨ :defs-is-list #t (⟨⟩? defs))

; Test 6: Can extract symbols from defs
; Defs should be list of (symbol . value) pairs
(≔ first-def (◁ defs))
(⊨ :first-is-pair #t (⟨⟩? first-def))

; ============================================================================
; PHASE 1C: Module has exports field
; ============================================================================

; Test 7: Module has :exports field
(⊨ :has-exports-field #t (⊙? mod :exports))

; If no explicit exports, should default to all defs
(≔ exports (⊙→ mod :exports))

; Test 8: Exports is a list
(⊨ :exports-is-list #t (⟨⟩? exports))

; ============================================================================
; Summary: 8 basic tests
; ============================================================================

(≋ "✅ Phase 1 complete: Module as value structure!")
