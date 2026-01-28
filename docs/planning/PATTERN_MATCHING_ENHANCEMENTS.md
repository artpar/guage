---
Status: CURRENT
Created: 2026-01-28
Updated: 2026-01-28
Purpose: Planning document for pattern matching enhancements (Days 58+)
---

# Pattern Matching Enhancements

## Current State (Day 57)

✅ **Pattern Matching Bug FIXED**
- `∇` now works correctly with De Bruijn indices in nested lambdas
- 57/58 tests passing (98%)
- 14 comprehensive pattern matching tests
- Solid foundation for enhancements

## Motivation

Pattern matching is currently functional but basic. Adding advanced features would make Guage's pattern matching world-class:

**Current Capabilities:**
- ✅ Literal patterns (#42, :foo, #t, #f)
- ✅ Variable patterns (x, y, z) - binds values
- ✅ Wildcard pattern (_) - matches anything
- ✅ Pair patterns (⟨⟩ a b) - destructures pairs
- ✅ Leaf structure patterns (⊙ :Type fields...)
- ✅ Node/ADT patterns (⊚ :Type :Variant fields...)

**Missing Features:**
- ❌ Guard conditions - conditional matching
- ❌ As-patterns - bind whole and parts
- ❌ Or-patterns - multiple alternatives
- ❌ View patterns - transform before matching

## Proposed Enhancements

### 1. Guard Conditions (Priority: HIGH)

**Syntax:** `(pattern | guard-expr) result-expr`

**Purpose:** Add conditional logic to patterns

**Examples:**
```scheme
;; Match positive even numbers
(∇ x (⌜ ((n | (∧ (> n #0) (≡ (⊘ n #2) #0))) :positive-even)
       ((n | (> n #0)) :positive-odd)
       (_ :negative-or-zero)))

;; Match non-empty lists with head > 10
(∇ lst (⌜ (((⟨⟩ h t) | (> h #10)) h)
          ((⟨⟩ h t) #0)
          (∅ #-1))))

;; Match Result.Ok with value in range [0, 100]
(∇ result (⌜ (((⊚ :Result :Ok v) | (∧ (≥ v #0) (≤ v #100))) :in-range)
             ((⊚ :Result :Ok v) :out-of-range)
             ((⊚ :Result :Err e) :error))))
```

**Implementation:**
- Parse `(pattern | guard)` syntax
- After pattern matches, evaluate guard in extended environment
- If guard returns #t, use this clause
- If guard returns #f, try next clause

**Estimated Time:** 2-3 hours

### 2. As-Patterns (Priority: MEDIUM)

**Syntax:** `name@pattern`

**Purpose:** Bind both the whole value AND its parts

**Examples:**
```scheme
;; Bind pair and its components
(∇ p (⌜ ((pair@(⟨⟩ a b) (⟨⟩ pair a b)))))  ; Returns: ⟨⟨#1 #2⟩ #1 #2⟩

;; Bind Result and its value
(∇ r (⌜ ((ok@(⊚ :Result :Ok v) (⟨⟩ ok v))
         (err@(⊚ :Result :Err e) (⟨⟩ err e)))))

;; Clone a list node
(∇ lst (⌜ ((node@(⟨⟩ h t) (⟨⟩ h node))))  ; Returns: ⟨head original-list⟩
```

**Implementation:**
- Parse `name@subpattern` syntax
- Match subpattern as normal
- Add binding: name → original value
- Merge all bindings

**Estimated Time:** 2-3 hours

### 3. Or-Patterns (Priority: MEDIUM)

**Syntax:** `(pattern₁ | pattern₂ | pattern₃)`

**Purpose:** Match any of several alternatives

**Examples:**
```scheme
;; Match 0, 1, or 2
(∇ x (⌜ ((#0 | #1 | #2) :small)
       (_ :other)))

;; Match Ok or Err (both are valid)
(∇ r (⌜ (((⊚ :Result :Ok _) | (⊚ :Result :Err _)) :is-result)
       (_ :not-result)))

;; Match multiple keywords
(∇ sym (⌜ ((:add | :sub | :mul | :div) :arithmetic-op)
         ((:eq | :ne | :lt | :gt) :comparison-op)
         (_ :unknown)))
```

**Implementation:**
- Parse `(pat1 | pat2 | ...)` syntax
- Try matching each pattern in order
- First match wins
- All patterns must bind same variables (or none)

**Estimated Time:** 3-4 hours

### 4. View Patterns (Priority: LOW)

**Syntax:** `(→ transform-fn pattern)`

**Purpose:** Transform value before matching

**Examples:**
```scheme
;; Extract length then match
(∇ lst (⌜ (((→ # n) | (≡ n #0)) :empty)
          ((→ # n) | (> n #10)) :long)
          (_ :short)))

;; Convert to uppercase then match
(∇ str (⌜ (((→ str-upper s) | (≡ s "HELLO")) :greeting)
          (_ :other)))

;; Unwrap Result.Ok before matching
(∇ result (⌜ (((→ (λ (r) (? (ok? r) (⊚→ r :value) #0)) n) | (> n #10))
              :large-value)
             (_ :other)))
```

**Implementation:**
- Parse `(→ transform pattern)` syntax
- Apply transform function to value
- Match transformed value against pattern
- Transform function must be pure

**Estimated Time:** 2-3 hours

## Total Estimated Time

- **Guard Conditions:** 2-3 hours (HIGH priority)
- **As-Patterns:** 2-3 hours (MEDIUM priority)
- **Or-Patterns:** 3-4 hours (MEDIUM priority)
- **View Patterns:** 2-3 hours (LOW priority)

**Total:** 9-13 hours (1-2 sessions)

## Implementation Strategy

### Phase 1: Guards (Day 58)

1. **Design syntax** - Decide on guard separator (`|` vs `:` vs other)
2. **Parse guards** - Extend pattern parser
3. **Evaluate guards** - After pattern match, before result
4. **Test** - Write comprehensive guard tests
5. **Document** - Update SPEC.md with examples

### Phase 2: As-Patterns (Day 59)

1. **Design syntax** - Decide on `@` vs other separator
2. **Parse as-patterns** - Extend pattern parser
3. **Bind whole value** - Add to bindings list
4. **Test** - Nested as-patterns, complex structures
5. **Document** - Update SPEC.md

### Phase 3: Or-Patterns (Day 60)

1. **Design syntax** - Handle variable consistency
2. **Parse alternatives** - Multiple pattern options
3. **Match first success** - Try each in order
4. **Test** - Variable binding edge cases
5. **Document** - Update SPEC.md

### Phase 4: View Patterns (Optional)

Only if time permits - this is least essential.

## Success Criteria

**For each enhancement:**
- ✅ Syntax is intuitive and consistent
- ✅ Implementation is correct (no edge case bugs)
- ✅ Tests are comprehensive (10+ tests per feature)
- ✅ Documentation is clear (SPEC.md updated)
- ✅ No regressions (all existing tests pass)

**Overall:**
- ✅ 70+ pattern matching tests total
- ✅ Real-world examples demonstrate power
- ✅ Performance is acceptable (no slowdowns)

## Test Plan

### Guard Conditions Tests
- Numeric guards (>, <, ≡)
- Boolean guards (∧, ∨, ¬)
- Complex guards (multiple conditions)
- Guard fails, try next pattern
- All guards fail → :no-match

### As-Patterns Tests
- Simple as-patterns (x@#42)
- Nested as-patterns (x@(y@(⟨⟩ a b)))
- ADT as-patterns (ok@(⊚ :Result :Ok v))
- Multiple as-patterns in same match

### Or-Patterns Tests
- Simple alternatives (#0 | #1 | #2)
- ADT alternatives (Ok | Err)
- Variable consistency check
- Deeply nested or-patterns

### View Patterns Tests
- Simple transforms (→ # n)
- Complex transforms (→ str-upper s)
- Chained transforms (→ f (→ g p))
- Transform + guard combination

## Documentation Updates

**Files to Update:**
1. `SPEC.md` - Add new pattern syntax
2. `docs/reference/SYMBOLIC_VOCABULARY.md` - Document new symbols
3. `SESSION_HANDOFF.md` - Track progress
4. `docs/INDEX.md` - Update test counts

**Examples to Add:**
- Practical use cases for each feature
- Comparison with other languages (Haskell, OCaml, Rust)
- Performance characteristics

## Future Enhancements (Beyond Day 60)

Once basic enhancements are complete:

1. **Pattern Compilation** - Compile patterns to decision trees
2. **Exhaustiveness Checking** - Already have warnings, make them stronger
3. **Redundancy Detection** - Warn about unreachable patterns (already done!)
4. **Pattern Coverage Analysis** - Report which cases are handled

## References

- **Bug Fix:** `docs/reference/PATTERN_MATCHING_DEBRUIJN_BUG.md`
- **Current Tests:** `bootstrap/tests/test_pattern_debruijn_fix.test`
- **Implementation:** `bootstrap/pattern.c`
- **Haskell Guards:** https://wiki.haskell.org/Pattern_guard
- **OCaml Patterns:** https://ocaml.org/manual/patterns.html
- **Rust Match:** https://doc.rust-lang.org/book/ch18-03-pattern-syntax.html

---

**Status:** PLANNED - Ready to implement
**Priority:** HIGH - Pattern matching is core feature
**Next Session:** Start with Guard Conditions (Day 58)
