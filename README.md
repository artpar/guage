---
Status: CURRENT
Created: 2025-12-01
Updated: 2026-01-31
Purpose: Project overview and quick start
---

# Guage: The Ultralanguage

**A Turing-complete ultralanguage with pure symbolic syntax, designed to subsume all other languages through careful primitive design and systematic extension.**

## Current Status

- **509 primitives** — all working
- **127/127 test files passing** (100%)
- **Turing complete** — lambda calculus with De Bruijn indices + TCO
- **Day 137 complete** — multi-scheduler activation with HFT-grade concurrency
- **~40K lines of C** across 61 source files
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
| Strings | `≈ ≈⊕ ≈# ≈→ ≈⊂ ≈? ≈∅? ≈≡ ≈< ...` | 33 string primitives (SIMD search) |
| Chars | `♯→ ♯← ♯? ♯≡ ♯< ♯⊕ ♯⊖ ♯ℕ? ♯⊛? ♯⊔?` | 10 character primitives |
| Buffers | `◈⊚ ◈→ ◈← ◈# ◈⊂ ◈≡ ◈⊕ ◈→≈` | Binary buffer operations |
| I/O | `≋ ≋≈ ≋← ≋⊳ ≋⊲ ≋⊕ ≋? ≋∅?` | Console + file operations |
| Errors | `⚠ ⚠? ⚠⊙ ⚠→ ⊢ ⟲` | First-class errors, assertions, tracing |
| Testing | `≟ ⊨ ⊨-prop gen-int gen-bool gen-symbol gen-list` | Deep equality, property-based testing |
| Structures | `⊙≔ ⊙ ⊙→ ⊙← ⊙?` | Leaf structures (records) |
| ADTs | `⊚≔ ⊚ ⊚→ ⊚?` | Algebraic data types |
| Graphs | `⊝≔ ⊝ ⊝⊕ ⊝⊗ ⊝→ ⊝? ⊝↦ ⊝⊃ ⊝⊚ ⊝⊙ ⊝⇝ ⊝∘` | Graph structures + BFS/DFS/Dijkstra/topo-sort |
| Pattern Matching | `∇` | Guards, as-patterns, or-patterns, view patterns, rest patterns |
| Macros | `⧉ ⧉⊜ ⧉→ ⧉?` | Simple + pattern-based with ellipsis |
| Metaprogramming | `⌜ ⌞ ⌞̃ ~ ⊡` | Quote, eval, quasiquote, apply |
| Modules | `⋘ ⌂⊚ ⋖ ⌂⊚→` | Load, provenance, selective import |
| Documentation | `⌂ ⌂∈ ⌂≔ ⌂⊛ ⌂⊨ ⌂⊨! ⌂⊨⊗ 📖 📖→ 📖⊛` | Auto-docs, mutation testing |
| CFG/DFG | `⌂⟿ ⌂⇝` | Control/data flow graphs |
| Type System | `∈ ∈? ∈⊙ ∈≡ ∈⊆ ∈! ∈◁ ∈▷ ∈✓ ∈✓* ∈⊢ ∈⍜ ...` | Annotations, validation, inference, refinement types |
| Effects | `⟪ ⟪? ⟪→ ⟪⟫ ⟪↺⟫ ↯ ⤴ ≫` | Algebraic effects, resumable handlers |
| Continuations | `⟪⊸⟫ ⊸` | Delimited continuations (shift/reset) |
| Actors | `⟳ →! ←? ⟳! ⟳? ⟳→ ⟳∅` | N:M actor model with work-stealing schedulers |
| Supervision | `⟳⊗ ⟳⊘ ⟳⊙ ⟳⊜ ⟳✕` | Links, monitors, exit signals |
| Supervisors | `⟳⊛ ⟳⊛? ⟳⊛!` | Restart strategies (one-for-one, one-for-all, rest-for-one) |
| Dynamic Supervisors | `⟳⊛⊕ ⟳⊛⊖ ⟳⊛#` | Add/remove children at runtime |
| Channels | `⟿⊚ ⟿→ ⟿← ⟿× ⟿∅ ⟿⊞ ⟿⊞?` | Vyukov MPMC ring buffers with select |
| GenServer | `⟳⇅ ⟳⇅!` | Synchronous call-reply pattern |
| Process Dict | `⟳⊔⊕ ⟳⊔? ⟳⊔⊖ ⟳⊔*` | Per-actor key-value store |
| Tasks | `⟳⊳ ⟳⊲ ⟳⊲?` | Async/await over actors |
| Timers | `⟳⏱ ⟳⏱× ⟳⏱?` | Scheduled message delivery |
| Registry | Named process registry | Erlang-style `register`/`whereis` |
| ETS | `⟳⊞ ⟳⊞⊕ ⟳⊞? ⟳⊞⊖ ⟳⊞× ⟳⊞# ⟳⊞*` | Shared named tables |
| Agents | Agent state containers | Functional state wrappers |
| GenStage | Producer-consumer pipelines | Backpressure-aware data flow |
| Flows | Lazy computation pipelines | Map/filter/reduce/each |
| Applications | `⟳⊛⊳ ⟳⊛⊲ ...` | OTP-style application containers |
| Scheduler Config | `⟳# ⟳#?` | Multi-scheduler control + stats |
| Execution Tracing | `⟳⊳⊳! ⟳⊳⊳? ⟳⊳⊳∅ ⟳⊳⊳# ⟳⊳⊳⊛ ⟳⊳⊳⊗ ⟳⊳⊳⊞` | Flight recorder, causal tracing |
| Networking | `⊞⊚ ⊞← ⊞→ ⊞× ⊞⊳ ⊞?` | TCP/UDP sockets (non-blocking) |
| FFI | `⌁⊳ ⌁→ ⌁! ⌁? ⌁⊙ ⌁∅ ⌁∅? ⌁# ⌁× ⌁⊞ ⌁≈→ ⌁→≈ ⌁◈→ ⌁→◈` | JIT-compiled stubs (ARM64 + x86-64) |
| Mutable Refs | `□ □→ □← □?` | Boxes (mutable references) |
| Data Structures | Vectors, sets, sorted maps, heaps, tries | Persistent/mutable collections |
| Iterators | Lazy iteration protocol | Range, map, filter, take, zip |
| Stdlib Macros | `∧* ∨* ⇒ ⇏ ⇒* ≔⇊ ⇤ ⚡ ⚡⊳ ⚡∅ ...` | Control, exception, iteration macros |
| Introspection | `⧉ ⊛` | Arity, source code |

## Quick Start

```bash
make              # Build (O2 optimized, 32MB stack)
make test         # Run full test suite (127 test files)
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

### FFI — Call C Functions
```scheme
(≔ libm (⌁⊳ "libm"))
(≔ sin (⌁→ libm "sin" (⟨⟩ :double ∅) :double))
(sin #1.5707963267948966)  ; → #1 (sin of pi/2)
(≔ pow (⌁→ libm "pow" (⟨⟩ :double (⟨⟩ :double ∅)) :double))
(pow #2 #10)  ; → #1024
```

### Pattern-Based Macros with Ellipsis
```scheme
(⧉⊜ sum
  (()              #0)
  (($x)            $x)
  (($x $rest ...)  (⊕ $x (sum $rest ...))))
(sum #1 #2 #3 #4 #5)  ; → #15
```

## Architecture

### Concurrency Model

N:M multi-scheduler built on assembly `fcontext` context switching (~4-20ns on Apple Silicon, vs ~600ns with `ucontext`):

- **BEAM-style reduction counting** — 4000 reductions per actor quantum, cooperative preemption
- **Chase-Lev work-stealing deque** + LIFO slot (Tokio/Go `runnext` pattern) + steal-half policy
- **Vyukov MPMC** global overflow queue (1024 capacity)
- **Platform-adaptive parking** — `__ulock` on macOS, `futex` on Linux (4 bytes vs 120B pthread_cond)
- **Stack pooling** — `mmap` + guard page + pre-fault, per-scheduler free-list
- **Biased reference counting** — owner thread non-atomic (~1 cycle), cross-thread `fetch_add` (~6 cycles)
- **HFT-grade execution tracing** — per-thread 64KB ring buffer, `rdtscp`/ISB timestamps, causal token propagation

### Memory Model

- **Biased reference counting** with thread-local fast path and atomic shared path
- **Immortal cells** for pre-allocated atoms (nil, true, false, error templates)
- **Stack pool** per scheduler with `mmap` + guard page
- **Epoch-based reclamation** planned for lock-free data structures

## Project Structure

```
guage/
├── Makefile              # Build system (from root)
├── README.md             # This file
├── SPEC.md               # Language specification (509 primitives)
├── CLAUDE.md             # Philosophy and principles
├── SESSION_HANDOFF.md    # Current progress and status
├── bootstrap/            # C implementation (~40K lines)
│   ├── cell.{c,h}       # Core data structures + biased refcounting
│   ├── eval.{c,h}       # Evaluator + special forms + reduction counting
│   ├── debruijn.{c,h}   # De Bruijn index conversion
│   ├── primitives.{c,h} # All 509 primitive operations
│   ├── debug.{c,h}      # Stack traces
│   ├── macro.{c,h}      # Pattern-based macro system
│   ├── pattern.{c,h}    # Pattern matching engine
│   ├── type.{c,h}       # Type annotations + validation + inference
│   ├── cfg.{c,h}        # Control flow graph generation
│   ├── dfg.{c,h}        # Data flow graph generation
│   ├── fiber.{c,h}      # Fiber infrastructure (fcontext-based)
│   ├── actor.{c,h}      # Actor model + supervision + OTP patterns
│   ├── channel.{c,h}    # Vyukov MPMC channel ring buffers
│   ├── scheduler.{c,h}  # N:M work-stealing scheduler
│   ├── fcontext.h        # Portable context switch API
│   ├── fcontext_arm64.S  # ARM64 asm context switch (192B, FPCR saved)
│   ├── fcontext_x86_64.S # x86-64 asm context switch (64B, MXCSR saved)
│   ├── park.{c,h}       # Platform-adaptive thread parking
│   ├── ffi_jit.{c,h}    # FFI JIT stub compiler
│   ├── ffi_emit_a64.c   # ARM64 JIT code emitter
│   ├── ffi_emit_x64.c   # x86-64 JIT code emitter
│   ├── ring.{c,h}       # Lock-free ring buffers
│   ├── module.{c,h}     # Module system
│   ├── testgen.{c,h}    # Test generation + mutation testing
│   ├── diagnostic.{c,h} # Error diagnostics
│   ├── main.c            # Parser, REPL with history/completion
│   ├── stdlib/           # Standard library (Guage code)
│   ├── tests/            # Test suite (127 test files)
│   └── run_tests.sh      # Test runner
└── docs/                 # Documentation
    ├── INDEX.md           # Navigation hub
    ├── reference/         # Technical deep-dives
    ├── planning/          # Active roadmaps
    └── archive/           # Historical documents
```

## Core Philosophy

- **Pure symbols only** — No English keywords, only Unicode mathematical symbols
- **First-class everything** — Functions, errors, tests, types, CFG/DFG, traces are all values
- **De Bruijn indices** — Variables referenced by index at runtime
- **Errors as values** — `⚠` creates error values, no exceptions
- **Single source of truth** — One canonical way to do things
- **Zero-compromise concurrency** — Assembly context switch, work-stealing, biased RC

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
- Phase 2F (Jan 2026): OTP patterns (GenServer, ETS, registry, timers, tasks, agents, applications)
- Phase 2G (Jan 2026): Data structures (vectors, sets, sorted maps, heaps, tries, iterators)
- Phase 2H (Jan 2026): String SDK (SIMD search), test runner, error diagnostics, FFI with JIT
- Phase 2I (Jan 2026): Networking, refinement types, biased RC, N:M scheduler, execution tracing

**Next:**
- Phase 3: Multi-scheduler stress tests, BWoS deque, optimizer
- Phase 4: Type system (refinement types, dependent types, proofs)
- Phase 5: Advanced metaprogramming (synthesis, time-travel debugging)
- Phase 6: Distribution, native compilation

**Progress tracking:** See [SESSION_HANDOFF.md](SESSION_HANDOFF.md)

## License

MIT
