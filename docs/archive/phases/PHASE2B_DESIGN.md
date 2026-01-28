# Phase 2B: Recursive Auto-Documentation Design

## Executive Summary

Implement automatic documentation generation for user functions through **recursive composition** of primitive and function documentation strings, creating human-readable descriptions.

## Core Principle: Recursive Composition

```
Level 0: Primitives (100% documented by spec)
   ⊗ → "Multiply two numbers"
   ? → "Conditional: if condition then true-branch else false-branch"

Level 1: User functions using primitives
   double = (λ (x) (⊗ x #2))
   → "Multiply x by 2"

Level 2: User functions using user functions
   quadruple = (λ (x) (double (double x)))
   → "Apply double to result of (double x)"
   → Recursively expand: "Multiply (multiply x by 2) by 2"
```

## Architecture

### 1. Doc Registry Data Structure

```c
// User function documentation entry
typedef struct UserFunctionDoc {
    char *name;                    // Function name
    char *description;             // Composed human-readable description
    char *type_signature;          // Inferred type (e.g., "ℕ → ℕ")
    Cell *dependencies;            // List of symbols used (⟨:⊗ ⟨:? ...⟩⟩)
    Cell *body;                    // Original lambda body (for re-composition)
    struct UserFunctionDoc *next;  // Linked list
} UserFunctionDoc;

// Global registry
typedef struct {
    UserFunctionDoc *head;
    size_t count;
} DocRegistry;

// Global instance
static DocRegistry user_docs = {NULL, 0};
```

**API Functions:**
```c
// Create/update documentation for a user function
void doc_register_function(const char *name, Cell *lambda_body);

// Query user function documentation
Cell *doc_get_description(Cell *symbol);   // Returns :string or ∅
Cell *doc_get_type(Cell *symbol);          // Returns :string or ∅
Cell *doc_get_deps(Cell *symbol);          // Returns list or ∅

// Free registry (for cleanup)
void doc_registry_free(void);
```

### 2. Dependency Extraction Algorithm

**Goal:** Extract all primitive and function symbols referenced in lambda body.

```c
// Extract all symbols (primitives + user functions) from AST
Cell *extract_dependencies(Cell *body, Cell *params) {
    if (body == NULL) return NULL;

    switch (body->type) {
        case CELL_SYMBOL: {
            // Skip if it's a parameter
            if (is_parameter(body, params)) return NULL;

            // Check if primitive or user function
            if (is_primitive(body->symbol) ||
                doc_is_user_function(body->symbol)) {
                return make_list_node(body);
            }
            return NULL;
        }

        case CELL_PAIR: {
            // Recursively extract from car and cdr
            Cell *left_deps = extract_dependencies(car(body), params);
            Cell *right_deps = extract_dependencies(cdr(body), params);
            return merge_unique(left_deps, right_deps);
        }

        case CELL_LAMBDA: {
            // Extract from lambda body with updated params
            Cell *new_params = cons(body->lambda_param, params);
            return extract_dependencies(body->lambda_body, new_params);
        }

        default:
            return NULL;
    }
}
```

**Helper Functions:**
```c
bool is_parameter(Cell *symbol, Cell *param_list);
bool is_primitive(const char *symbol);
bool doc_is_user_function(const char *name);
Cell *merge_unique(Cell *list1, Cell *list2);  // No duplicates
```

### 3. Recursive Description Composition

**Goal:** Build human-readable description from AST structure.

```c
// Recursively compose description from expression
char *compose_description(Cell *expr, Cell *params, int depth) {
    if (depth > MAX_RECURSION_DEPTH) {
        return strdup("(deeply nested expression)");
    }

    if (expr == NULL) return strdup("nil");

    switch (expr->type) {
        case CELL_NUMBER:
            return format_number(expr->number);

        case CELL_BOOL:
            return strdup(expr->bool_value ? "true" : "false");

        case CELL_SYMBOL: {
            // Check if parameter
            if (is_parameter(expr, params)) {
                return strdup(expr->symbol);
            }

            // Check if primitive
            PrimitiveDoc *prim = get_primitive_doc(expr->symbol);
            if (prim) {
                return strdup(prim->description);
            }

            // Check if user function - RECURSIVE!
            UserFunctionDoc *user_doc = doc_find_function(expr->symbol);
            if (user_doc) {
                return strdup(user_doc->description);
            }

            return strdup(expr->symbol);
        }

        case CELL_PAIR: {
            Cell *func = car(expr);
            Cell *args = cdr(expr);

            // Special handling for known patterns
            if (is_symbol(func, "?")) {
                return compose_conditional(args, params, depth);
            }
            if (is_symbol(func, "⊗")) {
                return compose_binary_op("multiply", args, params, depth);
            }
            if (is_symbol(func, "⊕")) {
                return compose_binary_op("add", args, params, depth);
            }
            // ... other operators

            // Generic application
            char *func_desc = compose_description(func, params, depth + 1);
            char *args_desc = compose_args(args, params, depth + 1);
            char *result = format("Apply %s to %s", func_desc, args_desc);
            free(func_desc);
            free(args_desc);
            return result;
        }

        case CELL_LAMBDA: {
            char *body_desc = compose_description(
                expr->lambda_body,
                cons(expr->lambda_param, params),
                depth + 1
            );
            char *result = format("Function that %s", body_desc);
            free(body_desc);
            return result;
        }

        default:
            return strdup("(unknown)");
    }
}
```

**Pattern-Specific Composers:**
```c
// Handle conditional: (? cond then else)
char *compose_conditional(Cell *args, Cell *params, int depth) {
    Cell *cond = nth(args, 0);
    Cell *then_branch = nth(args, 1);
    Cell *else_branch = nth(args, 2);

    char *cond_desc = compose_description(cond, params, depth + 1);
    char *then_desc = compose_description(then_branch, params, depth + 1);
    char *else_desc = compose_description(else_branch, params, depth + 1);

    char *result = format(
        "If %s, then %s, else %s",
        cond_desc, then_desc, else_desc
    );

    free(cond_desc);
    free(then_desc);
    free(else_desc);
    return result;
}

// Handle binary operations: (op a b)
char *compose_binary_op(const char *op_name, Cell *args,
                        Cell *params, int depth) {
    Cell *left = nth(args, 0);
    Cell *right = nth(args, 1);

    char *left_desc = compose_description(left, params, depth + 1);
    char *right_desc = compose_description(right, params, depth + 1);

    char *result = format("%s %s and %s", op_name, left_desc, right_desc);

    free(left_desc);
    free(right_desc);
    return result;
}
```

### 4. Simple Type Inference

**Goal:** Infer basic type signatures like "ℕ → ℕ" or "α → β".

```c
// Infer type signature from lambda structure
char *infer_type_signature(Cell *lambda) {
    if (!is_lambda(lambda)) return strdup("α");

    int arity = count_lambda_params(lambda);

    // Simple inference rules:
    // - If body uses only arithmetic: ℕ → ℕ → ... → ℕ
    // - If body uses comparison: α → α → 𝔹
    // - Otherwise: Generic α → β → ... → γ

    Cell *body = get_lambda_body(lambda);

    if (uses_only_arithmetic(body)) {
        return format_type_arrow(arity, "ℕ", "ℕ");
    }
    if (returns_bool(body)) {
        return format_type_arrow(arity, "α", "𝔹");
    }

    // Generic
    return format_type_arrow_generic(arity);
}

// Format: α → β → γ (arity params + result)
char *format_type_arrow(int arity, const char *param_type,
                        const char *result_type) {
    // Build: "param_type → param_type → ... → result_type"
    StringBuilder sb;
    for (int i = 0; i < arity; i++) {
        append(&sb, param_type);
        append(&sb, " → ");
    }
    append(&sb, result_type);
    return to_string(&sb);
}
```

### 5. Integration with eval_define

**Location:** `eval.c`, function `eval_define()`

**Current code:**
```c
Cell *eval_define(Cell *args, EvalContext *ctx) {
    Cell *name = car(args);
    Cell *value = eval(cadr(args), ctx);

    // Store in environment
    env_define(ctx->env, name->symbol, value);

    return value;
}
```

**Modified code:**
```c
Cell *eval_define(Cell *args, EvalContext *ctx) {
    Cell *name = car(args);
    Cell *value_expr = cadr(args);
    Cell *value = eval(value_expr, ctx);

    // Store in environment
    env_define(ctx->env, name->symbol, value);

    // AUTO-DOCUMENTATION: If defining a lambda, generate docs
    if (value->type == CELL_LAMBDA) {
        doc_register_function(name->symbol, value);

        // Print auto-generated documentation
        UserFunctionDoc *doc = doc_find_function(name->symbol);
        if (doc) {
            printf("📝 %s :: %s\n", doc->name, doc->type_signature);
            printf("   %s\n", doc->description);
            if (doc->dependencies) {
                printf("   Dependencies: ");
                print_symbol_list(doc->dependencies);
                printf("\n");
            }
        }
    }

    return value;
}
```

### 6. Implementation of doc_register_function

```c
void doc_register_function(const char *name, Cell *lambda) {
    // Extract dependencies
    Cell *params = NULL;  // Start with no params
    if (lambda->type == CELL_LAMBDA) {
        params = make_list(lambda->lambda_param);
    }
    Cell *deps = extract_dependencies(lambda->lambda_body, params);

    // Compose description recursively
    char *desc = compose_description(
        lambda->lambda_body,
        params,
        0  // Initial depth
    );

    // Infer type signature
    char *type_sig = infer_type_signature(lambda);

    // Check if already exists (update) or create new
    UserFunctionDoc *existing = doc_find_function(name);
    if (existing) {
        // Update existing
        free(existing->description);
        free(existing->type_signature);
        if (existing->dependencies) cell_release(existing->dependencies);
        if (existing->body) cell_release(existing->body);

        existing->description = desc;
        existing->type_signature = type_sig;
        existing->dependencies = deps;
        existing->body = cell_retain(lambda);
    } else {
        // Create new entry
        UserFunctionDoc *new_doc = malloc(sizeof(UserFunctionDoc));
        new_doc->name = strdup(name);
        new_doc->description = desc;
        new_doc->type_signature = type_sig;
        new_doc->dependencies = deps;
        new_doc->body = cell_retain(lambda);
        new_doc->next = user_docs.head;

        user_docs.head = new_doc;
        user_docs.count++;
    }
}
```

## File Structure

### New Files

**`bootstrap/user_docs.h`:**
```c
#ifndef USER_DOCS_H
#define USER_DOCS_H

#include "cell.h"

// User function documentation
typedef struct UserFunctionDoc {
    char *name;
    char *description;
    char *type_signature;
    Cell *dependencies;
    Cell *body;
    struct UserFunctionDoc *next;
} UserFunctionDoc;

// Registry management
void doc_register_function(const char *name, Cell *lambda);
UserFunctionDoc *doc_find_function(const char *name);
Cell *doc_get_user_description(const char *name);
Cell *doc_get_user_type(const char *name);
Cell *doc_get_user_deps(const char *name);
void doc_registry_free(void);

// Description composition
char *compose_description(Cell *expr, Cell *params, int depth);
Cell *extract_dependencies(Cell *body, Cell *params);
char *infer_type_signature(Cell *lambda);

#endif
```

**`bootstrap/user_docs.c`:**
- Implementation of all functions above

### Modified Files

**`bootstrap/eval.c`:**
- Modify `eval_define()` to call `doc_register_function()`
- Add `#include "user_docs.h"`

**`bootstrap/primitives.c`:**
- Extend `⌂` to check user functions if not a primitive
- Extend `⌂∈` to check user functions if not a primitive
- Extend `⌂≔` to return user function dependencies

**`bootstrap/main.c`:**
- Add `doc_registry_free()` on exit

**`Makefile`:**
- Add `user_docs.o` to build

## Implementation Steps

### Step 1: Basic Infrastructure (30 min)
1. Create `user_docs.h` with data structures
2. Create `user_docs.c` with empty implementations
3. Update Makefile
4. Verify compilation

### Step 2: Doc Registry (1 hour)
1. Implement `doc_register_function()` stub
2. Implement `doc_find_function()`
3. Implement `doc_registry_free()`
4. Test with simple storage/retrieval

### Step 3: Dependency Extraction (1.5 hours)
1. Implement `extract_dependencies()` core
2. Implement `is_parameter()` helper
3. Implement `merge_unique()` helper
4. Test with factorial, fibonacci

### Step 4: Description Composition (2 hours)
1. Implement `compose_description()` core with recursion
2. Implement `compose_conditional()` for `?`
3. Implement `compose_binary_op()` for arithmetic
4. Test with nested examples

### Step 5: Type Inference (1 hour)
1. Implement `infer_type_signature()` basic
2. Implement helper type analyzers
3. Test with various functions

### Step 6: Integration (1 hour)
1. Modify `eval_define()` to call docs
2. Extend primitive doc queries to check user functions
3. Add auto-print on definition
4. Test end-to-end

### Step 7: Testing (1 hour)
1. Create `tests/user_docs.test`
2. Test factorial documentation
3. Test fibonacci documentation
4. Test nested function composition
5. Test recursive references

## Test Plan

### Test Cases

**Test 1: Simple function**
```scheme
(≔ double (λ (x) (⊗ x #2)))
; Expected output:
; 📝 double :: ℕ → ℕ
;    Multiply x and 2
;    Dependencies: ⊗

(⌂ (⌜ double))   ; → :string
(⌂∈ (⌜ double))  ; → :ℕ → ℕ
(⌂≔ (⌜ double))  ; → ⟨:⊗ ∅⟩
```

**Test 2: Conditional function**
```scheme
(≔ abs (λ (x) (? (< x #0) (⊖ #0 x) x)))
; Expected:
; 📝 abs :: ℕ → ℕ
;    If x less than 0, then subtract x from 0, else x
;    Dependencies: ?, <, ⊖
```

**Test 3: Recursive function (factorial)**
```scheme
(≔ ! (λ (n) (? (≡ n #0) #1 (⊗ n (! (⊖ n #1))))))
; Expected:
; 📝 ! :: ℕ → ℕ
;    If n equals 0, then 1, else multiply n and (factorial of subtract n and 1)
;    Dependencies: ?, ≡, ⊗, !, ⊖
```

**Test 4: Composed functions**
```scheme
(≔ double (λ (x) (⊗ x #2)))
(≔ quadruple (λ (x) (double (double x))))
; Expected:
; 📝 quadruple :: ℕ → ℕ
;    Apply double to result of (double x)
;    Dependencies: double
```

**Test 5: Higher-order function**
```scheme
(≔ twice (λ (f) (λ (x) (f (f x)))))
; Expected:
; 📝 twice :: (α → α) → α → α
;    Function that applies f to result of (f x)
;    Dependencies: (none)
```

## Edge Cases

1. **Self-recursion:** Function references itself (like factorial)
   - Show function name in description
   - Include self in dependencies

2. **Mutual recursion:** Not supported yet
   - Will show as undefined function

3. **Deep nesting:** Limit recursion depth to prevent stack overflow
   - MAX_RECURSION_DEPTH = 20

4. **Non-lambda definitions:** Skip documentation
   - Only document lambdas

5. **Redefinition:** Update existing documentation
   - Replace old doc with new

## Success Criteria

- [ ] Can extract all dependencies from lambda body
- [ ] Can compose human-readable descriptions recursively
- [ ] Can infer basic type signatures
- [ ] Auto-prints documentation on function definition
- [ ] Can query user function docs via ⌂, ⌂∈, ⌂≔
- [ ] Handles recursive functions correctly
- [ ] Handles nested functions correctly
- [ ] All tests pass
- [ ] No memory leaks

## Estimated Time

- Planning: ✅ Complete (this document)
- Implementation: 6-8 hours
- Testing: 1 hour
- Total: 7-9 hours

## Notes

- Keep descriptions concise but readable
- Focus on correctness over elegance
- Test incrementally after each step
- Commit working code frequently

---

**Design complete. Ready for implementation.**
