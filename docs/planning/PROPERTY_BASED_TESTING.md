---
Status: PLANNED
Priority: HIGH
Estimated: 4-5 hours (1-2 sessions)
Created: 2026-01-28
---

# Property-Based Testing Implementation

## Overview

Implement QuickCheck-style property-based testing for Guage to improve test coverage and catch edge cases that manual tests miss.

## Why This Matters

**Current State:**
- 60/61 tests passing (98%)
- Tests are manually written examples
- Edge cases may be missed
- Limited coverage of random inputs

**After Property-Based Testing:**
- Automated generation of test cases
- 100+ random tests per property
- Shrinking finds minimal failing examples
- Better confidence in code correctness

## Goals

1. **Random Value Generation** - Generate typed random values
2. **Property Testing Framework** - Test properties with many random inputs
3. **Shrinking** - Minimize failing test cases
4. **Integration** - Work with existing test runner

## Implementation Plan

### Phase 1: Random Value Generators (1-2 hours)

**What to build:**
- Generator functions for each type
- Configurable constraints (ranges, sizes)
- Composable generators (lists of X, pairs of Y)

**API Design:**
```scheme
; Basic generators
(⌂⊨-gen :int)           → Random integer
(⌂⊨-gen :int-range min max) → Integer in range
(⌂⊨-gen :bool)          → Random boolean
(⌂⊨-gen :symbol)        → Random symbol
(⌂⊨-gen :string len)    → Random string of length

; Composite generators
(⌂⊨-gen :list gen n)    → List of n elements from gen
(⌂⊨-gen :pair gen1 gen2) → Pair of (gen1 . gen2)
(⌂⊨-gen :structure type gens...) → Random structure

; Custom generators
(⌂⊨-gen-custom (λ () ...))  → Use custom generator function
```

**Implementation:**
- C primitive: `prim_gen_random()` in primitives.c
- Use existing `rand` and `rand-int` primitives
- Type-based dispatch for generator selection

**Tests:**
```scheme
; Test integer generation
(⊨ :gen-int-range
   #t
   (let ((n (⌂⊨-gen :int-range #0 #100)))
     (∧ (≥ n #0) (≤ n #100))))

; Test list generation
(⊨ :gen-list-length
   #t
   (let ((lst (⌂⊨-gen :list :int #10)))
     (≡ (# lst) #10)))
```

### Phase 2: Property Testing Framework (1-2 hours)

**What to build:**
- Property test primitive: `⌂⊨-prop`
- Run 100+ test cases automatically
- Report statistics and failures

**API Design:**
```scheme
; Define a property test
(⌂⊨-prop name property generator [options])

; Example: Reverse is its own inverse
(⌂⊨-prop
  :reverse-inverse
  (λ (lst) (≟ lst (⊴← (⊴← lst))))  ; Property: double reverse = identity
  :gen-list-int                     ; Generator: random int lists
  (⌜ (:num-tests #100)))           ; Options: 100 test cases

; Example: Sort preserves length
(⌂⊨-prop
  :sort-length
  (λ (lst) (≡ (# lst) (# ((⊴ <) lst))))
  :gen-list-int)

; Example: Addition is commutative
(⌂⊨-prop
  :add-commutative
  (λ (pair)
    (let ((a (◁ pair)) (b (▷ pair)))
      (≡ (⊕ a b) (⊕ b a))))
  :gen-pair-int)
```

**Implementation:**
- C primitive: `prim_property_test()` in primitives.c
- Generate N random values using generator
- Apply property function to each
- Count successes/failures
- Report results

**Output Format:**
```
Property: reverse-inverse
  Tests: 100
  Passed: 100
  Failed: 0
  Status: ✓ PASSED

Property: sort-preserves-elements
  Tests: 100
  Passed: 97
  Failed: 3
  Status: ✗ FAILED
  First failure: [#5 #3 #3 #1]
```

### Phase 3: Shrinking (1-2 hours)

**What to build:**
- Minimize failing test cases
- Binary search through smaller inputs
- Report minimal reproducible failure

**Algorithm:**
```
1. Test fails on input X
2. Generate smaller variants of X:
   - For lists: Remove elements, reduce element values
   - For numbers: Try X/2, X-1, 0
   - For pairs: Shrink car, shrink cdr
3. Test each variant
4. If variant fails, recurse with smaller input
5. Return smallest failing input
```

**API:**
```scheme
; Shrinking happens automatically
(⌂⊨-prop
  :failing-prop
  (λ (lst) (< (# lst) #5))  ; Fails for lists with 5+ elements
  :gen-list-int)

; Output:
; Property: failing-prop
;   Tests: 12  (stopped early)
;   Failed on: [#0 #0 #0 #0 #0]
;   Shrinking... (original size: 47)
;   Minimal failure: [#0 #0 #0 #0 #0]
;   Status: ✗ FAILED
```

**Implementation:**
- C functions for shrinking each type
- Recursive shrinking with depth limit
- Cache to avoid retesting same values

### Phase 4: Integration (30 mins)

**What to do:**
1. Add property tests to test runner
2. Update test output format to include property tests
3. Add to SPEC.md documentation
4. Create examples in bootstrap/tests/

**Test File Format:**
```scheme
;; Property-based tests for list operations
(⋘ "bootstrap/stdlib/list.scm")

; Reverse is involutive
(⌂⊨-prop :reverse-involutive
  (λ (lst) (≟ lst (⊴← (⊴← lst))))
  :gen-list-int)

; Sort is idempotent
(⌂⊨-prop :sort-idempotent
  (λ (lst)
    (let ((sorted ((⊴ <) lst)))
      (≟ sorted ((⊴ <) sorted))))
  :gen-list-int)

; Filter preserves order
(⌂⊨-prop :filter-order
  (λ (lst)
    (let ((filtered ((↦⊟ (λ (x) (> x #0))) lst)))
      (≟ filtered ((⊴ <) filtered))))
  :gen-list-int-positive)
```

## File Structure

```
bootstrap/
├── primitives.c       # Add prim_gen_random, prim_property_test
├── primitives.h       # Export new primitives
└── tests/
    └── test_property.test  # Property-based tests

docs/
├── planning/
│   └── PROPERTY_BASED_TESTING.md  # This file
└── reference/
    └── PROPERTY_TESTING.md  # User documentation (after impl)
```

## Success Criteria

**MVP (Minimum Viable Product):**
- ✅ Random generators for 5+ types (int, bool, symbol, string, list)
- ✅ Property test primitive runs 100+ test cases
- ✅ Basic shrinking for numbers and lists
- ✅ Integration with test runner
- ✅ 10+ example property tests

**Nice to Have:**
- Custom generator combinators
- Configurable test counts
- Parallel test execution
- Coverage metrics
- HTML test reports

## Examples of Properties to Test

### List Operations
```scheme
; Reverse twice is identity
(⌂⊨-prop :reverse-inverse
  (λ (lst) (≟ lst (⊴← (⊴← lst))))
  :gen-list-int)

; Append length
(⌂⊨-prop :append-length
  (λ (pair)
    (let ((lst1 (◁ pair)) (lst2 (▷ pair)))
      (≡ (# (↑⊕ lst1 lst2))
         (⊕ (# lst1) (# lst2)))))
  :gen-pair-list)

; Map preserves length
(⌂⊨-prop :map-length
  (λ (lst)
    (≡ (# lst) (# ((↦⊕ (λ (x) (⊕ x #1))) lst))))
  :gen-list-int)
```

### Arithmetic Properties
```scheme
; Addition is commutative
(⌂⊨-prop :add-commutative
  (λ (pair) (≡ (⊕ (◁ pair) (▷ pair))
                (⊕ (▷ pair) (◁ pair))))
  :gen-pair-int)

; Multiplication distributes over addition
(⌂⊨-prop :mult-distributive
  (λ (triple)
    (let ((a (◁ triple))
          (b (◁ (▷ triple)))
          (c (▷ (▷ triple))))
      (≡ (⊗ a (⊕ b c))
         (⊕ (⊗ a b) (⊗ a c)))))
  :gen-triple-int)
```

### Sort Properties
```scheme
; Sorted output is actually sorted
(⌂⊨-prop :sort-is-sorted
  (λ (lst)
    (let ((sorted ((⊴ <) lst)))
      (or (≤ (# sorted) #1)
          (all-ordered? sorted))))
  :gen-list-int)

; Sort preserves elements (same multiset)
(⌂⊨-prop :sort-preserves-elements
  (λ (lst)
    (let ((sorted ((⊴ <) lst)))
      (same-multiset? lst sorted)))
  :gen-list-int)
```

## Comparison with Other Languages

### Haskell (QuickCheck)
```haskell
prop_reverse :: [Int] -> Bool
prop_reverse xs = reverse (reverse xs) == xs

quickCheck prop_reverse
-- +++ OK, passed 100 tests.
```

### Python (Hypothesis)
```python
from hypothesis import given
from hypothesis.strategies import lists, integers

@given(lists(integers()))
def test_reverse(lst):
    assert list(reversed(reversed(lst))) == lst
```

### Guage (Our Implementation)
```scheme
(⌂⊨-prop :reverse-inverse
  (λ (lst) (≟ lst (⊴← (⊴← lst))))
  :gen-list-int)
```

## Resources

**QuickCheck Papers:**
- "QuickCheck: A Lightweight Tool for Random Testing" (Claessen & Hughes, 2000)
- "QuickCheck Testing for Fun and Profit" (Hughes, 2007)

**Implementations:**
- Haskell: QuickCheck
- Scala: ScalaCheck
- Python: Hypothesis
- JavaScript: fast-check
- Erlang: PropEr

## Timeline

**Session 1 (Day 62, 2-3 hours):**
- Phase 1: Random generators
- Phase 2: Basic property testing (no shrinking)
- 5-10 example tests

**Session 2 (Day 63, 2 hours):**
- Phase 3: Shrinking implementation
- Phase 4: Integration and documentation
- Complete test suite

**Total:** 4-5 hours across 1-2 sessions

## Risks and Mitigations

**Risk:** Shrinking is complex
**Mitigation:** Start without shrinking, add later. MVP works without it.

**Risk:** Random tests may be flaky
**Mitigation:** Use seeded random number generator for reproducibility.

**Risk:** Performance of 100+ tests
**Mitigation:** Start with 100, make configurable. Parallelize if needed.

## Success Metrics

**After Implementation:**
- Find at least 1 bug in existing code
- 20+ property tests in test suite
- Test coverage increased
- Documentation complete

---

**Status:** Ready to implement
**Priority:** HIGH
**Next Action:** Start Phase 1 (Random Generators) in Day 62 session
