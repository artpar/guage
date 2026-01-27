# Next Steps Plan: Week 1-2 Days 4-7 (Comprehensive Testing)
## Date: 2026-01-27

---

## Current Status

**Completed:**
- ✅ Critical list operations bug FIXED (env_is_indexed)
- ✅ All 11/11 test suites passing (100%)
- ✅ 78+ tests passing
- ✅ Lists work correctly from lambdas

**Phase:** Week 1-2 of CORRECTNESS phase
**Current Day:** Day 4 (Days 1-3 complete)

---

## Immediate Priorities (Days 4-7)

### Priority 1: Comprehensive List Test Suite 🔴 CRITICAL
**Time:** 4-6 hours
**Status:** NOT STARTED

**Goals:**
1. Verify list operations work in ALL contexts
2. Test edge cases thoroughly
3. Prevent regressions
4. Build confidence in foundation

**Test Categories:**

#### 1.1 Basic Operations (10 tests)
- [ ] cons creates pair correctly
- [ ] car extracts first element
- [ ] cdr extracts second element
- [ ] nil? identifies empty list
- [ ] Empty list behavior
- [ ] Single element list
- [ ] Two element list
- [ ] Nested pairs
- [ ] Type errors (car of non-pair)
- [ ] Type errors (cdr of non-pair)

#### 1.2 List Construction (8 tests)
- [ ] Build list: (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅)))
- [ ] Build list from function calls
- [ ] Build list in lambda
- [ ] Build nested lists
- [ ] Build list of lists
- [ ] Build list of lambdas
- [ ] Build heterogeneous lists (numbers, symbols, functions)
- [ ] Build deep nested structures (10+ levels)

#### 1.3 List Traversal (8 tests)
- [ ] Length function
- [ ] Nth element access
- [ ] Map function
- [ ] Filter function
- [ ] Fold/reduce function
- [ ] Reverse function
- [ ] Append function
- [ ] Flatten nested lists

#### 1.4 Higher-Order Functions with Lists (8 tests)
- [ ] Pass list to lambda
- [ ] Return list from lambda
- [ ] Map with lambda
- [ ] Filter with lambda
- [ ] List of lambdas
- [ ] Lambda returning lambda that uses list
- [ ] Closure over list
- [ ] Recursive function with list accumulator

#### 1.5 Memory Management (5 tests)
- [ ] Large list creation (1000 elements)
- [ ] Large list traversal
- [ ] List creation in loop
- [ ] Nested list creation
- [ ] Verify no memory leaks (manual check)

**Test File:** `tests/comprehensive_lists.test`

**Expected Outcome:** 39+ tests passing, confidence that lists work everywhere

---

### Priority 2: Fix GCD/Division Semantics 🟡 HIGH
**Time:** 2-3 hours
**Status:** NOT STARTED

**Problem:**
```scheme
(≔ gcd (λ (a b) (? (≡ b #0) a (gcd b (⊘ a b)))))
(gcd #48 #18)  ; → #inf (should be #6)
```

**Investigation Steps:**
1. [ ] Test ⊘ in isolation
   ```scheme
   (⊘ #10 #3)   ; What does this return?
   (⊘ #48 #18)  ; 2.666... or 2?
   (⊘ #48 #0)   ; inf, error, or crash?
   ```

2. [ ] Determine division semantics
   - Is ⊘ integer division or float division?
   - Check primitives.c implementation
   - Check if we need modulo operator

3. [ ] Add tests for division
   - [ ] Integer division
   - [ ] Division with remainder
   - [ ] Division by zero
   - [ ] Negative numbers
   - [ ] Overflow behavior

4. [ ] Fix or document behavior
   - If float division: Add % (mod) primitive
   - If integer division: Verify correctness
   - Update SPEC.md with semantics

**Test File:** `tests/division.test`

**Expected Outcome:** Division works predictably, GCD algorithm works

---

### Priority 3: Fix Structure Symbol Parsing 🟡 HIGH
**Time:** 2-3 hours
**Status:** NOT STARTED

**Problem:**
When loading from file:
```
Error: Undefined variable 'Point'
Error: Undefined variable ':x'
⚠:⊙≔ type tag must be a symbol
```

**Investigation Steps:**
1. [ ] Test structure primitives in REPL
   ```scheme
   (⊙≔ Point :x :y)
   (≔ p (⊙ Point #3 #4))
   (⊙→ p :x)
   ```

2. [ ] Test from file
   ```scheme
   ; test_struct.scm
   (⊙≔ Point :x :y)
   (≔ p (⊙ Point #3 #4))
   (⊙→ p :x)
   ```

3. [ ] Compare parser behavior
   - REPL vs file loading
   - Symbol parsing differences
   - Tokenization of : prefix

4. [ ] Fix if needed
   - Update parser/tokenizer
   - Ensure : creates symbols correctly
   - Test all structure primitives

**Test File:** `tests/structures_from_file.test`

**Expected Outcome:** Structure primitives work identically in REPL and files

---

### Priority 4: Arithmetic Edge Cases 🟢 MEDIUM
**Time:** 1-2 hours
**Status:** NOT STARTED

**Tests Needed:**
- [ ] Overflow behavior
  - (⊗ #999999999999 #999999999999)
  - (⊕ #999999999999 #1)

- [ ] Underflow behavior
  - (⊖ #0 #1)
  - (⊖ #5 #10)

- [ ] Division edge cases
  - (⊘ #1 #0) - division by zero
  - (⊘ #0 #0) - 0/0
  - (⊘ #5 #2) - remainder handling

- [ ] Comparison edge cases
  - (< #inf #999999)
  - (≡ #inf #inf)
  - (< #0 #-1)

**Test File:** `tests/arithmetic_edge_cases.test`

**Expected Outcome:** No surprises, documented behavior

---

### Priority 5: Error Handling Consistency 🟢 MEDIUM
**Time:** 2-3 hours
**Status:** NOT STARTED

**Current Issues:**
- Some functions crash (assertions)
- Some return ⚠ values
- Some return special values (∅, #f)
- Inconsistent error messages

**Consistency Rules to Implement:**

**Rule 1: Type errors return ⚠**
```scheme
(◁ #42)           ; ⚠:type-error Expected pair, got number
(⊕ #1 :foo)       ; ⚠:type-error Expected number, got symbol
(⊙→ #42 :x)       ; ⚠:type-error Expected structure, got number
```

**Rule 2: Out of bounds returns ⚠**
```scheme
(⊇ list #999)     ; ⚠:out-of-bounds Index 999 out of range
```

**Rule 3: Arithmetic errors return special values**
```scheme
(⊘ #5 #0)         ; #inf (or ⚠:division-by-zero)
(⊖ #0 #1)         ; #-1 (allow negative)
```

**Rule 4: Assertions only for invariants**
- Keep assertions for internal consistency checks
- User errors should return ⚠
- Stack overflow can still crash

**Tasks:**
1. [ ] Document error philosophy in SPEC.md
2. [ ] Update primitives to return ⚠ instead of asserting
3. [ ] Standardize error value format
4. [ ] Add error handling examples
5. [ ] Create error handling test suite

**Test File:** `tests/error_handling.test`

**Expected Outcome:** Predictable, recoverable errors

---

## Testing Strategy

### Phase 1: Write Tests (Days 4-5)
1. Create comprehensive_lists.test (39+ tests)
2. Create division.test (10+ tests)
3. Create structures_from_file.test (8+ tests)
4. Create arithmetic_edge_cases.test (12+ tests)
5. Create error_handling.test (15+ tests)

**Total:** 84+ new tests

### Phase 2: Fix Issues (Days 5-6)
1. Fix division semantics
2. Fix structure symbol parsing
3. Standardize error handling
4. Fix any test failures

### Phase 3: Verify (Day 7)
1. Run ALL test suites (11 + new suites)
2. Verify 100% pass rate
3. Check for memory leaks
4. Run performance benchmarks
5. Update documentation

---

## Success Criteria

**Must Have:**
- ✅ All existing tests still pass (11/11 suites)
- ✅ 80+ new tests pass
- ✅ Division works correctly
- ✅ Structures work from files
- ✅ Consistent error handling
- ✅ No known bugs
- ✅ Zero memory leaks

**Documentation:**
- ✅ Division semantics documented
- ✅ Error handling philosophy documented
- ✅ Examples updated
- ✅ SESSION_HANDOFF.md updated

**Performance:**
- List(1000) creation < 10ms
- List(1000) traversal < 5ms
- No regressions

---

## Timeline

**Day 4 (Today):**
- Morning: Write comprehensive list tests (2-3 hours)
- Afternoon: Write division and arithmetic tests (1-2 hours)
- Evening: Write structure and error tests (1-2 hours)

**Day 5:**
- Morning: Fix division semantics (2 hours)
- Afternoon: Fix structure symbol parsing (2 hours)
- Evening: Run tests, fix failures (2-3 hours)

**Day 6:**
- Morning: Standardize error handling (2-3 hours)
- Afternoon: Write error handling tests (2 hours)
- Evening: Run all tests, verify pass (1-2 hours)

**Day 7:**
- Morning: Performance benchmarks (1-2 hours)
- Afternoon: Documentation updates (1-2 hours)
- Evening: Commit and handoff (1 hour)

**Total Time:** 18-24 hours

---

## After Days 4-7: What's Next?

**Days 8-10: Error Handling Deep Dive**
- Implement ⚠→ (extract error data)
- Implement ⚠∈ (get error type)
- Convert all assertions to ⚠ where appropriate
- Error handling guide

**Week 3-4: Pattern Matching**
- THE GAME CHANGER
- 10x usability improvement
- Foundation for everything else

**Week 5: Strings**
- Required for real programs

**Week 6: I/O**
- Interact with the world

**Week 7: Standard Library**
- MVP COMPLETE! 🎉

---

## Risks and Mitigation

**Risk:** Tests take longer than expected
**Mitigation:** Start with most critical tests, add more later

**Risk:** Division fix requires major changes
**Mitigation:** Add modulo primitive instead if needed

**Risk:** Error handling changes break existing tests
**Mitigation:** Update tests as we go, maintain compatibility

**Risk:** Scope creep
**Mitigation:** Stick to plan, defer nice-to-haves

---

## Immediate Action Items

**RIGHT NOW (next 30 minutes):**
1. ✅ Create this plan document
2. ⏳ Create tests/comprehensive_lists.test skeleton
3. ⏳ Start writing basic list operation tests

**NEXT (1-2 hours):**
1. ⏳ Write all list construction tests
2. ⏳ Write list traversal tests
3. ⏳ Run tests, verify current implementation

**AFTER THAT (2-3 hours):**
1. ⏳ Write higher-order function tests
2. ⏳ Write memory management tests
3. ⏳ Identify any failures

---

**Status:** READY TO EXECUTE
**Start Time:** 2026-01-27
**Expected Completion:** 2026-01-30
**Next Milestone:** Week 3 Pattern Matching

**Let's build a bulletproof foundation!** 🚀
