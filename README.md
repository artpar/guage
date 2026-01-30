---
Status: CURRENT
Created: 2025-12-01
Updated: 2026-01-30
Purpose: Project overview and quick start
---

# Guage: The Ultralanguage

**A Turing-complete ultralanguage with pure symbolic syntax, designed to subsume all other languages through careful primitive design and systematic extension.**

## Current Status

- **156 primitives** — all working
- **91/91 test files passing** (100%)
- **Turing complete** — lambda calculus with De Bruijn indices + TCO
- **Day 93 complete** — supervisor strategies (one-for-one, one-for-all)
- See [`SESSION_HANDOFF.md`](SESSION_HANDOFF.md) for detailed progress

## What's Working

| Category | Primitives | Description |
|----------|-----------|-------------|
| Core Lambda Calculus | `λ`, `≔`, `?`, De Bruijn | Closures, TCO, named recursion |
| Data Types | `⟨⟩`, `◁`, `▷`, `∅`, `#n`, `#t/#f`, `:sym` | Pairs, numbers, booleans, symbols, nil |
| Arithmetic | `⊕ ⊖ ⊗ ⊘ ÷ % < > ≤ ≥` | Full numeric operations |
| Math Library | `√ ^ \| ⌊⌋ ⌈⌉ sin cos tan log exp π e rand` | 22 math primitives |
| Logic | `≡ ≢ ∧ ∨ ¬` | Equality, boolean logic |
| Predicates | `ℕ? 𝔹? :? ∅? ⟨⟩? #?` | Type testing |
| Strings | `≈ ≈⊕ ≈# ≈→ ≈⊂ ≈? ≈∅? ≈≡ ≈<` | 9 string primitives + stdlib |
| I/O | `≋ ≋≈ ≋← ≋⊳ ≋⊲ ≋⊕ ≋? ≋∅?` | Console + file operations |
| Errors | `⚠ ⚠? ⚠⊙ ⚠→ ⊢ ⟲` | First-class errors, assertions, tracing |
| Testing | `≟ ⊨ ⊨-prop gen-int gen-bool gen-symbol gen-list` | Deep equality, property-based testing |
| Structures | `⊙≔ ⊙ ⊙→ ⊙← ⊙?` | Leaf structures (records) |
| ADTs | `⊚≔ ⊚ ⊚→ ⊚?` | Algebraic data types |
| Graphs | `⊝≔ ⊝ ⊝⊕ ⊝⊗ ⊝→ ⊝? ⊝↦ ⊝⊃ ⊝⊚ ⊝⊙ ⊝⇝ ⊝∘` | Graph structures + algorithms |
| Pattern Matching | `∇` | Guards, as-patterns, or-patterns, view patterns |
| Macros | `⧉ ⧉⊜ ⧉→ ⧉?` | Simple + pattern-based with ellipsis |
| Metaprogramming | `⌜ ⌞ ⌞̃ ~ ⊡` | Quote, eval, quasiquote, apply |
| Modules | `⋘ ⌂⊚ ⋖ ⌂⊚→` | Load, provenance, selective import |
| Documentation | `⌂ ⌂∈ ⌂≔ ⌂⊛ ⌂⊨ ⌂⊨! ⌂⊨⊗ 📖 📖→ 📖⊛` | Auto-docs, mutation testing |
| CFG/DFG | `⌂⟿ ⌂⇝` | Control/data flow graphs |
| Type System | `∈ ∈? ∈⊙ ∈≡ ∈⊆ ∈! ∈◁ ∈▷ ∈⊙ₜ ∈✓ ∈✓* ∈⊢ ∈⍜ ∈⍜⊕ ∈⍜*` + constants | Annotations, validation, inference |
| Effects | `⟪ ⟪? ⟪→ ⟪⟫ ⟪↺⟫ ↯ ⤴ ≫` | Algebraic effects, resumable handlers |
| Continuations | `⟪⊸⟫ ⊸` | Delimited continuations (shift/reset) |
| Actors | `⟳ →! ←? ⟳! ⟳? ⟳→ ⟳∅` | Cooperative actor model |
| Supervision | `⟳⊗ ⟳⊘ ⟳⊙ ⟳⊜ ⟳✕` | Links, monitors, exit signals |
| Supervisors | `⟳⊛ ⟳⊛? ⟳⊛!` | Restart strategies (one-for-one, one-for-all) |
| Channels | `⟿⊚ ⟿→ ⟿← ⟿× ⟿∅ ⟿⊞ ⟿⊞?` | Bounded ring buffers with select |
| Stdlib Macros | `∧* ∨* ⇒ ⇏ ⇒* ≔⇊ ⇤ ⚡ ⚡⊳ ⚡∅ ...` | Control, exception, iteration macros |
| Introspection | `⧉ ⊛` | Arity, source code |

## Quick Start

```bash
make              # Build (O2 optimized, 32MB stack)
make test         # Run full test suite (91 test files)
make repl         # Start interactive REPL
make clean        # Clean build artifacts
make rebuild      # Clean + rebuild
```

## Examples

### Factorial with Named Recursion
```scheme
(≔ ! (λ (n) (? (≡ n #0) #1 (⊗ n (! (⊖ n #1))))))
(! #5)  ; → #120
```

### Pattern Matching with Guards
```scheme
(≔ classify (λ (n)
  (∇ n (⌜ (((x | (> x #100)) :large)
            ((x | (> x #0)) :positive)
            (_ :non-positive))))))
(classify #150)  ; → :large
(classify #5)    ; → :positive
```

### Algebraic Data Types
```scheme
(⊚≔ :Option (⌜ (:None)) (⌜ (:Some :value)))
(≔ some-42 (⊚ :Option :Some #42))
(∇ some-42 (⌜ (((⊚ :Option :Some v) v)
               ((⊚ :Option :None) :empty))))  ; → #42
```

### Algebraic Effects (Resumable)
```scheme
(⟪ :State :get :put)
(⟪↺⟫ (⊕ (↯ :State :get) #1)
  (:State
    (:get (λ (k) (k #42)))
    (:put (λ (k v) (k ∅)))))
; → #43
```

### Actor Model with Message Passing
```scheme
(⟳∅)
(≔ echo (⟳ (λ (self) (←?))))
(→! echo :hello)
(⟳! #100)
(⟳→ echo)  ; → :hello
```

### Channels with Select
```scheme
(⟳∅)
(≔ ch1 (⟿⊚))
(≔ ch2 (⟿⊚))
(≔ producer (⟳ (λ (self) (⟿→ ch2 :from-ch2))))
(≔ consumer (⟳ (λ (self) (⟿⊞ ch1 ch2))))
(⟳! #200)
(⟳→ consumer)  ; → ⟨⟿[2] :from-ch2⟩
```

### Supervisor with Restart Strategy
```scheme
(⟳∅)
(≔ worker (λ (self) (←?)))
(≔ sup (⟳⊛ :one-for-one (⟨⟩ worker (⟨⟩ worker ∅))))
(⟳! #100)
(⟳⊛? sup)   ; → list of child actor cells
(⟳⊛! sup)   ; → restart count
```

### Pattern-Based Macros with Ellipsis
```scheme
(⧉⊜ sum
  (()              #0)
  (($x)            $x)
  (($x $rest ...)  (⊕ $x (sum $rest ...))))
(sum #1 #2 #3 #4 #5)  ; → #15
```

## Project Structure

```
guage/
├── Makefile              # Build system (from root)
├── README.md             # This file
├── SPEC.md               # Language specification (156 primitives)
├── CLAUDE.md             # Philosophy and principles
├── SESSION_HANDOFF.md    # Current progress and status
├── bootstrap/            # C implementation
│   ├── cell.{c,h}       # Core data structures + refcounting
│   ├── eval.{c,h}       # Evaluator + special forms
│   ├── debruijn.{c,h}   # De Bruijn index conversion
│   ├── primitives.{c,h} # All 156 primitive operations
│   ├── debug.{c,h}      # Stack traces
│   ├── macro.{c,h}      # Pattern-based macro system
│   ├── pattern.{c,h}    # Pattern matching engine
│   ├── type.{c,h}       # Type annotations + validation + inference
│   ├── cfg.{c,h}        # Control flow graph generation
│   ├── dfg.{c,h}        # Data flow graph generation
│   ├── fiber.{c,h}      # Fiber/coroutine infrastructure
│   ├── actor.{c,h}      # Actor model + supervision + supervisors
│   ├── channel.{c,h}    # Channel ring buffers
│   ├── module.{c,h}     # Module system
│   ├── testgen.{c,h}    # Test generation + mutation testing
│   ├── main.c            # Parser, REPL with history/completion
│   ├── stdlib/           # Standard library (Guage code)
│   ├── tests/            # Test suite (91 test files)
│   └── run_tests.sh      # Test runner
└── docs/                 # Documentation
    ├── INDEX.md           # Navigation hub
    ├── reference/         # Technical deep-dives
    ├── planning/          # Active roadmaps
    └── archive/           # Historical documents
```

## Core Philosophy

- **Pure symbols only** — No English keywords, only Unicode mathematical symbols
- **First-class everything** — Functions, errors, tests, types, CFG/DFG are all values
- **De Bruijn indices** — Variables referenced by index at runtime
- **Errors as values** — `⚠` creates error values, no exceptions
- **Single source of truth** — One canonical way to do things

## Documentation

- **Language specification** → [`SPEC.md`](SPEC.md)
- **Philosophy & principles** → [`CLAUDE.md`](CLAUDE.md)
- **Current status** → [`SESSION_HANDOFF.md`](SESSION_HANDOFF.md)
- **All documentation** → [`docs/INDEX.md`](docs/INDEX.md)
- **Technical reference** → [`docs/reference/`](docs/reference/)

## Timeline

**Completed:**
- Phase 1 (Dec 2025): Cell infrastructure + lambda calculus
- Phase 2A (Dec 2025): Turing completeness + named recursion
- Phase 2B (Jan 2026): Auto-documentation + structures + ADTs + graphs
- Phase 2C (Jan 2026): Pattern matching + macros + modules + type system
- Phase 2D (Jan 2026): Algebraic effects + delimited continuations + fibers
- Phase 2E (Jan 2026): Actors + channels + select + supervision + supervisors

**Next:**
- Phase 3: Dynamic supervisor management, rest-for-one strategy, optimizer
- Phase 4: Self-hosting (parser/compiler in Guage)
- Phase 5: Advanced metaprogramming (synthesis, time-travel debugging)
- Phase 6: Distribution, native compilation

**Progress tracking:** See [SESSION_HANDOFF.md](SESSION_HANDOFF.md)

## License

MIT
