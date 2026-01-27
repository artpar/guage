# Day 15 Implementation Plan: Pattern Matching Foundation

---
**Status:** IN PROGRESS
**Created:** 2026-01-27
**Purpose:** Day 15 detailed implementation plan
---

## Overview

**Goal:** Implement pattern matching foundation - wildcard and literal patterns

**Duration:** 6-8 hours

**Prerequisites:**
- ✅ ADT support working (Day 13)
- ✅ Eval implemented (Day 14)
- ✅ Week 3 roadmap exists

## Pattern Matching Design (Validated from Roadmap)

### Pattern Types to Support

```scheme
; Literals (TODAY - Day 15)
#42                    ; Number literal
#t, #f                 ; Boolean literals
:symbol                ; Symbol literal
∅                      ; Nil literal

; Wildcard (TODAY - Day 15)
_                      ; Match anything, don't bind

; Variables (Day 16)
x                      ; Bind to variable

; Pairs (Day 17)
(⟨⟩ a b)              ; Match pair, bind head/tail
(⟨⟩ x _)              ; Match pair, ignore tail
(⟨⟩ _ (⟨⟩ y _))       ; Nested pairs

; ADT Patterns (Day 18-19)
[:Nil]                 ; Match Nil variant
[:Cons h t]            ; Match Cons, bind fields
```

### Syntax

```scheme
; Match expression
(∇ expr
  [pattern₁ result₁]
  [pattern₂ result₂]
  ...
  [pattern_n result_n])

; Examples
(∇ #42
  [#42 :matched]
  [_ :not-matched])     ; → :matched

(∇ #99
  [#42 :matched]
  [_ :not-matched])     ; → :not-matched
```

## Implementation Strategy

### Phase 1: Core Infrastructure (2 hours)

**File:** `bootstrap/bootstrap/pattern.c` (new file)
**File:** `bootstrap/bootstrap/pattern.h` (new file)

**Data Structures:**
```c
/* Pattern matching result */
typedef struct {
    bool success;           /* Did pattern match? */
    Cell* bindings;         /* Alist: (var . value) pairs */
} MatchResult;

/* Pattern type */
typedef enum {
    PATTERN_WILDCARD,       /* _ */
    PATTERN_LITERAL,        /* #42, :foo, ∅, #t */
    PATTERN_VAR,            /* x (Day 16) */
    PATTERN_PAIR,           /* (⟨⟩ a b) (Day 17) */
    PATTERN_ADT             /* [:Cons h t] (Day 18) */
} PatternType;
```

**Core Functions:**
```c
/* Try to match value against pattern */
MatchResult try_match(Cell* value, Cell* pattern);

/* Evaluate match expression */
Cell* eval_match(Cell* expr, Cell* clauses, EvalContext* ctx);
```

### Phase 2: Wildcard Pattern (1 hour)

**Pattern:** `_` matches anything

**Implementation:**
```c
MatchResult try_match(Cell* value, Cell* pattern) {
    // Wildcard always matches
    if (is_symbol(pattern) && strcmp(symbol_name(pattern), "_") == 0) {
        return (MatchResult){.success = true, .bindings = NULL};
    }

    // Other patterns (to be implemented)
    return (MatchResult){.success = false, .bindings = NULL};
}
```

**Test Cases:**
```scheme
(⊨ :wildcard-matches-number #t
   (≡ (∇ #42 [_ :ok]) :ok))

(⊨ :wildcard-matches-symbol #t
   (≡ (∇ :foo [_ :ok]) :ok))

(⊨ :wildcard-matches-nil #t
   (≡ (∇ ∅ [_ :ok]) :ok))
```

### Phase 3: Literal Patterns (2-3 hours)

**Patterns:** Numbers, symbols, booleans, nil

**Implementation:**
```c
MatchResult try_match(Cell* value, Cell* pattern) {
    // Wildcard (already done)
    if (is_symbol(pattern) && strcmp(symbol_name(pattern), "_") == 0) {
        return (MatchResult){.success = true, .bindings = NULL};
    }

    // Number literal
    if (is_number(pattern)) {
        if (is_number(value) && number_value(value) == number_value(pattern)) {
            return (MatchResult){.success = true, .bindings = NULL};
        }
        return (MatchResult){.success = false, .bindings = NULL};
    }

    // Boolean literal
    if (is_bool(pattern)) {
        if (is_bool(value) && bool_value(value) == bool_value(pattern)) {
            return (MatchResult){.success = true, .bindings = NULL};
        }
        return (MatchResult){.success = false, .bindings = NULL};
    }

    // Symbol literal
    if (is_symbol(pattern)) {
        if (is_symbol(value) && strcmp(symbol_name(value), symbol_name(pattern)) == 0) {
            return (MatchResult){.success = true, .bindings = NULL};
        }
        return (MatchResult){.success = false, .bindings = NULL};
    }

    // Nil literal
    if (is_nil(pattern)) {
        if (is_nil(value)) {
            return (MatchResult){.success = true, .bindings = NULL};
        }
        return (MatchResult){.success = false, .bindings = NULL};
    }

    return (MatchResult){.success = false, .bindings = NULL};
}
```

**Test Cases:**
```scheme
; Number patterns
(⊨ :number-match #t
   (≡ (∇ #42 [#42 :matched] [_ :not-matched]) :matched))

(⊨ :number-no-match #t
   (≡ (∇ #99 [#42 :matched] [_ :not-matched]) :not-matched))

; Boolean patterns
(⊨ :bool-true #t
   (≡ (∇ #t [#t :true] [#f :false]) :true))

(⊨ :bool-false #t
   (≡ (∇ #f [#t :true] [#f :false]) :false))

; Symbol patterns
(⊨ :symbol-match #t
   (≡ (∇ :foo [:foo :matched] [_ :not-matched]) :matched))

(⊨ :symbol-no-match #t
   (≡ (∇ :bar [:foo :matched] [_ :not-matched]) :not-matched))

; Nil pattern
(⊨ :nil-match #t
   (≡ (∇ ∅ [∅ :empty] [_ :not-empty]) :empty))
```

### Phase 4: Match Primitive (2 hours)

**Primitive:** `∇` (match)

**Implementation:**
```c
/* In primitives.c */
Cell* prim_match(Cell* args) {
    // Get arguments
    Cell* expr = arg1(args);   // Expression to match
    Cell* clauses = arg2(args); // List of [pattern result] pairs

    // Get current context
    EvalContext* ctx = eval_get_current_context();
    if (!ctx) {
        return cell_error("no-context", expr);
    }

    // Evaluate expression
    Cell* value = eval(ctx, expr);
    if (is_error(value)) {
        return value;
    }

    // Try each clause
    Cell* current = clauses;
    while (!is_nil(current)) {
        if (!is_pair(current)) {
            return cell_error("invalid-clauses", clauses);
        }

        Cell* clause = car(current);
        if (!is_pair(clause) || !is_pair(cdr(clause))) {
            return cell_error("invalid-clause", clause);
        }

        Cell* pattern = car(clause);
        Cell* result = car(cdr(clause));

        // Try to match
        MatchResult match = try_match(value, pattern);
        if (match.success) {
            // For now, no variable bindings
            // Just evaluate result
            return eval(ctx, result);
        }

        current = cdr(current);
    }

    // No match found
    return cell_error("no-match", value);
}
```

**Register Primitive:**
```c
/* In primitives.c init function */
env_define(env, symbol_create("∇"),
           primitive_create(prim_match, "∇"));
```

## Test Suite

**File:** `tests/test_pattern_matching_day15.scm`

```scheme
;;; Day 15 Pattern Matching Tests
;;; Wildcard and Literal Patterns

;; ==================
;; Wildcard Tests
;; ==================

(⊨ :wildcard-number #t
   (≡ (∇ #42 [_ :ok]) :ok))

(⊨ :wildcard-symbol #t
   (≡ (∇ :foo [_ :ok]) :ok))

(⊨ :wildcard-bool #t
   (≡ (∇ #t [_ :ok]) :ok))

(⊨ :wildcard-nil #t
   (≡ (∇ ∅ [_ :ok]) :ok))

(⊨ :wildcard-pair #t
   (≡ (∇ (⟨⟩ #1 #2) [_ :ok]) :ok))

;; ==================
;; Number Patterns
;; ==================

(⊨ :number-exact-match #t
   (≡ (∇ #42 [#42 :yes] [_ :no]) :yes))

(⊨ :number-no-match #t
   (≡ (∇ #99 [#42 :yes] [_ :no]) :no))

(⊨ :number-zero #t
   (≡ (∇ #0 [#0 :zero] [_ :nonzero]) :zero))

(⊨ :number-negative #t
   (≡ (∇ #-5 [#-5 :neg] [_ :pos]) :neg))

;; ==================
;; Boolean Patterns
;; ==================

(⊨ :bool-true-match #t
   (≡ (∇ #t [#t :true] [#f :false]) :true))

(⊨ :bool-false-match #t
   (≡ (∇ #f [#t :true] [#f :false]) :false))

(⊨ :bool-with-wildcard #t
   (≡ (∇ #t [#t :yes] [_ :no]) :yes))

;; ==================
;; Symbol Patterns
;; ==================

(⊨ :symbol-exact-match #t
   (≡ (∇ :foo [:foo :matched] [_ :not]) :matched))

(⊨ :symbol-no-match #t
   (≡ (∇ :bar [:foo :yes] [_ :no]) :no))

(⊨ :symbol-multiple-cases #t
   (≡ (∇ :baz [:foo :f] [:bar :b] [:baz :z] [_ :none]) :z))

;; ==================
;; Nil Pattern
;; ==================

(⊨ :nil-match #t
   (≡ (∇ ∅ [∅ :empty] [_ :not-empty]) :empty))

(⊨ :nil-no-match #t
   (≡ (∇ #42 [∅ :empty] [_ :not-empty]) :not-empty))

;; ==================
;; Multiple Clauses
;; ==================

(⊨ :first-clause-wins #t
   (≡ (∇ #42 [#42 :first] [#42 :second]) :first))

(⊨ :fallthrough-to-wildcard #t
   (≡ (∇ #99 [#42 :no] [#43 :no] [_ :yes]) :yes))

;; ==================
;; Error Cases
;; ==================

(⊨ :no-match-error #t
   (⚠? (∇ #42 [#43 :no])))

(⊨ :invalid-pattern-structure #t
   (⚠? (∇ #42 #43)))  ; Not a list of clauses
```

## Timeline

**Total: 6-8 hours**

| Time | Task | Duration |
|------|------|----------|
| 09:00-09:30 | Design review & validation | 30 min |
| 09:30-11:30 | Core infrastructure (pattern.c/h) | 2 hours |
| 11:30-12:30 | Wildcard pattern | 1 hour |
| 12:30-13:30 | Lunch break | - |
| 13:30-16:00 | Literal patterns | 2.5 hours |
| 16:00-17:00 | Match primitive & integration | 1 hour |
| 17:00-17:30 | Testing & documentation | 30 min |

## Success Criteria

**Must Have:**
- [ ] pattern.c and pattern.h files created
- [ ] try_match() function working
- [ ] Wildcard pattern (_) working
- [ ] Literal patterns working (numbers, bools, symbols, nil)
- [ ] ∇ primitive registered
- [ ] 25+ tests passing
- [ ] No memory leaks

**Documentation:**
- [ ] Update SPEC.md with ∇ primitive
- [ ] Update SESSION_HANDOFF.md with progress
- [ ] Add pattern.c/h to build system

## Next Steps (Day 16)

- Variable patterns (bind values)
- Pattern environment management
- More comprehensive tests
- Start structural equality (≗)

---

**Status:** Ready to implement
**Estimated Completion:** End of Day 15
**Risk Level:** LOW (straightforward implementation)
