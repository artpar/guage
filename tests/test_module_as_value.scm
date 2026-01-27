; test_module_as_value.scm - first module system tests
; Tests modules as first-class queryable values

; ============================================================================
; PHASE 1: Module as Value Structure
; ============================================================================

; Test: Load returns module structure (not evaluation result)
(≔ math-str "(≔ double (λ (n) (⊗ n #2)))\n(≔ triple (λ (n) (⊗ n #3)))")
(≋⊲ "test_math_module.scm" math-str)
(≔ math-mod (⋘ "test_math_module.scm"))

; Module should be a structure
(⊨ :module-is-struct #t (⊙? math-mod))
(⊨ :module-has-path #t (⊙? math-mod :path))
(⊨ :module-has-defs #t (⊙? math-mod :defs))
(⊨ :module-has-exports #t (⊙? math-mod :exports))

; Path should be correct
(⊨ :module-path-correct "test_math_module.scm" (⊙→ math-mod :path))

; Defs should be a list of all definitions
(≔ defs (⊙→ math-mod :defs))
(⊨ :defs-is-list #t (⟨⟩? defs))
(⊨ :defs-has-double #t (∈ defs (⌜ double)))
(⊨ :defs-has-triple #t (∈ defs (⌜ triple)))

; ============================================================================
; PHASE 2: Explicit Exports
; ============================================================================

; Test: Module can declare exports
(≔ lib-str "(≔ helper (λ (x) (⊕ x #1)))\n(≔ public (λ (x) (helper x)))\n(⊙◇ :exports (⌜ [public]))")
(≋⊲ "test_lib_module.scm" lib-str)
(≔ lib-mod (⋘ "test_lib_module.scm"))

; Exports should only include public
(≔ exports (⊙→ lib-mod :exports))
(⊨ :exports-is-list #t (⟨⟩? exports))
(⊨ :exports-has-public #t (∈ exports (⌜ public)))
(⊨ :exports-no-helper #f (∈ exports (⌜ helper)))

; But helper should still be in defs (visible for AI!)
(≔ lib-defs (⊙→ lib-mod :defs))
(⊨ :defs-has-helper #t (∈ lib-defs (⌜ helper)))

; ============================================================================
; PHASE 3: Import Primitive
; ============================================================================

; Test: Import brings exports into scope
; Before import, double should not be defined
(⊨ :double-not-defined-yet #t (⚠? (double #5)))

; Import module
(⋖ math-mod)

; After import, double should work
(⊨ :double-works #10 (double #5))
(⊨ :triple-works #15 (triple #5))

; ============================================================================
; PHASE 4: Selective Import
; ============================================================================

; Test: Can import only specific symbols
(≔ sel-str "(≔ foo (λ (x) x))\n(≔ bar (λ (x) (⊕ x #1)))\n(≔ baz (λ (x) (⊗ x #2)))")
(≋⊲ "test_sel_module.scm" sel-str)
(≔ sel-mod (⋘ "test_sel_module.scm"))

; Import only foo and bar
(⋖ sel-mod (⌜ [foo bar]))

; foo and bar should work
(⊨ :foo-imported #42 (foo #42))
(⊨ :bar-imported #43 (bar #42))

; baz should not be imported (but still in module!)
(⊨ :baz-not-imported #t (⚠? (baz #42)))
(⊨ :baz-in-module #t (∈ (⊙→ sel-mod :defs) (⌜ baz)))

; ============================================================================
; PHASE 5: Module Registry
; ============================================================================

; Test: All loaded modules are in registry
(≔ all-mods (⌂⊚))
(⊨ :registry-is-list #t (⟨⟩? all-mods))

; Can get module by path
(≔ retrieved (⌂⊚ "test_math_module.scm"))
(⊨ :retrieved-is-struct #t (⊙? retrieved))
(⊨ :retrieved-same-path "test_math_module.scm" (⊙→ retrieved :path))

; ============================================================================
; PHASE 6: Provenance Tracking
; ============================================================================

; Test: Can query where symbol came from
(≔ double-source (⌂⊛ (⌜ double)))
(⊨ :double-from-math "test_math_module.scm" double-source)

(≔ foo-source (⌂⊛ (⌜ foo)))
(⊨ :foo-from-sel "test_sel_module.scm" foo-source)

; ============================================================================
; PHASE 7: Dependencies (Future)
; ============================================================================

; Test: Module can depend on other modules
; base.scm defines constant
(≔ base-str "(≔ MULTIPLIER #10)")
(≋⊲ "test_base.scm" base-str)

; derived.scm uses base
(≔ derived-str "(⋘ \"test_base.scm\")\n(≔ scale (λ (x) (⊗ x MULTIPLIER)))")
(≋⊲ "test_derived.scm" derived-str)

; Load derived (should auto-load base)
(≔ derived-mod (⋘ "test_derived.scm"))
(≔ derived-deps (⊙→ derived-mod :deps))

; Deps should include base
(⊨ :derived-deps-base #t (∈ derived-deps "test_base.scm"))

; ============================================================================
; PHASE 8: No Information Hiding (Core Principle!)
; ============================================================================

; Test: ALL code is visible, even "internal" helpers
(≔ full-str "(≔ internal-helper (λ (x) (⊕ x #1)))\n(≔ another-helper (λ (x) (⊗ x #2)))\n(≔ public (λ (x) (another-helper (internal-helper x))))\n(⊙◇ :exports (⌜ [public]))")
(≋⊲ "test_full_module.scm" full-str)
(≔ full-mod (⋘ "test_full_module.scm"))

; Exports should only have public
(≔ full-exports (⊙→ full-mod :exports))
(⊨ :only-public-exported #1 (# full-exports))

; But ALL definitions visible in defs (for AI!)
(≔ full-defs (⊙→ full-mod :defs))
(⊨ :all-defs-visible #3 (# full-defs))
(⊨ :internal-visible #t (∈ full-defs (⌜ internal-helper)))
(⊨ :another-visible #t (∈ full-defs (⌜ another-helper)))

; AI can query internal structure!
; This is what makes Guage different from traditional languages!

; ============================================================================
; Summary: 27 tests for first module system
; ============================================================================

(≋ "✅ All first module tests complete!")
(≋ "   - Modules as first-class values")
(≋ "   - Explicit exports (but everything queryable!)")
(≋ "   - Import primitive")
(≋ "   - Module registry")
(≋ "   - Provenance tracking")
(≋ "   - NO information hiding (AI can see ALL code)")
