---
Status: ACTIVE
Created: 2026-01-27
Purpose: Implementation plan for ⌞ (eval) primitive
---

# ⌞ (eval) Primitive Implementation Plan

## Executive Summary

**Goal:** Implement ⌞ (eval) to enable automatic test execution and metaprogramming.
**Duration:** ~2-3 hours
**Impact:** CRITICAL - unlocks test automation and metaprogramming foundation

## Current State

**Architecture discovered:**
- `eval(ctx, expr)` - Public interface in eval.c
- `eval_internal(ctx, env, expr)` - Core evaluator (lines 917-1116)
- `prim_eval(args)` - Currently stub (returns args)

**What eval_internal handles:**
- ✅ Self-evaluating: numbers, booleans, nil, keywords (:symbol)
- ✅ Variable lookup: regular symbols
- ✅ Special forms: ⌜, ≔, λ, ?
- ✅ Function application: evaluate function + args, then apply

## Implementation Strategy

### Phase 1: Wire up prim_eval (30 min)

**Goal:** Make prim_eval use existing eval infrastructure

**Current code (primitives.c:94-97):**
```c
Cell* prim_eval(Cell* args) {
    /* Eval is handled by the evaluator */
    return args;
}
```

**New implementation:**
```c
Cell* prim_eval(Cell* args) {
    /* Get the expression to evaluate */
    Cell* expr = arg1(args);

    /* Get current eval context */
    EvalContext* ctx = eval_get_current_context();
    if (!ctx) {
        return cell_error("no-context", expr);
    }

    /* Evaluate the expression in current environment */
    return eval(ctx, expr);
}
```

**Tests:**
```scheme
; Self-evaluating
(⌞ (⌜ #42))        ; → #42
(⌞ (⌜ #t))         ; → #t
(⌞ (⌜ ∅))          ; → ∅
(⌞ (⌜ :test))      ; → :test

; Primitives
(⌞ (⌜ (⊕ #1 #2)))  ; → #3
(⌞ (⌜ (≡ #1 #1)))  ; → #t
```

### Phase 2: Test with primitives (30 min)

**Goal:** Verify arithmetic, comparisons, logic work

**Tests:**
```scheme
; Arithmetic
(⌞ (⌜ (⊕ #5 #10)))              ; → #15
(⌞ (⌜ (⊗ #3 #4)))               ; → #12
(⌞ (⌜ (⊘ #10 #2)))              ; → #5

; Comparisons
(⌞ (⌜ (< #5 #10)))              ; → #t
(⌞ (⌜ (≡ #5 #5)))               ; → #t

; Logic
(⌞ (⌜ (∧ #t #t)))               ; → #t
(⌞ (⌜ (∨ #f #t)))               ; → #t

; Lists
(⌞ (⌜ (◁ (⟨⟩ #1 #2))))          ; → #1
(⌞ (⌜ (▷ (⟨⟩ #1 #2))))          ; → #2
```

### Phase 3: Test conditionals (30 min)

**Goal:** Verify ? (conditional) works

**Tests:**
```scheme
; Simple conditionals
(⌞ (⌜ (? #t #1 #2)))            ; → #1
(⌞ (⌜ (? #f #1 #2)))            ; → #2

; With evaluation
(⌞ (⌜ (? (≡ #1 #1) #42 #0)))   ; → #42
(⌞ (⌜ (? (< #5 #10) :yes :no))) ; → :yes
```

### Phase 4: Test with variables (30 min)

**Goal:** Verify variable lookup works

**Tests:**
```scheme
; Global variables
(≔ x #42)
(⌞ (⌜ x))                       ; → #42
(⌞ (⌜ (⊕ x #1)))                ; → #43

; Multiple variables
(≔ y #10)
(⌞ (⌜ (⊗ x y)))                 ; → #420
```

### Phase 5: Test with lambdas (1 hour)

**Goal:** Verify lambda creation and application

**Tests:**
```scheme
; Lambda creation
(⌞ (⌜ (λ (x) x)))               ; → lambda

; Lambda application
(⌞ (⌜ ((λ (x) (⊕ x #1)) #5)))   ; → #6

; Higher-order
(≔ twice (λ (f) (λ (x) (f (f x)))))
(≔ inc (λ (x) (⊕ x #1)))
(⌞ (⌜ ((twice inc) #5)))        ; → #7
```

### Phase 6: Test with auto-generated tests (30 min)

**Goal:** Verify eval works with test infrastructure

**Tests:**
```scheme
; Generate tests
(≔ tests (⌂⊨ (⌜ ⊕)))

; Evaluate first test
(≔ test1 (◁ tests))
(⌞ test1)                       ; → #t or #f

; Run all tests
(≔ run-test (λ (test) (⌞ test)))
```

## Technical Considerations

### 1. Environment Handling

**Question:** What environment should eval use?
**Answer:** Current evaluation context (ctx->env)

**Why:** Eval should see all currently defined variables.

### 2. Error Propagation

**Question:** How should eval handle errors?
**Answer:** Return error cells (already done by eval_internal)

**Example:**
```scheme
(⌞ (⌜ (⊕ #1 :not-a-number)))    ; → ⚠:type-error
(⌞ (⌜ undefined-var))            ; → ⚠:undefined-variable
```

### 3. Nested Quotes

**Question:** How does nested quote/eval work?
**Answer:** Each level adds/removes one quote

**Example:**
```scheme
(⌜ (⌞ (⌜ #42)))                  ; → (⌞ (⌜ #42)) (quoted)
(⌞ (⌜ (⌞ (⌜ #42))))              ; → #42 (eval removes quote)
```

### 4. Quote Semantics

**Current behavior:**
- `⌜` prevents evaluation, returns expression as-is
- Expression remains quoted (data, not code)

**Eval behavior:**
- `⌞` takes quoted expression
- Evaluates it in current environment
- Returns evaluated result

**Cycle:**
```scheme
#42
  → (⌜ #42)        ; Quote: code → data
  → (⌞ (⌜ #42))    ; Eval: data → code → result
  → #42            ; Result
```

## Implementation Steps

**Step 1:** Modify prim_eval in primitives.c
**Step 2:** Create tests/test_eval.scm
**Step 3:** Test Phase 1 (self-evaluating)
**Step 4:** Test Phase 2 (primitives)
**Step 5:** Test Phase 3 (conditionals)
**Step 6:** Test Phase 4 (variables)
**Step 7:** Test Phase 5 (lambdas)
**Step 8:** Test Phase 6 (auto-generated tests)
**Step 9:** Update SPEC.md (mark ⌞ as ✅ DONE)
**Step 10:** Update SESSION_HANDOFF.md
**Step 11:** Commit

## Success Criteria

- ✅ All 6 phases of tests pass
- ✅ Auto-generated tests can be executed automatically
- ✅ No memory leaks
- ✅ Error handling works correctly
- ✅ Documentation updated

## Risk Assessment

**Low Risk:**
- Infrastructure already exists (eval_internal)
- Just wiring up existing functionality
- Incremental testing at each phase

**Medium Risk:**
- Environment handling might be tricky
- Error propagation edge cases
- Nested quote/eval semantics

**Mitigation:**
- Test incrementally (self-eval → primitives → vars → lambdas)
- Clear error messages
- Document quote/eval semantics

## Expected Timeline

- Phase 1: 30 min (wire up)
- Phase 2: 30 min (primitives)
- Phase 3: 30 min (conditionals)
- Phase 4: 30 min (variables)
- Phase 5: 1 hour (lambdas)
- Phase 6: 30 min (auto-tests)
- Documentation: 30 min
- **Total: ~4 hours**

## Next Steps After Completion

1. **Enable automatic test execution** - Run all 110+ auto-generated tests
2. **Create test runner** - Evaluate all tests with ⌞
3. **Metaprogramming foundation** - Enable code-as-data transformations
4. **Self-hosting preparation** - Step toward Guage-in-Guage

---

**Status:** Ready to implement!
