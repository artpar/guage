# Guage: The Ultralanguage

## AI Quick Reference — READ THIS FIRST

**This section teaches you to write correct Guage code.** Guage uses Scheme-like English syntax.

### Literal Syntax

| Syntax | Type | Examples |
|--------|------|---------|
| `#42` `#-3` `#3.14` | Number | `#0`, `#100`, `#0.5` |
| `#t` `#f` | Boolean | |
| `nil` | Nil (empty list) | |
| `:name` | Symbol/keyword | `:error`, `:ok`, `:div-by-zero` |
| `"hello"` | String | `"config"`, `""` |
| `#42i` `#xFFi` | Native integer | |

### Core Forms and Primitives

**Definition & Functions:**
| Name | Meaning | Example |
|------|---------|---------|
| `define` | Define binding | `(define x #42)` |
| `lambda` | Lambda expression | `(lambda (x) (+ x #1))` |
| `if` | Conditional — exactly 3 args | `(if cond then else)` |

**Arithmetic (all prefix, all take 2 args):**
| Name | Meaning | Example |
|------|---------|---------|
| `+` | Add | `(+ #1 #2)` |
| `-` | Subtract | `(- #5 #3)` |
| `*` | Multiply | `(* #2 #3)` |
| `/` | Divide | `(/ #6 #2)` |
| `%` | Modulo | `(% #7 #3)` |
| `^` | Exponent | `(^ #2 #10)` |

**Comparison:**
| Name | Meaning | Example |
|------|---------|---------|
| `equal?` | Equals | `(equal? x y)` |
| `not-equal?` | Not equals | `(not-equal? x y)` |
| `<` `>` `<=` `>=` | Ordering | `(< #1 #2)` |
| `and` | AND (short-circuit) | `(and #t #f)` |
| `or` | OR (short-circuit) | `(or #t #f)` |
| `not` | Logical NOT | `(not #f)` |

**Pairs & Lists:**
| Name | Meaning | Example |
|------|---------|---------|
| `cons` | Cons pair — **STRICTLY 2 args** | `(cons #1 nil)` |
| `car` | Head | `(car pair)` |
| `cdr` | Tail | `(cdr pair)` |
| `null?` | Is nil? | `(null? lst)` |
| `pair?` | Is pair? | `(pair? lst)` |

**Errors (values, NOT exceptions):**
| Name | Meaning | Example |
|------|---------|---------|
| `error` | Create error | `(error :div-by-zero #0)` |
| `error?` | Is error? | `(error? val)` |
| `error-type` | Get error type | `(error-type err)` |
| `error-data` | Get error data | `(error-data err)` |
| `try` | Try (propagate error) | `(try expr)` |

**Control & Sequencing:**
| Name | Meaning | Example |
|------|---------|---------|
| `begin` | Sequence | `(begin expr1 expr2 ... last)` |
| `bind` | Bind/chain | `(bind val (lambda (x) body))` |
| `quote` | Quote | `(quote :name)` |

**Effects:**
| Name | Meaning |
|------|---------|
| `effect-def` | Declare effect |
| `perform` | Perform effect |
| `handle` | Handle effect |
| `handle-resume` | Resumable handler |

**Pattern Matching:**
| Name | Meaning | Example |
|------|---------|---------|
| `match` | Match expression | `(match val clauses)` |

**Mutable State:**
| Name | Meaning | Example |
|------|---------|---------|
| `box` | Create box | `(box #0)` |
| `unbox` | Read box | `(unbox b)` |
| `box-set!` | Write box | `(box-set! b #99)` |

**Structures & Types:**
| Name | Meaning |
|------|---------|
| `type-of` | Get type of value |
| `adt` | Construct ADT |
| `adt-define` | Define ADT |
| `struct-define` | Define struct |
| `struct-create` | Create struct instance |
| `struct-get` | Get struct field |

**Testing:**
| Name | Meaning | Example |
|------|---------|---------|
| `test-case` | Test case | `(test-case (quote :name) expected actual)` |
| `assert` | Assert | `(assert condition :msg)` |
| `deep-equal?` | Deep equal | `(deep-equal? val1 val2)` |

**Macros:**
| Name | Meaning | Example |
|------|---------|---------|
| `macro` | Define macro | `(macro name (args) body)` |
| `macro-rules` | Pattern-based macro | `(macro-rules name clauses)` |

### Worked Examples

```scheme
; 1. Factorial
(define factorial (lambda (n)
  (if (equal? n #0)
     #1
     (* n (factorial (- n #1))))))

; 2. Map over a list
(define my-map (lambda (f lst)
  (if (null? lst)
     nil
     (cons (f (car lst)) (my-map f (cdr lst))))))

; 3. Filter a list
(define my-filter (lambda (pred lst)
  (if (null? lst)
     nil
     (if (pred (car lst))
        (cons (car lst) (my-filter pred (cdr lst)))
        (my-filter pred (cdr lst))))))

; 4. Fold-left
(define my-fold (lambda (f acc lst)
  (if (null? lst)
     acc
     (my-fold f (f acc (car lst)) (cdr lst)))))

; 5. Build a list (3 elements) — note nested cons
(define my-list (cons #1 (cons #2 (cons #3 nil))))

; 6. Safe division with error handling
(define safe-div (lambda (x y)
  (if (equal? y #0)
     (error :div-by-zero y)
     (/ x y))))

; 7. Pattern matching
(match (cons #1 #2) (quote (
  (((cons h t)) (+ h t))
  (_ #0))))

; 8. Effect handling
(handle (perform :State :get)
  (:State
    (:get (lambda () #42))
    (:put (lambda (v) nil))))

; 9. ADT construction and matching
(adt-define :Maybe (quote (:Just :value)) (quote (:Nothing)))
(define val (adt :Maybe :Just #42))
(match val (quote (
  ((adt :Maybe :Just v) v)
  ((adt :Maybe :Nothing) #0))))

; 10. Sequencing with mutable state
(define counter (box #0))
(begin
  (box-set! counter #1)
  (box-set! counter (+ (unbox counter) #1))
  (unbox counter))  ; → #2

; 11. Test case
(test-case (quote :add-basic) #5 (+ #2 #3))

; 12. Accumulator with tail recursion
(define sum-to (lambda (n acc)
  (if (equal? n #0)
     acc
     (sum-to (- n #1) (+ acc n)))))
```

### Common Mistakes — DON'T / DO

| DON'T | DO | Why |
|-------|-----|-----|
| `(+ 1 2)` | `(+ #1 #2)` | Numbers need `#` prefix |
| `(define x 42)` | `(define x #42)` | Numbers need `#` prefix |
| `(list 1 2 3)` | `(cons #1 (cons #2 (cons #3 nil)))` | Build lists with nested `cons` |
| `(cons a b c)` | `(cons a (cons b (cons c nil)))` | `cons` takes exactly 2 args |
| `(a . b)` | `(cons a b)` | No dotted-pair syntax |
| `(lambda (x) e1 e2)` | `(lambda (x) (begin e1 e2))` | Multi-body needs `begin` |
| `(if c t)` | `(if c t nil)` | `if` requires exactly 3 args |
| `(error "msg")` | `(error :tag data)` | Errors need symbol tag + data |
| `(let ((x 1)) x)` | `(begin (define x #1) x)` | No `let` — use `begin` + `define` |
| `'symbol` | `(quote :symbol)` or `:symbol` | Symbols self-evaluate with `:` prefix |
| `(assert x)` | `(assert x :msg)` | `assert` needs condition AND message |
| `()` | `nil` | Empty list is `nil` |

### Data Structures

| Type | Create | Access | Example |
|------|--------|--------|---------|
| Pair/List | `cons` | `car`, `cdr` | `(cons #1 (cons #2 nil))` |
| Vector | `vector-create` | `vector-get` | `(vector-create #3 #0)` |
| HashMap | `hashmap-create` | `hashmap-get` | `(hashmap-set (hashmap-create) :key val)` |
| HashSet | `set-create` | `set-contains?` | `(set-add (set-create) #1)` |
| Box | `box` | `unbox` | `(box #42)` |
| Deque | `deque-create` | `deque-front` | `(deque-push-back (deque-create) #1)` |

### Type Predicates

| Name | Tests for |
|------|-----------|
| `number?` | Number |
| `boolean?` | Boolean |
| `string?` | String |
| `symbol?` | Symbol |
| `pair?` | Pair/cons cell |
| `null?` | Nil |
| `error?` | Error value |
| `procedure?` | Function |

---

## Vision

Guage is a **Turing-complete ultralanguage** designed to be the ultimate programming language - one that subsumes all others through careful design of core primitives and systematic extension.

### Ultimate Goals

1. **Expressive power** - Can express any computable abstraction
2. **Universality** - Can express any computable function
3. **First-class everything** - Debugging, errors, testing, types, effects
4. **Scheme-like syntax** - Familiar Lisp/Scheme naming with `#` number literals
5. **Type safety** - Gradual dependent types
6. **Concurrency** - Actor model with message passing
7. **Effects** - Algebraic effect handlers
8. **Performance** - Eventually compile to native code

## Core Principles

### 1. Scheme-Like Syntax

Guage uses familiar English/Scheme names for readability:

```scheme
lambda  - Lambda expressions
define  - Global definitions
if      - Conditional (requires 3 args)
+       - Addition
error   - Create error values
assert  - Assertions
```

**Number literals use `#` prefix:** `#42`, `#3.14`, `#-5`

**Why:** Familiar to Lisp/Scheme programmers. Easy for AI to generate correct code.

### 2. First-Class Everything (Including Metaprogramming)

**CRITICAL:** Everything is a value - not just data, but **ALL aspects of computation**:

**Already Implemented:**
- **Functions:** `lambda` expressions with closures
- **Errors:** `error` values, not exceptions
- **Debugging:** `trace` returns the value
- **Tests:** `test-case` are expressions
- **Structures:** `struct-define`/`adt-define` user-defined types

**Being Built NOW (Phase 2C):**
- **CFG/DFG:** Control and data flow graphs as graph structures
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
- CFG/DFG are graph structures you can search: `(graph-query cfg :entry)`
- Execution traces are queryable
- Types are inspectable: `(assert value Type)`
- Code structure is analyzable

**Provable:**
- Types carry proofs
- Termination provable
- Complexity provable
- Correctness provable

**Transformable:**
- Synthesize from specs
- Repair broken code
- Optimize automatically
- Generate docs/tests
- Hot-swap code

**Current Infrastructure (Phase 2C):**
- ✅ Graph structures for CFG/DFG
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
│   Surface Language (Scheme-like)     │  ← User writes this
├──────────────────────────────────────┤
│   Core Language (Lambda Calculus)    │  ← De Bruijn indices
├──────────────────────────────────────┤
│   Runtime (Closures + References)    │  ← Memory management
├──────────────────────────────────────┤
│   Primitives (Built-in Operations)   │  ← +, *, cons, etc.
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
(define safe-div (lambda (x y)
  (if (equal? y #0)
     (error :div-by-zero y)    ; Return error value
     (/ x y))))                 ; Return result

; Error checking
(error? result)                 ; Test if error

; Error handling
(if (error? result)
   (handle-error result)
   (use-value result))
```

**Why:** Explicit. Composable. First-class.

## Feature Set

### Current (Day 148+ — 173 tests passing, 558 primitives)

**Core:**
- `lambda` abstraction with De Bruijn indices
- Function application (beta reduction)
- Lexical scoping with closures
- `define` global definitions, named recursion
- TCO (tail call optimization via goto)
- `begin` sequencing

**Data:**
- Numbers (#42), Booleans (#t, #f), Nil (nil)
- Pairs (cons), Symbols (:name), Strings
- Errors (error — first-class values, not exceptions)
- Mutable refs (box/unbox/box-set!)

**Data Structures:**
- HashMap (hashmap-*), HashSet (set-*), Vector (vector-*)
- Deque (deque-*), Priority Queue (heap-*), Trie (trie-*)
- Sorted Map (sorted-map-*), Byte Buffer (bytebuf-*)

**Control:**
- `if` conditional, `quote`
- `match` pattern matching (guards, as-patterns, or-patterns, view patterns)

**Effects:**
- `perform`, `handle`, `handle-resume`
- Linear effects with `consume`
- `bind` for chaining

**Actors/Concurrency:**
- Actors, messages, channels
- Supervisors (one-for-one, one-for-all, rest-for-one)
- GenServer, ETS tables, process registry, timers
- Tasks, Agents, GenStage, DynamicSupervisor, Flow
- Multi-scheduler (BWoS deque), work-stealing
- Eventcount parking (Folly/Vyukov), QSBR reclamation

**Types:**
- Annotations (`type-decl`), validation (`type-validate`), inference (`type-infer`)

**Macros:**
- Pattern-based macros (`macro-rules`), stdlib macros (control, iteration, exception, pattern)

**Structures:**
- `struct-define`, `adt-define`, graph structures (CFG/DFG)

**Debug/Test:**
- `assert` assertions, `trace`, `type-of`
- `arity`, `source`, `deep-equal?`, `test-case`
- Structured test runner (--test mode, JSON Lines, coverage, leak detection)
- gen-int-shrink, gen-list-shrink (integrated Hedgehog-style shrinking)

**FFI:**
- JIT-compiled stubs (`ffi-call`) — ARM64 + x86-64

**Stdlib:**
- Math (sqrt, ^, trig, log, pi, e, rand), strings, POSIX
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

### Scheme-Like Naming

Guage uses familiar Scheme/Lisp naming conventions:

```
lambda  - Function abstraction
define  - Global definitions
if      - Conditional (3 args)
+       - Addition
error   - Create error values
assert  - Assertions
trace   - Debugging
type-of - Type inspection
nil     - Empty list
cons    - Pair constructor
car     - Head of pair
cdr     - Tail of pair
```

### Consistency Rules

1. **Prefix notation** - `(operator args...)`
2. **English names** - Familiar Scheme/Lisp vocabulary
3. **Predicate convention** - Names ending in `?` return boolean (`null?`, `error?`)
4. **Mutator convention** - Names ending in `!` mutate state (`box-set!`)
5. **Number literals** - Use `#` prefix (`#42`, `#3.14`)

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
(test-case (quote :test-name) expected actual)

; Assertions are expressions
(assert condition :error-message)

; Deep equality is primitive
(deep-equal? value1 value2)
```

### Test-Driven Development

1. Write test in Guage
2. Run test (fails)
3. Implement feature
4. Test passes
5. Commit

### No External Test Frameworks

Everything needed is **in the language**:
- `test-case` for test cases
- `assert` for assertions
- `deep-equal?` for equality
- `trace` for tracing

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
3. **Macros** - Pattern-based syntax transformation (`macro-rules`)
4. **Effects** - User-defined algebraic effect handlers (`handle`)
5. **Types** - User-defined types with ADTs (`adt-define`)

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

Use standard documentation comments:

```scheme
; Description: Factorial function
; Type: Int -> Int
; Property: (factorial 0) = 1
; Example: (factorial #5) -> #120

(define factorial (lambda (n)
  (if (equal? n #0)
     #1
     (* n (factorial (- n #1))))))
```

### API Documentation

- **Function name** - Show the name
- **Type signature** - Using arrow notation
- **Properties** - Using `assert`
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
**Primitives:** 558
**Tests:** 173/179 passing (97%)
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
    ├── primitives.{c,h}  - Built-in operations (558 primitives)
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
    ├── module.{c,h}      - Module loading (load)
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
- ✅ Module system (`load`, `module-info`)
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
(define spec (quote
  ((forall xs (equal? (length (sort xs)) (length xs)))  ; same length
   (forall xs (forall i (leq (get (sort xs) i) ...)))))) ; ordered
(define sort (synthesize spec))

; Repair broken implementation
(define broken (lambda (xs) (reverse xs)))   ; just reverses
(define fixed (repair spec broken))
```

**Time-Travel Debugging:**
```scheme
(define trace (trace-exec (f x)))            ; traced execution
(trace-at trace #42)                         ; state at step 42
(trace-cause trace :z)                       ; what caused z?
(define trace2 (trace-counterfactual trace (quote ((t #10) (x #999)))))
```

**Types That Prove:**
```scheme
(type-refine Sorted (assert (list Int) (forall i (leq (get self i) ...))))
(define merge (type-annotate (-> Sorted (-> Sorted Sorted))))
(assert quicksort (complexity O(n*log(n))))
```

**Cross-Program Analysis:**
```scheme
(define p1 (load (quote :service1)))         ; load program as value
(define p2 (load (quote :service2)))
(cfg-join p1 p2)                             ; joint CFG/DFG
(assert (cfg-join) (not deadlock))           ; prove no deadlock
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

1. **Follow Scheme-like syntax** - Use `lambda`, `define`, `if`, etc.
2. **Maintain first-class principles** - Everything is a value
3. **Write tests** - Use `test-case` and `assert`
4. **Document with conventions** - Use `; description:`, `; type:`, `; property:`, `Ex:`
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
