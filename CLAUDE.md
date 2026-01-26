# Guage: The Ultralanguage

## Vision

Guage is a **Turing-complete ultralanguage** designed to be the ultimate programming language - one that subsumes all others through careful design of core primitives and systematic extension.

### Ultimate Goals

1. **Self-hosting** - Guage compiler written in Guage
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

### 2. First-Class Everything

Everything is a value that can be passed, returned, and inspected:

- **Functions:** λ expressions with closures
- **Errors:** ⚠ values, not exceptions
- **Debugging:** ⟲ trace returns the value
- **Types:** Can be computed and passed (future)
- **Effects:** Composable effect handlers (future)
- **Tests:** ⊨ test cases are expressions

**Why:** Consistency. Composability. Metaprogramming.

### 3. Single Source of Truth

- **No dual paths** - One canonical way to do things
- **No unnecessary transforms** - Direct representation
- **Values as boundaries** - All interfaces use simple values
- **No glue layer complexity** - Direct mapping

**Why:** Simplicity. Maintainability. Understandability.

### 4. Development-First

- **Never lint** - Code is correct by construction
- **Never type check during dev** - Types are gradual
- **Human tests** - Developer validates behavior
- **Hot-reload assumed** - No build step during development
- **Values as API boundaries** - Simple integration

**Why:** Fast iteration. Developer productivity. Flexibility.

### 5. Mathematical Foundation

Based on:
- **Lambda calculus** - Functions as first-class values
- **De Bruijn indices** - Efficient variable representation
- **Type theory** - Dependent types (future)
- **Effect algebras** - Composable effects (future)
- **Actor model** - Concurrent computation (future)

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

### Current (Turing Complete ✅)

**Core:**
- λ abstraction with De Bruijn indices
- Function application (beta reduction)
- Lexical scoping with closures
- ≔ global definitions

**Data:**
- Numbers (#42)
- Booleans (#t, #f)
- Nil (∅)
- Pairs (⟨⟩)
- Symbols (:name)
- Errors (⚠)

**Control:**
- ? conditional
- ⌜ quote
- ⌞ eval (future)

**Primitives:**
- Arithmetic: ⊕ ⊖ ⊗ ⊘
- Comparison: ≡ ≢ < > ≤ ≥
- Logic: ∧ ∨ ¬
- Lists: ⟨⟩ ◁ ▷

**Debug/Test:**
- ⊢ assertions
- ⟲ trace
- ⊙ type-of
- ⧉ arity
- ⊛ source
- ≟ deep-equal
- ⊨ test-case

### Planned

**Near-term:**
- Named recursion (self-reference in ≔)
- Y combinator (pure lambda recursion)
- Pattern matching
- List comprehensions
- Standard library

**Mid-term:**
- Module system
- Dependent types
- Type inference
- Linear types (infrastructure present)
- Proof system

**Long-term:**
- Effect system (⟪⟫, ↯, ⤴, ≫)
- Actor model (⟳, →!, ←?)
- Native compilation
- Incremental compilation
- IDE support

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
├── Evaluate → Beta reduction
└── Reference counting GC
```

**Goal:** Get to self-hosting ASAP.

### Self-Hosting Phase (Future)

```
Guage implementation in Guage
├── Parser written in Guage
├── Compiler written in Guage
├── Runtime written in Guage (with FFI)
└── Standard library in Guage
```

**Goal:** Prove the language can express itself.

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
3. **Macros** - Syntax transformation (future)
4. **Effects** - User-defined effect handlers (future)
5. **Types** - User-defined types (future)

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
**GC:** Reference counting
**Representation:** De Bruijn indices
**Environment:** Hybrid (named at top, indexed in lambdas)
**Tests:** 14/14 passing ✅
**Status:** Turing complete ✅

### Code Organization

```
bootstrap/bootstrap/
├── cell.{c,h}        - Core data structures
├── eval.{c,h}        - Evaluator
├── debruijn.{c,h}    - De Bruijn conversion
├── debug.{c,h}       - Stack traces
├── primitives.{c,h}  - Built-in operations
├── main.c            - Parser and REPL
└── tests/            - Test suite
```

### Build and Test

```bash
make clean && make      # Build
./guage                 # REPL
./run_tests.sh          # Run tests
./guage < file.scm      # Run file
```

## Success Metrics

### Short-term (Bootstrap)

- ✅ Turing complete
- ✅ First-class errors/debug/test
- ⏳ Named recursion
- ⏳ Standard library basics
- ⏳ Module system

### Mid-term (Self-hosting)

- ⏳ Parser in Guage
- ⏳ Compiler in Guage
- ⏳ Type checker in Guage
- ⏳ Self-hosting complete

### Long-term (Production)

- ⏳ Native compilation
- ⏳ Effect system
- ⏳ Actor runtime
- ⏳ Package manager
- ⏳ Real-world usage

---

## Contribution Guidelines

When contributing to Guage:

1. **Follow symbol-only syntax** - No English keywords
2. **Maintain first-class principles** - Everything is a value
3. **Write tests** - Use ⊨ and ⊢
4. **Document with symbols** - Use ⌂, ∈, ⊢, Ex
5. **Keep it simple** - Single path, no complexity
6. **Reference count carefully** - No leaks

## Questions?

Read:
- `SESSION_HANDOFF.md` - Current implementation details
- `IMPLEMENTATION_STATUS.md` - Feature checklist
- `SUMMARY.md` - High-level overview
- This file - Philosophy and principles

---

**Guage: Where symbols speak louder than words** 🚀
