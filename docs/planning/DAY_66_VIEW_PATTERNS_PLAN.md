---
Status: CURRENT
Created: 2026-01-29
Updated: 2026-01-29
Purpose: Implementation plan for view patterns (Day 66)
---

# Day 66: View Patterns Implementation Plan

## Goal
Complete the pattern matching enhancement roadmap by implementing view patterns, bringing pattern matching to 100%.

## Background

**Current Pattern Matching Support (75% complete):**
- ✅ Basic patterns (literals, variables, pairs, structures, ADTs)
- ✅ Guard conditions (Day 58) - `(pattern | guard)`
- ✅ As-patterns (Day 59) - `name@pattern`
- ✅ Or-patterns (Day 60) - `(∨ pat1 pat2 ...)`
- 📋 View patterns (Day 66) - Transform before match

**View Patterns Enable:**
- Matching on computed properties (length, sum, etc.)
- Matching after transformation (abs, normalize, etc.)
- Complex structural tests before destructuring
- Pattern matching on derived values

## Syntax Design

### Proposed Syntax
```scheme
(→ transform pattern)
```

Where:
- `transform` is a function or quoted expression to apply
- `pattern` is any valid pattern to match the result against

### Examples
```scheme
; Match on list length
(∇ lst (⌜ (((→ # #3) :length-three)
           ((→ # #5) :length-five)
           (_ :other))))

; Match on absolute value
(∇ x (⌜ (((→ (λ (n) (? (< n #0) (⊖ #0 n) n)) (> #10)) :large-magnitude)
         (_ :small))))

; Combine with as-patterns
(∇ lst (⌜ (((original @ (→ ↑← #5)) (⟨⟩ original #5))
           (_ :not-length-5))))

; Combine with guards
(∇ x (⌜ ((((→ abs n) | (> n #10)) :large)
         (_ :small))))

; Multiple transformations
(∇ str (⌜ (((→ string-trim (→ string-length #5)) :five-char-trimmed)
            (_ :other))))
```

## Implementation Steps

### Phase 1: Syntax Detection (30 mins)
**File:** `bootstrap/pattern.c`

1. Add `is_view_pattern()` helper:
   ```c
   bool is_view_pattern(Cell* pat) {
       // Check for (→ transform pattern) syntax
       return is_pair(pat) &&
              is_symbol(car(pat)) &&
              strcmp(symbol_name(car(pat)), "→") == 0 &&
              is_pair(cdr(pat)) &&
              is_pair(cdr(cdr(pat)));
   }
   ```

2. Add `extract_view_pattern()` helper:
   ```c
   void extract_view_pattern(Cell* pat, Cell** transform, Cell** subpattern) {
       // Extract transform and subpattern from (→ transform pattern)
       *transform = car(cdr(pat));
       *subpattern = car(cdr(cdr(pat)));
   }
   ```

### Phase 2: Transform Evaluation (45 mins)
**File:** `bootstrap/pattern.c`

1. Modify `pattern_try_match()` to handle view patterns:
   ```c
   // After as-pattern check, before other pattern matching
   if (is_view_pattern(pat)) {
       Cell* transform;
       Cell* subpattern;
       extract_view_pattern(pat, &transform, &subpattern);

       // Evaluate transform(value) in current environment
       Cell* transformed = eval_transform(transform, value, env);

       if (is_error(transformed)) {
           // Transform failed, pattern doesn't match
           return create_bindings(); // empty
       }

       // Recursively match subpattern against transformed value
       return pattern_try_match(ctx, transformed, subpattern, env);
   }
   ```

2. Implement `eval_transform()`:
   ```c
   static Cell* eval_transform(Cell* transform, Cell* value, Env* env) {
       // If transform is a lambda or symbol, apply it
       if (is_lambda(transform) || is_symbol(transform)) {
           Cell* fn = is_symbol(transform) ?
                      env_lookup(env, transform) : transform;

           // Create argument list with single value
           Cell* args = cons(value, nil_cell());

           // Apply function
           return apply(ctx, fn, args);
       }

       // If transform is a quoted expression, evaluate it
       // This allows (→ (λ (x) ...) pattern) syntax
       Cell* expr = cons(transform, cons(value, nil_cell()));
       return eval_internal(ctx, env, expr);
   }
   ```

### Phase 3: Error Handling (15 mins)

1. Handle transform errors gracefully:
   - If transform throws error → pattern doesn't match, try next clause
   - If transform returns non-value → pattern doesn't match
   - Ensure no memory leaks from failed transforms

2. Add validation:
   - Transform must be callable (function or symbol)
   - Subpattern must be valid pattern

### Phase 4: Testing (60 mins)
**File:** `bootstrap/tests/test_pattern_view_patterns.test`

Create comprehensive test suite:

```scheme
; Test 1: Basic view pattern - list length
(⊨ :view-length #t
   (∇ (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅)))
      (⌜ (((→ # #3) :matched) (_ :failed)))))

; Test 2: View pattern with function
(≔ abs (λ (x) (? (< x #0) (⊖ #0 x) x)))
(⊨ :view-abs #t
   (∇ #-5 (⌜ (((→ abs #5) :matched) (_ :failed)))))

; Test 3: View pattern with literal mismatch
(⊨ :view-no-match #t
   (≡ (∇ (⟨⟩ #1 #2) (⌜ (((→ # #5) :matched) (_ :failed))))
      :failed))

; Test 4: View pattern with variable binding
(⊨ :view-bind #t
   (∇ (⟨⟩ #1 (⟨⟩ #2 ∅))
      (⌜ (((→ # n) n) (_ #0)))))  ; Binds length to n

; Test 5: View pattern with as-pattern
(⊨ :view-as-pattern #t
   (∇ (⟨⟩ #1 (⟨⟩ #2 ∅))
      (⌜ (((original @ (→ # #2)) (⟨⟩ original #2))
           (_ :failed)))))

; Test 6: View pattern with guard
(⊨ :view-guard #t
   (∇ #-15 (⌜ ((((→ abs n) | (> n #10)) :large)
               (_ :small)))))

; Test 7: Nested view patterns
(⊨ :view-nested #t
   (∇ "  hello  "
      (⌜ (((→ string-trim (→ string-length #5)) :five-char)
           (_ :other)))))

; Test 8: View pattern with pair destructuring
(⊨ :view-pair #t
   (∇ (⟨⟩ #3 #4)
      (⌜ (((→ (λ (p) (⊕ (◁ p) (▷ p))) #7) :sum-seven)
           (_ :other)))))

; Test 9: View pattern error handling (transform fails)
(⊨ :view-error-handling #t
   (∇ #-5 (⌜ (((→ (λ (x) (⚠ :error x)) #5) :matched)
              (_ :failed)))))  ; Should fail and try next clause

; Test 10: View pattern with ADT
(⊚≔ :Option (⌜ (:None)) (⌜ (:Some :value)))
(≔ extract-some (λ (opt)
   (∇ opt (⌜ (((⊚ :Option :Some v) v)
              ((⊚ :Option :None) ∅))))))
(⊨ :view-adt #t
   (∇ (⊚ :Option :Some #42)
      (⌜ (((→ extract-some #42) :matched) (_ :failed)))))

; Test 11: Multiple view patterns in same match
(⊨ :view-multiple #t
   (∇ (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅)))
      (⌜ (((→ # #5) :length-five)
           ((→ # #3) :length-three)
           ((→ # #1) :length-one)
           (_ :other)))))

; Test 12: View pattern with complex transformation
(≔ sum-list (λ (lst)
   (∇ lst (⌜ ((∅ #0)
              ((⟨⟩ h t) (⊕ h (sum-list t))))))))
(⊨ :view-sum #t
   (∇ (⟨⟩ #10 (⟨⟩ #20 (⟨⟩ #30 ∅)))
      (⌜ (((→ sum-list #60) :sum-sixty) (_ :other)))))
```

**Test Coverage Requirements:**
- ✅ Basic view patterns with literals
- ✅ View patterns with functions
- ✅ View patterns with variable binding
- ✅ Combination with as-patterns
- ✅ Combination with guards
- ✅ Nested view patterns
- ✅ Error handling (transform fails)
- ✅ View patterns with ADTs
- ✅ Multiple view patterns in same match
- ✅ Complex transformations

### Phase 5: Documentation (30 mins)

1. Update `SPEC.md`:
   - Add view pattern syntax to pattern matching section
   - Add examples showing all combinations
   - Document error handling behavior

2. Update `SESSION_HANDOFF.md`:
   - Mark Day 66 complete
   - Update pattern matching status to 100%
   - Add examples to "Recent Achievements"

3. Update `docs/planning/PATTERN_MATCHING_ENHANCEMENTS.md`:
   - Mark Phase 4 complete
   - Update progress to 4/4 (100%)

### Phase 6: Integration & Verification (30 mins)

1. Run full test suite:
   ```bash
   make clean && make
   ./run_tests.sh
   ```

2. Verify:
   - All existing tests still pass
   - All 12 new view pattern tests pass
   - No memory leaks
   - Clean compilation

3. Update primitive/test counts in SESSION_HANDOFF.md

## Expected Outcomes

**After Day 66:**
- ✅ View patterns fully implemented
- ✅ Pattern matching enhancement roadmap 100% complete (4/4 phases)
- ✅ 12 new comprehensive tests (58/59 → 70/71 tests passing, +12 view pattern tests)
- ✅ Pattern matching comparable to Haskell, OCaml, Rust, F#
- ✅ Foundation for advanced metaprogramming complete

**Pattern Matching Roadmap:**
- ✅ Phase 1: Guard Conditions (Day 58)
- ✅ Phase 2: As-Patterns (Day 59)
- ✅ Phase 3: Or-Patterns (Day 60)
- ✅ Phase 4: View Patterns (Day 66) **← COMPLETE!**

## Time Estimate

**Total:** ~3 hours

- Phase 1: Syntax Detection - 30 mins
- Phase 2: Transform Evaluation - 45 mins
- Phase 3: Error Handling - 15 mins
- Phase 4: Testing - 60 mins
- Phase 5: Documentation - 30 mins
- Phase 6: Integration - 30 mins

## Success Criteria

1. All 12 view pattern tests pass
2. No regressions in existing tests (maintain 98%+ pass rate)
3. Clean compilation with no warnings
4. Documentation updated in SPEC.md and SESSION_HANDOFF.md
5. Pattern matching roadmap marked 100% complete

## Alternative: If View Patterns Prove Complex

If view patterns take longer than expected or reveal architectural issues, we can pivot to:

**Alternative 1:** CFG/DFG enhancements (add graph algorithms, queries)
**Alternative 2:** Self-hosting improvements (work towards 100%)
**Alternative 3:** New high-value feature from metaprogramming roadmap

But view patterns should be straightforward given our solid pattern matching foundation.

---

**Ready to proceed!** Let's make pattern matching 100% complete! 🚀
