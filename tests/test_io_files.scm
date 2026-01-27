; Test File I/O Primitives (≋⊳, ≋⊲, ≋⊕)
; Day 24 - Phase 2: File Operations

; ============ Setup: Create Test Files ============

; Write initial test file
(≔ test-file "/tmp/guage_test_io.txt")
(≔ write-result (≋⊲ test-file "initial content"))

; ============ ≋⊳ (read-file) Tests ============

; Test read file - basic
(⊨ :read-file-basic "initial content"
   (≋⊳ test-file))

; Test read file returns string
(⊨ :read-file-is-string #t
   (≈? (≋⊳ test-file)))

; Test read file - non-existent file (error)
(⊨ :read-file-not-found #t
   (⚠? (≋⊳ "/tmp/does_not_exist_guage_test.txt")))

; Test read file - error on non-string path
(⊨ :read-file-error-number #t
   (⚠? (≋⊳ #42)))

; ============ ≋⊲ (write-file) Tests ============

; Test write file - overwrite
(≔ _ (≋⊲ test-file "overwritten"))
(⊨ :write-file-overwrite "overwritten"
   (≋⊳ test-file))

; Test write file returns path
(⊨ :write-file-returns-path test-file
   (≋⊲ test-file "content"))

; Test write file - empty string
(≔ _ (≋⊲ test-file ""))
(⊨ :write-file-empty ""
   (≋⊳ test-file))

; Test write file - newlines
(≔ _ (≋⊲ test-file "line1\nline2\nline3"))
(⊨ :write-file-newlines "line1\nline2\nline3"
   (≋⊳ test-file))

; Test write file - error on non-string path
(⊨ :write-file-error-path #t
   (⚠? (≋⊲ #42 "content")))

; Test write file - error on non-string content
(⊨ :write-file-error-content #t
   (⚠? (≋⊲ test-file #42)))

; ============ ≋⊕ (append-file) Tests ============

; Setup for append tests
(≔ _ (≋⊲ test-file "start"))

; Test append file - basic
(≔ _ (≋⊕ test-file " middle"))
(≔ _ (≋⊕ test-file " end"))
(⊨ :append-file-basic "start middle end"
   (≋⊳ test-file))

; Test append file returns path
(⊨ :append-file-returns-path test-file
   (≋⊕ test-file "more"))

; Test append file - empty string
(≔ _ (≋⊲ test-file "base"))
(≔ _ (≋⊕ test-file ""))
(⊨ :append-file-empty "base"
   (≋⊳ test-file))

; Test append file - newlines
(≔ _ (≋⊲ test-file "line1\n"))
(≔ _ (≋⊕ test-file "line2\n"))
(⊨ :append-file-newlines "line1\nline2\n"
   (≋⊳ test-file))

; Test append file - error on non-string path
(⊨ :append-file-error-path #t
   (⚠? (≋⊕ #42 "content")))

; Test append file - error on non-string content
(⊨ :append-file-error-content #t
   (⚠? (≋⊕ test-file :symbol)))

; ============ Integration Tests ============

; Test write-read cycle
(≔ test-data "test data 123")
(≔ _ (≋⊲ test-file test-data))
(⊨ :write-read-cycle test-data
   (≋⊳ test-file))

; Test multiple writes
(≔ _ (≋⊲ test-file "v1"))
(≔ _ (≋⊲ test-file "v2"))
(≔ _ (≋⊲ test-file "v3"))
(⊨ :multiple-writes "v3"
   (≋⊳ test-file))

; Test append after write
(≔ _ (≋⊲ test-file "A"))
(≔ _ (≋⊕ test-file "B"))
(≔ _ (≋⊕ test-file "C"))
(⊨ :append-after-write "ABC"
   (≋⊳ test-file))

; Test string operations with file content
(≔ _ (≋⊲ test-file "hello"))
(⊨ :string-ops-with-file #5
   (≈# (≋⊳ test-file)))

; Test file content manipulation
(≔ original "lowercase")
(≔ _ (≋⊲ test-file original))
(⊨ :file-manipulation original
   (≋⊳ test-file))

; ============ Cleanup ============

; Note: Test file remains at /tmp/guage_test_io.txt
; Manual cleanup may be needed

; ============ Summary ============
; Total: 25 tests
; - ≋⊳ (read-file): 4 tests
; - ≋⊲ (write-file): 6 tests
; - ≋⊕ (append-file): 6 tests
; - Integration: 5 tests
; - Error handling: 4 tests
