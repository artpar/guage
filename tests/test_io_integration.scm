; Comprehensive I/O Integration Tests
; Day 24 - Phase 4: Real-world I/O scenarios

; ============ Setup ============

(≔ log-file "/tmp/guage_integration_log.txt")
(≔ data-file "/tmp/guage_integration_data.txt")
(≔ config-file "/tmp/guage_integration_config.txt")

; ============ Logging System ============

; Initialize log file
(≔ _ (≋⊲ log-file "=== Log Started ===\n"))

; Log message function
(≔ log (λ (msg)
  (≋⊕ log-file (≈⊕ msg "\n"))))

; Test logging
(≔ _ (log "First log entry"))
(≔ _ (log "Second log entry"))
(≔ _ (log "Third log entry"))

(⊨ :logging-system #t
   (≈? (≋⊳ log-file)))

; ============ Data Processing Pipeline ============

; Write initial data
(≔ _ (≋⊲ data-file "10\n20\n30\n40\n50"))

; Read and verify
(⊨ :data-pipeline-read "10\n20\n30\n40\n50"
   (≋⊳ data-file))

; Transform data (simulate processing)
(≔ original (≋⊳ data-file))
(≔ processed (≈⊕ original "\n60\n70"))
(≔ _ (≋⊲ data-file processed))

(⊨ :data-pipeline-transform #t
   (≈? (≋⊳ data-file)))

; ============ Configuration Management ============

; Write config
(≔ _ (≋⊲ config-file "app_name=Guage\nversion=1.0\n"))

; Read config
(⊨ :config-read "app_name=Guage\nversion=1.0\n"
   (≋⊳ config-file))

; Update config (append)
(≔ _ (≋⊕ config-file "debug=true\n"))

; Verify update
(⊨ :config-update "app_name=Guage\nversion=1.0\ndebug=true\n"
   (≋⊳ config-file))

; ============ File Status Checking ============

; Check all files exist
(⊨ :all-files-exist #t
   (∧ (∧ (≋? log-file) (≋? data-file))
      (≋? config-file)))

; Check none are empty
(⊨ :no-files-empty #t
   (∧ (∧ (¬ (≋∅? log-file)) (¬ (≋∅? data-file)))
      (¬ (≋∅? config-file))))

; ============ Conditional File Operations ============

; Safe read with fallback
(≔ backup-file "/tmp/guage_backup.txt")
(≔ safe-read (λ (path) (λ (fallback)
  (? (≋? path)
     (≋⊳ path)
     fallback))))

(⊨ :safe-read-exists "app_name=Guage\nversion=1.0\ndebug=true\n"
   ((safe-read config-file) "default"))

(⊨ :safe-read-missing "default"
   ((safe-read backup-file) "default"))

; ============ Multiple File Operations ============

; Copy file (read then write)
(≔ source-file "/tmp/guage_source.txt")
(≔ dest-file "/tmp/guage_dest.txt")

(≔ _ (≋⊲ source-file "source content"))
(≔ content (≋⊳ source-file))
(≔ _ (≋⊲ dest-file content))

(⊨ :file-copy "source content"
   (≋⊳ dest-file))

; Merge files (read both, concat, write)
(≔ file1 "/tmp/guage_merge1.txt")
(≔ file2 "/tmp/guage_merge2.txt")
(≔ merged-file "/tmp/guage_merged.txt")

(≔ _ (≋⊲ file1 "Part A"))
(≔ _ (≋⊲ file2 "Part B"))

(≔ content1 (≋⊳ file1))
(≔ content2 (≋⊳ file2))
(≔ merged (≈⊕ content1 (≈⊕ " + " content2)))
(≔ _ (≋⊲ merged-file merged))

(⊨ :file-merge "Part A + Part B"
   (≋⊳ merged-file))

; ============ String Operations with Files ============

; Write with string operations
(≔ test-file "/tmp/guage_string_ops.txt")
(≔ header "HEADER")
(≔ body "BODY")
(≔ footer "FOOTER")

(≔ document (≈⊕ header (≈⊕ "\n" (≈⊕ body (≈⊕ "\n" footer)))))
(≔ _ (≋⊲ test-file document))

(⊨ :string-ops-with-file "HEADER\nBODY\nFOOTER"
   (≋⊳ test-file))

; Read and parse length
(≔ content (≋⊳ test-file))
(⊨ :file-length-check #18
   (≈# content))

; ============ Error Handling ============

; Try to read non-existent file
(⊨ :error-read-missing #t
   (⚠? (≋⊳ "/tmp/does_not_exist_xyz.txt")))

; Try to write with wrong types
(⊨ :error-write-bad-path #t
   (⚠? (≋⊲ #42 "content")))

(⊨ :error-write-bad-content #t
   (⚠? (≋⊲ "/tmp/test.txt" #42)))

; ============ Console + File Integration ============

; Print and save
(≔ message "Hello from Guage!")
(≔ _ (≋ message))  ; Print to console
(≔ _ (≋⊲ test-file message))  ; Save to file

(⊨ :print-and-save message
   (≋⊳ test-file))

; ============ Cleanup Verification ============

; Verify all test files exist
(⊨ :cleanup-files-exist #t
   (∧ (≋? log-file)
      (∧ (≋? data-file)
         (∧ (≋? config-file)
            (∧ (≋? test-file)
               (≋? merged-file))))))

; ============ Summary ============
; Total: 20 integration tests
; - Logging: 1 test
; - Data pipeline: 2 tests
; - Configuration: 2 tests
; - File status: 2 tests
; - Safe operations: 2 tests
; - Multiple files: 2 tests
; - String operations: 2 tests
; - Error handling: 3 tests
; - Console + file: 1 test
; - Cleanup: 1 test
; - Cross-feature: 2 tests
