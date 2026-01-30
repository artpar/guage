---
Status: CURRENT
Created: 2026-01-28
Updated: 2026-01-29 (Day 73 Complete)
Purpose: Roadmap for completing self-hosting evaluator
---

# Self-Hosting Evaluator Completion Roadmap

## Current State (Day 73 Complete)

**Status:** 47/47 eval tests passing (100%)
**Core Evaluator:** COMPLETE with recursive letrec
**What's Next:** Mutual recursion OR self-hosting parser

### Evaluator Capabilities (bootstrap/stdlib/eval.scm)

| Feature | Status | Notes |
|---------|--------|-------|
| Numbers, booleans, nil | ✅ | Self-evaluate |
| Symbol lookup | ✅ | Via env-lookup |
| Lambda creation | ✅ | Creates closures |
| Function application | ✅ | Closures and primitives |
| Multi-param lambdas | ✅ | Via bind-params |
| Closures with capture | ✅ | Environment in closure |
| Conditionals (?) | ✅ | Full support |
| Quote (⌜) | ✅ | Day 71 |
| Primitives | ✅ | Via ⊡ apply |
| Y-combinator recursion | ✅ | Factorial verified |
| Define (≔) | ✅ | Day 72 via eval-body |
| Letrec (⊛) | ✅ | Day 72 (let-style) |
| Meta-eval (⌞) | ✅ | Day 72 |
| **Recursive letrec** | ✅ | **Day 73: Y-combinator transform** |
| Mutual recursion | ⏳ | Next: simultaneous binding |

## Files Involved

```
bootstrap/stdlib/eval.scm      # Main evaluator (~320 lines)
bootstrap/stdlib/eval-env.scm  # Environment module (37 lines)
bootstrap/tests/test_eval.test # Test suite (47 tests)
```

---

## ✅ COMPLETED: Recursive Letrec (Day 73)

**Implementation:**
- `contains-symbol?` / `contains-symbol-list?` - Detect if symbol appears in expression
- `is-recursive-binding?` - Check if binding name appears in its body
- `transform-recursive-ast` - Apply Y-combinator self-application pattern
- Updated `eval-letrec` to auto-detect and transform recursive bindings

**How it works:**
```scheme
; Original:
(⊛ ((:fact (λ (:n) (? (:≤ :n #1) #1 (:⊗ :n (:fact (:⊖ :n #1)))))))
   (:fact #5))

; Automatically transformed to:
((λ (:self) (λ (:n) (? (:≤ :n #1) #1 (:⊗ :n ((:self :self) (:⊖ :n #1))))))
 (λ (:self) (λ (:n) (? (:≤ :n #1) #1 (:⊗ :n ((:self :self) (:⊖ :n #1)))))))
```

**Tests passing:**
- `:eval-letrec-factorial` (5! = 120) ✓
- `:eval-letrec-factorial-base` (1! = 1) ✓
- `:eval-letrec-fib` (fib(6) = 8) ✓

---

## NEXT: Mutual Recursion

### The Problem

Current implementation handles single recursive bindings but not mutual recursion:
```scheme
(⊛ ((:even? (λ (:n) (? (:≡ :n #0) #t (:odd? (:⊖ :n #1)))))
     (:odd? (λ (:n) (? (:≡ :n #0) #f (:even? (:⊖ :n #1))))))
   (:even? #4))  ; ❌ Needs simultaneous binding
```

### The Solution: Tuple-Based Mutual Recursion

Transform mutually recursive bindings using a shared self-application tuple.

**Conceptual approach:**
```scheme
; Original: even? references odd?, odd? references even?

; Transform to single recursive function returning tuple:
(≔ fns
  ((λ (:self)
     (⟨⟩ (λ (:n) (? (:≡ :n #0) #t ((▷ (:self :self)) (:⊖ :n #1))))   ; even?
         (λ (:n) (? (:≡ :n #0) #f ((◁ (:self :self)) (:⊖ :n #1)))))) ; odd?
   (λ (:self)
     (⟨⟩ (λ (:n) (? (:≡ :n #0) #t ((▷ (:self :self)) (:⊖ :n #1))))
         (λ (:n) (? (:≡ :n #0) #f ((◁ (:self :self)) (:⊖ :n #1))))))))

; Then extract:
(≔ even? (◁ fns))
(≔ odd? (▷ fns))
```

### Implementation Steps

1. **Detect mutual recursion** - Check if bindings reference each other
   - Existing: `contains-symbol?` can check each binding against all names
   - New: `find-mutual-bindings` - group bindings that reference each other

2. **Transform binding group** - All mutually recursive bindings together
   - Replace all names with tuple accessors
   - Create single recursive function returning tuple
   - Apply to itself

3. **Update eval-letrec** - Handle mutual recursion case
   - If mutual recursion detected, use tuple transformation
   - Otherwise use existing single-recursion transformation

### Key Helpers (Already Have)
- `subst-all` - Substitute multiple names at once (lines 105-109)
- `contains-symbol?` - Check if symbol in expression (lines 117-129)
- `subst-list` - Substitute in list of expressions (lines 98-102)

### Test Cases
```scheme
;;; Test: Mutual recursion (even/odd)
(⊨ :eval-letrec-even-odd
   #t
   ((eval (⌜ (⊛ ((:even? (λ (:n) (? (:≡ :n #0) #t (:odd? (:⊖ :n #1)))))
                 (:odd? (λ (:n) (? (:≡ :n #0) #f (:even? (:⊖ :n #1))))))
              (:even? #4)))) env-full))

(⊨ :eval-letrec-odd
   #t
   ((eval (⌜ (⊛ ((:even? ...) (:odd? ...)) (:odd? #3)))) env-full))
```

### Complexity Notes

- Medium-high complexity due to tuple construction
- Existing `subst-all` handles multi-name substitution
- Main challenge: determining correct tuple accessor for each name

---

## ALTERNATIVE: Self-Hosting Parser

### Goal
Parser written in Guage that can parse Guage source code.

### Prerequisites
- String operations (already have: ⊕⊕, ⌷, ⌷⌷, etc.)
- Character handling (have: →#, #→)
- List operations (complete)

### Components Needed

1. **Tokenizer** - Split string into tokens
   - Numbers: `#42`, `#-5`
   - Symbols: `:name`
   - Strings: `"text"`
   - Delimiters: `(`, `)`, `'`
   - Operators: `λ`, `≔`, `?`, etc.

2. **Parser** - Build AST from tokens
   - Handle nested lists
   - Handle quote shorthand
   - Error recovery

3. **Integration** - Connect to evaluator
   - `(parse string)` → AST
   - `(eval (parse string) env)` → result

### Estimated Effort
- Tokenizer: 2-3 hours
- Parser: 3-4 hours
- Integration & testing: 1-2 hours
- **Total: 6-9 hours**

---

## Current Test Coverage

```
bootstrap/tests/test_eval.test - 42 tests:
- Atoms (6): numbers, booleans, nil
- Symbols (2): lookup, undefined error
- Lambda (3): creation, identity, const
- Arithmetic (3): add, multiply, nested
- Conditionals (3): true, false, comparison
- Closures (3): capture, two-params, higher-order
- Errors (2): empty list, non-function
- Quote (3): symbol, list, number
- Combinators (3): curried-add, K-combinator, double-apply
- Recursion (1): Y-combinator factorial
- Lists (2): cons-is-pair, cons-head
- Nested (1): nested conditionals
- Define (6): simple, expression, sequential, shadow, uses-outer, standalone
- Letrec (2): simple binding, multiple bindings
- Meta-eval (4): simple, variable, nested, conditional
```

---

## Session Continuation Guide

### Starting Next Session

1. **Read this file** for context
2. **Run tests:** `make test` (should show 71/71 passing)
3. **Check eval tests:** `./bootstrap/guage < bootstrap/tests/test_eval.test`
4. **Choose task:**
   - Recursive letrec (3-4 hours, HIGH VALUE)
   - Self-hosting parser (6-9 hours, MILESTONE)

### Key Files to Understand

```scheme
; Main evaluator - eval-letrec at lines 143-154
bootstrap/stdlib/eval.scm

; Substitution helpers at lines 64-109 (ready for Y-combinator!)
; member?, subst, subst-list, subst-all

; Environment operations
bootstrap/stdlib/eval-env.scm

; Tests - add recursive tests when implementing
bootstrap/tests/test_eval.test
```

### Common Patterns in Evaluator

**Adding to special-form? check:**
```scheme
(? (≡ (◁ expr) (⌜ new-form))
   #t
   ...)
```

**Environment extension:**
```scheme
(((env-extend env) name) value)  ; Returns new env with binding
```

**Substitution (for Y-combinator):**
```scheme
(((subst :old-name) :new-name) expr)  ; Replace symbol in expr
```

---

## Completed Milestones

| Milestone | Tests | Status |
|-----------|-------|--------|
| Basic eval (atoms, symbols) | 8 | ✅ |
| Lambda + application | 6 | ✅ |
| Conditionals | 3 | ✅ |
| Primitives via ⊡ | 3 | ✅ |
| Quote support | 3 | ✅ |
| Recursion (Y-combinator) | 1 | ✅ |
| Combinators | 3 | ✅ |
| Lists | 2 | ✅ |
| Define (≔) | 6 | ✅ Day 72 |
| Letrec (⊛) simple | 2 | ✅ Day 72 |
| Meta-eval (⌞) | 4 | ✅ Day 72 |
| Recursive Letrec | 3 | ✅ Day 73 |
| **Total Core** | **47** | **47/47 (100%)** |

## Remaining Work

| Feature | Complexity | Time Est. | Value |
|---------|------------|-----------|-------|
| Mutual recursion | High | 2-3 hours | MEDIUM |
| Self-hosting parser | Very High | 6-9 hours | MILESTONE |
| TCO in meta-evaluator | Medium | 2-3 hours | MEDIUM |

---

**Last Updated:** 2026-01-29 (Day 73 Complete)
**Core Evaluator:** COMPLETE with recursive letrec
**Next Priority:** Mutual recursion OR self-hosting parser
