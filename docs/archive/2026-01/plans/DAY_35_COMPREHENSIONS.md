---
Status: CURRENT
Created: 2026-01-27
Updated: 2026-01-27
Purpose: Day 35 implementation plan - List comprehensions and advanced control flow
---

# Day 35: List Comprehensions & Advanced Control Flow

## Executive Summary

**Goal:** Build ergonomic list comprehensions and loop macros using the macro system

**Duration:** 1 day (6-8 hours)

**Prerequisites:**
- ✅ Macro system (Day 33)
- ✅ Stdlib macros (Day 34)
- ✅ Pattern matching (Days 15-19)
- ✅ List operations (Days 20-22)

**Impact:** Transform list processing from recursive functions to declarative expressions

---

## Why List Comprehensions?

### Current State (Functional but Verbose)

**Manual recursion for list operations:**
```scheme
; Map - manual recursion
(≔ map (λ (𝕗 𝕩)
  (? (∅? 𝕩)
     ∅
     (⟨⟩ (𝕗 (◁ 𝕩))
          (map 𝕗 (▷ 𝕩))))))

; Filter - manual recursion
(≔ filter (λ (𝕡 𝕩)
  (? (∅? 𝕩)
     ∅
     (? (𝕡 (◁ 𝕩))
        (⟨⟩ (◁ 𝕩) (filter 𝕡 (▷ 𝕩)))
        (filter 𝕡 (▷ 𝕩))))))

; Building lists - manual construction
(≔ squares (λ (𝕩)
  (? (≡ 𝕩 #0)
     ∅
     (⟨⟩ (⊗ 𝕩 𝕩)
          (squares (⊖ 𝕩 #1))))))
```

### With List Comprehensions (After Day 35)

**Declarative list operations:**
```scheme
; Map - declarative
[((⊗ 𝕩 #2)) for 𝕩 in (⋯ #1 #10)]
; → ⟨#2 ⟨#4 ⟨#6 ... ⟨#20 ∅⟩⟩⟩⟩

; Filter - declarative
[(𝕩) for 𝕩 in (⋯ #1 #20) if (≡ (% 𝕩 #2) #0)]
; → ⟨#2 ⟨#4 ⟨#6 ... ⟨#20 ∅⟩⟩⟩⟩

; Complex transformations - clear intent
[(⟨⟩ 𝕩 (⊗ 𝕩 𝕩)) for 𝕩 in (⋯ #1 #5)]
; → ⟨⟨#1 #1⟩ ⟨⟨#2 #4⟩ ⟨⟨#3 #9⟩ ...⟩⟩⟩

; Multiple filters
[(𝕩) for 𝕩 in (⋯ #1 #100)
      if (≡ (% 𝕩 #2) #0)   ; Even
      if (≡ (% 𝕩 #5) #0)]  ; Divisible by 5
; → ⟨#10 ⟨#20 ⟨#30 ...⟩⟩⟩
```

**Advanced loops:**
```scheme
; For-each with side effects
(for 𝕩 in items do
  (≋ 𝕩))

; While loop
(while (< 𝕩 #10) do
  (≋ 𝕩)
  (≔ 𝕩 (⊕ 𝕩 #1)))

; Until loop
(until (≡ 𝕩 #0) do
  (≔ 𝕩 (⊖ 𝕩 #1)))
```

---

## Implementation Plan

### Phase 1: Basic List Comprehensions (3 hours)

#### 1.1 Simple Map Comprehension (60 min)

**Goal:** `[(expr) for var in list]` → transformed list

**Implementation:**
```scheme
; Basic comprehension - transforms each element
; Syntax: [(expr) for var in list]
; Example: [(⊗ x #2) for x in (⋯ #1 #5)]
; Result: ⟨#2 ⟨#4 ⟨#6 ⟨#8 ⟨#10 ∅⟩⟩⟩⟩⟩

(⧉ [_for_in_]
    (expr :for var :in lst)
    (⌞̃ (↦ (λ ((~ var)) (~ expr)) (~ lst))))
```

**Tests:**
- Basic transformation (multiply by 2)
- Complex expression (pair construction)
- Nested lists
- Empty list
- Single element
- Arithmetic operations

**Deliverable:** Basic comprehension working (10 tests)

#### 1.2 Filter Comprehension (60 min)

**Goal:** `[(expr) for var in list if (pred)]` → filtered + transformed

**Implementation:**
```scheme
; Comprehension with filter
; Syntax: [(expr) for var in list if (predicate)]
; Example: [(x) for x in (⋯ #1 #10) if (≡ (% x #2) #0)]
; Result: ⟨#2 ⟨#4 ⟨#6 ⟨#8 ⟨#10 ∅⟩⟩⟩⟩⟩

(⧉ [_for_in_if_]
    (expr :for var :in lst :if pred)
    (⌞̃ (↦ (λ ((~ var))
             (? (~ pred) (⟨⟩ (~ expr) ∅) ∅))
          (~ lst))))
```

**Challenge:** Need to flatten nested results.

**Solution:** Use `⊕⊳` (fold-right) to build list:
```scheme
(⧉ [_for_in_if_]
    (expr :for var :in lst :if pred)
    (⌞̃ (⊕⊳ (λ ((~ var) acc)
              (? (~ pred)
                 (⟨⟩ (~ expr) acc)
                 acc))
           ∅
           (~ lst))))
```

**Tests:**
- Filter even numbers
- Filter by condition
- Filter + transform
- Multiple conditions (needs Phase 2)
- No matches (empty result)
- All match (full list)

**Deliverable:** Filter comprehension working (12 tests)

#### 1.3 Range Generation Helper (60 min)

**Goal:** `(⋯ start end)` → list of numbers from start to end

**Implementation:**
```scheme
; Range generation
; Syntax: (⋯ start end)
; Example: (⋯ #1 #5) → ⟨#1 ⟨#2 ⟨#3 ⟨#4 ⟨#5 ∅⟩⟩⟩⟩⟩

(≔ ⋯ (λ (start end)
  (? (> start end)
     ∅
     (⟨⟩ start (⋯ (⊕ start #1) end)))))
```

**Tests:**
- Basic range (1 to 5)
- Single element (1 to 1)
- Empty range (5 to 1)
- Large range (1 to 100)
- Negative ranges (-5 to 5)

**Deliverable:** Range function + 5 tests

---

### Phase 2: Advanced Comprehensions (2 hours)

#### 2.1 Multiple Filters (45 min)

**Goal:** Support chaining multiple `if` clauses

**Syntax:**
```scheme
[(x) for x in (⋯ #1 #100)
     if (≡ (% x #2) #0)   ; Even
     if (≡ (% x #5) #0)]  ; Divisible by 5
```

**Implementation:**
```scheme
; Combine predicates with AND
(⧉ [_for_in_if_if_]
    (expr :for var :in lst :if pred1 :if pred2)
    (⌞̃ (⊕⊳ (λ ((~ var) acc)
              (? (∧… (~ pred1) (~ pred2))
                 (⟨⟩ (~ expr) acc)
                 acc))
           ∅
           (~ lst))))
```

**Alternative:** Use `∧…` from Day 34:
```scheme
[(x) for x in (⋯ #1 #100)
     if (∧… (≡ (% x #2) #0) (≡ (% x #5) #0))]
```

**Tests:**
- Two filters (even AND divisible by 5)
- Contradictory filters (no results)
- Redundant filters (all pass)

**Deliverable:** Multiple filters working (5 tests)

#### 2.2 Nested Comprehensions (45 min)

**Goal:** Comprehensions that produce lists of lists

**Syntax:**
```scheme
; Cartesian product
[[(⟨⟩ x y)] for x in (⋯ #1 #3)
            for y in (⋯ #1 #3)]
```

**Implementation:**
```scheme
; Nested iteration
(⧉ [_for_in_for_in_]
    (expr :for var1 :in lst1 :for var2 :in lst2)
    (⌞̃ (⊕⊳ (λ ((~ var1) acc1)
              (⊕⊳ (λ ((~ var2) acc2)
                    (⟨⟩ (~ expr) acc2))
                  acc1
                  (~ lst2)))
          ∅
          (~ lst1))))
```

**Challenge:** Flattening semantics - do we want flat or nested?

**Decision:** Two variants:
- `[[ ]]` - Nested (list of lists)
- `[ ]` - Flat (concatenated)

**Tests:**
- Cartesian product
- Matrix operations
- Nested transformations

**Deliverable:** Nested comprehensions (6 tests)

#### 2.3 Accumulating Comprehensions (30 min)

**Goal:** Fold-style comprehensions with accumulator

**Syntax:**
```scheme
; Sum with accumulator
[((⊕ acc x)) for x in (⋯ #1 #10) with acc = #0]
```

**Implementation:**
```scheme
(⧉ [_for_in_with_=_]
    (expr :for var :in lst :with acc := init)
    (⌞̃ (⊕⊳ (λ ((~ var) (~ acc))
              (~ expr))
          (~ init)
          (~ lst))))
```

**Use cases:**
- Running sum
- Running product
- Building maps/sets

**Tests:**
- Sum accumulation
- Product accumulation
- Complex state

**Deliverable:** Accumulator comprehensions (4 tests)

---

### Phase 3: Loop Macros (2 hours)

#### 3.1 For-Each Loop (45 min)

**Goal:** Iterate with side effects (printing, I/O)

**Syntax:**
```scheme
(for x in items do
  (≋ x))
```

**Implementation:**
```scheme
(⧉ for_in_do
    (:for var :in lst :do body)
    (⌞̃ (⊕⊳ (λ ((~ var) _)
              (⌜ ⊙ (~ body) ∅))
          ∅
          (~ lst))))
```

**Tests:**
- Print each element
- Accumulate side effects
- Empty list

**Deliverable:** For-each macro (5 tests)

#### 3.2 While Loop (45 min)

**Goal:** Loop while condition is true

**Syntax:**
```scheme
(while (< x #10) do
  (≔ x (⊕ x #1)))
```

**Challenge:** Mutable state! Need to handle variable updates.

**Solution:** Use recursive helper:
```scheme
(⧉ while_do
    (:while cond :do body)
    (⌞̃ (⌜ ⊙ [helper (λ () (? (~ cond)
                              (⌜ ⊙ (~ body) (helper))
                              ∅))]
             (helper))))
```

**Note:** This requires global state or closure over mutable variable.

**Limitation:** May need to defer to Day 36+ when we have better mutation support.

**Tests:**
- Simple counter
- Conditional break
- Infinite loop guard

**Deliverable:** While macro (if possible) or documented limitation

#### 3.3 Until Loop (30 min)

**Goal:** Loop until condition becomes true

**Syntax:**
```scheme
(until (≡ x #0) do
  (≔ x (⊖ x #1)))
```

**Implementation:**
```scheme
(⧉ until_do
    (:until cond :do body)
    (⌞̃ (while_do (¬ (~ cond)) (~ body))))
```

**Tests:**
- Countdown
- Convergence

**Deliverable:** Until macro (3 tests)

---

### Phase 4: Integration & Testing (1 hour)

#### 4.1 Real-World Examples (30 min)

**Example 1: FizzBuzz**
```scheme
[(? (≡ (% n #15) #0) :FizzBuzz
    (? (≡ (% n #3) #0) :Fizz
       (? (≡ (% n #5) #0) :Buzz
          n)))
 for n in (⋯ #1 #20)]
```

**Example 2: Pythagorean Triples**
```scheme
[[(⟨⟩ a (⟨⟩ b (⟨⟩ c ∅)))]
 for a in (⋯ #1 #20)
 for b in (⋯ a #20)
 for c in (⋯ b #20)
 if (≡ (⊕ (⊗ a a) (⊗ b b)) (⊗ c c))]
```

**Example 3: List Processing Pipeline**
```scheme
; Original data
(≔ data (⋯ #1 #100))

; Filter, transform, sum
(⊕⊳ ⊕ #0
  [(⊗ x #2)
   for x in data
   if (≡ (% x #3) #0)])
```

#### 4.2 Performance Tests (30 min)

**Test:** Large list performance (1000+ elements)
**Test:** Deep nesting performance
**Test:** Memory usage (no leaks)

---

## Test Strategy

### Test Coverage Goals

| Category | Tests | Description |
|----------|-------|-------------|
| Basic comprehensions | 10 | Map, simple transforms |
| Filter comprehensions | 12 | Single + multiple filters |
| Range generation | 5 | Number ranges |
| Multiple filters | 5 | Chained conditions |
| Nested comprehensions | 6 | Cartesian products |
| Accumulator | 4 | Fold-style |
| For-each | 5 | Side effects |
| While/until | 6 | Loops (if possible) |
| Integration | 10 | Real-world examples |
| **TOTAL** | **63** | **Comprehensive** |

---

## Success Criteria

### Must Have ✅

- [ ] Basic list comprehension working `[(expr) for x in list]`
- [ ] Filter comprehension working `[... if (pred)]`
- [ ] Range generation `(⋯ start end)`
- [ ] For-each loop `(for x in list do ...)`
- [ ] 40+ tests passing
- [ ] All macros in stdlib/
- [ ] Documentation complete

### Should Have 📋

- [ ] Multiple filters `if ... if ...`
- [ ] Nested comprehensions
- [ ] Accumulator comprehensions
- [ ] Real-world examples working
- [ ] 60+ tests passing

### Nice to Have 🎯

- [ ] While/until loops (may need mutation support)
- [ ] Parallel comprehensions
- [ ] Performance optimizations
- [ ] Generator-style lazy evaluation

---

## Timeline

| Phase | Duration | Tasks | Deliverable |
|-------|----------|-------|-------------|
| 1. Basic | 3h | Map, filter, range | Core comprehensions |
| 2. Advanced | 2h | Multiple filters, nesting | Full feature set |
| 3. Loops | 2h | For-each, while, until | Loop constructs |
| 4. Integration | 1h | Examples, tests, docs | Complete |
| **TOTAL** | **8h** | **All features** | **63+ tests** |

---

## Dependencies

### Requires (Already Complete)

- ✅ Macro system (Day 33)
- ✅ Stdlib macros (Day 34)
- ✅ Pattern matching (Days 15-19)
- ✅ List functions (↦, ⊕⊳) (Days 20-22)

### Enables (Future Work)

1. **Cleaner code** - 5-10x reduction in boilerplate
2. **Functional programming** - More declarative style
3. **Algorithm implementation** - Easier to write complex operations
4. **Standard library** - Many functions become one-liners

---

## Risk Assessment

### Low Risk ✅

- Macro system proven (Day 33-34)
- List operations working
- Clear examples from Python/Haskell
- Can test incrementally

### Medium Risk ⚠️

- While/until loops need mutation handling
- Nested comprehensions can be complex
- Performance on large lists unknown
- Syntax parsing for `[...]` notation

### Mitigation

1. **Start with basic comprehensions** - Build incrementally
2. **Defer mutation-heavy loops** - Focus on pure operations
3. **Profile performance** - Test on realistic data
4. **Alternative syntax** - Use `⌜ ⟨...⟩ ⌝` if `[...]` problematic

---

## Alternative: If `[...]` Syntax Not Available

Use symbolic alternative:
```scheme
; Instead of: [(expr) for x in list]
; Use: (⊡ expr ⌜ x list)

(⊡ (⊗ x #2) ⌜ x (⋯ #1 #5))
; → ⟨#2 ⟨#4 ⟨#6 ⟨#8 ⟨#10 ∅⟩⟩⟩⟩⟩
```

Symbol: `⊡` (squared dot) for "collection builder"

---

## Conclusion

**Day 35 Goal:** Production-ready list comprehensions

**Impact:**
- **Usability:** Transform list processing
- **Leverage:** Uses macro system immediately
- **Foundation:** Enables functional programming style

**Critical Path:**
1. Basic comprehensions (map, filter)
2. Range generation
3. Advanced features (nested, filters)
4. Loop macros (for-each)

**Status:** READY TO IMPLEMENT

---

**Prepared by:** Claude Sonnet 4.5
**Date:** 2026-01-27
**Phase:** Day 35 Planning
**Next:** Implement basic list comprehensions
