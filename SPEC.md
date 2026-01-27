---
Status: CURRENT
Created: 2025-12-01
Updated: 2026-01-27
Purpose: Canonical language specification
---

# Guage Language Specification v0.1

## Core Data Structure

Everything is a **Cell**:
- **Atom**: `#n` (number), `#t`/`#f` (bool), `:symbol` (keyword), `∅` (nil)
- **Pair**: `⟨a b⟩` (cons cell)

### Keywords (Self-Evaluating Symbols)

**Symbols starting with `:` (colon) are self-evaluating** - they don't require quoting:

```scheme
:test      ; → :test (self-evaluating, like #42 or #t)
:Point     ; → :Point
:x         ; → :x
```

**Use cases:**
- Structure type tags: `:Point`, `:Rectangle`
- Field names: `:x`, `:y`, `:width`
- Enum values: `:red`, `:green`, `:blue`
- Message tags: `:ok`, `:error`

**See:** `KEYWORDS.md` for complete specification.

## Runtime Primitives (63 Total)

**Status:** 63 primitives implemented (6 placeholders, 57 fully functional)

### Core Lambda Calculus (3) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⟨⟩` | `α → β → ⟨α β⟩` | Construct cell | ✅ DONE |
| `◁` | `⟨α β⟩ → α` | Head (car) | ✅ DONE |
| `▷` | `⟨α β⟩ → β` | Tail (cdr) | ✅ DONE |

**Note:** `λ`, `·`, `≔`, and De Bruijn indices (0, 1, 2...) are part of the evaluator, not primitives.

### Metaprogramming Core (2) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⌜` | `α → ⌜α⌝` | Quote (code→data) | ✅ DONE |
| `⌞` | `⌜α⌝ → α` | Eval (data→code) | ✅ DONE |

### Pattern Matching (1) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `∇` | `α → [[⌜pattern⌝ result]] → β` | Pattern match expression | ✅ DONE (Day 16) |

**Note:** As of Day 16, supports:
- **Wildcard** (_) - matches anything
- **Literals** - numbers, booleans, symbols, keywords
- **Variables** - bind matched value to name (Day 16 ✅)
- **Pair patterns** (Day 17) - coming soon
- **ADT patterns** (Day 18-19) - coming soon

**Syntax:**
```scheme
; New clean syntax (Day 16+)
(∇ value (⌜ ((pattern₁ result₁)
             (pattern₂ result₂)
             ...)))
```

**Pattern Types:**
- `_` - Wildcard (matches anything, no binding)
- `#42`, `#t`, `:foo` - Literals (match exact value)
- `x`, `n`, `value` - Variables (bind value to name)

**Examples:**
```scheme
; Wildcard pattern
(∇ #42 (⌜ ((_ :ok))))  ; → :ok

; Literal patterns with fallback
(∇ #42 (⌜ ((#42 :matched) (_ :other))))  ; → :matched

; Variable pattern (Day 16)
(∇ #42 (⌜ ((x x))))  ; → #42 (x binds to #42)

; Variable in computation
(∇ #5 (⌜ ((n (⊗ n #2)))))  ; → #10

; Multiple clauses
(∇ #50 (⌜ ((#42 :is-42) (n (⊗ n #2)))))  ; → #100
```

### Comparison & Logic (5) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `≡` | `α → α → 𝔹` | Equality | ✅ DONE |
| `≢` | `α → α → 𝔹` | Inequality | ✅ DONE |
| `∧` | `𝔹 → 𝔹 → 𝔹` | Logical AND | ✅ DONE |
| `∨` | `𝔹 → 𝔹 → 𝔹` | Logical OR | ✅ DONE |
| `¬` | `𝔹 → 𝔹` | Logical NOT | ✅ DONE |

### Arithmetic (9) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⊕` | `ℕ → ℕ → ℕ` | Addition | ✅ DONE |
| `⊖` | `ℕ → ℕ → ℕ` | Subtraction | ✅ DONE |
| `⊗` | `ℕ → ℕ → ℕ` | Multiplication | ✅ DONE |
| `⊘` | `ℕ → ℕ → ℕ` | Division (float) | ✅ DONE |
| `%` | `ℕ → ℕ → ℕ` | Modulo (remainder) | ✅ DONE |
| `<` | `ℕ → ℕ → 𝔹` | Less than | ✅ DONE |
| `>` | `ℕ → ℕ → 𝔹` | Greater than | ✅ DONE |
| `≤` | `ℕ → ℕ → 𝔹` | Less or equal | ✅ DONE |
| `≥` | `ℕ → ℕ → 𝔹` | Greater or equal | ✅ DONE |

### Type Predicates (6) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `ℕ?` | `α → 𝔹` | Is number | ✅ DONE |
| `𝔹?` | `α → 𝔹` | Is boolean | ✅ DONE |
| `:?` | `α → 𝔹` | Is symbol | ✅ DONE |
| `∅?` | `α → 𝔹` | Is nil | ✅ DONE |
| `⟨⟩?` | `α → 𝔹` | Is pair | ✅ DONE |
| `#?` | `α → 𝔹` | Is atom | ✅ DONE |

### Debug & Error Handling (4) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⚠` | `:symbol → α → ⚠` | Create error value | ✅ DONE |
| `⚠?` | `α → 𝔹` | Test if error | ✅ DONE |
| `⊢` | `𝔹 → :symbol → 𝔹 \| ⚠` | Assert condition | ✅ DONE |
| `⟲` | `α → α` | Trace (debug print) | ✅ DONE |

### Self-Introspection (2) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⧉` | `λ → ℕ` | Get arity of lambda | ✅ DONE |
| `⊛` | `λ → expression` | Get source code | ✅ DONE |

### Testing (2) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `≟` | `α → α → 𝔹` | Deep equality test | ✅ DONE |
| `⊨` | `:symbol → α → α → 𝔹 \| ⚠` | Test case | ✅ DONE |

### Effects (4) - PLACEHOLDERS ONLY
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⟪⟫` | `effect → α` | Effect block | ❌ PLACEHOLDER |
| `↯` | `effect → handler → α` | Effect handler | ❌ PLACEHOLDER |
| `⤴` | `α → effect` | Lift to effect | ❌ PLACEHOLDER |
| `≫` | `effect → (α → effect) → effect` | Effect bind | ❌ PLACEHOLDER |

**Note:** Effects are stubs for Phase 4+. Return nil currently.

### Actors (3) - PLACEHOLDERS ONLY
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⟳` | `behavior → actor` | Spawn actor | ❌ PLACEHOLDER |
| `→!` | `actor → message → ()` | Send message | ❌ PLACEHOLDER |
| `←?` | `() → message` | Receive message | ❌ PLACEHOLDER |

**Note:** Actors are stubs for Phase 5+. Return nil currently.

### Documentation (5) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⌂` | `:symbol → string` | Get description | ✅ DONE |
| `⌂∈` | `:symbol → string` | Get type signature | ✅ DONE |
| `⌂≔` | `:symbol → [symbols]` | Get dependencies | ✅ DONE |
| `⌂⊛` | `:symbol → expression` | Get source code | ✅ DONE |
| `⌂⊨` | `:symbol → [tests]` | Auto-generate tests | ✅ DONE |

### Control/Data Flow (2) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⌂⟿` | `:symbol → CFG` | Get control flow graph | ✅ DONE |
| `⌂⇝` | `:symbol → DFG` | Get data flow graph | ✅ DONE |

### Structure Primitives - Leaf (5) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⊙≔` | `:symbol → [:symbol] → :symbol` | Define leaf structure | ✅ DONE |
| `⊙` | `:symbol → [α] → ⊙` | Create structure instance | ✅ DONE |
| `⊙→` | `⊙ → :symbol → α` | Get field value | ✅ DONE |
| `⊙←` | `⊙ → :symbol → α → ⊙` | Update field (immutable) | ✅ DONE |
| `⊙?` | `α → :symbol → 𝔹` | Check structure type | ✅ DONE |

### Structure Primitives - Node/ADT (4) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⊚≔` | `:symbol → [[variant]] → :symbol` | Define node/ADT type | ✅ DONE |
| `⊚` | `:symbol → :symbol → [α] → ⊚` | Create node instance | ✅ DONE |
| `⊚→` | `⊚ → :symbol → α` | Get node field | ✅ DONE |
| `⊚?` | `α → :symbol → :symbol → 𝔹` | Check node type/variant | ✅ DONE |

### Graph Primitives (6) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⊝≔` | `:symbol → :symbol → [:symbol] → :symbol` | Define graph type | ✅ DONE |
| `⊝` | `:symbol → ⊝` | Create empty graph | ✅ DONE |
| `⊝⊕` | `⊝ → α → ⊝` | Add node (immutable) | ✅ DONE |
| `⊝⊗` | `⊝ → α → α → α → ⊝` | Add edge (immutable) | ✅ DONE |
| `⊝→` | `⊝ → :symbol → α` | Query graph property | ✅ DONE |
| `⊝?` | `α → :symbol → 𝔹` | Check graph type | ✅ DONE |

**Graph Type Restrictions:**
Graph types are currently restricted to 5 predefined types for metaprogramming:
- `:generic` - General-purpose user-defined graphs
- `:cfg` - Control Flow Graphs (from ⌂⟿)
- `:dfg` - Data Flow Graphs (from ⌂⇝)
- `:call` - Call Graphs (future)
- `:dep` - Dependency Graphs (future)

Use `:generic` for custom graph types. This restriction enables specialized graph algorithms and optimizations for compiler metaprogramming while still allowing user-defined graph structures.

---

## Planned Primitives (Not Yet Implemented)

### Pattern Matching (3) - CRITICAL FOR METAPROGRAMMING
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `∇` | Pattern match | Destructure with patterns | 🎯 NEXT |
| `≗` | `α → β → 𝔹` | Structural equality | 🎯 NEXT |
| `_` | Pattern | Wildcard (match anything) | 🎯 NEXT |

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
; Keywords (:Point, :x, :y) are self-evaluating - no quotes needed!
(⊙≔ :Point :x :y)
(≔ p (⊙ :Point #3 #4))
(⊙→ p :x)  ; → #3

; Node structure (recursive ADT)
; Note: Variant definitions must be quoted!
(⊚≔ :List (⌜ (:Nil)) (⌜ (:Cons :head :tail)))
(≔ empty (⊚ :List :Nil))
(≔ l (⊚ :List :Cons #1 empty))
(⊚→ l :head)  ; → #1

; Graph structure
; Note: graph_type must be :generic, :cfg, :dfg, :call, or :dep
(⊝≔ :MyGraph :generic :nodes :edges)
(≔ g (⊝ :MyGraph))
(≔ g (⊝⊕ g :node1))
(≔ g (⊝⊗ g :node1 :node2 :edge-label))
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
| `⊘` | Division (float) | ✅ DONE |
| `%` | Modulo (remainder) | ✅ DONE |
| `<` | Less than | ✅ DONE |
| `>` | Greater than | ✅ DONE |
| `≤` | Less or equal | ✅ DONE |
| `≥` | Greater or equal | ✅ DONE |

**Note:** Division `⊘` performs floating-point division. For integer division with remainder, use `⊘` followed by `%`:
```scheme
(⊘ #10 #3)  ; → #3.33333 (float result)
(% #10 #3)  ; → #1 (remainder)

; GCD using modulo
(≔ gcd (λ (a b) (? (≡ b #0) a (gcd b (% a b)))))
(gcd #48 #18)  ; → #6 ✅
```

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
7. **EVERYTHING IS QUERYABLE** - CFG/DFG/traces as first-class values
8. **CODE IS DATA IS PROOF** - Metaprogramming at all levels

## Metaprogramming Vision (Native First-Class Features)

**Status:** Core language design - Infrastructure being built NOW
**Implementation:** Incremental (infrastructure first, full features later)
**See:** `METAPROGRAMMING_VISION.md` for complete specification

**CRITICAL:** These are NOT afterthoughts or "Phase 5 extras". They are **native, first-class citizens** that must be supported from the ground up. Current work (Phase 2C) is building the foundation.

### I. Program Synthesis & Repair
- **⊛** - Synthesize code from specifications
- **◂** - Repair broken code automatically
- Specifications as first-class values

### II. Semantic Versioning & API Evolution
- **⊑** - Subtype compatibility check
- **⋈** - Migration adapter generation
- **⊿** - Automatic client upgrade functions
- APIs as first-class, analyzable values

### III. Refinement Types & Dependent Types
- **⊢** - Types carry proofs (refinement)
- **⊡** - Dependent type definitions
- **↓** - Termination proofs
- **O** - Complexity bounds in types
- Invalid states unrepresentable

### IV. Time-Travel Debugging & Causal Analysis
- **⊙⊳** - Traced execution (full history)
- **⊆** - Modify trace (counterfactual)
- **⨳** - Search trace for violations
- **◊** - Replay from any point
- Execution traces as queryable values

### V. Transparent Distribution & Migration
- **⫸** - Capturable computations
- **⤒/⤓** - Upload/download state
- **⫷** - Auto-parallelize
- **⇝** - Hot code swapping
- Serialize, migrate, resume anywhere

### VI. Self-Optimizing Code
- **⊛** - Profile-guided optimization
- **◎** - Continuous optimization
- **Θ** - Optimization strategies
- Code improves itself at runtime

### VII. Self-Documenting & Self-Testing
- **📖** - Generate documentation
- **⊙?** - Generate tests from types
- **⊙⊗** - Mutation testing
- Coverage and property-based testing

### VIII. Cross-Program Analysis
- **⋘** - Load program as value
- **⊙⋈** - Joint CFG/DFG analysis
- Taint analysis, deadlock proofs
- Whole-program optimization

**Why This Matters:**

Unlike Coq/Agda (separate proof languages) or traditional metaprogramming (text manipulation), Guage makes **all aspects of computation** first-class values:
- CFG/DFG are data structures you can query
- Specifications generate code automatically
- Types prove properties at compile time
- Execution traces are replayable and modifiable
- Programs analyze and optimize themselves

This enables AI-assisted development where the compiler helps you write, prove, test, optimize, and deploy code.

**Implementation Timeline:**
- Phase 2C: Data structures (CURRENT)
- Phase 3: Pattern matching, macros, generics (18 weeks)
- Phase 4: Self-hosting, type system (12 weeks)
- Phase 5: Advanced metaprogramming (36 weeks)
- Phase 6: Distribution and analysis (24 weeks)
- **Total:** ~21 months to full vision

See `METAPROGRAMMING_VISION.md` for detailed specifications and implementation strategy.
