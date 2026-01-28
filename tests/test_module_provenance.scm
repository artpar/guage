; Test provenance for module-loaded vs REPL-defined functions

; Load a module
(⋘ "tests/fixtures/provenance_test.scm")

; Test 1: Module-loaded function should have module path
(≔ mod-prov (⌂⊛ :cube))
(⊢ (⊙? mod-prov :Provenance) :test-module-prov-struct)

; Test 2: Module path should be the file path, not <repl>
(≔ mod-path (⊙→ mod-prov :module))
(⊢ (≢ mod-path "<repl>") :test-module-not-repl)
(⊢ (≢ mod-path "<primitive>") :test-module-not-primitive)

; Test 3: Define a function in REPL (after loading module)
(≔ repl-fn (λ (x) (⊕ x #1)))

; Test 4: REPL-defined function should show <repl>
(≔ repl-prov (⌂⊛ :repl-fn))
(⊢ (≡ (⊙→ repl-prov :module) "<repl>") :test-repl-shows-repl-module)

; Test 5: Both should have different load orders
; Note: <repl> module is initialized first (order #1)
; File modules loaded later get higher order numbers
(≔ mod-order (⊙→ mod-prov :load-order))
(≔ repl-order (⊙→ repl-prov :load-order))
(⊢ (> mod-order repl-order) :test-module-loaded-after-repl)

; Test 6: Module-loaded function from another module
(≔ add2-prov (⌂⊛ :add2))
(⊢ (≡ (⊙→ add2-prov :module) (⊙→ mod-prov :module)) :test-same-module-functions)
