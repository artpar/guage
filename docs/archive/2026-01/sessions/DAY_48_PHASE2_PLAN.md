---
Status: ARCHIVED
Created: 2026-01-28
Purpose: Detailed implementation plan for Trampoline Phase 2 (State Handlers)
---

# Day 48: Trampoline Phase 2 - State Handlers

## Overview

**Goal:** Implement evaluation logic for all frame states (the "handlers")

**Status:** Phase 1 COMPLETE ✅ (data structures + 10 tests passing)

**This Phase:** Implement the actual evaluation logic that processes frames

## What Phase 1 Gave Us

✅ **Data Structures:**
- `StackFrame` - Represents one computation step
- `EvalStack` - Manages frame stack with dynamic growth
- 7 frame creation functions
- Stack operations (push, pop, peek, size)
- Reference counting working correctly
- Debug utilities (print functions)

✅ **Testing Infrastructure:**
- 10 C unit tests passing (100%)
- Test framework in `bootstrap/test_trampoline.c`
- Clean memory (no leaks)

## What Phase 2 Will Add

The **evaluation logic** - the handlers that process each frame type:

1. **handle_eval_expr()** - Evaluate expressions
   - Atoms (numbers, booleans, nil, errors) → return immediately
   - Symbols → lookup in environment
   - Pairs → function application
   - Special forms (λ, ≔, ?, ⌜)

2. **handle_eval_apply()** - Apply functions
   - Primitives → call C function directly
   - Lambdas → create new environment + evaluate body
   - Closures → use captured environment

3. **handle_eval_args()** - Evaluate arguments
   - Process left-to-right
   - Accumulate evaluated values
   - Handle empty list

4. **handle_eval_return()** - Return value propagation
   - Pass result to parent frame
   - Set final result if no parent

5. **handle_eval_if()** - Conditional branching
   - Evaluate condition
   - Choose then/else branch

6. **handle_eval_define()** - Global definition
   - Evaluate value expression
   - Define in environment

7. **handle_eval_quote()** - Quote (literal)
   - Return expression without evaluation

## Implementation Strategy

### Test-First Approach

For each handler:
1. Write C unit test first (should fail)
2. Implement minimal handler
3. Run test (should pass)
4. Add edge case tests
5. Refine implementation
6. Move to next handler

### Order of Implementation

**Phase 2A: Core Evaluation (2 hours)**
1. `handle_eval_expr()` - Base case (atoms, symbols)
2. `handle_eval_return()` - Value propagation
3. Test together with simple expressions

**Phase 2B: Function Application (2 hours)**
4. `handle_eval_apply()` - Apply primitives + lambdas
5. `handle_eval_args()` - Argument evaluation
6. Test together with function calls

**Phase 2C: Special Forms (2 hours)**
7. `handle_eval_quote()` - Quote
8. `handle_eval_if()` - Conditionals
9. `handle_eval_define()` - Definitions
10. Test all special forms

**Total: ~6 hours**

## Detailed Implementation Plan

### Phase 2A: Core Evaluation (2 hours)

#### Step 1: handle_eval_expr() - Atoms & Symbols

**Test (add to test_trampoline.c):**
```c
/* Test atom evaluation - numbers */
void test_eval_atom_number(void) {
    EvalStack* stack = stack_create();
    Cell* expr = cell_number(42);
    Cell* env = NULL; /* Simple case, no env needed */

    StackFrame* frame = frame_create_eval(expr, env);
    handle_eval_expr(frame, stack);

    /* Should push EVAL_RETURN with the number */
    StackFrame* ret_frame = stack_pop(stack);
    assert(ret_frame->state == EVAL_RETURN);
    assert(cell_is_number(ret_frame->value));
    assert(cell_number_value(ret_frame->value) == 42);

    frame_destroy(frame);
    frame_destroy(ret_frame);
    stack_destroy(stack);
    printf("✓ test_eval_atom_number\n");
}

/* Test symbol lookup */
void test_eval_symbol(void) {
    EvalStack* stack = stack_create();
    Env* env = env_create();

    /* Define x = 42 */
    Cell* symbol = symbol_create("x");
    Cell* value = cell_number(42);
    env_define(env, symbol, value);

    /* Evaluate x */
    StackFrame* frame = frame_create_eval(symbol, (Cell*)env);
    handle_eval_expr(frame, stack);

    /* Should push EVAL_RETURN with value 42 */
    StackFrame* ret_frame = stack_pop(stack);
    assert(ret_frame->state == EVAL_RETURN);
    assert(cell_is_number(ret_frame->value));
    assert(cell_number_value(ret_frame->value) == 42);

    frame_destroy(frame);
    frame_destroy(ret_frame);
    stack_destroy(stack);
    env_destroy(env);
    printf("✓ test_eval_symbol\n");
}
```

**Implementation (add to trampoline.c):**
```c
void handle_eval_expr(StackFrame* frame, EvalStack* stack) {
    Cell* expr = frame->expr;
    Cell* env = frame->env;

    /* Atoms - self-evaluating */
    if (cell_is_number(expr) || cell_is_boolean(expr) ||
        cell_is_nil(expr) || cell_is_error(expr) ||
        cell_is_string(expr)) {
        /* Push return frame with the atom */
        StackFrame* ret = frame_create_return(expr);
        stack_push(stack, ret);
        return;
    }

    /* Keywords - self-evaluating */
    if (cell_is_keyword(expr)) {
        StackFrame* ret = frame_create_return(expr);
        stack_push(stack, ret);
        return;
    }

    /* Symbol - lookup in environment */
    if (cell_is_symbol(expr)) {
        Env* e = (Env*)env;
        Cell* value = env_lookup(e, expr);
        if (!value) {
            /* Symbol not found - return error */
            Cell* err = cell_error(symbol_create("symbol-not-found"), expr);
            StackFrame* ret = frame_create_return(err);
            stack_push(stack, ret);
            return;
        }
        StackFrame* ret = frame_create_return(value);
        stack_push(stack, ret);
        return;
    }

    /* Pair - function application or special form */
    if (cell_is_pair(expr)) {
        /* TODO: Check for special forms first */
        /* TODO: Then handle function application */

        /* For now, error */
        Cell* err = cell_error(symbol_create("not-implemented"),
                               symbol_create("pair-eval"));
        StackFrame* ret = frame_create_return(err);
        stack_push(stack, ret);
        return;
    }

    /* Unknown type - error */
    Cell* err = cell_error(symbol_create("invalid-expr"), expr);
    StackFrame* ret = frame_create_return(err);
    stack_push(stack, ret);
}
```

#### Step 2: handle_eval_return() - Value Propagation

**Test:**
```c
void test_eval_return_to_empty_stack(void) {
    EvalStack* stack = stack_create();
    Cell* value = cell_number(42);

    StackFrame* frame = frame_create_return(value);
    handle_eval_return(frame, stack);

    /* Should set final result */
    assert(stack_get_result(stack) == value);
    assert(stack_is_empty(stack));

    frame_destroy(frame);
    stack_destroy(stack);
    printf("✓ test_eval_return_to_empty_stack\n");
}

void test_eval_return_to_parent(void) {
    EvalStack* stack = stack_create();

    /* Parent frame waiting for result */
    StackFrame* parent = frame_create_eval(NULL, NULL);
    parent->state = EVAL_RETURN; /* Waiting for value */
    stack_push(stack, parent);

    /* Child returns value */
    Cell* value = cell_number(42);
    StackFrame* child = frame_create_return(value);
    handle_eval_return(child, stack);

    /* Parent should receive value */
    assert(parent->value == value);

    frame_destroy(child);
    stack_destroy(stack);
    printf("✓ test_eval_return_to_parent\n");
}
```

**Implementation:**
```c
void handle_eval_return(StackFrame* frame, EvalStack* stack) {
    Cell* value = frame->value;

    if (stack_is_empty(stack)) {
        /* No parent - this is the final result */
        stack_set_result(stack, value);
        return;
    }

    /* Get parent frame and pass value to it */
    StackFrame* parent = stack_peek(stack);

    /* Store value in parent */
    if (parent->value) {
        cell_release(parent->value);
    }
    parent->value = value;
    if (value) {
        cell_retain(value);
    }

    /* Parent will process value based on its state */
}
```

**Checkpoint:** Run tests, verify atoms and symbols work end-to-end.

---

### Phase 2B: Function Application (2 hours)

#### Step 3: handle_eval_apply() - Apply Functions

**Test:**
```c
void test_eval_apply_primitive(void) {
    EvalStack* stack = stack_create();
    Env* env = env_create();
    init_primitives(env); /* Load + primitive */

    /* Get + primitive */
    Cell* plus = env_lookup(env, symbol_create("⊕"));
    assert(cell_is_primitive(plus));

    /* Args: (1 2) */
    Cell* args = cell_cons(cell_number(1),
                          cell_cons(cell_number(2), cell_nil()));

    StackFrame* frame = frame_create_apply(plus, args, (Cell*)env);
    handle_eval_apply(frame, stack);

    /* Should push EVAL_RETURN with result 3 */
    StackFrame* ret = stack_pop(stack);
    assert(ret->state == EVAL_RETURN);
    assert(cell_is_number(ret->value));
    assert(cell_number_value(ret->value) == 3);

    frame_destroy(frame);
    frame_destroy(ret);
    stack_destroy(stack);
    env_destroy(env);
    printf("✓ test_eval_apply_primitive\n");
}

void test_eval_apply_lambda(void) {
    EvalStack* stack = stack_create();
    Env* env = env_create();

    /* Lambda: (λ (⊕ 0 #1)) - add 1 to argument */
    Cell* body = cell_cons(symbol_create("⊕"),
                          cell_cons(cell_number(0), /* De Bruijn 0 */
                          cell_cons(cell_number(1),
                                   cell_nil())));
    Cell* lambda = cell_lambda(body, env, 1);

    /* Args: (5) */
    Cell* args = cell_cons(cell_number(5), cell_nil());

    StackFrame* frame = frame_create_apply(lambda, args, (Cell*)env);
    handle_eval_apply(frame, stack);

    /* Should push EVAL_EXPR for body in new environment */
    StackFrame* next = stack_peek(stack);
    assert(next->state == EVAL_EXPR);

    frame_destroy(frame);
    stack_destroy(stack);
    env_destroy(env);
    printf("✓ test_eval_apply_lambda\n");
}
```

**Implementation:**
```c
void handle_eval_apply(StackFrame* frame, EvalStack* stack) {
    Cell* func = frame->func;
    Cell* args = frame->expr;
    Cell* env = frame->env;

    /* Primitive - call directly */
    if (cell_is_primitive(func)) {
        PrimitiveFn fn = cell_primitive_fn(func);
        Cell* result = fn(args, (Env*)env);

        StackFrame* ret = frame_create_return(result);
        stack_push(stack, ret);
        return;
    }

    /* Lambda - apply with De Bruijn indices */
    if (cell_is_lambda(func)) {
        Env* closure_env = cell_get_env(func);
        Cell* body = cell_get_body(func);
        int arity = cell_get_arity(func);

        /* Create new environment with arguments */
        Env* new_env = env_extend(closure_env, args, arity);

        /* Evaluate body in new environment */
        StackFrame* eval = frame_create_eval(body, (Cell*)new_env);
        stack_push(stack, eval);
        return;
    }

    /* Not a function - error */
    Cell* err = cell_error(symbol_create("not-a-function"), func);
    StackFrame* ret = frame_create_return(err);
    stack_push(stack, ret);
}
```

#### Step 4: handle_eval_args() - Argument Evaluation

**Test:**
```c
void test_eval_args_empty(void) {
    EvalStack* stack = stack_create();
    Cell* args = cell_nil();
    Cell* env = NULL;

    StackFrame* frame = frame_create_args(args, env, 0);
    handle_eval_args(frame, stack);

    /* Should push EVAL_RETURN with empty list */
    StackFrame* ret = stack_pop(stack);
    assert(ret->state == EVAL_RETURN);
    assert(cell_is_nil(ret->value));

    frame_destroy(frame);
    frame_destroy(ret);
    stack_destroy(stack);
    printf("✓ test_eval_args_empty\n");
}

void test_eval_args_single(void) {
    EvalStack* stack = stack_create();

    /* Args: (42) */
    Cell* args = cell_cons(cell_number(42), cell_nil());
    Cell* env = NULL;

    StackFrame* frame = frame_create_args(args, env, 0);
    handle_eval_args(frame, stack);

    /* Should push EVAL_EXPR for first arg */
    StackFrame* next = stack_peek(stack);
    assert(next->state == EVAL_EXPR);
    assert(cell_is_number(next->expr));

    frame_destroy(frame);
    stack_destroy(stack);
    printf("✓ test_eval_args_single\n");
}
```

**Implementation:**
```c
void handle_eval_args(StackFrame* frame, EvalStack* stack) {
    Cell* args = frame->expr;
    Cell* accumulated = frame->accumulated_args;

    if (cell_is_nil(args)) {
        /* All arguments evaluated - return accumulated list */
        StackFrame* ret = frame_create_return(accumulated);
        stack_push(stack, ret);
        return;
    }

    /* Get first argument */
    Cell* arg = cell_car(args);
    Cell* rest = cell_cdr(args);

    /* Push frame to continue with rest after this arg evaluates */
    StackFrame* cont = frame_create_args(rest, frame->env,
                                        frame->arg_index + 1);
    cont->accumulated_args = accumulated;
    stack_push(stack, cont);

    /* Evaluate current argument */
    StackFrame* eval = frame_create_eval(arg, frame->env);
    stack_push(stack, eval);
}
```

**Checkpoint:** Run tests, verify function application works.

---

### Phase 2C: Special Forms (2 hours)

#### Step 5: handle_eval_quote() - Quote

**Test:**
```c
void test_eval_quote(void) {
    EvalStack* stack = stack_create();
    Cell* expr = cell_cons(symbol_create("foo"), cell_nil());

    StackFrame* frame = frame_create_quote(expr);
    handle_eval_quote(frame, stack);

    /* Should push EVAL_RETURN with literal expression */
    StackFrame* ret = stack_pop(stack);
    assert(ret->state == EVAL_RETURN);
    assert(ret->value == expr);

    frame_destroy(frame);
    frame_destroy(ret);
    stack_destroy(stack);
    printf("✓ test_eval_quote\n");
}
```

**Implementation:**
```c
void handle_eval_quote(StackFrame* frame, EvalStack* stack) {
    /* Quote - return expression without evaluation */
    StackFrame* ret = frame_create_return(frame->expr);
    stack_push(stack, ret);
}
```

#### Step 6: handle_eval_if() - Conditionals

**Test:**
```c
void test_eval_if_true(void) {
    EvalStack* stack = stack_create();
    Cell* cond = cell_boolean(true);
    Cell* then_branch = cell_number(1);
    Cell* else_branch = cell_number(2);
    Cell* env = NULL;

    StackFrame* frame = frame_create_if(cond, then_branch, else_branch, env);

    /* Simulate condition evaluated to true */
    frame->value = cell_boolean(true);

    handle_eval_if(frame, stack);

    /* Should push EVAL_EXPR for then branch */
    StackFrame* next = stack_peek(stack);
    assert(next->state == EVAL_EXPR);
    assert(next->expr == then_branch);

    frame_destroy(frame);
    stack_destroy(stack);
    printf("✓ test_eval_if_true\n");
}
```

**Implementation:**
```c
void handle_eval_if(StackFrame* frame, EvalStack* stack) {
    Cell* cond_value = frame->value;

    if (!cond_value) {
        /* Condition not evaluated yet - evaluate it first */
        StackFrame* eval = frame_create_eval(frame->expr, frame->env);
        stack_push(stack, eval);
        return;
    }

    /* Condition evaluated - choose branch */
    Cell* branch = cell_is_true(cond_value) ?
                   frame->then_branch : frame->else_branch;

    StackFrame* eval = frame_create_eval(branch, frame->env);
    stack_push(stack, eval);
}
```

#### Step 7: handle_eval_define() - Definitions

**Test:**
```c
void test_eval_define(void) {
    EvalStack* stack = stack_create();
    Env* env = env_create();

    Cell* symbol = symbol_create("x");
    Cell* value_expr = cell_number(42);

    StackFrame* frame = frame_create_define(symbol, value_expr, (Cell*)env);

    /* Simulate value evaluated */
    frame->value = cell_number(42);

    handle_eval_define(frame, stack);

    /* Should define symbol in environment */
    Cell* result = env_lookup(env, symbol);
    assert(cell_is_number(result));
    assert(cell_number_value(result) == 42);

    /* Should push EVAL_RETURN with the value */
    StackFrame* ret = stack_pop(stack);
    assert(ret->state == EVAL_RETURN);

    frame_destroy(frame);
    frame_destroy(ret);
    stack_destroy(stack);
    env_destroy(env);
    printf("✓ test_eval_define\n");
}
```

**Implementation:**
```c
void handle_eval_define(StackFrame* frame, EvalStack* stack) {
    Cell* value = frame->value;

    if (!value) {
        /* Value not evaluated yet - evaluate it first */
        StackFrame* eval = frame_create_eval(frame->expr, frame->env);
        stack_push(stack, eval);
        return;
    }

    /* Value evaluated - define in environment */
    Env* env = (Env*)frame->env;
    env_define(env, frame->symbol, value);

    /* Return the value */
    StackFrame* ret = frame_create_return(value);
    stack_push(stack, ret);
}
```

**Checkpoint:** Run all tests, verify all special forms work.

---

## Integration with handle_eval_expr()

Now update `handle_eval_expr()` to recognize special forms:

```c
void handle_eval_expr(StackFrame* frame, EvalStack* stack) {
    Cell* expr = frame->expr;
    Cell* env = frame->env;

    /* [... atom and symbol handling ...] */

    /* Pair - check for special forms */
    if (cell_is_pair(expr)) {
        Cell* head = cell_car(expr);
        Cell* tail = cell_cdr(expr);

        /* Lambda: (λ body) */
        if (cell_is_symbol(head) &&
            strcmp(cell_symbol_name(head), "λ") == 0) {
            /* Create closure immediately */
            Cell* body = cell_car(tail);
            int arity = 1; /* TODO: count params */
            Cell* closure = cell_lambda(body, (Env*)env, arity);

            StackFrame* ret = frame_create_return(closure);
            stack_push(stack, ret);
            return;
        }

        /* Define: (≔ symbol value) */
        if (cell_is_symbol(head) &&
            strcmp(cell_symbol_name(head), "≔") == 0) {
            Cell* symbol = cell_car(tail);
            Cell* value_expr = cell_car(cell_cdr(tail));

            StackFrame* def = frame_create_define(symbol, value_expr, env);
            stack_push(stack, def);
            return;
        }

        /* Conditional: (? cond then else) */
        if (cell_is_symbol(head) &&
            strcmp(cell_symbol_name(head), "?") == 0) {
            Cell* cond = cell_car(tail);
            Cell* then_branch = cell_car(cell_cdr(tail));
            Cell* else_branch = cell_car(cell_cdr(cell_cdr(tail)));

            StackFrame* if_frame = frame_create_if(cond, then_branch,
                                                   else_branch, env);
            stack_push(stack, if_frame);
            return;
        }

        /* Quote: (⌜ expr) */
        if (cell_is_symbol(head) &&
            strcmp(cell_symbol_name(head), "⌜") == 0) {
            Cell* quoted = cell_car(tail);
            StackFrame* quote = frame_create_quote(quoted);
            stack_push(stack, quote);
            return;
        }

        /* Function application: (func arg1 arg2 ...) */
        /* First evaluate function, then arguments */
        StackFrame* apply = frame_create_apply(NULL, tail, env);
        stack_push(stack, apply);

        StackFrame* eval_func = frame_create_eval(head, env);
        stack_push(stack, eval_func);
        return;
    }

    /* Unknown type - error */
    Cell* err = cell_error(symbol_create("invalid-expr"), expr);
    StackFrame* ret = frame_create_return(err);
    stack_push(stack, ret);
}
```

## Testing Strategy

### Unit Tests (Individual Handlers)

Each handler gets at least 3 tests:
1. Happy path (normal case)
2. Edge case (empty, nil, etc.)
3. Error case (invalid input)

### Integration Tests (End-to-End)

Test complete evaluation scenarios:

```c
void test_eval_complete_factorial(void) {
    /* Define factorial:
       (≔ ! (λ (n) (? (≡ n #0) #1 (⊗ n (! (⊖ n #1))))))
    */
    /* Evaluate (! 5) */
    /* Expected: 120 */
}

void test_eval_complete_higher_order(void) {
    /* Define twice: (≔ twice (λ (f) (λ (x) (f (f x))))) */
    /* Define inc: (≔ inc (λ (x) (⊕ x #1))) */
    /* Evaluate ((twice inc) #5) */
    /* Expected: 7 */
}
```

## Success Criteria

**Phase 2 Complete When:**
- ✅ All 7 handlers implemented
- ✅ At least 20 new C unit tests passing
- ✅ All handlers tested independently
- ✅ At least 3 integration tests passing
- ✅ No memory leaks (valgrind clean)
- ✅ Clean compilation (no warnings)

## Files to Create/Modify

**Create:**
- None (all handlers go in existing trampoline.c)

**Modify:**
- `bootstrap/trampoline.h` - Add handler function declarations
- `bootstrap/trampoline.c` - Implement all 7 handlers
- `bootstrap/test_trampoline.c` - Add ~20 new tests

**Expected Lines:**
- Handlers: ~400 lines of C
- Tests: ~600 lines of C
- Total: ~1000 new lines

## Estimated Timeline

**Phase 2A:** Core Evaluation (2 hours)
- handle_eval_expr() basic
- handle_eval_return()
- 5 tests

**Phase 2B:** Function Application (2 hours)
- handle_eval_apply()
- handle_eval_args()
- 8 tests

**Phase 2C:** Special Forms (2 hours)
- handle_eval_quote()
- handle_eval_if()
- handle_eval_define()
- Update handle_eval_expr() for special forms
- 7 tests

**Total: ~6 hours**

## Risk Mitigation

**Risk 1: Complex State Management**
- **Mitigation:** Test each handler independently first
- **Fallback:** Add debug prints to trace state transitions

**Risk 2: Reference Counting Bugs**
- **Mitigation:** Run valgrind on every test
- **Fallback:** Add assertions for ref counts

**Risk 3: Integration Issues**
- **Mitigation:** Write integration tests early
- **Fallback:** Parallel old evaluator for comparison

## Next Steps After Phase 2

**Phase 3 (Day 49):** Integration & Testing
1. Wire handlers into main evaluation loop
2. Replace eval_internal() with trampoline
3. Run full test suite (33 Guage tests)
4. Benchmark performance
5. Document changes

---

**Status:** Ready to implement
**Start:** Phase 2A - Core Evaluation
**First Test:** test_eval_atom_number()
