---
Status: CURRENT
Created: 2026-01-28
Updated: 2026-01-29 (Day 72 Complete)
Purpose: Roadmap for completing self-hosting evaluator
---

# Self-Hosting Evaluator Completion Roadmap

## Current State (Day 72 Complete)

**Status:** 42/42 eval tests passing (100%)
**Core Evaluator:** COMPLETE - all basic special forms implemented
**What's Next:** Recursive letrec OR self-hosting parser

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
| Letrec (⊛) | ✅ | Day 72 (let-style, non-recursive) |
| Meta-eval (⌞) | ✅ | Day 72 |
| **Recursive letrec** | ⏳ | **Next: Y-combinator transform** |

## Files Involved

```
bootstrap/stdlib/eval.scm      # Main evaluator (~275 lines)
bootstrap/stdlib/eval-env.scm  # Environment module (37 lines)
bootstrap/tests/test_eval.test # Test suite (42 tests)
```

---

## NEXT: Recursive Letrec via Y-Combinator Transformation

### The Problem

Current letrec (⊛) uses sequential let-style binding:
```scheme
(⊛ ((:fact (λ (:n) ...))) (:fact #5))  ; ❌ :fact not in scope during definition
```

For recursion, we need the binding available during the lambda body evaluation.

### The Solution: Self-Application Transformation

Transform recursive bindings using the Y-combinator pattern:

```scheme
; Original:
(⊛ ((:fact (λ (:n) (? (:≤ :n #1) #1 (:⊗ :n (:fact (:⊖ :n #1)))))))
   (:fact #5))

; Transform to:
(⊛ ((:fact-impl (λ (:self) (λ (:n)
                   (? (:≤ :n #1) #1 (:⊗ :n ((:self :self) (:⊖ :n #1))))))))
   (⊛ ((:fact (λ (:n) ((:fact-impl :fact-impl) :n))))
      (:fact #5)))
```

### Implementation Strategy

**Option A: AST Transformation (Recommended)**

Transform letrec bindings before evaluation:

```scheme
(≔ transform-recursive (λ (name) (λ (body)
  ; Replace (name args...) with ((self self) args...) in body
  ; Wrap in (λ (self) ...)
  ...)))

(≔ eval-letrec-recursive (λ (bindings) (λ (body) (λ (env)
  ; For each binding:
  ; 1. Transform (name (λ (params) body)) to self-application form
  ; 2. Bind transformed version
  ; 3. Create wrapper that applies (impl impl)
  ...))))
```

**Option B: Environment Mutation (Simpler but impure)**

Use mutable cells to create forward references:

```scheme
; Create placeholder, evaluate, update placeholder
; Not recommended for pure functional evaluator
```

### Substitution Helpers (Already Implemented!)

Day 72 added these helpers in eval.scm (lines 64-109):

```scheme
(≔ member? (λ (x) (λ (lst) ...)))        ; Check if symbol in list
(≔ subst (λ (name) (λ (replacement) (λ (expr) ...))))  ; Substitute in expr
(≔ subst-list (λ (name) (λ (replacement) (λ (exprs) ...))))  ; Substitute in list
(≔ subst-all (λ (names) (λ (replacements) (λ (expr) ...))))  ; Multiple substitutions
```

These handle lambda shadowing correctly - ready for Y-combinator transform!

### Test Cases for Recursive Letrec

```scheme
;;; Test: Single recursive binding (factorial)
(⊨ :eval-letrec-factorial
   #120
   ((eval (⌜ (⊛ ((:fact (λ (:n)
                   (? (:≤ :n #1) #1 (:⊗ :n (:fact (:⊖ :n #1)))))))
              (:fact #5)))) env-full))

;;; Test: Mutual recursion (even/odd)
(⊨ :eval-letrec-even-odd
   #t
   ((eval (⌜ (⊛ ((:even? (λ (:n) (? (:≡ :n #0) #t (:odd? (:⊖ :n #1)))))
                 (:odd? (λ (:n) (? (:≡ :n #0) #f (:even? (:⊖ :n #1))))))
              (:even? #4)))) env-full))
```

### Implementation Steps

1. **Detect recursive bindings** - Check if name appears in body
2. **Transform single recursive binding**:
   ```scheme
   ; (name (λ (params) body-with-name))
   ; becomes:
   ; (name-impl (λ (self) (λ (params) body-with-self-self)))
   ; (name (λ (params) ((name-impl name-impl) params)))
   ```
3. **Handle mutual recursion** - Transform all names simultaneously
4. **Update eval-letrec** to use transformation

### Complexity Notes

- Single recursion: Medium complexity
- Mutual recursion: High complexity (need simultaneous substitution)
- Substitution helpers already handle shadowing

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
| **Total Core** | **42** | **42/42 (100%)** |

## Remaining Work

| Feature | Complexity | Time Est. | Value |
|---------|------------|-----------|-------|
| Recursive letrec (Y-transform) | High | 3-4 hours | HIGH |
| Self-hosting parser | Very High | 6-9 hours | MILESTONE |
| TCO in meta-evaluator | Medium | 2-3 hours | MEDIUM |

---

**Last Updated:** 2026-01-29 (Day 72 Complete)
**Core Evaluator:** COMPLETE
**Next Priority:** Recursive letrec OR self-hosting parser
