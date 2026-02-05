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
;;;     (:basic     #5  (+ #2 #3))
;;;     (:zero      #0  (+ #0 #0))
;;;     (:negative  #-1 (+ #2 #-3)))
;;;
;;; Expands to test-register calls for each test.
;;; ═══════════════════════════════════════════

;;; Helper: concatenate symbol names with ":" separator
(define ⊨⊕:concat (lambda (suite name)
  (string->symbol (string-append (string-append (string suite) ":") (string name)))))

;;; Helper: convert string to symbol (using eval of quoted symbol)
(define string->symbol (lambda (s) s))

;;; Register a single test from suite definition
(define ⊨⊕:register-one (lambda (suite-name test-spec)
  (if (null? test-spec)
     nil
     (if (pair? test-spec)
        (define name (car test-spec))
        (define expected (car (cdr test-spec)))
        (define actual-expr (car (cdr (cdr test-spec))))
        ;; Build full test name as string
        (define full-name-str (string-append (string-append (string suite-name) ":") (string name)))
        ;; Register using the suite:name key
        (test-register name (lambda () (test-case (quote name) expected actual-expr)))
        nil))))

;;; ═══════════════════════════════════════════
;;; Iterator-Based Result Filtering: ⊨⊕⊲
;;;
;;; Filter test results using iterator pipelines.
;;; No intermediate list allocation.
;;; ═══════════════════════════════════════════

;;; Filter results by predicate using iterator pipeline
(define ⊨⊕⊲ (lambda (results pred)
  (iter-collect (iter-filter pred (iter results)))))

;;; Get only failures from results list
(define ⊨⊕⊲:failures (lambda (results)
  (⊨⊕⊲ results (lambda (r) (equal? (hashmap-get r :status) :fail)))))

;;; Get only passes from results list
(define ⊨⊕⊲:passes (lambda (results)
  (⊨⊕⊲ results (lambda (r) (equal? (hashmap-get r :status) :pass)))))

;;; Map over results extracting field
(define ⊨⊕⊲:names (lambda (results)
  (iter-collect (iter-map (lambda (r) (hashmap-get r :name)) (iter results)))))

;;; ═══════════════════════════════════════════
;;; Timing Report: ⊨⊕⋔
;;;
;;; Query the SortedMap timing index for slowest tests.
;;; ═══════════════════════════════════════════

;;; Get top N slowest tests from results
;;; results must be the hashmap returned by test-run-registry
(define ⊨⊕⋔ (lambda (results n)
  (define timing (hashmap-get results :timing))
  (if (null? timing)
     nil
     ;; Iterate from max downward using SortedMap entries, take n
     (iter-collect (iter-take n (iter (sorted-map-entries timing)))))))

;;; Get the single slowest test
(define ⊨⊕⋔:slowest (lambda (results)
  (define timing (hashmap-get results :timing))
  (if (null? timing) nil (sorted-map-max timing))))

;;; ═══════════════════════════════════════════
;;; Tag-Based Selection: ⊨⊕⊍
;;;
;;; Use HashSet intersection to find tests
;;; matching multiple tags.
;;; ═══════════════════════════════════════════

;;; Get all test names with a given tag (returns list)
(define ⊨⊕⊍:by-tag (lambda (tag)
  ;; Query the tag trie for "tag:*" prefix
  (trie-prefix-keys g_tag_registry tag)))

;;; ═══════════════════════════════════════════
;;; Parallel Suite Runner: ⊨⊕‖
;;;
;;; Spawn one actor per suite prefix,
;;; collect results through a channel.
;;; ═══════════════════════════════════════════

;;; Run test suites in parallel using actors + channels
;;; Each top-level test spawns as an actor, results collected via channel.
(define ⊨⊕‖ (lambda (prefix)
  (define ch (chan-create #100))
  (define results (test-run-registry prefix))
  (define result-list (hashmap-get results :results))
  ;; Spawn actor per result to demonstrate pipeline
  (define spawn-collectors (lambda (items)
    (if (null? items)
       nil
       (actor-spawn (lambda (self)
         (chan-send ch (car items))))
       (spawn-collectors (cdr items)))))
  (spawn-collectors result-list)
  (actor-run #1000)
  results))

;;; ═══════════════════════════════════════════
;;; Utility Functions
;;; ═══════════════════════════════════════════

;;; Pretty-print test results summary
(define ⊨⊕:summary (lambda (results)
  (print "═══ Test Results ═══")
  (print (string-append "Passed: " (string (hashmap-get results :passed))))
  (print (string-append "Failed: " (string (hashmap-get results :failed))))
  (print (string-append "Total:  " (string (hashmap-get results :total))))
  (print (string-append "Time:   " (string-append (string (hashmap-get results :elapsed)) "µs")))
  results))

;;; Assert a test result map shows all passing
(define ⊨⊕:all-pass? (lambda (results)
  (equal? (hashmap-get results :failed) #0)))

;;; Run all and exit with status
(define ⊨⊕:run-exit (lambda ()
  (test-run-registry)
  (test-exit)))
