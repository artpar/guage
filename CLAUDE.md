# Guage: The Ultralanguage

## Vision

Guage is a **Turing-complete ultralanguage** designed to be the ultimate programming language - one that subsumes all others through careful design of core primitives and systematic extension.

### Ultimate Goals

1. **Expressive power** - Can express any computable abstraction
2. **Universality** - Can express any computable function
3. **First-class everything** - Debugging, errors, testing, types, effects
4. **Pure symbolic syntax** - No English keywords, only mathematical symbols
5. **Type safety** - Gradual dependent types
6. **Concurrency** - Actor model with message passing
7. **Effects** - Algebraic effect handlers
8. **Performance** - Eventually compile to native code

## Core Principles

### 1. Pure Symbols Only

**No English keywords.** Every construct uses mathematical or symbolic notation:

```scheme
λ   - Lambda (not "lambda" or "fn")
≔   - Define (not "def" or "let")
?   - Conditional (not "if")
⊕   - Add (not "add" or "+")
⚠   - Error (not "error" or "throw")
⊢   - Assert (not "assert")
```

**Why:** Universal. Language-independent. Mathematically precise.

### 2. First-Class Everything (Including Metaprogramming)

**CRITICAL:** Everything is a value - not just data, but **ALL aspects of computation**:

**Already Implemented:**
- **Functions:** λ expressions with closures
- **Errors:** ⚠ values, not exceptions
- **Debugging:** ⟲ trace returns the value
- **Tests:** ⊨ test cases are expressions
- **Structures:** ⊙/⊚ user-defined types

**Being Built NOW (Phase 2C):**
- **CFG/DFG:** Control and data flow graphs as ⊝ graph structures
- **Type Schemas:** Stored in registry, queryable
- **Code Structure:** AST as data you can transform

**Coming Soon:**
- **Execution Traces:** Complete program history as queryable graph
- **Specifications:** Formal specs that generate implementations
- **Optimizations:** Optimization passes as composable functions
- **Proofs:** Type refinements that prove properties
- **APIs:** Interface definitions as comparable values
- **Programs:** Other codebases loadable and analyzable

**Why:** This isn't "nice to have" - it's the **core of Guage**. If something exists in the language, it must be a first-class value you can inspect, transform, and reason about. No special cases. No "compiler magic." Everything is data.

### 3. Everything is Queryable, Provable, Transformable

**This is what makes Guage an "ultralanguage":**

**Queryable:**
- CFG/DFG are graph structures you can search: `(⊝→ cfg :entry)`
- Execution traces are queryable: `(⨳ trace predicate)`
- Types are inspectable: `(⊢ value Type)`
- Code structure is analyzable: `(⊙⋈ program₁ program₂)`

**Provable:**
- Types carry proofs: `(⊡ Sorted [ℤ] (∀ i (≼ (⊇ ⊙ i) (⊇ ⊙ (⊕ i 1)))))`
- Termination provable: `(⊢ φ ↓)`
- Complexity provable: `(⊢ φ (O (⊗ n (ℓ n))))`
- Correctness provable: `(⊢ φ spec)`

**Transformable:**
- Synthesize from specs: `(⊛ spec)`
- Repair broken code: `(⊛ spec ◂ broken_code)`
- Optimize automatically: `(◎ code)`
- Generate docs/tests: `(📖 code)` `(⊙? code)`
- Hot-swap: `(⇝ old_version new_version)`

**Current Infrastructure (Phase 2C):**
- ✅ Graph structures (⊝) for CFG/DFG
- ✅ Type registry for queryable schemas
- ✅ Immutable operations for time-travel
- ✅ Reference counting for serialization

**Why This Architecture:**
Traditional languages treat the compiler as a black box. In Guage:
- The compiler is a library you can call
- CFG/DFG are data structures you can query
- Types are values you can compute with
- Code is data you can transform
- Everything is inspectable and modifiable

This enables **assisted development** where the language helps you write, prove, test, optimize, and deploy code.

### 4. Single Source of Truth

- **No dual paths** - One canonical way to do things
- **No unnecessary transforms** - Direct representation
- **Values as boundaries** - All interfaces use simple values
- **No glue layer complexity** - Direct mapping

**Why:** Simplicity. Maintainability. Understandability.

### 5. Development-First

- **Never lint** - Code is correct by construction
- **Never type check during dev** - Types are gradual
- **Human tests** - Developer validates behavior
- **Hot-reload assumed** - No build step during development
- **Values as API boundaries** - Simple integration

**Why:** Fast iteration. Developer productivity. Flexibility.

### 6. Mathematical Foundation

Based on:
- **Lambda calculus** - Functions as first-class values
- **De Bruijn indices** - Efficient variable representation
- **Type theory** - Dependent types (future)
- **Effect algebras** - Algebraic effects with resumable handlers
- **Actor model** - BEAM-style actors with supervision

**Why:** Solid theoretical foundation. Proven correctness.

## Architecture Principles

### Layered Design

```
┌──────────────────────────────────────┐
│   Surface Language (Pure Symbols)    │  ← User writes this
├──────────────────────────────────────┤
│   Core Language (Lambda Calculus)    │  ← De Bruijn indices
├──────────────────────────────────────┤
│   Runtime (Closures + References)    │  ← Memory management
├──────────────────────────────────────┤
│   Primitives (Built-in Operations)   │  ← ⊕, ⊗, ⟨⟩, etc.
└──────────────────────────────────────┘
```

### Evaluation Strategy

1. **Parse** - Surface syntax → S-expressions
2. **Convert** - Named variables → De Bruijn indices (at lambda creation)
3. **Evaluate** - Beta reduction with closures
4. **Manage** - Reference counting GC

### Error Model

**Errors are values, not exceptions:**

```scheme
; Error creation
(≔ safe-div (λ (x y)
  (? (≡ y #0)
     (⚠ :div-by-zero y)    ; Return error value
     (⊘ x y))))             ; Return result

; Error checking
(⚠? result)                 ; Test if error

; Error handling
(? (⚠? result)
   (handle-error result)
   (use-value result))
```

**Why:** Explicit. Composable. First-class.

## Feature Set

### Current (Day 148+ — 175 tests, 540 primitives)

**Core:**
- λ abstraction with De Bruijn indices
- Function application (beta reduction)
- Lexical scoping with closures
- ≔ global definitions, named recursion
- TCO (tail call optimization via goto)
- ⪢ sequencing

**Data:**
- Numbers (#42), Booleans (#t, #f), Nil (∅)
- Pairs (⟨⟩), Symbols (:name), Strings
- Errors (⚠ — first-class values, not exceptions)
- Mutable refs (□ box/unbox/swap)

**Data Structures:**
- HashMap (⊞), HashSet (⊍), Vector (⟦⟧)
- Deque (⊟), Priority Queue (△), Trie (⊮)
- Sorted Map (⋔), Byte Buffer (◈)

**Control:**
- ? conditional, ⌜ quote
- ∇ pattern matching (guards, as-patterns, or-patterns, view patterns)

**Effects:**
- ⟪ perform, ⟪⟫ handler, ⟪↺⟫ resumable handler
- ↯ raise, ⟪⊸⟫ linear effect, ⊸ consume
- ⤴ resume, ≫ chain

**Actors/Concurrency:**
- ⟳ actors, →!/←? messages, channels
- Supervisors (one-for-one, one-for-all, rest-for-one)
- GenServer, ETS tables, process registry, timers
- Tasks, Agents, GenStage, DynamicSupervisor, Flow
- Multi-scheduler (BWoS deque), work-stealing
- Eventcount parking (Folly/Vyukov), QSBR reclamation

**Types:**
- Annotations (∈), validation (∈✓), inference (∈⍜)

**Macros:**
- Pattern-based macros (⧉⊜), stdlib macros (control, iteration, exception, pattern, unicode)

**Structures:**
- ⊙ leaf, ⊚ node/ADT, ⊝ graphs (CFG/DFG)

**Debug/Test:**
- ⊢ assertions, ⟲ trace, ⊙ type-of
- ⧉ arity, ⊛ source, ≟ deep-equal, ⊨ test-case
- Structured test runner (--test mode, JSON Lines, coverage, leak detection)
- gen-int-shrink, gen-list-shrink (integrated Hedgehog-style shrinking)

**FFI:**
- JIT-compiled stubs (⌁) — ARM64 + x86-64

**Stdlib:**
- Math (√, ^, trig, log, π, e, rand), strings, POSIX
- Lists (extended utilities, sort, comprehensions)
- Iterator protocol, option/result types, networking

**Infrastructure:**
- Multi-scheduler with BWoS deque and work-stealing
- Deterministic actor scheduling (seeded PRNG for reproducible tests)
- Eventcount parking (3-tier: YIELD→WFE→ulock)
- QSBR actor reclamation, global trace aggregation
- String interning (SipHash + SwissTable), fiber runtime

### Planned

**Near-term:**
- Module system improvements
- Stdlib expansion (more data structure operations)

**Mid-term:**
- Dependent types
- Linear types (infrastructure present)
- Proof system

**Long-term:**
- Native compilation (LLVM backend)
- Incremental compilation
- IDE support
- Dependent types and proof system

## Syntax Philosophy

### Symbol Selection

Each symbol chosen for **intuitive meaning**:

```
λ  - Lambda shape suggests function abstraction
≔  - Assignment/definition (colon-equals)
?  - Question suggests conditional
⊕  - Circled plus (pure addition)
⚠  - Warning triangle (error)
⊢  - Turnstile (proves/asserts)
⟲  - Circular arrows (trace/loop)
⊙  - Circled dot (examine/inspect)
∅  - Empty set (nil)
⟨⟩ - Angle brackets (pair/cons)
◁  - Left triangle (head/car)
▷  - Right triangle (tail/cdr)
```

### Consistency Rules

1. **Operators are symbolic** - No word operators
2. **Prefix notation** - (operator args...)
3. **Pure symbols** - No mixing symbols and English
4. **Unicode encouraged** - Use proper mathematical symbols
5. **Self-documenting** - Symbols suggest meaning

## Development Workflow

### Bootstrap Phase (Current)

```
C implementation
├── Parse symbols → S-expressions
├── Convert names → De Bruijn indices
├── Evaluate → Beta reduction + effects + actors
└── Reference counting GC + QSBR
```

**Goal:** Build the language to full expressive power.

### Type System Phase (Future)

```
Advanced type system
├── Refinement types (subsets, constraints)
├── Dependent types (length-indexed, etc.)
├── Proof system (termination, complexity)
└── Effect tracking in types
```

**Goal:** Provably correct programs through the type system.

### Native Compilation Phase (Future)

```
Guage → LLVM → Native
├── Type inference
├── Effect analysis
├── Optimization passes
└── Code generation
```

**Goal:** Production-ready performance.

## Testing Philosophy

### Built-in Testing

Tests are **part of the language**, not external:

```scheme
; Test cases are expressions
(⊨ (⌜ :test-name) expected actual)

; Assertions are expressions
(⊢ condition :error-message)

; Deep equality is primitive
(≟ value1 value2)
```

### Test-Driven Development

1. Write test in Guage
2. Run test (fails)
3. Implement feature
4. Test passes
5. Commit

### No External Test Frameworks

Everything needed is **in the language**:
- ⊨ for test cases
- ⊢ for assertions
- ≟ for equality
- ⟲ for tracing

## Performance Philosophy

### Phase 1: Correctness (Current)

- **Reference counting** - Simple, predictable
- **Interpreter** - Easy to debug
- **O(1) variable lookup** - De Bruijn indices
- **Minimal optimization** - Keep it simple

### Phase 2: Optimization (Future)

- **JIT compilation** - Hot path optimization
- **Escape analysis** - Stack allocation when possible
- **Inline expansion** - Remove call overhead
- **Tail call optimization** - Constant stack space

### Phase 3: Native (Future)

- **Ahead-of-time compilation** - LLVM backend
- **Whole-program optimization** - Cross-module inlining
- **Effect specialization** - Monomorphization
- **SIMD** - Vectorization where applicable

## Extensibility

### Language Extension Points

1. **Primitives** - Add new built-in operations
2. **Special forms** - Extend evaluator
3. **Macros** - Pattern-based syntax transformation (⧉⊜)
4. **Effects** - User-defined algebraic effect handlers (⟪⟫)
5. **Types** - User-defined types with ADTs (⊚)

### Library Design

Standard library organized as:

```
stdlib/
├── core/        - Lists, maps, sets
├── math/        - Advanced math functions
├── io/          - File, network, console
├── concurrency/ - Actors, channels
├── effects/     - Common effect handlers
├── types/       - Type constructors
└── test/        - Testing utilities
```

## Documentation Standards

### Code Comments

Use **symbolic documentation**:

```scheme
; ⌂: Description (house = home/description)
; ∈: Type signature (element-of)
; ⊢: Property/invariant (proves)
; Ex: Example usage

(≔ ! (λ (n)
  "⌂: Factorial function
   ∈: ℕ → ℕ
   ⊢: (! 0) ≡ 1
   ⊢: (! n) ≡ (⊗ n (! (⊖ n 1)))
   Ex: (! 5) → #120"
  (? (≡ n #0) #1 (⊗ n ((! (⊖ n 1)))))))
```

### API Documentation

- **Symbols first** - Show the symbol
- **Type signature** - Using ∈ notation
- **Properties** - Using ⊢ notation
- **Examples** - Real code that works

## Community Principles

### Open Source

- **MIT/Apache dual license** - Maximum freedom
- **Public development** - GitHub from day one
- **Contributor-friendly** - Clear guidelines
- **Inclusive** - Welcoming to all

### Language Evolution

- **RFC process** - For major changes
- **Backwards compatibility** - Don't break existing code
- **Versioning** - Semantic versioning
- **Stability** - Stable vs experimental features

## Implementation Notes

### Current Implementation

**Language:** C11
**GC:** Reference counting + QSBR for actors
**Representation:** De Bruijn indices
**Environment:** Hybrid (named at top, indexed in lambdas)
**Primitives:** 511
**Tests:** 175/175 passing ✅
**Status:** Turing complete + effects + actors + multi-scheduler + pattern matching

### Code Organization

```
/
├── Makefile          - Build system (root level)
├── .gitignore        - Git ignore patterns (root level)
├── README.md         - Project overview
├── SPEC.md           - Language specification
├── CLAUDE.md         - This file
├── SESSION_HANDOFF.md - Current status
├── docs/             - Documentation
└── bootstrap/        - C implementation
    ├── cell.{c,h}        - Core data structures (cells, pairs, refs)
    ├── eval.{c,h}        - Evaluator (TCO, effects, actors)
    ├── debruijn.{c,h}    - De Bruijn conversion
    ├── debug.{c,h}       - Stack traces
    ├── primitives.{c,h}  - Built-in operations (511 primitives)
    ├── main.c            - Parser and REPL
    ├── actor.{c,h}       - BEAM-style actors, supervisors, GenServer
    ├── fiber.{c,h}       - Fiber/coroutine runtime
    ├── channel.{c,h}     - Channel-based communication
    ├── scheduler.{c,h}   - Multi-scheduler, BWoS deque, work-stealing
    ├── eventcount.h      - Folly-derived eventcount (parking)
    ├── park.{c,h}        - Tiered parking (YIELD→WFE→ulock)
    ├── pattern.{c,h}     - Pattern matching engine
    ├── pattern_check.{c,h} - Pattern exhaustiveness checking
    ├── macro.{c,h}       - Pattern-based macro system
    ├── type.{c,h}        - Type annotations and validation
    ├── intern.{c,h}      - String interning (SipHash)
    ├── module.{c,h}      - Module loading (⋘)
    ├── ffi_jit.{c,h}     - JIT FFI stubs
    ├── ffi_emit_a64.c    - ARM64 JIT emitter
    ├── ffi_emit_x64.c    - x86-64 JIT emitter
    ├── cfg.{c,h}         - Control flow graphs
    ├── dfg.{c,h}         - Data flow graphs
    ├── span.{c,h}        - Source spans
    ├── diagnostic.{c,h}  - Diagnostic messages
    ├── ring.{c,h}        - Ring buffer
    ├── testgen.{c,h}     - Test generation
    ├── siphash.h         - SipHash for interning
    ├── swisstable.h      - SwissTable hash map
    ├── fcontext.h        - Fiber context switching
    ├── iter_batch.h      - Batch iterator
    ├── stdlib/           - Standard library (30 Guage files)
    └── tests/            - Test suite (175 test files)
```

### Build and Test

```bash
make                    # Build (from project root)
make test               # Run full test suite (175 test files)
make repl               # Start REPL
make help               # Show all available targets
make run FILE=file.scm  # Run a specific file
make clean              # Clean build artifacts
make rebuild            # Clean and rebuild from scratch
```

## Success Metrics

### Short-term (Bootstrap)

- ✅ Turing complete
- ✅ First-class errors/debug/test
- ✅ Named recursion
- ✅ Standard library basics
- ✅ Module system (⋘ load, ⌂⊚ info)
- ✅ Pattern matching (guards, as-patterns, or-patterns, view patterns)
- ✅ Effect system (algebraic effects with resumable handlers)
- ✅ Actor runtime (BEAM-style with supervisors, GenServer, ETS)
- ✅ Macro system (pattern-based)
- ✅ Type annotations and validation
- ✅ FFI (JIT-compiled stubs, ARM64 + x86-64)
- ✅ Multi-scheduler with work-stealing

### Mid-term (Type System)

- ⏳ Refinement types
- ⏳ Dependent types
- ⏳ Proof system
- ⏳ Effect tracking in types

### Long-term (Production)

- ⏳ Native compilation
- ⏳ Dependent types
- ⏳ Package manager
- ⏳ Real-world usage

---

## Metaprogramming Vision: Native First-Class Features

**See `METAPROGRAMMING_VISION.md` for complete specification.**

### What Makes Guage an "Ultralanguage"

Unlike traditional languages where metaprogramming is an afterthought, **Guage is designed from the ground up** to make all aspects of computation queryable, provable, and transformable:

**Program Synthesis & Repair:**
```scheme
; Synthesize sort from specification
(≔ spec (⌜⟨
  (∀ xs (≡ (# (φ xs)) (# xs)))           ; same length
  (∀ xs (∀ i (≼ (⊇ (φ xs) i) ...)))     ; ordered
⟩⌝))
(≔ sort (⊛ spec))                         ; ⊛ = synthesize

; Repair broken implementation
(≔ broken (λ (xs) (⊳ xs ⌽)))            ; just reverses
(≔ fixed (⊛ spec ◂ broken))              ; ◂ = repair
```

**Time-Travel Debugging:**
```scheme
(≔ τ (⊙⊳ (φ x)))                         ; traced execution
(⊇ τ (⌜ ⟨t∷42⟩⌝))                        ; state at step 42
(⊇ τ (⌜ ⟨←∷z⟩⌝))                         ; what caused z?
(≔ τ′ (⊆ τ (⌜ ⟨t∷10 x∷999⟩⌝)))          ; counterfactual
```

**Types That Prove:**
```scheme
(⊡ Sorted (⊢ [ℤ] (∀ i (≼ (⊇ ⊙ i) (⊇ ⊙ (⊕ i 1))))))
(≔ merge ∷ (→ Sorted (→ Sorted Sorted))) ; proven at compile time
(⊢ quicksort (O (⊗ n (ℓ n))))            ; complexity proven
```

**Cross-Program Analysis:**
```scheme
(≔ π₁ (⋘ (⌜ :service1.η)))               ; load program as value
(≔ π₂ (⋘ (⌜ :service2.η)))
(⊙⋈ π₁ π₂)                                ; joint CFG/DFG
(⊢ (⊙⋈) (¬ deadlock))                    ; prove no deadlock
```

### Why Current Work (Phase 2C) Matters

**We're building the foundation NOW:**

1. **Graph Structures (⊝)** - CFG/DFG as first-class values
2. **Type Registry** - Foundation for dependent types
3. **Immutable Operations** - Enables time-travel debugging
4. **Reference Counting** - Required for serialization/migration

These aren't "useful later" - they're **essential for the language to work as designed**.

### Implementation Timeline

Not "maybe someday" - here's the concrete plan:

- **Phase 2C** (CURRENT): Data structures - 3 weeks
- **Phase 3**: Pattern matching, macros, generics - 18 weeks
- **Phase 4**: Type system and proofs - 12 weeks
- **Phase 5**: Synthesis, optimization, time-travel - 36 weeks
- **Phase 6**: Distribution, cross-program analysis - 24 weeks

**Total:** ~21 months to full vision

### What This Enables

**Assisted Development:**
- Compiler synthesizes code from natural language specs
- Automatic bug repair with minimal edits
- Tests generated from types
- Documentation extracted from structure
- Code optimizes itself based on profiling

**Provably Correct Software:**
- Types prove properties at compile time
- Termination guaranteed
- Complexity bounds verified
- No runtime errors for proven properties

**Living, Evolving Systems:**
- Hot code swapping without downtime
- Automatic migration between API versions
- Continuous optimization based on usage
- Programs that analyze and improve themselves

This isn't science fiction - it's **architectural requirements** being built into the language from day one.

---

## Contribution Guidelines

When contributing to Guage:

1. **Follow symbol-only syntax** - No English keywords
2. **Maintain first-class principles** - Everything is a value
3. **Write tests** - Use ⊨ and ⊢
4. **Document with symbols** - Use ⌂, ∈, ⊢, Ex
5. **Keep it simple** - Single path, no complexity
6. **Reference count carefully** - No leaks

## Documentation Structure

**We maintain an organized documentation system to prevent duplication and staleness.**

### Living Documents (Root Directory)

These 4 documents are **always current** and never archived:

- **README.md** - Project overview (update at milestones)
- **SPEC.md** - Language specification (update when primitives/syntax change)
- **CLAUDE.md** (this file) - Philosophy and principles (rarely change)
- **SESSION_HANDOFF.md** - Current progress and status (update every session)

### Organized Documentation (docs/ Directory)

All other documentation lives under `docs/` with clear organization:

```
docs/
├── INDEX.md              # Navigation hub + documentation governance rules
├── reference/            # Stable, deep-dive technical docs
├── planning/             # Active roadmaps and TODOs
└── archive/              # Historical documents organized by date
    ├── YYYY-MM/
    │   ├── audits/
    │   ├── plans/
    │   └── sessions/
    └── phases/
```

### Documentation Patterns (Prevent Duplication)

**Rule: Single Source of Truth**
- Each type of information has ONE canonical location
- Never copy information between documents - link instead
- Update in one place only

**Rule: Where New Documents Go**
1. **Temporary (1-7 days)** → Archive after session ends
2. **Active planning (1-4 weeks)** → `docs/planning/`, archive when complete
3. **Reference (months/years)** → `docs/reference/`, rarely updated
4. **Current status** → Update `SESSION_HANDOFF.md` only

**Rule: When to Archive**
- After session ends → Move session notes to `archive/YYYY-MM/sessions/`
- After plan completed → Move to `archive/YYYY-MM/plans/`
- After audit done → Move to `archive/YYYY-MM/audits/`
- After phase complete → Move to `archive/phases/`

**Full documentation governance:** See [docs/INDEX.md](docs/INDEX.md)

### Quick Navigation

**Start here:**
- New to Guage? → [README.md](README.md)
- Current status? → [SESSION_HANDOFF.md](SESSION_HANDOFF.md)
- Language spec? → [SPEC.md](SPEC.md)
- Find anything? → [docs/INDEX.md](docs/INDEX.md)

**Deep dives:**
- Long-term vision → [docs/reference/METAPROGRAMMING_VISION.md](docs/reference/METAPROGRAMMING_VISION.md)
- Data structures → [docs/reference/DATA_STRUCTURES.md](docs/reference/DATA_STRUCTURES.md)
- Symbol meanings → [docs/reference/SYMBOLIC_VOCABULARY.md](docs/reference/SYMBOLIC_VOCABULARY.md)
- Design decisions → [docs/reference/TECHNICAL_DECISIONS.md](docs/reference/TECHNICAL_DECISIONS.md)
- Self-evaluating symbols → [docs/reference/KEYWORDS.md](docs/reference/KEYWORDS.md)

**Active work:**
- What's next? → [docs/planning/WEEK_3_ROADMAP.md](docs/planning/WEEK_3_ROADMAP.md)
- Task tracking? → [docs/planning/TODO.md](docs/planning/TODO.md)

**Historical:**
- Past sessions/plans → [docs/archive/2026-01/](docs/archive/2026-01/)
- Phase completions → [docs/archive/phases/](docs/archive/phases/)

---

**Guage: Where symbols speak louder than words** 🚀
