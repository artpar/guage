---
Status: CURRENT
Created: 2025-12-01
Updated: 2026-01-30
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

## Runtime Primitives (165 Total)

**Status:** 165 primitives implemented and stable (94/94 test files passing)
**Note:** All primitives fully working including graph algorithms, actors, channels, supervision, supervisors, registry, and timers

### Core Lambda Calculus (3) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⟨⟩` | `α → β → ⟨α β⟩` | Construct cell | ✅ DONE |
| `◁` | `⟨α β⟩ → α` | Head (car) | ✅ DONE |
| `▷` | `⟨α β⟩ → β` | Tail (cdr) | ✅ DONE |

**Note:** `λ`, `·`, `≔`, and De Bruijn indices (0, 1, 2...) are part of the evaluator, not primitives.

### Metaprogramming Core (5) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⌜` | `α → ⌜α⌝` | Quote (code→data) | ✅ DONE |
| `⌞` | `⌜α⌝ → α` | Eval (data→code) | ✅ DONE |
| `⊡` | `(α → β) → [α] → β` | Apply primitive to argument list | ✅ DONE (Day 65) |
| `⌞̃` | `α → ⌜α⌝` | Quasiquote (template with unquote) | ✅ DONE (Day 32 Part 2) |
| `~` | `α → α` | Unquote (evaluate in quasiquote) | ✅ DONE (Day 32 Part 2) |

### Pattern Matching (1) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `∇` | `α → [[⌜pattern⌝ result]] → β` | Pattern match expression | ✅ DONE (Day 16) |

**Note:** As of Day 66, supports:
- **Wildcard** (_) - matches anything
- **Literals** - numbers, booleans, symbols, keywords
- **Variables** - bind matched value to name (Day 16 ✅)
- **Pair patterns** - destructure pairs (Day 17 ✅)
- **Leaf structure patterns** (⊙) - destructure simple structures (Day 18 ✅)
- **Node/ADT patterns** (⊚) - destructure algebraic data types (Day 18 ✅)
- **Exhaustiveness checking** - warnings for incomplete/unreachable patterns (Day 19 ✅)
- **Guard conditions** - conditional matching with boolean expressions (Day 58 ✅)
- **As-patterns** - bind both whole value and parts (Day 59 ✅)
- **Or-patterns** - match multiple alternatives (Day 60 ✅)
- **View patterns** - transform before matching (Day 66 ✅)

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
- `(pattern | guard-expr)` - Guard condition (Day 58)
- `(name @ pattern)` - As-pattern, binds both whole and parts (Day 59)
- `(∨ pat1 pat2 ...)` - Or-pattern, match alternatives (Day 60)
- `(→ transform pattern)` - View pattern, transform before matching (Day 66)

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

; Guard conditions (Day 58) - conditional matching
; Syntax: (pattern | guard-expr) result-expr
; Guard is evaluated after pattern matches; if #t, use this clause; if #f, try next

; Match positive numbers
(∇ #5 (⌜ (((n | (> n #0)) :positive) (_ :non-positive))))  ; → :positive
(∇ #-3 (⌜ (((n | (> n #0)) :positive) (_ :non-positive))))  ; → :non-positive

; Complex boolean guards - positive even numbers
(∇ #10 (⌜ (((n | (∧ (> n #0) (≡ (% n #2) #0))) :positive-even)
          ((n | (> n #0)) :positive-odd)
          (_ :other))))  ; → :positive-even

; Guards with pattern bindings - uses bound variables
(∇ #15 (⌜ (((x | (> x #10)) (⊕ x #100)) (_ #0))))  ; → #115

; Guards with pair patterns
(∇ (⟨⟩ #3 #4) (⌜ ((((⟨⟩ a b) | (≡ (⊕ a b) #7)) :sum-seven)
                  ((⟨⟩ a b) :other))))  ; → :sum-seven

; Guards with range checks
(∇ #50 (⌜ (((n | (∧ (≥ n #0) (≤ n #100))) :in-range) (_ :out-of-range))))  ; → :in-range

; Guards with ADT patterns
(⊚≔ :Result (⌜ (:Ok :value)) (⌜ (:Err :error)))
(∇ (⊚ :Result :Ok #150) (⌜ ((((⊚ :Result :Ok v) | (> v #100)) :large)
                            ((⊚ :Result :Ok v) :small)
                            ((⊚ :Result :Err e) :error))))  ; → :large

; As-patterns (Day 59) - bind both whole value AND parts
; Syntax: (name @ pattern)
; Binds 'name' to the whole value and also matches the subpattern

; Bind pair and its components
(∇ (⟨⟩ #1 #2) (⌜ (((pair @ (⟨⟩ a b)) (⟨⟩ pair (⟨⟩ a b))))))
; → ⟨⟨#1 #2⟩ ⟨#1 #2⟩⟩
; pair = ⟨#1 #2⟩, a = #1, b = #2

; Bind Result.Ok and its value
(⊚≔ :Result (⌜ (:Ok :value)) (⌜ (:Err :error)))
(∇ (⊚ :Result :Ok #42) (⌜ (((ok @ (⊚ :Result :Ok v)) (⟨⟩ ok v)))))
; → ⟨⊚[:Result :Ok #42] #42⟩

; Nested as-patterns
(∇ (⟨⟩ #5 #6) (⌜ (((outer @ (inner @ (⟨⟩ a b))) (⟨⟩ outer inner)))))
; → ⟨⟨#5 #6⟩ ⟨#5 #6⟩⟩
; outer = ⟨#5 #6⟩, inner = ⟨#5 #6⟩, a = #5, b = #6

; Clone a list node with as-pattern
(∇ (⟨⟩ #42 (⟨⟩ #99 ∅)) (⌜ (((node @ (⟨⟩ h t)) (⟨⟩ h node)))))
; → ⟨#42 ⟨#42 ⟨#99 ∅⟩⟩⟩

; As-patterns combined with guards
(∇ (⟨⟩ #5 #10) (⌜ ((((pair @ (⟨⟩ a b)) | (> a #0)) pair)
                   (_ :failed))))  ; → ⟨#5 #10⟩

; Or-patterns (Day 60) - match multiple alternatives (first match wins)
; Syntax: (∨ pattern₁ pattern₂ pattern₃ ...)
; Important: All alternatives MUST bind the same variables (or none)
; This is standard in OCaml and Rust

; Match multiple literal values
(∇ #1 (⌜ (((∨ #0 #1 #2) :small) (_ :other))))  ; → :small

; Match multiple symbols
(∇ :blue (⌜ (((∨ :red :green :blue) :primary) (_ :other))))  ; → :primary

; Match multiple ADT variants
(⊚≔ :Option (⌜ (:None)) (⌜ (:Some :value)))
(∇ (⊚ :Option :None) (⌜ (((∨ (⊚ :Option :None) (⊚ :Option :Some #42)) :matched)
                          (_ :other))))  ; → :matched

; Or-patterns with variables (both must bind same variables)
(⊚≔ :Result (⌜ (:Ok :value)) (⌜ (:Err :error)))
(∇ (⊚ :Result :Ok #42) (⌜ (((∨ (⊚ :Result :Ok v) (⊚ :Result :Err v)) v)
                            (_ :other))))  ; → #42

; Nested or-patterns
(∇ #1 (⌜ (((∨ (∨ #0 #1) #2) :matched) (_ :other))))  ; → :matched

; Or-patterns with guards
(∇ #42 (⌜ ((((∨ x x) | (> x #0)) x) (_ :failed))))  ; → #42

; Or-patterns combined with as-patterns
(∇ #1 (⌜ (((whole @ (∨ #0 #1 #2)) (⟨⟩ whole whole))
           (_ :other))))  ; → ⟨#1 #1⟩

; View patterns (Day 66) - transform value before matching
; Syntax: (→ transform pattern)
; Applies transform to value, then matches result against pattern
; If transform returns error, pattern doesn't match

; Match on list length
(⋘ "bootstrap/stdlib/list.scm")  ; Loads # (length) function
(∇ (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅))) (⌜ (((→ # #3) :length-three) (_ :other))))  ; → :length-three

; Match on absolute value
(≔ abs (λ (x) (? (< x #0) (⊖ #0 x) x)))
(∇ #-5 (⌜ (((→ abs #5) :matched) (_ :failed))))  ; → :matched

; Bind transformed value to variable
(∇ (⟨⟩ #1 (⟨⟩ #2 ∅)) (⌜ (((→ # len) len) (_ #0))))  ; → #2

; Combined with as-patterns - bind both original and transformed
(∇ (⟨⟩ #1 (⟨⟩ #2 ∅)) (⌜ (((original @ (→ # #2)) (⟨⟩ original #2))
                         (_ :failed))))  ; → ⟨⟨#1 ⟨#2 ∅⟩⟩ #2⟩

; Combined with guards - transform then guard
(∇ #-15 (⌜ ((((→ abs n) | (> n #10)) :large) (_ :small))))  ; → :large

; Multiple view patterns in same match
(∇ (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅))) (⌜ (((→ # #5) :length-five)
                                 ((→ # #3) :length-three)
                                 (_ :other))))  ; → :length-three

; Nested view patterns (transforms composed)
(≔ double (λ (n) (⊗ n #2)))
(≔ inc (λ (n) (⊕ n #1)))
(∇ #5 (⌜ (((→ double (→ inc #11)) :nested) (_ :failed))))  ; → :nested

; Error handling - if transform returns error, pattern fails
(≔ failing (λ (x) (⚠ :error x)))
(∇ #-5 (⌜ (((→ failing #5) :matched) (_ :failed))))  ; → :failed

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

### Arithmetic (10) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⊕` | `ℕ → ℕ → ℕ` | Addition | ✅ DONE |
| `⊖` | `ℕ → ℕ → ℕ` | Subtraction | ✅ DONE |
| `⊗` | `ℕ → ℕ → ℕ` | Multiplication | ✅ DONE |
| `⊘` | `ℕ → ℕ → ℕ` | Division (float) | ✅ DONE |
| `÷` | `ℕ → ℕ → ℕ` | Integer division (floor) | ✅ DONE |
| `%` | `ℕ → ℕ → ℕ` | Modulo (remainder) | ✅ DONE |
| `<` | `ℕ → ℕ → 𝔹` | Less than | ✅ DONE |
| `>` | `ℕ → ℕ → 𝔹` | Greater than | ✅ DONE |
| `≤` | `ℕ → ℕ → 𝔹` | Less or equal | ✅ DONE |
| `≥` | `ℕ → ℕ → 𝔹` | Greater or equal | ✅ DONE |

### Math Operations (22) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `√` | `ℕ → ℕ` | Square root | ✅ DONE |
| `^` | `ℕ → ℕ → ℕ` | Power (exponentiation) | ✅ DONE |
| `\|` | `ℕ → ℕ` | Absolute value | ✅ DONE |
| `⌊⌋` | `ℕ → ℕ` | Floor (round down) | ✅ DONE |
| `⌈⌉` | `ℕ → ℕ` | Ceiling (round up) | ✅ DONE |
| `⌊⌉` | `ℕ → ℕ` | Round (nearest integer) | ✅ DONE |
| `min` | `ℕ → ℕ → ℕ` | Minimum of two numbers | ✅ DONE |
| `max` | `ℕ → ℕ → ℕ` | Maximum of two numbers | ✅ DONE |
| `sin` | `ℕ → ℕ` | Sine (radians) | ✅ DONE |
| `cos` | `ℕ → ℕ` | Cosine (radians) | ✅ DONE |
| `tan` | `ℕ → ℕ` | Tangent (radians) | ✅ DONE |
| `asin` | `ℕ → ℕ` | Arcsine (radians) | ✅ DONE |
| `acos` | `ℕ → ℕ` | Arccosine (radians) | ✅ DONE |
| `atan` | `ℕ → ℕ` | Arctangent (radians) | ✅ DONE |
| `atan2` | `ℕ → ℕ → ℕ` | Two-arg arctangent | ✅ DONE |
| `log` | `ℕ → ℕ` | Natural logarithm | ✅ DONE |
| `log10` | `ℕ → ℕ` | Base-10 logarithm | ✅ DONE |
| `exp` | `ℕ → ℕ` | Exponential (e^x) | ✅ DONE |
| `π` | `() → ℕ` | Pi constant | ✅ DONE |
| `e` | `() → ℕ` | Euler's number | ✅ DONE |
| `rand` | `() → ℕ` | Random [0,1) | ✅ DONE |
| `rand-int` | `ℕ → ℕ` | Random integer [0,n) | ✅ DONE |

**Examples:**
```scheme
;; Basic math
(√ #16)                          ; → #4
(^ #2 #8)                        ; → #256
(| #-42)                         ; → #42
(min #5 #3)                      ; → #3
(max #5 #3)                      ; → #5

;; Constants
(π)                              ; → #3.14159...
(e)                              ; → #2.71828...

;; Trigonometry
(sin (π))                        ; → ~0
(cos (π))                        ; → #-1
(tan (⊘ (π) #4))                 ; → ~1

;; Logarithms
(log (e))                        ; → #1
(log10 #100)                     ; → #2
(exp #2)                         ; → ~7.389

;; Random
(rand)                           ; → random in [0,1)
(rand-int #10)                   ; → random in [0,10)

;; Pythagorean theorem
(≔ hypotenuse (λ (a) (λ (b)
  (√ (⊕ (^ a #2) (^ b #2))))))
((hypotenuse #3) #4)             ; → #5
```

### Type Predicates (6) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `ℕ?` | `α → 𝔹` | Is number | ✅ DONE |
| `𝔹?` | `α → 𝔹` | Is boolean | ✅ DONE |
| `:?` | `α → 𝔹` | Is symbol | ✅ DONE |
| `∅?` | `α → 𝔹` | Is nil | ✅ DONE |
| `⟨⟩?` | `α → 𝔹` | Is pair | ✅ DONE |
| `#?` | `α → 𝔹` | Is atom | ✅ DONE |

### Type Annotations (18) ✅
**Type Constants (5):**
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `ℤ` | `() → Type` | Integer type constant | ✅ DONE (Day 83) |
| `𝔹` | `() → Type` | Boolean type constant | ✅ DONE (Day 83) |
| `𝕊` | `() → Type` | String type constant | ✅ DONE (Day 83) |
| `⊤` | `() → Type` | Any type constant (top type) | ✅ DONE (Day 83) |
| `∅ₜ` | `() → Type` | Nil type constant | ✅ DONE (Day 83) |

**Type Constructors (4):**
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `→` | `Type... → Type` | Function type (curried) | ✅ DONE (Day 83) |
| `[]ₜ` | `Type → Type` | List type | ✅ DONE (Day 83) |
| `⟨⟩ₜ` | `Type → Type → Type` | Pair type | ✅ DONE (Day 83) |
| `∪ₜ` | `Type → Type → Type` | Union type | ✅ DONE (Day 83) |

**Type Operations (4):**
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `∈⊙` | `α → Type` | Get runtime type of value | ✅ DONE (Day 83) |
| `∈≡` | `Type → Type → 𝔹` | Type equality test | ✅ DONE (Day 83) |
| `∈⊆` | `Type → Type → 𝔹` | Subtype check (t1 ≤ t2) | ✅ DONE (Day 83) |
| `∈!` | `α → Type → α \| ⚠` | Assert value has type | ✅ DONE (Day 83) |

**Type Declaration (2 special forms):**
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `∈` | `symbol → Type → Type` | Declare type for binding | ✅ DONE (Day 83) |
| `∈?` | `symbol → Type \| ∅` | Query declared type | ✅ DONE (Day 83) |

**Type Introspection (3):**
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `∈◁` | `Type → Type` | Get domain (input) of function type | ✅ DONE (Day 83) |
| `∈▷` | `Type → Type` | Get codomain (output) of function type | ✅ DONE (Day 83) |
| `∈⊙ₜ` | `Type → Type` | Get element type of list type | ✅ DONE (Day 83) |

### Type Validation (3) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `∈✓` | `:symbol → 𝔹 \| ⚠` | Validate binding against declared type | ✅ DONE (Day 84) |
| `∈✓*` | `() → 𝔹 \| ⚠` | Validate ALL declared types | ✅ DONE (Day 84) |
| `∈⊢` | `:symbol → α... → 𝔹 \| ⚠` | Type-check function application | ✅ DONE (Day 84) |

### Type Inference (3) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `∈⍜` | `α → Type` | Deep type inference (recursive pair/list/struct) | ✅ DONE (Day 85) |
| `∈⍜⊕` | `:symbol → Type \| ∅` | Get primitive type signature | ✅ DONE (Day 85) |
| `∈⍜*` | `expr → Type` | Infer expression type without evaluation (special form) | ✅ DONE (Day 85) |

**Note:** Type annotations are optional. Type validation (`∈✓`, `∈✓*`, `∈⊢`) provides
runtime checking. Type inference (`∈⍜`, `∈⍜⊕`, `∈⍜*`) enables static analysis.
See `test_type_inference.test`, `test_type_validation.test`.

### Debug & Error Handling (6) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⚠` | `:symbol → α → ⚠` | Create error value | ✅ DONE |
| `⚠?` | `α → 𝔹` | Test if error | ✅ DONE |
| `⚠⊙` | `⚠ → :symbol` | Get error type | ✅ DONE |
| `⚠→` | `⚠ → α` | Get error data | ✅ DONE |
| `⊢` | `𝔹 → :symbol → 𝔹 \| ⚠` | Assert condition | ✅ DONE |
| `⟲` | `α → α` | Trace (debug print) | ✅ DONE |

### Self-Introspection (2) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⧉` | `λ → ℕ` | Get arity of lambda | ✅ DONE |
| `⊛` | `λ → expression` | Get source code | ✅ DONE |

### Testing (7) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `≟` | `α → α → 𝔹` | Deep equality test | ✅ DONE |
| `⊨` | `:symbol → α → α → 𝔹 \| ⚠` | Test case | ✅ DONE |
| `gen-int` | `ℕ → ℕ → ℕ` | Random integer in range | ✅ DONE |
| `gen-bool` | `() → 𝔹` | Random boolean | ✅ DONE |
| `gen-symbol` | `[α] → α` | Random symbol from list | ✅ DONE |
| `gen-list` | `(() → α) → ℕ → [α]` | Generate random list | ✅ DONE |
| `⊨-prop` | `:symbol → (α → 𝔹) → (() → α) → 𝔹 \| ⚠` | Property-based test with shrinking | ✅ DONE |

### Effects (9) - Algebraic Effect System & Delimited Continuations
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⟪` | `:name :op... → 𝔹` | Declare effect type with operations | ✅ DONE (special form) |
| `⟪?` | `:name → 𝔹` | Query if effect is declared | ✅ DONE (special form) |
| `⟪→` | `:name → [:symbol]` | Get effect operations list | ✅ DONE (special form) |
| `⟪⟫` | `expr handler-spec... → α` | Handle effects in body (non-resumable) | ✅ DONE (special form) |
| `⟪↺⟫` | `expr handler-spec... → α` | Handle effects with resumable continuation `k` (fiber-based) | ✅ DONE (special form) |
| `↯` | `:effect :op args... → α` | Perform effect operation | ✅ DONE (special form) |
| `⟪⊸⟫` | `expr → α` | Reset/prompt — install delimited continuation delimiter | ✅ DONE (special form) |
| `⊸` | `(α → β) → α` | Shift/control — capture one-shot delimited continuation | ✅ DONE (special form) |
| `⤴` | `α → α` | Pure lift (identity) | ✅ DONE |
| `≫` | `α → (α → β) → β` | Effect bind (apply fn to value) | ✅ DONE |

**Effect System:**
```scheme
; Declare effect with operations
(⟪ :State :get :put)

; Non-resumable: handler result replaces perform
(⟪⟫ (⊕ (↯ :State :get) #1)
  (:State
    (:get (λ () #42))
    (:put (λ (v) ∅))))
; → #43

; Resumable: handler receives continuation k
(⟪↺⟫ (⊕ (↯ :State :get) #1)
  (:State
    (:get (λ (k) (k #42)))
    (:put (λ (k v) (k ∅)))))
; → #43 (k resumes body at perform point)

; Generator/yield pattern
(⟪↺⟫ (⊎ (↯ :Yield :value #1) (↯ :Yield :value #2) ∅)
  (:Yield (:value (λ (k v) (⟨⟩ v (k ∅))))))
; → (#1 #2)

; Abort (don't call k)
(⟪↺⟫ (↯ :State :get)
  (:State (:get (λ (k) :aborted))))
; → :aborted
```

**Delimited Continuations (shift/reset):**
```scheme
; Reset with no shift — body value passes through
(⟪⊸⟫ #42)                                    ; → #42

; Shift captures k, handler calls k with value
(⟪⊸⟫ (⊕ (⊸ (λ (k) (k #10))) #2))            ; → #12

; Shift abort — handler doesn't call k
(⟪⊸⟫ (⊕ (⊸ (λ (k) #999)) #2))               ; → #999

; Shift post-process — handler transforms k result
(⟪⊸⟫ (⊸ (λ (k) (⊗ (k #42) #2))))            ; → #84

; Multiple shifts in sequence
(⟪⊸⟫ (⊕ (⊸ (λ (k) (k #10)))
          (⊸ (λ (k) (k #20)))))               ; → #30

; Nested resets
(⟪⊸⟫ (⊕ (⟪⊸⟫ (⊸ (λ (k) (k #3)))) #7))      ; → #10

; One-shot: calling k twice returns error
(⚠? (⟪⊸⟫ (⊸ (λ (k) (⊎ (k #1) (k #2))))))   ; → #t
```

**Dynamic handler stack:** Inner handlers shadow outer for the same effect.
Non-resumable (`⟪⟫`) handlers receive perform arguments directly.
Resumable (`⟪↺⟫`) handlers receive continuation `k` as first argument; calling `(k value)` resumes body.
Implementation: fiber-based coroutines using `ucontext` — O(n) cost for n performs.
Delimited continuations (`⟪⊸⟫`/`⊸`) provide standalone shift/reset for general-purpose control flow.
Continuations are one-shot (linear) — calling `k` twice returns `⚠:one-shot-continuation-already-used`.
Unhandled effects return `⚠:unhandled-effect` errors.

### Actors (7) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⟳` | `(λ (self) ...) → ⟳[id]` | Spawn actor with behavior | ✅ |
| `→!` | `⟳ → α → ∅` | Send message (fire-and-forget) | ✅ |
| `←?` | `() → α` | Receive message (yields if empty) | ✅ |
| `⟳!` | `ℕ → ℕ` | Run scheduler for N ticks | ✅ |
| `⟳?` | `⟳ → 𝔹` | Check if actor is alive | ✅ |
| `⟳→` | `⟳ → α` | Get finished actor's result | ✅ |
| `⟳∅` | `() → ∅` | Reset all actors (testing) | ✅ |

Cooperative actor model built on fibers. Single-threaded round-robin scheduling.
Actors yield at `←?` when mailbox is empty. Use `≫` (bind) to sequence multiple receives.

### Channels (7) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⟿⊚` | `() → ⟿` or `ℕ → ⟿` | Create channel (optional capacity, default 64) | ✅ |
| `⟿→` | `⟿ → α → ∅` | Send value to channel (yields if full) | ✅ |
| `⟿←` | `⟿ → α` | Receive from channel (yields if empty) | ✅ |
| `⟿×` | `⟿ → ∅` | Close channel | ✅ |
| `⟿∅` | `() → ∅` | Reset all channels (testing) | ✅ |
| `⟿⊞` | `[⟿] → ⟨⟿ α⟩` | Select from multiple channels (blocking) | ✅ |
| `⟿⊞?` | `[⟿] → ⟨⟿ α⟩ \| ∅` | Try select (non-blocking) | ✅ |

Channels are first-class bounded ring buffers. Any actor can send/recv on any channel.
Scheduler polls channel state via `SuspendReason` on the fiber (SUSPEND_CHAN_RECV/SUSPEND_CHAN_SEND/SUSPEND_SELECT).

### Supervision (5) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⟳⊗` | `⟳ → ∅` | Bidirectional link to actor | ✅ |
| `⟳⊘` | `⟳ → ∅` | Remove bidirectional link | ✅ |
| `⟳⊙` | `⟳ → ∅` | One-way monitor (receive `:DOWN` on death) | ✅ |
| `⟳⊜` | `𝔹 → ∅` | Enable/disable exit trapping | ✅ |
| `⟳✕` | `⟳ → α → ∅` | Send exit signal with reason | ✅ |

Erlang-style supervision primitives. Bidirectional links propagate failure (error exit kills linked actors unless trapping). Monitors provide one-way `:DOWN` notifications. Trap-exit converts exit signals to `⟨:EXIT sender-id reason⟩` messages. Normal exits do NOT propagate.

### Supervisor Strategies (3) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⟳⊛` | `:strategy → [λ] → ℕ` | Create supervisor with strategy and child specs | ✅ |
| `⟳⊛?` | `ℕ → [⟳]` | Get list of current child actor cells | ✅ |
| `⟳⊛!` | `ℕ → ℕ` | Get supervisor restart count | ✅ |
| `⟳⊛⊕` | `ℕ → λ → ℕ` | Add child to supervisor dynamically | ✅ |
| `⟳⊛⊖` | `ℕ → ⟳ → 𝔹` | Remove child from supervisor | ✅ |

Supervisors manage groups of child actors and automatically restart them on failure. Strategies: `:one-for-one` (restart only failed child), `:one-for-all` (kill all siblings then restart all), `:rest-for-one` (restart failed child and all children after it). Max 5 restarts per supervisor prevents infinite restart loops. Normal exits do NOT trigger restarts. Children can be added/removed dynamically.

### Process Registry (4) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⟳⊜⊕` | `:symbol → ⟳ → #t \| ⚠` | Register actor under a name | ✅ |
| `⟳⊜⊖` | `:symbol → #t \| ⚠` | Unregister a name | ✅ |
| `⟳⊜?` | `:symbol → ⟳ \| ∅` | Look up actor by name | ✅ |
| `⟳⊜*` | `() → [:symbol]` | List all registered names | ✅ |

Erlang-style named process registry. Names are symbols. One name per actor, one actor per name. Dead actors are automatically deregistered via `actor_notify_exit`. `⟳∅` (reset) clears registry for test isolation.

### Timers (3) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⟳⏱` | `ℕ → ⟳ → α → ℕ` | Schedule message after N ticks | ✅ |
| `⟳⏱×` | `ℕ → #t \| ⚠` | Cancel a pending timer | ✅ |
| `⟳⏱?` | `ℕ → #t \| #f` | Check if timer is active | ✅ |

Timers schedule message delivery to an actor after N scheduler ticks. The scheduler keeps spinning while timers are pending. Dead actor targets silently drop the message. `⟳∅` (reset) clears all timers.

### Documentation (10) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⌂` | `:symbol → string` | Get description | ✅ DONE |
| `⌂∈` | `:symbol → string` | Get type signature | ✅ DONE |
| `⌂≔` | `:symbol → [symbols]` | Get dependencies | ✅ DONE |
| `⌂⊛` | `:symbol → ⊙` | Get provenance metadata | ✅ DONE |
| `⌂⊨` | `:symbol → [tests]` | Auto-generate tests | ✅ DONE |
| `⌂⊨!` | `:symbol → (ℕ ℕ ℕ)` | Execute auto-generated tests | ✅ DONE (Day 63+) |
| `⌂⊨⊗` | `:symbol → (ℕ ℕ ℕ)` | Mutation testing - validate test quality | ✅ DONE (Day 64) |
| `📖` | `≈ → ≈` | Generate markdown docs for module | ✅ DONE (Day 63) |
| `📖→` | `≈ → ≈ → ≈` | Export docs to file | ✅ DONE (Day 63) |
| `📖⊛` | `() → ≈ \| ≈ → ≈` | Generate module index with cross-refs | ✅ DONE (Day 63) |

**Mutation Testing (⌂⊨⊗):**

Mutation testing validates test suite quality by introducing small changes (mutations) to the code and checking if tests catch them. Returns a tuple `⟨killed ⟨survived ⟨total ∅⟩⟩⟩`:
- **killed**: Number of mutations caught by tests (good!)
- **survived**: Number of mutations that went undetected (bad - tests need improvement)
- **total**: Total mutations generated

**Mutation Strategies:**
1. **Operator mutations**: ⊕→⊖, ⊕→⊗, ⊗→⊘, ≡→≢, >→<, etc.
2. **Constant mutations**: #1→#2, #2→#3, #5→#6, etc. (sequential increments)
3. **Conditional mutations**: Swap then/else branches in `?` expressions

**Examples:**
```scheme
; Function with no tests
(≔ double (λ (n) (⊗ n #2)))
(⌂⊨⊗ :double)
; → ⟨#0 ⟨#2 ⟨#2 ∅⟩⟩⟩  (0 killed, 2 survived, 2 total)
; This means: NO mutations were caught! Tests are inadequate.

; Function with good tests
(≔ abs (λ (x) (? (< x #0) (⊖ #0 x) x)))
(⌂⊨ :abs)      ; Generate tests first
(⌂⊨! :abs)     ; Run tests: ⟨#3 ⟨#0 ⟨#3 ∅⟩⟩⟩ (all pass)
(⌂⊨⊗ :abs)     ; Mutation testing
; → ⟨#4 ⟨#0 ⟨#4 ∅⟩⟩⟩  (4 killed, 0 survived, 4 total)
; Perfect! All mutations were caught by tests.

; Verify the sum formula
(≔ r (⌂⊨⊗ :double))
(≡ (⊕ (◁ r) (◁ (▷ r))) (◁ (▷ (▷ r))))  ; → #t
; killed + survived = total (always true)
```

**Workflow:**
1. `(⌂⊨ :function)` - Generate tests from function structure
2. `(⌂⊨! :function)` - Verify tests pass
3. `(⌂⊨⊗ :function)` - Validate test quality with mutations
4. Improve tests if mutations survive
5. Repeat until all mutations are killed

**Known Limitations (Day 64):**
- Constants #0 and #1 not mutated (De Bruijn index ambiguity)
- No control over mutation count (fixed at 2-5 per function)
- Future: Mutation testing on surface syntax (before De Bruijn conversion)

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

### Graph Primitives (12) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⊝≔` | `:symbol → :symbol → [:symbol] → :symbol` | Define graph type | ✅ DONE |
| `⊝` | `:symbol → ⊝` | Create empty graph | ✅ DONE |
| `⊝⊕` | `⊝ → α → ⊝` | Add node (immutable) | ✅ DONE |
| `⊝⊗` | `⊝ → α → α → α → ⊝` | Add edge (immutable) | ✅ DONE |
| `⊝→` | `⊝ → :symbol → α` | Query graph property | ✅ DONE |
| `⊝?` | `α → :symbol → 𝔹` | Check graph type | ✅ DONE |

### Graph Algorithms (6) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⊝↦` | `⊝ → :symbol → α → (α → β) → [β]` | Traverse graph (BFS/DFS) | ✅ DONE |
| `⊝⊃` | `⊝ → α → α → 𝔹` | Check node reachability | ✅ DONE |
| `⊝⊚` | `⊝ → α → [α]` | Get node successors | ✅ DONE |
| `⊝⊙` | `⊝ → α → [α]` | Get node predecessors | ✅ DONE |
| `⊝⇝` | `⊝ → α → α → [α] \| ∅` | Find shortest path | ✅ DONE |
| `⊝∘` | `⊝ → [[α]] \| ∅` | Detect cycles | ✅ DONE |

**Graph Algorithm Usage:**
```scheme
; Get CFG for function
(≔ ! (λ (n) (? (≡ n #0) #1 (⊗ n (! (⊖ n #1))))))
(≔ cfg (⌂⟿ :!))

; Traverse all nodes
(⊝↦ cfg (⌜ :bfs) (⌜ :entry) (λ (node) node))  ; BFS from entry

; Check reachability
(⊝⊃ cfg (⌜ :entry) (⌜ :exit))  ; → #t (exit reachable from entry)

; Get successors/predecessors
(⊝⊚ cfg (⌜ :entry))  ; → List of nodes following entry
(⊝⊙ cfg (⌜ :exit))   ; → List of nodes leading to exit

; Find execution path
(⊝⇝ cfg (⌜ :entry) (⌜ :exit))  ; → Shortest path or ∅

; Detect recursion
(⊝∘ cfg)  ; → List of cycles (or ∅ if acyclic)
```

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

## Additional Implemented Features

### Pattern Matching (∇) ✅

Full pattern matching with guards, as-patterns, or-patterns, and view patterns. Implemented as a special form in the evaluator.

```scheme
(∇ expr (⌜ ((pattern₁ result₁)
             (pattern₂ result₂)
             ...)))

; Patterns: numbers, symbols, nil, pairs, wildcards (_), guards (|), as (@)
(≔ classify (λ (n)
  (∇ n (⌜ (((x | (> x #100)) :large)
            ((x | (> x #0)) :positive)
            (_ :non-positive))))))
```

### Macro System (6) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⧉` | Special form | Define simple macro | ✅ DONE |
| `⧉⊜` | Special form | Define pattern-based macro | ✅ DONE |
| `~` | Unquote | Evaluate in macro body | ✅ DONE |
| `⌞̃` | Quasiquote | Quote with unquote support | ✅ DONE |
| `⧉→` | `expr → expr` | Expand macro (debug) | ✅ DONE |
| `⧉?` | `() → [:symbol]` | List defined macros | ✅ DONE |

**Simple Macro Syntax (⧉):**
```scheme
(⧉ name (param₁ param₂ ...)
  (⌞̃ (template with (~ param₁) and (~ param₂))))

; Usage: (name arg₁ arg₂)
; Expands at compile-time
```

**Example (Simple Macro):**
```scheme
(⧉ when (condition body)
  (⌞̃ (? (~ condition) (~ body) ∅)))

(when (> x #0) (⊕ x #1))
; Expands to: (? (> x #0) (⊕ x #1) ∅)
```

**Pattern-Based Macro Syntax (⧉⊜):**
```scheme
(⧉⊜ name
  ((pattern₁) template₁)
  ((pattern₂) template₂)
  ...)

; Pattern variables start with $
; First matching pattern wins
; Expands at compile-time
```

**Rest/Ellipsis Pattern Syntax (`$var ...`):**
```scheme
; Capture remaining arguments into a list
(⧉⊜ variadic-fn
  (()                    ...)   ; Base case
  (($first $rest ...)    ...))  ; $rest captures remaining args as list

; Splice list back as arguments in template
(⧉⊜ sum
  (()          #0)                      ; Empty: return 0
  (($x)        $x)                      ; Single: return it
  (($x $rest ...) (⊕ $x (sum $rest ...)))) ; Recurse with spliced rest

(sum #1 #2 #3 #4 #5)  ; → #15

; Pattern: ($var ...) at end of pattern list
; - $var must be a pattern variable (starts with $)
; - Followed by literal ... symbol
; - Captures all remaining arguments as a list

; Template: ($var ...) splices list elements as arguments
; - Bound list from pattern match
; - Each element becomes a separate argument
```

**Ellipsis Pattern Examples:**
```scheme
;; Variadic product
(⧉⊜ product
  (()           #1)
  (($x)         $x)
  (($x $rest ...) (⊗ $x (product $rest ...))))

(product #2 #3 #4)  ; → #24

;; Variadic cond with pairs
(⧉⊜ cond*
  (()               ∅)
  ((($c $r))        (? $c $r ∅))
  ((($c $r) $rest ...) (? $c $r (cond* $rest ...))))

(cond* (#f :a) (#f :b) (#t :c))  ; → :c

;; Keyword dispatch with variadic args
(⧉⊜ calc
  ((:sum $args ...)     (sum $args ...))
  ((:product $args ...) (product $args ...)))

(calc :sum #1 #2 #3)      ; → #6
(calc :product #2 #3 #4)  ; → #24
```

**Example (Pattern Macro):**
```scheme
;; Multi-arity add
(⧉⊜ my-add
  (($x) $x)
  (($x $y) (⊕ $x $y))
  (($x $y $z) (⊕ $x (⊕ $y $z))))

(my-add #3 #4)      ; → #7
(my-add #1 #2 #3)   ; → #6

;; Keyword dispatch
(⧉⊜ kw-test
  ((:left $x) (⟨⟩ :l $x))
  ((:right $x) (⟨⟩ :r $x)))

(kw-test :left #5)   ; → ⟨:l #5⟩
(kw-test :right #10) ; → ⟨:r #10⟩

;; Literal matching
(⧉⊜ factorial-base
  ((#0) #1)
  ((#1) #1)
  (($n) :other))

(factorial-base #0) ; → #1
(factorial-base #5) ; → :other
```

**Stdlib Pattern Macros (stdlib/macros_pattern.scm):**

```scheme
;; ⇒* (cond) - Multi-branch conditional (1-5 clauses)
(⇒* ((> x #10) :big)
    ((> x #5) :medium)
    (#t :small))           ; → :small if x ≤ 5

;; ≔⇊ (let*) - Sequential bindings (1-4 bindings)
(≔⇊ ((:x #5)
      (:y (⊕ :x #1)))      ; :y = 6 (can reference :x)
     (⊕ :x :y))            ; → 11

;; ⇤ (case) - Value dispatch
(⇤ color
   (:red #ff0000)
   (:green #00ff00)
   (:else #000000))        ; → #000000 if no match
```

**Stdlib Control Macros (stdlib/macros_control.scm):**

```scheme
;; ∧* (and*) - Short-circuit AND (1-4 args)
;; Returns first #f, or last value if all non-false
(∧* #t #t)               ; → #t
(∧* #t #f)               ; → #f
(∧* #t #t #42)           ; → #42 (returns last value)
(∧* #f (⊘ #1 #0))        ; → #f (short-circuits, no div-by-zero)

;; ∨* (or*) - Short-circuit OR (1-4 args)
;; Returns first non-false value, or last value
(∨* #f #t)               ; → #t
(∨* #f #42)              ; → #42 (first non-false)
(∨* #t (⊘ #1 #0))        ; → #t (short-circuits)

;; ⇒ (when) - Execute body if condition true
(⇒ #t :yes)              ; → :yes
(⇒ #f :never)            ; → ∅ (nil, body not evaluated)

;; ⇏ (unless) - Execute body if condition false
(⇏ #f :yes)              ; → :yes
(⇏ #t :never)            ; → ∅ (nil, body not evaluated)
```

**Stdlib Exception Macros (stdlib/macros_exception.scm):**

```scheme
;; ⚡ (try-with) - Execute body, call handler if error
(⚡ (⊘ #6 #2) (λ (e) :error))   ; → #3 (success)
(⚡ (⊘ #1 #0) (λ (e) :error))   ; → :error (handler called)

;; ⚡⊳ (try-or) - Execute with fallback default on error
(⚡⊳ (⊘ #6 #2) #0)              ; → #3 (success)
(⚡⊳ (⊘ #1 #0) #0)              ; → #0 (default on error)

;; ⚡∅ (ignore-errors) - Execute, return nil on any error
(⚡∅ (⊘ #6 #2))                 ; → #3 (success)
(⚡∅ (⊘ #1 #0))                 ; → ∅ (error ignored)

;; ⚡? (error-type?) - Check if error has specific type
(⚡? (⊘ #1 #0) :div-by-zero)    ; → #t (error type matches)
(⚡? #42 :any)                   ; → #f (not an error)

;; ⚡⊙ (error-data) - Extract error data safely
(⚡⊙ (⚠ :not-found "key"))      ; → "key"
(⚡⊙ #42)                        ; → ∅ (not an error)

;; ⚡∧ (all-succeed) - Execute all, fail if any fails
(⚡∧ (⊘ #6 #2) (⊕ #1 #1))       ; → #2 (both succeed)
(⚡∧ (⊘ #1 #0) (⊕ #1 #1))       ; → ⚠:div-by-zero (first fails)

;; ⚡∨ (first-success) - Return first successful result
(⚡∨ (⊘ #1 #0) (⊘ #6 #2))       ; → #3 (first fails, second succeeds)
(⚡∨ (⊘ #1 #0) (⊘ #1 #0))       ; → ⚠ (all fail)

;; ⚡⟲ (try-finally) - Execute with cleanup
(⚡⟲ (⊘ #6 #2) (⟲ :cleanup))    ; → #3 (prints :cleanup)

;; ⚡↺ (retry) - Retry on error up to n times
(⚡↺ #3 (may-fail))             ; Try up to 3 times
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
| `⟪` | Declare effect | Register effect type with operations | ✅ DONE |
| `⟪?` | Query effect | Check if effect is declared | ✅ DONE |
| `⟪→` | Effect operations | Get operations list for effect | ✅ DONE |
| `⟪⟫` | Handle effects | Install non-resumable handlers, evaluate body | ✅ DONE |
| `⟪↺⟫` | Resumable handle | Install handlers with continuation `k` (fiber-based) | ✅ DONE |
| `↯` | Perform effect | Trigger effect operation | ✅ DONE |
| `⟪⊸⟫` | Reset/prompt | Install delimited continuation delimiter | ✅ DONE |
| `⊸` | Shift/control | Capture one-shot delimited continuation | ✅ DONE |
| `⤴` | Pure lift | Identity (value unchanged) | ✅ DONE |
| `≫` | Effect bind | Apply function to value | ✅ DONE |

### Refinement Types (4) - COMPILE TIME ONLY
| Symbol | Type | Meaning |
|--------|------|---------|
| `{⋅∣φ}` | `{ν:τ ∣ φ}` | Refinement |
| `⊢` | `⊢ φ` | Proof |
| `⊨` | `⊨ α φ` | Assert |
| `∴` | Therefore | Conclusion |

### Actors (7) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⟳` | `(λ (self) ...) → ⟳[id]` | Spawn actor | ✅ |
| `→!` | `⟳ → α → ∅` | Send message | ✅ |
| `←?` | `() → α` | Receive message | ✅ |
| `⟳!` | `ℕ → ℕ` | Run scheduler | ✅ |
| `⟳?` | `⟳ → 𝔹` | Actor alive? | ✅ |
| `⟳→` | `⟳ → α` | Actor result | ✅ |
| `⟳∅` | `() → ∅` | Reset actors | ✅ |

### Channels (7) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⟿⊚` | `() → ⟿` | Create channel | ✅ |
| `⟿→` | `⟿ → α → ∅` | Send to channel | ✅ |
| `⟿←` | `⟿ → α` | Receive from channel | ✅ |
| `⟿×` | `⟿ → ∅` | Close channel | ✅ |
| `⟿∅` | `() → ∅` | Reset channels | ✅ |
| `⟿⊞` | `[⟿] → ⟨⟿ α⟩` | Select (blocking) | ✅ |
| `⟿⊞?` | `[⟿] → ⟨⟿ α⟩ \| ∅` | Try select (non-blocking) | ✅ |

### Data Structures (15) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⊙≔` | Define leaf | Define simple structure | ✅ DONE |
| `⊙` | Create leaf | Create leaf instance | ✅ DONE |
| `⊙→` | Get field | Access structure field | ✅ DONE |
| `⊙←` | Set field | Update structure field | ✅ DONE |
| `⊙?` | Type check | Check structure type | ✅ DONE |
| `⊚≔` | Define node | Define recursive structure (ADT) | ✅ DONE |
| `⊚` | Create node | Create node instance | ✅ DONE |
| `⊚→` | Get field | Access node field | ✅ DONE |
| `⊚?` | Variant check | Check type and variant | ✅ DONE |
| `⊝≔` | Define graph | Define graph structure | ✅ DONE |
| `⊝` | Create graph | Create graph instance | ✅ DONE |
| `⊝⊕` | Add node | Add node to graph | ✅ DONE |
| `⊝⊗` | Add edge | Add edge to graph | ✅ DONE |
| `⊝→` | Query graph | Query graph structure | ✅ DONE |
| `⊝?` | Graph check | Check graph type | ✅ DONE |

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

### Control/Data Flow (2) ✅
| Symbol | Type | Meaning | Status |
|--------|------|---------|--------|
| `⌂⟿` | `:symbol → CFG` | Get control flow graph | ✅ DONE |
| `⌂⇝` | `:symbol → DFG` | Get data flow graph | ✅ DONE |

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
- Phases 1-2E: Complete (lambda calculus through actors/channels/supervision/supervisors)
- Phase 3: Dynamic supervisor management, rest-for-one strategy, optimizer
- Phase 4: Self-hosting (parser/compiler in Guage)
- Phase 5: Advanced metaprogramming (synthesis, time-travel debugging)
- Phase 6: Distribution, native compilation

See `METAPROGRAMMING_VISION.md` for detailed specifications and implementation strategy.
