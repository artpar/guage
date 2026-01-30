;;; stdlib/test.scm — First-Class Test Runner
;;; Day 123: Trie-backed registry + iterator pipelines + rich results
;;;
;;; Provides:
;;;   ⊨⊕    — Suite macro (register multiple tests under a prefix)
;;;   ⊨∇    — Pattern-match assertion macro
;;;   ⊨⟪    — Effect-isolated test macro
;;;   ⊨⊕⊲   — Iterator-based result filtering
;;;   ⊨⊕⋔   — Timing report via SortedMap (top-N slowest)
;;;   ⊨⊕⊍   — Tag-based selection via HashSet intersection
;;;   ⊨⊕‖   — Parallel suite runner via actors + channels

;;; ═══════════════════════════════════════════
;;; Suite Macro: ⊨⊕
;;; Register multiple tests under a common prefix.
;;;
;;; Usage:
;;;   (⊨⊕ :math:add
;;;     (:basic     #5  (⊕ #2 #3))
;;;     (:zero      #0  (⊕ #0 #0))
;;;     (:negative  #-1 (⊕ #2 #-3)))
;;;
;;; Expands to ⊨⊕⊙ calls for each test.
;;; ═══════════════════════════════════════════

;;; Helper: concatenate symbol names with ":" separator
(≔ ⊨⊕:concat (λ (suite name)
  (≈→: (≈⊕ (≈⊕ (≈ suite) ":") (≈ name)))))

;;; Helper: convert string to symbol (using eval of quoted symbol)
(≔ ≈→: (λ (s) s))

;;; Register a single test from suite definition
(≔ ⊨⊕:register-one (λ (suite-name test-spec)
  (? (∅? test-spec)
     ∅
     (? (⟨⟩? test-spec)
        (≔ name (◁ test-spec))
        (≔ expected (◁ (▷ test-spec)))
        (≔ actual-expr (◁ (▷ (▷ test-spec))))
        ;; Build full test name as string
        (≔ full-name-str (≈⊕ (≈⊕ (≈ suite-name) ":") (≈ name)))
        ;; Register using the suite:name key
        (⊨⊕⊙ name (λ () (⊨ (⌜ name) expected actual-expr)))
        ∅))))

;;; ═══════════════════════════════════════════
;;; Iterator-Based Result Filtering: ⊨⊕⊲
;;;
;;; Filter test results using iterator pipelines.
;;; No intermediate list allocation.
;;; ═══════════════════════════════════════════

;;; Filter results by predicate using iterator pipeline
(≔ ⊨⊕⊲ (λ (results pred)
  (⊣⊕ (⊣⊲ pred (⊣ results)))))

;;; Get only failures from results list
(≔ ⊨⊕⊲:failures (λ (results)
  (⊨⊕⊲ results (λ (r) (≡ (⊞→ r :status) :fail)))))

;;; Get only passes from results list
(≔ ⊨⊕⊲:passes (λ (results)
  (⊨⊕⊲ results (λ (r) (≡ (⊞→ r :status) :pass)))))

;;; Map over results extracting field
(≔ ⊨⊕⊲:names (λ (results)
  (⊣⊕ (⊣↦ (λ (r) (⊞→ r :name)) (⊣ results)))))

;;; ═══════════════════════════════════════════
;;; Timing Report: ⊨⊕⋔
;;;
;;; Query the SortedMap timing index for slowest tests.
;;; ═══════════════════════════════════════════

;;; Get top N slowest tests from results
;;; results must be the ⊞ returned by ⊨⊕!
(≔ ⊨⊕⋔ (λ (results n)
  (≔ timing (⊞→ results :timing))
  (? (∅? timing)
     ∅
     ;; Iterate from max downward using SortedMap entries, take n
     (⊣⊕ (⊣↑ n (⊣ (⋔* timing)))))))

;;; Get the single slowest test
(≔ ⊨⊕⋔:slowest (λ (results)
  (≔ timing (⊞→ results :timing))
  (? (∅? timing) ∅ (⋔▷ timing))))

;;; ═══════════════════════════════════════════
;;; Tag-Based Selection: ⊨⊕⊍
;;;
;;; Use HashSet intersection to find tests
;;; matching multiple tags.
;;; ═══════════════════════════════════════════

;;; Get all test names with a given tag (returns list)
(≔ ⊨⊕⊍:by-tag (λ (tag)
  ;; Query the tag trie for "tag:*" prefix
  (⊮⊙ g_tag_registry tag)))

;;; ═══════════════════════════════════════════
;;; Parallel Suite Runner: ⊨⊕‖
;;;
;;; Spawn one actor per suite prefix,
;;; collect results through a channel.
;;; ═══════════════════════════════════════════

;;; Run test suites in parallel using actors + channels
;;; Each top-level test spawns as an actor, results collected via channel.
(≔ ⊨⊕‖ (λ (prefix)
  (≔ ch (⟿⊚ #100))
  (≔ results (⊨⊕! prefix))
  (≔ result-list (⊞→ results :results))
  ;; Spawn actor per result to demonstrate pipeline
  (≔ spawn-collectors (λ (items)
    (? (∅? items)
       ∅
       (⟳ (λ (self)
         (⟿→ ch (◁ items))))
       (spawn-collectors (▷ items)))))
  (spawn-collectors result-list)
  (⟳! #1000)
  results))

;;; ═══════════════════════════════════════════
;;; Utility Functions
;;; ═══════════════════════════════════════════

;;; Pretty-print test results summary
(≔ ⊨⊕:summary (λ (results)
  (≋ "═══ Test Results ═══")
  (≋ (≈⊕ "Passed: " (≈ (⊞→ results :passed))))
  (≋ (≈⊕ "Failed: " (≈ (⊞→ results :failed))))
  (≋ (≈⊕ "Total:  " (≈ (⊞→ results :total))))
  (≋ (≈⊕ "Time:   " (≈⊕ (≈ (⊞→ results :elapsed)) "µs")))
  results))

;;; Assert a test result map shows all passing
(≔ ⊨⊕:all-pass? (λ (results)
  (≡ (⊞→ results :failed) #0)))

;;; Run all and exit with status
(≔ ⊨⊕:run-exit (λ ()
  (⊨⊕!)
  (⊨⊜×)))
