---
Status: CURRENT
Created: 2026-01-28
Updated: 2026-01-28
Purpose: Track remaining work to make trampoline evaluator production-ready
Priority: LOW (deferred in favor of language features)
---

# Trampoline Evaluator: Remaining Work

## Status Summary

**Current State:** Proof of concept complete
- ✅ Basic cases work (arithmetic, lambdas, simple recursion)
- ✅ 21/21 C unit tests passing
- ❌ Complex stdlib code causes segfaults
- ❌ Only 11/33 Guage tests passing with trampoline

**Goal:** Production-ready trampoline evaluator
- All 33 Guage tests passing
- Stdlib loads without crashes
- Performance comparable to recursive evaluator
- Memory safe (no leaks, no segfaults)

## Phase 3E: Production Hardening (Deferred)

### Task 1: Debug Stdlib Crashes (Est: 1-2 days)

**Priority:** HIGH (if we pursue trampoline)
**Blocked By:** None
**Estimated:** 8-16 hours

**Steps:**
1. [ ] Add verbose logging to trampoline
   - Log every frame push/pop
   - Log frame state transitions
   - Log environment changes
   - Add log level flag: `-DTRAMPOLINE_DEBUG=1`

2. [ ] Enable Address Sanitizer
   ```bash
   make clean
   make CFLAGS="-fsanitize=address -DUSE_TRAMPOLINE=1"
   ```

3. [ ] Find minimal failing case
   - Start with smallest stdlib module
   - Bisect to find first failing expression
   - Create isolated test case

4. [ ] Investigate frame lifecycle
   - Are frames being destroyed too early?
   - Are environments being released prematurely?
   - Are there circular references?

5. [ ] Fix segfaults
   - Likely issues:
     - Memory management bug
     - Frame state machine bug
     - Environment capture bug

**Success Criteria:**
- `(⋘ "stdlib/list.scm")` loads without crashes
- At least 20/33 Guage tests passing

### Task 2: Add Integration Tests (Est: 1 day)

**Priority:** MEDIUM
**Blocked By:** Task 1 (need stable trampoline)
**Estimated:** 4-8 hours

**Test Coverage Needed:**

1. [ ] Global definitions
   ```scheme
   (≔ x #42)
   (≔ f (λ (n) (⊕ n x)))  ; closure over global
   (f #10)                ; should be #52
   ```

2. [ ] Nested lambdas
   ```scheme
   (≔ curry (λ (f) (λ (x) (λ (y) ((f x) y)))))
   (((curry ⊕) #1) #2)    ; should be #3
   ```

3. [ ] Mutual recursion
   ```scheme
   (≔ even? (λ (n) (? (≡ n #0) #t (odd? (⊖ n #1)))))
   (≔ odd? (λ (n) (? (≡ n #0) #f (even? (⊖ n #1)))))
   (even? #4)             ; should be #t
   ```

4. [ ] Stdlib functions
   ```scheme
   (⋘ "stdlib/list.scm")
   ((↦ (λ (x) (⊕ x #1))) (⟨⟩ #1 (⟨⟩ #2 ∅)))  ; map
   ```

5. [ ] Macros (if trampoline supports them)
   ```scheme
   (⋘ "stdlib/macros.scm")
   ; Test macro expansion with trampoline
   ```

**Success Criteria:**
- 10+ new integration tests
- All integration tests passing
- Coverage of all special forms

### Task 3: Performance Profiling (Est: 1 day)

**Priority:** LOW
**Blocked By:** Tasks 1-2 (need stable, correct implementation)
**Estimated:** 4-8 hours

**Measurements:**

1. [ ] Benchmark simple operations
   ```bash
   # Recursive evaluator
   time echo "(! #10)" | ./guage

   # Trampoline evaluator
   time echo "(! #10)" | ./guage  # with USE_TRAMPOLINE=1
   ```

2. [ ] Profile with instruments (macOS)
   ```bash
   instruments -t "Time Profiler" ./guage
   ```

3. [ ] Measure memory usage
   - Stack usage (should be constant for trampoline)
   - Heap usage (frame allocations)
   - Reference counting overhead

4. [ ] Identify hot paths
   - Frame allocation/deallocation
   - Handler dispatch (switch statement)
   - Environment operations

5. [ ] Optimize critical paths
   - Frame pooling (reuse frames)
   - Inline small handlers
   - Optimize environment lookup

**Success Criteria:**
- Trampoline within 2x of recursive performance
- Memory usage documented
- Optimization opportunities identified

### Task 4: Memory Safety (Est: 0.5 days)

**Priority:** HIGH (if we pursue trampoline)
**Blocked By:** Task 1
**Estimated:** 2-4 hours

**Checks:**

1. [ ] Run with Address Sanitizer
   - No memory leaks
   - No use-after-free
   - No double-free

2. [ ] Run with Valgrind (if on Linux)
   ```bash
   valgrind --leak-check=full ./guage < test.scm
   ```

3. [ ] Stress test
   - Deep recursion (1000+ levels)
   - Large lists (10000+ elements)
   - Many allocations

4. [ ] Reference counting audit
   - All cell_retain() have matching cell_release()
   - Frames properly release all fields
   - No circular references

**Success Criteria:**
- Zero memory leaks
- All sanitizer checks pass
- Stress tests complete successfully

## Estimated Timeline

**Total Effort:** 3-4 days (24-32 hours)

**Breakdown:**
- Day 1: Debug stdlib crashes (8-16 hours)
- Day 2: Add integration tests (4-8 hours)
- Day 3: Performance profiling (4-8 hours)
- Day 4: Memory safety checks (2-4 hours)

**Confidence:** Medium
- Could be faster if issues are simple
- Could be slower if fundamental design issues

## Decision Points

### Should We Complete Trampoline?

**Arguments FOR:**
1. ✅ Unlimited recursion depth (no more stack overflow)
2. ✅ Foundation for advanced features:
   - Call/cc (continuations)
   - Coroutines
   - Time-travel debugging
3. ✅ Industry-standard architecture for interpreters
4. ✅ Learning opportunity

**Arguments AGAINST:**
1. ❌ 3-4 days of effort (~10-15% of project time so far)
2. ❌ Current 32MB stack is sufficient for foreseeable use
3. ❌ No user complaints about stack depth
4. ❌ Higher priority: language features, stdlib, documentation

### Recommendation: DEFER

**Current Priority:** Continue with language features
- Math library
- Result/Either type
- Fix remaining test failures
- Pattern matching improvements

**Revisit When:**
1. User reports stack overflow issues
2. We need continuations (call/cc)
3. We need coroutines
4. We have 1-2 weeks for polish

## How to Resume This Work

### Step 1: Set Up Environment

```bash
# Create debug build
make clean
make CFLAGS="-g -O0 -fsanitize=address -DUSE_TRAMPOLINE=1 -DTRAMPOLINE_DEBUG=1"

# Run tests
./bootstrap/guage < tests/recursion.test  # Should work
./bootstrap/guage < tests/test_sort.test  # Probably crashes
```

### Step 2: Find Minimal Failing Case

```bash
# Create test file
cat > /tmp/test.scm <<'EOF'
(⋘ "stdlib/list.scm")
EOF

# Run with sanitizer
./bootstrap/guage < /tmp/test.scm 2>&1 | tee debug.log

# Check for:
# - Stack trace
# - Memory error location
# - Frame state at crash
```

### Step 3: Add Logging

Edit `bootstrap/trampoline.c`:
```c
#ifdef TRAMPOLINE_DEBUG
#define DEBUG_LOG(...) fprintf(stderr, "[TRAMP] " __VA_ARGS__)
#else
#define DEBUG_LOG(...)
#endif

void trampoline_loop(EvalStack* stack) {
    while (!stack_is_empty(stack)) {
        StackFrame* frame = stack_pop(stack);
        DEBUG_LOG("Frame state=%d, expr=%p, env=%p\n",
                  frame->state, frame->expr, frame->env);
        // ...
    }
}
```

### Step 4: Bisect to Find Bug

1. Add logging to all handlers
2. Run failing test
3. Find last successful frame
4. Inspect state at crash
5. Form hypothesis
6. Fix and test

## Related Documents

- `docs/archive/2026-01/sessions/DAY_49_TRAMPOLINE_PHASE_3D.md` - Phase 3D completion
- `docs/archive/2026-01/plans/TRAMPOLINE_EVAL_PLAN.md` - Original design
- `bootstrap/trampoline.{c,h}` - Implementation
- `bootstrap/test_trampoline.c` - C unit tests

## Updates Log

- 2026-01-28: Created after Phase 3D completion, work deferred

---

**Status:** Work deferred in favor of language features
**Estimated Effort:** 3-4 days when resumed
**Priority:** LOW (revisit when needed)
