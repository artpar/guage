# Phase 2: Documentation as First-Class Citizen (CORRECTED)

## Core Principle

**Documentation is NOT optional. It is an intrinsic property of every symbol.**

- ❌ No "attach docs" - docs are automatic
- ❌ No "has docs?" - everything has docs by definition
- ✅ Every primitive has manual documentation
- ✅ Every user function has auto-generated documentation
- ✅ Documentation is ALWAYS available

## Architecture Change

### Primitives Layer

**Every primitive MUST be defined with documentation:**

```c
// In primitives.c
typedef struct {
    const char* symbol;
    PrimitiveFn fn;
    int arity;
    const char* description;
    const char* type_signature;
} Primitive;

static Primitive primitives[] = {
    {"⊕", prim_add, 2, "Add two numbers", "ℕ → ℕ → ℕ"},
    {"⊗", prim_mul, 2, "Multiply two numbers", "ℕ → ℕ → ℕ"},
    {"≡", prim_eq, 2, "Test equality", "α → α → 𝔹"},
    // ... ALL primitives MUST have docs
};
```

**No primitive without documentation is allowed.**

### User Functions Layer

**When ≔ defines a function, docs are AUTOMATICALLY generated:**

```scheme
; User defines function
(≔ square (λ (x) (⊗ x x)))

; System AUTOMATICALLY generates:
; - Description: "Function using: ⊗ (multiply two numbers)"
; - Type: "ℕ → ℕ" (inferred from ⊗)
; - Dependencies: [⊗]
; - Source: "(λ (x) (⊗ x x))"
```

**No function exists without documentation.**

### Query Interface

```scheme
; ALWAYS succeeds - every symbol has docs
(⌂ ⊕)           ; → "Add two numbers"
(⌂ square)      ; → Auto-generated description

; ALWAYS succeeds - every symbol has type
(⌂∈ ⊕)          ; → "ℕ → ℕ → ℕ"
(⌂∈ square)     ; → "ℕ → ℕ"

; ALWAYS succeeds - every function has dependencies
(⌂≔ square)     ; → [⊗]

; ALWAYS succeeds - source is always available
(⌂⊛ square)     ; → "(λ (x) (⊗ x x))"
```

**No "has docs?" check needed because EVERYTHING has docs.**

## Implementation

### 1. Primitive Documentation (primitives.c)

```c
typedef struct {
    const char* symbol;
    PrimitiveFn fn;
    int arity;
    PrimitiveDoc doc;  // NEW: Required documentation
} Primitive;

typedef struct {
    const char* description;
    const char* type_signature;
    const char** properties;      // Array of property strings
    const char** examples;         // Array of examples
} PrimitiveDoc;

// EVERY primitive MUST have docs
static Primitive primitives[] = {
    {
        "⊕",
        prim_add,
        2,
        {
            "Add two numbers",
            "ℕ → ℕ → ℕ",
            (const char*[]){"Commutative", "Associative", NULL},
            (const char*[]){"(⊕ #2 #3) → #5", NULL}
        }
    },
    {
        "⊗",
        prim_mul,
        2,
        {
            "Multiply two numbers",
            "ℕ → ℕ → ℕ",
            (const char*[]){"Commutative", "Associative", NULL},
            (const char*[]){"(⊗ #6 #7) → #42", NULL}
        }
    },
    // ... ALL primitives documented
};
```

### 2. Automatic Documentation Generation (eval.c)

```c
void eval_define(EvalContext* ctx, const char* name, Cell* value) {
    // ... existing code ...

    // AUTOMATICALLY generate documentation
    Cell* docs = doc_generate(name, value);

    // Store in global registry
    doc_registry_add(name, docs);

    // Bind value
    env_define(env, name, value);
}

Cell* doc_generate(const char* name, Cell* value) {
    if (cell_is_lambda(value)) {
        // Extract structure
        Cell* body = value->data.lambda.body;
        int arity = value->data.lambda.arity;

        // Find dependencies
        Cell* deps = extract_dependencies(body);

        // Compose description from dependency docs
        char* description = compose_description(name, deps, arity);

        // Infer type
        char* type_sig = infer_type(deps, arity);

        // Return doc cell
        return cell_doc(description, type_sig, deps, body);
    }

    // Non-function values still get basic docs
    return cell_doc("Value", "Unknown", NULL, value);
}
```

### 3. Documentation Primitives (NO optional checks)

```c
// ⌂ - Get documentation (ALWAYS succeeds)
Cell* prim_doc_get(Cell* args) {
    Cell* name_sym = arg1(args);

    // Lookup in primitive table
    Primitive* prim = primitive_lookup(name_sym);
    if (prim) {
        return cell_string(prim->doc.description);
    }

    // Lookup in user definitions
    Cell* doc = doc_registry_lookup(name_sym);
    if (doc) {
        return cell_get_doc_description(doc);
    }

    // Should NEVER happen - everything has docs
    return cell_string("UNDOCUMENTED ERROR");
}

// ⌂∈ - Get type (ALWAYS succeeds)
Cell* prim_doc_type(Cell* args) {
    Cell* name_sym = arg1(args);

    // Primitives have manual types
    Primitive* prim = primitive_lookup(name_sym);
    if (prim) {
        return cell_string(prim->doc.type_signature);
    }

    // User functions have inferred types
    Cell* doc = doc_registry_lookup(name_sym);
    if (doc) {
        return cell_get_doc_type(doc);
    }

    // Should NEVER happen
    return cell_string("Unknown");
}

// ⌂≔ - Get dependencies (ALWAYS succeeds)
Cell* prim_doc_deps(Cell* args) {
    Cell* name_sym = arg1(args);

    // Primitives have no dependencies
    Primitive* prim = primitive_lookup(name_sym);
    if (prim) {
        return cell_nil();
    }

    // User functions have auto-discovered deps
    Cell* doc = doc_registry_lookup(name_sym);
    if (doc) {
        return cell_get_doc_deps(doc);
    }

    // Should NEVER happen
    return cell_nil();
}

// ⌂⊛ - Get source (ALWAYS succeeds)
Cell* prim_doc_source(Cell* args) {
    Cell* name_sym = arg1(args);

    // Primitives are built-in
    Primitive* prim = primitive_lookup(name_sym);
    if (prim) {
        return cell_string("<primitive>");
    }

    // User functions have source
    Cell* doc = doc_registry_lookup(name_sym);
    if (doc) {
        return cell_get_doc_source(doc);
    }

    // Should NEVER happen
    return cell_nil();
}
```

### 4. Example Usage

```scheme
; Query primitive docs (ALWAYS works)
(⌂ ⊕)          ; → "Add two numbers"
(⌂∈ ⊕)         ; → "ℕ → ℕ → ℕ"
(⌂≔ ⊕)         ; → ∅ (primitives have no dependencies)

; Define function - docs AUTO-GENERATED
(≔ square (λ (x) (⊗ x x)))

; Query auto-generated docs (ALWAYS works)
(⌂ square)     ; → "Function using: ⊗ (multiply two numbers)"
(⌂∈ square)    ; → "ℕ → ℕ"
(⌂≔ square)    ; → (⟨⟩ :⊗ ∅)
(⌂⊛ square)    ; → "(λ (x) (⊗ x x))"

; Compose higher-level function
(≔ sum-of-squares (λ (a b) (⊕ (square a) (square b))))

; Auto-generated docs from constituents
(⌂ sum-of-squares)
; → "Function using: ⊕ (add two numbers), square (function using: ⊗)"

(⌂∈ sum-of-squares)
; → "ℕ → ℕ → ℕ"

(⌂≔ sum-of-squares)
; → (⟨⟩ :⊕ (⟨⟩ :square ∅))
```

## Dependency Composition Example

```scheme
; Low-level primitives (documented manually)
⊕ : "Add two numbers" : ℕ → ℕ → ℕ
⊗ : "Multiply two numbers" : ℕ → ℕ → ℕ

; User defines (auto-documented)
(≔ square (λ (x) (⊗ x x)))
; → square : "Function using: ⊗ (multiply two numbers)" : ℕ → ℕ

(≔ cube (λ (x) (⊗ x (square x))))
; → cube : "Function using: ⊗, square (function using: ⊗)" : ℕ → ℕ

(≔ sum-of-cubes (λ (a b) (⊕ (cube a) (cube b))))
; → "Function using: ⊕, cube (function using: ⊗, square)"
```

**Documentation flows upward automatically!**

## Invariants (MUST HOLD)

```
INV-DOC-1: Every primitive has documentation (enforced at compile time)
INV-DOC-2: Every user function has documentation (auto-generated)
INV-DOC-3: ⌂ NEVER fails (every symbol has docs)
INV-DOC-4: ⌂∈ NEVER fails (every symbol has type)
INV-DOC-5: Documentation is immutable once generated
INV-DOC-6: Auto-generated docs reflect current dependencies
INV-DOC-7: Dependency graph is always accurate
```

## Build-Time Enforcement

```c
// At compile time, verify all primitives documented
#define PRIMITIVE_DOC_CHECK() \
    for (int i = 0; primitives[i].symbol != NULL; i++) { \
        if (primitives[i].doc.description == NULL) { \
            #error "Primitive missing documentation" \
        } \
        if (primitives[i].doc.type_signature == NULL) { \
            #error "Primitive missing type signature" \
        } \
    }

// Run at compile time
static_assert(/* check all primitives have docs */,
              "All primitives must have documentation");
```

## Testing

```scheme
; Test: All primitives documented
(⊨ (⌜ :test-prim-docs) #t (≢ (⌂ ⊕) ""))
(⊨ (⌜ :test-prim-type) #t (≢ (⌂∈ ⊕) ""))

; Test: Auto-generated docs exist
(≔ test-fn (λ (x) (⊕ x #1)))
(⊨ (⌜ :test-auto-doc) #t (≢ (⌂ test-fn) ""))
(⊨ (⌜ :test-auto-type) #t (≢ (⌂∈ test-fn) ""))

; Test: Dependencies accurate
(≔ f (λ (x) (⊕ x #1)))
(≔ g (λ (x) (⊗ (f x) #2)))
(⊨ (⌜ :test-deps)
   (⟨⟩ :⊗ (⟨⟩ :f ∅))
   (⌂≔ g))
```

## Benefits

### 1. Guaranteed Documentation
**Every symbol has docs. No exceptions. Ever.**

### 2. No Optional Checks
```scheme
; ❌ NO NEED for this:
(? (⌂? symbol) (⌂ symbol) "No docs")

; ✅ Just do this:
(⌂ symbol)  ; ALWAYS works
```

### 3. Self-Describing System
Query any symbol, get documentation immediately.

### 4. Composable Knowledge
High-level docs automatically composed from low-level docs.

### 5. Enforced Quality
Can't commit code without documentation (enforced at primitive level).

## Primitives (Final List)

- `⌂` - Get description (ALWAYS succeeds)
- `⌂∈` - Get type signature (ALWAYS succeeds)
- `⌂≔` - Get dependencies (ALWAYS succeeds)
- `⌂⇐` - Get reverse dependencies (ALWAYS succeeds)
- `⌂⊛` - Get source code (ALWAYS succeeds)
- `⌂⊢` - Get properties (ALWAYS succeeds)
- `⌂Ex` - Get examples (ALWAYS succeeds)

**No "attach" or "has docs?" primitives needed.**

## Implementation Checklist

- [ ] Add doc fields to Primitive struct
- [ ] Document ALL existing primitives
- [ ] Create doc_generate() function
- [ ] Create doc_registry
- [ ] Modify eval_define() to auto-generate docs
- [ ] Implement ⌂ primitive
- [ ] Implement ⌂∈ primitive
- [ ] Implement ⌂≔ primitive
- [ ] Implement ⌂⊛ primitive
- [ ] Test: all primitives have docs
- [ ] Test: auto-generation works
- [ ] Test: composition works

---

**This is the correct design: Documentation is mandatory, automatic, and always available.**
