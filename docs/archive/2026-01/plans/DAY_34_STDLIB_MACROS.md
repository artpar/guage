---
Status: CURRENT
Created: 2026-01-27
Updated: 2026-01-27
Purpose: Day 34 implementation plan - Standard library macros leveraging Day 33's macro system
---

# Day 34: Standard Library Macros

## Executive Summary

**Goal:** Build ergonomic macros using Day 33's macro system to make Guage more expressive

**Duration:** 1 day (6-8 hours)

**Prerequisites:**
- ✅ Macro system working (Day 33)
- ✅ Quasiquote/unquote (Day 32 Part 2)
- ✅ Pattern matching (Days 15-19)
- ✅ Module system (Days 26-30)

**Impact:** Immediate usability improvements - common patterns become one-liners

---

## Why Standard Library Macros?

### Current State (Without Macros)

**Verbose control flow:**
```scheme
; When pattern - want action only if condition true
(? condition
   (do-something)
   ∅)  ; Awkward empty else

; Unless pattern - want action if condition false
(? condition
   ∅
   (do-something))  ; Confusing order

; Multi-clause conditional - nested ifs
(? (≡ x #1)
   :one
   (? (≡ x #2)
      :two
      (? (≡ x #3)
         :three
         :other)))  ; Pyramid of doom
```

**Verbose let bindings:**
```scheme
; Want to bind multiple values
((λ (a)
  ((λ (b)
    ((λ (c)
      (⊕ a (⊕ b c)))
     (compute-c)))
   (compute-b)))
 (compute-a))  ; Nested lambda hell
```

**Verbose iteration:**
```scheme
; Want to map with side effects
(≔ process-all (λ (items)
  (? (∅? items)
     ∅
     (⌜ ⊙ (≋ (◁ items))
          (process-all (▷ items))))))  ; Manual recursion every time
```

### With Standard Library Macros (After Day 34)

**Clean control flow:**
```scheme
; When macro - clear intent
(when (> x #0)
  (increment x))

; Unless macro - obvious meaning
(unless (empty? list)
  (process list))

; Cond macro - readable multi-clause
(cond
  [(≡ x #1) :one]
  [(≡ x #2) :two]
  [(≡ x #3) :three]
  [else :other])
```

**Clean bindings:**
```scheme
; Let macro - multiple bindings
(let ([a (compute-a)]
      [b (compute-b)]
      [c (compute-c)])
  (⊕ a (⊕ b c)))
```

**Clean iteration:**
```scheme
; For-each macro - side effects
(for-each (λ (item) (≋ item)) items)

; Collect macro - build lists
(collect (⊗ x #2) for x in (⋯ #1 #10))
; → ⟨#2 ⟨#4 ⟨#6 ... ⟨#20 ∅⟩⟩⟩⟩
```

---

## Implementation Plan

### Phase 1: Control Flow Macros (2 hours)

**Macros to implement:**

#### 1.1 `when` - Conditional without else (30 min)
```scheme
; Syntax: (when condition body...)
; Expands to: (? condition (⌜ ⊙ body...) ∅)

(⧉ :when
    (⌜ ⟨condition body⟩)
    (⌞̃ (? (~ condition) (~ body) ∅)))

; Usage:
(when (> x #0)
  (≋ "positive"))

; Expands to:
(? (> x #0)
   (≋ "positive")
   ∅)
```

#### 1.2 `unless` - Negated conditional (30 min)
```scheme
; Syntax: (unless condition body...)
; Expands to: (? (¬ condition) (⌜ ⊙ body...) ∅)

(⧉ :unless
    (⌜ ⟨condition body⟩)
    (⌞̃ (? (¬ (~ condition)) (~ body) ∅)))

; Usage:
(unless (∅? list)
  (process list))

; Expands to:
(? (¬ (∅? list))
   (process list)
   ∅)
```

#### 1.3 `cond` - Multi-clause conditional (60 min)
```scheme
; Syntax: (cond [test₁ result₁] [test₂ result₂] ... [else default])
; Expands to: nested ? expressions

(⧉ :cond
    (⌜ clauses)
    (cond-expand clauses))

; Helper function:
(≔ cond-expand (λ (clauses)
  (? (∅? clauses)
     (⚠ :cond-no-match ∅)
     (⌜ ⊙ [first-clause (◁ clauses)]
            [rest-clauses (▷ clauses)]
            [test (◁ first-clause)]
            [result (◁ (▷ first-clause))]
            (? (≡ test :else)
               result  ; else clause
               (⌞̃ (? (~ test)
                     (~ result)
                     (~ (cond-expand rest-clauses)))))))))

; Usage:
(cond
  [(< x #0) :negative]
  [(≡ x #0) :zero]
  [(> x #0) :positive])

; Expands to:
(? (< x #0)
   :negative
   (? (≡ x #0)
      :zero
      (? (> x #0)
         :positive
         (⚠ :cond-no-match ∅))))
```

**Tests:** 15 tests for control flow macros

---

### Phase 2: Binding Macros (2 hours)

#### 2.1 `let` - Local bindings (60 min)
```scheme
; Syntax: (let ([var₁ expr₁] [var₂ expr₂] ...) body)
; Expands to: nested lambdas

(⧉ :let
    (⌜ ⟨bindings body⟩)
    (let-expand bindings body))

; Helper function:
(≔ let-expand (λ (bindings body)
  (? (∅? bindings)
     body
     (⌜ ⊙ [first-binding (◁ bindings)]
            [var (◁ first-binding)]
            [expr (◁ (▷ first-binding))]
            [rest-bindings (▷ bindings)]
            (⌞̃ ((λ ((~ var))
                  (~ (let-expand rest-bindings body)))
                (~ expr)))))))

; Usage:
(let ([a #1]
      [b #2]
      [c #3])
  (⊕ a (⊕ b c)))

; Expands to:
((λ (a)
  ((λ (b)
    ((λ (c)
      (⊕ a (⊕ b c)))
     #3))
   #2))
 #1)
```

#### 2.2 `let*` - Sequential bindings (60 min)
```scheme
; Syntax: (let* ([var₁ expr₁] [var₂ expr₂] ...) body)
; Each binding can reference previous ones

(⧉ :let*
    (⌜ ⟨bindings body⟩)
    (let*-expand bindings body))

; Same as let-expand (already sequential)
; Just an alias for clarity
```

**Tests:** 10 tests for binding macros

---

### Phase 3: List Comprehension Macros (2 hours)

#### 3.1 `for-each` - Iterate with side effects (45 min)
```scheme
; Syntax: (for-each func list)
; Execute func on each element

(⧉ :for-each
    (⌜ ⟨func list⟩)
    (⌞̃ (⊕← (λ (acc item) (⌜ ⊙ ((~ func) item) acc))
           ∅
           (~ list))))

; Usage:
(for-each (λ (x) (≋ x)) (list #1 #2 #3))
; Prints: #1, #2, #3
```

#### 3.2 `collect` - Build lists from expressions (75 min)
```scheme
; Syntax: (collect expr for var in list)
; Build list by transforming elements

(⧉ :collect
    (⌜ ⟨expr :for var :in list⟩)
    (⌞̃ (↦ (λ ((~ var)) (~ expr)) (~ list))))

; Usage:
(collect (⊗ x #2) for x in (⋯ #1 #5))
; → ⟨#2 ⟨#4 ⟨#6 ⟨#8 ⟨#10 ∅⟩⟩⟩⟩⟩

; With condition:
(collect x for x in (⋯ #1 #10) if (≡ (% x #2) #0))
; → ⟨#2 ⟨#4 ⟨#6 ⟨#8 ⟨#10 ∅⟩⟩⟩⟩⟩
```

**Tests:** 12 tests for comprehension macros

---

### Phase 4: Utility Macros (1.5 hours)

#### 4.1 `and` / `or` short-circuit (45 min)
```scheme
; Syntax: (and expr₁ expr₂ ...)
; Short-circuit evaluation

(⧉ :and
    (⌜ exprs)
    (and-expand exprs))

(≔ and-expand (λ (exprs)
  (? (∅? exprs)
     #t
     (? (∅? (▷ exprs))
        (◁ exprs)
        (⌞̃ (? (~ (◁ exprs))
              (~ (and-expand (▷ exprs)))
              #f))))))

; Usage:
(and (> x #0) (< x #10) (≡ (% x #2) #0))
```

#### 4.2 `assert` - Better assertions (45 min)
```scheme
; Syntax: (assert condition message)
; More readable than ⊢

(⧉ :assert
    (⌜ ⟨condition message⟩)
    (⌞̃ (⊢ (~ condition) (~ message))))

; Usage:
(assert (> x #0) :must-be-positive)
; Expands to:
(⊢ (> x #0) :must-be-positive)
```

**Tests:** 10 tests for utility macros

---

### Phase 5: Integration and Testing (1.5 hours)

#### 5.1 Create stdlib/macros.scm (30 min)
- All macros in one module
- Organized by category
- Full documentation

#### 5.2 Integration tests (45 min)
- Real-world usage examples
- Macro composition
- Error cases

#### 5.3 Documentation (15 min)
- Update SPEC.md with macro examples
- Add to SESSION_HANDOFF.md
- Create MACROS.md reference

---

## Test Strategy

### Test Categories

| Category | Tests | Description |
|----------|-------|-------------|
| Control flow | 15 | when, unless, cond |
| Bindings | 10 | let, let* |
| Comprehensions | 12 | for-each, collect |
| Utilities | 10 | and, or, assert |
| Integration | 8 | Real-world examples |
| Error cases | 5 | Invalid syntax |
| **TOTAL** | **60** | **Comprehensive** |

### Test File Structure

```
tests/
  test_macro_control.scm      # 15 tests
  test_macro_bindings.scm     # 10 tests
  test_macro_comprehension.scm # 12 tests
  test_macro_utilities.scm    # 10 tests
  test_macro_integration.scm  # 13 tests (8 + 5)
```

---

## Success Criteria

### Must Have ✅

- [ ] `when` macro working
- [ ] `unless` macro working
- [ ] `cond` macro working
- [ ] `let` macro working
- [ ] `for-each` macro working
- [ ] 60+ tests passing
- [ ] All macros in stdlib/macros.scm
- [ ] Documentation updated

### Should Have 📋

- [ ] `collect` comprehension working
- [ ] `and`/`or` short-circuit working
- [ ] Integration tests passing
- [ ] Error handling correct

### Nice to Have 🎯

- [ ] `let*` (sequential bindings)
- [ ] `assert` macro
- [ ] Pattern matching in `let`
- [ ] More comprehension variants

---

## Timeline

| Phase | Duration | Tasks | Deliverable |
|-------|----------|-------|-------------|
| 1. Control flow | 2h | when, unless, cond | 15 tests |
| 2. Bindings | 2h | let, let* | 10 tests |
| 3. Comprehensions | 2h | for-each, collect | 12 tests |
| 4. Utilities | 1.5h | and, or, assert | 10 tests |
| 5. Integration | 1.5h | Module, tests, docs | Complete |
| **TOTAL** | **9h** | **All macros** | **60+ tests** |

Note: Estimated 9 hours with buffer, target 6-8 hours actual.

---

## Dependencies

### Requires (Already Complete)

- ✅ Macro system (⧉) working (Day 33)
- ✅ Quasiquote/unquote (⌞̃, ~) working (Day 32 Part 2)
- ✅ Pattern matching (∇) working (Days 15-19)
- ✅ Standard library basics (Days 20-22)
- ✅ Module system (Days 25-30)

### Enables (Future Work)

1. **More expressive code** - Users write cleaner programs
2. **Additional macros** - Build on these foundations
3. **DSLs** - Domain-specific languages via macros
4. **Compiler optimizations** - Macro expansion at compile time

---

## Risk Assessment

### Low Risk ✅

- Macro system already working (Day 33)
- Quasiquote/unquote proven (Day 32 Part 2)
- Well-understood patterns (Scheme/Lisp tradition)
- Can test incrementally

### Medium Risk ⚠️

- Macro expansion complexity
- Error messages might be confusing
- Need to handle edge cases
- Quasiquote nesting in helpers

### Mitigation

1. **Test each macro individually** before integration
2. **Simple implementations first** - optimize later
3. **Clear error messages** - show original macro call
4. **Comprehensive tests** - 60+ covering edge cases

---

## Post-Day 34 Plans

### Day 35: More Standard Library

**Build on macro foundation:**
```scheme
; Error handling macros
(try expr (catch error handler))

; Loop macros
(while condition body)
(until condition body)
(dotimes n body)

; Pattern matching helpers
(match-let pattern expr body)
```

### Day 36: Parser Improvements

**Enable better macro debugging:**
- Line number tracking
- Source location in macros
- Better error messages
- Macro expansion tracing

---

## Example Usage

### Before Day 34 (Verbose)

```scheme
; Verbose control flow
(≔ process-value (λ (x)
  (? (> x #0)
     (? (< x #100)
        (≋ x)
        ∅)
     ∅)))

; Verbose bindings
((λ (radius)
  ((λ (area)
    ((λ (circumference)
      (≋ area)
      (≋ circumference))
     (⊗ #2 (⊗ #3.14159 radius))))
   (⊗ #3.14159 (⊗ radius radius))))
 #5)
```

### After Day 34 (Clean)

```scheme
; Clean control flow
(≔ process-value (λ (x)
  (when (and (> x #0) (< x #100))
    (≋ x))))

; Clean bindings
(let ([radius #5])
  (let ([area (⊗ #3.14159 (⊗ radius radius))]
        [circumference (⊗ #2 (⊗ #3.14159 radius))])
    (≋ area)
    (≋ circumference)))
```

---

## Conclusion

**Day 34 Goal:** Build ergonomic standard library macros

**Impact:**
- **Usability:** 10x improvement in code clarity
- **Leverage:** Uses Day 33's macro system immediately
- **Foundation:** Enables future macro development

**Critical Path:**
1. Control flow macros (when, unless, cond)
2. Binding macros (let)
3. Comprehension macros (for-each, collect)
4. Integration and testing

**Status:** READY TO IMPLEMENT

---

**Prepared by:** Claude Sonnet 4.5
**Date:** 2026-01-27
**Phase:** Day 34 Planning
**Next:** Implement control flow macros
