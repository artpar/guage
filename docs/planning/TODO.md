# Guage TODO List

## CRITICAL: Make Turing Complete

### Phase 0.1: Lambda Calculus Core (BLOCKING)
**Status: 🔴 CRITICAL - Nothing works without this**

- [ ] **De Bruijn Index Evaluation**
  - [ ] `eval.c`: Add De Bruijn index lookup in environment
  - [ ] Environment as De Bruijn index list (not assoc list)
  - [ ] Variable reference by position (0, 1, 2...)

- [ ] **Lambda Abstraction**
  - [ ] `cell.h`: Lambda cell stores body only (no env needed yet)
  - [ ] `eval.c`: Create closure with captured environment
  - [ ] Proper closure representation

- [ ] **Function Application**
  - [ ] `eval.c`: Implement beta reduction
  - [ ] Apply function to argument
  - [ ] Environment extension for application
  - [ ] Test: `((λ.0) 5)` → `5`

- [ ] **Recursion**
  - [ ] Option A: Y combinator in library
  - [ ] Option B: Named recursion with ≔
  - [ ] Test factorial: `! ≔ λ.((≡ 0 0) 1 (⊗ 0 (! (⊖ 0 1))))`

**Tests to pass:**
```lisp
; Identity
((λ.0) 42) → 42

; Const
((λ.λ.1) 5 10) → 5

; Compose
(((λ.λ.λ.(2 (1 0))) (λ.(⊕ 0 1)) (λ.(⊗ 0 2))) 3) → 7

; Factorial
(! 5) → 120
```

---

## Phase 0.2: Self-Implementation Basics

### Metaprogramming
- [ ] **Proper ⌞ Eval**
  - [ ] `eval.c`: Eval quoted expression
  - [ ] Test: `(⌞ (⌜ (⊕ 1 2)))` → `3`

- [ ] **Code Manipulation**
  - [ ] Library functions for AST manipulation
  - [ ] Pattern matching on cells
  - [ ] Test: Write eval in Guage itself

**Self-implementation test:**
- [ ] Write minimal eval in Guage
- [ ] Bootstrap: C runtime → Guage eval → Guage eval

---

## Phase 1: Parser Improvements

### Current Parser Issues
- [ ] Parser doesn't handle Unicode symbols properly
- [ ] No error messages
- [ ] No line numbers
- [ ] Can't parse De Bruijn indices as bare numbers

### Fixes Needed
- [ ] Better tokenizer for Unicode
- [ ] Distinguish `0` (De Bruijn) from `#0` (number)
- [ ] Error reporting with position
- [ ] Comments support

---

## Phase 2: Standard Library (Symbolic Only)

### Core Definitions
```lisp
; Identity
𝕀 ≔ λ.0

; Const
𝕂 ≔ λ.λ.1

; Compose
∘ ≔ λ.λ.λ.(2 (1 0))

; Church booleans
𝕋 ≔ λ.λ.1  ; True
𝔽 ≔ λ.λ.0  ; False

; Church numerals
0̅ ≔ λ.λ.0
1̅ ≔ λ.λ.(1 0)
2̅ ≔ λ.λ.(1 (1 0))

; List constructors
⟐ ≔ λ.λ.λ.((≡ 1 ∅) 2 (0 (◁ 1) ((▷ 1) 0 2)))  ; fold
```

- [ ] Write stdlib in pure symbols
- [ ] No English words anywhere
- [ ] Test coverage for all functions

---

## Phase 3: Type System (Compile-Time)

**Status: Not started**

### Type Checker (Separate from runtime)
- [ ] Dependent types (Π, Σ)
- [ ] Linear types (⊸, !, ?)
- [ ] Session types (▷, ◁, ⊕, &, ε)
- [ ] Refinement types ({⋅∣φ})
- [ ] Effect types (⟪⟫, ↯, ⤴, ≫)
- [ ] Capability types (CAP_*)

### Type Checker Files
- [ ] `types/dependent.guage` - Pi/Sigma types
- [ ] `types/linear.guage` - Linearity checker
- [ ] `types/session.guage` - Session type checker
- [ ] `types/refinement.guage` - SMT integration
- [ ] `types/effect.guage` - Effect tracker

---

## Phase 4: Effects Runtime

### Actor System
- [ ] `actor.c`: Lightweight green threads
- [ ] `actor.c`: Message queue per actor
- [ ] `primitives.c`: Real ⟳ spawn implementation
- [ ] `primitives.c`: Real →! send implementation
- [ ] `primitives.c`: Real ←? receive implementation

### Effect Handlers
- [ ] `effect.c`: Delimited continuations
- [ ] `effect.c`: Effect handler stack
- [ ] `primitives.c`: Real ⟪⟫ implementation
- [ ] `primitives.c`: Real ↯ implementation

---

## Phase 5: Garbage Collection

### Current: Simple Refcounting
- Problem: Cycles leak memory
- Problem: Not concurrent

### Upgrade to:
- [ ] Precise GC with mark-sweep
- [ ] Generational GC
- [ ] Concurrent GC for actors
- [ ] Linear types reduce GC pressure

---

## Phase 6: Optimization

### Zero-Cost Abstractions
- [ ] Inline primitives
- [ ] Lambda lifting
- [ ] Dead code elimination
- [ ] Const propagation

### Compiler
- [ ] Guage → C codegen
- [ ] LLVM backend
- [ ] Native compilation

---

## Phase 7: Design Patterns as Types

**Status: Future**

- [ ] `patterns/solid.guage` - SOLID enforcement
- [ ] `patterns/gof.guage` - GoF patterns
- [ ] `patterns/architecture.guage` - Hexagonal, CQRS
- [ ] `patterns/typestate.guage` - State machines

---

## Phase 8: Proof-Carrying Code

**Status: Future**

- [ ] `proofs/core.guage` - Proof language
- [ ] `proofs/tactics.guage` - Proof automation
- [ ] Lean 4 integration
- [ ] Verified standard library

---

## Testing

### Unit Tests
- [ ] Test suite for primitives
- [ ] Test suite for lambda evaluation
- [ ] Test suite for type checker
- [ ] Property-based testing

### Integration Tests
- [ ] Self-interpreter test
- [ ] Factorial, fibonacci
- [ ] QuickSort in pure Guage
- [ ] Actor ping-pong
- [ ] Effect handler examples

### Benchmarks
- [ ] vs Python
- [ ] vs JavaScript
- [ ] vs Racket
- [ ] Goal: Match C performance

---

## Documentation

- [x] README.md
- [x] SPEC.md
- [ ] TUTORIAL.md (pure symbols only!)
- [ ] EXAMPLES.md
- [ ] API reference
- [ ] Type system guide

---

## Tooling

### IDE Support
- [ ] VS Code extension
- [ ] Syntax highlighting
- [ ] De Bruijn → names hover
- [ ] Type hints
- [ ] Error squiggles

### Formatter
- [ ] Automatic code formatter (like gofmt)
- [ ] No configuration options
- [ ] One true style

### REPL Improvements
- [ ] Multi-line input
- [ ] History
- [ ] Tab completion
- [ ] Pretty printing with colors

---

## Current Blockers (DO THESE FIRST)

1. 🔴 **CRITICAL**: Lambda evaluation not implemented
2. 🔴 **CRITICAL**: De Bruijn indices not working
3. 🔴 **CRITICAL**: Cannot write recursive functions
4. 🟡 **HIGH**: Parser doesn't handle all symbols
5. 🟡 **HIGH**: No error messages

## Next 3 Tasks

1. Implement De Bruijn index lookup
2. Implement lambda abstraction (closure creation)
3. Implement application (beta reduction)

**Once these 3 are done: Language is Turing complete.**

---

## Questions to Answer

- [ ] Y combinator or named recursion?
- [ ] Call-by-value or call-by-name?
- [ ] Eager or lazy evaluation?
- [ ] Stack machine or graph reduction?

## Current Line Count

- Bootstrap C: ~1000 LOC
- Spec: Complete
- Type system: 0 LOC
- Stdlib: 0 LOC

**Total: ~1000 / 17000 LOC (6% complete)**
