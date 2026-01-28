---
Status: CURRENT
Created: 2026-01-28
Updated: 2026-01-28 (Day 59 - Phase 2 Complete)
Purpose: Planning document for pattern matching enhancements (Days 58-60)
---

# Pattern Matching Enhancements

## 🚀 Quick Start for Day 60

**Task:** Implement Or-Patterns (`(pattern₁ | pattern₂)` syntax)
**Time:** 3-4 hours
**Priority:** MEDIUM (next in sequence)

**What to implement:**
- Syntax: `name@pattern` binds both whole value AND pattern parts
- Example: `pair@(⟨⟩ a b)` binds `pair`, `a`, and `b`
- Works with all pattern types (literals, pairs, structures, ADTs)

**Implementation steps:**
1. Add `is_as_pattern()` helper to detect `@` syntax
2. Add `extract_as_pattern()` to parse `name@subpattern`
3. Modify `pattern_try_match()` to:
   - Detect as-pattern syntax
   - Match subpattern recursively
   - Add binding for `name` → original value
   - Merge with subpattern bindings
4. Write 10+ comprehensive tests
5. Update SPEC.md with syntax and examples

**See:** Phase 2 section below for detailed implementation plan

---

## Progress Overview

**Phase 1 (Day 58):** ✅ **COMPLETE** - Guard Conditions (2.5 hours)
**Phase 2 (Day 59):** ✅ **COMPLETE** - As-Patterns (2.5 hours)
**Phase 3 (Day 60):** ⏳ **NEXT** - Or-Patterns (3-4 hours estimated)
**Phase 4 (Optional):** ⏳ **PLANNED** - View Patterns (2-3 hours estimated)

## Current State (Day 59 End)

✅ **Pattern Matching with Guard Conditions and As-Patterns**
- `∇` works with De Bruijn indices in closures (Day 57)
- Guard conditions fully implemented (Day 58)
- As-patterns fully implemented (Day 59)
- 59/60 tests passing (98%)
- 72 pattern matching tests (14 De Bruijn + 30 guards + 28 as-patterns)
- World-class pattern matching achieved

## Motivation

Pattern matching enhancements make Guage comparable to Haskell, OCaml, and Rust:

**Current Capabilities:**
- ✅ Literal patterns (#42, :foo, #t, #f)
- ✅ Variable patterns (x, y, z) - binds values
- ✅ Wildcard pattern (_) - matches anything
- ✅ Pair patterns (⟨⟩ a b) - destructures pairs
- ✅ Leaf structure patterns (⊙ :Type fields...)
- ✅ Node/ADT patterns (⊚ :Type :Variant fields...)
- ✅ **Guard conditions (pattern | guard-expr)** - Day 58 ✅
- ✅ **As-patterns (name @ pattern)** - Day 59 ✅

**Remaining Features:**
- ⏳ Or-patterns - multiple alternatives (Day 60 NEXT)
- ⏳ View patterns - transform before matching (Optional)

## Enhancements

### 1. Guard Conditions ✅ **COMPLETE** (Day 58)

**Status:** DONE - 30 tests passing, fully integrated
**Time Taken:** 2.5 hours (estimated 2-3 hours)
**Impact:** HIGH - Pattern matching now world-class

**Syntax:** `(pattern | guard-expr) result-expr`

**Purpose:** Add conditional logic to patterns

**Examples:**
```scheme
;; Match positive even numbers
(∇ x (⌜ ((n | (∧ (> n #0) (≡ (% n #2) #0))) :positive-even)
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

**Implementation Completed:**
- ✅ Parse `(pattern | guard)` syntax
- ✅ After pattern matches, evaluate guard in extended environment
- ✅ If guard returns #t, use this clause
- ✅ If guard returns #f, try next clause
- ✅ Fully backward compatible with patterns without guards
- ✅ All pattern types work with guards

**Files Modified:**
- `bootstrap/pattern.c` - Added guard parsing and evaluation
- `bootstrap/tests/test_pattern_guards.test` - 30 comprehensive tests
- `SPEC.md` - Updated with guard syntax
- `docs/archive/2026-01/sessions/SESSION_END_DAY_58.md` - Session notes

### 2. As-Patterns ✅ **COMPLETE** (Day 59)

**Status:** DONE - 28 tests passing, fully integrated
**Time Taken:** 2.5 hours (estimated 2-3 hours)
**Impact:** MEDIUM - More expressive pattern matching

**Syntax:** `name@pattern`

**Purpose:** Bind both the whole value AND its parts

**Examples:**
```scheme
;; Bind pair and its components
(∇ (⟨⟩ #1 #2) (⌜ (((pair @ (⟨⟩ a b)) (⟨⟩ pair (⟨⟩ a b))))))
; → ⟨⟨#1 #2⟩ ⟨#1 #2⟩⟩

;; Bind Result and its value
(∇ (⊚ :Result :Ok #42) (⌜ (((ok @ (⊚ :Result :Ok v)) (⟨⟩ ok v)))))
; → ⟨⊚[:Result :Ok #42] #42⟩

;; Clone a list node
(∇ (⟨⟩ #42 (⟨⟩ #99 ∅)) (⌜ (((node @ (⟨⟩ h t)) (⟨⟩ h node)))))
; → ⟨#42 ⟨#42 ⟨#99 ∅⟩⟩⟩

;; Nested as-patterns
(∇ (⟨⟩ #5 #6) (⌜ (((outer @ (inner @ (⟨⟩ a b))) (⟨⟩ outer inner)))))
; → ⟨⟨#5 #6⟩ ⟨#5 #6⟩⟩

;; As-patterns with guards
(∇ (⟨⟩ #5 #10) (⌜ ((((pair @ (⟨⟩ a b)) | (> a #0)) pair)
                   (_ :failed))))  ; → ⟨#5 #10⟩
```

**Implementation Completed:**
- ✅ Parse `name@subpattern` syntax with `is_as_pattern()` and `extract_as_pattern()`
- ✅ Match subpattern recursively
- ✅ Add binding: name → original value
- ✅ Merge whole-value binding with subpattern bindings
- ✅ Works with all pattern types (literals, pairs, structures, ADTs)
- ✅ Combines seamlessly with guards

**Files Modified:**
- `bootstrap/pattern.c` - Added as-pattern parsing and matching
- `bootstrap/tests/test_pattern_as_patterns.test` - 28 comprehensive tests
- `SPEC.md` - Updated with as-pattern syntax
- `docs/planning/PATTERN_MATCHING_ENHANCEMENTS.md` - Updated status

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

## Time Tracking

- **Phase 1 - Guard Conditions:** ✅ 2.5 hours actual (estimated 2-3 hours)
- **Phase 2 - As-Patterns:** ✅ 2.5 hours actual (estimated 2-3 hours)
- **Phase 3 - Or-Patterns:** ⏳ 3-4 hours estimated (NEXT)
- **Phase 4 - View Patterns:** ⏳ 2-3 hours estimated (optional)

**Total Completed:** 5 hours (Days 58-59)
**Total Remaining:** 5-7 hours (Days 60+)
**Overall Estimate:** 10-12 hours (1.5-2 sessions)

## Implementation Strategy

### Phase 1: Guards (Day 58) ✅ **COMPLETE**

1. ✅ **Design syntax** - Chose `|` separator (clear, distinct)
2. ✅ **Parse guards** - Extended pattern parser with `has_guard()` and `extract_pattern_and_guard()`
3. ✅ **Evaluate guards** - After pattern match, before result, in extended environment
4. ✅ **Test** - 30 comprehensive guard tests covering all use cases
5. ✅ **Document** - Updated SPEC.md with syntax and examples

**Result:** Guard conditions fully integrated, 58/59 tests passing

### Phase 2: As-Patterns (Day 59) ✅ **COMPLETE**

**Completed:** Successful implementation in 2.5 hours

1. ✅ **Design syntax** - Used `@` separator (clear, standard in Haskell/Rust)
2. ✅ **Parse as-patterns** - Detect `name@pattern` syntax with helpers
3. ✅ **Bind whole value** - Add binding for `name` pointing to original value
4. ✅ **Merge bindings** - Combine whole-value binding with pattern bindings
5. ✅ **Test** - 28 comprehensive tests covering all cases
6. ✅ **Document** - Updated SPEC.md with syntax and examples

**Actual Time:** 2.5 hours
**Result:** All 28 tests passing, no regressions

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

**Status:** IN PROGRESS - Phases 1-2 complete, Phase 3 next
**Priority:** HIGH - Pattern matching is core feature
**Next Session:** Implement Or-Patterns (Day 60)
**Completion:** 2/4 phases done (50% complete, 5/12 hours spent)
