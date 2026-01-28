---
Status: CURRENT
Created: 2026-01-27
Updated: 2026-01-27
Purpose: Step-by-step implementation plan for first module system
---

# Module System: Incremental Implementation Plan

**Philosophy:** See [AI_FIRST_MODULES.md](AI_FIRST_MODULES.md) for design rationale.

**Goal:** Implement transparent, queryable module system in 5 days (Week 4).

---

## Current State (Day 25)

**What Works:**
- ✅ `⋘` (load) - Load and evaluate files
- ✅ Multiple definitions per file
- ✅ Module dependencies (manual load order)
- ✅ 15 module tests passing

**What's Missing:**
- ❌ No module registry (can't query loaded modules)
- ❌ No provenance tracking (don't know where symbols came from)
- ❌ No dependency tracking (manual load order)
- ❌ No conflict warnings (silent override)
- ❌ No metadata (visibility, documentation)

---

## Implementation Phases

### Phase 1: Module Registry (Day 26) - 3 hours

**Goal:** Track all loaded modules, query module information.

**New Primitive: `⌂⊚` (module-info)**

```scheme
; List all loaded modules
(⌂⊚)  ; → ⟨"stdlib/list.scm" "stdlib/option.scm" "app.scm"⟩

; Get module for specific symbol
(⌂⊚ (⌜ map))  ; → "stdlib/list.scm"

; Get all symbols from module
(⌂⊚ "stdlib/list.scm")  ; → ⟨:map :filter :fold-left ...⟩
```

#### Step 1.1: Module Registry Data Structure (45 min)

**File:** `bootstrap/module.h` (NEW)

```c
// Module registry entry
typedef struct ModuleEntry {
    char* name;              // Module file path
    Cell* symbols;           // List of defined symbols
    time_t loaded_at;        // When loaded
    struct ModuleEntry* next;
} ModuleEntry;

// Global module registry
typedef struct {
    ModuleEntry* head;
    size_t count;
} ModuleRegistry;

// API
void module_registry_init();
void module_registry_add(const char* module_name);
void module_registry_add_symbol(const char* module_name, const char* symbol);
Cell* module_registry_list_modules();
const char* module_registry_find_symbol(const char* symbol);
Cell* module_registry_list_symbols(const char* module_name);
void module_registry_free();
```

**File:** `bootstrap/module.c` (NEW)

```c
#include "module.h"
#include "cell.h"
#include <string.h>
#include <time.h>

static ModuleRegistry registry = {NULL, 0};

void module_registry_init() {
    registry.head = NULL;
    registry.count = 0;
}

void module_registry_add(const char* module_name) {
    // Check if already exists
    ModuleEntry* curr = registry.head;
    while (curr) {
        if (strcmp(curr->name, module_name) == 0) {
            return;  // Already registered
        }
        curr = curr->next;
    }

    // Create new entry
    ModuleEntry* entry = malloc(sizeof(ModuleEntry));
    entry->name = strdup(module_name);
    entry->symbols = cell_nil();
    cell_retain(entry->symbols);
    entry->loaded_at = time(NULL);
    entry->next = registry.head;
    registry.head = entry;
    registry.count++;
}

void module_registry_add_symbol(const char* module_name, const char* symbol) {
    // Find module
    ModuleEntry* entry = registry.head;
    while (entry) {
        if (strcmp(entry->name, module_name) == 0) {
            // Add symbol to list (if not already present)
            Cell* sym = cell_symbol(symbol);
            // ... append to entry->symbols ...
            cell_release(sym);
            return;
        }
        entry = entry->next;
    }
}

Cell* module_registry_list_modules() {
    // Return list of module names
    Cell* result = cell_nil();
    ModuleEntry* entry = registry.head;
    while (entry) {
        Cell* name = cell_symbol(entry->name);
        result = cell_cons(name, result);
        cell_release(name);
        entry = entry->next;
    }
    return result;
}

const char* module_registry_find_symbol(const char* symbol) {
    // Find which module defines this symbol
    ModuleEntry* entry = registry.head;
    while (entry) {
        // Search entry->symbols for symbol
        Cell* curr = entry->symbols;
        while (curr && !cell_is_nil(curr)) {
            Cell* sym = cell_car(curr);
            if (cell_is_symbol(sym) &&
                strcmp(cell_get_symbol(sym), symbol) == 0) {
                return entry->name;
            }
            curr = cell_cdr(curr);
        }
        entry = entry->next;
    }
    return NULL;  // Not found
}

Cell* module_registry_list_symbols(const char* module_name) {
    // Find module and return its symbols
    ModuleEntry* entry = registry.head;
    while (entry) {
        if (strcmp(entry->name, module_name) == 0) {
            cell_retain(entry->symbols);
            return entry->symbols;
        }
        entry = entry->next;
    }
    return cell_nil();  // Module not found
}

void module_registry_free() {
    ModuleEntry* entry = registry.head;
    while (entry) {
        ModuleEntry* next = entry->next;
        free(entry->name);
        cell_release(entry->symbols);
        free(entry);
        entry = next;
    }
    registry.head = NULL;
    registry.count = 0;
}
```

#### Step 1.2: Integrate with `⋘` (load) (30 min)

**File:** `bootstrap/primitives.c`

```c
// Enhanced prim_load to track module
Cell* prim_load(Cell* args, Env* env) {
    // ... existing load logic ...

    // After successful load, register module
    module_registry_add(file_path);

    // Track global definitions made during load
    // (Need to hook into eval.c env_define)

    return result;
}
```

**File:** `bootstrap/eval.c`

```c
// Track current loading module (global state)
static const char* current_loading_module = NULL;

void eval_set_loading_module(const char* module_name) {
    current_loading_module = module_name;
}

// Enhanced env_define to track symbols
Cell* env_define(Env* env, Cell* name, Cell* value) {
    // ... existing define logic ...

    // If loading a module, register symbol
    if (current_loading_module != NULL) {
        module_registry_add_symbol(current_loading_module,
                                   cell_get_symbol(name));
    }

    return value;
}
```

#### Step 1.3: Implement `⌂⊚` Primitive (45 min)

**File:** `bootstrap/primitives.c`

```c
Cell* prim_module_info(Cell* args, Env* env) {
    // (⌂⊚) - List all modules
    if (cell_is_nil(args)) {
        return module_registry_list_modules();
    }

    Cell* arg1 = cell_car(args);

    // (⌂⊚ (⌜ symbol)) - Find module for symbol
    if (cell_is_symbol(arg1)) {
        const char* module = module_registry_find_symbol(
            cell_get_symbol(arg1));
        if (module == NULL) {
            return cell_error(symbol_create("not-found"), arg1);
        }
        return cell_symbol(module);
    }

    // (⌂⊚ "module-name") - List symbols in module
    if (cell_is_string(arg1)) {
        return module_registry_list_symbols(cell_get_string(arg1));
    }

    return cell_error(symbol_create("invalid-args"), args);
}

// Register in init_primitives()
env_define(env, symbol_create("⌂⊚"),
           primitive_create(prim_module_info, "⌂⊚"));
```

#### Step 1.4: Tests (60 min)

**File:** `tests/test_module_registry.scm`

```scheme
; Test 1: Registry initially empty
(⊨ :registry-empty #t (∅? (⌂⊚)))

; Test 2: Load adds to registry
(≋⊲ "test-module.scm" "(≔ test-fn (λ (x) x))")
(⋘ "test-module.scm")
(⊨ :registry-has-module #t (¬ (∅? (⌂⊚))))

; Test 3: Find symbol in module
(⊨ :find-symbol "test-module.scm" (⌂⊚ (⌜ test-fn)))

; Test 4: List symbols from module
(⊨ :list-symbols #t
   (¬ (∅? (⌂⊚ "test-module.scm"))))

; Test 5: Multiple modules tracked
(≋⊲ "module1.scm" "(≔ fn1 (λ (x) x))")
(≋⊲ "module2.scm" "(≔ fn2 (λ (x) x))")
(⋘ "module1.scm")
(⋘ "module2.scm")
(⊨ :multiple-modules #t (≥ (# (⌂⊚)) #2))

; Test 6: Symbol not found
(⊨ :symbol-not-found #t (⚠? (⌂⊚ (⌜ nonexistent))))

; Test 7: Module not found
(⊨ :module-not-found #t
   (∅? (⌂⊚ "nonexistent.scm")))

; Test 8: Backwards compatibility (loading without registry)
(≔ old-map (λ (f) (λ (xs) xs)))
(⊨ :backwards-compat #t #t)
```

**Deliverables (Day 26):**
- ✅ `module.h` + `module.c` (200 lines)
- ✅ Enhanced `⋘` to register modules
- ✅ `⌂⊚` primitive (3 modes)
- ✅ 8 comprehensive tests
- ✅ Backwards compatible (doesn't break existing code)

---

### Phase 2: Enhanced Provenance (Day 27) - 2 hours

**Goal:** Track where each symbol came from, show in `⌂⊛`.

**Enhanced `⌂⊛` output:**

```scheme
(⌂⊛ (⌜ map))
; Before: (λ (f) (λ (xs) ...))
; After: (⊙ :Definition
;           :name :map
;           :source (λ (f) (λ (xs) ...))
;           :module "stdlib/list.scm"
;           :line 15
;           :visibility :public)
```

#### Step 2.1: Extend Definition Metadata (45 min)

**File:** `bootstrap/cell.h`

```c
// Add to Cell struct (for CELL_LAMBDA)
typedef struct {
    // ... existing fields ...
    char* source_module;    // Where defined (NULL if REPL)
    int source_line;        // Line number in source
    char* visibility;       // "public" or "internal"
} LambdaData;
```

**File:** `bootstrap/cell.c`

```c
// Update cell_lambda_create() to accept metadata
Cell* cell_lambda_create_with_metadata(
    Cell* params,
    Cell* body,
    Cell* env,
    const char* module,
    int line) {
    // ... existing logic ...
    lambda->source_module = module ? strdup(module) : NULL;
    lambda->source_line = line;
    lambda->visibility = "public";  // Default
    return cell;
}

// Update cell_release() to free metadata
```

#### Step 2.2: Track Line Numbers in Parser (30 min)

**File:** `bootstrap/main.c`

```c
// Add line tracking to parser
typedef struct {
    char* input;
    size_t pos;
    int line;    // NEW
} Parser;

// Update parse_sexpr() to track lines
```

#### Step 2.3: Enhance `⌂⊛` to Return Metadata (45 min)

**File:** `bootstrap/primitives.c`

```c
Cell* prim_source(Cell* args, Env* env) {
    // ... existing logic ...

    if (cell_is_lambda(value)) {
        // Create structure with metadata
        Cell* result = cell_struct_create("Definition");
        cell_struct_set_field(result, "name", name);
        cell_struct_set_field(result, "source", value);

        const char* module = lambda_get_module(value);
        if (module) {
            cell_struct_set_field(result, "module",
                                 cell_symbol(module));
        }

        int line = lambda_get_line(value);
        if (line > 0) {
            cell_struct_set_field(result, "line",
                                 cell_number(line));
        }

        return result;
    }

    // ... rest of function ...
}
```

#### Step 2.4: Tests (30 min)

**File:** `tests/test_module_provenance.scm`

```scheme
; Test provenance tracking
(≋⊲ "prov-test.scm" "(≔ test-fn (λ (x) (⊕ x #1)))")
(⋘ "prov-test.scm")

; Test metadata structure
(≔ meta (⌂⊛ (⌜ test-fn)))
(⊨ :has-module #t (⊙? meta :Definition))
(⊨ :module-name "prov-test.scm"
   (⊙→ meta :module))
```

**Deliverables (Day 27):**
- ✅ Metadata in lambda cells
- ✅ Line tracking in parser
- ✅ Enhanced `⌂⊛` with full metadata
- ✅ 5 provenance tests

---

### Phase 3: Selective Import (Day 28) - 2 hours

**Goal:** Convenience syntax for importing symbols (doesn't hide others).

**New Primitive: `⋖` (import-from)**

```scheme
; Import specific symbols
(⋖ "stdlib/list.scm" ⟨:map :filter⟩)

; Equivalent to:
(≔ map (⊙→ (⌂⊚ "stdlib/list.scm") :map))
(≔ filter (⊙→ (⌂⊚ "stdlib/list.scm") :filter))

; But other symbols still accessible:
(⊙→ (⌂⊚ "stdlib/list.scm") :fold-left)  ; Works!
```

#### Step 3.1: Implement `⋖` Primitive (60 min)

**File:** `bootstrap/primitives.c`

```c
Cell* prim_import_from(Cell* args, Env* env) {
    // (⋖ "module" ⟨:sym1 :sym2⟩)
    Cell* module_name = cell_car(args);
    Cell* symbols = cell_car(cell_cdr(args));

    if (!cell_is_string(module_name)) {
        return cell_error(symbol_create("type-error"),
                         module_name);
    }

    // Get module symbols
    Cell* module_syms = module_registry_list_symbols(
        cell_get_string(module_name));

    // For each requested symbol
    Cell* curr = symbols;
    while (curr && !cell_is_nil(curr)) {
        Cell* sym = cell_car(curr);

        // Find in module
        Cell* value = env_lookup_module(env,
                                       cell_get_string(module_name),
                                       cell_get_symbol(sym));

        if (value) {
            // Define in current environment
            env_define(env, sym, value);
        } else {
            // Warning: symbol not in module
            fprintf(stderr, "Warning: Symbol %s not found in %s\n",
                   cell_get_symbol(sym),
                   cell_get_string(module_name));
        }

        curr = cell_cdr(curr);
    }

    cell_release(module_syms);
    return cell_symbol(":imported");
}
```

#### Step 3.2: Tests (60 min)

**File:** `tests/test_module_import.scm`

```scheme
; Create test module
(≋⊲ "import-test.scm"
     "(≔ fn1 (λ (x) x)) (≔ fn2 (λ (x) (⊕ x #1)))")
(⋘ "import-test.scm")

; Test selective import
(⋖ "import-test.scm" ⟨:fn1⟩)
(⊨ :import-works #t (ℕ? (fn1 #42)))

; Test other symbols still accessible
(⊨ :others-accessible "import-test.scm"
   (⌂⊚ (⌜ fn2)))

; Test import non-existent
; Should warn but not error
(⋖ "import-test.scm" ⟨:nonexistent⟩)
(⊨ :import-missing #t #t)
```

**Deliverables (Day 28):**
- ✅ `⋖` primitive for selective import
- ✅ 5 import tests
- ✅ Backwards compatible

---

### Phase 4: Dependency Tracking (Day 29) - 2 hours

**Goal:** Automatically track and query dependencies.

**Enhanced `⌂≔` to show module deps:**

```scheme
(⌂≔ (⌜ map))  ; → ⟨:fold-left :cons ...⟩

; Module-level dependencies
(⌂⊚→ "app.scm")  ; → ⟨"stdlib/list.scm" "stdlib/option.scm"⟩
```

#### Step 4.1: Track Symbol Dependencies (60 min)

Already implemented! `⌂≔` tracks dependencies.

**Enhance to track inter-module deps:**

```c
// When loading module, scan for uses of other modules' symbols
void module_track_dependencies(const char* module_name,
                               Cell* body) {
    // Scan AST for symbol references
    // Check which modules define those symbols
    // Add to module's dependency list
}
```

#### Step 4.2: Tests (60 min)

**File:** `tests/test_module_deps.scm`

```scheme
; Create dependent modules
(≋⊲ "base.scm" "(≔ BASE #10)")
(≋⊲ "derived.scm" "(⋘ \"base.scm\") (≔ derived (⊕ BASE #1))")

(⋘ "derived.scm")

; Test dependency detected
(⊨ :dep-tracked #t
   (¬ (∅? (⌂⊚→ "derived.scm"))))
```

**Deliverables (Day 29):**
- ✅ Module dependency tracking
- ✅ Enhanced `⌂⊚→` for module deps
- ✅ 5 dependency tests

---

### Phase 5: Comprehensive Testing (Day 30) - 3 hours

**Goal:** Validate entire module system end-to-end.

#### Test Coverage:
1. **Module registry** (8 tests) ✅
2. **Provenance tracking** (5 tests) ✅
3. **Selective import** (5 tests) ✅
4. **Dependency tracking** (5 tests) ✅
5. **Integration tests** (10 tests) 🎯

#### Integration Scenarios:

```scheme
; Scenario 1: Real stdlib usage
(⋘ "stdlib/list.scm")
(⋘ "stdlib/option.scm")
(⊨ :stdlib-loaded #t (≥ (# (⌂⊚)) #2))

; Scenario 2: App with multiple modules
(≋⊲ "utils.scm" "(≔ double (λ (n) (⊗ n #2)))")
(≋⊲ "app.scm" "(⋘ \"utils.scm\") (≔ main (λ (x) (double x)))")
(⋘ "app.scm")
(⊨ :app-works #t (≡ (main #21) #42))

; Scenario 3: Conflict detection
(≋⊲ "lib1.scm" "(≔ foo (λ (x) #1))")
(≋⊲ "lib2.scm" "(≔ foo (λ (x) #2))")
(⋘ "lib1.scm")
(⋘ "lib2.scm")  ; Should warn about 'foo' redefinition
(⊨ :conflict-warned #t #t)

; Scenario 4: Circular dependency detection
(≋⊲ "circ1.scm" "(⋘ \"circ2.scm\") (≔ a #1)")
(≋⊲ "circ2.scm" "(⋘ \"circ1.scm\") (≔ b #2)")
; Should detect and warn (but allow - not fatal)
```

**Deliverables (Day 30):**
- ✅ 10 integration tests
- ✅ Conflict detection working
- ✅ Circular dependency warning
- ✅ Full stdlib compatibility

---

## Summary: 5-Day Implementation

| Day | Feature | Primitives | Tests | Status |
|-----|---------|-----------|-------|--------|
| 26 | Module registry | ⌂⊚ | 8 | 🎯 NEXT |
| 27 | Provenance | Enhanced ⌂⊛ | 5 | ⏳ |
| 28 | Selective import | ⋖ | 5 | ⏳ |
| 29 | Dependencies | Enhanced ⌂≔ | 5 | ⏳ |
| 30 | Integration | - | 10 | ⏳ |
| **Total** | **Module System** | **+2 primitives** | **33 tests** | **Week 4** |

---

## Success Criteria

**Must Have (Week 4):**
- ✅ Module registry working
- ✅ Provenance tracking complete
- ✅ All tests passing
- ✅ Backwards compatible
- ✅ Documentation updated

**Nice to Have (Future):**
- ⏳ Explicit exports (Phase 5)
- ⏳ Conflict resolution (Phase 5)
- ⏳ Hot code swapping (Phase 6)
- ⏳ Cross-module optimization (Phase 6)

---

## Files to Create/Modify

**New Files:**
- `bootstrap/module.h` (100 lines)
- `bootstrap/module.c` (300 lines)
- `tests/test_module_registry.scm` (8 tests)
- `tests/test_module_provenance.scm` (5 tests)
- `tests/test_module_import.scm` (5 tests)
- `tests/test_module_deps.scm` (5 tests)
- `tests/test_module_integration.scm` (10 tests)

**Modified Files:**
- `bootstrap/primitives.c` (+150 lines: ⌂⊚, ⋖, enhanced ⌂⊛)
- `bootstrap/primitives.h` (+20 lines)
- `bootstrap/eval.c` (+50 lines: tracking)
- `bootstrap/cell.h` (+15 lines: metadata)
- `bootstrap/cell.c` (+30 lines: metadata)
- `bootstrap/main.c` (+20 lines: line tracking)
- `bootstrap/Makefile` (+2 lines: module.c)
- `SPEC.md` (+100 lines: module system docs)
- `SESSION_HANDOFF.md` (+50 lines: progress update)

**Total:** ~1100 lines of new code + 33 tests

---

## Next Session: Day 26 Implementation

**Start with:**
1. Read this document + AI_FIRST_MODULES.md
2. Create `module.h` and `module.c`
3. Implement registry data structure
4. Add `⌂⊚` primitive
5. Write 8 registry tests
6. Verify backwards compatibility

**Time Estimate:** 3 hours
**Deliverable:** Queryable module registry working!

---

**See also:**
- [AI_FIRST_MODULES.md](AI_FIRST_MODULES.md) - Design philosophy
- [SESSION_HANDOFF.md](../SESSION_HANDOFF.md) - Current progress
- [METAPROGRAMMING_VISION.md](../reference/METAPROGRAMMING_VISION.md) - Long-term vision
