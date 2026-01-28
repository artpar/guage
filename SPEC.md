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

## Runtime Primitives (80 Total)

**Status:** 80 primitives implemented (6 placeholders, 74 fully functional + 6 placeholders = 80 total)

### Core Lambda Calculus (3) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⟨⟩` | `α → β → ⟨α β⟩` | Construct cell | ✅ DONE |
| `◁` | `⟨α β⟩ → α` | Head (car) | ✅ DONE |
| `▷` | `⟨α β⟩ → β` | Tail (cdr) | ✅ DONE |

**Note:** `λ`, `·`, `≔`, and De Bruijn indices (0, 1, 2...) are part of the evaluator, not primitives.

### Metaprogramming Core (4) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⌜` | `α → ⌜α⌝` | Quote (code→data) | ✅ DONE |
| `⌞` | `⌜α⌝ → α` | Eval (data→code) | ✅ DONE |
| `⌞̃` | `α → ⌜α⌝` | Quasiquote (template with unquote) | ✅ DONE (Day 32 Part 2) |
| `~` | `α → α` | Unquote (evaluate in quasiquote) | ✅ DONE (Day 32 Part 2) |

### Pattern Matching (1) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `∇` | `α → [[⌜pattern⌝ result]] → β` | Pattern match expression | ✅ DONE (Day 16) |

**Note:** As of Day 19, supports:
- **Wildcard** (_) - matches anything
- **Literals** - numbers, booleans, symbols, keywords
- **Variables** - bind matched value to name (Day 16 ✅)
- **Pair patterns** - destructure pairs (Day 17 ✅)
- **Leaf structure patterns** (⊙) - destructure simple structures (Day 18 ✅)
- **Node/ADT patterns** (⊚) - destructure algebraic data types (Day 18 ✅)
- **Exhaustiveness checking** - warnings for incomplete/unreachable patterns (Day 19 ✅)

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
- `(⟨⟩ pat1 pat2)` - Pair destructuring (recursive matching)
- `(⊙ :Type pat1 pat2 ...)` - Leaf structure destructuring (Day 18)
- `(⊚ :Type :Variant pat1 ...)` - Node/ADT destructuring (Day 18)

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

; Quasiquote and Unquote (Day 32 Part 2)
(≔ x #42)
(⌞̃ (⊕ #1 (~ x)))  ; → (⊕ #1 #42) - unquote evaluates x

; Build expressions programmatically
(≔ make-add (λ (a) (λ (b) (⌞̃ (⊕ (~ a) (~ b))))))
((make-add #3) #4)  ; → (⊕ #3 #4)

; Pair patterns (Day 17)
(∇ (⟨⟩ #1 #2) (⌜ (((⟨⟩ x y) (⊕ x y)))))  ; → #3

; Nested pairs
(∇ (⟨⟩ (⟨⟩ #1 #2) #3) (⌜ (((⟨⟩ (⟨⟩ a b) c) (⊕ a (⊕ b c))))))  ; → #6

; List patterns
(∇ (⟨⟩ #42 ∅) (⌜ (((⟨⟩ x ∅) x))))  ; → #42 (single-element list)
(∇ (⟨⟩ #3 (⟨⟩ #4 ∅)) (⌜ (((⟨⟩ x (⟨⟩ y ∅)) (⊕ x y)))))  ; → #7

; Leaf structure patterns (Day 18)
(⊙≔ :Point :x :y)
(≔ p (⊙ :Point #3 #4))
(∇ p (⌜ (((⊙ :Point x y) (⊕ x y)))))  ; → #7

; Nested leaf structures
(⊙≔ :Line :start :end)
(≔ line (⊙ :Line p1 p2))
(∇ line (⌜ (((⊙ :Line (⊙ :Point x1 y1) (⊙ :Point x2 y2))
              (⊕ (⊕ x1 y1) (⊕ x2 y2))))))  ; → #37

; Node/ADT patterns (Day 18)
(⊚≔ :Option (⌜ (:None)) (⌜ (:Some :value)))
(≔ some-42 (⊚ :Option :Some #42))
(≔ none (⊚ :Option :None))

; Match ADT variants
(∇ some-42 (⌜ (((⊚ :Option :Some v) v))))  ; → #42
(∇ none (⌜ (((⊚ :Option :None) :empty))))  ; → :empty

; Multiple clauses with ADT
(∇ none (⌜ (((⊚ :Option :Some v) v)
            ((⊚ :Option :None) #99))))  ; → #99

; Recursive ADT (List)
(⊚≔ :List (⌜ (:Nil)) (⌜ (:Cons :head :tail)))
(≔ lst (⊚ :List :Cons #1 (⊚ :List :Cons #2 (⊚ :List :Nil))))

; Nested ADT patterns
(∇ lst (⌜ (((⊚ :List :Cons h1 (⊚ :List :Cons h2 t)) h2))))  ; → #2

; Binary tree ADT
(⊚≔ :Tree (⌜ (:Leaf :value)) (⌜ (:Node :left :value :right)))
(≔ tree (⊚ :Tree :Node
         (⊚ :Tree :Leaf #5)
         #10
         (⊚ :Tree :Leaf #15)))

; Extract from nested tree
(∇ tree (⌜ (((⊚ :Tree :Node (⊚ :Tree :Leaf lv) v (⊚ :Tree :Leaf rv))
             (⊕ lv rv)))))  ; → #20
```

**Exhaustiveness Checking (Day 19):**

The pattern matcher emits warnings (not errors) to help catch incomplete or redundant patterns:

**Incomplete pattern warnings:**
```scheme
; ⚠️ Warning: Pattern match may be incomplete
; → Matching value of type: number (infinite domain)
(∇ #42 (⌜ ((#42 :matched))))  ; Missing catch-all for other numbers
```

**Unreachable pattern warnings:**
```scheme
; ⚠️ Warning: Unreachable pattern detected
; → Pattern at position 2 will never match
(∇ #42 (⌜ ((_ :any) (#42 :specific))))  ; #42 pattern is unreachable
```

**Complete patterns (no warnings):**
```scheme
; Wildcard covers all cases
(∇ #42 (⌜ ((#42 :specific) (_ :other))))  ; ✓ Complete

; Variable covers all cases
(∇ #42 (⌜ ((#42 :specific) (x x))))  ; ✓ Complete
```

The warnings help identify:
- **Missing cases** that could cause runtime `:no-match` errors
- **Dead code** from unreachable patterns
- **Incomplete ADT handling** when not all variants are covered

Warnings are non-fatal and do not stop execution.

---

### Quasiquote and Unquote (Day 32 Part 2) ✅

**Quasiquote** (`⌞̃`) is like quote but supports **unquote** (`~`) for selective evaluation. This enables code templating and macro construction.

**Basic Usage:**
```scheme
; Without unquote - acts like quote
(⌞̃ (⊕ #1 #2))  ; → (⊕ #1 #2)

; With unquote - evaluates marked parts
(≔ x #42)
(⌞̃ (⊕ #1 (~ x)))  ; → (⊕ #1 #42)
```

**Multiple Unquotes:**
```scheme
(≔ a #10)
(≔ b #20)
(⌞̃ (⊕ (~ a) (~ b)))  ; → (⊕ #10 #20)
```

**Code Templates:**
```scheme
; Build expressions programmatically
(≔ make-add (λ (a) (λ (b)
  (⌞̃ (⊕ (~ a) (~ b))))))

((make-add #3) #4)  ; → (⊕ #3 #4)
```

**Macro-Like Usage:**
```scheme
; Conditional builder
(≔ make-gt (λ (a) (λ (b)
  (⌞̃ (> (~ a) (~ b))))))

(⌞ ((make-gt #10) #5))  ; → #t
```

**Key Features:**
- ✅ **Selective evaluation** - Only unquoted parts evaluated
- ✅ **Code as data** - Build expressions programmatically
- ✅ **Template functions** - Create code generators
- ✅ **Macro foundation** - Enables macro system (Day 33)

---

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
| `⌂⊛` | `:symbol → ⊙` | Get provenance metadata | ✅ DONE |
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

### String Operations (9) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `≈` | `α → ≈` | Convert value to string | ✅ DONE |
| `≈⊕` | `≈ → ≈ → ≈` | Concatenate strings | ✅ DONE |
| `≈#` | `≈ → ℕ` | String length | ✅ DONE |
| `≈→` | `≈ → ℕ → :symbol` | Character at index | ✅ DONE |
| `≈⊂` | `≈ → ℕ → ℕ → ≈` | Substring (start, end) | ✅ DONE |
| `≈?` | `α → 𝔹` | Is string? | ✅ DONE |
| `≈∅?` | `≈ → 𝔹` | Is empty string? | ✅ DONE |
| `≈≡` | `≈ → ≈ → 𝔹` | String equality | ✅ DONE |
| `≈<` | `≈ → ≈ → 𝔹` | String ordering (lexicographic) | ✅ DONE |

**String Literals:**
Strings are enclosed in double quotes with escape sequences:
```scheme
"hello"           ; Basic string
"hello world"     ; String with spaces
"with\nnewline"   ; Escape sequences: \n \t \r \\ \"
""                ; Empty string
```

**Examples:**
```scheme
(≈ #42)                      ; → "42"
(≈⊕ "hello" " world")        ; → "hello world"
(≈# "test")                  ; → #4
(≈→ "hello" #0)              ; → :h
(≈⊂ "hello world" #0 #5)     ; → "hello"
(≈? "test")                  ; → #t
(≈∅? "")                     ; → #t
(≈≡ "hello" "hello")         ; → #t
(≈< "apple" "banana")        ; → #t
```

**Higher-Level String Library:**

The `stdlib/string.scm` module provides higher-level string manipulation utilities built on the primitive operations above:

| Function | Alias | Description |
|----------|-------|-------------|
| `string-split` | `≈÷` | Split string by delimiter or into characters |
| `string-join` | `≈⊗` | Join list of strings with delimiter |
| `string-trim` | `≈⊏⊐` | Trim whitespace from both ends |
| `string-contains?` | `≈∈?` | Check if substring exists |
| `string-replace` | `≈⇔` | Replace all occurrences |
| `string-split-lines` | `≈÷⊳` | Split by newlines |
| `string-index-of` | `≈⊳` | Find substring position (or ∅) |

**Examples:**
```scheme
(⋘ "stdlib/string.scm")

; Split and join
(string-split "a,b,c" ",")          ; → ⟨"a" ⟨"b" ⟨"c" ∅⟩⟩⟩
(≈÷ "hello" "")                     ; → ⟨"h" ⟨"e" ⟨"l" ⟨"l" ⟨"o" ∅⟩⟩⟩⟩⟩
(string-join ⟨"a" ⟨"b" ∅⟩⟩ ",")     ; → "a,b"

; Trim whitespace
(string-trim "  hello  ")           ; → "hello"
(≈⊏⊐ "\n\ttest\t\n")                ; → "test"

; Search and replace
(string-contains? "hello world" "world")  ; → #t
(≈∈? "test" "xyz")                       ; → #f
(string-index-of "hello world" "world")  ; → #6
(≈⊳ "test" "xyz")                        ; → ∅
(string-replace "hello world" "world" "there")  ; → "hello there"
(≈⇔ "aaa" "a" "b")                       ; → "bbb"

; Real-world usage
(string-split-lines "a\nb\nc")      ; → ⟨"a" ⟨"b" ⟨"c" ∅⟩⟩⟩
(≈÷ "Alice,30,Engineer" ",")        ; → CSV parsing
(≈⊗ words " ")                      ; → Sentence building
```

See `bootstrap/tests/string.test` for comprehensive examples (42 passing tests).

### I/O Operations (8) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `≋` | `α → α` | Print value to stdout with newline | ✅ DONE |
| `≋≈` | `≈ → ≈` | Print string without newline | ✅ DONE |
| `≋←` | `() → ≈` | Read line from stdin | ✅ DONE |
| `≋⊳` | `≈ → ≈` | Read entire file as string | ✅ DONE |
| `≋⊲` | `≈ → ≈ → ≈` | Write string to file (overwrites) | ✅ DONE |
| `≋⊕` | `≈ → ≈ → ≈` | Append string to file | ✅ DONE |
| `≋?` | `≈ → 𝔹` | Check if file exists | ✅ DONE |
| `≋∅?` | `≈ → 𝔹` | Check if file is empty | ✅ DONE |

**Console I/O:**
```scheme
; Print with newline
(≋ "Hello, world!")          ; → "Hello, world!" (and prints)
(≋ #42)                      ; → #42 (and prints "42")

; Print without newline
(≋≈ "Name: ")                ; → "Name: " (no newline)

; Read from console (interactive)
; (≋←)                       ; → string from stdin
```

**File I/O:**
```scheme
; Write to file
(≋⊲ "test.txt" "content")    ; → "test.txt" (file created/overwritten)

; Read from file
(≋⊳ "test.txt")              ; → "content"

; Append to file
(≋⊕ "test.txt" " more")      ; → "test.txt"
(≋⊳ "test.txt")              ; → "content more"

; File predicates
(≋? "test.txt")              ; → #t (file exists)
(≋∅? "test.txt")             ; → #f (not empty)
```

**Real-world Example:**
```scheme
; Logging system
(≔ log (λ (msg)
  (≋⊕ "app.log" (≈⊕ msg "\n"))))

(log "Application started")
(log "Processing data...")
(log "Application stopped")

; Safe file read
(≔ safe-read (λ (path) (λ (default)
  (? (≋? path)
     (≋⊳ path)
     default))))

((safe-read "config.txt") "default config")
```

**Error Handling:**
All I/O operations return errors on failure:
- `≋⊳` - Returns `:file-not-found` if file doesn't exist
- `≋⊲` - Returns `:file-write-error` on write failure
- `≋⊕` - Returns `:file-append-error` on append failure
- `≋←` - Returns `:read-error` on stdin error

**Technical Details:**
- All I/O is synchronous (blocking)
- Files are opened, operated on, and closed immediately
- File paths must be strings
- No file locking or concurrent access control
- UTF-8 encoding assumed

---

### Module System (4) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⋘` | `≈ → α` | Load and evaluate file | ✅ DONE |
| `⌂⊚` | `() → [≈]` / `:α → ≈` / `≈ → [:α]` | Module information / provenance | ✅ DONE |
| `⋖` | `≈ → [:α] → :ok | ⚠` | Validate symbols exist in module | ✅ DONE |
| `⌂⊚→` | `≈ → [≈]` | Get module dependencies | ✅ DONE |

**Basic Load:**
```scheme
; Create a module file
(≋⊲ "math.scm" "(≔ double (λ (n) (⊗ n #2)))")

; Load and evaluate the module
(⋘ "math.scm")                ; → result of last expression

; Use the loaded function
(double #21)                   ; → #42
```

**Multiple Definitions:**
```scheme
; Module with multiple definitions
(≔ stdlib "(≔ inc (λ (n) (⊕ n #1))) (≔ dec (λ (n) (⊖ n #1)))")
(≋⊲ "stdlib.scm" stdlib)

; Load all definitions
(⋘ "stdlib.scm")

; All functions available
(inc #5)                       ; → #6
(dec #10)                      ; → #9
```

**Module Dependencies:**
```scheme
; base-module.scm
(≋⊲ "base.scm" "(≔ PI #3.14159)")

; derived-module.scm
(≋⊲ "derived.scm" "(≔ area (λ (r) (⊗ PI (⊗ r r))))")

; Load in order
(⋘ "base.scm")                 ; Defines PI
(⋘ "derived.scm")              ; Uses PI

(area #5)                      ; → #78.53975
```

**Standard Library Usage:**
```scheme
; Load standard library modules
(⋘ "stdlib/list.scm")          ; List operations
(⋘ "stdlib/option.scm")        ; Option/Result types
(⋘ "stdlib/string.scm")        ; String manipulation
(⋘ "stdlib/math.scm")          ; Math utilities

; Use loaded functions
(map inc (list #1 #2 #3))      ; → ⟨#2 ⟨#3 ⟨#4 ∅⟩⟩⟩
(string-split "a,b,c" ",")     ; → ⟨"a" ⟨"b" ⟨"c" ∅⟩⟩⟩
```

**Error Handling:**
```scheme
; File not found
(⋘ "missing.scm")              ; → ⚠:file-not-found
(⚠? (⋘ "missing.scm"))         ; → #t

; Invalid argument
(⋘ #42)                        ; → ⚠ error

; Safe loading with fallback
(≔ safe-load (λ (path) (λ (default)
  (? (≋? path)
     (? (⚠? (⋘ path))
        default
        #t)
     default))))

((safe-load "config.scm") #f)  ; → #f if file missing/error
```

**Module Registry (⌂⊚):**
```scheme
; Load a module
(⋘ "math.scm")                  ; Defines square, cube, etc.

; List all loaded modules
(⌂⊚)                            ; → ⟨"math.scm" ∅⟩

; Find which module defines a symbol
(⌂⊚ :square)                    ; → "math.scm"
(⌂⊚ :undefined-symbol)          ; → ⚠:symbol-not-in-any-module

; List all symbols from a module
(⌂⊚ "math.scm")                 ; → ⟨:square ⟨:cube ⟨:double ∅⟩⟩⟩
(⌂⊚ "nonexistent.scm")          ; → ∅

; Check symbol provenance
(≔ check-source (λ (sym)
  (? (⚠? (⌂⊚ sym))
     (⌜ :builtin)
     (⌂⊚ sym))))

(check-source :square)          ; → "math.scm"
(check-source :⊕)               ; → :builtin
```

**Enhanced Provenance (⌂⊛) - Day 27:**
```scheme
; Load a module
(⋘ "math.scm")

; Get full provenance for a symbol
(⌂⊛ :square)
; → ⊙[::Provenance ⟨⟨::module "math.scm"⟩
;                    ⟨⟨::line #0⟩
;                     ⟨⟨::load-order #1⟩
;                      ⟨⟨::defined-at #1737584932⟩ ∅⟩⟩⟩⟩]

; Access provenance fields
(≔ prov (⌂⊛ :square))
(⊙→ prov :module)              ; → "math.scm"
(⊙→ prov :line)                ; → #0 (line number, currently 0)
(⊙→ prov :load-order)          ; → #1 (first module loaded)
(⊙→ prov :defined-at)          ; → #1737584932 (Unix timestamp)

; Primitives return simple provenance
(⌂⊛ :⊕)
; → ⊙[::Provenance ⟨⟨::module "<primitive>"⟩ ∅⟩]

; REPL-defined functions show <repl> module (Day 43)
(≔ double (λ (x) (⊗ x #2)))
(⌂⊛ :double)
; → ⊙[::Provenance ⟨⟨::module "<repl>"⟩
;                    ⟨⟨::line #0⟩
;                     ⟨⟨::load-order #1⟩
;                      ⟨⟨::defined-at #1769584932⟩ ∅⟩⟩⟩⟩]

; Undefined symbols return error
(⌂⊛ :nonexistent)              ; → ⚠:symbol-not-found
```

**Provenance Structure Fields:**
- **:module** (string) - Module file path, "<primitive>", or "<repl>"
- **:line** (number) - Line number in source (currently 0, parser enhancement pending)
- **:load-order** (number) - Sequential module load number (1, 2, 3...)
- **:defined-at** (number) - Unix timestamp when module was loaded

**Use Cases:**
```scheme
; Find oldest loaded module
(≔ find-oldest (λ (symbols)
  (⊳ (⊠ (λ (a b)
          (< (⊙→ (⌂⊛ a) :load-order)
             (⊙→ (⌂⊛ b) :load-order)))
        symbols))))

; Check if symbol is from standard library
(≔ from-stdlib? (λ (sym)
  (≈⊂ "stdlib/" (⊙→ (⌂⊛ sym) :module))))

; Get load time difference between modules
(≔ load-gap (λ (sym1 sym2)
  (⊖ (⊙→ (⌂⊛ sym2) :defined-at)
     (⊙→ (⌂⊛ sym1) :defined-at))))
```

**Selective Import (⋖) - Day 28:**
```scheme
; Load a module
(⋘ "math.scm")                  ; Defines square, cube, double

; Validate single symbol exists in module
(⋖ "math.scm" (⟨⟩ :square ∅))  ; → :ok

; Validate multiple symbols
(⋖ "math.scm" (⟨⟩ :square (⟨⟩ :cube ∅)))  ; → :ok

; All symbols must exist
(⋖ "math.scm" (⟨⟩ :square (⟨⟩ :nonexistent ∅)))  ; → ⚠:symbol-not-in-module

; Module must be loaded first
(⋖ "never_loaded.scm" (⟨⟩ :foo ∅))  ; → ⚠:module-not-loaded

; Empty list is valid (vacuous truth)
(⋖ "math.scm" ∅)               ; → :ok

; Use for safe importing
(≔ safe-import (λ (module symbols)
  (? (⚠? (⋖ module symbols))
     (⚠ :import-failed module)
     :ok)))

(safe-import "math.scm" (⟨⟩ :square ∅))  ; → :ok
```

**Selective Import Features:**
- Validates symbols exist before use (documentation + validation)
- Module must be loaded first (use ⋘)
- Returns :ok on success, error on failure
- All symbols must exist (no partial validation)
- Transparent: doesn't restrict access, only validates

**Module Dependencies (⌂⊚→) - Day 29:**
```scheme
; Create module dependencies
(≋⊲ "base.scm" "(≔ BASE #10)")
(≋⊲ "derived.scm" "(⋘ \"base.scm\") (≔ DERIVED (⊕ BASE #1))")

; Load derived module (automatically tracks dependency)
(⋘ "derived.scm")

; Query module dependencies
(⌂⊚→ "derived.scm")           ; → ⟨"base.scm" ∅⟩

; Independent module has no dependencies
(≋⊲ "independent.scm" "(≔ INDEP #42)")
(⋘ "independent.scm")
(⌂⊚→ "independent.scm")       ; → ∅

; Transitive dependencies NOT included (only direct loads)
(≋⊲ "c.scm" "(≔ C #3)")
(≋⊲ "b.scm" "(⋘ \"c.scm\") (≔ B (⊕ C #2))")
(≋⊲ "a.scm" "(⋘ \"b.scm\") (≔ A (⊕ B #1))")
(⋘ "a.scm")
(⌂⊚→ "a.scm")                 ; → ⟨"b.scm" ∅⟩  (NOT ⟨"b.scm" ⟨"c.scm" ∅⟩⟩)

; Error on unloaded module
(⌂⊚→ "never_loaded.scm")      ; → ⚠:module-not-loaded
```

**Dependency Tracking Features:**
- Automatic tracking when module loads another via ⋘
- Only direct dependencies recorded (not transitive)
- No self-dependencies (module doesn't depend on itself)
- Dependencies stored as list of module paths (strings)
- Works with full module system transparency

**Module Registry Features:**
- Every loaded file is automatically registered
- Symbols defined during load are tracked
- Provenance: know which module defines each symbol
- Transparency: all modules and symbols are queryable
- No information hiding (first design)

**How It Works:**
1. Reads entire file into memory
2. Parses all expressions sequentially
3. Evaluates each expression in current environment
4. Returns result of last expression
5. All definitions become available in current scope

**Technical Details:**
- Files are evaluated in **current environment** (no isolation)
- All definitions are global (added to current scope)
- Module can redefine existing variables
- No circular dependency detection
- No caching (loading twice evaluates twice)
- Return value is the last expression in the file

**Limitations (Current Implementation):**
- No namespace isolation
- No explicit imports/exports
- No dependency tracking (planned for Day 28-29)
- Parse errors may crash (needs improvement)
- Line numbers currently not tracked by parser (always 0)

**Future Enhancements:**
- `⊞◇` (module-define) - Define module with explicit exports
- `⊞⊳` (module-import) - Import specific symbols
- Module registry to prevent double-loading
- Namespace isolation
- Dependency resolution

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

This enables assisted development where the compiler helps you write, prove, test, optimize, and deploy code.

**Implementation Timeline:**
- Phase 2C: Data structures (CURRENT)
- Phase 3: Pattern matching, macros, generics (18 weeks)
- Phase 4: Self-hosting, type system (12 weeks)
- Phase 5: Advanced metaprogramming (36 weeks)
- Phase 6: Distribution and analysis (24 weeks)
- **Total:** ~21 months to full vision

See `METAPROGRAMMING_VISION.md` for detailed specifications and implementation strategy.
