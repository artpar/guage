---
Status: CURRENT
Created: 2026-01-27
Purpose: Document known bugs that need fixing
---

# Known Bugs in Guage

## Critical Bugs

### Bug #1: Symbol Lists Fail When Passed to Lambdas (CRITICAL)

**Discovered:** 2026-01-27 (Day 22)
**Severity:** CRITICAL - Blocks use of symbols in list-processing functions
**Status:** UNFIXED

**Description:**
When a list containing symbols (e.g., `(⟨⟩ :a (⟨⟩ :b ∅))`) is passed as a parameter to a lambda function, it gets corrupted or converted to a number (#0). This appears to be a De Bruijn index conversion bug.

**Reproduction:**
```scheme
; Direct access works fine:
(◁ (⟨⟩ :a (⟨⟩ :b ∅)))  ; → :a ✓

; But passing to lambda fails:
((λ (x) x) (⟨⟩ :a (⟨⟩ :b ∅)))  ; → #0 ✗ (should be the list!)

; This causes crashes when trying to access:
((λ (lst) (◁ lst)) (⟨⟩ :a (⟨⟩ :b ∅)))  ; CRASH: assertion failed in prim_car
```

**Workaround:**
Use number lists instead of symbol lists:
```scheme
; Instead of: (⟨⟩ :a (⟨⟩ :b ∅))
; Use: (⟨⟩ #1 (⟨⟩ #2 ∅))
```

**Root Cause:**
Likely issue in:
- De Bruijn conversion (debruijn.c)
- Lambda parameter binding
- Symbol handling in closures

Symbols (starting with `:`) may be confused with variable names or De Bruijn indices during conversion.

**Impact:**
- Cannot use symbols in lists with higher-order functions
- Limits expressiveness of functional programming patterns
- Tests must use numbers instead of meaningful symbol names

**Priority:** HIGH - Should fix before Week 4

**Test Cases:**
- `tests/debug_nth.scm` - Demonstrates crash
- `tests/debug_lambda_param.scm` - Shows corruption
- `tests/debug_symbol_list.scm` - Direct access works

**Related Code:**
- `bootstrap/bootstrap/debruijn.c` - De Bruijn conversion
- `bootstrap/bootstrap/eval.c` - Lambda evaluation
- `bootstrap/bootstrap/cell.c` - Symbol handling

---

## Medium Priority Bugs

(None currently)

---

## Low Priority Bugs

(None currently)

---

## Fixed Bugs

(Historical record of fixed bugs will go here)
