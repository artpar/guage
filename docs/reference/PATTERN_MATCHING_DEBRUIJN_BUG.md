---
Status: REFERENCE
Created: 2026-01-28
Updated: 2026-01-28
Purpose: Document pattern matching bug with De Bruijn indices in nested lambdas
---

# Pattern Matching Bug: De Bruijn Indices in Nested Lambdas

## Problem Summary

**Issue:** Pattern matching (`∇`) fails with `:no-match` errors when used inside nested lambdas with curried functions.

**Discovered:** Day 56 (2026-01-28) during Result/Either type implementation

**Impact:** HIGH - Affects all ADT operations in curried functions

## Reproduction

### Failing Example

```scheme
;; This FAILS with :no-match:#0
(≔ map (λ (f) (λ (r)
  (∇ r (⌜ (((⊚ :Result :Ok v) (ok (f v)))
           ((⊚ :Result :Err e) (err e))))))))

;; Test
((map (λ (x) (⊗ x #2))) (ok #42))
;; → ⚠:no-match:#0
```

### Working Workaround

```scheme
;; This WORKS using ⊚? and ⊚→
(≔ map (λ (f) (λ (r)
  (? (ok? r)
     (ok (f (⊚→ r :value)))
     r))))

;; Test
((map (λ (x) (⊗ x #2))) (ok #42))
;; → ⊚[::Result ::Ok ⟨⟨::value #84⟩ ∅⟩]  ✓
```

### Pattern That Works

```scheme
;; Direct pattern matching (not in nested lambda) WORKS
(≔ unwrap (λ (r)
  (∇ r (⌜ (((⊚ :Result :Ok v) v)
           ((⊚ :Result :Err e) (⚠ :unwrap-err e)))))))

;; Test
(unwrap (ok #42))
;; → #42  ✓
```

## Root Cause Analysis

### Hypothesis

When `∇` is inside nested lambdas:
1. Parameter references (like `r`) get converted to De Bruijn indices
2. The pattern matcher receives index `#0` instead of the actual value
3. Pattern matching tries to match the **index** instead of the **value**
4. Match fails because `#0` doesn't match any pattern

### Evidence

```scheme
;; In this code:
(λ (f) (λ (r) (∇ r ...)))
         ^      ^
         |      De Bruijn index 0 (refers to r)
         De Bruijn index 1 (refers to f)

;; When evaluating (∇ r ...), the system sees:
(∇ 0 ...)  ; Index, not value!

;; Pattern matcher tries to match:
0 vs (⊚ :Result :Ok v)  → no match
0 vs (⊚ :Result :Err e) → no match
→ :no-match:#0
```

### Why Workaround Works

```scheme
;; ⊚? and ⊚→ properly handle De Bruijn indices:
(⊚? r :Result :Ok)  ; Dereferences index 0 to get value, then checks type
(⊚→ r :value)       ; Dereferences index 0 to get value, then extracts field
```

## Technical Details

### Where the Bug Occurs

**File:** `bootstrap/pattern.c` (suspected)

**Function:** Likely in the pattern matching evaluator

**Issue:** De Bruijn indices are not dereferenced before pattern matching begins

### Expected Behavior

1. Receive De Bruijn index as first argument to `∇`
2. **Dereference index to get actual value**
3. Perform pattern matching on the value
4. Return matched result

### Actual Behavior

1. Receive De Bruijn index as first argument to `∇`
2. **Skip dereferencing step** ❌
3. Try to match the **index number** against patterns
4. Fail with `:no-match:#N`

## Fix Strategy

### Step 1: Locate the Bug

1. Examine `bootstrap/pattern.c` - pattern matching implementation
2. Find where `∇` evaluates its first argument
3. Check if De Bruijn indices are dereferenced

### Step 2: Add Dereferencing

**Before pattern matching:**
```c
// Current (suspected):
Cell* expr = args->car;  // Gets index, not value

// Should be:
Cell* expr = eval_internal(args->car, env);  // Dereferences index
```

### Step 3: Test the Fix

1. Revert Result type to use `∇`-based implementation
2. Run Result tests: `./bootstrap/guage < bootstrap/tests/result.test`
3. Verify all 44 tests pass
4. Run full test suite: `make test`
5. Verify no regressions

### Step 4: Add Regression Tests

Add tests specifically for `∇` in nested lambdas:
```scheme
;; Test pattern matching in curried function
(≔ test-curry (λ (x) (λ (y)
  (∇ y (⌜ ((#1 :one) (#2 :two) (_ :other)))))))

(⊨ :nested-pattern-match-1 :one ((test-curry #99) #1))
(⊨ :nested-pattern-match-2 :two ((test-curry #99) #2))
(⊨ :nested-pattern-match-3 :other ((test-curry #99) #42))
```

## Current Workarounds

### For ADT Operations

Use `⊚?` (type check) and `⊚→` (field access) instead of `∇`:

```scheme
;; Instead of:
(∇ value (⌜ (((⊚ :Type :Variant field) (use field)))))

;; Use:
(? (⊚? value :Type :Variant)
   (use (⊚→ value :field-name))
   ...)
```

### For Simple Values

Pattern matching works fine for simple values and in non-nested contexts:

```scheme
;; This works:
(≔ safe-head (λ (lst)
  (∇ lst (⌜ ((∅ (err :empty))
             ((⟨⟩ h t) (ok h)))))))
```

## Impact on Codebase

### Files Affected

1. **`bootstrap/stdlib/result.scm`** - Had to use `⊚?`/`⊚→` workaround
2. **Any future ADT libraries** - Will need similar workarounds
3. **Pattern matching examples** - May mislead users

### Benefits of Fixing

1. **Cleaner code** - `∇` is more elegant than `?` chains
2. **Better examples** - Can showcase pattern matching properly
3. **Performance** - `∇` may be optimized better than manual checks
4. **Correctness** - Language should work as designed

## Related Issues

### Pattern Matching in Stdlib

Check these files for potential similar issues:
- `bootstrap/stdlib/option.scm` - May have workarounds
- `bootstrap/stdlib/list.scm` - May avoid `∇` in nested contexts

### De Bruijn Index Handling

Check these files for correct dereferencing examples:
- `bootstrap/eval.c` - How does evaluation handle indices?
- `bootstrap/debruijn.c` - How are indices converted?

## Testing Checklist

When fixing this bug, verify:

- [ ] All 44 Result tests still pass
- [ ] Result implementation can be simplified to use `∇`
- [ ] No regressions in main test suite (56/57 → 56/57)
- [ ] Pattern matching works in deeply nested lambdas (3+ levels)
- [ ] Pattern matching works with multiple parameters
- [ ] Pattern matching works with closures

## References

- **Discovered in:** `docs/archive/2026-01/sessions/SESSION_END_DAY_56.md`
- **Original implementation:** `bootstrap/stdlib/result.scm` (uses workaround)
- **Test suite:** `bootstrap/tests/result.test` (44 tests, all passing)

---

**Status:** UNRESOLVED - Workaround in place, proper fix needed
**Priority:** HIGH - Affects language correctness
**Estimated Fix Time:** 2-3 hours (investigation + implementation + testing)
