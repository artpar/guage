# Day 18: ADT Pattern Matching Implementation Plan

---
**Status:** CURRENT - Active work plan
**Created:** 2026-01-27
**Purpose:** Detailed plan for implementing ADT pattern matching
---

## Goal

Enable pattern matching on user-defined structures (⊙) and enums/ADTs (⊚).

## Why This Matters

- **Completes pattern matching** - All value types now matchable!
- **Real-world usability** - Can't build applications without destructuring custom types
- **Foundation for standard library** - List, Option, Result all need this
- **Type-safe destructuring** - Enforces correct field access at pattern level

## Current State

### ✅ Pattern Matching Working:
1. Wildcard patterns (`_`)
2. Literal patterns (`#42`, `:foo`, `#t`)
3. Variable patterns (`x` binds value)
4. Pair patterns (`(⟨⟩ a b)` destructures pairs)

### 📊 Test Status:
- 72 pattern tests passing (29 pair + 25 variable + 18 Day 15)
- 37 ADT tests passing (structures work, just not in patterns)

## What We Need to Implement

### Target Syntax

**Leaf Structure Patterns (⊙):**
```scheme
; Pattern: (⊙ type-tag field-patterns...)
(∇ point (⌜ (((⊙ :Point x y) (⊕ x y)))))  ; binds x, y from Point fields

; Examples:
(⊙ :Point x y)           ; matches Point{x, y}, binds both
(⊙ :Point #3 y)          ; matches Point{x: 3, y: any}, binds y
(⊙ :Point _ _)           ; matches any Point, no bindings
```

**Node/ADT Patterns (⊚):**
```scheme
; Pattern: (⊚ type-tag variant field-patterns...)
(∇ opt (⌜ (((⊚ :Option :Some v) v)
           ((⊚ :Option :None) #0))))  ; binds v from Some

; Examples:
(⊚ :Option :Some v)      ; matches Some(v), binds v
(⊚ :Option :None)        ; matches None, no bindings
(⊚ :List :Cons h t)      ; matches Cons(h, t), binds head and tail
```

## Implementation Strategy

### Phase 1: Leaf Structure Patterns (⊙) - 2-3 hours

**Step 1.1: Pattern Detection**
```c
/* Helper: Check if pattern is leaf structure pattern */
static bool is_leaf_struct_pattern(Cell* pattern) {
    /* Pattern: (⊙ :type field1 field2 ...) */
    if (!pattern || pattern->type != CELL_PAIR) return false;

    Cell* first = cell_car(pattern);
    if (!first || first->type != CELL_ATOM_SYMBOL) return false;
    if (strcmp(first->data.atom.symbol, "⊙") != 0) return false;

    /* Check second element is type tag (:Point, etc) */
    Cell* rest = cell_cdr(pattern);
    if (!rest || rest->type != CELL_PAIR) return false;

    Cell* type_tag = cell_car(rest);
    if (!type_tag || type_tag->type != CELL_ATOM_SYMBOL) return false;
    if (!is_keyword(type_tag->data.atom.symbol)) return false;

    return true;
}
```

**Step 1.2: Structure Field Extraction**
```c
/* Helper: Get field patterns from structure pattern */
static void extract_leaf_pattern_parts(Cell* pattern,
                                       Cell** type_tag,
                                       Cell** field_patterns) {
    /* pattern: (⊙ :type pat1 pat2 ...) */
    Cell* rest = cell_cdr(pattern);
    *type_tag = cell_car(rest);
    *field_patterns = cell_cdr(rest);  /* List of patterns */
}
```

**Step 1.3: Matching Algorithm**
```c
/* Match leaf structure against pattern */
if (is_leaf_struct_pattern(pattern)) {
    /* Value must be a structure */
    if (!value || value->type != CELL_STRUCT) return failure;

    /* Must be a leaf structure */
    if (cell_struct_kind(value) != STRUCT_LEAF) return failure;

    /* Extract pattern components */
    Cell* pattern_type_tag;
    Cell* field_patterns;
    extract_leaf_pattern_parts(pattern, &pattern_type_tag, &field_patterns);

    /* Match type tag */
    Cell* value_type = cell_struct_type_tag(value);
    if (!symbols_equal(value_type, pattern_type_tag)) return failure;

    /* Get field alist from value */
    Cell* fields = cell_struct_fields(value);

    /* Match each field pattern against corresponding field value */
    Cell* bindings = NULL;
    Cell* current_pattern = field_patterns;
    Cell* current_field = fields;

    while (current_pattern && current_pattern->type == CELL_PAIR) {
        if (!current_field || current_field->type != CELL_PAIR) {
            /* Pattern has more fields than value - mismatch */
            if (bindings) cell_release(bindings);
            return failure;
        }

        Cell* field_pat = cell_car(current_pattern);
        Cell* field_binding = cell_car(current_field);  /* (name . value) */
        Cell* field_value = cell_cdr(field_binding);

        /* Recursively match field value against field pattern */
        MatchResult field_match = pattern_try_match(field_value, field_pat);
        if (!field_match.success) {
            if (bindings) cell_release(bindings);
            return failure;
        }

        /* Merge field bindings */
        if (field_match.bindings) {
            bindings = merge_bindings(bindings, field_match.bindings);
        }

        current_pattern = cell_cdr(current_pattern);
        current_field = cell_cdr(current_field);
    }

    /* Check if value has more fields than pattern */
    if (current_field && current_field->type == CELL_PAIR) {
        /* Value has more fields than pattern - mismatch */
        if (bindings) cell_release(bindings);
        return failure;
    }

    MatchResult result = {.success = true, .bindings = bindings};
    return result;
}
```

**Step 1.4: Tests (15 tests)**
```scheme
; Basic leaf pattern matching
(⊙≔ :Point :x :y)
(≔ p (⊙ :Point #3 #4))

; Test 1: Simple pattern with variable bindings
(⊨ :test-leaf-simple #t
    (≡ (∇ p (⌜ (((⊙ :Point x y) (⊕ x y))))) #7))

; Test 2: Pattern with literal
(⊨ :test-leaf-literal #t
    (≡ (∇ p (⌜ (((⊙ :Point #3 y) y)))) #4))

; Test 3: Pattern with wildcard
(⊨ :test-leaf-wildcard #t
    (≡ (∇ p (⌜ (((⊙ :Point _ y) y)))) #4))

; Test 4: Wrong type fails
(⊨ :test-leaf-wrong-type #t
    (⚠? (∇ p (⌜ (((⊙ :Rectangle _ _) :no))))))

; Test 5: Wrong field count fails
(⊨ :test-leaf-wrong-count #t
    (⚠? (∇ p (⌜ (((⊙ :Point x) :no))))))

; ... 10 more tests
```

### Phase 2: Node/ADT Patterns (⊚) - 3-4 hours

**Step 2.1: Pattern Detection**
```c
/* Helper: Check if pattern is node/ADT pattern */
static bool is_node_struct_pattern(Cell* pattern) {
    /* Pattern: (⊚ :type :variant field1 field2 ...) */
    if (!pattern || pattern->type != CELL_PAIR) return false;

    Cell* first = cell_car(pattern);
    if (!first || first->type != CELL_ATOM_SYMBOL) return false;
    if (strcmp(first->data.atom.symbol, "⊚") != 0) return false;

    /* Check second element is type tag */
    Cell* rest = cell_cdr(pattern);
    if (!rest || rest->type != CELL_PAIR) return false;

    Cell* type_tag = cell_car(rest);
    if (!type_tag || type_tag->type != CELL_ATOM_SYMBOL) return false;
    if (!is_keyword(type_tag->data.atom.symbol)) return false;

    /* Check third element is variant tag */
    Cell* rest2 = cell_cdr(rest);
    if (!rest2 || rest2->type != CELL_PAIR) return false;

    Cell* variant = cell_car(rest2);
    if (!variant || variant->type != CELL_ATOM_SYMBOL) return false;
    if (!is_keyword(variant->data.atom.symbol)) return false;

    return true;
}
```

**Step 2.2: Matching Algorithm (similar to leaf but with variant check)**
```c
/* Match node/ADT against pattern */
if (is_node_struct_pattern(pattern)) {
    /* Value must be a structure */
    if (!value || value->type != CELL_STRUCT) return failure;

    /* Must be a node structure */
    if (cell_struct_kind(value) != STRUCT_NODE) return failure;

    /* Extract pattern components */
    Cell* pattern_type_tag;
    Cell* pattern_variant;
    Cell* field_patterns;
    extract_node_pattern_parts(pattern, &pattern_type_tag,
                               &pattern_variant, &field_patterns);

    /* Match type tag */
    Cell* value_type = cell_struct_type_tag(value);
    if (!symbols_equal(value_type, pattern_type_tag)) return failure;

    /* Match variant tag */
    Cell* value_variant = cell_struct_variant(value);
    if (!symbols_equal(value_variant, pattern_variant)) return failure;

    /* Match fields (same as leaf) */
    /* ... similar to leaf matching ... */
}
```

**Step 2.3: Tests (20 tests)**
```scheme
; Test enum-like ADTs (no fields)
(⊚≔ :Bool (⌜ (:True)) (⌜ (:False)))
(≔ t (⊚ :Bool :True))
(≔ f (⊚ :Bool :False))

; Test 1: Simple enum match
(⊨ :test-node-enum-true #t
    (≡ (∇ t (⌜ (((⊚ :Bool :True) :yes)
                ((⊚ :Bool :False) :no)))) :yes))

; Test 2: Enum fallthrough
(⊨ :test-node-enum-false #t
    (≡ (∇ f (⌜ (((⊚ :Bool :True) :yes)
                ((⊚ :Bool :False) :no)))) :no))

; Test with fields
(⊚≔ :Option (⌜ (:None)) (⌜ (:Some :value)))
(≔ some-42 (⊚ :Option :Some #42))
(≔ none (⊚ :Option :None))

; Test 3: ADT with field binding
(⊨ :test-node-some-bind #t
    (≡ (∇ some-42 (⌜ (((⊚ :Option :Some v) v)))) #42))

; Test 4: ADT with no fields
(⊨ :test-node-none #t
    (≡ (∇ none (⌜ (((⊚ :Option :None) :empty)))) :empty))

; Test 5: ADT wrong variant
(⊨ :test-node-wrong-variant #t
    (⚠? (∇ some-42 (⌜ (((⊚ :Option :None) :no))))))

; ... 15 more tests
```

### Phase 3: Integration & Documentation - 1-2 hours

**Step 3.1: Update pattern.c**
- Add helper functions
- Add cases to pattern_try_match
- Verify reference counting
- Test memory management

**Step 3.2: Update SPEC.md**
```markdown
### Pattern Matching (1) ✅

| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `∇` | `α → [[⌜pattern⌝ result]] → β` | Pattern match | ✅ DONE (Day 18) |

**Pattern Types:**
- `_` - Wildcard (matches anything)
- `#42`, `:foo` - Literals (exact match)
- `x`, `n` - Variables (bind value)
- `(⟨⟩ a b)` - Pairs (destructure)
- `(⊙ :Type ...)` - Leaf structures (Day 18 ✅)
- `(⊚ :Type :Variant ...)` - ADTs (Day 18 ✅)

**Examples:**
```scheme
; Leaf structure pattern
(⊙≔ :Point :x :y)
(≔ p (⊙ :Point #3 #4))
(∇ p (⌜ (((⊙ :Point x y) (⊕ x y)))))  ; → #7

; ADT pattern
(⊚≔ :Option (⌜ (:None)) (⌜ (:Some :value)))
(≔ opt (⊚ :Option :Some #42))
(∇ opt (⌜ (((⊚ :Option :Some v) v)
           ((⊚ :Option :None) #0))))  ; → #42
```
```

**Step 3.3: Update SESSION_HANDOFF.md**
- Add Day 18 summary
- Update test counts
- Update "What's Next"
- Update progress metrics

**Step 3.4: Archive scratchpad files**
- Move DAY_17_PAIR_PATTERNS_PLAN.md to archive
- Keep DAY_18 plan until session ends

## Test Strategy

### Test Categories (35 total)

**Leaf Structure Tests (15):**
1. Simple pattern with variables (3)
2. Pattern with literals (3)
3. Pattern with wildcards (2)
4. Wrong type (2)
5. Wrong field count (2)
6. Nested structures (3)

**Node/ADT Tests (20):**
1. Simple enum patterns (4)
2. ADT with single field (4)
3. Recursive ADTs (List) (5)
4. Wrong variant (2)
5. Wrong type (2)
6. Multiple variants (3)

### Success Criteria

**Must Have:**
- ✅ All 35 tests passing
- ✅ Memory management verified
- ✅ No regressions in existing tests
- ✅ SPEC.md updated
- ✅ SESSION_HANDOFF.md updated

**Nice to Have:**
- ✅ Performance benchmarks
- ✅ Complex nested examples
- ✅ Integration with existing patterns

## Timeline

- **Hours 0-1:** Read code, understand structure representation
- **Hours 1-3:** Implement leaf structure patterns + tests
- **Hours 3-6:** Implement node/ADT patterns + tests
- **Hours 6-7:** Integration, documentation, edge cases
- **Hours 7-8:** Final verification, commit

**Estimated:** 8 hours (SESSION_HANDOFF says 8-10, we'll aim for 6-8)

## Risks & Mitigations

**Risk 1: Field ordering complexity**
- Mitigation: Fields stored as alist, access by name not index

**Risk 2: Variant matching complexity**
- Mitigation: Simple symbol equality check

**Risk 3: Binding merge edge cases**
- Mitigation: Use existing merge_bindings (already handles all cases)

**Risk 4: Memory leaks**
- Mitigation: Careful reference counting, test with valgrind

## Dependencies

**Required:**
- ✅ pattern.c/h (existing)
- ✅ cell.h CELL_STRUCT representation (existing)
- ✅ Structure primitives working (⊙, ⊚ all functional)
- ✅ Binding merge algorithm (existing from Day 17)

**No blockers!**

## Next Steps After This

**Day 19:** (if time) Exhaustiveness checking
**Days 20-21:** Pattern matching documentation and examples

---

**Ready to implement!** 🚀

Let's start with Phase 1: Leaf Structure Patterns.
