---
Status: CURRENT
Created: 2026-01-27
Purpose: Incremental AI-first module system implementation
---

# Incremental AI-First Module System

## Problem with Big Bang Approach

Changing ⋘ to return Module instead of evaluating is **too disruptive**:
- Breaks all existing code
- Requires immediate implementation of ⋖ (import)
- Need to update all stdlib files
- Need to change how REPL loads files

## Better: Incremental Enhancement

**Keep ⋘ working as-is**, but add AI-query primitives alongside:

```scheme
; Current behavior (KEEP THIS!)
(⋘ "stdlib/list.scm")       ; Evaluates, returns last expr
(map double (list #1 #2))   ; Works immediately

; NEW: Query what was loaded (AI-friendly!)
(⌂⊚)                         ; List all loaded modules
(⌂⊚ "stdlib/list.scm")       ; Get metadata for specific module
(⌂⊛ (⌜ map))                 ; Where was 'map' defined?
```

---

## Phase 1: Module Registry (Week 4, Day 26)

### Implementation (2-3 hours)

**Add global module registry:**
```c
// In eval.c or new module.c
typedef struct {
    char* path;
    Cell* definitions;  // List of (symbol . value) pairs
    Cell* exports;      // List of symbols
    Cell* dependencies; // List of paths
} ModuleInfo;

static ModuleInfo* modules = NULL;
static int module_count = 0;
```

**Enhance ⋘ to track modules:**
```c
Cell* prim_load(Cell* args) {
    // ... existing code ...

    // NEW: Track what was defined
    Cell* old_env_snapshot = capture_env_snapshot(ctx);

    // Evaluate all expressions (existing code)
    // ...

    // NEW: Capture new definitions
    Cell* new_defs = diff_env_snapshots(old_env_snapshot, ctx->env);

    // NEW: Register module
    register_module(filename, new_defs, exports, dependencies);

    // Return result as before (backwards compatible!)
    return result;
}
```

**Add query primitives:**
```scheme
; ⌂⊚ (module-list) - List all loaded modules
(⌂⊚)  ; → ["stdlib/list.scm" "stdlib/math.scm" ...]

; ⌂⊚ (module-get) - Get metadata for module
(⌂⊚ "stdlib/list.scm")  ; → Module structure with metadata

; Enhanced ⌂⊛ - Get source module for symbol
(⌂⊛ (⌜ map))  ; → "stdlib/list.scm"
```

### Tests
```scheme
; Load a module
(⋘ "test.scm")

; Query registry
(≔ mods (⌂⊚))
(⊨ :has-modules #t (⟨⟩? mods))

; Get specific module metadata
(≔ mod-info (⌂⊚ "test.scm"))
(⊨ :has-path #t (⊙? mod-info :path))

; Query provenance
(≔ source (⌂⊛ (⌜ my-function)))
(⊨ :correct-source "test.scm" source)
```

---

## Phase 2: Explicit Exports (Week 4, Day 27)

### Implementation (1-2 hours)

**Add ⊙◇ (export-declare) primitive:**
```scheme
; In module file:
(≔ helper (λ (x) (⊕ x #1)))      ; Internal
(≔ public (λ (x) (helper x)))    ; Public

; Declare exports (end of file)
(⊙◇ :exports (⌜ [public]))
```

**Implementation:**
```c
Cell* prim_export_declare(Cell* args) {
    Cell* exports = arg1(args);  // List of symbols

    // Store in module metadata (last loaded module)
    set_module_exports(current_module(), exports);

    return cell_nil();
}
```

**AI can still see everything:**
```scheme
(≔ mod (⌂⊚ "math.scm"))
(⊙→ mod :exports)    ; → [public]
(⊙→ mod :defs)       ; → All defs (including helper!)
```

---

## Phase 3: Selective Import (Week 4, Day 28)

### Implementation (2 hours)

**Add ⋖ (import) primitive for selective importing:**
```scheme
; Current: Everything auto-imported
(⋘ "stdlib/list.scm")
(map ...)  ; Works

; NEW: Load without importing
(⋘! "stdlib/list.scm")  ; Load but don't import

; Explicitly import what you need
(⋖ "stdlib/list.scm" (⌜ [map filter]))
(map ...)  ; Works
(fold ...) ; ERROR: not imported

; But AI can still see everything!
(⊙→ (⌂⊚ "stdlib/list.scm") :defs)  ; All functions visible!
```

---

## Phase 4: Dependency Tracking (Week 5, Day 29)

### Implementation (2 hours)

**Automatic dependency detection:**
```c
// In prim_load, detect nested ⋘ calls
// Track as dependencies in module metadata
```

**Dependency graph queries:**
```scheme
(⌂⟿ :modules)  ; → Dependency graph as ⊝ structure

; Query dependencies
(⊝→ (⌂⟿ :modules) "derived.scm" :deps)  ; → ["base.scm"]

; Query dependents
(⊝→ (⌂⟿ :modules) "base.scm" :dependents)  ; → ["derived.scm" ...]
```

---

## Benefits of Incremental Approach

**✅ Backwards Compatible**
- Existing code keeps working
- No breaking changes
- Progressive adoption

**✅ Immediate Value**
- Module registry available immediately
- AI queries work right away
- Provenance tracking functional

**✅ Lower Risk**
- Small, testable changes
- Can validate each phase
- Easy to debug

**✅ AI-Friendly from Day 1**
- Everything queryable
- No information hiding
- Transparent by design

---

## Comparison: Traditional vs Incremental

### Traditional Module System (WRONG)
```python
# math.py
def _helper(): ...  # Hidden from AI!
def public(): ...

# main.py
from math import public  # Only public visible
# AI CANNOT see _helper
# AI CANNOT query module structure
```

### Guage Incremental (RIGHT)
```scheme
; math.scm
(≔ helper (λ (x) ...))  ; Visible to AI!
(≔ public (λ (x) (helper x)))
(⊙◇ :exports (⌜ [public]))

; main.scm
(⋘ "math.scm")  ; Auto-imports (default)

; AI can query EVERYTHING:
(⌂⊚ "math.scm")          ; → Module metadata
(⊙→ (⌂⊚ "math.scm") :defs)  ; → ALL definitions (including helper!)
(⌂⊛ (⌜ helper))         ; → "math.scm"
```

---

## Implementation Timeline

**Week 4:**
- Day 26: Module registry (3h)
- Day 27: Explicit exports (2h)
- Day 28: Selective import (2h)

**Week 5:**
- Day 29: Dependency tracking (2h)
- Day 30: Comprehensive tests (3h)
- Day 31: Documentation (2h)

**Total: ~14 hours over 6 days**

---

## Success Metrics

**Must Have:**
- ✅ ⌂⊚ lists all modules
- ✅ ⌂⊚ gets specific module metadata
- ✅ ⌂⊛ shows provenance
- ✅ All code visible (no hiding)
- ✅ Backwards compatible

**Should Have:**
- ✅ ⊙◇ declares exports
- ✅ ⋖ selective import
- ✅ Dependency tracking

**Future:**
- ⋘! load without import
- Module composition (⊙⊕, ⊙⊲)
- Hot code swapping

---

**This approach is pragmatic, incremental, and AI-first from day one.**

