# Phase 2 Implementation Plan: Mandatory Documentation

## Goal
Every symbol in Guage MUST have documentation. No exceptions.

## Step-by-Step Implementation

### Step 1: Add Documentation to Primitive Structure ✅ NEXT
**Time: 30 minutes**

```c
// primitives.h - Add doc structure
typedef struct {
    const char* description;
    const char* type_signature;
} PrimitiveDoc;

typedef struct {
    const char* symbol;
    PrimitiveFn fn;
    int arity;
    PrimitiveDoc doc;  // NEW
} Primitive;
```

**Files:** `primitives.h`

### Step 2: Document All Existing Primitives
**Time: 1-2 hours**

Document all ~25 primitives:
- Arithmetic: ⊕, ⊖, ⊗, ⊘
- Comparison: ≡, ≢, <, >, ≤, ≥
- Logic: ∧, ∨, ¬
- Lists: ⟨⟩, ◁, ▷, ∅?
- Introspection: ⊙, ⧉, ⊛
- Testing: ⊨, ⊢, ≟, ⟲
- Error: ⚠, ⚠?

**Files:** `primitives.c`

### Step 3: Add Primitive Doc Query Function
**Time: 30 minutes**

```c
// primitives.c
PrimitiveDoc* primitive_get_doc(const char* symbol) {
    for (int i = 0; primitives[i].symbol != NULL; i++) {
        if (strcmp(primitives[i].symbol, symbol) == 0) {
            return &primitives[i].doc;
        }
    }
    return NULL;
}
```

**Files:** `primitives.c`, `primitives.h`

### Step 4: Implement Basic ⌂ and ⌂∈ Primitives
**Time: 1 hour**

```c
// primitives.c
Cell* prim_doc_get(Cell* args) {
    Cell* name = arg1(args);
    const char* sym = cell_get_symbol(name);

    PrimitiveDoc* doc = primitive_get_doc(sym);
    if (doc) {
        return cell_symbol(doc->description);
    }

    // TODO: User function docs
    return cell_symbol("Undocumented");
}

Cell* prim_doc_type(Cell* args) {
    Cell* name = arg1(args);
    const char* sym = cell_get_symbol(name);

    PrimitiveDoc* doc = primitive_get_doc(sym);
    if (doc) {
        return cell_symbol(doc->type_signature);
    }

    return cell_symbol("Unknown");
}
```

**Files:** `primitives.c`

### Step 5: Add Global Doc Registry
**Time: 1 hour**

```c
// doc.h (new file)
typedef struct DocEntry {
    const char* name;
    Cell* dependencies;
    Cell* source;
    char* generated_description;
    char* inferred_type;
    struct DocEntry* next;
} DocEntry;

typedef struct {
    DocEntry* head;
} DocRegistry;

DocRegistry* doc_registry_new(void);
void doc_registry_add(DocRegistry* reg, const char* name, Cell* value);
DocEntry* doc_registry_lookup(DocRegistry* reg, const char* name);
```

**Files:** `doc.h`, `doc.c` (new)

### Step 6: Implement Dependency Extraction
**Time: 2 hours**

```c
// doc.c
Cell* extract_dependencies(Cell* expr) {
    if (cell_is_symbol(expr)) {
        // Check if it's a known symbol (primitive or user function)
        if (is_known_symbol(expr)) {
            return cell_cons(expr, cell_nil());
        }
        return cell_nil();
    }

    if (cell_is_pair(expr)) {
        Cell* first_deps = extract_dependencies(cell_car(expr));
        Cell* rest_deps = extract_dependencies(cell_cdr(expr));
        return merge_unique(first_deps, rest_deps);
    }

    return cell_nil();
}

Cell* merge_unique(Cell* list1, Cell* list2) {
    // Merge two lists, removing duplicates
    // ...
}
```

**Files:** `doc.c`

### Step 7: Auto-Generate Documentation
**Time: 2 hours**

```c
// doc.c
DocEntry* doc_generate(const char* name, Cell* value) {
    DocEntry* entry = malloc(sizeof(DocEntry));
    entry->name = strdup(name);
    entry->source = value;
    cell_retain(value);

    if (cell_is_lambda(value)) {
        Cell* body = value->data.lambda.body;

        // Extract dependencies
        entry->dependencies = extract_dependencies(body);

        // Compose description
        entry->generated_description = compose_description(
            name, entry->dependencies, value->data.lambda.arity
        );

        // Infer type (simple for now)
        entry->inferred_type = infer_simple_type(
            entry->dependencies, value->data.lambda.arity
        );
    }

    return entry;
}
```

**Files:** `doc.c`

### Step 8: Integrate with eval_define
**Time: 1 hour**

```c
// eval.c
void eval_define(EvalContext* ctx, const char* name, Cell* value) {
    // ... existing code ...

    // AUTO-GENERATE documentation
    doc_registry_add(ctx->doc_registry, name, value);

    // ... existing code ...
}
```

**Files:** `eval.c`, `eval.h`

### Step 9: Complete Doc Primitives
**Time: 1 hour**

Implement:
- `⌂≔` - Get dependencies
- `⌂⊛` - Get source

**Files:** `primitives.c`

### Step 10: Test Everything
**Time: 2 hours**

Create comprehensive tests:
```scheme
; Test primitive docs
(⊨ (⌜ :test-prim-doc) "Add two numbers" (⌂ ⊕))
(⊨ (⌜ :test-prim-type) "ℕ → ℕ → ℕ" (⌂∈ ⊕))

; Test auto-generation
(≔ square (λ (x) (⊗ x x)))
(⊨ (⌜ :test-auto-doc) #t (≢ (⌂ square) ""))
(⊨ (⌜ :test-auto-deps) (⟨⟩ :⊗ ∅) (⌂≔ square))
```

**Files:** `tests/documentation.test`

## Execution Order

### Phase A: Primitive Documentation (3-4 hours)
1. ✅ Step 1: Add doc structure
2. ✅ Step 2: Document all primitives
3. ✅ Step 3: Query function
4. ✅ Step 4: Basic ⌂ and ⌂∈

**Test:** All primitives have docs

### Phase B: Auto-Generation (4-5 hours)
5. ✅ Step 5: Doc registry
6. ✅ Step 6: Dependency extraction
7. ✅ Step 7: Auto-generate docs
8. ✅ Step 8: Integrate with ≔

**Test:** User functions auto-documented

### Phase C: Complete System (3-4 hours)
9. ✅ Step 9: Complete primitives
10. ✅ Step 10: Full test suite

**Test:** Everything works end-to-end

## Total Time: 10-13 hours

## Validation Criteria

After each phase:

**Phase A:**
- [ ] All primitives have description
- [ ] All primitives have type signature
- [ ] (⌂ ⊕) returns description
- [ ] (⌂∈ ⊕) returns type

**Phase B:**
- [ ] User functions auto-registered
- [ ] Dependencies extracted correctly
- [ ] (⌂ user-fn) returns generated docs
- [ ] (⌂≔ user-fn) returns dependencies

**Phase C:**
- [ ] All doc primitives work
- [ ] Composition example works
- [ ] No symbol without docs

## Files to Create/Modify

### New Files
- `doc.h` - Doc registry interface
- `doc.c` - Doc generation implementation
- `tests/documentation.test` - Doc tests

### Modified Files
- `primitives.h` - Add PrimitiveDoc struct
- `primitives.c` - Document all primitives, add ⌂ primitives
- `eval.h` - Add doc_registry to EvalContext
- `eval.c` - Call doc_registry_add in eval_define
- `Makefile` - Add doc.c to build

## Start Now

Begin with Step 1: Add PrimitiveDoc structure.
