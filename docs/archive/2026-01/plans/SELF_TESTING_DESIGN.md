# Self-Testing Design: Tests as First-Class Auto-Generated Values

## Problem Statement

**Current:** Tests written manually in external `.test` files
**Should be:** Tests auto-generated from function definitions, just like docs/CFG/DFG

## Core Insight

If a function exists, its tests must exist. They're not separate artifacts - they're **derived properties** of the function definition.

```scheme
(≔ ! (λ (n) (? (≡ n #0) #1 (⊗ n (! (⊖ n #1))))))

; Auto-generates:
; - ⌂ !      → Documentation (description)
; - ⌂∈ !     → Type signature (ℕ → ℕ)
; - ⌂≔ !     → Dependencies (?, ≡, ⊗, ⊖)
; - ⌂⊛ !     → Source code (AST)
; - ⌂⟿ !     → Control flow graph
; - ⌂⇝ !     → Data flow graph
; - ⌂⊨ !     → TESTS (auto-generated!)  ← MISSING!
```

## Auto-Test Generation Strategy

### Level 1: Type-Based Tests (IMMEDIATE)

**From type signature, generate constraint tests:**

```scheme
; Type: ℕ → ℕ
; Auto-generate:
(⊨ :input-type-check #t (ℕ? #5))              ; Input must be number
(⊨ :output-type-check #t (ℕ? (! #5)))        ; Output must be number
(⊨ :handles-zero #t (ℕ? (! #0)))             ; Base case
```

### Level 2: Property-Based Tests (NEAR-TERM)

**From structure analysis, generate property tests:**

```scheme
; Recursive function → test base case + recursive case
(⊨ :base-case #1 (! #0))                      ; Base case value
(⊨ :recursive-relation #t                      ; Recursive property
  (≡ (! #5) (⊗ #5 (! #4))))

; Sorted type → ordering property
(⊨ :preserves-length #t
  (≡ (length xs) (length (sort xs))))
(⊨ :ordering-property #t
  (∀ i (≤ (nth (sort xs) i) (nth (sort xs) (+ i 1)))))
```

### Level 3: Mutation Testing (FUTURE)

**Generate mutants and verify tests catch them:**

```scheme
; Original
(≔ max (λ (a b) (? (> a b) a b)))

; Mutants
(≔ max′₁ (λ (a b) (? (< a b) a b)))  ; Wrong comparison
(≔ max′₂ (λ (a b) (? (> a b) b a)))  ; Swapped return

; Verify tests catch mutants
(⊙⊗ max)  ; → Mutation score: 100%
```

## Implementation Plan

### Phase 1: Add `⌂⊨` Primitive (Days 10-11)

**File:** `primitives.c`

```c
/* ⌂⊨ - Auto-generate tests for symbol */
Cell* prim_doc_tests(Cell* args) {
    Cell* quoted = arg1(args);
    if (!cell_is_pair(quoted)) {
        return cell_error("⌂⊨ requires a quoted function name", cell_nil());
    }

    Cell* symbol_cell = cell_car(quoted);
    if (!cell_is_symbol(symbol_cell)) {
        return cell_error("⌂⊨ requires a symbol argument", quoted);
    }

    const char* name = cell_get_symbol(symbol_cell);
    EnvEntry* entry = env_lookup(global_env, name);

    if (!entry || !entry->value) {
        return cell_error("⌂⊨ function not found", quoted);
    }

    if (!cell_is_lambda(entry->value)) {
        return cell_error("⌂⊨ argument must be a function", quoted);
    }

    // Generate tests based on type signature and structure
    return generate_tests_for_function(name, entry->value, entry->type_sig);
}

// Register in primitives table
{"⌂⊨", prim_doc_tests, 1, {"Auto-generate tests for function", ":symbol → [tests]"}},
```

**Test generation logic:**

```c
Cell* generate_tests_for_function(const char* name, Cell* lambda, const char* type_sig) {
    Cell* tests = cell_nil();

    // 1. Type-based tests
    tests = cons(generate_type_tests(name, lambda, type_sig), tests);

    // 2. Structure-based tests
    tests = cons(generate_structure_tests(name, lambda), tests);

    // 3. Edge case tests
    tests = cons(generate_edge_case_tests(name, lambda, type_sig), tests);

    return tests;
}
```

### Phase 2: Type-Based Test Generation (Day 12)

**Strategy:**

1. Parse type signature (e.g., "ℕ → ℕ → ℕ")
2. Generate type constraint tests:
   - Input type checks
   - Output type checks
   - Nil handling
   - Error propagation

**Example:**

```c
Cell* generate_type_tests(const char* name, Cell* lambda, const char* type_sig) {
    Cell* tests = cell_nil();

    // Parse "ℕ → ℕ → ℕ"
    int arity = count_arrows(type_sig);
    char* input_types[MAX_ARITY];
    char* output_type;
    parse_type_signature(type_sig, input_types, &output_type);

    // Generate input type test
    // (⊨ :input-type #t (ℕ? #5))
    tests = cons(make_test_case(
        make_symbol(":input-type"),
        cell_true(),
        make_type_check(input_types[0], cell_number(5))
    ), tests);

    // Generate output type test
    // (⊨ :output-type #t (ℕ? (func #5)))
    Cell* call = make_call(make_symbol(name), cell_number(5));
    tests = cons(make_test_case(
        make_symbol(":output-type"),
        cell_true(),
        make_type_check(output_type, call)
    ), tests);

    return tests;
}
```

### Phase 3: Structure-Based Tests (Day 13)

**Analyze AST to generate tests:**

1. **Conditional branches:** Test both branches
2. **Recursion:** Test base case + recursive case
3. **Arithmetic:** Test edge cases (0, 1, negative)
4. **Lists:** Test empty list, single element, multiple elements

**Example:**

```c
Cell* generate_structure_tests(const char* name, Cell* lambda) {
    Cell* tests = cell_nil();
    Cell* body = cell_lambda_get_body(lambda);

    // Analyze body structure
    if (is_conditional(body)) {
        // Test both branches
        tests = cons(generate_branch_tests(name, body), tests);
    }

    if (is_recursive(name, body)) {
        // Test base case
        tests = cons(generate_base_case_test(name, body), tests);
        // Test recursive case
        tests = cons(generate_recursive_test(name, body), tests);
    }

    if (uses_arithmetic(body)) {
        // Test arithmetic edge cases
        tests = cons(generate_arithmetic_tests(name), tests);
    }

    return tests;
}
```

### Phase 4: Property-Based Tests (Day 14)

**Generate property tests from refined types:**

```c
Cell* generate_property_tests(const char* name, RefinementType* refinement) {
    Cell* tests = cell_nil();

    // For Sorted type: ∀ i (≤ (nth xs i) (nth xs (+ i 1)))
    if (is_sorted_type(refinement)) {
        tests = cons(generate_ordering_property_test(name), tests);
    }

    // For Pure functions: same input → same output
    if (is_pure_function(refinement)) {
        tests = cons(generate_purity_test(name), tests);
    }

    // For Total functions: terminates for all valid inputs
    if (is_total_function(refinement)) {
        tests = cons(generate_termination_test(name), tests);
    }

    return tests;
}
```

## Usage Examples

### Basic Function

```scheme
; Define factorial
(≔ ! (λ (n) (? (≡ n #0) #1 (⊗ n (! (⊖ n #1))))))

; Auto-generated tests available immediately
(⌂⊨ (⌜ !))  ; → List of tests

; Returns:
; (⟨⟩ (⊨ :input-type #t (ℕ? #5))
;     (⊨ :output-type #t (ℕ? (! #5)))
;     (⊨ :base-case #1 (! #0))
;     (⊨ :recursive-case #120 (! #5))
;     (⊨ :handles-one #1 (! #1)))
```

### Run Auto-Generated Tests

```scheme
; Get tests
(≔ tests (⌂⊨ (⌜ !)))

; Run all tests
(≔ run-tests (λ (tests)
  (? (∅? tests)
     ∅
     (⟨⟩ (◁ tests) (run-tests (▷ tests))))))

(run-tests tests)  ; → All test results
```

### Test Coverage Analysis

```scheme
; Get test coverage
(≔ coverage (⌂◎ (⌜ !)))  ; Future: coverage primitive

; Returns:
; ⟨:lines-total 5
;  :lines-covered 5
;  :branches-total 2
;  :branches-covered 2
;  :coverage-percent #100⟩
```

## Benefits

### 1. Tests Can't Be Missing

Function definition → Tests exist automatically

### 2. Tests Always Match Implementation

Change function → Tests regenerate automatically

### 3. Zero Boilerplate

No test framework setup, no test file management

### 4. Queryable Test Results

```scheme
; Find failing tests
(filter (λ (t) (⚠? t)) (⌂⊨ (⌜ my-func)))

; Get test count
(length (⌂⊨ (⌜ my-func)))

; Analyze test quality
(⊙⊗ my-func)  ; Mutation score
```

### 5. AI Can Reason About Tests

Since tests are first-class values, AI can:
- Generate missing test cases
- Identify under-tested code paths
- Suggest edge cases
- Verify test completeness

## Integration with Existing Features

### With Auto-Documentation

```scheme
(≔ ! (λ (n) ...))

; Get everything about the function
(⌂ (⌜ !))      ; → Description
(⌂∈ (⌜ !))     ; → Type signature
(⌂≔ (⌜ !))     ; → Dependencies
(⌂⊛ (⌜ !))     ; → Source code
(⌂⟿ (⌜ !))     ; → CFG
(⌂⇝ (⌜ !))     ; → DFG
(⌂⊨ (⌜ !))     ; → Tests  ← NEW!
```

### With CFG/DFG

```scheme
; Use CFG to generate path tests
(≔ cfg (⌂⟿ (⌜ my-func)))
(≔ paths (extract-paths cfg))
(≔ tests (map path-to-test paths))
```

### With Error Handling

```scheme
; Tests automatically handle errors
(⌂⊨ (⌜ safe-div))  ; → Includes div-by-zero test

; Returns:
; (⊨ :normal-case #2 (safe-div #6 #3))
; (⊨ :div-by-zero #t (⚠? (safe-div #5 #0)))
```

## Future Enhancements

### Mutation Testing (`⊙⊗`)

```scheme
; Generate mutants
(≔ mutants (⊙⊗ my-func))

; Run tests against mutants
(≔ score (test-mutation-score (⌂⊨ (⌜ my-func)) mutants))
```

### Property-Based Testing (`⊙?`)

```scheme
; Generate random test cases from type
(≔ random-tests (⊙? my-func Sorted #100))

; Run property tests
(all? (map run-test random-tests))
```

### Test Minimization

```scheme
; Find minimal failing test case
(≔ minimal (minimize-test failing-test))
```

## Implementation Timeline

- **Day 10:** Design + `⌂⊨` primitive scaffolding
- **Day 11:** Type-based test generation
- **Day 12:** Structure-based test generation
- **Day 13:** Integration with existing primitives
- **Day 14:** Property-based tests (future work)

## Success Criteria

✅ `(⌂⊨ (⌜ func))` returns list of tests for any function
✅ Auto-generated tests execute correctly
✅ Tests cover: types, base cases, recursive cases, edge cases
✅ Tests integrate with existing `⊨` primitive
✅ Documentation updated with examples

---

**Status:** Design complete, ready for implementation
**Next Step:** Implement `⌂⊨` primitive in primitives.c
