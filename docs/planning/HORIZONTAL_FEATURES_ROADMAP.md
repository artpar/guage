---
Status: CURRENT
Created: 2026-01-27
Updated: 2026-01-27
Purpose: Roadmap for horizontal language features (core capabilities)
---

# Horizontal Features Roadmap

## Philosophy Shift

**Decision:** Deprioritize stdlib expansion, focus on **horizontal features** that enable broader capabilities.

**Why:**
- Stdlib is nice-to-have, not essential for language progress
- Horizontal features unlock new possibilities (metaprogramming, self-hosting, etc.)
- Core language capabilities more impactful than library functions
- Can build stdlib later once foundations are stronger

---

## Priority 1: IMMEDIATE IMPACT (Days 32-35)

### A. REPL Enhancements (Day 32-33 - 8 hours) ⭐ START HERE

**Goal:** Better developer experience with improved REPL

**Features:**

1. **Help System** (3 hours)
   ```scheme
   :help              ; List all commands
   :help ⊕            ; Show primitive documentation
   :doc map           ; Show function documentation
   :type ⊕            ; Show type signature
   ```
   - Use existing ⌂, ⌂∈, ⌂≔, ⌂⊛ primitives
   - Pretty-print documentation
   - Search by category

2. **Module Introspection** (2 hours)
   ```scheme
   :modules           ; List loaded modules
   :exports list.scm  ; Show module exports
   :imports           ; Show all imported symbols
   :provenance map    ; Show where symbol defined
   ```
   - Use existing ⌂⊚, ⌂⊚→, ⌂⊛ primitives
   - Clean formatting

3. **Better Error Display** (3 hours)
   - Pretty-print error values
   - Show error context (stack trace)
   - Suggest fixes for common errors
   - Colorize output (if terminal supports)

**Impact:** Immediate usability improvement, makes Guage pleasant to use

**Files to modify:**
- `bootstrap/bootstrap/main.c` - REPL loop
- Add new command parser for `:help`, `:doc`, etc.
- Enhance error printing

---

### B. Parser Improvements (Day 34 - 6 hours)

**Goal:** Better error handling and source tracking

**Features:**

1. **Line Number Tracking** (3 hours)
   - Track source position during parsing
   - Store line numbers in lambda cells (infrastructure exists!)
   - Enhanced ⌂⊛ provenance (already has :line field, just need parser support)

2. **Better Parse Errors** (2 hours)
   - Show line/column where parse failed
   - Helpful error messages ("Expected closing paren", not "Parse error")
   - Suggest fixes for common mistakes

3. **Multi-line Input** (1 hour)
   - REPL should handle incomplete expressions
   - Prompt for continuation when paren balance incomplete
   - Example:
     ```scheme
     > (≔ factorial
     ... (λ (n)
     ... (? (≡ n #0) #1
     ... (⊗ n (factorial (⊖ n #1))))))
     ```

**Impact:** Better error messages = faster debugging, happier developers

**Files to modify:**
- `bootstrap/bootstrap/main.c` - Parser with position tracking
- `bootstrap/bootstrap/eval.c` - Store line numbers in cells
- `bootstrap/bootstrap/debug.c` - Enhanced stack traces with line numbers

---

## Priority 2: METAPROGRAMMING FOUNDATIONS (Days 36-40)

### C. Macro System (Days 36-38 - 12 hours) 🔥

**Goal:** Hygenic code transformation capabilities

**Design:**

```scheme
; Syntax: (⧉ name (params...) template)
; Template uses backquote ` with unquote , and splice ,@

; Example: when macro
(⧉ when (condition body)
  `(? ,condition ,body ∅))

; Usage:
(when (> x #0) (⊕ x #1))
; Expands to:
(? (> x #0) (⊕ x #1) ∅)

; Example: unless macro
(⧉ unless (condition body)
  `(? (¬ ,condition) ,body ∅))

; Example: cond macro (multi-clause conditional)
(⧉ cond clauses
  `(∇ #t (⌜ ,@clauses)))

; Usage:
(cond
  [(< x #0) :negative]
  [(> x #0) :positive]
  [#t :zero])
```

**Implementation:**

1. **Quasiquote/Unquote** (4 hours)
   - `` ` `` (backquote) - Quote with holes
   - `,` (unquote) - Evaluate in quoted context
   - `,@` (splice) - Splice list elements
   - Primitives: `⌞` (eval) exists, add quasiquote support

2. **Macro Definition** (4 hours)
   - `⧉` primitive - Define macro
   - Store macros in environment (separate from functions)
   - Macro expansion at read time (before eval)

3. **Hygiene** (4 hours)
   - Gensym for temporary variables
   - Avoid variable capture
   - Proper scoping

**Impact:** Enables DSLs, syntax extensions, code generation

**Tests:** 30+ macro tests

---

### D. Self-Hosting Preparation (Days 39-40 - 8 hours)

**Goal:** Begin writing Guage in Guage

**Milestones:**

1. **S-expression Parser in Guage** (4 hours)
   ```scheme
   (≔ parse-sexpr (λ (str)
     ; Tokenize
     ; Parse tokens into S-expressions
     ; Handle quotes, lists, atoms
     ))
   ```
   - Use string primitives (≈, ≈⊕, ≈⊂, etc.)
   - Pattern matching for token types
   - Returns AST as nested lists

2. **Simple Evaluator in Guage** (4 hours)
   ```scheme
   (≔ eval-guage (λ (expr) (λ (env)
     (∇ expr
       [(:symbol s) (lookup env s)]
       [(:number n) n]
       [(:list (⟨⟩ (⌜ λ) rest)) ...]
       ...))))
   ```
   - Subset of eval.c in Guage
   - Prove language can express itself
   - Bootstrap path

**Impact:** Major milestone toward self-hosting

---

## Priority 3: ADVANCED FEATURES (Days 41-50)

### E. Effect System (Days 41-44 - 16 hours)

**Goal:** Algebraic effects for controlled side effects

**Currently:** Placeholders (⟪⟫, ↯, ⤴, ≫)

**Design:**
```scheme
; Effect definition
(⟪⟫ :State
  [:get () → α]
  [:put α → ()])

; Effect handler
(↯ (⟪⟫ :State)
  {:initial #0
   :handlers
    {:get (λ (state k) ((k state) state))
     :put (λ (state k) (λ (new-state) ((k ∅) new-state)))}})

; Effect usage
(⤴ :get)      ; Perform effect
(⤴ :put #42)  ; Perform effect
```

**Implementation:**
- Effect types as ADTs
- Handler as special form
- Continuation-passing style
- Effect tracking in types (future)

**Impact:** Controlled side effects, testable I/O, concurrent programming

---

### F. Type Inference (Days 45-47 - 12 hours)

**Goal:** Automatic type inference for better safety

**Currently:** Runtime untyped, compile-time types planned

**Design:**
- Hindley-Milner type inference
- Type equations from code
- Unification algorithm
- Type error reporting

**Impact:** Catch errors at compile time, better documentation

---

### G. Optimizer (Days 48-50 - 12 hours)

**Goal:** Performance improvements through optimization

**Optimizations:**
1. **Tail Call Optimization** - Constant stack space
2. **Constant Folding** - Evaluate constants at compile time
3. **Inline Expansion** - Remove function call overhead
4. **Dead Code Elimination** - Remove unused code

**Impact:** Better performance, more efficient code

---

## Priority 4: DISTRIBUTION & PACKAGING (Days 51-60)

### H. Package Manager

**Goal:** Share and reuse Guage code

**Features:**
- Package registry
- Dependency resolution
- Semantic versioning
- Module installation

### I. Native Compilation

**Goal:** Compile Guage to native code

**Path:** Guage → C → Native (or Guage → LLVM → Native)

---

## Implementation Timeline

| Days | Feature | Impact | Priority |
|------|---------|--------|----------|
| 32-33 | REPL enhancements | Immediate UX improvement | ⭐⭐⭐ |
| 34 | Parser improvements | Better errors | ⭐⭐⭐ |
| 36-38 | Macro system | Metaprogramming | ⭐⭐⭐ |
| 39-40 | Self-hosting prep | Major milestone | ⭐⭐ |
| 41-44 | Effect system | Controlled effects | ⭐⭐ |
| 45-47 | Type inference | Safety | ⭐ |
| 48-50 | Optimizer | Performance | ⭐ |
| 51-60 | Package manager & compilation | Distribution | ⭐ |

---

## Success Metrics

### Short-term (Days 32-35)
- [ ] REPL help system working
- [ ] Module introspection commands
- [ ] Better error messages
- [ ] Line number tracking in parser
- [ ] Multi-line REPL input

### Mid-term (Days 36-40)
- [ ] Macro system complete (⧉, `, ,, ,@)
- [ ] 30+ macro tests passing
- [ ] S-expression parser in Guage
- [ ] Simple evaluator in Guage
- [ ] Bootstrap proof-of-concept

### Long-term (Days 41-60)
- [ ] Effect system working
- [ ] Type inference basic version
- [ ] Optimization passes implemented
- [ ] Package manager prototype

---

## Current Status

**Where we are:**
- ✅ Turing complete language
- ✅ Pattern matching complete
- ✅ Module system complete
- ✅ 78 functional primitives
- ✅ 850+ tests passing
- ✅ 54 stdlib functions

**What's next:**
1. REPL enhancements (Days 32-33) ← START HERE
2. Parser improvements (Day 34)
3. Macro system (Days 36-38)
4. Self-hosting preparation (Days 39-40)

**Stdlib work:** DEFERRED (lowest priority, revisit after horizontal features)

---

## Why This Approach?

**Horizontal features:**
- ✅ Unlock new capabilities (metaprogramming, self-hosting)
- ✅ Make language more powerful
- ✅ Enable better tooling (REPL, debugger)
- ✅ Critical for language maturity

**Stdlib functions:**
- ❌ Nice-to-have, not essential
- ❌ Can be written by users
- ❌ Debugging individual functions time-consuming
- ❌ Better to have solid foundations first

**Result:** Stronger language core, better developer experience, faster path to self-hosting

---

**Prepared by:** Claude Sonnet 4.5
**Date:** 2026-01-27
**Phase:** Week 4+ Planning (Days 32-60)
**Next:** Implement REPL enhancements (Day 32-33)
