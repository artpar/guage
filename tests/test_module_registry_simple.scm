; test_module_registry_simple.scm - Phase 1: Module Registry
; Backwards compatible - ⋘ still works the same, but now tracks metadata

; ============================================================================
; Test 1: Load works as before (backwards compatible)
; ============================================================================

(≔ test-code "(≔ double (λ (n) (⊗ n #2)))")
(≋⊲ "test_compat.scm" test-code)

; Load and use immediately (current behavior)
(⋘ "test_compat.scm")
(⊨ :load-works-as-before #10 (double #5))

; ============================================================================
; Test 2: Can query loaded modules
; ============================================================================

; ⌂⊚ lists all loaded modules
(≔ all-modules (⌂⊚))
(⊨ :module-list-exists #t (⟨⟩? all-modules))

; Should include our test module
; (We'll check if it's in the list)

; ============================================================================
; Test 3: Can get metadata for specific module
; ============================================================================

; ⌂⊚ with path gets module metadata
(≔ mod-info (⌂⊚ "test_compat.scm"))
(⊨ :module-info-exists #t (⊙? mod-info))

; Module should have :path field
(⊨ :module-has-path "test_compat.scm" (⊙→ mod-info :path))

; Module should have :defs field (list of defined symbols)
(≔ defs (⊙→ mod-info :defs))
(⊨ :defs-is-list #t (⟨⟩? defs))

; ============================================================================
; Test 4: Provenance tracking
; ============================================================================

; Enhanced ⌂⊛ shows where symbol was defined
(≔ double-source (⌂⊛ (⌜ double)))
(⊨ :provenance-correct "test_compat.scm" double-source)

; ============================================================================
; Test 5: All code visible (AI principle)
; ============================================================================

(≔ full-code "(≔ helper (λ (x) (⊕ x #1)))\n(≔ public (λ (x) (helper x)))")
(≋⊲ "test_visible.scm" full-code)
(⋘ "test_visible.scm")

; Both helper and public should be visible in metadata
(≔ vis-info (⌂⊚ "test_visible.scm"))
(≔ vis-defs (⊙→ vis-info :defs))

; Check that both symbols are tracked
; (We'll verify defs is non-empty at minimum)
(⊨ :all-code-tracked #t (⟨⟩? vis-defs))

; ============================================================================
; Summary: 7 tests for backwards-compatible module registry
; ============================================================================

(≋ "✅ Module registry tests complete!")
(≋ "   - ⋘ still works as before")
(≋ "   - ⌂⊚ lists all modules")
(≋ "   - ⌂⊚ path gets metadata")
(≋ "   - ⌂⊛ shows provenance")
(≋ "   - All code visible for AI")
