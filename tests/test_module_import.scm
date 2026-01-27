;;;; Day 28: Selective Import (⋖) Tests
;;;; Validates that symbols exist in specified modules

;; Test module path
(≔ test-module-path "../../tests/fixtures/test_import.scm")

;; Test 1: Validate single symbol import
;; When module defines symbol, validation succeeds
(⊢ (≡ (⋖ test-module-path (⟨⟩ :add ∅)) :ok)
   "Single symbol import validation should succeed")

;; Test 2: Validate multiple symbols import
;; When module defines all requested symbols, validation succeeds
(⊢ (≡ (⋖ test-module-path (⟨⟩ :add (⟨⟩ :multiply ∅))) :ok)
   "Multiple symbols import validation should succeed")

;; Test 3: Error when symbol not in module
;; When requested symbol doesn't exist in module, return error
(⊢ (⚠? (⋖ test-module-path (⟨⟩ :nonexistent ∅)))
   "Import of nonexistent symbol should return error")

;; Test 4: Error when module not loaded
;; When module hasn't been loaded yet, return error
(⊢ (⚠? (⋖ "../../tests/fixtures/never_loaded.scm" (⟨⟩ :foo ∅)))
   "Import from unloaded module should return error")

;; Test 5: Error on malformed arguments
;; Missing symbol list argument should return error
(⊢ (⚠? (⋖ test-module-path))
   "Import with missing symbol list should return error")

;; Test 6: Error when module path is not a string
;; First argument must be string (module path)
(⊢ (⚠? (⋖ #42 (⟨⟩ :foo ∅)))
   "Import with non-string module path should return error")

;; Test 7: Error when symbol list is not a list
;; Second argument must be list of symbols
(⊢ (⚠? (⋖ test-module-path #42))
   "Import with non-list symbols should return error")

;; Test 8: Error when symbol list contains non-symbols
;; All elements in symbol list must be symbols
(⊢ (⚠? (⋖ test-module-path (⟨⟩ :add (⟨⟩ #42 (⟨⟩ :multiply ∅)))))
   "Import with non-symbol in list should return error")

;; Test 9: Empty symbol list is valid
;; Importing no symbols should succeed (vacuous truth)
(⊢ (≡ (⋖ test-module-path ∅) :ok)
   "Import with empty symbol list should succeed")

;; Test 10: All defined symbols can be imported
;; Verify all three symbols from fixture module
(⊢ (≡ (⋖ test-module-path (⟨⟩ :add (⟨⟩ :multiply (⟨⟩ :subtract ∅)))) :ok)
   "All fixture symbols should be importable")
