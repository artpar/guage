# Phase 2: First-Class Documentation Design

## Vision: Auto-Discoverable Documentation

**Key Insight:** If all low-level symbols are documented, everything built above them should be automatically documented by recursively composing constituent documentation.

### Example

```scheme
; Low-level primitives (already documented)
(⊕ a b)  ; ⌂: "Add two numbers" ∈: "ℕ → ℕ → ℕ"
(⊗ a b)  ; ⌂: "Multiply two numbers" ∈: "ℕ → ℕ → ℕ"

; User defines function using documented primitives
(≔ square (λ (x) (⊗ x x)))

; Automatic documentation generated:
(⌂ square)
; → "Function that:
;     Takes parameter x: ℕ
;     Returns: (⊗ x x)
;     Uses primitives: ⊗
;     Type: ℕ → ℕ"

; Higher-order composition
(≔ sum-of-squares (λ (a b) (⊕ (square a) (square b))))

; Automatic documentation:
(⌂ sum-of-squares)
; → "Function that:
;     Takes parameters a: ℕ, b: ℕ
;     Returns: (⊕ (square a) (square b))
;     Calls: square (ℕ → ℕ), ⊕ (ℕ → ℕ → ℕ)
;     Type: ℕ → ℕ → ℕ"
```

## Architecture

### 1. Documentation Cell Type

```c
typedef struct {
    char* description;       // Manual description (optional)
    char* type_signature;    // Type (can be inferred)
    Cell* properties;        // List of properties (⊢ assertions)
    Cell* examples;          // List of examples
    Cell* dependencies;      // Auto-discovered: what this uses
    Cell* source_code;       // The actual code
    bool auto_generated;     // True if generated from constituents
} DocData;
```

### 2. Documentation Primitives

#### Core Operations
- `⌂` - Attach/create documentation
- `⌂?` - Has documentation?
- `⌂→` - Get full documentation object
- `⌂→∈` - Get type signature
- `⌂→⊢` - Get properties
- `⌂→Ex` - Get examples

#### Auto-Discovery
- `⌂↺` - Regenerate docs from constituents
- `⌂≔` - Get dependencies (what it uses)
- `⌂⇐` - Get dependents (what uses it)

### 3. Documentation Composition Rules

When documentation is requested for a function without explicit docs:

1. **Analyze AST:** Extract structure
   ```
   (λ (x y) (⊕ (⊗ x x) (⊗ y y)))
   ```

2. **Identify constituents:** `⊕`, `⊗`

3. **Retrieve constituent docs:**
   - `⊗`: "Multiply two numbers" (ℕ → ℕ → ℕ)
   - `⊕`: "Add two numbers" (ℕ → ℕ → ℕ)

4. **Compose description:**
   ```
   "Function that:
    - Takes parameters: x (ℕ), y (ℕ)
    - Computes: (x * x) + (y * y)
    - Returns: ℕ"
   ```

5. **Infer type:** From constituent types + usage
   ```
   (⊗ x x) : ℕ → ℕ (since x:ℕ and ⊗:ℕ→ℕ→ℕ)
   (⊗ y y) : ℕ → ℕ
   (⊕ a b) : ℕ → ℕ (where a,b both ℕ)
   Therefore: ℕ → ℕ → ℕ
   ```

### 4. Manual vs Auto Documentation

Users can provide partial documentation and the system fills in the rest:

```scheme
; Minimal doc: just description
(≔ sum-of-squares
  (⌂ "Computes x² + y²")  ; User provides this
  (λ (a b) (⊕ (square a) (square b))))

; System auto-generates:
; - Type: ℕ → ℕ → ℕ (inferred from usage)
; - Dependencies: [square, ⊕]
; - Properties: Commutative, always positive
; - Examples: Generated from property tests
```

## Implementation Plan

### Phase 2.1: Add CELL_DOC Type

1. **In cell.h:**
   ```c
   typedef enum {
       // ... existing types
       CELL_DOC          // Documentation cell
   } CellType;

   typedef struct {
       char* description;
       char* type_signature;
       Cell* properties;
       Cell* examples;
       Cell* dependencies;
       Cell* source_code;
       bool auto_generated;
   } DocData;
   ```

2. **Memory management:**
   - cell_doc() constructor
   - Cleanup in cell_release()
   - Printing in cell_print()

### Phase 2.2: Basic Documentation Primitives

Implement in primitives.c:

```c
Cell* prim_doc_create(Cell* args);     // ⌂
Cell* prim_doc_has(Cell* args);        // ⌂?
Cell* prim_doc_get(Cell* args);        // ⌂→
Cell* prim_doc_type(Cell* args);       // ⌂→∈
Cell* prim_doc_props(Cell* args);      // ⌂→⊢
Cell* prim_doc_examples(Cell* args);   // ⌂→Ex
```

### Phase 2.3: Documentation Storage

Store docs in a global registry:

```c
typedef struct {
    Cell* name_symbol;
    Cell* doc_cell;
    Cell* next;
} DocEntry;

typedef struct {
    DocEntry* head;
} DocRegistry;
```

When `≔` defines a function, automatically register it:
```c
void eval_define(EvalContext* ctx, const char* name, Cell* value) {
    // ... existing code ...

    // Register in doc system
    doc_registry_add(name, value);
}
```

### Phase 2.4: Auto-Documentation Generation

```c
Cell* doc_generate_auto(Cell* name_symbol, Cell* value) {
    if (cell_is_lambda(value)) {
        // Extract lambda structure
        Cell* body = value->data.lambda.body;
        int arity = value->data.lambda.arity;

        // Analyze body to find dependencies
        Cell* deps = extract_dependencies(body);

        // Compose documentation from dependencies
        Cell* description = compose_description(deps, arity);

        // Infer type from usage
        Cell* type_sig = infer_type(body, deps);

        // Create doc cell
        return cell_doc(description, type_sig, deps, NULL, true);
    }
    return NULL;
}
```

### Phase 2.5: Dependency Analysis

```c
Cell* extract_dependencies(Cell* expr) {
    // Traverse AST and collect all symbols
    // Filter out parameters and locals
    // Return list of free variables (global functions/primitives)

    if (cell_is_symbol(expr)) {
        // Check if it's a global/primitive
        return cell_cons(expr, cell_nil());
    }

    if (cell_is_pair(expr)) {
        Cell* first_deps = extract_dependencies(cell_car(expr));
        Cell* rest_deps = extract_dependencies(cell_cdr(expr));
        return merge_unique(first_deps, rest_deps);
    }

    return cell_nil();
}
```

### Phase 2.6: Type Inference (Simple)

For Phase 2, implement simple type inference:

```c
Cell* infer_type(Cell* body, Cell* deps) {
    // Look up types of dependencies
    // Compose based on usage
    // For now: return string like "α → β"

    // Future: Proper Hindley-Milner inference in Phase 5
    return cell_symbol(":inferred-type");
}
```

### Phase 2.7: Documentation Composition

```c
char* compose_description(Cell* deps, int arity) {
    // Build description from dependency docs
    char* result = malloc(1024);
    sprintf(result, "Function with %d parameter(s) that uses: ", arity);

    Cell* current = deps;
    while (cell_is_pair(current)) {
        Cell* dep = cell_car(current);
        Cell* dep_doc = doc_registry_lookup(dep);

        if (dep_doc) {
            strcat(result, cell_get_symbol(dep));
            strcat(result, " (");
            strcat(result, get_doc_description(dep_doc));
            strcat(result, "), ");
        }

        current = cell_cdr(current);
    }

    return result;
}
```

## Usage Examples

### Example 1: Primitive Documentation

```scheme
; Built-in primitives come with docs
(⌂→ ⊕)
; → "Add two numbers"

(⌂→∈ ⊕)
; → "ℕ → ℕ → ℕ"
```

### Example 2: User Function (Auto-Generated)

```scheme
; Define without explicit docs
(≔ square (λ (x) (⊗ x x)))

; Auto-generated docs available
(⌂ square)
; → "Function with 1 parameter that uses: ⊗ (multiply two numbers)"

(⌂→∈ square)
; → "ℕ → ℕ"  (inferred from ⊗ usage)
```

### Example 3: User Function (Manual + Auto)

```scheme
; Provide partial docs
(≔ factorial
  (⌂ "Computes n!"
     (⊢ "(! #0) ≡ #1")
     (⊢ "(! n) ≡ (⊗ n (! (⊖ n #1)))")
     (Ex "(! #5)" "#120"))
  (λ (n) (? (≡ n #0) #1 (⊗ n (! (⊖ n #1))))))

; System augments with auto-discovered info
(⌂→≔ factorial)
; → Dependencies: [⊗, ⊖, ≡, ?, factorial]

(⌂→∈ factorial)
; → "ℕ → ℕ"  (inferred)
```

### Example 4: Dependency Graph

```scheme
(≔ square (λ (x) (⊗ x x)))
(≔ sum-of-squares (λ (a b) (⊕ (square a) (square b))))

; Query dependencies
(⌂→≔ sum-of-squares)
; → [⊕, square]

; Query reverse dependencies
(⌂→⇐ square)
; → [sum-of-squares]
```

## Benefits

### 1. **Guaranteed Documentation**
Every symbol in the system has documentation, even if auto-generated.

### 2. **Composable Knowledge**
Understanding flows from primitives upward through all levels.

### 3. **Self-Describing Code**
```scheme
(⌂ my-complex-function)
; Always returns something useful, even if user didn't document it
```

### 4. **Interactive Learning**
Users can explore the system by querying documentation:
```scheme
(⌂→ mystery-function)    ; What does this do?
(⌂→≔ mystery-function)   ; What does it use?
(⌂→∈ mystery-function)   ; What's its type?
```

### 5. **Refactoring Safety**
When dependencies change, documentation automatically updates.

## Testing

Create comprehensive tests:

```scheme
; Test: Primitive docs exist
(⊨ (⌜ :test-prim-docs) #t (⌂? ⊕))
(⊨ (⌜ :test-prim-type) :ℕ→ℕ→ℕ (⌂→∈ ⊕))

; Test: Auto-generation
(≔ test-fn (λ (x) (⊕ x #1)))
(⊨ (⌜ :test-auto-doc) #t (⌂? test-fn))
(⊨ (⌜ :test-auto-deps) (⟨⟩ :⊕ ∅) (⌂→≔ test-fn))

; Test: Dependency tracking
(≔ f (λ (x) (⊕ x #1)))
(≔ g (λ (x) (⊗ (f x) #2)))
(⊨ (⌜ :test-deps) (⟨⟩ :⊗ (⟨⟩ :f ∅)) (⌂→≔ g))
```

## Implementation Effort

- **Phase 2.1:** CELL_DOC type - 2-3 hours
- **Phase 2.2:** Basic primitives - 2-3 hours
- **Phase 2.3:** Storage - 2 hours
- **Phase 2.4:** Auto-generation - 4-5 hours
- **Phase 2.5:** Dependency analysis - 3-4 hours
- **Phase 2.6:** Type inference (simple) - 2-3 hours
- **Phase 2.7:** Composition - 3-4 hours
- **Testing:** 2-3 hours

**Total: 20-28 hours**

## Future Enhancements (Phase 5+)

1. **Proper Type Inference:** Hindley-Milner
2. **Property Inference:** Detect commutativity, associativity automatically
3. **Example Generation:** Generate examples from properties
4. **Proof Verification:** Verify properties hold
5. **Documentation Search:** Full-text search across all docs
6. **Documentation Export:** Generate static HTML/markdown docs

---

**This design ensures every symbol in Guage is documented, either explicitly or automatically, making the entire system self-describing and explorable.**
