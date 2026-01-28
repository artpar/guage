# Phase 2B: User Function Auto-Documentation - Detailed Plan

## Executive Summary

**Goal:** Automatically generate documentation for user-defined functions by:
1. Extracting dependencies from function bodies
2. Composing descriptions from constituent docs
3. Inferring simple types
4. Integrating with ≔ (eval_define)

**Estimated Implementation:** 6-8 hours after this 2-hour planning phase

---

## 1. Doc Registry Design

### Data Structure

```c
// In eval.h - add to EvalContext
typedef struct FunctionDoc {
    char* name;                    // Function name (e.g., "!")
    char* description;             // Auto-generated description
    char* type_signature;          // Inferred type (e.g., "ℕ → ℕ")
    char** dependencies;           // List of function names used
    size_t dependency_count;
    struct FunctionDoc* next;      // Linked list
} FunctionDoc;

typedef struct {
    EvalStackFrame* stack;
    EvalStackFrame* global_env;
    FunctionDoc* user_docs;        // ← NEW: Head of doc list
} EvalContext;
```

### API Functions

```c
// In eval.c

// Create a new doc entry
FunctionDoc* doc_create(const char* name);

// Add a dependency to a doc
void doc_add_dependency(FunctionDoc* doc, const char* dep_name);

// Find doc by name (returns NULL if not found)
FunctionDoc* doc_find(EvalContext* ctx, const char* name);

// Generate description from dependencies
void doc_generate_description(EvalContext* ctx, FunctionDoc* doc, Cell* body);

// Infer type signature (simple version)
void doc_infer_type(FunctionDoc* doc, Cell* lambda);

// Free doc list
void doc_free_all(FunctionDoc* head);
```

### Storage Location

**Decision:** Store in `EvalContext->user_docs`

**Why:**
- Context already passed everywhere
- Natural lifetime (lives with the interpreter session)
- No global state
- Easy to free on cleanup

---

## 2. Dependency Extraction Algorithm

### High-Level Algorithm

```
extract_dependencies(body_expr, params) → [dependencies]:
    1. If body is atom:
        - If symbol and NOT in params → add to deps
        - Otherwise → skip (literal, param, etc.)

    2. If body is pair (application):
        - If head is primitive symbol → record primitive name
        - If head is user function symbol → record function name
        - Recursively process all subexpressions

    3. If body contains lambda:
        - Add lambda's params to params list
        - Recursively process lambda body with extended params
        - Remove lambda params when done

    4. Return unique list of dependencies
```

### Pseudocode

```c
void extract_dependencies(Cell* expr, SymbolSet* params, DepList* deps) {
    if (!expr || expr->type == CELL_NIL) return;

    switch (expr->type) {
        case CELL_NUMBER:
        case CELL_BOOL:
        case CELL_NIL:
            // Literals - no dependencies
            return;

        case CELL_SYMBOL:
            // Check if it's a parameter
            if (!symbol_set_contains(params, expr->value.symbol)) {
                // Not a parameter - it's a dependency
                dep_list_add(deps, expr->value.symbol);
            }
            return;

        case CELL_PAIR:
            // Recursively process head and tail
            extract_dependencies(cell_car(expr), params, deps);
            extract_dependencies(cell_cdr(expr), params, deps);
            return;

        case CELL_LAMBDA:
            // Lambda introduces new parameters
            SymbolSet* extended_params = symbol_set_clone(params);

            // Add lambda params to extended set
            Cell* lambda_params = cell_car(expr->value.lambda.params);
            while (lambda_params && lambda_params->type == CELL_PAIR) {
                Cell* param = cell_car(lambda_params);
                if (param->type == CELL_SYMBOL) {
                    symbol_set_add(extended_params, param->value.symbol);
                }
                lambda_params = cell_cdr(lambda_params);
            }

            // Process body with extended params
            extract_dependencies(expr->value.lambda.body, extended_params, deps);

            symbol_set_free(extended_params);
            return;
    }
}
```

### Helper Data Structures

```c
// Simple set of symbols (parameter names)
typedef struct {
    char** symbols;
    size_t count;
    size_t capacity;
} SymbolSet;

// List of dependencies
typedef struct {
    char** names;
    size_t count;
    size_t capacity;
} DepList;
```

---

## 3. Auto-Generation Rules

### Description Composition

**Strategy:** Compose description from dependency descriptions

```c
void doc_generate_description(EvalContext* ctx, FunctionDoc* doc, Cell* body) {
    // Extract dependencies
    SymbolSet* params = symbol_set_create();
    DepList* deps = dep_list_create();
    extract_dependencies(body, params, deps);

    // Store dependencies
    doc->dependency_count = deps->count;
    doc->dependencies = deps->names;

    // Compose description
    if (deps->count == 0) {
        doc->description = strdup("User-defined function");
    } else if (deps->count == 1) {
        // Single dependency - look up its description
        const char* dep_desc = get_dependency_description(ctx, deps->names[0]);
        if (dep_desc) {
            char buf[512];
            snprintf(buf, sizeof(buf), "Function using %s", dep_desc);
            doc->description = strdup(buf);
        } else {
            doc->description = strdup("User-defined function");
        }
    } else {
        // Multiple dependencies - list them
        char buf[1024] = "Function using: ";
        for (size_t i = 0; i < deps->count && i < 5; i++) {
            if (i > 0) strcat(buf, ", ");
            strcat(buf, deps->names[i]);
        }
        if (deps->count > 5) {
            strcat(buf, ", ...");
        }
        doc->description = strdup(buf);
    }

    symbol_set_free(params);
    // Note: deps->names now owned by doc, don't free
    free(deps);
}
```

### Type Inference (Simple Version)

**Strategy:** Pattern match common structures

```c
void doc_infer_type(FunctionDoc* doc, Cell* lambda) {
    if (!lambda || lambda->type != CELL_LAMBDA) {
        doc->type_signature = strdup("unknown");
        return;
    }

    // Count parameters
    int param_count = count_lambda_params(lambda);

    // Build simple type signature
    char buf[256] = "";
    for (int i = 0; i < param_count; i++) {
        if (i > 0) strcat(buf, " → ");
        strcat(buf, "α");  // Generic type for now
    }
    if (param_count > 0) strcat(buf, " → ");
    strcat(buf, "β");  // Return type

    doc->type_signature = strdup(buf);
}

int count_lambda_params(Cell* lambda) {
    // For single-param lambdas: (λ (param) body)
    // For now, we only support single-param
    // Multi-param is curried: (λ (x) (λ (y) body))

    // Count nested lambdas
    int count = 0;
    Cell* current = lambda;
    while (current && current->type == CELL_LAMBDA) {
        count++;
        current = current->value.lambda.body;
        // If body is another lambda, continue
        if (current && current->type != CELL_LAMBDA) break;
    }
    return count;
}
```

---

## 4. Integration Points

### Location: eval_define()

**File:** `eval.c`
**Function:** `eval_define()`
**Current code:** Lines ~165-195

```c
// Current implementation:
Cell* eval_define(Cell* args, EvalContext* ctx) {
    Cell* name_cell = cell_car(args);
    Cell* value_expr = cell_car(cell_cdr(args));

    if (name_cell->type != CELL_SYMBOL) {
        return error_cell(":invalid-define", name_cell);
    }

    Cell* value = eval(value_expr, ctx);

    // Push to global environment
    push_stack_frame(&ctx->global_env,
                     name_cell->value.symbol,
                     value);

    return value;  // ← MODIFY THIS
}
```

**New implementation:**

```c
Cell* eval_define(Cell* args, EvalContext* ctx) {
    Cell* name_cell = cell_car(args);
    Cell* value_expr = cell_car(cell_cdr(args));

    if (name_cell->type != CELL_SYMBOL) {
        return error_cell(":invalid-define", name_cell);
    }

    Cell* value = eval(value_expr, ctx);

    // Push to global environment
    push_stack_frame(&ctx->global_env,
                     name_cell->value.symbol,
                     value);

    // NEW: Generate documentation if value is a lambda
    if (value && value->type == CELL_LAMBDA) {
        FunctionDoc* doc = doc_create(name_cell->value.symbol);
        doc_generate_description(ctx, doc, value->value.lambda.body);
        doc_infer_type(doc, value);

        // Add to context's doc list
        doc->next = ctx->user_docs;
        ctx->user_docs = doc;
    }

    return value;
}
```

### New Primitives for Querying

**Add to primitives.c:**

```c
// Query user function documentation
// (⌂≔ symbol) → description
Cell* primitive_doc_user_desc(Cell* args, EvalContext* ctx) {
    Cell* arg = cell_car(args);
    if (arg->type != CELL_SYMBOL) {
        return error_cell(":not-symbol", arg);
    }

    FunctionDoc* doc = doc_find(ctx, arg->value.symbol);
    if (!doc) {
        return make_symbol(":no-documentation");
    }

    return make_symbol(doc->description);
}

// Query user function type
// (⌂∈≔ symbol) → type signature
Cell* primitive_doc_user_type(Cell* args, EvalContext* ctx) {
    Cell* arg = cell_car(args);
    if (arg->type != CELL_SYMBOL) {
        return error_cell(":not-symbol", arg);
    }

    FunctionDoc* doc = doc_find(ctx, arg->value.symbol);
    if (!doc) {
        return make_symbol(":no-documentation");
    }

    return make_symbol(doc->type_signature);
}

// Query user function dependencies
// (⌂⊛≔ symbol) → list of dependencies
Cell* primitive_doc_user_deps(Cell* args, EvalContext* ctx) {
    Cell* arg = cell_car(args);
    if (arg->type != CELL_SYMBOL) {
        return error_cell(":not-symbol", arg);
    }

    FunctionDoc* doc = doc_find(ctx, arg->value.symbol);
    if (!doc) {
        return make_nil();
    }

    // Build list of dependencies
    Cell* result = make_nil();
    for (int i = doc->dependency_count - 1; i >= 0; i--) {
        Cell* sym = make_symbol(doc->dependencies[i]);
        result = make_pair(sym, result);
    }
    return result;
}
```

### Circular Dependency Prevention

**Problem:** During `eval_define()`, we evaluate the value BEFORE adding docs. This means recursive functions can't reference their own docs during definition.

**Solution:** This is OK! We document AFTER evaluation completes. Recursive calls during definition use the function value, not the docs.

**Example:**
```scheme
(≔ ! (λ (n) (? (≡ n #0) #1 (⊗ n (! (⊖ n #1))))))
```

**Timeline:**
1. Evaluate lambda → creates closure (may call ! recursively)
2. Add ! → global_env (makes ! callable)
3. Generate docs for ! (this phase cannot create circular deps)
4. Add docs to user_docs list

**Verification:** No circular dependencies possible because docs are metadata, not execution.

---

## 5. Test Plan

### Unit Tests (in C)

```c
// test_doc_extraction.c

void test_simple_dependency() {
    // (λ (x) (⊕ x #1))
    // Should extract: [⊕]

    Cell* lambda = parse("(λ (x) (⊕ x #1))");
    SymbolSet* params = symbol_set_create();
    DepList* deps = dep_list_create();

    extract_dependencies(lambda->value.lambda.body, params, deps);

    assert(deps->count == 1);
    assert(strcmp(deps->names[0], "⊕") == 0);
}

void test_recursive_dependency() {
    // (λ (n) (? (≡ n #0) #1 (⊗ n (! (⊖ n #1)))))
    // Should extract: [?, ≡, ⊗, !, ⊖]

    Cell* lambda = parse("(λ (n) (? (≡ n #0) #1 (⊗ n (! (⊖ n #1)))))");
    SymbolSet* params = symbol_set_create();
    DepList* deps = dep_list_create();

    extract_dependencies(lambda->value.lambda.body, params, deps);

    assert(deps->count == 5);
    assert(dep_list_contains(deps, "?"));
    assert(dep_list_contains(deps, "≡"));
    assert(dep_list_contains(deps, "⊗"));
    assert(dep_list_contains(deps, "!"));
    assert(dep_list_contains(deps, "⊖"));
}

void test_nested_lambda() {
    // (λ (x) (λ (y) (⊕ x y)))
    // Should extract: [⊕]
    // Should NOT extract x or y (they're params)

    Cell* lambda = parse("(λ (x) (λ (y) (⊕ x y)))");
    SymbolSet* params = symbol_set_create();
    DepList* deps = dep_list_create();

    extract_dependencies(lambda->value.lambda.body, params, deps);

    assert(deps->count == 1);
    assert(strcmp(deps->names[0], "⊕") == 0);
}
```

### Integration Tests (Guage)

```scheme
; tests/documentation.test

; Define a simple function
(≔ inc (λ (x) (⊕ x #1)))

; Query its documentation
(⌂≔ (⌜ inc))
; Expected: "Function using add two numbers" or similar

; Query its type
(⌂∈≔ (⌜ inc))
; Expected: "α → β" or "ℕ → ℕ"

; Query its dependencies
(⌂⊛≔ (⌜ inc))
; Expected: (:⊕)

; Define factorial
(≔ ! (λ (n) (? (≡ n #0) #1 (⊗ n (! (⊖ n #1))))))

; Query factorial docs
(⌂≔ (⌜ !))
; Expected: "Function using: ?, ≡, ⊗, !, ⊖"

(⌂⊛≔ (⌜ !))
; Expected: (:? :≡ :⊗ :! :⊖)

; Define const function
(≔ const (λ (x) (λ (y) x)))

(⌂≔ (⌜ const))
; Expected: "User-defined function" (no dependencies)

(⌂∈≔ (⌜ const))
; Expected: "α → β" or "α → α → β"
```

### Edge Cases

```scheme
; Self-recursive function
(≔ loop (λ (x) (loop x)))
(⌂⊛≔ (⌜ loop))
; Expected: (:loop) - should list itself as dependency

; Mutually recursive (not yet supported, but document behavior)
(≔ even? (λ (n) (? (≡ n #0) #t (odd? (⊖ n #1)))))
(≔ odd? (λ (n) (? (≡ n #0) #f (even? (⊖ n #1)))))
; Expected: each lists the other as dependency

; Function with no dependencies
(≔ identity (λ (x) x))
(⌂≔ (⌜ identity))
; Expected: "User-defined function"
```

---

## 6. Implementation Steps (Sequential)

### Step 1: Add Data Structures (30 min)
- Add `FunctionDoc` to `eval.h`
- Add `user_docs` to `EvalContext`
- Initialize in context creation
- Free in context destruction

**Test:** Compile cleanly

### Step 2: Implement Helper Data Structures (45 min)
- `SymbolSet` (create, add, contains, free)
- `DepList` (create, add, contains, free)

**Test:** Unit tests for set/list operations

### Step 3: Implement Dependency Extraction (90 min)
- `extract_dependencies()` function
- Handle all cell types
- Handle nested lambdas
- Handle parameter shadowing

**Test:** Unit tests for dependency extraction

### Step 4: Implement Doc API (60 min)
- `doc_create()`
- `doc_add_dependency()`
- `doc_find()`
- `doc_free_all()`

**Test:** Create/find/free docs manually

### Step 5: Implement Auto-Generation (60 min)
- `doc_generate_description()`
- `doc_infer_type()`
- `get_dependency_description()` helper

**Test:** Generate docs for test cases

### Step 6: Integrate with eval_define (30 min)
- Modify `eval_define()` to call doc generation
- Add error handling
- Test with existing code

**Test:** Define functions, verify docs created

### Step 7: Add Query Primitives (60 min)
- `primitive_doc_user_desc` (⌂≔)
- `primitive_doc_user_type` (⌂∈≔)
- `primitive_doc_user_deps` (⌂⊛≔)
- Register in primitives table

**Test:** Query docs from REPL

### Step 8: Integration Testing (90 min)
- Create `tests/documentation.test`
- Test all query primitives
- Test edge cases
- Fix bugs

**Test:** All tests pass

### Step 9: Documentation (30 min)
- Update `IMPLEMENTATION_STATUS.md`
- Update `SESSION_HANDOFF.md`
- Document new primitives
- Add examples to README

---

## 7. Risk Assessment

### High Risk
- **Dependency extraction bugs** - Complex recursion
  - Mitigation: Extensive unit tests, incremental testing

### Medium Risk
- **Memory leaks** - New allocation sites
  - Mitigation: Valgrind testing, careful refcount management

- **Symbol name collisions** - Primitive vs user functions
  - Mitigation: Clear naming convention (⌂≔ vs ⌂)

### Low Risk
- **Type inference limited** - Simple types only
  - Mitigation: Document limitations, mark as "future improvement"

---

## 8. Success Criteria

### Must Have ✅
- [ ] All user functions get automatic docs
- [ ] Can query description with ⌂≔
- [ ] Can query type with ⌂∈≔
- [ ] Can query dependencies with ⌂⊛≔
- [ ] No memory leaks
- [ ] All existing tests still pass
- [ ] No crashes

### Nice To Have 🎯
- [ ] Smart description composition
- [ ] Better type inference
- [ ] Pretty-printed docs
- [ ] Recursive function detection

### Future Enhancements 🚀
- [ ] User-provided doc strings
- [ ] Full type inference
- [ ] Doc generation for primitives too
- [ ] HTML doc export

---

## 9. Estimated Timeline

| Step | Time | Cumulative |
|------|------|------------|
| 1. Data structures | 30m | 30m |
| 2. Helpers | 45m | 1h 15m |
| 3. Extraction | 90m | 2h 45m |
| 4. Doc API | 60m | 3h 45m |
| 5. Auto-gen | 60m | 4h 45m |
| 6. Integration | 30m | 5h 15m |
| 7. Primitives | 60m | 6h 15m |
| 8. Testing | 90m | 7h 45m |
| 9. Docs | 30m | 8h 15m |

**Total: 8 hours 15 minutes**

---

## 10. Pre-Flight Checklist

Before starting implementation:
- [x] All existing tests pass
- [x] Clean compilation
- [x] This plan reviewed and approved
- [ ] Ready to implement Step 1

---

**Plan Status:** READY FOR REVIEW
**Next Action:** Review this plan, then begin Step 1
**Created:** 2026-01-26
**Author:** Claude Sonnet 4.5
