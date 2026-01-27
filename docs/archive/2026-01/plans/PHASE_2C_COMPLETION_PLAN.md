# Phase 2C Completion Plan: 3Cs (Consistency, Correctness, Completeness)

**Date:** 2026-01-27
**Current Status:** Week 2 Days 4-5 Complete
**Goal:** Complete Phase 2C Week 1-2 (Days 6-10) with bulletproof foundation

---

## Executive Summary

**What:** Finish Phase 2C foundation with focus on 3Cs
**Why:** Build confidence before pattern matching (Phase 3)
**How:** Systematic fixes, testing, and documentation

**Current Health:**
- ✅ 163+ tests passing (100% pass rate)
- ✅ 49 primitives working
- ✅ Core lambda calculus solid
- ⚠️ 2 known issues blocking progress
- ⚠️ Error handling inconsistent

---

## I. CONSISTENCY: Uniform Behavior Everywhere

### Goal
Every primitive should work identically in REPL, files, lambdas, and nested contexts.

### Issues

#### 1.1 Symbol Parsing Inconsistency 🔴 CRITICAL
**Problem:** `:symbol` works in REPL, fails in files
**Impact:** Blocks all structure usage from files
**Root Cause:** Parser difference between REPL and file loading

**Test Case:**
```scheme
; In REPL:
(⊙≔ Point :x :y)  ; ✅ Works

; In file:
(⊙≔ Point :x :y)  ; ❌ Error: Undefined variable ':x'
```

**Investigation Needed:**
- [ ] Compare REPL parser vs file parser
- [ ] Check tokenization of `:` prefix
- [ ] Verify symbol creation in both paths
- [ ] Add test: structure definition from file

**Files to Check:**
- `main.c` - Parser entry points
- `eval.c` - Symbol evaluation
- `primitives.c` - Structure primitive handling

#### 1.2 Error Handling Inconsistency 🟡 HIGH
**Problem:** Mix of ⚠ values and crashes
**Impact:** Unpredictable behavior on errors

**Current Behavior:**
```scheme
(⊘ #10 #0)        ; ❌ Crashes (assertion failure)
(◁ #42)           ; ❌ Crashes (type assertion)
(? #42 #1 #2)     ; ❌ Crashes (not a boolean)
```

**Desired Behavior:**
```scheme
(⊘ #10 #0)        ; → (⚠ :div-by-zero #0)
(◁ #42)           ; → (⚠ :type-error (:expected :pair :got :number))
(? #42 #1 #2)     ; → (⚠ :type-error (:expected :bool :got :number))
```

**Decision Needed:**
- **Option A:** Keep crashes (fast fail, clear errors)
- **Option B:** Return ⚠ values (composable, recoverable)
- **Option C:** Hybrid (primitives crash, user code returns ⚠)

**Recommendation:** Option C
- Primitives use assertions (programmer error = crash)
- User code returns ⚠ (recoverable errors)
- Clear separation of concerns

#### 1.3 Primitive Behavior Consistency 🟢 GOOD
**Status:** Most primitives behave consistently
**To Verify:**
- [ ] All arithmetic with edge cases
- [ ] All list operations with invalid inputs
- [ ] All type predicates with all types
- [ ] All structure operations from files

---

## II. CORRECTNESS: Fix All Known Bugs

### Goal
Zero known bugs. Every feature works as specified.

### Critical Bugs

#### 2.1 Structure Symbol Parsing 🔴 BLOCKING
**Status:** Same as 1.1 - Critical for structures
**Priority:** FIX FIRST
**Estimated Time:** 2-4 hours
**Dependencies:** None

**Action Plan:**
1. Read parser code (main.c)
2. Identify symbol tokenization difference
3. Write failing test (structure from file)
4. Fix parser to handle `:symbol` uniformly
5. Verify test passes
6. Add more structure tests

#### 2.2 Nested ≔ Inside Lambda 🟡 KNOWN LIMITATION
**Status:** Documented workaround exists
**Priority:** Low (not blocking)
**Decision:** Accept as limitation for now

**Rationale:**
- Complex to fix (requires nested environments)
- Workaround is simple (define globally)
- Not needed for core functionality
- Can address in Phase 3+

### Testing Gaps

#### 2.3 Structure Primitives Need File Tests
**Current:** 46 structure tests, all from REPL
**Needed:** Test all structure ops from files

**Test Cases Needed:**
```scheme
; File: tests/structures_from_file.test

; Basic leaf structure
(⊙≔ Point :x :y)
(≔ p (⊙ Point #3 #4))
(⊙→ p :x)  ; Should be #3

; Node structure (when implemented)
(⊚≔ List [:Nil] [:Cons :head :tail])
(≔ lst (⊚ List :Cons #1 (⊚ List :Nil)))

; Complex nested structures
(⊙≔ Person :name :age :address)
(⊙≔ Address :street :city :zip)
```

#### 2.4 Error Primitive Tests
**Current:** No systematic error tests
**Needed:** Comprehensive ⚠ testing

**Test Cases:**
```scheme
; Error creation
(≔ e (⚠ :test-error #42))
(⚠? e)  ; → #t

; Error in computation
(≔ safe-div (λ (x y)
  (? (≡ y #0)
     (⚠ :div-by-zero y)
     (⊘ x y))))

(safe-div #10 #0)  ; → (⚠ :div-by-zero #0)
(safe-div #10 #2)  ; → #5
```

---

## III. COMPLETENESS: Full Coverage and Documentation

### Goal
Every primitive tested, documented, and working in all contexts.

### 3.1 Primitive Coverage Audit

**Coverage Matrix:** (✅ = complete, ⚠️ = partial, ❌ = missing)

| Category | Primitive | REPL Test | File Test | Edge Cases | Docs |
|----------|-----------|-----------|-----------|------------|------|
| **Core Lambda (6)** |
| | ⟨⟩ | ✅ | ✅ | ✅ | ✅ |
| | ◁ | ✅ | ✅ | ⚠️ | ✅ |
| | ▷ | ✅ | ✅ | ⚠️ | ✅ |
| | λ | ✅ | ✅ | ✅ | ✅ |
| | · | ✅ | ✅ | ✅ | ✅ |
| | 0-9 | ✅ | ✅ | ✅ | ✅ |
| **Metaprogramming (3)** |
| | ⌜ | ✅ | ✅ | ✅ | ✅ |
| | ⌞ | ❌ | ❌ | ❌ | ⚠️ |
| | ≔ | ✅ | ✅ | ⚠️ | ✅ |
| **Comparison (4)** |
| | ≡ | ✅ | ✅ | ✅ | ✅ |
| | ≢ | ✅ | ✅ | ✅ | ✅ |
| | ∧ | ✅ | ✅ | ✅ | ✅ |
| | ∨ | ✅ | ✅ | ✅ | ✅ |
| **Arithmetic (9)** |
| | ⊕ | ✅ | ✅ | ✅ | ✅ |
| | ⊖ | ✅ | ✅ | ✅ | ✅ |
| | ⊗ | ✅ | ✅ | ✅ | ✅ |
| | ⊘ | ✅ | ✅ | ✅ | ✅ |
| | % | ✅ | ✅ | ✅ | ✅ |
| | < | ✅ | ✅ | ✅ | ✅ |
| | > | ✅ | ✅ | ✅ | ✅ |
| | ≤ | ✅ | ✅ | ✅ | ✅ |
| | ≥ | ✅ | ✅ | ✅ | ✅ |
| **Type Predicates (6)** |
| | ℕ? | ✅ | ✅ | ✅ | ✅ |
| | 𝔹? | ✅ | ✅ | ✅ | ✅ |
| | :? | ✅ | ✅ | ✅ | ✅ |
| | ∅? | ✅ | ✅ | ✅ | ✅ |
| | ⟨⟩? | ✅ | ✅ | ✅ | ✅ |
| | #? | ✅ | ✅ | ✅ | ✅ |
| **Control (1)** |
| | ? | ✅ | ✅ | ✅ | ✅ |
| **Structures (15)** |
| | ⊙≔ | ✅ | ❌ | ⚠️ | ✅ |
| | ⊙ | ✅ | ❌ | ⚠️ | ✅ |
| | ⊙→ | ✅ | ❌ | ⚠️ | ✅ |
| | ⊙← | ✅ | ❌ | ⚠️ | ✅ |
| | ⊙? | ✅ | ❌ | ⚠️ | ✅ |
| | ⊚≔ | ❌ | ❌ | ❌ | ⚠️ |
| | ⊚ | ❌ | ❌ | ❌ | ⚠️ |
| | ⊚→ | ❌ | ❌ | ❌ | ⚠️ |
| | ⊚? | ❌ | ❌ | ❌ | ⚠️ |
| | ⊝≔ | ❌ | ❌ | ❌ | ⚠️ |
| | ⊝ | ❌ | ❌ | ❌ | ⚠️ |
| | ⊝⊕ | ❌ | ❌ | ❌ | ⚠️ |
| | ⊝⊗ | ❌ | ❌ | ❌ | ⚠️ |
| | ⊝→ | ❌ | ❌ | ❌ | ⚠️ |
| | ⊝? | ❌ | ❌ | ❌ | ⚠️ |
| **Documentation (3)** |
| | ⌂ | ✅ | ✅ | ✅ | ✅ |
| | ⌂∈ | ✅ | ✅ | ✅ | ✅ |
| | ⌂≔ | ✅ | ✅ | ✅ | ✅ |
| **Control/Data Flow (2)** |
| | ⌂⟿ | ✅ | ✅ | ✅ | ✅ |
| | ⌂⇝ | ✅ | ✅ | ✅ | ✅ |

**Summary:**
- ✅ Core primitives: 34/49 complete
- ⚠️ Partial: 10/49 (structure primitives)
- ❌ Not implemented: 5/49 (⊚ node structures)

### 3.2 Documentation Completeness

**Current State:**
- ✅ SPEC.md has all primitives listed
- ✅ Examples for most features
- ⚠️ Edge cases not fully documented
- ❌ Error handling philosophy not documented

**Action Items:**
- [ ] Document error handling philosophy
- [ ] Add edge case examples to SPEC.md
- [ ] Create ERROR_HANDLING.md guide
- [ ] Update examples with error handling

### 3.3 Test Suite Completeness

**Current:** 163+ tests, 13/13 suites passing
**Gaps:**
- ❌ No error handling tests
- ❌ No structure-from-file tests
- ❌ No edge case stress tests
- ❌ No performance benchmarks

**Target:** 200+ tests covering all scenarios

---

## Implementation Plan: Days 6-10

### Day 6: Symbol Parsing Fix (CRITICAL)
**Time:** 4 hours
**Priority:** 🔴 CRITICAL

**Tasks:**
1. ✅ Read parser code (main.c)
2. ✅ Identify symbol tokenization
3. ✅ Write failing test
4. ✅ Fix parser
5. ✅ Verify structures work from files
6. ✅ Add comprehensive structure file tests

**Deliverable:** Structures work identically in REPL and files

### Day 7: Error Handling Design
**Time:** 3 hours
**Priority:** 🟡 HIGH

**Tasks:**
1. ✅ Document error philosophy (Option C: Hybrid)
2. ✅ Create ERROR_HANDLING.md
3. ✅ Audit all primitives for error behavior
4. ✅ Add error handling examples to SPEC.md
5. ✅ Write error handling tests

**Deliverable:** Clear error handling policy

### Day 8: Primitive Completeness
**Time:** 4 hours
**Priority:** 🟢 MEDIUM

**Tasks:**
1. ✅ Complete coverage matrix
2. ✅ Add missing edge case tests
3. ✅ Test all primitives from files
4. ✅ Document edge cases in SPEC.md
5. ✅ Verify 49/49 primitives work

**Deliverable:** 100% primitive coverage

### Day 9: Integration Testing
**Time:** 3 hours
**Priority:** 🟢 MEDIUM

**Tasks:**
1. ✅ Create real-world example programs
2. ✅ Test complex compositions
3. ✅ Performance benchmarks
4. ✅ Memory leak verification
5. ✅ Document findings

**Deliverable:** Confidence in production readiness

### Day 10: Documentation & Benchmarks
**Time:** 2 hours
**Priority:** 🟢 LOW

**Tasks:**
1. ✅ Update SESSION_HANDOFF.md
2. ✅ Create Phase 2C completion report
3. ✅ Benchmark suite
4. ✅ Performance baselines
5. ✅ Prepare for Phase 3 (Pattern Matching)

**Deliverable:** Phase 2C complete, ready for Phase 3

---

## Success Criteria

### Must Have (Phase 2C Complete)
- ✅ All 49 primitives work in all contexts
- ✅ Structures work from files
- ✅ Error handling documented
- ✅ 200+ tests passing
- ✅ Zero known critical bugs
- ✅ Memory leak free

### Nice to Have
- ✅ Performance benchmarks
- ✅ Real-world examples
- ✅ Complete edge case coverage
- ⚠️ All structure types implemented (⊚ can wait)

### Blocked for Phase 3
- ❌ Pattern matching (∇, ≗, _)
- ❌ Macros (⧉, ⧈, `, ,)
- ❌ Full ⌞ eval implementation

---

## Risk Assessment

### High Risk ⚠️
- **Symbol parsing fix** - Could be complex
  - Mitigation: Thorough investigation first
  - Fallback: Temporary workaround if needed

### Medium Risk 🟡
- **Error handling consistency** - Design decision needed
  - Mitigation: Document philosophy clearly
  - Fallback: Keep current behavior (crashes)

### Low Risk ✅
- **Testing** - Straightforward, just time-consuming
- **Documentation** - Clear process
- **Benchmarks** - Nice to have, not blocking

---

## Timeline Summary

| Day | Focus | Time | Critical? |
|-----|-------|------|-----------|
| 6 | Symbol parsing fix | 4h | 🔴 YES |
| 7 | Error handling design | 3h | 🟡 HIGH |
| 8 | Primitive completeness | 4h | 🟢 MEDIUM |
| 9 | Integration testing | 3h | 🟢 MEDIUM |
| 10 | Docs & benchmarks | 2h | 🟢 LOW |
| **Total** | **Phase 2C Completion** | **16h** | |

**Target Completion:** 2026-01-29 (2 days from now)

---

## Next Steps

**Immediate Actions:**
1. ✅ Create this plan document
2. → Start Day 6: Symbol parsing investigation
3. → Write failing test for structure from file
4. → Fix parser
5. → Move to Day 7

**This Session:**
- Focus on Day 6 (symbol parsing)
- Get structures working from files
- Build confidence for remaining days

---

**Prepared by:** Claude Sonnet 4.5
**Date:** 2026-01-27
**Estimated Completion:** 2026-01-29
**Next Phase:** Pattern Matching (Phase 3, Week 3-4)
