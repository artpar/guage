---
Status: CURRENT
Created: 2026-01-27
Updated: 2026-01-27
Purpose: Design first module system for Guage ultralanguage
---

# First Module System Design

## Vision: Transparency Over Encapsulation

**Core Principle:** In an ultralanguage, **everything must be queryable, provable, and transformable**. Traditional module systems with imports/exports/namespaces **hide information** that needs to reason about code.

**Guage's Approach:** Modules are **first-class values** with **full transparency** and **provenance tracking**.

---

## What's Wrong with Traditional Modules?

### Traditional Approach (Python, JavaScript, etc.)
```python
# module.py
def _internal_helper():  # Hidden from outside
    return 42

def public_api():
    return _internal_helper()

# client.py
from module import public_api  # Can't see _internal_helper
```

**Problems:**
1. **Information Hiding** - can't see `_internal_helper` implementation
2. **Selective Visibility** - Import lists restrict what's available
3. **Namespace Isolation** - Code is fragmented into silos
4. **Not Queryable** - Can't analyze full program structure
5. **Not Transformable** - Can't modify "private" functions

### Why This Fails for Ultralanguage

**Guage's Mission:**
- **Queryable** - CFG/DFG are first-class values you can query
- **Provable** - Types prove properties at compile time
- **Transformable** - Code synthesizes/repairs automatically
- **Assisted** - Compiler helps you write, test, optimize code

**Information hiding breaks all of this!**

---

## Guage's Solution: Module Registry + Provenance

### Principle 1: Everything is Visible (But Tagged)

Instead of hiding code, **tag it with metadata**:

```scheme
; Module defines functions with provenance
(⋘ "stdlib/list.scm")  ; Loads all definitions

; Every symbol now has provenance:
(⌂⊛ (⌜ map))  ; → Returns source + metadata
; Metadata includes:
; - Origin module: "stdlib/list.scm"
; - Visibility: :public | :internal
; - Dependencies: (filter, fold-left, ...)
; - Type signature: (α → β) → [α] → [β]
```

**Key Insight:** "Private" is **documentation**, not **restriction**:
- can still see and analyze everything
- Humans get guidance on what's stable API
- No actual information hiding

### Principle 2: Modules as First-Class Values

Modules are **data structures** you can query:

```scheme
; Create module registry (automatic on load)
(⋘ "stdlib/list.scm")  ; Adds to registry

; Query loaded modules
(⌂⊚)  ; → List of all loaded modules
; Returns: ⟨(⊙ :Module
;             :name "stdlib/list.scm"
;             :symbols ⟨:map :filter :fold-left ...⟩
;             :dependencies ⟨⟩
;             :loaded-at "2026-01-27T10:30:00")
;          ...⟩

; Get module for specific symbol
(⌂⊚ (⌜ map))  ; → "stdlib/list.scm"

; Get all symbols from module
(⌂⊚ "stdlib/list.scm")  ; → ⟨:map :filter :fold-left ...⟩
```

### Principle 3: Provenance Tracking

Every definition knows its origin:

```scheme
; Original ⌂⊛ (get source)
(⌂⊛ (⌜ map))  ; → Function body only

; Enhanced ⌂⊛ with provenance
(⌂⊛ (⌜ map))
; Returns:
; (⊙ :Definition
;    :name :map
;    :source (λ (f) (λ (xs) ...))
;    :module "stdlib/list.scm"
;    :line 15
;    :visibility :public
;    :dependencies ⟨:fold-left :cons ...⟩
;    :type "(α → β) → [α] → [β]")
```

### Principle 4: No Namespace Collisions (By Warning)

Instead of isolation, **warn on conflicts**:

```scheme
; Load first module
(⋘ "lib1.scm")  ; Defines `foo`

; Load second module
(⋘ "lib2.scm")  ; Also defines `foo`
; ⚠ Warning: Symbol 'foo' redefined
; → Previous: lib1.scm:10
; → New: lib2.scm:5
; → Use (⌂⊚ (⌜ foo)) to see current binding

; Explicitly choose which `foo`
(≔ lib1-foo (⊙→ (⌂⊚ "lib1.scm") :foo))
(≔ lib2-foo (⊙→ (⌂⊚ "lib2.scm") :foo))
```

**Why This Works:**
- sees both definitions (full transparency)
- Developer gets warning (not silent override)
- Explicit disambiguation (no magic)
- All code visible for analysis

---

## Implementation: Three Phases

### Phase 1: Module Registry (Day 26) - 3 hours

**New Primitive: `⌂⊚` (list/get modules)**

```scheme
; List all loaded modules
(⌂⊚)  ; → ⟨(⊙ :Module ...) (⊙ :Module ...) ...⟩

; Get module for symbol
(⌂⊚ (⌜ map))  ; → "stdlib/list.scm"

; Get symbols from module
(⌂⊚ "stdlib/list.scm")  ; → ⟨:map :filter ...⟩
```

**Implementation:**
1. Add `ModuleRegistry` struct to track loaded modules
2. Enhance `⋘` to register modules when loaded
3. Add `⌂⊚` primitive for querying registry
4. Store module metadata (name, symbols, load time)

**Test Coverage:**
- Query all modules
- Query symbol origin
- Query module symbols
- Handle unloaded modules

### Phase 2: Enhanced Provenance (Day 27) - 2 hours

**Enhance `⌂⊛` to include module info:**

```scheme
(⌂⊛ (⌜ map))
; Returns:
; (⊙ :Definition
;    :name :map
;    :source <lambda>
;    :module "stdlib/list.scm"
;    :visibility :public)
```

**Implementation:**
1. Extend definition metadata structure
2. Track module name when loading
3. Return full metadata from `⌂⊛`
4. Add visibility hints (documentation only)

### Phase 3: Dependency Tracking (Day 28) - 2 hours

**Automatic dependency graph:**

```scheme
; Get dependencies
(⌂≔ (⌜ map))  ; → ⟨:fold-left :cons ...⟩

; Get module dependencies
(⌂⊚→ "stdlib/list.scm")  ; → ⟨⟩ (no module deps yet)

; Future: Module-level dependencies
(⋘ "app.scm")  ; Loads stdlib/list.scm automatically
```

**Implementation:**
1. Track symbol dependencies when defining functions
2. Build module dependency graph
3. Detect circular dependencies (warning, not error)
4. Enable automatic dependency resolution

---

## Advanced Features (Future)

### 1. Explicit "Exports" (Documentation Only)

```scheme
; Module file: stdlib/list.scm

; Mark stable API (documentation, not restriction)
(⊙◇ :public ⟨:map :filter :fold-left :fold-right⟩)

; Everything else is :internal (but still visible!)
(≔ _list-helper (λ (xs) ...))  ; can still see this
```

**Key:** `:public` vs `:internal` is **metadata**, not access control.

### 2. Selective Import (Convenience, Not Hiding)

```scheme
; Bring symbols into scope (doesn't hide others)
(⋖ "stdlib/list.scm" ⟨:map :filter⟩)

; Short syntax instead of:
(≔ map (⊙→ (⌂⊚ "stdlib/list.scm") :map))
(≔ filter (⊙→ (⌂⊚ "stdlib/list.scm") :filter))

; Other symbols still accessible:
(⊙→ (⌂⊚ "stdlib/list.scm") :fold-left)  ; Works!
```

**Key:** Import is **convenience**, not **restriction**.

### 3. Conflict Resolution

```scheme
; Load conflicting modules
(⋘ "lib1.scm")  ; Defines `foo`
(⋘ "lib2.scm")  ; Also defines `foo`
; ⚠ Warning: 'foo' redefined

; Explicit resolution
(⋖ "lib1.scm" ⟨:foo⟩ :as ⟨:foo1⟩)
(⋖ "lib2.scm" ⟨:foo⟩ :as ⟨:foo2⟩)
```

### 4. Module Hot-Swapping

```scheme
; Reload module (update all references)
(⇝ "stdlib/list.scm")  ; Reloads and updates

; Swap implementations
(⇝ "old-impl.scm" "new-impl.scm")  ; Hot code swap
```

---

## Comparison: Traditional vs Guage

| Feature | Traditional | Guage |
|---------|-------------|-------|
| **Information Access** | Hidden (private) | Transparent (tagged) |
| **Imports** | Restrict visibility | Convenience only |
| **Exports** | Enforce boundaries | Document intent |
| **Namespace** | Isolated scopes | Single scope + provenance |
| **Dependencies** | Manual declaration | Automatic tracking |
| **Conflicts** | Compile error | Warning + resolution |
| **Analysis** | Limited (can't see private) | Full (everything visible) |
| **Queryability** | Restricted | Complete |
| **Transformability** | Blocked | Enabled |

---

## Why This Enables the Ultralanguage Vision

### 1. Assisted Development
```scheme
; can see and analyze EVERYTHING:
(⌂⊚)  ; List all modules
(⌂⊛ (⌜ map))  ; See full implementation + metadata
(⌂⟿ (⌜ map))  ; Get CFG (including "private" helpers)
(⌂⇝ (⌜ map))  ; Get DFG (full data flow)

; No hidden code means:
; - Better code suggestions
; - Accurate refactoring
; - Complete analysis
; - Full-program optimization
```

### 2. Cross-Module Analysis
```scheme
; Load multiple modules
(⋘ "service1.scm")
(⋘ "service2.scm")

; Analyze interactions (see METAPROGRAMMING_VISION.md)
(≔ π₁ (⋘ (⌜ :service1)))
(≔ π₂ (⋘ (⌜ :service2)))
(⊙⋈ π₁ π₂)  ; Joint CFG/DFG analysis
(⊢ (⊙⋈) (¬ deadlock))  ; Prove no deadlock
```

### 3. Program Synthesis & Repair
```scheme
; Synthesize implementation from spec
(≔ spec (⌜ (∀ xs (sorted? (sort xs)))))
(≔ sort (⊛ spec))  ; Synthesize

; Repair broken function
(≔ broken-map ...)  ; Buggy implementation
(⌂⊚ (⌜ broken-map))  ; See module origin
(≔ fixed-map (⊛ spec ◂ broken-map))  ; Repair
```

### 4. Time-Travel Debugging
```scheme
; Full program trace (including "private" functions)
(≔ τ (⊙⊳ (main)))
(⊇ τ (⌜ ⟨t∷42⟩⌝))  ; State at step 42
(⊇ τ (⌜ ⟨←∷_list-helper⟩⌝))  ; Even "internal" functions visible!
```

---

## Philosophy: Why Transparency Beats Encapsulation

### Traditional View (OOP/Modules)
> "Encapsulation protects implementation details, enables information hiding, prevents misuse."

**Problem:** Information hiding blocks analysis, transformation, and reasoning.

### Ultralanguage View (Guage)
> "Transparency enables analysis, transformation, and assistance. Document intent without hiding information."

**Benefits:**
- **Full visibility** → Better analysis
- **Metadata** → Guide without restricting
- **Provenance** → Know where everything came from
- **First-class** → Modules are queryable values
- **friendly** → See full context for reasoning

### The Key Insight

**In traditional languages:** Compiler is a black box, code is hidden, analysis is limited.

**In Guage:** Compiler is a library, code is data, everything is queryable.

This is what makes Guage an **ultralanguage**.

---

## Next Steps

**Immediate (Week 4):**
1. Implement module registry (Day 26)
2. Enhance provenance tracking (Day 27)
3. Add dependency tracking (Day 28)
4. Write comprehensive tests (Day 29)

**Future (Phase 4+):**
1. Explicit exports (documentation)
2. Selective imports (convenience)
3. Conflict resolution
4. Hot code swapping
5. Cross-module optimization

**Validation:**
- Does this enable reasoning? ✅
- Does this maintain queryability? ✅
- Does this support transformation? ✅
- Does this align with ultralanguage vision? ✅

---

## Summary

**Traditional modules:** Hide information, restrict access, create silos.

**Guage modules:** Transparent by design, first-class values, fully queryable.

This isn't just "different" - it's **fundamentally necessary** for an ultralanguage where can see, analyze, and transform all code.

**The ultralanguage promise:** Everything is a value. Everything is queryable. Everything is transformable.

Traditional module systems **break this promise**. Guage's transparent module system **keeps it**.

---

**Next:** Implement module registry primitive (⌂⊚) - See MODULE_SYSTEM_INCREMENTAL.md
