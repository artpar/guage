# Day 12 Plan: Complete Test Infrastructure & Systematic Coverage
## 2026-01-27 (Continuation)

## Executive Summary

**Previous:** Day 11 - Structure-based test generation implemented ✅
**Today:** Complete test runner + systematic primitive coverage
**Goal:** Full test infrastructure with comprehensive coverage report

---

## Current State Analysis

### What Works ✅
- Structure-based test generation (⌂⊨)
- Test collection framework (test_runner.scm)
- Helper functions (flatten, append, length, count-status)
- 62 primitives (55 functional, 7 placeholders)
- 243+ existing tests passing

### What's Missing ⏳
- **Test execution logic** - execute-test is stub (returns :pending)
- **Complete primitive coverage** - Only 20/55 primitives collected
- **Test reporting** - No pass/fail summary
- **Coverage matrix** - Need systematic verification

### Gaps to Fill

**Primitives Not Yet Tested:**
- ❌ Core Lambda Calculus (3): ⟨⟩, ◁, ▷ [have manual tests]
- ❌ Metaprogramming (1): ⌜ [⌞ is placeholder]
- ❌ Debug/Error (4): ⚠, ⚠?, ⊢, ⟲
- ❌ Self-Introspection (2): ⧉, ⊛
- ❌ Testing (2): ≟, ⊨
- ❌ Documentation (5): ⌂, ⌂∈, ⌂≔, ⌂⊛, ⌂⊨
- ❌ CFG/DFG (2): ⌂⟿, ⌂⇝
- ❌ Structure - Leaf (5): ⊙≔, ⊙, ⊙→, ⊙←, ⊙?
- ❌ Structure - Node/ADT (4): ⊚≔, ⊚, ⊚→, ⊚?
- ❌ Graph (6): ⊝≔, ⊝, ⊝⊕, ⊝⊗, ⊝→, ⊝?

**Total Missing:** 35 functional primitives

---

## Phase 1: Complete Test Execution Logic ⚡

### 1.1 Fix execute-test Function

**Current (lines 27-35):**
```scheme
(≔ execute-test (λ (test)
  (? (∅? test)
     ⟨:error :empty-test⟩
     (≔ name (◁ (▷ test)))
     ⟨:pending name⟩)))
```

**Problem:** Always returns :pending, doesn't actually run test

**Solution:**
```scheme
(≔ execute-test (λ (test)
  (? (∅? test)
     ⟨:error :empty-test⟩
     ; Test structure: (⊨ :name expected actual)
     (≔ test-sym (◁ test))              ; Should be ⊨
     (≔ name (◁ (▷ test)))              ; Get :name
     (≔ expected (◁ (▷ (▷ test))))      ; Get expected
     (≔ actual (◁ (▷ (▷ (▷ test)))))    ; Get actual

     ; Compare using deep equality
     (? (≟ expected actual)
        ⟨:pass name⟩
        ⟨:fail name expected actual⟩))))
```

### 1.2 Add Test Runner Loop

**New function:**
```scheme
; Execute all tests in list
(≔ run-test-list (λ (tests)
  (? (∅? tests)
     ∅
     (⟨⟩ (execute-test (◁ tests))
         (run-test-list (▷ tests))))))

; Collect results and count
(≔ summarize-results (λ (results)
  (≔ passed (count-status results :pass))
  (≔ failed (count-status results :fail))
  (≔ errors (count-status results :error))
  ⟨:passed passed :failed failed :errors errors :total (length results)⟩))
```

### 1.3 Update run-all-tests

**Enhanced version:**
```scheme
(≔ run-all-tests (λ ()
  (≔ tests (flatten (all-primitive-tests)))  ; Flatten nested lists
  (≔ results (run-test-list tests))
  (≔ summary (summarize-results results))
  ⟨:summary summary :results results⟩))
```

**Implementation:** Modify tests/test_runner.scm (lines 27-98)

---

## Phase 2: Add All 55 Functional Primitives 📋

### 2.1 Organize by Category

**Add to test_runner.scm:**

```scheme
; Core list operations
(≔ list-tests (λ ()
  (append (⌂⊨ (⌜ ⟨⟩))
  (append (⌂⊨ (⌜ ◁))
  (append (⌂⊨ (⌜ ▷))
  ∅)))))

; Metaprogramming
(≔ meta-tests (λ ()
  (append (⌂⊨ (⌜ ⌜))
  ∅)))

; Debug and error handling
(≔ debug-tests (λ ()
  (append (⌂⊨ (⌜ ⚠))
  (append (⌂⊨ (⌜ ⚠?))
  (append (⌂⊨ (⌜ ⊢))
  (append (⌂⊨ (⌜ ⟲))
  ∅))))))

; Self-introspection
(≔ introspection-tests (λ ()
  (append (⌂⊨ (⌜ ⧉))
  (append (⌂⊨ (⌜ ⊛))
  ∅))))

; Testing primitives
(≔ testing-tests (λ ()
  (append (⌂⊨ (⌜ ≟))
  (append (⌂⊨ (⌜ ⊨))
  ∅))))

; Documentation primitives
(≔ doc-tests (λ ()
  (append (⌂⊨ (⌜ ⌂))
  (append (⌂⊨ (⌜ ⌂∈))
  (append (⌂⊨ (⌜ ⌂≔))
  (append (⌂⊨ (⌜ ⌂⊛))
  (append (⌂⊨ (⌜ ⌂⊨))
  ∅)))))))

; CFG/DFG
(≔ cfg-dfg-tests (λ ()
  (append (⌂⊨ (⌜ ⌂⟿))
  (append (⌂⊨ (⌜ ⌂⇝))
  ∅))))

; Structure - Leaf
(≔ structure-leaf-tests (λ ()
  (append (⌂⊨ (⌜ ⊙≔))
  (append (⌂⊨ (⌜ ⊙))
  (append (⌂⊨ (⌜ ⊙→))
  (append (⌂⊨ (⌜ ⊙←))
  (append (⌂⊨ (⌜ ⊙?))
  ∅)))))))

; Structure - Node/ADT
(≔ structure-node-tests (λ ()
  (append (⌂⊨ (⌜ ⊚≔))
  (append (⌂⊨ (⌜ ⊚))
  (append (⌂⊨ (⌜ ⊚→))
  (append (⌂⊨ (⌜ ⊚?))
  ∅))))))

; Graph
(≔ graph-tests (λ ()
  (append (⌂⊨ (⌜ ⊝≔))
  (append (⌂⊨ (⌜ ⊝))
  (append (⌂⊨ (⌜ ⊝⊕))
  (append (⌂⊨ (⌜ ⊝⊗))
  (append (⌂⊨ (⌜ ⊝→))
  (append (⌂⊨ (⌜ ⊝?))
  ∅))))))))
```

### 2.2 Update all-primitive-tests

**Complete version:**
```scheme
(≔ all-primitive-tests (λ ()
  (append (arithmetic-tests)
  (append (comparison-tests)
  (append (logic-tests)
  (append (type-predicate-tests)
  (append (list-tests)
  (append (meta-tests)
  (append (debug-tests)
  (append (introspection-tests)
  (append (testing-tests)
  (append (doc-tests)
  (append (cfg-dfg-tests)
  (append (structure-leaf-tests)
  (append (structure-node-tests)
  (append (graph-tests)
  ∅))))))))))))))))
```

---

## Phase 3: Create Coverage Report 📊

### 3.1 Coverage Matrix Generator

**New function:**
```scheme
; Generate coverage report by category
(≔ coverage-by-category (λ ()
  (≔ arithmetic (length (arithmetic-tests)))
  (≔ comparison (length (comparison-tests)))
  (≔ logic (length (logic-tests)))
  (≔ type-pred (length (type-predicate-tests)))
  (≔ lists (length (list-tests)))
  (≔ meta (length (meta-tests)))
  (≔ debug (length (debug-tests)))
  (≔ introspect (length (introspection-tests)))
  (≔ testing (length (testing-tests)))
  (≔ docs (length (doc-tests)))
  (≔ cfg-dfg (length (cfg-dfg-tests)))
  (≔ struct-leaf (length (structure-leaf-tests)))
  (≔ struct-node (length (structure-node-tests)))
  (≔ graphs (length (graph-tests)))

  ⟨
    ⟨:arithmetic arithmetic⟩
    ⟨:comparison comparison⟩
    ⟨:logic logic⟩
    ⟨:type-predicates type-pred⟩
    ⟨:lists lists⟩
    ⟨:metaprogramming meta⟩
    ⟨:debug debug⟩
    ⟨:introspection introspect⟩
    ⟨:testing testing⟩
    ⟨:documentation docs⟩
    ⟨:cfg-dfg cfg-dfg⟩
    ⟨:structures-leaf struct-leaf⟩
    ⟨:structures-node struct-node⟩
    ⟨:graphs graphs⟩
  ⟩))
```

### 3.2 Report Generator

**Create:** reports/COVERAGE_REPORT.md

**Template:**
```markdown
# Test Coverage Report
## Generated: 2026-01-27 Day 12

## Summary
- **Total Primitives:** 62 (55 functional, 7 placeholders)
- **Primitives Tested:** XX/55
- **Total Auto-Generated Tests:** XXX
- **Tests Passed:** XXX/XXX
- **Tests Failed:** X/XXX
- **Coverage:** XX%

## By Category
| Category | Primitives | Tests Generated | Pass | Fail |
|----------|-----------|-----------------|------|------|
| Arithmetic | 9 | XX | XX | X |
| Comparison | 6 | XX | XX | X |
| Logic | 3 | XX | XX | X |
| Type Predicates | 6 | XX | XX | X |
| Lists | 3 | XX | XX | X |
| Metaprogramming | 1 | XX | XX | X |
| Debug/Error | 4 | XX | XX | X |
| Introspection | 2 | XX | XX | X |
| Testing | 2 | XX | XX | X |
| Documentation | 5 | XX | XX | X |
| CFG/DFG | 2 | XX | XX | X |
| Structures (Leaf) | 5 | XX | XX | X |
| Structures (Node) | 4 | XX | XX | X |
| Graphs | 6 | XX | XX | X |

## Detailed Results
[Full test output]

## Known Issues
[Any failures or problems]

## Next Steps
[Gaps to address]
```

---

## Phase 4: Verification & Documentation ✅

### 4.1 Run Full Test Suite

**Commands:**
```bash
# Run auto-generated tests
./guage < tests/test_runner.scm > reports/test_output.txt

# Run existing test suite
./run_tests.sh

# Compare coverage
diff reports/test_output.txt tests/*.test
```

### 4.2 Identify Gaps

**Analysis:**
1. Which primitives failed?
2. Which primitives have no tests?
3. Which categories need more coverage?
4. Are there edge cases missing?

### 4.3 Update Documentation

**Files to update:**
- SESSION_HANDOFF.md - Add Day 12 summary
- CONSISTENCY_COMPLETENESS_PLAN.md - Mark phases complete
- COVERAGE_REPORT.md - Generate fresh report

---

## Implementation Order

### Step 1: Fix Test Execution (30 min)
1. Update execute-test function
2. Add run-test-list function
3. Add summarize-results function
4. Test with existing 20 primitives

### Step 2: Add Missing Primitives (45 min)
1. Add 7 new test collection functions
2. Update all-primitive-tests to include all
3. Verify syntax and structure
4. Run test collection (no execution yet)

### Step 3: Run Full Test Suite (30 min)
1. Execute all tests
2. Collect results
3. Identify failures
4. Debug issues

### Step 4: Generate Report (30 min)
1. Create coverage matrix
2. Write COVERAGE_REPORT.md
3. Update SESSION_HANDOFF.md
4. Commit changes

**Total Time:** ~2.5 hours

---

## Success Criteria

### Must Have ✅
- [ ] Test execution logic complete
- [ ] All 55 functional primitives collected
- [ ] Test suite runs to completion
- [ ] Coverage report generated
- [ ] Documentation updated

### Should Have 📋
- [ ] 90%+ tests passing
- [ ] Clear categorization
- [ ] Failures documented
- [ ] Next steps identified

### Nice to Have 🎯
- [ ] Performance metrics
- [ ] Test execution time
- [ ] Memory usage statistics

---

## Risk Mitigation

### Low Risk ✅
- Test execution is straightforward
- Primitive collection is mechanical
- No breaking changes to existing code

### Medium Risk ⚠️
- Some primitives might fail tests
- Test structure might be inconsistent
- Report generation might reveal gaps

### Mitigation Strategy
1. Test incrementally (one category at a time)
2. Document failures clearly
3. Don't fix bugs now - just document
4. Focus on completeness, not perfection

---

## Next Session (Day 13)

**Based on Day 12 results:**
1. Fix any critical test failures
2. Enhance structure analysis (nested conditionals, list edges)
3. Add property-based testing foundation
4. Prepare for Week 3 (Pattern Matching)

---

**Status:** Ready to implement
**Priority:** HIGH - Systematic coverage is critical
**Timeline:** Day 12 (~2.5 hours)
**Goal:** Complete test infrastructure with full primitive coverage

---

**Prepared by:** Claude Sonnet 4.5
**Date:** 2026-01-27
**Session:** Day 12 Planning
