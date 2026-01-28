---
Status: CURRENT
Created: 2026-01-28
Updated: 2026-01-28
Purpose: Implementation guide for completing trampoline evaluator (quasiquote)
---

# Trampoline Quasiquote Implementation Guide

**Goal:** Implement quasiquote (⌞̃) and unquote (~) special forms in trampoline evaluator to achieve 100% test coverage.

**Current Status:** 28/33 tests passing (85%) | Missing: quasiquote support
**Estimated Time:** 1-2 hours
**Expected Outcome:** 33/33 tests passing (100%) with trampoline

---

## Background

### What is Quasiquote?

Quasiquote (⌞̃) is like quote (⌜) but supports **selective evaluation** via unquote (~):

```scheme
; Quote - no evaluation
(⌜ (⊕ #1 #2))  ; → (⊕ #1 #2)

; Quasiquote - evaluates unquoted parts
(≔ x #42)
(⌞̃ (⊕ #1 (~ x)))  ; → (⊕ #1 #42)
```

**Use cases:**
- Code generation
- Macro expansion
- Template functions
- Metaprogramming

### Why It's Needed

5 tests fail without quasiquote:
- `test_quasiquote.test` - Direct quasiquote tests
- `test_stdlib_macros.test` - Macros use quasiquote internally
- `test_parser.test` - One test uses quasiquote for code generation
- 2 other tests with indirect quasiquote usage

### Reference Implementation

Quasiquote is already implemented in the **recursive evaluator** at `bootstrap/eval.c:846-870`:

```c
/* Evaluate quasiquote expression (supports unquote)
 * Recursively walks the expression, evaluating unquoted parts */
static Cell* eval_quasiquote(EvalContext* ctx, Cell* env, Cell* expr) {
    /* If expr is a pair starting with ~, evaluate it */
    if (cell_is_pair(expr)) {
        Cell* first = cell_car(expr);
        if (cell_is_symbol(first)) {
            const char* sym = cell_get_symbol(first);
            if (strcmp(sym, "~") == 0 || strcmp(sym, "unquote") == 0) {
                Cell* arg = cell_car(cell_cdr(expr));
                if (!arg) {
                    return cell_error("quasiquote-error",
                                    cell_from_string("unquote requires argument"));
                }
                return eval(ctx, env, arg);  /* Evaluate unquoted part */
            }
        }

        /* Recursively process car and cdr */
        Cell* new_car = eval_quasiquote(ctx, env, first);
        cell_retain(new_car);
        Cell* new_cdr = eval_quasiquote(ctx, env, rest);
        /* ... build new pair ... */
    }

    /* Atoms and other types - return as-is */
    cell_retain(expr);
    return expr;
}
```

**Key insight:** Quasiquote walks the expression tree, evaluating only the parts marked with `~`.

---

## Implementation Plan

### Step 1: Add EVAL_QUASIQUOTE Frame State (~5 min)

**File:** `bootstrap/trampoline.h`

**Add to FrameState enum:**

```c
typedef enum {
    EVAL_EXPR,      /* Evaluate an expression */
    EVAL_APPLY,     /* Apply function to arguments */
    EVAL_ARGS,      /* Evaluate argument list */
    EVAL_RETURN,    /* Return value to parent frame */
    EVAL_IF,        /* Conditional branch */
    EVAL_DEFINE,    /* Global definition */
    EVAL_QUOTE,     /* Quote - return literal */
    EVAL_MACRO,     /* Macro expansion */
    EVAL_QUASIQUOTE, /* NEW: Quasiquote with unquote support */
} FrameState;
```

**Add frame creation function:**

```c
/* Create quasiquote evaluation frame */
StackFrame* frame_create_quasiquote(Cell* expr, Cell* env);
```

### Step 2: Implement frame_create_quasiquote (~10 min)

**File:** `bootstrap/trampoline.c`

**Location:** After other frame_create_* functions

```c
/* Create EVAL_QUASIQUOTE frame */
StackFrame* frame_create_quasiquote(Cell* expr, Cell* env) {
    StackFrame* frame = (StackFrame*)malloc(sizeof(StackFrame));
    frame->state = EVAL_QUASIQUOTE;
    frame->expr = expr;
    cell_retain(expr);
    frame->env = env;
    if (env) cell_retain(env);
    frame->value = NULL;
    frame->func = NULL;
    frame->arg_index = 0;
    frame->accumulated_args = NULL;
    frame->then_branch = NULL;
    frame->else_branch = NULL;
    frame->symbol = NULL;
    return frame;
}
```

### Step 3: Implement handle_eval_quasiquote (~30 min)

**File:** `bootstrap/trampoline.c`

**Location:** After handle_eval_quote, before handle_eval_apply

**Implementation strategy:**

The trampoline version needs to be **non-recursive**. Instead of recursively calling itself, it uses a state machine:

```c
/*
 * handle_eval_quasiquote - Evaluate quasiquote expression
 *
 * Quasiquote is like quote but supports unquote (~) for selective evaluation.
 *
 * State machine:
 *   1. value == NULL: Check if expr is unquote (~) or needs recursion
 *   2. value != NULL: Received result from sub-evaluation, continue processing
 *
 * For pairs: Process car, then cdr, then combine
 * For atoms: Return as-is (like quote)
 */
static bool handle_eval_quasiquote(EvalStack* stack, StackFrame* frame) {
    Cell* expr = frame->expr;
    Cell* env = frame->env;

    /* Case 1: Unquote (~) - evaluate the expression */
    if (cell_is_pair(expr)) {
        Cell* first = cell_car(expr);
        if (cell_is_symbol(first)) {
            const char* sym = cell_get_symbol(first);
            if (strcmp(sym, "~") == 0 || strcmp(sym, "unquote") == 0) {
                /* Get argument */
                Cell* rest = cell_cdr(expr);
                if (!cell_is_pair(rest)) {
                    Cell* err = cell_error("quasiquote-error",
                                          cell_from_string("unquote requires argument"));
                    StackFrame* ret = frame_create_return(err);
                    stack_push(stack, ret);
                    return true;  /* Frame done */
                }

                Cell* arg = cell_car(rest);

                /* Evaluate the unquoted expression */
                StackFrame* eval_frame = frame_create_eval(arg, env);
                stack_push(stack, eval_frame);
                return false;  /* Wait for result */
            }
        }

        /* Case 2: Regular pair - need to process car and cdr */
        /* This requires a multi-step state machine:
         * - Evaluate car (quasiquote)
         * - Evaluate cdr (quasiquote)
         * - Combine results into new pair
         *
         * For simplicity in trampoline, we can push frames for car/cdr
         * and handle combination in a helper state.
         */

        /* SIMPLIFIED APPROACH: For pairs without unquote, return as-is */
        /* A full implementation would recursively process car/cdr */
        /* This handles most common cases */
        cell_retain(expr);
        StackFrame* ret = frame_create_return(expr);
        stack_push(stack, ret);
        return true;
    }

    /* Case 3: Atoms - return as-is (like quote) */
    cell_retain(expr);
    StackFrame* ret = frame_create_return(expr);
    stack_push(stack, ret);
    return true;  /* Frame done */
}
```

**Note:** The above is a simplified version that handles:
- Unquote at top level: `(⌞̃ (~ expr))`
- Direct atoms: `(⌞̃ #42)`
- Pairs without unquote: `(⌞̃ (⊕ #1 #2))`

For a **complete implementation** that handles nested unquotes like `(⌞̃ (⊕ (~ x) #2))`, you'd need to:
1. Process car of pair
2. Process cdr of pair
3. Combine results

This requires additional state tracking (similar to handle_eval_args).

### Step 4: Add Quasiquote to handle_eval_expr (~10 min)

**File:** `bootstrap/trampoline.c`

**Location:** In handle_eval_expr, after the quote check, before the define check

```c
/* In handle_eval_expr, in the pair/symbol check section: */

/* ⌜ - quote (return expression without evaluation) */
if (strcmp(sym, "⌜") == 0) {
    Cell* arg = cell_car(cell_cdr(expr));
    StackFrame* quote_frame = frame_create_quote(arg);
    stack_push(stack, quote_frame);
    return true;
}

/* ⌞̃ - quasiquote (quote with unquote support) */
if (strcmp(sym, "⌞̃") == 0 || strcmp(sym, "quasiquote") == 0) {
    Cell* arg = cell_car(cell_cdr(expr));
    StackFrame* qq_frame = frame_create_quasiquote(arg, env);
    stack_push(stack, qq_frame);
    return true;
}

/* ≔ - define */
if (strcmp(sym, "≔") == 0) {
    /* ... existing code ... */
}
```

### Step 5: Add to trampoline_loop Switch (~5 min)

**File:** `bootstrap/trampoline.c`

**Location:** In trampoline_loop, in the switch statement

```c
void trampoline_loop(EvalStack* stack) {
    while (!stack_is_empty(stack)) {
        StackFrame* frame = stack_pop(stack);
        bool frame_done = false;

        switch (frame->state) {
            case EVAL_EXPR:
                frame_done = handle_eval_expr(stack, frame);
                break;
            case EVAL_APPLY:
                frame_done = handle_eval_apply(stack, frame);
                break;
            case EVAL_ARGS:
                frame_done = handle_eval_args(stack, frame);
                break;
            case EVAL_RETURN:
                frame_done = handle_eval_return(stack, frame);
                break;
            case EVAL_IF:
                frame_done = handle_eval_if(stack, frame);
                break;
            case EVAL_DEFINE:
                frame_done = handle_eval_define(stack, frame);
                break;
            case EVAL_QUOTE:
                frame_done = handle_eval_quote(stack, frame);
                break;
            case EVAL_MACRO:
                frame_done = handle_eval_macro(stack, frame);
                break;
            case EVAL_QUASIQUOTE:  /* NEW */
                frame_done = handle_eval_quasiquote(stack, frame);
                break;
        }

        /* ... rest of loop ... */
    }
}
```

### Step 6: Test and Verify (~20 min)

**Enable trampoline:**

```bash
# Edit Makefile, change:
CFLAGS = -Wall -Wextra -std=c11 -g -O2 -fno-omit-frame-pointer
# To:
CFLAGS = -Wall -Wextra -std=c11 -g -O2 -fno-omit-frame-pointer -DUSE_TRAMPOLINE=1
```

**Run tests:**

```bash
make clean && make
./bootstrap/guage < bootstrap/tests/test_quasiquote.test
make test
```

**Expected:**
- test_quasiquote.test passes
- 28/33 → 33/33 tests passing

**If tests fail:**
1. Check symbol comparison (⌞̃ vs "quasiquote")
2. Verify frame creation is correct
3. Add debug prints to see what's being evaluated
4. Compare behavior with recursive eval

---

## Advanced: Full Recursive Quasiquote (~30 min extra)

The simplified version above handles common cases. For **full recursive quasiquote** (handles `(⌞̃ (⊕ (~ x) (~ y)))`), you need a more complex state machine.

**Approach:** Use accumulated_args to track:
- Processing state (car done? cdr done?)
- Partial results

**Pseudo-code:**

```c
static bool handle_eval_quasiquote(EvalStack* stack, StackFrame* frame) {
    /* State 1: Check for unquote at top level */
    if (is_unquote(frame->expr)) {
        /* Evaluate and return */
    }

    /* State 2: For pairs, use multi-step processing */
    if (cell_is_pair(frame->expr)) {
        if (frame->accumulated_args == NULL) {
            /* Step 2a: Process car */
            frame->accumulated_args = cell_nil();  /* Mark as started */
            StackFrame* car_frame = frame_create_quasiquote(cell_car(expr), env);
            stack_push(stack, car_frame);
            return false;  /* Wait for car result */
        } else if (frame->value != NULL && frame->arg_index == 0) {
            /* Step 2b: Got car result, now process cdr */
            frame->accumulated_args = frame->value;  /* Save car result */
            frame->value = NULL;
            frame->arg_index = 1;
            StackFrame* cdr_frame = frame_create_quasiquote(cell_cdr(expr), env);
            stack_push(stack, cdr_frame);
            return false;  /* Wait for cdr result */
        } else if (frame->value != NULL && frame->arg_index == 1) {
            /* Step 2c: Got both car and cdr, combine */
            Cell* new_car = frame->accumulated_args;
            Cell* new_cdr = frame->value;
            Cell* result = cell_cons(new_car, new_cdr);
            /* Return combined result */
            StackFrame* ret = frame_create_return(result);
            stack_push(stack, ret);
            return true;  /* Frame done */
        }
    }

    /* State 3: Atoms - return as-is */
    /* ... */
}
```

This is more complex but handles all cases correctly.

---

## Checklist

- [ ] Add EVAL_QUASIQUOTE to FrameState enum
- [ ] Add frame_create_quasiquote declaration to trampoline.h
- [ ] Implement frame_create_quasiquote function
- [ ] Implement handle_eval_quasiquote (simplified or full)
- [ ] Add quasiquote check to handle_eval_expr
- [ ] Add EVAL_QUASIQUOTE case to trampoline_loop switch
- [ ] Update Makefile to enable USE_TRAMPOLINE=1
- [ ] Test with test_quasiquote.test
- [ ] Run full test suite
- [ ] Verify 33/33 passing
- [ ] Update SESSION_HANDOFF.md with completion
- [ ] Commit changes

---

## Success Criteria

**Before:** 28/33 tests passing with trampoline (85%)
**After:** 33/33 tests passing with trampoline (100%)

**Deliverables:**
- Quasiquote special form implemented
- All tests passing
- Trampoline evaluator production-ready
- Documentation updated

**Time:** 1-2 hours total

---

## Notes

- Start with simplified version (handles top-level unquote)
- If needed, extend to full recursive version
- Reference eval.c implementation for logic
- Test incrementally as you build
- Use debug prints to trace execution

**Key difference from recursive version:**
- Recursive: Calls itself directly
- Trampoline: Pushes frames and returns, loop handles iteration

This is the **final piece** to complete the trampoline evaluator! 🎯
