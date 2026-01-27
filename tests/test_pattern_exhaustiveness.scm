; Test pattern exhaustiveness checking
; Day 19: Exhaustiveness warnings for pattern matching

; ============================================================================
; COMPLETE COVERAGE - Should NOT warn
; ============================================================================

; Test 1: Wildcard pattern (complete)
; Should NOT warn - wildcard catches all
(⊨ :complete-wildcard :ok
   (∇ #50 (⌜ ((_ :ok)))))

; Test 2: Variable pattern (complete)
; Should NOT warn - variable catches all
(⊨ :complete-variable #50
   (∇ #50 (⌜ ((x x)))))

; Test 3: Multiple patterns with catch-all (complete)
; Should NOT warn - has catch-all at end
(⊨ :complete-fallback :other
   (∇ #50 (⌜ ((#42 :is-42) (_ :other)))))

; Test 4: Boolean patterns complete
; Should NOT warn - both #t and #f covered with wildcard
(⊨ :complete-bool-wildcard :ok
   (∇ #t (⌜ ((_ :ok)))))

; Test 5: ADT with catch-all variable
; Should NOT warn - catch-all handles all variants
(⊚≔ :Option (⌜ (:None)) (⌜ (:Some :value)))
(≔ test-opt (⊚ :Option :Some #42))
(⊨ :complete-adt-wildcard #t
   (⊚? test-opt :Option :Some))  ; Check type and variant

; ============================================================================
; PARTIAL COVERAGE - SHOULD warn (incomplete)
; ============================================================================

; Test 6: Number without catch-all
; SHOULD WARN - numbers are infinite domain
(⊨ :partial-number :matched
   (∇ #42 (⌜ ((#42 :matched)))))

; Test 7: Symbol without catch-all
; SHOULD WARN - symbols are infinite domain
(⊨ :partial-symbol :matched
   (∇ :foo (⌜ ((:foo :matched)))))

; Test 8: Multiple literals without catch-all
; SHOULD WARN - doesn't cover all numbers
(⊨ :partial-multi-literal #t
   (⚠? (∇ #99 (⌜ ((#42 :first) (#50 :second))))))  ; Will error - no match

; Test 9: Boolean only one case
; SHOULD WARN - missing #f case
(⊨ :partial-bool-one :matched
   (∇ #t (⌜ ((#t :matched)))))

; Test 10: Pair without catch-all
; SHOULD WARN - pairs have infinite variations
(≔ test-pair (⟨⟩ #1 #2))
(⊨ :partial-pair :matched
   (∇ test-pair (⌜ (((⟨⟩ #1 #2) :matched)))))

; ============================================================================
; REDUNDANT PATTERNS - SHOULD warn (unreachable)
; ============================================================================

; Test 11: Pattern after wildcard (unreachable)
; SHOULD WARN - #42 pattern is unreachable
(⊨ :redundant-after-wildcard :any
   (∇ #42 (⌜ ((_ :any) (#42 :specific)))))

; Test 12: Pattern after variable (unreachable)
; SHOULD WARN - #42 pattern is unreachable
(⊨ :redundant-after-variable #50
   (∇ #50 (⌜ ((x x) (#42 :specific)))))

; Test 13: Multiple catch-alls (second unreachable)
; SHOULD WARN - second wildcard is unreachable
(⊨ :redundant-double-wildcard :ok
   (∇ #42 (⌜ ((_ :ok) (_ :unreachable)))))

; Test 14: Specific after catch-all (unreachable)
; SHOULD WARN - boolean pattern after wildcard is unreachable
(⊨ :redundant-specific-after-catch :ok
   (∇ #t (⌜ ((_ :ok) (#t :unreachable) (#f :unreachable)))))

; ============================================================================
; ADT EXHAUSTIVENESS
; ============================================================================

; Test 15: ADT with all variants covered (complete)
; Should NOT warn - all variants explicitly matched
(⊚≔ :Result (⌜ (:Ok :value)) (⌜ (:Err :message)))
(≔ test-ok (⊚ :Result :Ok #42))
(≔ test-err (⊚ :Result :Err :error))

(⊨ :adt-complete-ok #42
   (∇ test-ok (⌜ (((⊚ :Result :Ok v) v)
                  ((⊚ :Result :Err m) #0)))))

(⊨ :adt-complete-err #0
   (∇ test-err (⌜ (((⊚ :Result :Ok v) v)
                   ((⊚ :Result :Err m) #0)))))

; Test 16: ADT with missing variant (partial)
; SHOULD WARN - missing :Err variant
(⊨ :adt-partial-ok #42
   (∇ test-ok (⌜ (((⊚ :Result :Ok v) v)))))

; Test 17: ADT with catch-all instead of explicit variants (complete but suboptimal)
; Should NOT warn - catch-all covers all
(⊨ :adt-catch-all-ok :any
   (∇ test-ok (⌜ ((_ :any)))))

; ============================================================================
; STRUCTURE EXHAUSTIVENESS
; ============================================================================

; Test 18: Structure with catch-all (complete)
; Should NOT warn - catch-all covers all
(⊙≔ :Point :x :y)
(≔ test-point (⊙ :Point #3 #4))

(⊨ :struct-complete-wildcard :ok
   (∇ test-point (⌜ ((_ :ok)))))

; Test 19: Structure without catch-all (partial)
; SHOULD WARN - only matches specific values
(⊨ :struct-partial-literal :matched
   (∇ test-point (⌜ (((⊙ :Point #3 #4) :matched)))))

; Test 20: Structure with variable destructuring (complete)
; Should NOT warn - variables catch all values
(⊨ :struct-complete-vars #7
   (∇ test-point (⌜ (((⊙ :Point x y) (⊕ x y))))))

; ============================================================================
; EDGE CASES
; ============================================================================

; Test 21: Empty clause list
; Should handle gracefully (runtime will error with :no-match)
(⊨ :edge-empty-clauses #t
   (⚠? (∇ #42 (⌜ ()))))

; Test 22: Single clause with literal (partial)
; SHOULD WARN - no catch-all
(⊨ :edge-single-literal :matched
   (∇ #42 (⌜ ((#42 :matched)))))

; Test 23: Nil pattern with catch-all (complete)
; Should NOT warn - catch-all present
(⊨ :edge-nil-complete :nil-case
   (∇ ∅ (⌜ ((∅ :nil-case) (_ :other)))))

; Test 24: Pair patterns with multiple levels (complete)
; Should NOT warn - has catch-all
(≔ nested-pair (⟨⟩ (⟨⟩ #1 #2) #3))
(⊨ :edge-nested-complete #6
   (∇ nested-pair (⌜ (((⟨⟩ (⟨⟩ a b) c) (⊕ a (⊕ b c)))
                      (_ #0)))))

; Test 25: Mixed pattern types (complete)
; Should NOT warn - wildcard at end
(⊨ :edge-mixed-complete :literal
   (∇ #42 (⌜ ((#42 :literal)
              ((⟨⟩ x y) :pair)
              (_ :wildcard)))))

; ============================================================================
; SUMMARY
; ============================================================================

; Expected warnings during test run:
; - Tests 6-10: Incomplete pattern warnings (5 warnings)
; - Tests 11-14: Unreachable pattern warnings (4 warnings)
; - Test 16: Incomplete ADT pattern warning (1 warning)
; - Test 19: Incomplete structure pattern warning (1 warning)
; - Test 22: Incomplete single literal warning (1 warning)
;
; Total expected warnings: 12 warnings
;
; Tests that should NOT warn: 13 tests (complete coverage)
; Tests that SHOULD warn: 12 tests (partial/redundant)
;
; Total tests: 25

(⊢ #t :all-exhaustiveness-tests-defined)
