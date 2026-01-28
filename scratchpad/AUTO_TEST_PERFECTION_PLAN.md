# Auto-Test Generation: Path to Perfection
**Priority:** ZERO (Highest)
**Status:** CRITICAL - Must be perfect before continuing
**Date:** 2026-01-27

## Current Reality

**⌂⊨ (auto-test) works for:** ~15% of primitives (arithmetic, predicates)
**⌂⊨ (auto-test) fails for:** ~85% of primitives (pattern matching, pairs, control flow, etc.)

**This is UNACCEPTABLE** for an ultralanguage where testing is first-class.

## The Vision

```scheme
; EVERY primitive should auto-generate comprehensive tests
(⌂⊨ (⌜ ∇))   ; → Full test suite for pattern matching
(⌂⊨ (⌜ ⟨⟩))  ; → Full test suite for pairs
(⌂⊨ (⌜ ?))   ; → Full test suite for conditionals
(⌂⊨ (⌜ λ))   ; → Full test suite for lambdas
; ... and so on for ALL 63 primitives
```

## Root Problem: Hardcoded Pattern Matching

Current implementation (primitives.c:1520-1656) uses string matching:

```c
if (strstr(type_sig, "ℕ → ℕ → ℕ")) {
    // Generate tests for binary arithmetic
}
else if (strstr(type_sig, "α → 𝔹")) {
    // Generate tests for predicates
}
// Only 2 patterns supported!
```

**This doesn't scale.** We have dozens of type signature patterns.

## Solution: Type-Directed Test Generation

### Phase 1: Parse Type Signatures (2-3 hours)

Create a proper type parser:

```c
typedef enum {
    TYPE_VAR,      // α, β, γ
    TYPE_NUM,      // ℕ
    TYPE_BOOL,     // 𝔹
    TYPE_NIL,      // ∅
    TYPE_SYMBOL,   // :symbol
    TYPE_PAIR,     // ⟨α β⟩
    TYPE_LIST,     // [α]
    TYPE_FUNC,     // α → β
    TYPE_PATTERN,  // [[pattern result]]
    TYPE_ERROR,    // ⚠
    // ... more as needed
} TypeKind;

typedef struct TypeExpr {
    TypeKind kind;
    union {
        char var_name;              // For TYPE_VAR: 'α'
        struct {
            struct TypeExpr* from;
            struct TypeExpr* to;
        } func;                     // For TYPE_FUNC
        struct {
            struct TypeExpr* car;
            struct TypeExpr* cdr;
        } pair;                     // For TYPE_PAIR
        struct TypeExpr* elem;      // For TYPE_LIST
    } data;
} TypeExpr;

/* Parse "α → [[pattern result]] → β" into TypeExpr tree */
TypeExpr* type_parse(const char* sig);
```

### Phase 2: Generate Tests from Type Structure (3-4 hours)

```c
/* Generate tests based on type structure */
Cell* generate_tests_from_type(const char* prim_name, TypeExpr* type) {
    Cell* tests = cell_nil();

    switch (type->kind) {
        case TYPE_FUNC:
            // Generate tests for function
            tests = generate_function_tests(prim_name, type);
            break;

        case TYPE_VAR:
            // Generic type - test with multiple values
            tests = generate_polymorphic_tests(prim_name, type);
            break;

        // ... handle all type kinds
    }

    return tests;
}

Cell* generate_function_tests(const char* name, TypeExpr* func_type) {
    Cell* tests = cell_nil();

    // Analyze argument types
    TypeExpr* arg_type = func_type->data.func.from;
    TypeExpr* ret_type = func_type->data.func.to;

    // Generate: type conformance test
    tests = add_test(tests, name, arg_type, ret_type, TEST_TYPE_CONFORM);

    // Generate: edge case tests based on arg type
    if (arg_type->kind == TYPE_NUM) {
        tests = add_test(tests, name, arg_type, ret_type, TEST_ZERO);
        tests = add_test(tests, name, arg_type, ret_type, TEST_NEGATIVE);
    }

    // Generate: error case tests
    tests = add_test(tests, name, arg_type, ret_type, TEST_WRONG_TYPE);

    return tests;
}
```

### Phase 3: Special Cases for Complex Types (2-3 hours)

```c
/* Pattern matching: α → [[pattern result]] → β */
Cell* generate_pattern_match_tests(const char* name) {
    Cell* tests = cell_nil();

    // Test 1: Wildcard pattern
    // (⊨ :pattern-wildcard #t
    //    (≡ (∇ #42 (⟨⟩ (⟨⟩ (⌜ _) (⟨⟩ :ok ∅)) ∅)) :ok))
    tests = add_wildcard_test(tests, name);

    // Test 2: Literal match
    tests = add_literal_match_test(tests, name);

    // Test 3: No match error
    tests = add_no_match_error_test(tests, name);

    // Test 4: Multiple clauses
    tests = add_multiple_clauses_test(tests, name);

    return tests;
}

/* Pair construction: α → β → ⟨α β⟩ */
Cell* generate_pair_tests(const char* name) {
    Cell* tests = cell_nil();

    // Test 1: Creates pair
    // (⊨ :pair-creates #t (⟨⟩? (⟨⟩ #1 #2)))
    tests = add_creates_pair_test(tests, name);

    // Test 2: First element accessible
    tests = add_car_test(tests, name);

    // Test 3: Second element accessible
    tests = add_cdr_test(tests, name);

    return tests;
}
```

### Phase 4: Integration & Testing (1-2 hours)

```c
Cell* prim_doc_tests(Cell* args) {
    Cell* name = arg1(args);
    if (!cell_is_symbol(name)) {
        return cell_error("⌂⊨ requires a symbol argument", name);
    }

    const char* sym = cell_get_symbol(name);

    /* Check if it's a primitive */
    const Primitive* prim = primitive_lookup_by_name(sym);
    if (prim) {
        const char* type_sig = prim->doc.type_signature;

        /* Parse type signature */
        TypeExpr* type = type_parse(type_sig);
        if (!type) {
            return cell_error("⌂⊨ invalid type signature", name);
        }

        /* Generate tests from type structure */
        Cell* tests = generate_tests_from_type(sym, type);

        /* Free type expression */
        type_free(type);

        return tests;
    }

    /* User functions... */
    // ... existing code ...
}
```

## Implementation Plan

### Task Breakdown

**Phase 1: Type Parser (3 hours)**
- [ ] Create type.h and type.c files
- [ ] Define TypeExpr structure
- [ ] Implement type_parse() function
- [ ] Test with all 63 primitive signatures
- [ ] Handle Unicode symbols (→, ℕ, 𝔹, etc.)

**Phase 2: Test Generators (4 hours)**
- [ ] Implement generate_tests_from_type()
- [ ] Implement generate_function_tests()
- [ ] Implement generate_polymorphic_tests()
- [ ] Add test case builders (add_test, etc.)

**Phase 3: Special Type Handlers (3 hours)**
- [ ] Pattern matching types: [[pattern result]]
- [ ] Pair types: ⟨α β⟩
- [ ] List types: [α]
- [ ] Conditional types
- [ ] Lambda types

**Phase 4: Integration (2 hours)**
- [ ] Update prim_doc_tests() to use type parser
- [ ] Remove hardcoded string matching
- [ ] Test with ALL primitives
- [ ] Verify 100% coverage

**Phase 5: Verification (2 hours)**
- [ ] Run (⌂⊨ (⌜ prim)) for all 63 primitives
- [ ] Verify tests are comprehensive
- [ ] Verify tests are runnable
- [ ] Document the system

**Total Estimated Time:** 14 hours (~2 days)

## Success Criteria

### Must Have:
- ✅ (⌂⊨ (⌜ ∇)) generates comprehensive tests
- ✅ (⌂⊨ (⌜ ⟨⟩)) generates comprehensive tests
- ✅ (⌂⊨ (⌜ ?)) generates comprehensive tests
- ✅ ALL 63 primitives generate non-empty tests
- ✅ No hardcoded string matching
- ✅ Type-directed generation

### Should Have:
- ✅ Generated tests are runnable
- ✅ Generated tests cover edge cases
- ✅ Generated tests check error conditions
- ✅ System is extensible (easy to add new type patterns)

### Nice to Have:
- ⏳ Property-based test generation
- ⏳ Exhaustiveness checking
- ⏳ Test minimization

## Architecture

```
User calls: (⌂⊨ (⌜ ∇))
     ↓
prim_doc_tests()
     ↓
type_parse("α → [[pattern result]] → β")
     ↓
TypeExpr tree: FUNC(VAR α, FUNC(PATTERN(...), VAR β))
     ↓
generate_tests_from_type()
     ↓
dispatch based on type structure
     ↓
generate_pattern_match_tests()
     ↓
build test cases as Cell structures
     ↓
return: (⟨⟩ test1 (⟨⟩ test2 (⟨⟩ test3 ∅)))
```

## Files to Create/Modify

**New Files:**
- `bootstrap/type.h` - Type expression definitions
- `bootstrap/type.c` - Type parser and utilities
- `bootstrap/testgen.h` - Test generation interface
- `bootstrap/testgen.c` - Test generation implementations

**Modified Files:**
- `bootstrap/primitives.c` - Update prim_doc_tests()
- `bootstrap/Makefile` - Add type.c and testgen.c

**Test Files:**
- `tests/test_auto_testing.scm` - Verify ⌂⊨ for all primitives

## Next Immediate Steps

1. **Create type.h** - Define TypeExpr structure
2. **Create type.c** - Implement type_parse()
3. **Test parser** - Parse all 63 primitive signatures
4. **Create testgen.c** - Implement test generators
5. **Integrate** - Update prim_doc_tests()
6. **Verify** - Test all primitives

## Timeline

**Day 15 (Today):** Type parser + basic test generation
**Day 16 (Tomorrow):** Special cases + integration + verification

**Goal:** Auto-testing PERFECTION before resuming pattern matching work.

---

**This is the foundation of Guage's ultralanguage vision. Everything else waits until this is perfect.**
