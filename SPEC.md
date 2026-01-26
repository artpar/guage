# Guage Language Specification v0.1

## Core Data Structure

Everything is a **Cell**:
- **Atom**: `#n` (number), `#t`/`#f` (bool), `:symbol`, `∅` (nil)
- **Pair**: `⟨a b⟩` (cons cell)

## The 42 Primitives (Runtime Evaluated)

### Core Lambda Calculus (6)
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⟨⟩` | `α → β → ⟨α β⟩` | Construct cell | ✅ DONE |
| `◁` | `⟨α β⟩ → α` | Head (car) | ✅ DONE |
| `▷` | `⟨α β⟩ → β` | Tail (cdr) | ✅ DONE |
| `λ` | Abstraction | Lambda | ✅ DONE |
| `·` | Application | Apply function | ✅ DONE |
| `0 1 2...` | Variable ref | De Bruijn index | ✅ DONE |

### Metaprogramming (3)
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⌜` | `α → ⌜α⌝` | Quote (code→data) | ✅ DONE |
| `⌞` | `⌜α⌝ → α` | Eval (data→code) | ❌ PLACEHOLDER |
| `≔` | Binding | Definition | ✅ DONE |

### Type Constructors (9) - COMPILE TIME ONLY
| Symbol | Type | Meaning |
|--------|------|---------|
| `→` | `α → β` | Function type |
| `⊗` | `α ⊗ β` | Product type |
| `⊎` | `α ⊎ β` | Sum type |
| `Π` | `Π.α` | Pi type (dependent) |
| `Σ` | `Σ.α` | Sigma type (dependent) |
| `⊤` | Top type | Any |
| `⊥` | Bottom type | Never |
| `∀` | Universal | Forall |
| `∃` | Existential | Exists |

### Linear Logic (4) - COMPILE TIME ONLY
| Symbol | Type | Meaning |
|--------|------|---------|
| `⊸` | `α ⊸ β` | Linear function |
| `!` | `!α` | Of-course (unlimited) |
| `?` | `?α` | Why-not (weakening) |
| `⊛` | `α ⊛ β` | Linear tensor |

### Session Types (5) - COMPILE TIME ONLY
| Symbol | Type | Meaning |
|--------|------|---------|
| `▷τ` | Send type | Send message |
| `◁τ` | Receive type | Receive message |
| `⊕` | Choice | Internal choice |
| `&` | Choice | External choice |
| `ε` | End | End session |

### Effects (4)
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⟪⟫` | Effect block | Effect computation | ❌ PLACEHOLDER |
| `↯` | Effect handler | Handle effects | ❌ PLACEHOLDER |
| `⤴` | Pure lift | Lift to effect | ❌ PLACEHOLDER |
| `≫` | Effect bind | Sequence effects | ❌ PLACEHOLDER |

### Refinement Types (4) - COMPILE TIME ONLY
| Symbol | Type | Meaning |
|--------|------|---------|
| `{⋅∣φ}` | `{ν:τ ∣ φ}` | Refinement |
| `⊢` | `⊢ φ` | Proof |
| `⊨` | `⊨ α φ` | Assert |
| `∴` | Therefore | Conclusion |

### Actors (3)
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⟳` | Spawn | Spawn actor | ❌ PLACEHOLDER |
| `→!` | Send | Send message | ❌ PLACEHOLDER |
| `←?` | Receive | Receive message | ❌ PLACEHOLDER |

### Documentation (3) - Auto-generated for user functions
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⌂` | `:symbol → string` | Get description | ✅ DONE |
| `⌂∈` | `:symbol → string` | Get type signature | ✅ DONE |
| `⌂≔` | `:symbol → list` | Get dependencies | ✅ DONE |

**Auto-Documentation System:**
- Every user function gets automatic documentation
- Extracts dependencies from function body
- Composes descriptions from constituent docs
- Infers simple type signatures
- Auto-prints when function is defined

Example:
```scheme
(≔ ! (λ (n) (? (≡ n #0) #1 (⊗ n (! (⊖ n #1))))))
```
Auto-prints:
```
📝 ! :: α → β
   Function using: ?, ≡, ⌜, ⊗, !, ...
   Dependencies: ?, ≡, ⌜, ⊗, !, ⊖
```

Query docs:
```scheme
(⌂ (⌜ !))      ; → :Function using: ...
(⌂∈ (⌜ !))     ; → :α → β
(⌂≔ (⌜ !))     ; → ⟨:? ⟨:≡ ⟨:⌜ ...⟩⟩⟩
```

### Comparison & Logic (4)
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `≡` | Equality | Equal | ✅ DONE |
| `≢` | Inequality | Not equal | ✅ DONE |
| `∧` | AND | Logical AND | ✅ DONE |
| `∨` | OR | Logical OR | ✅ DONE |

## Derived Operations (Not Primitives)

### Arithmetic
| Symbol | Meaning | Status |
|--------|---------|--------|
| `⊕` | Addition | ✅ DONE |
| `⊖` | Subtraction | ✅ DONE |
| `⊗` | Multiplication | ✅ DONE |
| `⊘` | Division | ✅ DONE |
| `<` | Less than | ✅ DONE |
| `>` | Greater than | ✅ DONE |
| `≤` | Less or equal | ✅ DONE |
| `≥` | Greater or equal | ✅ DONE |

### Type Predicates
| Symbol | Meaning | Status |
|--------|---------|--------|
| `ℕ?` | Is number | ✅ DONE |
| `𝔹?` | Is bool | ✅ DONE |
| `:?` | Is symbol | ✅ DONE |
| `∅?` | Is nil | ✅ DONE |
| `⟨⟩?` | Is pair | ✅ DONE |
| `#?` | Is atom | ✅ DONE |

### Control Flow
| Symbol | Meaning | Status |
|--------|---------|--------|
| `?` | Conditional (if) | ✅ DONE |

## Syntax Rules

### NO ENGLISH WORDS
- ❌ `cons`, `car`, `cdr`
- ❌ `define`, `lambda`, `quote`
- ❌ `true`, `false`, `nil`
- ✅ Only Unicode symbols

### De Bruijn Indices Only
Variables are referenced by index, not name:
- `λ.0` - identity function (λx.x)
- `λ.λ.1` - const function (λx.λy.x)
- `λ.λ.λ.(2 0 (1 0))` - compose (λf.λg.λx.f(g x))

### S-Expression Syntax
```
(⊕ 1 2)           ; 1 + 2 = 3
(⟨⟩ 1 2)          ; cons(1, 2)
(◁ (⟨⟩ 1 2))      ; car of cons = 1
(≔ x 42)          ; define x = 42
(λ 0)             ; λx.x (identity)
(⌜ (⊕ 1 2))       ; quote expression
```

## Examples

### Identity Function
```lisp
𝕀 ≔ λ.0
```

### Constant Function
```lisp
𝕂 ≔ λ.λ.1
```

### Composition
```lisp
∘ ≔ λ.λ.λ.(2 (1 0))
```

### Factorial (using named recursion)
```scheme
(≔ ! (λ (n) (? (≡ n #0) #1 (⊗ n (! (⊖ n #1))))))
(! #5)  ; → #120 ✅
```

### Fibonacci
```scheme
(≔ fib (λ (n) (? (< n #2) n (⊕ (fib (⊖ n #1)) (fib (⊖ n #2))))))
(fib #7)  ; → #13 ✅
```

## Turing Completeness

### Current Status: ✅ TURING COMPLETE

**What works:**
- ✅ Cell construction/destruction
- ✅ Arithmetic operations
- ✅ Boolean logic
- ✅ Conditionals
- ✅ Global definitions
- ✅ Lambda calculus with De Bruijn indices
- ✅ Function application (beta reduction)
- ✅ Lexical scoping with closures
- ✅ Named recursion (self-reference in ≔)
- ✅ Nested lambdas
- ✅ First-class functions

**Examples:**
```scheme
; Factorial
(≔ ! (λ (n) (? (≡ n #0) #1 (⊗ n (! (⊖ n #1))))))
(! #10)  ; → 3628800

; Fibonacci
(≔ fib (λ (n) (? (< n #2) n (⊕ (fib (⊖ n #1)) (fib (⊖ n #2))))))
(fib #10)  ; → 55

; Higher-order functions
(≔ twice (λ (f) (λ (x) (f (f x)))))
(≔ inc (λ (x) (⊕ x #1)))
((twice inc) #5)  ; → #7
```

### Proof of Turing Completeness:
1. ✅ De Bruijn index evaluation implemented
2. ✅ Lambda abstraction (closure creation) implemented
3. ✅ Application (function calling) implemented
4. ✅ Recursion via named bindings implemented
5. ✅ Can implement any computable function

## Self-Implementation Status: 🚧 IN PROGRESS

**Progress:**
- ✅ Turing complete (can compute anything)
- ✅ Code-as-data (cells + quote/eval)
- ✅ Auto-documentation system
- 🚧 Need: Parser in Guage
- 🚧 Need: Compiler in Guage
- 🚧 Need: Full ⌞ eval implementation

## Type System (Future)

All type primitives are **compile-time only**:
- Dependent types (Π, Σ)
- Linear types (⊸, !, ?)
- Session types (▷, ◁, ⊕, &)
- Refinement types ({⋅∣φ})
- Effect types (tracked at compile time)

Runtime is **untyped lambda calculus + cells**.

## Memory Model

### Reference Counting
- Every cell has refcount
- `cell_retain()` increments
- `cell_release()` decrements
- Zero refcount = freed

### Linear Types (tracked at runtime for debugging)
- `LINEAR_UNIQUE` = must consume exactly once
- `LINEAR_CONSUMED` = already used (error to use again)
- `LINEAR_BORROWED` = temporary borrow

### Capabilities (compile-time, debug at runtime)
- `CAP_READ` = can read
- `CAP_WRITE` = can write
- `CAP_SEND` = can send across actors
- `CAP_SHARE` = can share between threads

## Compilation Model

```
Source (symbols)
  → Parser (S-expressions)
  → Type Checker (dependent, linear, session, refinement)
  → De Bruijn Conversion (remove names)
  → Optimizer (zero-cost abstractions)
  → Codegen (C or LLVM)
  → Binary
```

## Philosophy

1. **NO ENGLISH** - Pure symbols only
2. **NO NAMES** - De Bruijn indices only
3. **TYPES ARE COMPILE TIME** - Runtime is untyped
4. **PATTERNS ARE TYPES** - Design patterns enforced
5. **EFFECTS ARE TRACKED** - No hidden side effects
6. **PROVABLY CORRECT** - Proof-carrying code optional
