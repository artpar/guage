---
Status: REFERENCE
Created: 2026-01-10
Updated: 2026-01-27
Purpose: Complete catalog of all symbols and their meanings
---

# Guage Symbolic Vocabulary
## Complete Symbol System for Pure First Language

**Principle:** NO ENGLISH. EVER. Not in core, not in stdlib, not in documentation.
**Why:** AI doesn't think in English. Symbols are universal, unambiguous, language-independent.

---

## Data Structures (Structural Patterns Only)

### Linear Sequences

No "List" type - just tagged pairs:

```scheme
; Empty sequence
∅

; Single element
(⟨⟩ #1 ∅)

; Multiple elements
(⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅)))
```

**Construction:**
- `⟨⟩` - Pair (already have)
- No `cons`, no `list` - just compose pairs

### Binary Trees

Tagged patterns with symbols:

```scheme
; Empty tree
∅

; Leaf (terminal node)
(⟨⟩ :⊙ value)         ; ⊙ = dot/leaf marker

; Internal node
(⟨⟩ :⊚ (⟨⟩ left (⟨⟩ value right)))  ; ⊚ = circle/node marker
```

**Symbols:**
- `:⊙` (U+2299 CIRCLED DOT) - Leaf/terminal
- `:⊚` (U+229A CIRCLED RING) - Internal node

### Hash Maps / Dictionaries

Key-value pairs:

```scheme
; Empty map
∅

; Single entry
(⟨⟩ (⟨⟩ key value) ∅)

; Multiple entries
(⟨⟩ (⟨⟩ :x #10) (⟨⟩ (⟨⟩ :y #20) ∅))
```

**No "HashMap" type - just nested pairs with structure.**

### Sets

Just list of unique elements (no duplicates):

```scheme
(⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅)))
```

**Uniqueness enforced by algorithms, not type.**

### Graphs

Adjacency representation:

```scheme
; Node: (⟨⟩ :⊝ (⟨⟩ id (⟨⟩ value edges)))
; Edge: (⟨⟩ :⊲ (⟨⟩ from to))

; Example: 1→2, 1→3
(⟨⟩ (⟨⟩ :⊝ (⟨⟩ #1 (⟨⟩ :a (⟨⟩ (⟨⟩ :⊲ (⟨⟩ #1 #2))
                             (⟨⟩ (⟨⟩ :⊲ (⟨⟩ #1 #3)) ∅)))))
    ∅)
```

**Symbols:**
- `:⊝` (U+229D CIRCLED DASH) - Graph node
- `:⊲` (U+22B2 NORMAL SUBGROUP) - Directed edge
- `:⊳` (U+22B3 CONTAINS) - Reverse edge

---

## Standard Library Operations

### Sequence Operations

| Symbol | Unicode | Meaning | Type |
|--------|---------|---------|------|
| `⤇` | U+2907 | Map/transform | `(α → β) → [α] → [β]` |
| `⊻` | U+22BB | Filter/select | `(α → 𝔹) → [α] → [α]` |
| `⥁` | U+2941 | Fold left | `(β → α → β) → β → [α] → β` |
| `⥂` | U+2942 | Fold right | `(α → β → β) → β → [α] → β` |
| `⊎` | U+228E | Concat/append | `[α] → [α] → [α]` |
| `⥮` | U+296E | Reverse | `[α] → [α]` |
| `⊐` | U+2290 | Take n | `ℕ → [α] → [α]` |
| `⊏` | U+228F | Drop n | `ℕ → [α] → [α]` |
| `⫴` | U+2AF4 | Zip | `[α] → [β] → [(α,β)]` |
| `⊙⊙` | U+2299 | Flatten | `[[α]] → [α]` |
| `⊼` | U+22BC | Sort | `[α] → [α]` (needs Ord) |
| `⊽` | U+22BD | Group by | `(α → β) → [α] → [[α]]` |
| `⧺` | U+29FA | Intersperse | `α → [α] → [α]` |

**Examples:**
```scheme
; Map: transform each element
(⤇ (λ (x) (⊗ x #2)) (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅))))
; → (⟨⟩ #2 (⟨⟩ #4 (⟨⟩ #6 ∅)))

; Filter: keep elements matching predicate
(⊻ (λ (x) (≡ (⊘ x #2) #0)) (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 (⟨⟩ #4 ∅)))))
; → (⟨⟩ #2 (⟨⟩ #4 ∅))

; Fold: reduce to single value
(⥁ ⊕ #0 (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅))))
; → #6
```

### Tree Operations

| Symbol | Unicode | Meaning | Type |
|--------|---------|---------|------|
| `⊙→` | - | Leaf constructor | `α → Tree α` |
| `⊚→` | - | Node constructor | `Tree α → α → Tree α → Tree α` |
| `⊙←` | - | Tree map | `(α → β) → Tree α → Tree β` |
| `⊙⥁` | - | Tree fold | `(β → α → β → β) → β → Tree α → β` |
| `⊙⊼` | - | Tree insert | `α → Tree α → Tree α` (needs Ord) |
| `⊙∋` | - | Tree search | `α → Tree α → 𝔹` |
| `⊙⊣` | - | Tree left rotate | `Tree α → Tree α` |
| `⊙⊢` | - | Tree right rotate | `Tree α → Tree α` |
| `⊙⚖` | - | Tree balance | `Tree α → Tree α` |

**Examples:**
```scheme
; Construct tree
(≔ tree (⊚→ (⊙→ #1) #2 (⊙→ #3)))
; Structure: node(leaf(1), 2, leaf(3))

; Map over tree
(⊙← (λ (x) (⊗ x #2)) tree)

; Fold tree (sum all values)
(⊙⥁ (λ (l v r) (⊕ l (⊕ v r))) #0 tree)
; → #6
```

### Map/Dictionary Operations

| Symbol | Unicode | Meaning | Type |
|--------|---------|---------|------|
| `⟐` | U+27D0 | Map empty | `Map k v` |
| `⟐≔` | - | Map insert | `k → v → Map k v → Map k v` |
| `⟐∋` | - | Map lookup | `k → Map k v → Maybe v` |
| `⟐∌` | - | Map delete | `k → Map k v → Map k v` |
| `⟐⊨` | - | Map has key | `k → Map k v → 𝔹` |
| `⟐◁` | - | Map keys | `Map k v → [k]` |
| `⟐▷` | - | Map values | `Map k v → [v]` |
| `⟐⤇` | - | Map transform | `(v → w) → Map k v → Map k w` |
| `⟐⊻` | - | Map filter | `(k → v → 𝔹) → Map k v → Map k v` |
| `⟐⊎` | - | Map merge | `Map k v → Map k v → Map k v` |

### Set Operations

| Symbol | Unicode | Meaning | Type |
|--------|---------|---------|------|
| `∅` | U+2205 | Empty set | (already have) |
| `∪` | U+222A | Union | `Set α → Set α → Set α` |
| `∩` | U+2229 | Intersection | `Set α → Set α → Set α` |
| `∖` | U+2216 | Difference | `Set α → Set α → Set α` |
| `⊆` | U+2286 | Subset | `Set α → Set α → 𝔹` |
| `⊇` | U+2287 | Superset | `Set α → Set α → 𝔹` |
| `∈` | U+2208 | Member | `α → Set α → 𝔹` |
| `∉` | U+2209 | Not member | `α → Set α → 𝔹` |
| `⊌` | U+228C | Multiset union | `Set α → Set α → Set α` |

---

## Algorithms

### Sorting

| Symbol | Unicode | Meaning | Algorithm |
|--------|---------|---------|-----------|
| `⊼` | U+22BC | Generic sort | Uses Ord constraint |
| `⊼⇅` | - | Quicksort | Divide and conquer |
| `⊼⊎` | - | Merge sort | Stable, O(n log n) |
| `⊼⊙` | - | Heap sort | In-place, O(n log n) |
| `⊼⟲` | - | Bubble sort | Simple, O(n²) |
| `⊼⥁` | - | Insertion sort | Adaptive, O(n²) |

### Searching

| Symbol | Unicode | Meaning | Algorithm |
|--------|---------|---------|-----------|
| `⊐∋` | - | Linear search | O(n) |
| `⊐⇅` | - | Binary search | O(log n), needs sorted |
| `⊐⊙` | - | Jump search | O(√n) |
| `⊐⌗` | - | Interpolation search | O(log log n) |
| `⊐⊚` | - | Depth-first search | Graph traversal |
| `⊐⊡` | - | Breadth-first search | Graph traversal |

### Graph Algorithms

| Symbol | Unicode | Meaning | Algorithm |
|--------|---------|---------|-----------|
| `⊚⟿` | - | Shortest path | Dijkstra's |
| `⊚∞` | - | All pairs shortest | Floyd-Warshall |
| `⊚⊼` | - | Topological sort | DAG ordering |
| `⊚⊙` | - | Minimum spanning tree | Prim's/Kruskal's |
| `⊚⟲` | - | Cycle detection | DFS-based |
| `⊚⊳` | - | Strongly connected components | Tarjan's |

### Dynamic Programming

| Symbol | Unicode | Meaning | Algorithm |
|--------|---------|---------|-----------|
| `⊡⊡` | - | Memoization | Cache results |
| `⊡⇅` | - | Divide and conquer with memo | |
| `⊡⊕` | - | Knapsack | 0/1 knapsack |
| `⊡⤇` | - | Longest common subsequence | |
| `⊡⊏` | - | Longest increasing subsequence | |

---

## Design Patterns (Structural)

### Creational Patterns

| Symbol | Unicode | Meaning | Pattern |
|--------|---------|---------|---------|
| `⊚→` | - | Factory | Return constructor function |
| `⊚⊚` | - | Builder | Compose with multiple steps |
| `⊙!` | - | Singleton | Unique instance (linear type) |
| `⊚⧉` | - | Prototype | Clone existing structure |
| `⊚⊳` | - | Abstract factory | Return factory functions |

### Structural Patterns

| Symbol | Unicode | Meaning | Pattern |
|--------|---------|---------|---------|
| `⊚⊎` | - | Adapter | Wrap to match interface |
| `⊚⊡` | - | Composite | Tree of uniform operations |
| `⊚◁` | - | Decorator | Add behavior via wrapping |
| `⊚⊲` | - | Facade | Simplified interface |
| `⊚⋈` | - | Proxy | Control access |
| `⊚⊻` | - | Bridge | Decouple abstraction/implementation |

### Behavioral Patterns

| Symbol | Unicode | Meaning | Pattern |
|--------|---------|---------|---------|
| `⊚⟲` | - | Observer | Pub/sub notification |
| `⊚⇄` | - | Iterator | Sequential access |
| `⊚⤇` | - | Visitor | Operate on structure |
| `⊚∇` | - | Strategy | Pluggable algorithms |
| `⊚⊧` | - | Command | Encapsulate action |
| `⊚⋮` | - | Chain of responsibility | Pass through handlers |
| `⊚⊨` | - | State machine | State transitions |

---

## Type-Level Constructs

### Type Constructors (Compile-time only)

| Symbol | Unicode | Meaning | Kind |
|--------|---------|---------|------|
| `ℕ` | U+2115 | Natural numbers | `*` |
| `ℤ` | U+2124 | Integers | `*` |
| `ℚ` | U+211A | Rationals | `*` |
| `ℝ` | U+211D | Reals | `*` |
| `𝔹` | U+1D539 | Booleans | `*` |
| `→` | U+2192 | Function type | `* → * → *` |
| `⊗` | U+2297 | Product type | `* → * → *` |
| `⊎` | U+228E | Sum type | `* → * → *` |
| `Π` | U+03A0 | Dependent product | `(x:α) → β(x)` |
| `Σ` | U+03A3 | Dependent sum | `(x:α) × β(x)` |
| `∀` | U+2200 | Universal quantification | `∀α. τ` |
| `∃` | U+2203 | Existential quantification | `∃α. τ` |
| `⊤` | U+22A4 | Top type (any) | `*` |
| `⊥` | U+22A5 | Bottom type (never) | `*` |

### Common Type Aliases (Just Patterns)

```scheme
; Maybe type: ∅ or (⟨⟩ :⊙ value)
:⊙  ; "Just" tag
∅   ; "Nothing"

; Either type: (⟨⟩ :◁ left) or (⟨⟩ :▷ right)
:◁  ; "Left" tag
:▷  ; "Right" tag

; Result type: (⟨⟩ :✓ value) or (⟨⟩ :✗ error)
:✓  ; Success tag
:✗  ; Failure tag
```

---

## Trait System (Constraints)

### Comparison Traits

| Symbol | Unicode | Meaning | Methods |
|--------|---------|---------|---------|
| `⊧≡` | - | Equality | `≡`, `≢` |
| `⊧⊴` | - | Ordering | `<`, `>`, `≤`, `≥` |
| `⊧⊻` | - | Partial order | `⊴`, `⋢` (not comparable) |

### Numeric Traits

| Symbol | Unicode | Meaning | Methods |
|--------|---------|---------|---------|
| `⊧⊕` | - | Addition | `⊕`, `⊖` |
| `⊧⊗` | - | Multiplication | `⊗`, `⊘` |
| `⊧ℕ` | - | Natural | All numeric ops |
| `⊧ℤ` | - | Integer | Includes negation |
| `⊧ℝ` | - | Real | Includes √, sin, cos, etc |

### Collection Traits

| Symbol | Unicode | Meaning | Methods |
|--------|---------|---------|---------|
| `⊧⤇` | - | Mappable (Functor) | `⤇` (map) |
| `⊧⊎` | - | Appendable | `⊎` (append) |
| `⊧⊻` | - | Filterable | `⊻` (filter) |
| `⊧⥁` | - | Foldable | `⥁`, `⥂` |
| `⊧∈` | - | Membership | `∈`, `∉` |

### Higher-Kinded Traits

| Symbol | Unicode | Meaning | Methods |
|--------|---------|---------|---------|
| `⊧⤇` | - | Functor | `⤇ : (α → β) → F α → F β` |
| `⊧⊛` | - | Applicative | `⊛ : F(α → β) → F α → F β` |
| `⊧≫` | - | Monad | `≫ : F α → (α → F β) → F β` |
| `⊧⊎` | - | Alternative | `⊎ : F α → F α → F α` |
| `⊧⥁` | - | Foldable | Fold structure to value |
| `⊧⊚` | - | Traversable | Map with effects |

---

## Operators & Combinators

### Function Composition

| Symbol | Unicode | Meaning | Type |
|--------|---------|---------|------|
| `∘` | U+2218 | Compose | `(β → γ) → (α → β) → (α → γ)` |
| `⊳` | U+22B3 | Forward compose | `(α → β) → (β → γ) → (α → γ)` |
| `⊛` | U+229B | Apply | `(α → β) → α → β` |
| `⋘` | U+22D8 | Left pipe | `α → (α → β) → β` |
| `⋙` | U+22D9 | Right pipe | `(α → β) → α → β` |

### Combinators (SKI calculus)

| Symbol | Unicode | Meaning | Type |
|--------|---------|---------|------|
| `𝕀` | U+1D540 | Identity | `α → α` |
| `𝕂` | U+1D542 | Constant | `α → β → α` |
| `𝕊` | U+1D54A | Substitution | `(α → β → γ) → (α → β) → α → γ` |
| `𝔹` | U+1D539 | Boolean comb | `α → α → 𝔹 → α` |
| `ℂ` | U+2102 | Flip | `(α → β → γ) → β → α → γ` |
| `𝕎` | U+1D54E | Duplicate | `(α → α → β) → α → β` |

### Fixed Point

| Symbol | Unicode | Meaning | Type |
|--------|---------|---------|------|
| `𝕐` | U+1D550 | Y combinator | `(α → α) → α` |
| `ℤ` | U+2124 | Z combinator | `(α → α) → α` (strict) |
| `⟲` | U+27F2 | Recursion | Already have for trace |

---

## Effects & IO

### Effect Types

| Symbol | Unicode | Meaning | Type |
|--------|---------|---------|------|
| `⟪⟫` | U+27EA/B | Effect block | Already defined |
| `↯` | - | Handle effect | Already defined |
| `⤴` | - | Pure lift | Already defined |
| `≫` | U+226B | Effect bind | Already defined |

### IO Operations

| Symbol | Unicode | Meaning | Type |
|--------|---------|---------|------|
| `⟪◁⟫` | - | Read file | `Path → IO String` |
| `⟪▷⟫` | - | Write file | `Path → String → IO ()` |
| `⟪⊕⟫` | - | Append file | `Path → String → IO ()` |
| `⟪⊙⟫` | - | Read line | `() → IO String` |
| `⟪⊛⟫` | - | Print | `String → IO ()` |
| `⟪⊡⟫` | - | Print line | `String → IO ()` |
| `⟪⚠⟫` | - | Error stream | `String → IO ()` |

### File System

| Symbol | Unicode | Meaning | Type |
|--------|---------|---------|------|
| `⟪∋⟫` | - | File exists | `Path → IO 𝔹` |
| `⟪⊚⟫` | - | Directory list | `Path → IO [Path]` |
| `⟪→⟫` | - | Create file | `Path → IO ()` |
| `⟪×⟫` | - | Delete file | `Path → IO ()` |
| `⟪↔⟫` | - | Move/rename | `Path → Path → IO ()` |
| `⟪⊗⟫` | - | Copy | `Path → Path → IO ()` |

---

## Concurrency & Parallelism

### Actors (Already defined)

| Symbol | Unicode | Meaning | Type |
|--------|---------|---------|------|
| `⟳` | U+27F3 | Spawn actor | Already defined |
| `→!` | - | Send message | Already defined |
| `←?` | - | Receive message | Already defined |

### Additional Concurrency

| Symbol | Unicode | Meaning | Type |
|--------|---------|---------|------|
| `⫴` | U+2AF4 | Parallel zip | `[α] → [β] → [(α,β)]` (parallel) |
| `⫴⤇` | - | Parallel map | `(α → β) → [α] → [β]` (parallel) |
| `⊚⟳` | - | Fork computation | `(() → α) → Future α` |
| `⊚⊙` | - | Await future | `Future α → α` |
| `⊚≫` | - | Future bind | `Future α → (α → Future β) → Future β` |

### Channels

| Symbol | Unicode | Meaning | Type |
|--------|---------|---------|------|
| `⟿→` | - | Send on channel | `Chan α → α → IO ()` |
| `⟿←` | - | Receive from channel | `Chan α → IO α` |
| `⟿⊚` | - | Create channel | `() → IO (Chan α)` |
| `⟿×` | - | Close channel | `Chan α → IO ()` |

---

## Math Operations

### Advanced Arithmetic

| Symbol | Unicode | Meaning | Type |
|--------|---------|---------|------|
| `√` | U+221A | Square root | `ℝ → ℝ` |
| `∛` | U+221B | Cube root | `ℝ → ℝ` |
| `∜` | U+221C | Fourth root | `ℝ → ℝ` |
| `^` | U+005E | Power | `ℝ → ℝ → ℝ` |
| `⌊⌋` | U+230A/B | Floor | `ℝ → ℤ` |
| `⌈⌉` | U+2308/9 | Ceiling | `ℝ → ℤ` |
| `∣∣` | U+2223 | Absolute value | `ℝ → ℝ` |
| `%` | U+0025 | Modulo | `ℤ → ℤ → ℤ` |

### Trigonometry

| Symbol | Unicode | Meaning | Type |
|--------|---------|---------|------|
| `sin` | - | NO! Use `∿` | `ℝ → ℝ` |
| `∿` | U+223F | Sine wave | `ℝ → ℝ` |
| `⌢` | U+2322 | Cosine | `ℝ → ℝ` |
| `⌙` | U+2319 | Tangent | `ℝ → ℝ` |
| `∿⁻¹` | - | Arcsine | `ℝ → ℝ` |
| `⌢⁻¹` | - | Arccosine | `ℝ → ℝ` |
| `⌙⁻¹` | - | Arctangent | `ℝ → ℝ` |

### Logarithms

| Symbol | Unicode | Meaning | Type |
|--------|---------|---------|------|
| `ln` | - | NO! Use `㏑` | `ℝ → ℝ` |
| `㏑` | U+33D1 | Natural log | `ℝ → ℝ` |
| `㏒` | U+33D2 | Log base 10 | `ℝ → ℝ` |
| `㏒ₓ` | - | Log base x | `ℝ → ℝ → ℝ` |
| `e` | - | NO! Use `ℯ` | Euler's number |
| `ℯ` | U+212F | Euler constant | `ℝ` |
| `π` | U+03C0 | Pi | `ℝ` |

---

## Example: Complete Program

### Quicksort (No English!)

```scheme
; ⊼⇅ = quicksort
(≔ ⊼⇅ (λ (⊳ α : (⊧⊴)) (λ (lst)
  (∇ lst
    [∅ ∅]
    [(⟨⟩ ⊙ rest)  ; ⊙ = pivot
     (≔ ◁ (⊻ (λ (x) (⊴ x ⊙)) rest))  ; ◁ = smaller
     (≔ ▷ (⊻ (λ (x) (⊵ x ⊙)) rest))  ; ▷ = larger
     (⊎ (⊼⇅ ◁) (⟨⟩ ⊙ (⊼⇅ ▷)))]))))

; Usage
((⊲ ⊼⇅ ℕ) (⟨⟩ #3 (⟨⟩ #1 (⟨⟩ #4 (⟨⟩ #1 (⟨⟩ #5 ∅))))))
; → (⟨⟩ #1 (⟨⟩ #1 (⟨⟩ #3 (⟨⟩ #4 (⟨⟩ #5 ∅)))))
```

### Binary Search Tree (No English!)

```scheme
; ⊙∋ = tree search
(≔ ⊙∋ (λ (⊳ α : (⊧⊴)) (λ (target tree)
  (∇ tree
    [∅ #f]
    [(⟨⟩ :⊙ val)  ; Leaf
     (≡ target val)]
    [(⟨⟩ :⊚ (⟨⟩ ◁ (⟨⟩ val ▷)))  ; Node
     (? (≡ target val)
        #t
        (? (⊴ target val)
           (⊙∋ target ◁)
           (⊙∋ target ▷)))]))))

; ⊙≔ = tree insert
(≔ ⊙≔ (λ (⊳ α : (⊧⊴)) (λ (item tree)
  (∇ tree
    [∅ (⟨⟩ :⊙ item)]
    [(⟨⟩ :⊙ val)
     (? (⊴ item val)
        (⟨⟩ :⊚ (⟨⟩ (⟨⟩ :⊙ item) (⟨⟩ val ∅)))
        (⟨⟩ :⊚ (⟨⟩ ∅ (⟨⟩ val (⟨⟩ :⊙ item)))))]
    [(⟨⟩ :⊚ (⟨⟩ ◁ (⟨⟩ val ▷)))
     (? (⊴ item val)
        (⟨⟩ :⊚ (⟨⟩ (⊙≔ item ◁) (⟨⟩ val ▷)))
        (⟨⟩ :⊚ (⟨⟩ ◁ (⟨⟩ val (⊙≔ item ▷)))))]))))
```

### Map-Reduce (No English!)

```scheme
; ⤇ = map, ⥁ = fold/reduce
(≔ ⤇⥁ (λ (⊳ α) (λ (⊳ β) (λ (⤇-fn ⥁-fn init lst)
  (⥁ ⥁-fn init (⤇ ⤇-fn lst))))))

; Usage: sum of squares
(((⤇⥁ ℕ) ℕ) (λ (x) (⊗ x x)) ⊕ #0 (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅))))
; → #14
```

---

## Implementation Notes

### How to Remember Symbols

**For humans (documentation only):**
- Create mnemonic images
- ⤇ looks like "flowing through" → map
- ⊻ looks like "sieve" → filter
- ⊼ looks like "ordering" → sort

**For AI:**
- No mnemonics needed
- Learn symbols → behavior mapping
- No linguistic ambiguity

### Symbol Selection Criteria

1. **Visually distinctive** - Easy to recognize at a glance
2. **Mathematically grounded** - Use standard math symbols where applicable
3. **Compositional** - Related operations use related symbols
4. **Unicode standard** - All symbols are valid Unicode
5. **Never English** - Not even abbreviations

---

## Next Steps

1. **Update all examples** in SPEC.md to use symbols
2. **Create symbol lookup tool** for developers
3. **Update implementation plan** with symbolic names
4. **Implement stdlib** with only symbols
5. **Documentation** using symbol → concept mapping

---

**PRINCIPLE: If it can be named in English, it can be symbolized.**

**Guage is for AI. Symbols are universal.**
