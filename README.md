---
Status: CURRENT
Created: 2025-12-01
Updated: 2026-01-27
Purpose: Project overview and quick start
---

# Guage: The Ultralanguage

**A Turing-complete ultralanguage with pure symbolic syntax, designed to subsume all other languages through careful primitive design and systematic extension.**

## Quick Start

**New session?** Copy and paste: [`START_SESSION.txt`](START_SESSION.txt)

**Current Status:**
- ✅ **55 functional primitives** (ALL WORKING!)
- ✅ **408+ tests passing** (243 manual + 110 auto + 55 new)
- ✅ **Turing complete** with lambda calculus + De Bruijn indices
- ✅ **Week 2 Day 13 complete** - Week 3 pattern matching ready!
- 📍 **See:** [`SESSION_HANDOFF.md`](SESSION_HANDOFF.md) for detailed status

**Documentation:**
- Language specification → [`SPEC.md`](SPEC.md)
- Philosophy & principles → [`CLAUDE.md`](CLAUDE.md)
- All documentation → [`docs/INDEX.md`](docs/INDEX.md)

## Core Philosophy

Strongly opinionated, strongly enforced. Design patterns aren't conventions - they're TYPES. Bad code doesn't just lint poorly - it CANNOT COMPILE.

## Key Features

- **42 Pure Symbolic Primitives**: No English words, only Unicode symbols + De Bruijn indices
- **Dependent Types (Gradual)**: PUNK-style gradual dependent types
- **Linear Types**: Austral-style resource management
- **Session Types**: Deadlock-free communication protocols
- **Algebraic Effects**: Koka 3.x style effect handlers
- **Typed Actors**: Gleam-style typed actor model
- **Reference Capabilities**: Pony-style data-race freedom
- **Refinement Types**: Liquid Haskell + SMT solver integration
- **Phantom Types**: Typestate pattern for state machines
- **Design Patterns as Types**: SOLID, GoF patterns enforced at compile time
- **Proof-Carrying Code**: Optional correctness proofs

## Project Structure

```
guage/
├── bootstrap/          # Phase 0: C runtime (~1000 LOC)
│   ├── cell.c         # Cell structure + linear tracking
│   ├── gc.c           # Precise GC with linearity
│   ├── actor.c        # Actor scheduler
│   ├── effect.c       # Effect handler runtime
│   ├── primitives.c   # All 42 primitives
│   └── main.c         # REPL
├── core/              # Phase 1: Self-hosted interpreter (~2000 LOC)
├── types/             # Phase 2: Complete type system (~5000 LOC)
├── patterns/          # Phase 3: Design patterns as types (~2000 LOC)
├── proofs/            # Phase 4: Proof-carrying code (~2000 LOC)
├── std/               # Phase 5: Standard library (~5000 LOC)
└── examples/          # Example programs
```

## Primitives (55 Functional + 7 Placeholders)

### Core (Evaluator Built-ins)
- `λ` - Lambda abstraction
- `0 1 2...` - De Bruijn indices (variable references)
- `≔` - Global definition
- `?` - Conditional (if-then-else)

### Lists (3)
- `⟨⟩` - Pair construction (cons)
- `◁` - Head (car)
- `▷` - Tail (cdr)

### Metaprogramming (2)
- `⌜` - Quote (code→data)
- `⌞` - Eval (data→code) - PLACEHOLDER

### Arithmetic (9)
- `⊕` `⊖` `⊗` `⊘` `%` - Add, subtract, multiply, divide, modulo
- `<` `>` `≤` `≥` - Comparisons

### Logic (5)
- `≡` `≢` - Equality, inequality
- `∧` `∨` `¬` - AND, OR, NOT

### Type Predicates (6)
- `ℕ?` `𝔹?` `:?` `∅?` `⟨⟩?` `#?` - Test types

### Debug & Error (4)
- `⚠` - Create error value
- `⚠?` - Test if error
- `⊢` - Assert condition
- `⟲` - Trace (debug print)

### Testing (2)
- `≟` - Deep equality test
- `⊨` - Test case

### Documentation (5)
- `⌂` - Get description
- `⌂∈` - Get type signature
- `⌂≔` - Get dependencies
- `⌂⊛` - Get source code
- `⌂⊨` - Auto-generate tests

### CFG/DFG (2)
- `⌂⟿` - Get control flow graph
- `⌂⇝` - Get data flow graph

### Structures - Leaf (5)
- `⊙≔` `⊙` `⊙→` `⊙←` `⊙?` - Define, create, get, set, check

### Structures - Node/ADT (4)
- `⊚≔` `⊚` `⊚→` `⊚?` - Define, create, get, check

### Structures - Graph (6)
- `⊝≔` `⊝` `⊝⊕` `⊝⊗` `⊝→` `⊝?` - Define, create, add node/edge, query, check

### Placeholders (7)
- `⌞` - Eval (Day 14)
- `⟪⟫` `↯` `⤴` `≫` - Effects (Phase 4+)
- `⟳` `→!` `←?` - Actors (Phase 5+)

**Full specification:** See [SPEC.md](SPEC.md)

## Building

```bash
cd bootstrap
make
./guage
```

## Examples (Working Now!)

### Factorial with Named Recursion
```scheme
; Documentation form (for humans)
(≔ ! (λ (n) (? (≡ n #0) #1 (⊗ n (! (⊖ n #1))))))

; De Bruijn form (what actually runs)
(≔ ! (λ (? (≡ 0 #0) #1 (⊗ 0 (! (⊖ 0 #1))))))

(! #5)  ; → #120
```

### Fibonacci
```scheme
(≔ fib (λ (n) (? (< n #2) n (⊕ (fib (⊖ n #1)) (fib (⊖ n #2))))))
(fib #10)  ; → #55
```

### Structure: Point (Leaf)
```scheme
; Define structure
(⊙≔ :Point :x :y)

; Create instance
(≔ p (⊙ :Point #3 #4))

; Access fields
(⊙→ p :x)  ; → #3
(⊙→ p :y)  ; → #4

; Check type
(⊙? p :Point)  ; → #t
```

### Structure: List (ADT)
```scheme
; Define recursive ADT
(⊚≔ :List (⌜ (:Nil)) (⌜ (:Cons :head :tail)))

; Create list: [42, 13, 7]
(≔ empty (⊚ :List :Nil))
(≔ l (⊚ :List :Cons #42
         (⊚ :List :Cons #13
            (⊚ :List :Cons #7 empty))))

; Access
(⊚→ l :head)  ; → #42

; Check variant
(⊚? l :List :Cons)  ; → #t
(⊚? empty :List :Nil)  ; → #t
```

### Auto-Documentation
```scheme
(≔ double (λ (x) (⊗ x #2)))

; Auto-prints:
; 📝 double :: ℕ → ℕ
;    multiply x and 2
;    Dependencies: ⊗

; Query docs
(⌂ (⌜ double))   ; → Description
(⌂∈ (⌜ double))  ; → Type signature
```

### Auto-Generated Tests
```scheme
; Generate tests from function
(⌂⊨ (⌜ ⊕))
; → ⟨(⊨ :test-normal-case #t (ℕ? (⊕ #5 #3)))
;     (⊨ :test-zero-operand #t (ℕ? (⊕ #0 #5)))⟩

; Tests are first-class values (data, not executed yet)
```

**More examples:** See `tests/*.scm` for 408+ working tests!

## Current Status (Week 2 Day 13)

**Phase 2C: Core Correctness** - 93% COMPLETE
- ✅ Cell structure with reference counting
- ✅ Lambda calculus with De Bruijn indices
- ✅ 55 functional primitives (arithmetic, logic, lists, structures, etc.)
- ✅ Self-testing system (⌂⊨ generates tests from code)
- ✅ Structure primitives (⊙ leaf, ⊚ node/ADT, ⊝ graph)
- ✅ Auto-documentation system
- ✅ 408+ tests passing
- ⏳ Pattern matching (Week 3)
- ⏳ Eval primitive (Day 14)
- ⏳ Effect handlers (Phase 4+)
- ⏳ Actor runtime (Phase 5+)

## Timeline

**Completed:**
- ✅ Phase 1 (Dec 2025): Cell infrastructure
- ✅ Phase 2A (Dec 2025): Lambda calculus + Turing completeness
- ✅ Phase 2B (Jan 2026): Named recursion + auto-documentation
- ✅ Phase 2C (Jan 2026): Week 1-2 - Structure primitives + self-testing (93% complete)

**In Progress:**
- 🔄 Phase 2C Week 3 (Days 15-21): Pattern matching (∇, ≗, _)

**Next Up:**
- Phase 3 (3 weeks): Macros, generics, standard library basics
- Phase 4 (3 months): Self-hosting (parser/compiler in Guage)
- Phase 5 (6 months): Advanced metaprogramming (synthesis, time-travel debugging)
- Phase 6 (6 months): Distribution, native compilation, optimization

**Estimated to MVP:** 6-7 weeks (~225 hours)
**Estimated to production:** ~21 months total

**Progress tracking:** See [SESSION_HANDOFF.md](SESSION_HANDOFF.md)

## Developer Guide

**Starting a new session?**
1. Copy and paste: [`START_SESSION.txt`](START_SESSION.txt)
2. Read: [`SESSION_HANDOFF.md`](SESSION_HANDOFF.md)
3. Follow methodology in: [`SESSION_START_PROMPT.md`](SESSION_START_PROMPT.md)

**Documentation:**
- All docs indexed at: [`docs/INDEX.md`](docs/INDEX.md)
- Governance rules prevent duplication
- Clear naming conventions (no "advanced", "new", "temp")

**Development Workflow:**
1. Feature-by-feature, test-first
2. Update docs as you go
3. Commit after each complete feature
4. Archive completed work immediately

**Testing:**
```bash
./guage < tests/test_[feature].scm  # Single test
./run_tests.sh                       # All tests
```

## Note on Syntax

**Runtime:** Pure symbolic De Bruijn notation (0, 1, 2...)
**Documentation:** Named parameters for humans (𝕩, 𝕪, 𝕫, ƒ, 𝕘)
**Philosophy:** No English keywords - only Unicode symbols

The language uses De Bruijn indices internally for efficiency, but documentation uses mathematical notation for clarity.

## License

MIT
