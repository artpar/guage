# Consistency, Correctness & Completeness Plan
## Week 2 Day 11+ (2026-01-27 onwards)

## Executive Summary

**Current State:** Day 10 complete - Self-testing primitive (⌂⊨) implemented
**Status:** 62 primitives (55 functional, 7 placeholders), 243+ tests passing
**Goal:** Complete test infrastructure, systematic primitive testing, prepare for Pattern Matching

---

## Phase 1: Consistency Audit ✅ COMPLETE

### 1.1 Primitive Count Verification ✅
- **Actual Count:** 62 primitives in primitives.c (verified)
- **Documentation:** SESSION_HANDOFF.md says 62 ✅
- **SPEC.md:** Says 62 ✅
- **Status:** CONSISTENT ✅

### 1.2 Primitive Breakdown
| Category | Count | Status |
|----------|-------|--------|
| Core Lambda Calculus | 3 | ✅ Functional |
| Metaprogramming | 2 | ⚠️ ⌞ placeholder |
| Comparison & Logic | 5 | ✅ Functional |
| Arithmetic | 9 | ✅ Functional |
| Type Predicates | 6 | ✅ Functional |
| Debug & Error | 4 | ✅ Functional |
| Self-Introspection | 2 | ✅ Functional |
| Testing | 2 | ✅ Functional |
| Effects | 4 | ❌ All placeholders |
| Actors | 3 | ❌ All placeholders |
| Documentation | 5 | ✅ Functional (incl. ⌂⊨!) |
| CFG/DFG | 2 | ✅ Functional |
| Structure - Leaf | 5 | ✅ Functional |
| Structure - Node/ADT | 4 | ✅ Functional |
| Graph | 6 | ✅ Functional |
| **TOTAL** | **62** | **55 functional, 7 placeholders** |

---

## Phase 2: Structure-Based Test Generation (CRITICAL) 🎯

### 2.1 Current Limitations of ⌂⊨

**Current Implementation (primitives.c:1340-1433):**
- ✅ Type-based test generation
- ✅ Works for primitives and user functions
- ❌ Only generates basic type conformance tests
- ❌ Doesn't analyze function structure
- ❌ Doesn't generate edge case tests
- ❌ Doesn't handle conditionals or recursion

**Example Current Output:**
```scheme
(⌂⊨ (⌜ ⊕))  ; → (⊨ :test-normal-case #t (ℕ? (⊕ #5 #3)))
```

**What's Missing:**
- No branch coverage for conditionals
- No base case tests for recursion
- No edge cases (0, negative, large numbers)
- No boundary tests (min, max values)

### 2.2 Structure Analysis Requirements

**Add AST Pattern Detection:**
1. **Conditional Detection** (`?` expressions)
   - Generate tests for both branches
   - Test guard conditions
   - Edge cases where condition flips

2. **Recursion Detection** (self-reference)
   - Base case tests
   - Simple recursive case tests
   - Termination tests

3. **Edge Case Detection** (from comparisons)
   - Zero values
   - Boundary values (min/max)
   - Empty lists (∅)
   - Single-element lists

**Implementation Approach:**
```c
/* Enhanced ⌂⊨ with structure analysis */
Cell* prim_doc_tests(Cell* args) {
    Cell* quoted = arg1(args);
    Cell* name = unquote_if_needed(quoted);

    /* Get function body */
    Cell* body = get_function_body(name);

    /* Type-based tests (existing) */
    Cell* type_tests = generate_type_tests(name, body);

    /* NEW: Structure-based tests */
    Cell* structure_tests = analyze_and_generate_tests(body, name);

    /* Combine all tests */
    return append_tests(type_tests, structure_tests);
}

/* Structure analysis helper */
Cell* analyze_and_generate_tests(Cell* body, Cell* func_name) {
    Cell* tests = cell_nil();

    /* Detect conditionals */
    if (has_conditional(body)) {
        tests = append_tests(tests, generate_branch_tests(body, func_name));
    }

    /* Detect recursion */
    if (has_recursion(body, func_name)) {
        tests = append_tests(tests, generate_recursion_tests(body, func_name));
    }

    /* Detect edge cases from comparisons */
    if (has_comparisons(body)) {
        tests = append_tests(tests, generate_edge_case_tests(body, func_name));
    }

    return tests;
}
```

**Expected Enhanced Output:**
```scheme
(≔ ! (λ (n) (? (≡ n #0) #1 (⊗ n (! (⊖ n #1))))))
(⌂⊨ (⌜ !))
; → ⟨
;     (⊨ :test-!-type #t (ℕ? (! #5)))               ; Type test
;     (⊨ :test-!-base-case #t (≡ (! #0) #1))        ; Base case
;     (⊨ :test-!-recursive #t (≡ (! #5) #120))      ; Recursive case
;     (⊨ :test-!-small #t (≡ (! #1) #1))            ; Edge: small n
;   ⟩
```

### 2.3 Implementation Steps

**Step 1: Add AST traversal helpers**
- `has_conditional(body)` - Detects `?` expressions
- `has_recursion(body, name)` - Detects self-reference
- `has_comparisons(body)` - Detects `≡`, `<`, `>`, etc.
- `extract_compared_values(body)` - Gets boundary values

**Step 2: Implement test generators**
- `generate_branch_tests()` - Tests both branches of conditionals
- `generate_recursion_tests()` - Tests base + recursive cases
- `generate_edge_case_tests()` - Tests boundaries from comparisons

**Step 3: Integration**
- Modify `prim_doc_tests()` to call new generators
- Ensure backward compatibility with existing tests
- Add comprehensive tests for new functionality

---

## Phase 3: Test Runner Infrastructure 🎯

### 3.1 Requirements

**Core Functionality:**
1. Collect all auto-generated tests from ⌂⊨
2. Execute each test
3. Report pass/fail with details
4. Integration with existing test suite

**Design:**
```scheme
; Test runner function
(≔ run-tests (λ (test-list)
  (? (∅? test-list)
     ⟨:passed #0 :failed #0⟩
     ; Execute test and recurse
     (execute-and-recurse (◁ test-list) (▷ test-list)))))

; Execute single test
(≔ execute-test (λ (test)
  ; Test is (⊨ :name expected actual)
  (≔ name (◁ (▷ test)))
  (≔ expected (◁ (▷ (▷ test))))
  (≔ actual (◁ (▷ (▷ (▷ test)))))

  (? (≟ expected actual)
     ⟨:pass name⟩
     ⟨:fail name expected actual⟩)))
```

**Integration Points:**
1. New Scheme file: `tests/test_runner.scm`
2. Bash wrapper: `run_auto_tests.sh`
3. Integration with `run_tests.sh`

### 3.2 Implementation Approach

**File: tests/test_runner.scm**
```scheme
; Collect tests for all primitives
(≔ all-primitive-tests (λ ()
  (⟨
    (⌂⊨ (⌜ ⊕))  ; Arithmetic
    (⌂⊨ (⌜ ⊖))
    (⌂⊨ (⌜ ⊗))
    ; ... all 55 functional primitives
  ⟩))

; Run all tests and report
(≔ run-all-tests (λ ()
  (≔ tests (all-primitive-tests))
  (≔ results (run-tests tests))
  (report-results results)))
```

**File: run_auto_tests.sh**
```bash
#!/bin/bash
echo "Running auto-generated tests..."
./guage < tests/test_runner.scm
```

---

## Phase 4: Systematic Primitive Testing 🎯

### 4.1 Test Coverage Matrix

| Primitive | Type Tests | Structure Tests | Edge Cases | Status |
|-----------|-----------|-----------------|------------|--------|
| **Arithmetic (9)** |
| ⊕ | ✅ | ⏳ | ⏳ | Partial |
| ⊖ | ✅ | ⏳ | ⏳ | Partial |
| ⊗ | ✅ | ⏳ | ⏳ | Partial |
| ⊘ | ✅ | ⏳ | ⏳ | Partial (div-by-zero handled) |
| % | ✅ | ⏳ | ⏳ | Partial (mod-by-zero handled) |
| <, >, ≤, ≥ | ✅ | ⏳ | ⏳ | Partial |
| **Comparison (5)** |
| ≡, ≢ | ✅ | N/A | ⏳ | Partial |
| ∧, ∨, ¬ | ✅ | N/A | ⏳ | Partial |
| **Type Predicates (6)** |
| ℕ?, 𝔹?, :?, ∅?, ⟨⟩?, #? | ✅ | N/A | ✅ | Complete |
| **Lists (3)** |
| ⟨⟩, ◁, ▷ | ✅ | N/A | ✅ | Complete (45+ tests) |
| **Structures (15)** |
| All ⊙, ⊚, ⊝ | ✅ | N/A | ✅ | Complete (46+ tests) |
| **Documentation (5)** |
| ⌂, ⌂∈, ⌂≔, ⌂⊛, ⌂⊨ | ✅ | N/A | ⏳ | Partial |
| **CFG/DFG (2)** |
| ⌂⟿, ⌂⇝ | ✅ | N/A | ✅ | Complete (22+ tests) |
| **Error Handling (4)** |
| ⚠, ⚠?, ⊢, ⟲ | ✅ | N/A | ✅ | Complete (40+ tests) |

### 4.2 Gaps to Fill

**High Priority:**
1. **Arithmetic edge cases**
   - Large numbers (overflow)
   - Negative numbers
   - Zero handling
   - Floating point precision

2. **Documentation primitives**
   - Error cases (undefined symbols)
   - Complex functions
   - Recursive functions

3. **Self-introspection**
   - Various lambda arities
   - Nested lambdas
   - Closures

**Medium Priority:**
4. **Metaprogramming**
   - Quote/unquote combinations
   - Nested quotes

5. **Comparison operators**
   - All combinations of values
   - Type mixing

---

## Phase 5: Property-Based Testing Foundation 🎯

### 5.1 Concept

**Property-based testing** generates random test cases to verify properties:

```scheme
; Instead of:
(⊨ :test-add (≡ (⊕ #2 #3) #5))

; Do:
(∀ x (∀ y (≡ (⊕ x y) (⊕ y x))))  ; Commutativity
```

### 5.2 Implementation Strategy

**Phase 5A: Random value generation**
```scheme
(≔ random-number (λ () ...))    ; Generate random ℕ
(≔ random-bool (λ () ...))      ; Generate random 𝔹
(≔ random-list (λ (n) ...))     ; Generate random list
```

**Phase 5B: Property assertions**
```scheme
(≔ property (λ (name predicate)
  (≔ runs #100)  ; Test 100 random cases
  (test-property name predicate runs)))
```

**Phase 5C: Integration with ⌂⊨**
```scheme
; Properties as tests
(⌂⊨ (⌜ ⊕))
; → ⟨
;     (property :add-commutative (λ (x y) (≡ (⊕ x y) (⊕ y x))))
;     (property :add-associative (λ (x y z) (≡ (⊕ (⊕ x y) z) (⊕ x (⊕ y z)))))
;   ⟩
```

---

## Implementation Timeline

### Week 2 Day 11 (Today)
- ✅ Phase 1: Consistency audit (DONE)
- 🎯 Phase 2.1: Design structure analysis (IN PROGRESS)
- 🎯 Phase 2.2: Implement conditional detection
- 🎯 Phase 2.3: Implement recursion detection

### Week 2 Day 12
- 🎯 Phase 2.4: Implement edge case detection
- 🎯 Phase 2.5: Integration and testing
- 🎯 Phase 3.1: Design test runner

### Week 2 Day 13
- 🎯 Phase 3.2: Implement test runner
- 🎯 Phase 3.3: Integration with existing tests
- 🎯 Phase 4.1: Begin systematic primitive testing

### Week 2 Day 14
- 🎯 Phase 4.2: Complete primitive testing
- 🎯 Phase 4.3: Document coverage
- 🎯 Phase 5.1: Design property-based testing

### Week 3 Day 1
- 🎯 Phase 5.2: Implement property-based foundation
- 🎯 Final documentation update
- 🎯 Prepare for Pattern Matching phase

---

## Success Metrics

### Must Have (Week 2 Complete)
- ✅ Structure-based test generation (conditionals, recursion, edges)
- ✅ Test runner infrastructure working
- ✅ All 55 functional primitives tested
- ✅ 300+ total tests passing

### Should Have
- ✅ Property-based testing foundation
- ✅ Comprehensive documentation
- ✅ Clean handoff to Pattern Matching phase

### Nice to Have
- ⏳ Mutation testing basics
- ⏳ Test coverage visualization
- ⏳ Performance benchmarks

---

## Risk Assessment

### Low Risk ✅
- Structure analysis is additive (doesn't break existing)
- Test runner is standalone
- Primitive testing is systematic

### Medium Risk ⚠️
- Structure analysis complexity might grow
- Test execution time might increase
- Edge case generation might be incomplete

### Mitigation
- Start simple, iterate
- Parallel test execution if needed
- Focus on high-value tests first

---

## Next Steps (Immediate)

1. **Start Phase 2.2:** Implement conditional detection in ⌂⊨
2. **Add helper functions:** AST traversal utilities
3. **Test incrementally:** Each enhancement should work independently
4. **Document as we go:** Update examples and tests

---

**Status:** Plan complete, ready to execute Phase 2.2
**Priority:** HIGH - Structure-based tests are critical for correctness
**Timeline:** Days 11-14 (4 days)
**Goal:** Self-testing system that truly validates correctness

---

**Prepared by:** Claude Sonnet 4.5
**Date:** 2026-01-27
**Session:** Day 11 Planning
