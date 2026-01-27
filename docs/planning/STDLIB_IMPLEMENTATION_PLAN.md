---
Status: CURRENT
Created: 2026-01-27
Updated: 2026-01-27
Purpose: Implementation plan for Guage standard library foundation
---

# Standard Library Implementation Plan

## Overview

**Goal:** Build foundational standard library using pattern matching capabilities
**Timeline:** Days 20-21 (8-10 hours)
**Dependencies:** Pattern matching complete ✅

## Motivation

**Why Now:**
- Pattern matching is complete - perfect for list operations!
- Need practical functions to demonstrate language power
- Foundation for MVP (can't ship without basic utilities)
- Natural next step after core language features

**What This Enables:**
- Real programs (not just toy examples)
- Showcase pattern matching in action
- Standard idioms for list/data manipulation
- Foundation for more complex libraries

## Architecture

### Library Structure

```
stdlib/
├── list.scm       - List operations (map, filter, fold, etc)
├── option.scm     - Option/Maybe type helpers
├── result.scm     - Result/Either type helpers
├── math.scm       - Extended math functions
└── util.scm       - General utilities
```

### Loading Strategy

**Phase 1 (Current):** Single file prelude
```scheme
; stdlib/prelude.scm - Auto-loaded at startup
; Contains all core stdlib functions
```

**Phase 2 (Future):** Module system
```scheme
(⋐ :list)     ; Import list module
(⋐⊙ :list :map :filter)  ; Import specific functions
```

## Implementation Plan

### Part 1: List Operations (4 hours)

**Core Functions:**

#### 1.1 map - Transform each element
```scheme
(≔ map (λ (f lst)
  (∇ lst (⌜ ((∅ ∅)
            ((⟨⟩ head tail) (⟨⟩ (f head) (map f tail))))))))

; Example:
(map (λ (x) (⊗ x #2)) (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅))))
; → (⟨⟩ #2 (⟨⟩ #4 (⟨⟩ #6 ∅)))
```

**Type:** `(α → β) → [α] → [β]`
**Tests:**
- Empty list → empty list
- Single element
- Multiple elements
- Nested transformations

#### 1.2 filter - Keep elements matching predicate
```scheme
(≔ filter (λ (pred lst)
  (∇ lst (⌜ ((∅ ∅)
            ((⟨⟩ head tail)
             (? (pred head)
                (⟨⟩ head (filter pred tail))
                (filter pred tail))))))))

; Example:
(filter (λ (x) (> x #5)) (⟨⟩ #3 (⟨⟩ #7 (⟨⟩ #2 (⟨⟩ #9 ∅)))))
; → (⟨⟩ #7 (⟨⟩ #9 ∅))
```

**Type:** `(α → 𝔹) → [α] → [α]`
**Tests:**
- Empty list
- All pass predicate
- None pass predicate
- Mixed results

#### 1.3 fold-left - Accumulate from left
```scheme
(≔ fold-left (λ (f acc lst)
  (∇ lst (⌜ ((∅ acc)
            ((⟨⟩ head tail)
             (fold-left f (f acc head) tail)))))))

; Example:
(fold-left ⊕ #0 (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅))))  ; Sum
; → #6
```

**Type:** `(α → β → α) → α → [β] → α`
**Tests:**
- Empty list → returns accumulator
- Sum of list
- Product of list
- Build reversed list

#### 1.4 fold-right - Accumulate from right
```scheme
(≔ fold-right (λ (f lst acc)
  (∇ lst (⌜ ((∅ acc)
            ((⟨⟩ head tail)
             (f head (fold-right f tail acc))))))))

; Example:
(fold-right ⟨⟩ ∅ (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅))))  ; Identity
; → (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅)))
```

**Type:** `(α → β → β) → [α] → β → β`
**Tests:**
- Empty list → returns accumulator
- Builds list (identity)
- Append operation

#### 1.5 length - Count elements
```scheme
(≔ length (λ (lst)
  (fold-left (λ (acc _) (⊕ acc #1)) #0 lst)))

; Or with pattern matching:
(≔ length (λ (lst)
  (∇ lst (⌜ ((∅ #0)
            ((⟨⟩ _ tail) (⊕ #1 (length tail))))))))
```

**Type:** `[α] → ℕ`
**Tests:**
- Empty list → 0
- Single element → 1
- Multiple elements

#### 1.6 append - Concatenate lists
```scheme
(≔ append (λ (lst1 lst2)
  (fold-right ⟨⟩ lst2 lst1)))

; Or with pattern matching:
(≔ append (λ (lst1 lst2)
  (∇ lst1 (⌜ ((∅ lst2)
             ((⟨⟩ head tail) (⟨⟩ head (append tail lst2))))))))
```

**Type:** `[α] → [α] → [α]`
**Tests:**
- Both empty
- First empty
- Second empty
- Both non-empty

#### 1.7 reverse - Reverse list order
```scheme
(≔ reverse (λ (lst)
  (fold-left (λ (acc x) (⟨⟩ x acc)) ∅ lst)))
```

**Type:** `[α] → [α]`
**Tests:**
- Empty list → empty
- Single element → same
- Multiple elements → reversed

#### 1.8 take - Take first n elements
```scheme
(≔ take (λ (n lst)
  (? (≡ n #0)
     ∅
     (∇ lst (⌜ ((∅ ∅)
               ((⟨⟩ head tail) (⟨⟩ head (take (⊖ n #1) tail)))))))))
```

**Type:** `ℕ → [α] → [α]`
**Tests:**
- Take 0 → empty
- Take more than length → entire list
- Take from empty → empty
- Normal case

#### 1.9 drop - Skip first n elements
```scheme
(≔ drop (λ (n lst)
  (? (≡ n #0)
     lst
     (∇ lst (⌜ ((∅ ∅)
               ((⟨⟩ _ tail) (drop (⊖ n #1) tail))))))))
```

**Type:** `ℕ → [α] → [α]`
**Tests:**
- Drop 0 → same list
- Drop more than length → empty
- Normal case

#### 1.10 zip - Pair corresponding elements
```scheme
(≔ zip (λ (lst1 lst2)
  (∇ (⟨⟩ lst1 lst2)
     (⌜ (((⟨⟩ ∅ _) ∅)
         ((⟨⟩ _ ∅) ∅)
         ((⟨⟩ (⟨⟩ x xs) (⟨⟩ y ys))
          (⟨⟩ (⟨⟩ x y) (zip xs ys))))))))
```

**Type:** `[α] → [β] → [⟨α β⟩]`
**Tests:**
- Both empty
- Different lengths (stops at shortest)
- Same length

#### 1.11 any - Test if any element matches
```scheme
(≔ any (λ (pred lst)
  (∇ lst (⌜ ((∅ #f)
            ((⟨⟩ head tail)
             (? (pred head) #t (any pred tail))))))))
```

**Type:** `(α → 𝔹) → [α] → 𝔹`

#### 1.12 all - Test if all elements match
```scheme
(≔ all (λ (pred lst)
  (∇ lst (⌜ ((∅ #t)
            ((⟨⟩ head tail)
             (? (pred head) (all pred tail) #f)))))))
```

**Type:** `(α → 𝔹) → [α] → 𝔹`

### Part 2: Option Type (1.5 hours)

**Define Option ADT:**
```scheme
(⊚≔ :Option (⌜ (:None)) (⌜ (:Some :value)))
```

#### 2.1 Option constructors
```scheme
(≔ none (⊚ :Option :None))
(≔ some (λ (x) (⊚ :Option :Some x)))
```

#### 2.2 map-option - Transform wrapped value
```scheme
(≔ map-option (λ (f opt)
  (∇ opt (⌜ (((⊚ :Option :None) none)
            ((⊚ :Option :Some v) (some (f v))))))))
```

**Type:** `(α → β) → Option α → Option β`

#### 2.3 bind-option - Monadic bind
```scheme
(≔ bind-option (λ (opt f)
  (∇ opt (⌜ (((⊚ :Option :None) none)
            ((⊚ :Option :Some v) (f v)))))))
```

**Type:** `Option α → (α → Option β) → Option β`

#### 2.4 unwrap-or - Get value with default
```scheme
(≔ unwrap-or (λ (opt default)
  (∇ opt (⌜ (((⊚ :Option :None) default)
            ((⊚ :Option :Some v) v))))))
```

**Type:** `Option α → α → α`

#### 2.5 is-some / is-none - Predicates
```scheme
(≔ is-some (λ (opt)
  (∇ opt (⌜ (((⊚ :Option :None) #f)
            ((⊚ :Option :Some _) #t))))))

(≔ is-none (λ (opt) (¬ (is-some opt))))
```

**Type:** `Option α → 𝔹`

### Part 3: Result Type (1.5 hours)

**Define Result ADT:**
```scheme
(⊚≔ :Result (⌜ (:Ok :value)) (⌜ (:Err :error)))
```

#### 3.1 Result constructors
```scheme
(≔ ok (λ (x) (⊚ :Result :Ok x)))
(≔ err (λ (e) (⊚ :Result :Err e)))
```

#### 3.2 map-result
```scheme
(≔ map-result (λ (f res)
  (∇ res (⌜ (((⊚ :Result :Ok v) (ok (f v)))
            ((⊚ :Result :Err e) (err e)))))))
```

**Type:** `(α → β) → Result α ε → Result β ε`

#### 3.3 bind-result
```scheme
(≔ bind-result (λ (res f)
  (∇ res (⌜ (((⊚ :Result :Ok v) (f v))
            ((⊚ :Result :Err e) (err e)))))))
```

**Type:** `Result α ε → (α → Result β ε) → Result β ε`

#### 3.4 unwrap-or-else
```scheme
(≔ unwrap-or-else (λ (res handler)
  (∇ res (⌜ (((⊚ :Result :Ok v) v)
            ((⊚ :Result :Err e) (handler e)))))))
```

**Type:** `Result α ε → (ε → α) → α`

#### 3.5 is-ok / is-err
```scheme
(≔ is-ok (λ (res)
  (∇ res (⌜ (((⊚ :Result :Ok _) #t)
            ((⊚ :Result :Err _) #f))))))

(≔ is-err (λ (res) (¬ (is-ok res))))
```

**Type:** `Result α ε → 𝔹`

### Part 4: Math Extensions (1 hour)

#### 4.1 abs - Absolute value
```scheme
(≔ abs (λ (x) (? (< x #0) (⊖ #0 x) x)))
```

#### 4.2 min / max
```scheme
(≔ min (λ (x y) (? (< x y) x y)))
(≔ max (λ (x y) (? (> x y) x y)))
```

#### 4.3 pow - Exponentiation
```scheme
(≔ pow (λ (base exp)
  (? (≡ exp #0)
     #1
     (⊗ base (pow base (⊖ exp #1))))))
```

#### 4.4 gcd - Greatest common divisor
```scheme
(≔ gcd (λ (a b)
  (? (≡ b #0) a (gcd b (% a b)))))
```

#### 4.5 lcm - Least common multiple
```scheme
(≔ lcm (λ (a b)
  (⊘ (⊗ a b) (gcd a b))))
```

### Part 5: Utilities (1 hour)

#### 5.1 identity
```scheme
(≔ identity (λ (x) x))
```

#### 5.2 const
```scheme
(≔ const (λ (x) (λ (_) x)))
```

#### 5.3 compose
```scheme
(≔ compose (λ (f g) (λ (x) (f (g x)))))
```

#### 5.4 flip
```scheme
(≔ flip (λ (f) (λ (x y) (f y x))))
```

#### 5.5 curry / uncurry
```scheme
(≔ curry (λ (f) (λ (x) (λ (y) (f (⟨⟩ x y))))))
(≔ uncurry (λ (f) (λ (pair)
  (∇ pair (⌜ (((⟨⟩ x y) (f x y))))))))
```

## Testing Strategy

### Test File Structure

```
tests/
├── test_stdlib_list.scm      - List operations
├── test_stdlib_option.scm    - Option type
├── test_stdlib_result.scm    - Result type
├── test_stdlib_math.scm      - Math extensions
└── test_stdlib_util.scm      - Utilities
```

### Test Coverage Requirements

**For each function:**
- Empty/nil cases
- Single element cases
- Multiple element cases
- Edge cases (zero, negative, etc)
- Error conditions
- Integration with other stdlib functions

**Example test structure:**
```scheme
; Test map with empty list
(⊨ :map-empty
   ∅
   (map (λ (x) (⊗ x #2)) ∅))

; Test map with single element
(⊨ :map-single
   (⟨⟩ #4 ∅)
   (map (λ (x) (⊗ x #2)) (⟨⟩ #2 ∅)))

; Test map with multiple elements
(⊨ :map-multi
   (⟨⟩ #2 (⟨⟩ #4 (⟨⟩ #6 ∅)))
   (map (λ (x) (⊗ x #2)) (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅)))))

; Test map composition
(⊨ :map-compose
   (⟨⟩ #4 (⟨⟩ #6 (⟨⟩ #8 ∅)))
   (map (λ (x) (⊕ x #2))
        (map (λ (x) (⊗ x #2)) (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅))))))
```

## Implementation Order

### Day 20 (4-5 hours)

**Morning (2-3 hours):**
1. List operations: map, filter, fold-left, fold-right
2. Tests for core list operations
3. Verify pattern matching works well

**Afternoon (2 hours):**
1. List utilities: length, append, reverse
2. List slicing: take, drop
3. List combinators: zip, any, all
4. Comprehensive tests

### Day 21 (4-5 hours)

**Morning (2-3 hours):**
1. Option type definition and constructors
2. Option operations: map, bind, unwrap-or
3. Option tests

**Afternoon (2 hours):**
1. Result type definition and constructors
2. Result operations: map, bind, unwrap-or-else
3. Result tests
4. Math extensions and utilities
5. Final integration tests

## Documentation Updates

**After completion:**
1. Update SPEC.md with stdlib reference
2. Add examples to SESSION_HANDOFF.md
3. Update primitive count if any new primitives added
4. Document design decisions in TECHNICAL_DECISIONS.md

## Success Criteria

**Must have:**
- ✅ All list operations working
- ✅ Option type complete
- ✅ Result type complete
- ✅ 100+ stdlib tests passing
- ✅ No memory leaks
- ✅ Clean compilation

**Should have:**
- ✅ Math extensions
- ✅ Utility functions
- ✅ Comprehensive examples
- ✅ Integration tests

**Nice to have:**
- ⏳ Performance benchmarks
- ⏳ Usage examples in REPL
- ⏳ Tutorial documentation

## Known Challenges

### Challenge 1: List Construction Performance
**Issue:** Naive recursion may be slow for large lists
**Solution:** Accept for now, optimize later with tail recursion

### Challenge 2: Pattern Matching Syntax
**Issue:** Nested patterns can be verbose
**Solution:** Use helper functions to reduce nesting

### Challenge 3: Type Documentation
**Issue:** No type checking at runtime
**Solution:** Document types clearly in comments

### Challenge 4: Error Handling
**Issue:** How to handle errors in stdlib functions?
**Solution:** Use Result type for fallible operations

## Next Steps After Stdlib

**Week 4 priorities:**
1. Macro system basics
2. String support
3. I/O primitives
4. Module system design

---

**Status:** READY TO START
**Dependencies:** Pattern matching ✅
**Timeline:** 2 days (Days 20-21)
**Estimated Effort:** 8-10 hours
