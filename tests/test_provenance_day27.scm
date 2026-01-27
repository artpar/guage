; ============================================================================
; Day 27 - Enhanced Provenance Tracking Tests
; ============================================================================
;
; Tests for the enhanced module provenance system:
; - Load order tracking (modules assigned sequential numbers)
; - Line number tracking (in lambda cells, currently 0)
; - Enhanced ⌂⊛ primitive (returns full metadata structure)
;

; ============================================================================
; Test 1: Provenance for primitives
; ============================================================================

; Primitives should return simple provenance with just module="<primitive>"
(≔ prim-prov (⌂⊛ :⊕))

(⊨ (⌜ :primitive-provenance-is-struct)
   #t
   (⊙? prim-prov :Provenance))

(⊨ (⌜ :primitive-module-is-primitive)
   #t
   (≈? (⊙→ prim-prov :module)))

; ============================================================================
; Test 2: Load first module and check load order
; ============================================================================

; Load test_module_math.scm
(≔ math-load-result (⋘ "../../tests/test_module_math.scm"))

; Verify load succeeded
(⊨ (⌜ :math-module-loaded)
   #t
   (¬ (⚠? math-load-result)))

; Get provenance for square
(≔ square-prov (⌂⊛ :square))

; Should be a provenance structure
(⊨ (⌜ :square-provenance-is-struct)
   #t
   (⊙? square-prov :Provenance))

; Should have module field
(⊨ (⌜ :square-has-module)
   #t
   (≈? (⊙→ square-prov :module)))

; Should have load order = 1 (first module)
(⊨ (⌜ :square-load-order-is-1)
   #1
   (⊙→ square-prov :load-order))

; Should have line number (currently 0)
(⊨ (⌜ :square-has-line-number)
   #0
   (⊙→ square-prov :line))

; Should have timestamp (defined-at)
(⊨ (⌜ :square-has-timestamp)
   #t
   (ℕ? (⊙→ square-prov :defined-at)))

; ============================================================================
; Test 3: Symbols not from modules return error
; ============================================================================

; Define a simple function in-line (not from a module)
(≔ test-fn1 (λ (x) (⊕ x #1)))

; Get provenance - should return error (not in module registry)
(≔ fn1-prov (⌂⊛ :test-fn1))

; Should be an error (symbol not tracked in module registry)
(⊨ (⌜ :inline-symbol-not-tracked)
   #t
   (⚠? fn1-prov))

; ============================================================================
; Test 4: Module list includes loaded modules
; ============================================================================

; Get all loaded modules
(≔ all-modules (⌂⊚))

; Should be a list
(⊨ (⌜ :modules-is-list)
   #t
   (∨ (⟨⟩? all-modules) (∅? all-modules)))

; Should not be empty (at least test_module_math.scm)
(⊨ (⌜ :modules-not-empty)
   #t
   (⟨⟩? all-modules))

; ============================================================================
; Test 5: Find symbol's module
; ============================================================================

; square should be in test_module_math.scm
(≔ square-module (⌂⊚ :square))

(⊨ (⌜ :square-module-found)
   #t
   (≈? square-module))

; ============================================================================
; Test 6: Verify all functions from test_module_math have load-order 1
; ============================================================================

(≔ cube-prov (⌂⊛ :cube))
(≔ add-one-prov (⌂⊛ :add-one))
(≔ double-prov (⌂⊛ :double))

; All should have load-order 1 (same module)
(⊨ (⌜ :cube-load-order-is-1)
   #1
   (⊙→ cube-prov :load-order))

(⊨ (⌜ :add-one-load-order-is-1)
   #1
   (⊙→ add-one-prov :load-order))

(⊨ (⌜ :double-load-order-is-1)
   #1
   (⊙→ double-prov :load-order))

; ============================================================================
; Test 7: All have valid timestamps
; ============================================================================

; Re-fetch provenances to ensure fresh data
(≔ square-prov-2 (⌂⊛ :square))
(≔ cube-prov-2 (⌂⊛ :cube))
(≔ add-one-prov-2 (⌂⊛ :add-one))
(≔ double-prov-2 (⌂⊛ :double))

(≔ square-time (⊙→ square-prov-2 :defined-at))
(≔ cube-time (⊙→ cube-prov-2 :defined-at))
(≔ add-one-time (⊙→ add-one-prov-2 :defined-at))
(≔ double-time (⊙→ double-prov-2 :defined-at))

; All should have valid timestamps (positive numbers)
(⊨ (⌜ :square-timestamp-valid)
   #t
   (> square-time #0))

(⊨ (⌜ :cube-timestamp-valid)
   #t
   (> cube-time #0))

(⊨ (⌜ :add-one-timestamp-valid)
   #t
   (> add-one-time #0))

(⊨ (⌜ :double-timestamp-valid)
   #t
   (> double-time #0))

; All should have same timestamp (loaded together)
(⊨ (⌜ :timestamps-match)
   #t
   (∧ (≡ square-time cube-time)
      (∧ (≡ cube-time add-one-time)
         (≡ add-one-time double-time))))

; ============================================================================
; Test 8: Provenance for non-existent symbol returns error
; ============================================================================

(≔ bad-prov (⌂⊛ :nonexistent-symbol-xyz))

(⊨ (⌜ :nonexistent-symbol-error)
   #t
   (⚠? bad-prov))

; ============================================================================
; Summary
; ============================================================================
;
; Day 27 Enhanced Provenance Tests verify:
; ✓ Primitives return simple provenance
; ✓ User functions return full provenance with 4 fields
; ✓ Load order tracking works (sequential numbers)
; ✓ Line number field exists (currently 0)
; ✓ Timestamps track when modules loaded
; ✓ Module registry integration works
; ✓ Error handling for non-existent symbols
;
