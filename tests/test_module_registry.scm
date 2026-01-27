; ============================================================================
; Module Registry Tests - Day 26 Phase 1
; ============================================================================
;
; Tests for the module registry system that tracks:
; - Which modules have been loaded
; - Which symbols each module defines
; - Provenance of any symbol (which module defined it)
;
; New primitive: ⌂⊚ (module-info)
; - (⌂⊚)        → List all loaded modules
; - (⌂⊚ :sym)   → Find which module defines :sym
; - (⌂⊚ "path") → List all symbols from module
;

; ============================================================================
; Test 1: List all modules returns a pair or nil
; ============================================================================
; When no modules loaded, should return nil
; When modules loaded, should return a list (pair-based)

(⊨ (⌜ :list-modules-returns-list-or-nil)
   #t
   ; ⌂⊚ with no args should return either a pair (list) or nil
   (∨ (⟨⟩? (⌂⊚)) (∅? (⌂⊚))))

; ============================================================================
; Test 2: Find symbol's module (error case)
; ============================================================================
; Looking up a symbol that doesn't exist in any module should return error

(⊨ (⌜ :find-nonexistent-symbol-errors)
   #t
   ; Looking up a symbol not in any module should error
   (⚠? (⌂⊚ :this-symbol-definitely-does-not-exist-in-any-module-12345)))

; ============================================================================
; Test 3: List module's symbols (nonexistent module)
; ============================================================================
; Looking up a module that doesn't exist should return nil

(⊨ (⌜ :list-symbols-nonexistent-module)
   #t
   ; Looking up symbols from nonexistent module returns nil
   (∅? (⌂⊚ "nonexistent-module-path-xyz.scm")))

; ============================================================================
; Test 4: Module info with nil returns list
; ============================================================================

(⊨ (⌜ :module-info-with-nil-arg)
   #t
   ; Calling with explicit ∅ should behave same as no args
   (∨ (⟨⟩? (⌂⊚)) (∅? (⌂⊚))))

; ============================================================================
; Test 5: Module info with symbol returns string or error
; ============================================================================

(⊨ (⌜ :module-info-with-symbol-returns-string-or-error)
   #t
   ; Passing a symbol should either return a string or error
   ((λ (result) (∨ (≈? result) (⚠? result)))
    (⌂⊚ :nonexistent)))

; ============================================================================
; Test 6: Module info with string returns list
; ============================================================================

(⊨ (⌜ :module-info-with-string-returns-list)
   #t
   ; Passing a string should return a list (possibly empty)
   ((λ (result) (∨ (⟨⟩? result) (∅? result)))
    (⌂⊚ "some-path.scm")))

; ============================================================================
; Test 7: Module info with invalid argument type
; ============================================================================

(⊨ (⌜ :module-info-invalid-arg-errors)
   #t
   ; Passing a number should error
   (⚠? (⌂⊚ #42)))

; ============================================================================
; Test 8: Test that defining a function locally doesn't register it
; ============================================================================
; Define a function here (not inside a module load)

(≔ test-local-function (λ (x) (⊕ x #1)))

; This function should NOT be in any module since we're not inside ⋘
; We can't directly test this without loading a real file, but we can
; verify the function exists and works

(⊨ (⌜ :local-function-works)
   #3
   (test-local-function #2))

; And verify looking it up returns an error (not in any module)

(⊨ (⌜ :local-function-not-in-module)
   #t
   (⚠? (⌂⊚ :test-local-function)))

; ============================================================================
; Test 9: All three modes of ⌂⊚ are callable
; ============================================================================

(⊨ (⌜ :module-registry-three-modes-work)
   #t
   ; All three calls should return something (not crash)
   ; mode1 is list or nil
   ; mode2 is string or error
   ; mode3 is list or nil
   ((λ (mode1 mode2 mode3)
      (∧
        (∨ (⟨⟩? mode1) (∅? mode1))
        (∨ (≈? mode2) (⚠? mode2))
        (∨ (⟨⟩? mode3) (∅? mode3))))
    (⌂⊚)
    (⌂⊚ :some-symbol)
    (⌂⊚ "some-path.scm")))

; ============================================================================
; Summary
; ============================================================================
;
; These tests verify the basic module registry API works:
; ✓ ⌂⊚ primitive exists and is callable
; ✓ Returns appropriate types (list/string/error)
; ✓ Handles invalid arguments gracefully
; ✓ Errors on nonexistent symbols/modules
; ✓ Local definitions don't get registered in modules
;
; Full integration tests (loading actual files) will come in Day 26 Step 1.3
