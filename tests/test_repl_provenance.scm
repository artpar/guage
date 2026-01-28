; Test ⌂⊛ provenance for REPL-defined functions
; This tests the fix for Day 43 where ⌂⊛ was broken for REPL definitions

; Define a simple function in "REPL" (simulated by loading this file)
(≔ square (λ (x) (⊗ x x)))

; Test 1: ⌂⊛ should work (not return error)
(⊢ (¬ (⚠? (⌂⊛ :square))) :test-provenance-not-error)

; Test 2: Provenance should have :module field
(≔ prov (⌂⊛ :square))
(⊢ (⊙? prov :Provenance) :test-provenance-is-struct)

; Test 3: Module field should exist
(≔ module-name (⊙→ prov :module))
(⊢ (¬ (⚠? module-name)) :test-module-field-exists)

; Test 4: For module-loaded symbols, should show the module
; (For REPL, might show "<repl>" or the file path)
(⊢ (≢ module-name ∅) :test-module-not-nil)

; Test 5: Should have load-order field
(≔ load-order (⊙→ prov :load-order))
(⊢ (¬ (⚠? load-order)) :test-load-order-exists)

; Test 6: Should have defined-at timestamp
(≔ defined-at (⊙→ prov :defined-at))
(⊢ (¬ (⚠? defined-at)) :test-timestamp-exists)

; Test 7: Primitive should still work
(≔ prim-prov (⌂⊛ :⊕))
(⊢ (≡ (⊙→ prim-prov :module) "<primitive>") :test-primitive-module)

; Test 8: Undefined symbol should still error
(⊢ (⚠? (⌂⊛ :nonexistent)) :test-undefined-symbol-error)
