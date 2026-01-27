---
Status: CURRENT
Created: 2026-01-27
Updated: 2026-01-27
Purpose: Day 32+ implementation plan for stdlib expansion
---

# Day 32+ Plan: Standard Library Expansion & Organization

## Executive Summary

**Goal:** Expand standard library with high-value functions and create organized, documented stdlib
**Duration:** 3-5 days (Days 32-36)
**Prerequisites:** Pattern matching complete ✅, Module system complete ✅, String library complete ✅
**Impact:** Production-ready standard library for real-world programs

---

## Current State Analysis

### What We Have (Day 31 Complete)

**Stdlib Modules (5):**
1. `stdlib/list.scm` - 15 core functions (map, filter, fold, zip, etc.)
2. `stdlib/list_extended.scm` - 6 functions (find, partition, concat, etc.)
3. `stdlib/option.scm` - 22 functions (Option/Result types)
4. `stdlib/math.scm` - 6 functions (sum, product, min/max)
5. `stdlib/string.scm` - 5 functions (join, contains, repeat, whitespace check)

**Total:** 54 stdlib functions, all tested and working!

**Infrastructure Complete:**
- ✅ Pattern matching with exhaustiveness checking
- ✅ Module system (load, registry, provenance, dependencies)
- ✅ String primitives (9 primitives)
- ✅ I/O primitives (8 primitives)
- ✅ Testing framework (850+ tests passing)

### What's Missing

**Deferred String Functions (need more infrastructure):**
- `≈⊞` (split) - Complex character-by-character recursion
- `≈⊳`/`≈⊴`/`≈⊲` (trim functions) - Character iteration patterns
- `≈↑`/`≈↓` (case conversion) - Character arithmetic primitives

**List Utilities (CAN implement now):**
- `take-while` / `drop-while` - Conditional slicing
- `group-by` - Grouping by key function
- `flatten` - Flatten nested lists
- `distinct` / `unique` - Remove duplicates
- `span` / `break` - Split at first failure
- `nth-or` - Safe indexed access with default
- `cycle` - Infinite list cycling (lazy evaluation?)

**Numeric Utilities (CAN implement now):**
- `abs` - Absolute value
- `clamp` - Clamp value between bounds
- `sign` - Return -1, 0, or 1
- `even?` / `odd?` - Parity predicates
- `gcd` / `lcm` - Number theory basics
- `range-list` - List of numbers (may exist?)

**Function Combinators (CAN implement now):**
- `compose` - Function composition (f ∘ g)
- `pipe` - Reverse composition (g |> f)
- `partial` - Partial application helper
- `flip` - Flip argument order
- `const` - Constant function
- `identity` - Already exists as pattern, formalize?

**Predicate Combinators (CAN implement now):**
- `conjoin` / `and-pred` - Combine predicates with AND
- `disjoin` / `or-pred` - Combine predicates with OR
- `negate` - Negate predicate
- `complement` - Logical complement

**Organization Needs:**
- Single "prelude" module for easy imports
- Better documentation (API reference)
- Usage examples and patterns
- Module interdependencies clear

---

## Priority Assessment

### Priority 1: HIGH VALUE + NO NEW INFRASTRUCTURE (Days 32-34)

**These can be implemented RIGHT NOW with existing primitives:**

**A. List Utilities** (Day 32 - 6 hours)
- `take-while` - O(n), useful for parsing
- `drop-while` - O(n), complement of take-while
- `span` - O(n), combination of take-while/drop-while
- `break` - O(n), span with negated predicate
- `group-by` - O(n), essential for data processing
- `flatten` - O(n*m), useful for nested structures
- `distinct` - O(n²) simple or O(n log n) with sorting
- `nth-or` - O(n), safe indexed access

**Estimated:** 8 functions, 40+ tests

**B. Numeric Utilities** (Day 33 - 4 hours)
- `abs` - O(1), basic math
- `clamp` - O(1), bounds checking
- `sign` - O(1), useful for comparisons
- `even?` / `odd?` - O(1), common predicates
- `gcd` - O(log n), Euclidean algorithm
- `lcm` - O(log n), derived from gcd

**Estimated:** 6-8 functions, 30+ tests

**C. Function Combinators** (Day 34 - 3 hours)
- `compose` - Essential for functional programming
- `pipe` - Reverse composition (more readable)
- `partial` - Currying helper
- `flip` - Argument reordering
- `const` - Constant function (already have lambda version)

**Estimated:** 5 functions, 20+ tests

### Priority 2: ORGANIZATION & DOCUMENTATION (Day 35)

**D. Stdlib Prelude** (4 hours)
- Create `stdlib/prelude.scm` that loads all modules
- Document load order (dependencies)
- Add usage guide
- Test that everything loads correctly

**E. API Documentation** (4 hours)
- Create `docs/reference/STDLIB_API.md`
- Complete function reference (all 70+ functions)
- Examples for each function
- Common patterns and use cases

### Priority 3: NICE TO HAVE (Day 36+)

**F. Predicate Combinators** (3 hours)
- `conjoin` / `disjoin` - Predicate combinators
- `negate` / `complement` - Negation helpers

**G. Advanced List Operations** (Future)
- Lazy evaluation (requires new infrastructure)
- Infinite lists (requires lazy evaluation)
- Memoization helpers

**H. String Functions** (Future - blocked on infrastructure)
- Character iteration primitives needed
- Character arithmetic primitives needed
- More complex string parsing

---

## Implementation Plan

### Day 32: List Utilities ⭐ START HERE

**Goal:** Add 8 high-value list utilities

**Functions:**

```scheme
; take-while :: (α → 𝔹) → [α] → [α]
; Take elements while predicate holds
(≔ take-while (λ (pred) (λ (lst)
  (? (∅? lst)
     ∅
     (? (pred (◁ lst))
        (⟨⟩ (◁ lst) ((take-while pred) (▷ lst)))
        ∅)))))

; drop-while :: (α → 𝔹) → [α] → [α]
; Drop elements while predicate holds
(≔ drop-while (λ (pred) (λ (lst)
  (? (∅? lst)
     ∅
     (? (pred (◁ lst))
        ((drop-while pred) (▷ lst))
        lst)))))

; span :: (α → 𝔹) → [α] → ⟨[α] [α]⟩
; Split list at first element that fails predicate
(≔ span (λ (pred) (λ (lst)
  (⟨⟩ ((take-while pred) lst)
      ((drop-while pred) lst)))))

; break :: (α → 𝔹) → [α] → ⟨[α] [α]⟩
; Split list at first element that satisfies predicate
(≔ break (λ (pred) (λ (lst)
  ((span (λ (x) (¬ (pred x)))) lst))))

; flatten :: [[α]] → [α]
; Flatten nested list structure
(≔ flatten (λ (lst)
  (? (∅? lst)
     ∅
     (⧺ (◁ lst) (flatten (▷ lst))))))

; distinct :: [α] → [α]
; Remove duplicates (O(n²) simple implementation)
(≔ distinct (λ (lst)
  (? (∅? lst)
     ∅
     ((λ (head)
       ((λ (tail-distinct)
         (? (∈ head tail-distinct)
            tail-distinct
            (⟨⟩ head tail-distinct)))
        (distinct (▷ lst))))
      (◁ lst)))))

; group-by :: (α → β) → [α] → [(β, [α])]
; Group elements by key function (returns association list)
; Implementation complex, needs helper functions

; nth-or :: [α] → ℕ → α → α
; Safe nth with default value
(≔ nth-or (λ (lst) (λ (n) (λ (default)
  (? (∅? lst)
     default
     (? (≡ n #0)
        (◁ lst)
        (((nth-or (▷ lst)) (⊖ n #1)) default)))))))
```

**Testing:**
- Edge cases (empty lists, single element, large lists)
- Predicate behavior (always true, always false, mixed)
- Integration tests (combining functions)

**Deliverable:** `stdlib/list_utilities.scm` + `tests/test_list_utilities.scm` (40+ tests)

---

### Day 33: Numeric Utilities

**Goal:** Add 6-8 essential numeric helpers

**Functions:**

```scheme
; abs :: ℕ → ℕ
; Absolute value
(≔ abs (λ (n)
  (? (< n #0) (⊖ #0 n) n)))

; clamp :: ℕ → ℕ → ℕ → ℕ
; Clamp value between min and max
(≔ clamp (λ (min) (λ (max) (λ (x)
  (? (< x min)
     min
     (? (> x max)
        max
        x))))))

; sign :: ℕ → ℕ
; Return -1 for negative, 0 for zero, 1 for positive
(≔ sign (λ (n)
  (? (< n #0) #-1
     (? (> n #0) #1
        #0))))

; even? :: ℕ → 𝔹
; Check if number is even
(≔ even? (λ (n)
  (≡ (% n #2) #0)))

; odd? :: ℕ → 𝔹
; Check if number is odd
(≔ odd? (λ (n)
  (¬ (even? n))))

; gcd :: ℕ → ℕ → ℕ
; Greatest common divisor (Euclidean algorithm)
(≔ gcd (λ (a) (λ (b)
  (? (≡ b #0)
     a
     ((gcd b) (% a b))))))

; lcm :: ℕ → ℕ → ℕ
; Least common multiple
(≔ lcm (λ (a) (λ (b)
  (⊘ (⊗ a b) ((gcd a) b)))))
```

**Testing:**
- Zero, negative, positive numbers
- Edge cases (very large numbers, overflow?)
- Mathematical properties (gcd/lcm relationships)

**Deliverable:** `stdlib/numeric.scm` + `tests/test_numeric.scm` (30+ tests)

---

### Day 34: Function Combinators

**Goal:** Essential higher-order function utilities

**Functions:**

```scheme
; compose :: (β → γ) → (α → β) → (α → γ)
; Function composition (f ∘ g)(x) = f(g(x))
(≔ compose (λ (f) (λ (g) (λ (x)
  (f (g x))))))

; pipe :: (α → β) → (β → γ) → (α → γ)
; Reverse composition (g |> f)(x) = f(g(x))
(≔ pipe (λ (g) (λ (f) (λ (x)
  (f (g x))))))

; flip :: (α → β → γ) → (β → α → γ)
; Flip argument order
(≔ flip (λ (f) (λ (b) (λ (a)
  ((f a) b)))))

; const :: α → β → α
; Constant function (ignores second argument)
(≔ const (λ (a) (λ (_) a)))

; identity :: α → α
; Identity function (already exists as λ 0, but formalize)
(≔ identity (λ (x) x))
```

**Testing:**
- Function composition chains
- Integration with list operations
- Higher-order combinations

**Deliverable:** `stdlib/combinators.scm` + `tests/test_combinators.scm` (20+ tests)

---

### Day 35: Organization & Documentation

**A. Stdlib Prelude** (Morning - 4 hours)

**Goal:** Single import for all stdlib functionality

```scheme
; stdlib/prelude.scm
; Guage Standard Library - Complete Prelude
; Load all stdlib modules in dependency order

; Core list operations (no dependencies)
(⋘ "stdlib/list.scm")

; Extended list operations (depends on list.scm)
(⋘ "stdlib/list_extended.scm")

; List utilities (depends on list.scm)
(⋘ "stdlib/list_utilities.scm")

; Option/Result types (depends on list.scm for some operations)
(⋘ "stdlib/option.scm")

; Math utilities (depends on list.scm for list-based operations)
(⋘ "stdlib/math.scm")

; Numeric utilities (no dependencies)
(⋘ "stdlib/numeric.scm")

; Function combinators (no dependencies)
(⋘ "stdlib/combinators.scm")

; String manipulation (minimal dependencies)
(⋘ "stdlib/string.scm")

; Total functions loaded: 70+ functions
; All tested and production-ready
```

**Testing:** Verify all modules load correctly, no circular dependencies, all functions available

**Deliverable:** `stdlib/prelude.scm` + integration test

---

**B. API Documentation** (Afternoon - 4 hours)

**Goal:** Complete, searchable API reference

**Structure:** `docs/reference/STDLIB_API.md`

```markdown
# Guage Standard Library API Reference

## Quick Reference

### List Operations (21 functions)
- Core: map, filter, fold-left, fold-right, etc. (15)
- Extended: find, partition, concat, etc. (6)
- Utilities: take-while, flatten, distinct, etc. (8)

### Option/Result Types (22 functions)
- Option: Some, None, map-option, etc. (11)
- Result: Ok, Err, map-result, etc. (9)
- Conversions: option-to-result, etc. (2)

### Math & Numeric (14 functions)
- Math: sum, product, min, max (6)
- Numeric: abs, clamp, gcd, even?, etc. (8)

### Function Combinators (5 functions)
- compose, pipe, flip, const, identity

### String Manipulation (5 functions)
- join, contains, repeat, whitespace check

## Complete Reference

[For each function: type signature, description, params, examples, related functions]
```

**Deliverable:** Complete API documentation with examples

---

## Success Metrics

### Day 32-34: Implementation ✅

- [ ] 8 list utilities implemented and tested (40+ tests)
- [ ] 6-8 numeric utilities implemented and tested (30+ tests)
- [ ] 5 function combinators implemented and tested (20+ tests)
- [ ] **Total:** 19-21 new functions, 90+ new tests
- [ ] All tests passing
- [ ] No breaking changes to existing stdlib

### Day 35: Organization ✅

- [ ] Prelude module loads all stdlib correctly
- [ ] API documentation complete (70+ functions documented)
- [ ] Examples for common use cases
- [ ] Integration tests pass

### Overall: Quality ✅

- [ ] All functions follow symbolic naming
- [ ] All functions have type signatures
- [ ] All functions thoroughly tested
- [ ] Documentation clear and helpful
- [ ] Ready for production use

---

## After Day 36: What's Next?

### Week 5 Candidates

**Option A: String Library Enhancement**
- Implement character iteration primitives
- Add character arithmetic primitives
- Complete deferred string functions (split, trim, case)

**Option B: REPL Improvements**
- Better error messages
- Module introspection commands
- Help system (`:help`, `:doc`, etc.)
- History and editing

**Option C: Advanced Stdlib**
- Lazy evaluation infrastructure
- Infinite lists
- Memoization helpers
- Advanced data structures (sets, maps)

**Option D: Self-Hosting Work**
- Parser in Guage
- Compiler in Guage
- Bootstrap towards full self-hosting

---

## Risk Assessment

### Low Risk ✅
- All planned functions use existing primitives
- Pattern matching works perfectly
- Module system tested and stable
- Clear implementation path

### Medium Risk ⚠️
- `group-by` implementation may be complex (association lists)
- Documentation completeness requires careful attention
- Integration testing needs thorough coverage

### Mitigation
- Start with simpler functions (abs, even?, etc.)
- Incremental testing (test each function immediately)
- Defer complex functions if needed (group-by can wait)
- Focus on high-value, proven patterns

---

## Timeline Summary

| Day | Focus | Functions | Tests | Hours |
|-----|-------|-----------|-------|-------|
| **32** | List utilities | 8 | 40+ | 6 |
| **33** | Numeric utilities | 6-8 | 30+ | 4 |
| **34** | Function combinators | 5 | 20+ | 3 |
| **35** | Prelude + API docs | - | - | 8 |
| **36+** | Polish + future work | - | - | TBD |

**Total:** 19-21 functions, 90+ tests, ~21 hours

---

## Conclusion

**Goal:** Expand stdlib from 54 → 73+ functions with complete organization and documentation

**Impact:**
- **Production-ready:** Comprehensive stdlib for real programs
- **Well-documented:** Complete API reference
- **Easy to use:** Single prelude import
- **Solid foundation:** Ready for advanced features

**Critical Path:**
1. Day 32: List utilities (HIGH VALUE)
2. Day 33: Numeric utilities (HIGH VALUE)
3. Day 34: Function combinators (HIGH VALUE)
4. Day 35: Organization & documentation (ESSENTIAL)

**Status:** READY TO PROCEED - All infrastructure in place!

---

**Prepared by:** Claude Sonnet 4.5
**Date:** 2026-01-27
**Phase:** Week 4 Planning (Days 32-36)
**Next:** Implement list utilities (Day 32)
