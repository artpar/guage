# Metaprogramming Implementation Plan

**Date:** 2026-01-27
**Goal:** Add industrial-strength metaprogramming to Guage
**Philosophy:** Structural (not textual), friendly, zero-cost abstractions

---

## Executive Summary

Implement **three interconnected metaprogramming systems** in order:

1. **Pattern Matching** (Foundation) - 2-4 weeks
2. **Hygienic Macros** (Code Generation) - 4-6 weeks
3. **Generic Programming** (Parametric Polymorphism) - 6-8 weeks

**Total Timeline:** 12-18 weeks to full metaprogramming

---

## Why This Order?

```
∇ Pattern Matching (Foundation)
  ↓ (enables structural manipulation)
⧉ Macros (Code Transformation)
  ↓ (enables generic syntax)
⊳ Generics (Parametric Code)
  ↓ (enables zero-cost abstractions)
⊧ Traits (Constraints)
```

**Pattern matching must come first** - it's the foundation for both macros and generics.

---

## Phase 1: Pattern Matching (CRITICAL FOUNDATION)

### Timeline: 2-4 weeks

### Why Critical?

**Macros need patterns:**
```scheme
(⧉ let (⧈ (bindings body)
  (∇ bindings  ; ← Need pattern matching!
    [((⟨⟩ (⟨⟩ var val) rest)) ...]
    [∅ body])))
```

**Generics need patterns:**
```scheme
(≔ map (λ (⊳ A) (λ (⊳ B) (λ (f) (λ (lst)
  (∇ lst  ; ← Need pattern matching!
    [∅ ∅]
    [(⟨⟩ h t) (⟨⟩ (f h) (map f t))]))))))
```

**Current situation:** Only have manual if/car/cdr chains - painful and error-prone.

### Primitives to Add

#### 1. `∇` - Pattern Match Expression

**Type:** `∇ : α → [(pattern → β)] → β`

**Syntax:**
```scheme
(∇ expr
  [pattern₁ result₁]
  [pattern₂ result₂]
  ...
  [patternₙ resultₙ])
```

**Pattern Types:**
- **Literal numbers:** `#42` - matches exactly `#42`
- **Literal booleans:** `#t`, `#f` - matches exactly
- **Literal nil:** `∅` - matches nil
- **Symbols:** `:foo` - matches symbol `:foo`
- **Wildcard:** `_` - matches anything, doesn't bind
- **Variable:** `x` - matches anything, binds to name
- **Pair:** `(⟨⟩ pat₁ pat₂)` - matches pair, recursive
- **Nested:** `(⟨⟩ (⟨⟩ a b) c)` - deeply nested patterns

**Examples:**
```scheme
; Match on list
(∇ lst
  [∅ :empty]
  [(⟨⟩ x ∅) :single]
  [(⟨⟩ x (⟨⟩ y ∅)) :pair]
  [(⟨⟩ _ _) :many])

; Match on binary tree
(∇ tree
  [:leaf value]
  [:node (⟨⟩ left (⟨⟩ value right))])

; Match with nested patterns
(∇ expr
  [(:⊕ (⟨⟩ a (⟨⟩ b ∅))) (handle-add a b)]
  [(:⊗ (⟨⟩ a (⟨⟩ b ∅))) (handle-mul a b)])
```

#### 2. `≗` - Structural Equality

**Type:** `≗ : α → α → 𝔹`

**Purpose:** Deep structural comparison (not reference equality)

**Examples:**
```scheme
(≗ (⟨⟩ #1 (⟨⟩ #2 ∅)) (⟨⟩ #1 (⟨⟩ #2 ∅)))  ; → #t
(≗ :foo :foo)  ; → #t
(≗ (λ (x) x) (λ (y) y))  ; → #f (different closures)
```

**Used by pattern matcher internally.**

### Implementation Strategy

#### Step 1: Extend Cell Type (Week 1)

**File:** `bootstrap/bootstrap/cell.h`

Add pattern representation:
```c
typedef enum {
    PATTERN_LITERAL,    // #42, #t, ∅, :symbol
    PATTERN_WILDCARD,   // _
    PATTERN_VARIABLE,   // x (binds)
    PATTERN_PAIR,       // (⟨⟩ pat₁ pat₂)
} PatternType;

typedef struct Pattern {
    PatternType type;
    union {
        Cell* literal;              // For PATTERN_LITERAL
        const char* variable;       // For PATTERN_VARIABLE
        struct {
            struct Pattern* car;    // For PATTERN_PAIR
            struct Pattern* cdr;
        } pair;
    } data;
} Pattern;
```

#### Step 2: Pattern Parser (Week 1)

**File:** `bootstrap/bootstrap/pattern.c` (new)

```c
// Parse cell into pattern
Pattern* parse_pattern(Cell* cell);

// Examples:
// #42 → PATTERN_LITERAL(#42)
// _ → PATTERN_WILDCARD
// x → PATTERN_VARIABLE("x")
// (⟨⟩ a b) → PATTERN_PAIR(PATTERN_VAR("a"), PATTERN_VAR("b"))
```

#### Step 3: Pattern Matcher (Week 2)

**File:** `bootstrap/bootstrap/pattern.c`

```c
// Match value against pattern, return bindings or NULL
typedef struct {
    const char* var;
    Cell* value;
} Binding;

typedef struct {
    Binding* bindings;
    int count;
} MatchResult;

MatchResult* pattern_match(Pattern* pat, Cell* value);

// Example:
// Pattern: (⟨⟩ x (⟨⟩ y ∅))
// Value: (⟨⟩ #1 (⟨⟩ #2 ∅))
// Result: {{"x", #1}, {"y", #2}}
```

**Algorithm:**
```
match(pattern, value):
    if pattern is LITERAL:
        return value == literal
    if pattern is WILDCARD:
        return true (no binding)
    if pattern is VARIABLE:
        return {var: value}
    if pattern is PAIR:
        if value is not pair:
            return FAIL
        left_bindings = match(pattern.car, value.car)
        right_bindings = match(pattern.cdr, value.cdr)
        return merge(left_bindings, right_bindings)
```

#### Step 4: ∇ Primitive (Week 2-3)

**File:** `bootstrap/bootstrap/primitives.c`

```c
Cell* prim_pattern_match(Cell* args) {
    Cell* expr = arg1(args);        // Value to match
    Cell* cases = arg2(args);       // List of [pattern result] pairs

    // Try each case in order
    for (Cell* case_list = cases; !cell_is_nil(case_list);
         case_list = cell_cdr(case_list)) {
        Cell* case_pair = cell_car(case_list);
        Pattern* pat = parse_pattern(cell_car(case_pair));
        Cell* result = cell_cadr(case_pair);

        // Try to match
        MatchResult* bindings = pattern_match(pat, expr);
        if (bindings) {
            // Success! Evaluate result with bindings
            return eval_with_bindings(result, bindings);
        }
    }

    // No match
    return cell_error("pattern-match-failed", expr);
}
```

#### Step 5: ≗ Primitive (Week 3)

**File:** `bootstrap/bootstrap/primitives.c`

```c
Cell* prim_structural_equal(Cell* args) {
    Cell* a = arg1(args);
    Cell* b = arg2(args);

    return cell_bool(structural_equal(a, b));
}

bool structural_equal(Cell* a, Cell* b) {
    // Same type?
    if (cell_type(a) != cell_type(b)) return false;

    // Compare based on type
    switch (cell_type(a)) {
        case CELL_ATOM_NUMBER:
            return cell_get_number(a) == cell_get_number(b);
        case CELL_ATOM_BOOL:
            return cell_get_bool(a) == cell_get_bool(b);
        case CELL_ATOM_SYMBOL:
            return strcmp(cell_get_symbol(a), cell_get_symbol(b)) == 0;
        case CELL_ATOM_NIL:
            return true;
        case CELL_PAIR:
            return structural_equal(cell_car(a), cell_car(b)) &&
                   structural_equal(cell_cdr(a), cell_cdr(b));
        case CELL_LAMBDA:
            return false;  // Lambdas not structurally comparable
        default:
            return false;
    }
}
```

#### Step 6: Testing (Week 4)

**File:** `bootstrap/bootstrap/tests/pattern_matching.test`

```scheme
; Basic patterns
(⊨ :pattern-number
   #42
   (∇ #42 [#42 :matched] [_ :not-matched]))

(⊨ :pattern-wildcard
   :ok
   (∇ #999 [_ :ok]))

; List patterns
(≔ length (λ (lst)
  (∇ lst
    [∅ #0]
    [(⟨⟩ _ tail) (⊕ #1 (length tail))])))

(⊨ :length-empty #0 (length ∅))
(⊨ :length-three #3 (length (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅)))))

; Nested patterns
(≔ deep-match (λ (x)
  (∇ x
    [(⟨⟩ (⟨⟩ a b) c) (⊕ a (⊕ b c))]
    [_ #0])))

(⊨ :deep-match #6 (deep-match (⟨⟩ (⟨⟩ #1 #2) #3)))

; Binary tree traversal
(≔ sum-tree (λ (tree)
  (∇ tree
    [∅ #0]
    [(⟨⟩ :leaf n) n]
    [(⟨⟩ :node (⟨⟩ left (⟨⟩ value right)))
     (⊕ value (⊕ (sum-tree left) (sum-tree right)))])))
```

### Deliverables

- [ ] Pattern type representation in cell.h
- [ ] Pattern parser (parse_pattern)
- [ ] Pattern matcher (pattern_match)
- [ ] ∇ primitive implemented
- [ ] ≗ primitive implemented
- [ ] Test suite passing (20+ tests)
- [ ] Documentation in SPEC.md
- [ ] Examples in examples/patterns.guage

---

## Phase 2: Hygienic Macro System

### Timeline: 4-6 weeks

### Prerequisites

- ✅ Pattern matching (Phase 1)
- ✅ Quote (⌜) already implemented
- Need: Macro expansion pipeline

### Primitives to Add

#### 1. `⧉` - Macro Definition

**Syntax:**
```scheme
(⧉ name (⧈ (param₁ param₂ ...) body))
```

**Example:**
```scheme
(⧉ when (⧈ (condition body)
  `(? ,condition ,body ∅)))
```

#### 2. `⧈` - Macro Parameters

**Purpose:** Mark parameters in macro definition

#### 3. `` ` `` - Backquote (Quasi-Quote)

**Purpose:** Quote with holes for evaluation

**Syntax:**
```scheme
`(template with ,hole1 and ,hole2)
```

**Example:**
```scheme
`(⊕ ,x #1)  ; x gets evaluated, rest quoted
```

#### 4. `,` - Unquote

**Purpose:** Evaluate expression inside backquote

#### 5. `,@` - Splice

**Purpose:** Insert list elements (not the list itself)

**Example:**
```scheme
(≔ args (list #1 #2 #3))
`(⊕ ,@args)  ; → (⊕ #1 #2 #3)
```

### Implementation Strategy

#### Step 1: Macro Table (Week 1)

**File:** `bootstrap/bootstrap/macro.h` (new)

```c
typedef struct {
    const char* name;
    Cell* params;        // List of parameter names
    Cell* body;          // Template with `,` holes
} MacroDef;

// Global macro registry
typedef struct {
    MacroDef* macros;
    int count;
    int capacity;
} MacroTable;

MacroTable* macro_table_create();
void macro_register(MacroTable* table, MacroDef* macro);
MacroDef* macro_lookup(MacroTable* table, const char* name);
```

#### Step 2: Backquote Expander (Week 2)

**File:** `bootstrap/bootstrap/macro.c`

```c
// Expand backquote template with substitutions
Cell* expand_backquote(Cell* template, Cell* bindings);

// Example:
// Template: `(⊕ ,x ,y)
// Bindings: {x: #5, y: #10}
// Result: (⊕ #5 #10)
```

**Algorithm:**
```
expand(template, bindings):
    if template is (unquote expr):
        return eval(expr, bindings)
    if template is (splice expr):
        return flatten(eval(expr, bindings))
    if template is pair:
        return cons(expand(car(template), bindings),
                   expand(cdr(template), bindings))
    else:
        return template
```

#### Step 3: Macro Expander (Week 3)

**File:** `bootstrap/bootstrap/macro.c`

```c
// Expand macro call
Cell* expand_macro(MacroDef* macro, Cell* args);

// Example:
// Macro: (⧉ when (⧈ (condition body) `(? ,condition ,body ∅)))
// Call: (when (> x #0) (⊕ x #1))
// Result: (? (> x #0) (⊕ x #1) ∅)
```

**Algorithm:**
```
expand_macro(macro, args):
    // Bind parameters to arguments
    bindings = zip(macro.params, args)

    // Expand template
    return expand_backquote(macro.body, bindings)
```

#### Step 4: Compilation Pipeline (Week 4)

**File:** `bootstrap/bootstrap/eval.c`

Add macro expansion phase:

```c
Cell* eval_with_macros(Cell* expr, MacroTable* macros) {
    // Check if expr is macro call
    if (cell_is_pair(expr)) {
        Cell* func = cell_car(expr);
        if (cell_is_symbol(func)) {
            MacroDef* macro = macro_lookup(macros, cell_get_symbol(func));
            if (macro) {
                // Expand macro
                Cell* expanded = expand_macro(macro, cell_cdr(expr));

                // Recursively expand result
                return eval_with_macros(expanded, macros);
            }
        }
    }

    // Not a macro, evaluate normally
    return eval(expr);
}
```

#### Step 5: Hygiene via De Bruijn (Week 5)

**Key insight:** De Bruijn indices provide automatic hygiene!

```scheme
; Traditional problem (with names):
(defmacro swap (a b)
  `(let ((tmp ,a))
     (setq ,a ,b)
     (setq ,b tmp)))

; If called with: (swap tmp x)
; Bug: tmp captured!

; Guage solution (De Bruijn):
(⧉ swap (⧈ (a b)
  `(λ (,a)          ; De Bruijn: 0
     (λ (,b)        ; De Bruijn: 1
       ,b))))       ; Refers to correct binding
```

**No variable capture possible** - indices are relative, not names.

#### Step 6: Testing (Week 6)

**File:** `bootstrap/bootstrap/tests/macros.test`

```scheme
; Basic macro
(⧉ when (⧈ (condition body)
  `(? ,condition ,body ∅)))

(⊨ :when-true #42 (when #t #42))
(⊨ :when-false ∅ (when #f #42))

; Let macro (multiple bindings)
(⧉ let (⧈ (bindings body)
  (∇ bindings
    [∅ body]
    [(⟨⟩ (⟨⟩ var val) rest)
     `((λ (,var) (let ,rest ,body)) ,val)])))

(⊨ :let-bindings #15
  (let ((x #5) (y #10))
    (⊕ x y)))

; Splice macro
(⧉ list-sum (⧈ (nums)
  `(⊕ ,@nums)))

(⊨ :splice #6 (list-sum #1 #2 #3))
```

### Deliverables

- [ ] Macro table and registration
- [ ] Backquote expander (`,` and `,@`)
- [ ] Macro expander
- [ ] Compilation pipeline with macro phase
- [ ] 10+ macro tests passing
- [ ] Standard macros (when, unless, let)
- [ ] Documentation
- [ ] Examples

---

## Phase 3: Generic Programming & Templates

### Timeline: 6-8 weeks

### Prerequisites

- ✅ Pattern matching (Phase 1)
- ✅ Macros (Phase 2)
- Need: Type system basics

### Primitives to Add

#### 1. `⊳` - Generic Parameter

**Syntax:**
```scheme
(λ (⊳ T) body)  ; T is type parameter
```

#### 2. `⊲` - Instantiation

**Syntax:**
```scheme
(⊲ generic-fn ⊳α)  ; Apply generic to type
```

#### 3. `⊧` - Trait Constraint

**Syntax:**
```scheme
(λ (⊳ T : (⊧ Ord)) ...)  ; T must satisfy Ord
```

### Implementation Strategy

#### Step 1: Type System Foundation (Week 1-2)

**File:** `bootstrap/bootstrap/types.h` (new)

```c
typedef enum {
    TYPE_CONCRETE,      // ℕ, 𝔹, etc
    TYPE_VARIABLE,      // α, β
    TYPE_FUNCTION,      // α → β
    TYPE_GENERIC,       // ⊳ T
} TypeKind;

typedef struct Type {
    TypeKind kind;
    union {
        const char* name;           // TYPE_CONCRETE, TYPE_VARIABLE
        struct {
            struct Type* param;     // TYPE_FUNCTION
            struct Type* result;
        } func;
        struct {
            const char* var;        // TYPE_GENERIC
            struct Type* body;
        } generic;
    } data;
} Type;
```

#### Step 2: Generic Functions (Week 3-4)

**File:** `bootstrap/bootstrap/generics.c` (new)

```c
// Instantiate generic function with type
Cell* instantiate_generic(Cell* generic, Type* type_arg);

// Example:
// Generic: (λ (⊳ T) (λ (x : T) x))
// Type: ℕ
// Result: (λ (x : ℕ) x)
```

**Algorithm:**
```
instantiate(generic, type_arg):
    // Parse generic: (λ (⊳ T) body)
    type_param = extract_type_param(generic)  // T
    body = extract_body(generic)

    // Substitute T → type_arg in body
    specialized = substitute_type(body, type_param, type_arg)

    // Return specialized function
    return specialized
```

#### Step 3: Monomorphization (Week 5-6)

**File:** `bootstrap/bootstrap/monomorphize.c` (new)

Generate separate code for each instantiation:

```c
// Track instantiations
typedef struct {
    Cell* generic;
    Type* type_arg;
    Cell* specialized;  // Cached result
} Instantiation;

// Monomorphization table
typedef struct {
    Instantiation* instances;
    int count;
} MonoTable;

Cell* monomorphize(Cell* program, MonoTable* table);
```

**Example:**
```scheme
; Source
(≔ id (λ (⊳ T) (λ (x : T) x)))
(⊲ id ℕ)
(⊲ id 𝔹)

; After monomorphization:
(≔ id_Nat (λ (x : ℕ) x))
(≔ id_Bool (λ (x : 𝔹) x))
```

#### Step 4: Trait System (Week 7-8)

**File:** `bootstrap/bootstrap/traits.c` (new)

```c
// Trait definition
typedef struct {
    const char* name;
    Cell* methods;  // List of (name, type) pairs
} Trait;

// Trait implementation
typedef struct {
    Type* type;
    Trait* trait;
    Cell* implementations;  // Method implementations
} TraitImpl;

// Check constraint
bool satisfies_constraint(Type* type, Trait* trait);
```

### Testing & Examples

```scheme
; Generic identity
(≔ id (λ (⊳ T) (λ (x : T) x)))
((⊲ id ℕ) #42)  ; → #42

; Generic map
(≔ map (λ (⊳ A) (λ (⊳ B) (λ (f : (A → B)) (λ (lst : (List A))
  (∇ lst
    [∅ ∅]
    [(⟨⟩ h t) (⟨⟩ (f h) ((((map A) B) f) t))]))))))

; Constrained generic
(≔ sort (λ (⊳ T : (⊧ Ord)) (λ (lst : (List T))
  (quicksort lst))))

((⊲ sort ℕ) (list #3 #1 #4))  ; → (list #1 #3 #4)
```

### Deliverables

- [ ] Type system foundation
- [ ] Generic function instantiation
- [ ] Monomorphization pass
- [ ] Trait system basics
- [ ] Generic standard library
- [ ] 20+ generic tests
- [ ] Documentation
- [ ] Performance benchmarks

---

## Success Criteria

### Phase 1 Complete When:
- [ ] Can write list functions with patterns
- [ ] Can match on binary trees
- [ ] No more manual car/cdr chains
- [ ] Test suite 100% passing

### Phase 2 Complete When:
- [ ] Can define control flow macros
- [ ] Can write let/letrec macros
- [ ] Hygiene verified (no capture)
- [ ] Standard macro library exists

### Phase 3 Complete When:
- [ ] Generic data structures (List, Tree, Map)
- [ ] Generic algorithms (sort, search)
- [ ] Trait-based polymorphism
- [ ] Zero-cost abstractions verified

---

## Risk Mitigation

### Risk: Pattern Matching Performance

**Problem:** Naive pattern matching is O(n) per case.

**Solution:** Compile patterns to decision trees (Phase 1.5, optional).

### Risk: Macro Expansion Loop

**Problem:** Recursive macro definitions could loop infinitely.

**Solution:** Limit expansion depth (configurable, default 1000).

### Risk: Code Bloat from Monomorphization

**Problem:** Many instantiations → large binaries.

**Solution:**
1. Dead code elimination (remove unused instantiations)
2. Specialization only when beneficial
3. Option for type erasure (future)

### Risk: Type Inference Complexity

**Problem:** Full Hindley-Milner is complex.

**Solution:** Start with explicit types, add inference gradually.

---

## Documentation Requirements

### For Each Phase:

1. **SPEC.md update** - Add new primitives
2. **Examples** - At least 5 working examples
3. **Tutorial** - Step-by-step guide
4. **Reference** - Complete API documentation
5. **Migration guide** - How to update existing code

---

## Performance Targets

### Pattern Matching:
- **Simple patterns** (literal, wildcard): O(1)
- **Nested patterns**: O(depth)
- **List patterns**: O(length)

### Macros:
- **Expansion time**: < 1ms per macro call
- **Compiled code**: Same performance as hand-written

### Generics:
- **Monomorphization**: < 100ms per instantiation
- **Zero overhead**: Generic code = specialized code

---

## Open Questions (Resolve in Phase 1)

1. **Pattern syntax:** Use Guage cells or special syntax?
   - **Decision:** Use cells (homoiconic)

2. **Macro hygiene:** De Bruijn sufficient or need more?
   - **Decision:** Try De Bruijn first, add gensym if needed

3. **Generic inference:** Explicit or implicit instantiation?
   - **Decision:** Start explicit, add inference later

4. **Trait dispatch:** Static or dynamic?
   - **Decision:** Static (monomorphization)

---

## Next Steps (This Week)

1. **Read this plan** - Understand the strategy
2. **Discuss with team** - Get feedback on approach
3. **Start Phase 1** - Begin pattern matching implementation
4. **Set up project tracking** - Use /tasks or GitHub issues

---

**END OF IMPLEMENTATION PLAN**

This plan provides a clear roadmap from current state (Turing complete, no metaprogramming) to industrial-strength metaprogramming (patterns, macros, generics) in ~18 weeks.
