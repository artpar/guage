# Week 3 Roadmap: Pattern Matching
## Days 15-21 (2026-01-28 to 2026-02-03)

## Executive Summary

**Goal:** Implement comprehensive pattern matching for Guage
**Duration:** 7 days (Days 15-21)
**Prerequisites:** ADT support fixed (Day 13), Eval implemented (Day 14)
**Impact:** GAME CHANGER - Transforms usability and unlocks standard library

---

## Why Pattern Matching?

### Current Pain Points

**Without pattern matching:**
```scheme
; Manual list destructuring (ugly!)
(≔ length (λ (lst)
  (? (∅? lst)
     #0
     (⊕ #1 (length (▷ lst))))))

; Manual ADT handling (verbose!)
(≔ eval-expr (λ (expr)
  (? (⊚? expr :Expr :Num)
     (⊚→ expr :value)
     (? (⊚? expr :Expr :Add)
        (⊕ (eval-expr (⊚→ expr :left))
           (eval-expr (⊚→ expr :right)))
        (⚠ :unknown-expr expr)))))
```

**With pattern matching:**
```scheme
; Clean and readable!
(≔ length (λ (lst)
  (∇ lst
    [∅ #0]
    [(⟨⟩ _ tail) (⊕ #1 (length tail))])))

; Elegant ADT handling!
(≔ eval-expr (λ (expr)
  (∇ expr
    [:Num value → value]
    [:Add left right → (⊕ (eval-expr left) (eval-expr right))]
    [_ → (⚠ :unknown-expr expr)])))
```

**Benefits:**
- **5-10x less code** for common patterns
- **Eliminates boilerplate** type checking
- **Enforces exhaustiveness** (catches missing cases)
- **Enables declarative style** (what not how)
- **Foundation for stdlib** (map, filter, fold)

---

## Prerequisites

### ✅ Must Be Done Before Week 3

1. **Fix ADT Support (Day 13)** - CRITICAL ⚡
   - Fix ⊚≔ variant syntax
   - All ADT operations working
   - Test cases passing
   - **Status:** BLOCKED - Must fix today

2. **Fix :? Primitive (Day 13)** - HIGH 📋
   - Symbol type checking working
   - Important for pattern guards
   - **Status:** BLOCKED - Should fix today

3. **Implement Eval (Day 14)** - HIGH 📋
   - Not strictly required for pattern matching
   - But very useful for testing patterns
   - Can proceed without if needed
   - **Status:** Planned for Day 14

---

## Week 3 Phase Breakdown

### Phase 1: Design & Specification (Day 15) - 1 day

**Goal:** Finalize pattern matching syntax and semantics

#### Task 1.1: Pattern Syntax Design (3 hours)

**Patterns to support:**
```scheme
; Literals
#42                    ; Number literal
#t, #f                 ; Boolean literals
:symbol                ; Symbol literal
∅                      ; Nil literal

; Variables
x                      ; Bind to variable
_                      ; Wildcard (match anything, don't bind)

; Pairs
(⟨⟩ a b)              ; Match pair, bind head to a, tail to b
(⟨⟩ x _)              ; Match pair, bind head to x, ignore tail
(⟨⟩ _ (⟨⟩ y _))       ; Match nested pair

; ADT Patterns
[:Nil]                 ; Match Nil variant
[:Cons h t]            ; Match Cons, bind head and tail
[:Some x]              ; Match Some variant
[:None]                ; Match None variant

; Guards (optional, Phase 2)
[x | (> x #0)]         ; Match and guard condition
```

**Syntax decisions:**
- Use `∇` for match primitive
- Use `[pattern expression]` for clauses
- Use `_` for wildcard
- Use `|` for guards (future)

**Deliverable:** PATTERN_MATCHING_SPEC.md (detailed spec)

#### Task 1.2: Exhaustiveness Checking Design (2 hours)

**Rules:**
- All variants of ADT must be covered
- Wildcard `_` counts as catch-all
- Literals must be exhaustive for finite types
- Compiler warning for non-exhaustive matches

**Examples:**
```scheme
; Non-exhaustive (ERROR)
(∇ opt
  [:Some x → x])
; Missing: [:None]

; Exhaustive with wildcard (OK)
(∇ opt
  [:Some x → x]
  [_ → #0])

; Exhaustive with all variants (OK)
(∇ opt
  [:Some x → x]
  [:None → #0])
```

**Deliverable:** Exhaustiveness algorithm documented

#### Task 1.3: Test Case Design (3 hours)

**Test categories:**
1. Literal patterns (10 tests)
2. Variable patterns (10 tests)
3. Pair patterns (20 tests)
4. ADT patterns (20 tests)
5. Nested patterns (15 tests)
6. Wildcard patterns (10 tests)
7. Exhaustiveness (15 tests)

**Deliverable:** tests/pattern_matching.test (100 tests designed)

---

### Phase 2: Core Implementation (Days 16-18) - 3 days

#### Day 16: Pattern Matching Primitive (∇)

**Task 2.1: Implement match evaluator (4 hours)**

```c
// In eval.c
Cell* eval_match(Cell* expr, Cell* clauses, EvalContext* ctx) {
    // 1. Evaluate expression
    // 2. Try each pattern clause
    // 3. If match, bind variables and evaluate body
    // 4. If no match, error or continue
    // 5. Return first successful match
}
```

**Pattern matching algorithm:**
```
for each clause (pattern → body):
    bindings = try_match(value, pattern)
    if bindings == success:
        env = extend_env(current_env, bindings)
        return eval(body, env)
return error("no match")
```

**Task 2.2: Implement pattern matcher (4 hours)**

```c
// In primitives.c or pattern.c
MatchResult try_match(Cell* value, Cell* pattern) {
    // Returns: {success: bool, bindings: [(name, value)]}

    // Literal patterns
    if (is_literal(pattern))
        return {value == pattern, []}

    // Variable patterns
    if (is_var(pattern))
        return {true, [(pattern, value)]}

    // Wildcard
    if (pattern == "_")
        return {true, []}

    // Pair patterns
    if (is_pair_pattern(pattern))
        return match_pair(value, pattern)

    // ADT patterns
    if (is_adt_pattern(pattern))
        return match_adt(value, pattern)
}
```

**Deliverables:**
- ∇ primitive implemented
- Basic pattern matching working
- Tests passing for literals and variables

---

#### Day 17: Pair and ADT Patterns

**Task 2.3: Implement pair matching (3 hours)**

```c
MatchResult match_pair(Cell* value, Cell* pattern) {
    // Pattern: (⟨⟩ p1 p2)
    // Value: ⟨v1 v2⟩

    if (!is_pair(value))
        return {false, []};

    // Recursively match head and tail
    r1 = try_match(car(value), car(pattern));
    if (!r1.success) return {false, []};

    r2 = try_match(cdr(value), cdr(pattern));
    if (!r2.success) return {false, []};

    return {true, merge(r1.bindings, r2.bindings)};
}
```

**Task 2.4: Implement ADT matching (4 hours)**

```c
MatchResult match_adt(Cell* value, Cell* pattern) {
    // Pattern: [:Cons h t]
    // Value: ⊚[:List :Cons #1 ⊚[:List :Nil]]

    if (!is_struct(value))
        return {false, []};

    // Extract pattern variant
    variant = car(pattern);
    fields = cdr(pattern);

    // Check type and variant match
    if (struct_variant(value) != variant)
        return {false, []};

    // Match each field
    bindings = [];
    for (field_pattern in fields) {
        field_value = struct_get_field(value, field_name);
        result = try_match(field_value, field_pattern);
        if (!result.success) return {false, []};
        bindings = merge(bindings, result.bindings);
    }

    return {true, bindings};
}
```

**Deliverables:**
- Pair patterns working
- ADT patterns working
- Nested patterns working
- 50+ tests passing

---

#### Day 18: Wildcard and Edge Cases

**Task 2.5: Implement wildcard (_) (2 hours)**

```scheme
; Wildcard matches anything, doesn't bind
(∇ pair
  [(⟨⟩ x _) x])  ; Match pair, only bind first element

(∇ lst
  [(⟨⟩ _ tail) tail])  ; Match list, only bind tail
```

**Task 2.6: Error handling (3 hours)**

```scheme
; No matching clause
(∇ #5 [:x #10])
; → ⚠::no-match:#5

; Type mismatch
(∇ #5 [(⟨⟩ x y) x])
; → ⚠::match-failed:#5

; Non-exhaustive match (warning, not error)
(∇ opt [:Some x → x])
; → ⚠::non-exhaustive:[:None]
```

**Task 2.7: Nested patterns (3 hours)**

```scheme
; Deeply nested
(∇ expr
  [(⟨⟩ :add (⟨⟩ x (⟨⟩ :add (⟨⟩ y z))))
   (⊕ x (⊕ y z))])

; Multiple nesting
(∇ tree
  [:Leaf value → value]
  [:Node left right → (⊕ (sum left) (sum right))])
```

**Deliverables:**
- Wildcards working
- Error handling correct
- Nested patterns working
- All edge cases covered

---

### Phase 3: Structural Equality (Day 19) - 1 day

#### Task 3.1: Implement ≗ Primitive (4 hours)

**Structural equality vs deep equality:**

```scheme
; Deep equality (≟) - values equal
(≟ (⟨⟩ #1 #2) (⟨⟩ #1 #2))  ; → #t

; Structural equality (≗) - structure matches pattern
(≗ (⟨⟩ #1 #2) (⟨⟩ _ _))  ; → #t (pair structure)
(≗ [:Some #5] [:Some _])  ; → #t (ADT structure)
```

**Implementation:**
```c
Cell* prim_structural_eq(Cell* args, EvalContext* ctx) {
    // Similar to try_match but just returns bool
    // Checks if value matches pattern structure
    result = try_match(value, pattern);
    return result.success ? TRUE : FALSE;
}
```

**Use cases:**
- Type guards in pattern matching
- Schema validation
- Quick structure checks

**Deliverables:**
- ≗ primitive implemented
- 20 tests passing
- Documentation updated

---

### Phase 4: Exhaustiveness Checking (Day 20) - 1 day

#### Task 4.1: Implement exhaustiveness checker (6 hours)

**Algorithm:**
```
check_exhaustive(patterns, type):
    if has_wildcard(patterns):
        return OK  # Catch-all present

    if is_adt(type):
        variants = get_variants(type)
        covered = get_covered_variants(patterns)
        missing = variants - covered
        if missing != empty:
            warning("non-exhaustive", missing)

    if is_bool(type):
        if not (has_true(patterns) and has_false(patterns)):
            warning("non-exhaustive-bool")
```

**Examples:**
```scheme
; Good - exhaustive
(∇ opt
  [:Some x → x]
  [:None → #0])

; Good - wildcard catch-all
(∇ opt
  [:Some x → x]
  [_ → #0])

; Bad - missing :None
(∇ opt
  [:Some x → x])
; Warning: Non-exhaustive match, missing: [:None]

; Bad - missing #f
(∇ bool
  [#t → #1])
; Warning: Non-exhaustive match, missing: #f
```

**Deliverables:**
- Exhaustiveness checker working
- Compile-time warnings
- 15 tests for edge cases

---

### Phase 5: Documentation & Examples (Day 21) - 1 day

#### Task 5.1: Write PATTERN_MATCHING.md (3 hours)

**Content:**
- Complete pattern syntax reference
- Exhaustiveness rules
- Best practices
- Common patterns
- Performance notes

#### Task 5.2: Create examples (2 hours)

**examples/pattern_matching_demo.scm:**
```scheme
; List length
(≔ length (λ (lst)
  (∇ lst
    [∅ #0]
    [(⟨⟩ _ tail) (⊕ #1 (length tail))])))

; List map
(≔ map (λ (f lst)
  (∇ lst
    [∅ ∅]
    [(⟨⟩ head tail) (⟨⟩ (f head) (map f tail))])))

; List filter
(≔ filter (λ (pred lst)
  (∇ lst
    [∅ ∅]
    [(⟨⟩ head tail)
     (? (pred head)
        (⟨⟩ head (filter pred tail))
        (filter pred tail))])))

; List fold
(≔ fold (λ (f acc lst)
  (∇ lst
    [∅ acc]
    [(⟨⟩ head tail) (fold f (f acc head) tail)])))

; ADT evaluator
(≔ eval-expr (λ (expr)
  (∇ expr
    [:Num val → val]
    [:Add l r → (⊕ (eval-expr l) (eval-expr r))]
    [:Sub l r → (⊖ (eval-expr l) (eval-expr r))]
    [:Mul l r → (⊗ (eval-expr l) (eval-expr r))])))

; Binary tree operations
(≔ tree-sum (λ (tree)
  (∇ tree
    [:Leaf val → val]
    [:Node l r → (⊕ (tree-sum l) (tree-sum r))])))
```

#### Task 5.3: Update documentation (3 hours)

**Files to update:**
- SESSION_HANDOFF.md - Week 3 complete
- SPEC.md - Add pattern matching primitives
- README.md - Add pattern matching examples
- IMPLEMENTATION_STATUS.md - Mark features complete

**Deliverables:**
- Complete documentation
- 10+ working examples
- All docs updated

---

## Testing Strategy

### Test Coverage Goals

| Category | Tests | Status |
|----------|-------|--------|
| Literal patterns | 10 | Day 16 |
| Variable patterns | 10 | Day 16 |
| Wildcard patterns | 10 | Day 18 |
| Pair patterns | 20 | Day 17 |
| ADT patterns | 20 | Day 17 |
| Nested patterns | 15 | Day 18 |
| Exhaustiveness | 15 | Day 20 |
| Edge cases | 20 | Day 18 |
| **TOTAL** | **120** | **Week 3** |

---

## Timeline Summary

| Day | Phase | Tasks | Deliverables |
|-----|-------|-------|--------------|
| **15** | Design | Syntax, exhaustiveness, tests | Spec + test plan |
| **16** | Core | ∇ primitive, literals, vars | Basic matching |
| **17** | Patterns | Pairs, ADT | All patterns |
| **18** | Edge cases | Wildcards, nesting, errors | Robust matching |
| **19** | Equality | ≗ primitive | Structure checks |
| **20** | Safety | Exhaustiveness checker | Warnings |
| **21** | Docs | Documentation, examples | Complete |

**Total:** 7 days = 1 week

---

## Success Criteria

### Must Have ✅

- [ ] ∇ (match) primitive working
- [ ] Literal patterns working
- [ ] Variable patterns working
- [ ] Wildcard (_) working
- [ ] Pair patterns working
- [ ] ADT patterns working
- [ ] Nested patterns working
- [ ] ≗ (structural equality) working
- [ ] Exhaustiveness checking working
- [ ] 100+ tests passing
- [ ] Documentation complete

### Should Have 📋

- [ ] Error messages clear
- [ ] Performance acceptable
- [ ] Examples demonstrate power
- [ ] Standard library functions (map, filter, fold)

### Nice to Have 🎯

- [ ] Pattern guards (| condition)
- [ ] Or patterns (p1 | p2)
- [ ] As patterns (x @ pattern)
- [ ] Performance optimizations

---

## Risk Assessment

### Low Risk ✅

- Design is straightforward (based on ML/Haskell)
- Implementation pattern is well-known
- Tests can guide development
- Can iterate incrementally

### Medium Risk ⚠️

- ADT support must be working (prerequisite)
- Exhaustiveness checking can be complex
- Error messages need to be clear
- Integration with De Bruijn indices

### High Risk 🔴

- None identified

### Mitigation Strategy

1. **Test ADT thoroughly first** - Ensure ⊚≔, ⊚, ⊚→ work perfectly
2. **Start simple** - Literals and variables first
3. **Incremental testing** - Test each pattern type separately
4. **Clear error messages** - User feedback is critical

---

## Dependencies

### Requires (BEFORE Week 3)

1. ✅ Working lists
2. 🔴 **Working ADT** (must fix Day 13)
3. ⚠️ Working :? primitive (should fix Day 13)
4. ✅ Working conditionals
5. ⏳ Eval (helpful but not required)

### Enables (AFTER Week 3)

1. **Standard library** - map, filter, fold, etc.
2. **Macro system** - Pattern-based macros
3. **Type system** - Pattern-based type checking
4. **Usability** - 10x improvement in code clarity

---

## Performance Considerations

### Expected Performance

**Pattern matching should be:**
- O(1) for literal matches
- O(n) for list patterns (length n)
- O(d) for nested patterns (depth d)
- O(v) for ADT matches (v = variant count)

**Optimization opportunities:**
- Compile patterns to decision trees
- Reorder clauses for common cases first
- Cache pattern matching results
- Inline simple patterns

**Not a priority for Week 3** - Focus on correctness first

---

## Post-Week 3 Plans

### Week 4: Standard Library

**Build on pattern matching:**
```scheme
; List utilities
(≔ map ...)
(≔ filter ...)
(≔ fold ...)
(≔ zip ...)
(≔ take ...)
(≔ drop ...)

; Tree utilities
(≔ tree-map ...)
(≔ tree-fold ...)
(≔ tree-filter ...)

; Maybe/Option utilities
(≔ map-maybe ...)
(≔ bind-maybe ...)
(≔ from-maybe ...)
```

---

## Conclusion

**Week 3 Goal:** Complete, working pattern matching

**Impact:**
- **Usability:** 10x improvement in code clarity
- **Foundation:** Enables standard library
- **Progress:** Major milestone toward production-ready language

**Critical Path:**
1. Day 13: Fix ADT support (BLOCKING)
2. Day 14: Implement eval (HELPFUL)
3. Days 15-21: Pattern matching (TRANSFORMATIVE)

**Status:** READY TO PROCEED after Day 13 fixes

---

**Prepared by:** Claude Sonnet 4.5
**Date:** 2026-01-27
**Phase:** Week 3 Planning
**Next:** Fix critical issues (Day 13), then begin Week 3
