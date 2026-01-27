; Test File Predicate Primitives (≋?, ≋∅?)
; Day 24 - Phase 3: File Predicates

; ============ Setup ============

(≔ test-file "/tmp/guage_test_predicates.txt")
(≔ non-existent "/tmp/guage_nonexistent_file.txt")

; ============ ≋? (file-exists?) Tests ============

; Test file exists - initially should not exist
(⊨ :file-exists-initially-false #f
   (≋? non-existent))

; Test file exists - after creating
(≔ _ (≋⊲ test-file "content"))
(⊨ :file-exists-after-create #t
   (≋? test-file))

; Test file exists - still exists after read
(≔ _ (≋⊳ test-file))
(⊨ :file-exists-after-read #t
   (≋? test-file))

; Test file exists - still exists after write
(≔ _ (≋⊲ test-file "new content"))
(⊨ :file-exists-after-write #t
   (≋? test-file))

; Test file exists - still exists after append
(≔ _ (≋⊕ test-file " more"))
(⊨ :file-exists-after-append #t
   (≋? test-file))

; Test file exists - non-string returns false
(⊨ :file-exists-non-string #f
   (≋? #42))

; Test file exists - symbol returns false
(⊨ :file-exists-symbol #f
   (≋? :test))

; ============ ≋∅? (file-empty?) Tests ============

; Test file empty - non-existent returns false
(⊨ :file-empty-nonexistent #f
   (≋∅? non-existent))

; Test file empty - create empty file
(≔ _ (≋⊲ test-file ""))
(⊨ :file-empty-true #t
   (≋∅? test-file))

; Test file empty - after adding content
(≔ _ (≋⊲ test-file "content"))
(⊨ :file-empty-false #f
   (≋∅? test-file))

; Test file empty - after append
(≔ _ (≋⊕ test-file " more"))
(⊨ :file-empty-after-append #f
   (≋∅? test-file))

; Test file empty - overwrite with empty
(≔ _ (≋⊲ test-file ""))
(⊨ :file-empty-overwrite-empty #t
   (≋∅? test-file))

; Test file empty - non-string returns false
(⊨ :file-empty-non-string #f
   (≋∅? #42))

; Test file empty - symbol returns false
(⊨ :file-empty-symbol #f
   (≋∅? :test))

; ============ Integration Tests ============

; Test exists and empty together - empty file
(≔ _ (≋⊲ test-file ""))
(⊨ :exists-and-empty-true #t
   (∧ (≋? test-file) (≋∅? test-file)))

; Test exists and empty together - non-empty file
(≔ _ (≋⊲ test-file "data"))
(⊨ :exists-but-not-empty #t
   (∧ (≋? test-file) (¬ (≋∅? test-file))))

; Test exists and empty together - non-existent file
(⊨ :not-exists-not-empty #t
   (∧ (¬ (≋? non-existent)) (¬ (≋∅? non-existent))))

; Test conditional file operations based on exists
(⊨ :conditional-on-exists "exists"
   (? (≋? test-file)
      "exists"
      "not found"))

; Test conditional based on empty
(≔ _ (≋⊲ test-file ""))
(≔ _ (≋⊕ test-file "added"))
(⊨ :conditional-on-empty "added"
   (? (≋∅? "/tmp/fake.txt")
      "was empty"
      (≋⊳ test-file)))

; Test safe read with exists check
(≔ _ (≋⊲ test-file "safe content"))
(⊨ :safe-read-with-check "safe content"
   (? (≋? test-file)
      (≋⊳ test-file)
      "file not found"))

; ============ Summary ============
; Total: 20 tests
; - ≋? (file-exists): 7 tests
; - ≋∅? (file-empty): 7 tests
; - Integration: 6 tests
