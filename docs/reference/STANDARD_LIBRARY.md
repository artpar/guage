---
Status: REFERENCE
Created: 2026-01-27
Updated: 2026-01-27
Purpose: Complete reference for Guage standard library functions
---

# Guage Standard Library Reference

This document provides a comprehensive reference for all standard library functions in Guage. All functions use pure symbolic notation and are implemented in pure Guage.

## Table of Contents

1. [Core List Operations](#core-list-operations) (4 functions)
2. [List Utilities](#list-utilities) (3 functions)
3. [List Slicing](#list-slicing) (2 functions)
4. [List Combinators](#list-combinators) (4 functions)
5. [List Search](#list-search) (1 function)
6. [List Building](#list-building) (2 functions)
7. [Extended List Operations](#extended-list-operations) (6 functions)
8. [Math Utilities](#math-utilities) (6 functions)
9. [String Manipulation](#string-manipulation) (5 functions)
10. [Option/Result Types](#optionresult-types) (22 functions)

**Total Functions: 55+**

---

## Core List Operations

### ↦ (map)
**Type:** `(α → β) → [α] → [β]`
**Description:** Transform each element using a function
**Complexity:** O(n) where n is list length

**Example:**
```scheme
; Double each element
((↦ (λ (x) (⊗ x #2))) (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅))))
; → (⟨⟩ #2 (⟨⟩ #4 (⟨⟩ #6 ∅)))
```

### ⊲ (filter)
**Type:** `(α → 𝔹) → [α] → [α]`
**Description:** Keep only elements satisfying predicate
**Complexity:** O(n)

**Example:**
```scheme
; Keep only elements > 5
((⊲ (λ (x) (> x #5))) (⟨⟩ #3 (⟨⟩ #7 (⟨⟩ #2 (⟨⟩ #9 ∅)))))
; → (⟨⟩ #7 (⟨⟩ #9 ∅))
```

### ⊕← (fold-left)
**Type:** `(α → β → α) → α → [β] → α`
**Description:** Accumulate from left to right
**Complexity:** O(n)

**Example:**
```scheme
; Sum all elements
(((⊕← (λ (a) (λ (b) (⊕ a b)))) #0) (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅))))
; → #6
```

### ⊕→ (fold-right)
**Type:** `(α → β → β) → [α] → β → β`
**Description:** Accumulate from right to left
**Complexity:** O(n)

**Example:**
```scheme
; Build new list (identity)
(((⊕→ (λ (a) (λ (b) (⟨⟩ a b)))) (⟨⟩ #1 (⟨⟩ #2 ∅))) ∅)
; → (⟨⟩ #1 (⟨⟩ #2 ∅))
```

---

## List Utilities

### # (length)
**Type:** `[α] → ℕ`
**Description:** Count elements in list
**Complexity:** O(n)

**Example:**
```scheme
(# (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅))))
; → #3
```

### ⧺ (append)
**Type:** `[α] → [α] → [α]`
**Description:** Concatenate two lists
**Complexity:** O(n) where n is first list length

**Example:**
```scheme
((⧺ (⟨⟩ #3 (⟨⟩ #4 ∅))) (⟨⟩ #1 (⟨⟩ #2 ∅)))
; → (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 (⟨⟩ #4 ∅))))
```

### ⇄ (reverse)
**Type:** `[α] → [α]`
**Description:** Reverse list order
**Complexity:** O(n)

**Example:**
```scheme
(⇄ (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅))))
; → (⟨⟩ #3 (⟨⟩ #2 (⟨⟩ #1 ∅)))
```

---

## List Slicing

### ↑ (take)
**Type:** `ℕ → [α] → [α]`
**Description:** First n elements
**Complexity:** O(min(n, list length))

**Example:**
```scheme
((↑ #2) (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅))))
; → (⟨⟩ #1 (⟨⟩ #2 ∅))
```

### ↓ (drop)
**Type:** `ℕ → [α] → [α]`
**Description:** Skip first n elements
**Complexity:** O(n)

**Example:**
```scheme
((↓ #2) (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅))))
; → (⟨⟩ #3 ∅)
```

---

## List Combinators

### ⊼ (zip)
**Type:** `[α] → [β] → [⟨α β⟩]`
**Description:** Pair corresponding elements
**Complexity:** O(min(n, m))

**Example:**
```scheme
((⊼ (⟨⟩ #4 (⟨⟩ #5 ∅))) (⟨⟩ #1 (⟨⟩ #2 ∅)))
; → (⟨⟩ (⟨⟩ #1 #4) (⟨⟩ (⟨⟩ #2 #5) ∅))
```

### ∃ (exists/any)
**Type:** `(α → 𝔹) → [α] → 𝔹`
**Description:** Test if any element satisfies predicate
**Complexity:** O(n) worst case, O(1) best case

**Example:**
```scheme
((∃ (λ (x) (> x #5))) (⟨⟩ #3 (⟨⟩ #7 ∅)))
; → #t
```

### ∀ (forall/all)
**Type:** `(α → 𝔹) → [α] → 𝔹`
**Description:** Test if all elements satisfy predicate
**Complexity:** O(n) worst case, O(1) best case

**Example:**
```scheme
((∀ (λ (x) (> x #0))) (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅))))
; → #t
```

### ∈ (contains)
**Type:** `α → [α] → 𝔹`
**Description:** Test membership
**Complexity:** O(n)

**Example:**
```scheme
((∈ #2) (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅))))
; → #t
```

---

## List Search

### ⇶ (find)
**Type:** `(α → 𝔹) → [α] → α | ∅`
**Description:** First element satisfying predicate (returns ∅ if not found)
**Complexity:** O(n) worst case, O(1) best case

**Example:**
```scheme
((⇶ (λ (x) (> x #5))) (⟨⟩ #3 (⟨⟩ #7 (⟨⟩ #2 ∅))))
; → #7
```

---

## List Building

### ⋯ (range)
**Type:** `ℕ → ℕ → [ℕ]`
**Description:** Numbers from start to end (exclusive)
**Complexity:** O(end - start)

**Example:**
```scheme
((⋯ #5) #1)
; → (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 (⟨⟩ #4 ∅))))
```

### ⊚⊚ (replicate)
**Type:** `α → ℕ → [α]`
**Description:** n copies of value
**Complexity:** O(n)

**Example:**
```scheme
((⊚⊚ #7) #3)
; → (⟨⟩ #7 (⟨⟩ #7 (⟨⟩ #7 ∅)))
```

---

## Extended List Operations

### ⊡ (nth)
**Type:** `ℕ → [α] → α | ∅`
**Description:** Get element at index (0-based, returns ∅ if out of bounds)
**Complexity:** O(n)

**Example:**
```scheme
((⊡ #1) (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅))))
; → #2
```

### ⊳ (partition)
**Type:** `(α → 𝔹) → [α] → ⟨[α] [α]⟩`
**Description:** Split into (satisfies, doesn't-satisfy) pair
**Complexity:** O(n)

**Example:**
```scheme
((⊳ (λ (x) (> x #5))) (⟨⟩ #3 (⟨⟩ #7 (⟨⟩ #2 (⟨⟩ #9 ∅)))))
; → (⟨⟩ (⟨⟩ #7 (⟨⟩ #9 ∅)) (⟨⟩ #3 (⟨⟩ #2 ∅)))
```

### ⊞ (concat)
**Type:** `[[α]] → [α]`
**Description:** Flatten list of lists into single list
**Complexity:** O(n * m) where n is outer list length, m is average inner list length

**Example:**
```scheme
(⊞ (⟨⟩ (⟨⟩ #1 (⟨⟩ #2 ∅)) (⟨⟩ (⟨⟩ #3 (⟨⟩ #4 ∅)) ∅)))
; → (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 (⟨⟩ #4 ∅))))
```

### ⊟ (intersperse)
**Type:** `α → [α] → [α]`
**Description:** Insert separator between elements
**Complexity:** O(n)

**Example:**
```scheme
((⊟ #0) (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅))))
; → (⟨⟩ #1 (⟨⟩ #0 (⟨⟩ #2 (⟨⟩ #0 (⟨⟩ #3 ∅)))))
```

### ⊠ (cartesian)
**Type:** `[α] → [β] → [⟨α β⟩]`
**Description:** Cartesian product of two lists
**Complexity:** O(n * m) where n and m are list lengths

**Example:**
```scheme
((⊠ (⟨⟩ #10 (⟨⟩ #20 ∅))) (⟨⟩ #1 (⟨⟩ #2 ∅)))
; → (⟨⟩ (⟨⟩ #1 #10) (⟨⟩ (⟨⟩ #1 #20) (⟨⟩ (⟨⟩ #2 #10) (⟨⟩ (⟨⟩ #2 #20) ∅))))
```

---

## Math Utilities

### ⊕⊕ (sum)
**Type:** `[ℕ] → ℕ`
**Description:** Sum of all numbers in list
**Complexity:** O(n)

**Example:**
```scheme
(⊕⊕ (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅))))
; → #6
```

### ⊗⊗ (product)
**Type:** `[ℕ] → ℕ`
**Description:** Product of all numbers in list
**Complexity:** O(n)

**Example:**
```scheme
(⊗⊗ (⟨⟩ #2 (⟨⟩ #3 (⟨⟩ #4 ∅))))
; → #24
```

### ↥ (max)
**Type:** `ℕ → ℕ → ℕ`
**Description:** Maximum of two numbers
**Complexity:** O(1)

**Example:**
```scheme
((↥ #5) #3)
; → #5
```

### ↧ (min)
**Type:** `ℕ → ℕ → ℕ`
**Description:** Minimum of two numbers
**Complexity:** O(1)

**Example:**
```scheme
((↧ #5) #3)
; → #3
```

### ↥↥ (maximum)
**Type:** `[ℕ] → ℕ | ∅`
**Description:** Maximum value in list (returns ∅ if empty)
**Complexity:** O(n)

**Example:**
```scheme
(↥↥ (⟨⟩ #3 (⟨⟩ #7 (⟨⟩ #2 (⟨⟩ #9 ∅)))))
; → #9
```

### ↧↧ (minimum)
**Type:** `[ℕ] → ℕ | ∅`
**Description:** Minimum value in list (returns ∅ if empty)
**Complexity:** O(n)

**Example:**
```scheme
(↧↧ (⟨⟩ #3 (⟨⟩ #7 (⟨⟩ #2 (⟨⟩ #9 ∅)))))
; → #2
```

---

## String Manipulation

See `stdlib/string.scm` for implementation. This is a simplified first version focusing on essential operations.

### ≈⊙? (is-whitespace)
**Type:** `:symbol → 𝔹`
**Description:** Check if character symbol is whitespace (space, tab, newline, carriage return)
**Complexity:** O(1)

**Example:**
```scheme
(≈⊙? :  )   ; → #t (space)
(≈⊙? :\t)   ; → #t (tab)
(≈⊙? :a)    ; → #f (letter)
```

### ≈⊠ (join)
**Type:** `[≈] → ≈ → ≈`
**Description:** Join list of strings with delimiter
**Complexity:** O(n × m) where n is list length, m is average string length

**Example:**
```scheme
; CSV header
((≈⊠ (⟨⟩ "name" (⟨⟩ "age" (⟨⟩ "city" ∅)))) ",")
; → "name,age,city"

; Path construction
((≈⊠ (⟨⟩ "usr" (⟨⟩ "local" (⟨⟩ "bin" ∅)))) "/")
; → "usr/local/bin"

; Join words
((≈⊠ (⟨⟩ "hello" (⟨⟩ "world" ∅))) " ")
; → "hello world"
```

### ≈⊃ (contains)
**Type:** `≈ → ≈ → 𝔹`
**Description:** Check if string contains substring
**Complexity:** O(n × m) where n is string length, m is substring length

**Example:**
```scheme
((≈⊃ "hello world") "world")    ; → #t
((≈⊃ "hello world") "goodbye")  ; → #f
((≈⊃ "hello world") "")         ; → #t (empty always contained)
```

### ≈⊗ (repeat)
**Type:** `≈ → ℕ → ≈`
**Description:** Repeat string n times
**Complexity:** O(n × m) where n is count, m is string length

**Example:**
```scheme
((≈⊗ "ab") #3)      ; → "ababab"
((≈⊗ "x") #5)       ; → "xxxxx"
((≈⊗ "test") #0)    ; → ""
```

### ≈⊃→ (contains-at)
**Type:** `≈ → ≈ → ℕ → 𝔹`
**Description:** Helper function - check if substring exists at position i
**Complexity:** O(m) where m is substring length

**Note:** This is an internal helper for ≈⊃. Direct use not typically needed.

### Real-World Examples

**Build CSV row:**
```scheme
(≔ build-csv (λ (fields)
  ((≈⊠ fields) ",")))

(build-csv (⟨⟩ "Alice" (⟨⟩ "30" (⟨⟩ "NYC" ∅))))
; → "Alice,30,NYC"
```

**Join words with spaces:**
```scheme
(≔ join-words (λ (words)
  ((≈⊠ words) " ")))

(join-words (⟨⟩ "The" (⟨⟩ "quick" (⟨⟩ "brown" ∅))))
; → "The quick brown"
```

**Search in text:**
```scheme
(≔ has-keyword? (λ (text) (λ (keyword)
  ((≈⊃ text) keyword))))

((has-keyword? "Guage is awesome") "awesome")  ; → #t
```

**Repeat for padding:**
```scheme
(≔ pad-left (λ (s) (λ (n)
  (≈⊕ ((≈⊗ " ") n) s))))

((pad-left "test") #3)  ; → "   test"
```

### Future Functions (Deferred)

The following functions require more complex character-by-character processing and will be added once implementation patterns are established:

- **≈⊞** (split) - Split string by delimiter (complex recursion)
- **≈⊳** (trim-left) - Remove leading whitespace (char iteration)
- **≈⊴** (trim-right) - Remove trailing whitespace (char iteration)
- **≈⊲** (trim) - Remove both leading/trailing whitespace (composition)
- **≈↑** (uppercase) - Convert to uppercase (char arithmetic)
- **≈↓** (lowercase) - Convert to lowercase (char arithmetic)

---

## Option/Result Types

See `stdlib/option.scm` for complete documentation of the 22 Option/Result functions:

- **Option Type (11 functions):** ⊙◇, ⊙∅, ⊙?, ⊙∅?, ⊙→, ⊙⊙, ⊙∨, ⊙!, ⊙⊕
- **Result Type (9 functions):** ⊙✓, ⊙✗, ⊙✓?, ⊙✗?, ⊙⇒, ⊙⇐, ⊙⊙⇒, ⊙‼, ⊙‼∨
- **Conversions (2 functions):** ⊙→⊙, ⊙⊙→

**Type-safe error handling without exceptions!**

---

## Common Patterns

### Pattern 1: Pipeline with map and filter
```scheme
; Get doubled values of numbers > 5
((↦ (λ (x) (⊗ x #2))) ((⊲ (λ (x) (> x #5))) (⟨⟩ #3 (⟨⟩ #7 (⟨⟩ #2 (⟨⟩ #9 ∅))))))
; → (⟨⟩ #14 (⟨⟩ #18 ∅))
```

### Pattern 2: Fold for aggregation
```scheme
; Sum all elements > 5
(((⊕← (λ (a) (λ (b) (⊕ a b)))) #0) ((⊲ (λ (x) (> x #5))) (⟨⟩ #3 (⟨⟩ #7 (⟨⟩ #2 (⟨⟩ #9 ∅))))))
; → #16
```

### Pattern 3: Using partition
```scheme
; Split even and odd
(≔ is-even (λ (x) (≡ (% x #2) #0)))
((⊳ is-even) (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 (⟨⟩ #4 ∅)))))
; → (⟨⟩ (⟨⟩ #2 (⟨⟩ #4 ∅)) (⟨⟩ #1 (⟨⟩ #3 ∅)))
```

### Pattern 4: Finding with exists and find
```scheme
; Check if any > 10
((∃ (λ (x) (> x #10))) (⟨⟩ #5 (⟨⟩ #15 ∅))))  ; → #t

; Get first > 10
((⇶ (λ (x) (> x #10))) (⟨⟩ #5 (⟨⟩ #15 ∅))))  ; → #15
```

### Pattern 5: Cartesian product for combinations
```scheme
; All pairs (a, b) where a ∈ [1,2], b ∈ [10,20]
((⊠ (⟨⟩ #10 (⟨⟩ #20 ∅))) (⟨⟩ #1 (⟨⟩ #2 ∅)))
; → all 4 combinations
```

---

## Performance Notes

### Time Complexity Summary

| Operation | Complexity | Notes |
|-----------|------------|-------|
| ↦, ⊲, ⊕←, ⊕→ | O(n) | Single pass |
| #, ⇄ | O(n) | Single pass |
| ⧺ | O(n) | n = first list |
| ↑, ↓ | O(n) | n = elements taken/dropped |
| ⊼ | O(min(n,m)) | Stops at shorter list |
| ∃, ∀, ∈ | O(n) worst | Early exit possible |
| ⇶ | O(n) worst | Early exit possible |
| ⋯, ⊚⊚ | O(n) | n = count |
| ⊡ | O(n) | Index access |
| ⊳ | O(n) | Single pass (but calls self twice) |
| ⊞ | O(n*m) | n = outer, m = avg inner |
| ⊟ | O(n) | Single pass |
| ⊠ | O(n*m) | All pairs |
| ⊕⊕, ⊗⊗ | O(n) | Single pass |
| ↥, ↧ | O(1) | Binary comparison |
| ↥↥, ↧↧ | O(n) | Single pass |

### Space Complexity

All functions use O(n) space for the result list (or less for filters/predicates).

---

## Known Limitations

### 1. Symbol Lists in Lambdas (CRITICAL BUG)

**Issue:** Lists containing symbols (e.g., `(⟨⟩ :a (⟨⟩ :b ∅))`) cannot be passed as lambda parameters due to a De Bruijn conversion bug.

**Workaround:** Use number lists instead of symbol lists.

**Status:** Documented in `tests/KNOWN_BUGS.md`, needs fixing before Week 4.

### 2. No Module System Yet

**Issue:** Must inline all function definitions in each file.

**Workaround:** Copy-paste function definitions or concatenate files.

**Status:** Module system planned for Week 4-5.

### 3. Explicit Currying Required

**Issue:** All multi-argument functions are curried and require explicit parentheses.

**Example:**
```scheme
; WRONG: (↦ f list)
; RIGHT: ((↦ f) list)
```

This is by design (lambda calculus purity) but can be surprising.

---

## Source Files

- **Core lists:** `stdlib/list.scm` (15 functions)
- **Extended lists:** `stdlib/list_extended.scm` (6 functions)
- **Math:** `stdlib/math.scm` (6 functions)
- **Option/Result:** `stdlib/option.scm` (22 functions)

**Tests:**
- `tests/stdlib_list.test` (33 tests)
- `tests/test_list_extended.scm` (38 tests)
- `tests/test_math.scm` (36 tests)
- `tests/test_option_combined.scm` (55 tests)

**Total: 162+ stdlib tests, all passing!**

---

## Future Additions

Planned for upcoming weeks:

- **String operations** (Week 5)
- **I/O operations** (Week 5)
- **More math** (trigonometry, logarithms, etc.)
- **Tree utilities** (traversals, balancing)
- **Graph algorithms** (DFS, BFS, topological sort)
- **Parser combinators** (for building parsers)

---

**Last Updated:** 2026-01-27 (Day 22)
**Maintainer:** Guage Core Team
