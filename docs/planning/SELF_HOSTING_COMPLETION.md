---
Status: CURRENT
Created: 2026-01-28
Updated: 2026-01-29 (Day 72 Complete)
Purpose: Roadmap for completing self-hosting evaluator
---

# Self-Hosting Evaluator Completion Roadmap

## Current State (Day 72 Complete)

**Status:** 42/42 eval tests passing (100% of current tests)
**What Works:** Full lambda calculus + primitives via ⊡ + define (≔) + letrec (⊛) + meta-eval (⌞)
**What's Missing:** Y-combinator transformation for recursive letrec

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
| Quote (⌜) | ✅ | Added Day 71 |
| Primitives | ✅ | Via ⊡ apply |
| Y-combinator recursion | ✅ | Factorial verified |
| Define (≔) | ✅ | Added Day 72 via eval-body |
| Letrec (⊛) | ✅ | Let-style bindings (Day 72) |
| Eval (⌞) | ✅ | Meta-evaluation (Day 72) |
| Recursive letrec | ⏳ | Needs Y-combinator transform |

## Files Involved

```
bootstrap/stdlib/eval.scm      # Main evaluator (141 lines)
bootstrap/stdlib/eval-env.scm  # Environment module (37 lines)
bootstrap/tests/test_eval.test # Test suite (32 tests)
```

## Next Steps to Complete Self-Hosting

### Phase 1: Add Define (≔) Support (1-2 hours)

**Goal:** Allow local definitions within evaluated code

**Challenge:** Define mutates environment, but our evaluator is functional

**Approach A: Let-style binding (Recommended)**
```scheme
; Transform (≔ x 5) into a let-binding for the rest of the expression
; In evaluator, handle ≔ by extending environment and evaluating next expr
(≔ eval-define (λ (name) (λ (value-expr) (λ (env)
  (((env-extend env) name) ((eval value-expr) env))))))
```

**Approach B: Return modified environment**
```scheme
; eval returns (value . new-env) pair
; Allows sequential defines to accumulate
```

**Implementation in eval-list:**
```scheme
; Add to special-form? check
(? (≡ (◁ expr) (⌜ ≔))
   #t
   ...)

; Add to eval-list handling
(? (≡ (◁ expr) (⌜ ≔))
   ; (≔ name value) - extend env and return value
   (? (⟨⟩? (▷ expr))
      (? (⟨⟩? (▷ (▷ expr)))
         ; Get name and value
         (((env-extend env) (◁ (▷ expr)))
          ((eval (◁ (▷ (▷ expr)))) env))
         (⚠ :define-missing-value expr))
      (⚠ :define-missing-name expr))
   ...)
```

### Phase 2: Add Letrec (2-3 hours)

**Goal:** Support mutually recursive definitions

**Why needed:** Y-combinator works but is awkward. Letrec is cleaner.

**Implementation:**
```scheme
; letrec creates environment with forward references
; then fills in the actual values

(≔ eval-letrec (λ (bindings) (λ (body) (λ (env)
  ; 1. Create env with placeholder values
  ; 2. Evaluate each binding in extended env
  ; 3. Update bindings with actual values
  ; 4. Evaluate body in final env
  ...))))
```

### Phase 3: Add Eval (⌞) Special Form (1 hour)

**Goal:** Meta-circular evaluation - evaluate quoted code

**Implementation:**
```scheme
; (⌞ expr) - evaluate expr, then evaluate the result
(? (≡ (◁ expr) (⌜ ⌞))
   ((eval ((eval (◁ (▷ expr))) env)) env)
   ...)
```

**Test:**
```scheme
(≔ code (⌜ (⊕ #1 #2)))
((eval (⌜ (⌞ code))) env)  ; → #3
```

### Phase 4: Self-Hosting Parser (Future)

**Goal:** Parser written in Guage that can parse Guage

**Prerequisites:** String operations, character handling

**Not in immediate scope** - focus on evaluator completion first

## Test Plan for Each Phase

### Phase 1 Tests (Define)
```scheme
; Test local define
(⊨ :eval-define-simple
   #5
   ((eval (⌜ (≔ x #5))) empty-env))

; Test define with expression
(⊨ :eval-define-expr
   #7
   ((eval (⌜ (≔ y (⊕ #3 #4)))) env-arith))

; Test sequential defines (if supported)
(⊨ :eval-define-seq
   #15
   ((eval (⌜ ((λ () (≔ a #5) (≔ b #10) (⊕ a b))))) env-arith))
```

### Phase 2 Tests (Letrec)
```scheme
; Test mutual recursion: even/odd
(⊨ :eval-letrec-even-odd
   #t
   ((eval (⌜ (letrec ((even? (λ (n) (? (≡ n #0) #t (odd? (⊖ n #1)))))
                       (odd? (λ (n) (? (≡ n #0) #f (even? (⊖ n #1))))))
               (even? #4)))) env-full))
```

### Phase 3 Tests (Eval)
```scheme
; Test meta-evaluation
(⊨ :eval-meta-eval
   #3
   ((eval (⌜ (⌞ (⌜ (⊕ #1 #2))))) env-arith))
```

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

## Session Continuation Guide

### Starting Next Session

1. **Read this file** for context
2. **Run tests:** `make test` (should show 71/71 passing)
3. **Check eval tests:** `./bootstrap/guage < bootstrap/tests/test_eval.test`
4. **Choose phase** to work on (recommend Phase 1 first)

### Key Files to Understand

```scheme
; Main evaluator - add new special forms here
bootstrap/stdlib/eval.scm

; Environment operations - may need extension for letrec
bootstrap/stdlib/eval-env.scm

; Tests - add new tests as you implement features
bootstrap/tests/test_eval.test
```

### Common Patterns in Evaluator

**Adding a special form:**
1. Add check in `special-form?` function
2. Add handling in `eval-list` conditional chain
3. Create helper function if needed (like `eval-lambda`, `eval-if`)
4. Add tests

**Environment extension:**
```scheme
(((env-extend env) name) value)  ; Returns new env with binding
```

**Recursive evaluation:**
```scheme
((eval sub-expr) env)  ; Evaluate in current env
```

## Success Metrics

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
| Letrec (⊛) | 2 | ✅ Day 72 |
| Meta-eval (⌞) | 4 | ✅ Day 72 |
| **Total** | **42** | **42/42 (100%)** |

## Completed Phases

| Phase | Time | Complexity | Status |
|-------|------|------------|--------|
| Phase 1: Define | 1-2 hours | Medium | ✅ Complete Day 72 |
| Phase 2: Letrec | 1 hour | Medium | ✅ Complete Day 72 (let-style) |
| Phase 3: Eval | 30 min | Low | ✅ Complete Day 72 |
| **Core Evaluator** | **Complete!** | |

## Future Enhancements

| Feature | Complexity | Notes |
|---------|------------|-------|
| Y-combinator transformation for recursive letrec | High | Enable mutual recursion |
| Self-hosting parser | Very High | Parser in Guage |
| Tail-call optimization in meta-evaluator | Medium | Performance |

---

**Last Updated:** 2026-01-29 (Day 72 Complete)
**Next Session:** Y-combinator letrec or self-hosting parser
