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

### Metaprogramming Core (3)
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⌜` | `α → ⌜α⌝` | Quote (code→data) | ✅ DONE |
| `⌞` | `⌜α⌝ → α` | Eval (data→code) | ❌ PLACEHOLDER |
| `≔` | Binding | Definition | ✅ DONE |

### Pattern Matching (3) - CRITICAL FOR METAPROGRAMMING
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `∇` | Pattern match | Destructure with patterns | 🎯 NEXT |
| `≗` | `α → β → 𝔹` | Structural equality | 🎯 NEXT |
| `_` | Pattern | Wildcard (match anything) | 🎯 NEXT |

**Pattern Syntax:**
```scheme
(∇ expr
  [pattern₁ expr₁]
  [pattern₂ expr₂]
  ...)

; Patterns:
; - Numbers: #42
; - Symbols: :foo
; - Nil: ∅
; - Pairs: (⟨⟩ a b)
; - Wildcard: _
```

**Example:**
```scheme
(≔ length (λ (lst)
  (∇ lst
    [∅ #0]
    [(⟨⟩ _ tail) (⊕ #1 (length tail))])))
```

### Macro System (5) - HYGIENIC CODE TRANSFORMATION
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⧉` | Macro def | Define structural macro | ⏳ PLANNED |
| `⧈` | Macro params | Macro parameter list | ⏳ PLANNED |
| `` ` `` | Backquote | Quote with holes | ⏳ PLANNED |
| `,` | Unquote | Evaluate in quote | ⏳ PLANNED |
| `,@` | Splice | Splice list elements | ⏳ PLANNED |

**Macro Syntax:**
```scheme
(⧉ name (⧈ (param₁ param₂ ...)
  `(template with ,param₁ and ,param₂)))

; Usage: (name arg₁ arg₂)
; Expands at compile-time
```

**Example:**
```scheme
(⧉ when (⧈ (condition body)
  `(? ,condition ,body ∅)))

; (when (> x #0) (⊕ x #1))
; Expands to: (? (> x #0) (⊕ x #1) ∅)
```

### Generic Programming (3) - PARAMETRIC POLYMORPHISM
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⊳` | Generic param | Type/value parameter | ⏳ PLANNED |
| `⊲` | Instantiate | Apply generic | ⏳ PLANNED |
| `⊧` | Constraint | Type satisfies trait | ⏳ PLANNED |

**Generic Syntax:**
```scheme
(≔ identity (λ (⊳ T) (λ (x : T) x)))

; Instantiate: (⊲ identity ℕ)
; With constraint: (λ (⊳ T : (⊧ Ord)) ...)
```

**Example:**
```scheme
(≔ max (λ (⊳ T : (⊧ Ord)) (λ (a : T) (λ (b : T)
  (? (> a b) a b)))))

((⊲ max ℕ) #5 #10)  ; → #10
```

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

### Data Structures (15) - CRITICAL FOR METAPROGRAMMING
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⊙≔` | Define leaf | Define simple structure | ✅ DONE |
| `⊙` | Create leaf | Create leaf instance | ✅ DONE |
| `⊙→` | Get field | Access structure field | ✅ DONE |
| `⊙←` | Set field | Update structure field | ✅ DONE |
| `⊙?` | Type check | Check structure type | ✅ DONE |
| `⊚≔` | Define node | Define recursive structure (ADT) | 🎯 NEXT |
| `⊚` | Create node | Create node instance | 🎯 NEXT |
| `⊚→` | Get field | Access node field | 🎯 NEXT |
| `⊚?` | Variant check | Check type and variant | 🎯 NEXT |
| `⊝≔` | Define graph | Define graph structure | ⏳ PLANNED |
| `⊝` | Create graph | Create graph instance | ⏳ PLANNED |
| `⊝⊕` | Add node | Add node to graph | ⏳ PLANNED |
| `⊝⊗` | Add edge | Add edge to graph | ⏳ PLANNED |
| `⊝→` | Query graph | Query graph structure | ⏳ PLANNED |
| `⊝?` | Graph check | Check graph type | ⏳ PLANNED |

**Structure Syntax:**
```scheme
; Leaf structure (non-recursive)
(⊙≔ Point :x :y)
(≔ p (⊙ Point #3 #4))
(⊙→ p :x)  ; → #3

; Node structure (recursive ADT)
(⊚≔ List [:Nil] [:Cons :head :tail])
(≔ l (⊚ List :Cons #1 (⊚ List :Nil)))

; Graph structure
(⊝≔ Graph :nodes :edges)
(≔ g (⊝ Graph ∅ ∅))
(≔ g (⊝⊕ g node-data))
(≔ g (⊝⊗ g from-id to-id label))
```

**Why Data Structures Matter:**
- Foundation for pattern matching (can't match without knowing structure)
- CFG/DFG are graph structures (first-class values)
- Enable type-safe metaprogramming
- AI can reason about data shape

See `DATA_STRUCTURES.md` for complete specification.

### Documentation (3) - Auto-generated for user functions
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⌂` | `:symbol → string` | Get description | ✅ DONE |
| `⌂∈` | `:symbol → string` | Get type signature | ✅ DONE |
| `⌂≔` | `:symbol → list` | Get dependencies | ✅ DONE |

### Control/Data Flow (4) - Auto-generated first-class graphs
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⌂⟿` | `:symbol → CFG` | Get control flow graph | 🎯 NEXT |
| `⌂⇝` | `:symbol → DFG` | Get data flow graph | 🎯 NEXT |
| `⌂⊚` | `:symbol → CallGraph` | Get call graph | 🎯 NEXT |
| `⌂⊙` | `:symbol → DepGraph` | Get dependency graph | 🎯 NEXT |

**Auto-Documentation System:**
- Every user function gets automatic documentation via **recursive composition**
- Extracts dependencies from function body
- **Recursively composes** human-readable descriptions from AST structure
- Infers **most specific** type signatures (strongest typing first)
- Auto-prints when function is defined
- Generates **inverse of code execution** - natural language from code

**Type Inference (Strongest First):**
1. ℕ → ℕ - Uses only arithmetic (⊕, ⊖, ⊗, ⊘)
2. α → 𝔹 - Returns boolean (comparisons, predicates)
3. α → β - Generic polymorphic (fallback)

**Auto-Generated Graphs (First-Class Citizens):**

1. **⌂⟿ Control Flow Graph (CFG)**
   - Nodes: Basic blocks (sequences without branches)
   - Edges: Control flow (?, recursion, function calls)
   - Shows all possible execution paths
   - Identifies unreachable code
   - Used for: Optimization, dead code elimination

2. **⌂⇝ Data Flow Graph (DFG)**
   - Nodes: Operations (⊕, ⊗, ?, λ, etc)
   - Edges: Data dependencies (producer → consumer)
   - Shows value flow through computation
   - Identifies unused values
   - Used for: Optimization, const folding, CSE

3. **⌂⊚ Call Graph**
   - Nodes: Functions
   - Edges: Function calls
   - Shows caller/callee relationships
   - Identifies recursion cycles
   - Used for: Inlining, optimization order

4. **⌂⊙ Dependency Graph**
   - Nodes: Definitions (≔)
   - Edges: Symbol dependencies
   - Shows declaration order requirements
   - Identifies circular dependencies
   - Used for: Compilation order, module resolution

Example (documentation form):
```scheme
(≔ ! (λ (𝕩) (? (≡ 𝕩 #0) #1 (⊗ 𝕩 (! (⊖ 𝕩 #1))))))
```

Example (De Bruijn form - what actually runs):
```scheme
(≔ ! (λ (? (≡ 0 #0) #1 (⊗ 0 (! (⊖ 0 #1))))))
```

Auto-prints:
```
📝 ! :: ℕ → ℕ
   if equals 𝕩 and 0 then 1 else multiply 𝕩 and apply ! to subtract 𝕩 and 1
   Dependencies: ?, ≡, ⌜, ⊗, !, ⊖
```

More examples:
```scheme
; Documentation form
(≔ ⊗2 (λ (𝕩) (⊗ 𝕩 #2)))
📝 ⊗2 :: ℕ → ℕ
   multiply 𝕩 and 2

; De Bruijn form (actual)
(≔ ⊗2 (λ (⊗ 0 #2)))

; Documentation form
(≔ ≡0 (λ (𝕩) (≡ 𝕩 #0)))
📝 ≡0 :: α → 𝔹
   equals 𝕩 and 0

; De Bruijn form (actual)
(≔ ≡0 (λ (≡ 0 #0)))
```

Query docs:
```scheme
(⌂ (⌜ !))      ; → :if equals 𝕩 and 0...
(⌂∈ (⌜ !))     ; → :ℕ → ℕ
(⌂≔ (⌜ !))     ; → ⟨:? ⟨:≡ ⟨:⌜ ...⟩⟩⟩
```

Query graphs:
```scheme
(⌂⟿ (⌜ !))     ; → Control flow graph
; CFG:
;   [entry] → [≡ 𝕩 #0]
;   [≡ 𝕩 #0] --true--> [return #1]
;   [≡ 𝕩 #0] --false--> [⊗ 𝕩 (! (⊖ 𝕩 #1))]
;   [⊗ 𝕩 (! (⊖ 𝕩 #1))] → [recursive call !]
;   [recursive call !] → [exit]

(⌂⇝ (⌜ !))     ; → Data flow graph
; DFG:
;   𝕩 → [≡ with #0]
;   𝕩 → [⊖ with #1] → [! recursive] → [⊗ with 𝕩]
;   [⊗] → return

(⌂⊚ (⌜ !))     ; → Call graph
; CallGraph:
;   ! → {?, ≡, ⊗, ⊖, !}  ; Calls itself (recursion)

(⌂⊙ (⌜ !))     ; → Dependency graph
; DepGraph:
;   ! ← {?, ≡, ⌜, ⊗, ⊖}  ; Depends on these primitives
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
- `(λ 0)` - identity function
- `(λ (λ 1))` - const function
- `(λ (λ (λ (2 (1 0)))))` - compose

**Parameter naming:**
- At runtime: De Bruijn indices (0, 1, 2...) - NO NAMES
- In documentation: Mathematical symbols for clarity
  - `ƒ`, `𝕘`, `𝕙` - functions
  - `𝕩`, `𝕪`, `𝕫` - values
  - `⊙`, `◁`, `▷` - list elements
  - `⊡` - accumulator

**NO ENGLISH:** Not even single letters like `x`, `n`, `f`, `lst`

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
