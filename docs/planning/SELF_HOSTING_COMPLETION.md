---
Status: CURRENT
Created: 2026-01-28
Updated: 2026-01-28
Purpose: Roadmap for completing self-hosting evaluator (59% → 100%)
---

# Self-Hosting Evaluator Completion Roadmap

## Current State (Day 53/54 Extended)

**Status:** 59% complete (13/22 tests passing)
**What Works:** Pure lambda calculus evaluation
**What Doesn't:** Cannot call C primitives

## Goal

**Objective:** Complete self-hosting evaluator to handle full Guage language
**Target:** 22/22 tests passing (100%)
**Estimated Time:** 3-4 hours over 1-2 sessions

## Architectural Challenge

**Problem:** Guage evaluator (pure Guage code) cannot call C primitives

**Root Cause:**
- Evaluator lives in `bootstrap/stdlib/eval.scm` (Guage code)
- Primitives live in `bootstrap/primitives.c` (C code)
- No bridge exists between Guage and C function calls
- When evaluator gets a primitive from environment, it's opaque

**Current Behavior:**
```scheme
(≔ env (((env-extend (env-empty)) :⊕) ⊕))  ; Store primitive
((eval (⌜ (⊕ #2 #3))) env)                 ; Try to call it
; → ⚠:cannot-apply-primitive:<builtin>
```

## Solution Design

### Phase 1: Expose Primitive Type (30 minutes)

**Goal:** Add `primitive?` predicate to check if value is a primitive

**Implementation:**

1. Add C function in `bootstrap/primitives.c`:
```c
// Check if cell is a primitive function
Cell* prim_is_primitive(Cell* args) {
    if (!cell_is_pair(args)) return error_create(symbol_create("type-error"));
    Cell* value = car(args);
    return value->type == CELL_PRIMITIVE ? cell_true() : cell_false();
}
```

2. Register in `init_primitives()`:
```c
env_define(env, symbol_create("primitive?"), 
           primitive_create(prim_is_primitive, "primitive?"));
```

3. Test from Guage:
```scheme
(primitive? ⊕)   ; → #t
(primitive? #42) ; → #f
```

**Files to modify:**
- `bootstrap/primitives.c` (~15 lines)

### Phase 2: Primitive Metadata (1 hour)

**Goal:** Store primitive name and arity with primitive value

**Current Problem:**
- Primitives are opaque `<builtin>` values
- No way to know which primitive it is
- Cannot extract name or arity

**Solution 1: Primitive Wrapper Structure**

Wrap primitives in a structure when storing in environment:
```scheme
; Instead of:
(((env-extend env) :⊕) ⊕)

; Use:
(((env-extend env) :⊕) (⟨⟩ :primitive (⟨⟩ :⊕ (⟨⟩ ⊕ ∅))))
```

**Pros:** Pure Guage solution
**Cons:** Ugly, breaks existing code

**Solution 2: Primitive with Name (Recommended)**

Add name field to primitive in C:
```c
// In bootstrap/cell.h
typedef struct {
    Cell* (*fn)(Cell*);
    const char* name;  // NEW: store name
} Primitive;

// In bootstrap/primitives.c
Cell* primitive_create_named(Cell* (*fn)(Cell*), const char* name) {
    Cell* cell = cell_alloc();
    cell->type = CELL_PRIMITIVE;
    cell->value.primitive.fn = fn;
    cell->value.primitive.name = strdup(name);  // NEW
    return cell;
}

Cell* prim_primitive_name(Cell* args) {
    Cell* prim = car(args);
    if (prim->type != CELL_PRIMITIVE) return error_create(...);
    return string_create(prim->value.primitive.name);
}
```

**Files to modify:**
- `bootstrap/cell.h` (add name field)
- `bootstrap/cell.c` (update primitive_create, add primitive_name)
- `bootstrap/primitives.c` (update all registrations)

### Phase 3: Primitive Call Bridge (1 hour)

**Goal:** Allow Guage code to call C primitives by name

**Implementation:**

Add `call-primitive` function in C:
```c
Cell* prim_call_primitive(Cell* args, EvalContext* ctx) {
    // Args: (name arg-list)
    Cell* name = car(args);
    Cell* arg_list = car(cdr(args));
    
    // Look up primitive by name
    Cell* prim = eval_lookup(ctx, symbol_from_string(name));
    if (prim->type != CELL_PRIMITIVE) {
        return error_create(symbol_create("not-a-primitive"));
    }
    
    // Call the primitive with args
    return prim->value.primitive.fn(arg_list);
}
```

Register as `call-primitive` or `%call-prim`.

**Files to modify:**
- `bootstrap/primitives.c` (~30 lines)

### Phase 4: Update Guage Evaluator (30 minutes)

**Goal:** Use primitive call bridge in `apply-fn`

**Implementation:**

Modify `bootstrap/stdlib/eval.scm`:
```scheme
(≔ apply-fn (λ (fn) (λ (args) (λ (env)
  (? (:? fn)
     ((env-lookup env) fn)
     (? (⟨⟩? fn)
        (? (≡ (◁ fn) :closure)
           ; ... existing closure logic ...
           (⚠ :not-a-closure fn))
        ; NEW: Handle primitives
        (? (primitive? fn)
           (call-primitive fn args)  ; Call through bridge
           (⚠ :not-a-function fn))))))))
```

**Files to modify:**
- `bootstrap/stdlib/eval.scm` (~10 lines)

### Phase 5: Test and Debug (1 hour)

**Goal:** Get all 22 tests passing

**Process:**
1. Run `./guage < bootstrap/tests/test_eval.test`
2. Check which tests now pass
3. Debug failures one by one
4. Handle edge cases (arity checking, error propagation)

**Expected new passing tests:**
- test 12: eval-add (uses ⊕)
- test 13: eval-multiply (uses ⊗)
- test 14: eval-nested-arithmetic (uses ⊕, ⊗)
- test 17: eval-if-comparison (uses >)
- test 18: eval-lambda-capture (uses ⊕)
- test 19: eval-lambda-two-params (uses ⟨⟩)
- test 20: eval-higher-order (uses ⊕)

**Common issues to watch:**
- Arity mismatches (primitives expect pair, not list)
- Error propagation (primitive returns error)
- Environment lookup (correct environment passed)
- Argument evaluation (already evaluated before call)

## Session-by-Session Plan

### Session 1: Infrastructure (1.5-2 hours)

**Tasks:**
1. ✅ Phase 1: Add `primitive?` predicate (30 min)
2. ✅ Phase 2: Add primitive metadata (1 hour)
3. ⏳ Test primitive detection works

**Success Criteria:**
- `(primitive? ⊕)` returns `#t`
- `(primitive-name ⊕)` returns `"⊕"` (if implemented)
- No regressions in main test suite

**Deliverables:**
- Modified `bootstrap/cell.h`, `bootstrap/cell.c`, `bootstrap/primitives.c`
- New tests for primitive predicates

### Session 2: Bridge and Integration (1.5-2 hours)

**Tasks:**
1. ✅ Phase 3: Implement `call-primitive` (1 hour)
2. ✅ Phase 4: Update Guage evaluator (30 min)
3. ✅ Phase 5: Test and debug (1 hour)

**Success Criteria:**
- All 22 eval tests passing (100%)
- No regressions in main tests
- Clean error messages for edge cases

**Deliverables:**
- Modified `bootstrap/primitives.c`, `bootstrap/stdlib/eval.scm`
- Full test suite passing
- Documentation updated

## Alternative: Minimal Bridge Approach

**If full implementation is too complex, try this minimal version:**

### Minimal Phase: Hardcoded Primitive Dispatch

Instead of generic bridge, hardcode common primitives in evaluator:

```scheme
(≔ call-builtin (λ (name) (λ (args)
  (? (≡ name :⊕)
     ; Hardcode addition
     (%native-add (◁ args) (◁ (▷ args)))
     (? (≡ name :⊗)
        ; Hardcode multiply
        (%native-mul (◁ args) (◁ (▷ args)))
        (? (≡ name :⟨⟩)
           ; Hardcode cons
           (%native-cons (◁ args) (◁ (▷ args)))
           (⚠ :unsupported-primitive name)))))))
```

Add minimal native ops in C:
```c
Cell* prim_native_add(Cell* args) {
    double a = car(args)->value.number;
    double b = car(cdr(args))->value.number;
    return number_create(a + b);
}
// ... similar for mul, cons, car, cdr, etc.
```

**Pros:**
- Simpler implementation
- No primitive metadata needed
- Faster (no lookup overhead)

**Cons:**
- Not extensible
- Must add each primitive manually
- Ugly code duplication

**Estimated Time:** 2 hours (simpler but limited)

## Decision Matrix

| Approach | Time | Extensibility | Completeness | Complexity |
|----------|------|---------------|--------------|------------|
| Full Bridge | 3-4h | High | 100% | Medium |
| Minimal Hardcode | 2h | Low | Good enough | Low |
| Declare Victory | 0h | N/A | 59% | None |

**Recommendation:** Try Full Bridge first. If blocked, fall back to Minimal. If not high-priority, Declare Victory.

## Success Metrics

**Goal:** 22/22 tests passing (100%)

**Intermediate Milestones:**
- After Phase 1-2: Can detect primitives ✅
- After Phase 3: Can call simple primitives (⊕, ⊗)
- After Phase 4: Can call all primitives from evaluator
- After Phase 5: All tests passing ✅

**Quality Metrics:**
- No regressions in main test suite (52/55)
- Clean error messages (no crashes)
- Documented limitation (what primitives work)
- Maintainable code (not too hacky)

## Risks and Mitigations

**Risk 1: Primitive calling is too complex**
- Mitigation: Start with minimal hardcoded approach
- Fallback: Document limitation, move on

**Risk 2: Arity mismatches cause crashes**
- Mitigation: Add arity checking in bridge
- Test with various argument counts

**Risk 3: Error propagation breaks**
- Mitigation: Test error returns from primitives
- Handle errors explicitly in evaluator

**Risk 4: Performance issues**
- Mitigation: Don't worry about perf in v1
- Can optimize later if needed

## Next Steps

**When Starting Next Session:**

1. **Read context:**
   - `SESSION_END_DAY_53_54_EXTENDED.md` (session notes)
   - This file (implementation plan)
   - `bootstrap/stdlib/eval.scm` (current evaluator)

2. **Verify current state:**
   ```bash
   ./guage < bootstrap/tests/test_eval.test | grep -E "(PASS|FAIL)"
   # Should show 13 PASS, 9 FAIL
   ```

3. **Choose approach:**
   - Full Bridge (recommended) - 3-4 hours
   - Minimal Hardcode - 2 hours
   - Declare Victory - 0 hours, move on

4. **Start implementation:**
   - Follow phase-by-phase plan
   - Test after each phase
   - Document decisions made

## Questions to Answer

**Before starting:**
- [ ] Is 100% self-hosting evaluator high-priority?
- [ ] Is time better spent on other features?
- [ ] Do we need full primitive support or just enough?

**During implementation:**
- [ ] Should primitives be wrapped or stored directly?
- [ ] Should we support all primitives or just core ones?
- [ ] How should arity mismatches be handled?
- [ ] Should we add primitive documentation to evaluator?

**After completion:**
- [ ] Can we use this for meta-circular interpreter?
- [ ] Does this enable new language features?
- [ ] Is performance acceptable?
- [ ] Should this become the default evaluator?

---

**Status:** Ready for implementation
**Next Session:** Choose approach and start Phase 1
**Estimated Completion:** 1-2 sessions (3-4 hours total)
