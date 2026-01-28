---
Status: REFERENCE
Created: 2026-01-28
Updated: 2026-01-28
Purpose: Comprehensive plan for production-ready trampoline evaluator architecture
---

# Trampoline Evaluator Architecture

## Overview

Replace C recursion-based evaluator with **explicit stack-based trampoline evaluator** for production-ready, unlimited recursion depth.

## Why Replace C Recursion?

**Current Architecture (BROKEN for production):**
```c
Cell* eval_internal(Cell* expr, Env* env) {
    // ... parse expr ...
    return eval_internal(subexpr, env);  // C recursion!
}
```

**Problems:**
- ❌ Stack overflow at ~254 frames (8MB default stack)
- ❌ Crash on deep recursion (merge sort, factorial, etc.)
- ❌ No way to implement continuations (call/cc)
- ❌ No way to implement coroutines
- ❌ No stack inspection/time-travel debugging
- ❌ Limited by OS stack size (macOS: 8MB default, Linux: varies)

**Band-Aid Fix (Day 46):**
- Increased stack to 32MB + O2 optimization
- **Still fundamentally limited** - just delays the problem

## Trampoline Architecture (PROPER FIX)

**Trampoline = Explicit heap-allocated evaluation stack**

**Key Insight:** Replace C call stack with our own data structure.

```c
// Instead of C recursion
Cell* eval(expr, env) {
    EvalStack* stack = stack_create();
    stack_push(stack, EVAL_EXPR, expr, env);

    while (!stack_is_empty(stack)) {
        StackFrame* frame = stack_pop(stack);
        handle_state(frame, stack);  // Process + push new frames
    }

    return stack_get_result(stack);
}
```

**Benefits:**
- ✅ Unlimited recursion (only limited by heap)
- ✅ No stack overflow crashes
- ✅ Foundation for call/cc (save/restore stack)
- ✅ Foundation for coroutines (multiple stacks)
- ✅ Foundation for time-travel debugging (stack history)
- ✅ Stack inspection (for debugger, profiler)
- ✅ Industry-standard architecture

## Architecture Design

### 1. Data Structures

#### StackFrame

Represents one "frame" of computation:

```c
typedef enum {
    EVAL_EXPR,      // Evaluate an expression
    EVAL_APPLY,     // Apply function to args
    EVAL_ARGS,      // Evaluate argument list
    EVAL_RETURN,    // Return value to parent
    EVAL_IF,        // Conditional branch
    EVAL_DEFINE,    // Global definition
} FrameState;

typedef struct StackFrame {
    FrameState state;          // What to do
    Cell* expr;                // Expression to evaluate
    Env* env;                  // Environment
    Cell* value;               // Result (when returning)
    int arg_index;             // For EVAL_ARGS: which arg we're on
    Cell* accumulated_args;    // For EVAL_ARGS: evaluated args so far
    Cell* func;                // For EVAL_APPLY: function to apply
    struct StackFrame* parent; // Link to parent frame
} StackFrame;
```

#### EvalStack

Manages the stack of frames:

```c
typedef struct EvalStack {
    StackFrame** frames;  // Array of frame pointers
    int capacity;         // Allocated size
    int size;             // Current depth
    Cell* result;         // Final result
} EvalStack;
```

### 2. Core Operations

#### Stack Management

```c
/* Create new evaluation stack */
EvalStack* stack_create(void);

/* Push new frame onto stack */
void stack_push(EvalStack* stack, FrameState state,
                Cell* expr, Env* env);

/* Pop top frame from stack */
StackFrame* stack_pop(EvalStack* stack);

/* Check if stack is empty */
bool stack_is_empty(EvalStack* stack);

/* Get final result */
Cell* stack_get_result(EvalStack* stack);

/* Clean up stack */
void stack_destroy(EvalStack* stack);
```

#### Frame Creation

```c
/* Create frame for expression evaluation */
StackFrame* frame_create_eval(Cell* expr, Env* env);

/* Create frame for function application */
StackFrame* frame_create_apply(Cell* func, Cell* args, Env* env);

/* Create frame for argument evaluation */
StackFrame* frame_create_args(Cell* args, Env* env);

/* Create frame for return value */
StackFrame* frame_create_return(Cell* value);

/* Destroy frame */
void frame_destroy(StackFrame* frame);
```

### 3. State Handlers

Each handler processes one type of frame and pushes new frames as needed.

#### handle_eval_expr

Evaluate an expression (atom, symbol, pair):

```c
void handle_eval_expr(StackFrame* frame, EvalStack* stack) {
    Cell* expr = frame->expr;
    Env* env = frame->env;

    if (cell_is_number(expr) || cell_is_boolean(expr) ||
        cell_is_nil(expr) || cell_is_error(expr)) {
        // Atom: return immediately
        stack_push_return(stack, expr);
        return;
    }

    if (cell_is_symbol(expr)) {
        // Symbol lookup
        Cell* value = env_lookup(env, expr);
        stack_push_return(stack, value);
        return;
    }

    if (cell_is_pair(expr)) {
        // Function application: (func arg1 arg2 ...)
        Cell* func_expr = cell_car(expr);
        Cell* args = cell_cdr(expr);

        // First evaluate function
        stack_push_apply(stack, func_expr, args, env);
        stack_push_eval(stack, func_expr, env);
        return;
    }

    // Unknown type
    stack_push_return(stack, cell_error("eval", expr));
}
```

#### handle_eval_apply

Apply function to arguments:

```c
void handle_eval_apply(StackFrame* frame, EvalStack* stack) {
    Cell* func = frame->func;
    Cell* args = frame->accumulated_args;
    Env* env = frame->env;

    if (cell_is_primitive(func)) {
        // Call primitive directly
        Cell* result = primitive_apply(func, args, env);
        stack_push_return(stack, result);
        return;
    }

    if (cell_is_lambda(func)) {
        // Apply lambda with De Bruijn indices
        Env* closure_env = cell_get_env(func);
        Cell* body = cell_get_body(func);
        int arity = cell_get_arity(func);

        // Create new environment with args
        Env* new_env = env_extend(closure_env, args, arity);

        // Evaluate body in new environment
        stack_push_eval(stack, body, new_env);
        return;
    }

    // Not a function
    stack_push_return(stack, cell_error("not-a-function", func));
}
```

#### handle_eval_args

Evaluate argument list left-to-right:

```c
void handle_eval_args(StackFrame* frame, EvalStack* stack) {
    Cell* args = frame->expr;
    int index = frame->arg_index;
    Cell* accumulated = frame->accumulated_args;

    if (cell_is_nil(args)) {
        // All args evaluated - return list
        stack_push_return(stack, accumulated);
        return;
    }

    // Evaluate next argument
    Cell* arg = cell_car(args);
    Cell* rest = cell_cdr(args);

    // Push frame to continue with rest after this arg
    stack_push_args_continue(stack, rest, index + 1, accumulated);

    // Evaluate current argument
    stack_push_eval(stack, arg, frame->env);
}
```

#### handle_eval_return

Propagate return value to parent:

```c
void handle_eval_return(StackFrame* frame, EvalStack* stack) {
    Cell* value = frame->value;

    if (stack_is_empty(stack)) {
        // No parent - this is the final result
        stack_set_result(stack, value);
        return;
    }

    // Get parent frame and pass value to it
    StackFrame* parent = stack_peek(stack);

    switch (parent->state) {
        case EVAL_APPLY:
            // Function evaluated - now evaluate args
            parent->func = value;
            parent->state = EVAL_ARGS;
            break;

        case EVAL_ARGS:
            // Argument evaluated - add to accumulated list
            parent->accumulated_args = cell_cons(value, parent->accumulated_args);
            break;

        case EVAL_IF:
            // Condition evaluated - choose branch
            if (cell_is_true(value)) {
                stack_push_eval(stack, parent->then_branch, parent->env);
            } else {
                stack_push_eval(stack, parent->else_branch, parent->env);
            }
            break;

        default:
            // Unknown state - error
            stack_push_return(stack, cell_error("invalid-state", parent->state));
    }
}
```

### 4. Main Evaluation Loop

```c
Cell* eval_trampolined(Cell* expr, Env* env) {
    // Create evaluation stack
    EvalStack* stack = stack_create();

    // Push initial frame
    stack_push(stack, EVAL_EXPR, expr, env, NULL);

    // Trampoline loop
    while (!stack_is_empty(stack)) {
        StackFrame* frame = stack_pop(stack);

        // Dispatch to appropriate handler
        switch (frame->state) {
            case EVAL_EXPR:
                handle_eval_expr(frame, stack);
                break;
            case EVAL_APPLY:
                handle_eval_apply(frame, stack);
                break;
            case EVAL_ARGS:
                handle_eval_args(frame, stack);
                break;
            case EVAL_RETURN:
                handle_eval_return(frame, stack);
                break;
            case EVAL_IF:
                handle_eval_if(frame, stack);
                break;
            case EVAL_DEFINE:
                handle_eval_define(frame, stack);
                break;
        }

        frame_destroy(frame);
    }

    // Get final result
    Cell* result = stack_get_result(stack);
    stack_destroy(stack);

    return result;
}
```

## Implementation Plan

### Phase 1: Data Structures (Day 1, ~4 hours)

**Goals:**
- Define `StackFrame` struct
- Implement `EvalStack` with operations
- Add tests for stack management
- No evaluation logic yet - just data structures

**Files to create/modify:**
- `bootstrap/trampoline.h` - Data structure definitions
- `bootstrap/trampoline.c` - Stack operations
- `bootstrap/tests/trampoline.test` - Unit tests

**Success Criteria:**
- ✅ Can create/destroy stacks
- ✅ Can push/pop frames
- ✅ Stack grows dynamically
- ✅ No memory leaks

**Estimated:** 4 hours

### Phase 2: State Handlers (Day 2, ~6 hours)

**Goals:**
- Implement `handle_eval_expr()`
- Implement `handle_eval_apply()`
- Implement `handle_eval_args()`
- Implement `handle_eval_return()`
- Handle special forms (?, ≔, λ, etc.)

**Files to modify:**
- `bootstrap/trampoline.c` - Add handlers
- `bootstrap/tests/trampoline.test` - Add handler tests

**Success Criteria:**
- ✅ Can evaluate atoms (numbers, booleans)
- ✅ Can evaluate symbols (lookups)
- ✅ Can evaluate function calls
- ✅ Can evaluate special forms
- ✅ All handlers tested independently

**Estimated:** 6 hours

### Phase 3: Integration (Day 3, ~6 hours)

**Goals:**
- Replace `eval_internal()` with `eval_trampolined()`
- Run full test suite
- Fix any bugs
- Benchmark performance
- Document changes

**Files to modify:**
- `bootstrap/eval.c` - Replace evaluator
- `bootstrap/eval.h` - Update API
- `Makefile` - Link trampoline.c
- All test files - Verify no regressions

**Success Criteria:**
- ✅ All 33 tests pass
- ✅ No stack overflow on deep recursion (1000+ levels)
- ✅ Performance within 20% of current (acceptable tradeoff)
- ✅ Clean memory (no leaks)

**Estimated:** 6 hours

### Total Timeline

**Day 1:** 4 hours - Data structures
**Day 2:** 6 hours - State handlers
**Day 3:** 6 hours - Integration & testing
**Total:** ~16 hours (~3 work days)

## Testing Strategy

### Unit Tests (Phase 1)

```scheme
; Test stack creation
(⊨ :stack-create #t
   (⊙? (stack-create)))

; Test push/pop
(⊨ :stack-push-pop #t
   (let ((s (stack-create)))
     (stack-push s :eval #42 env)
     (= 1 (stack-size s))))

; Test dynamic growth
(⊨ :stack-growth #t
   (let ((s (stack-create)))
     (repeat 1000 (stack-push s :eval #42 env))
     (= 1000 (stack-size s))))
```

### Integration Tests (Phase 3)

```scheme
; Test deep recursion (factorial)
(⊨ :deep-recursion #3628800
   (! 10))

; Test very deep recursion (should not crash)
(⊨ :very-deep #500500
   ((λ (n sum)
      (? (= n 0) sum (self (- n 1) (+ sum n))))
    1000 0))

; Test all existing tests pass
(run-test-suite)
```

### Performance Benchmarks

```bash
# Before trampoline
time ./guage < bootstrap/stdlib/list.scm

# After trampoline
time ./guage < bootstrap/stdlib/list.scm

# Accept if within 20% slowdown
```

## Memory Management

**Reference Counting:**
- Stack frames hold Cell references
- Must properly inc/dec ref counts
- Clean up on frame destruction

**Stack Growth:**
- Start with 64 frames capacity
- Double when full
- Never shrink (reuse for future calls)

**Frame Pooling (Future Optimization):**
- Pool of pre-allocated frames
- Reduces malloc/free overhead
- Can be added after basic implementation works

## Special Forms Handling

### Lambda (λ)

Already uses De Bruijn indices - no change needed:

```c
case SPECIAL_LAMBDA:
    // Create closure immediately
    Cell* closure = cell_lambda(body, env, arity);
    stack_push_return(stack, closure);
    break;
```

### Define (≔)

```c
case SPECIAL_DEFINE:
    // Evaluate value first
    stack_push_define_complete(stack, symbol);
    stack_push_eval(stack, value_expr, env);
    break;
```

### Conditional (?)

```c
case SPECIAL_IF:
    // Evaluate condition first
    stack_push_if_branch(stack, then_expr, else_expr, env);
    stack_push_eval(stack, cond_expr, env);
    break;
```

## Advanced Features (Future)

### Continuations (call/cc)

With explicit stack, we can implement first-class continuations:

```c
/* Capture current stack as continuation */
Cell* capture_continuation(EvalStack* stack) {
    StackFrame** snapshot = copy_stack_frames(stack);
    return cell_continuation(snapshot, stack->size);
}

/* Resume from continuation */
void resume_continuation(Cell* cont, Cell* value, EvalStack* stack) {
    restore_stack_frames(stack, cell_get_cont_frames(cont));
    stack_push_return(stack, value);
}
```

### Coroutines

Multiple stacks = multiple execution contexts:

```c
typedef struct Coroutine {
    EvalStack* stack;
    CoroutineState state;  // RUNNING, SUSPENDED, DEAD
    Cell* value;           // Yielded/returned value
} Coroutine;

/* Yield from coroutine */
void coroutine_yield(Coroutine* co, Cell* value);

/* Resume coroutine */
Cell* coroutine_resume(Coroutine* co);
```

### Time-Travel Debugging

Save stack snapshots at each step:

```c
typedef struct StackHistory {
    StackFrame*** snapshots;  // Array of stack snapshots
    int count;                // Number of snapshots
    int current;              // Current position
} StackHistory;

/* Step forward/backward */
void debug_step_forward(StackHistory* hist);
void debug_step_backward(StackHistory* hist);

/* Query state at any point */
Cell* debug_get_expr_at(StackHistory* hist, int step);
Env* debug_get_env_at(StackHistory* hist, int step);
```

## Performance Considerations

### Expected Slowdown

**Trampoline vs C recursion:**
- **C recursion:** ~1 instruction per call (call + ret)
- **Trampoline:** ~10-20 instructions per frame (malloc, memcpy, free)
- **Expected slowdown:** 10-20%
- **Acceptable tradeoff:** Unlimited recursion worth the cost

### Optimizations (Future)

1. **Frame Pooling:** Reuse frames instead of malloc/free
2. **Tail Call Optimization:** Detect tail calls, reuse frame
3. **JIT Compilation:** Compile hot paths to native code
4. **Stack Compression:** Store only changed parts of environment

### Benchmarking

Run before/after comparisons:

```bash
# Fibonacci (recursive)
time ./guage -e "(! 20)"

# List operations (stdlib)
time ./guage < bootstrap/stdlib/list.scm

# Deep recursion (new capability)
time ./guage -e "((λ (n) (? (= n 0) #t (self (- n 1)))) 10000)"
```

## Migration Strategy

### Phase 1: Parallel Implementation

Keep both evaluators during development:

```c
/* eval.c */
Cell* eval_internal(Cell* expr, Env* env);     // Old (C recursion)
Cell* eval_trampolined(Cell* expr, Env* env);  // New (trampoline)

/* Use flag to switch */
#ifdef USE_TRAMPOLINE
    return eval_trampolined(expr, env);
#else
    return eval_internal(expr, env);
#endif
```

### Phase 2: Testing

Run all tests with both evaluators:

```bash
# Old evaluator
make test

# New evaluator
make test USE_TRAMPOLINE=1

# Compare results
diff old_results.txt new_results.txt
```

### Phase 3: Cutover

Once trampoline passes all tests:
1. Make trampoline the default
2. Keep old evaluator for 1-2 releases
3. Remove old evaluator after confidence

## Documentation Updates

After implementation, update:

1. **SPEC.md:** Note unlimited recursion depth
2. **SESSION_HANDOFF.md:** Update status to "production-ready architecture"
3. **docs/reference/TECHNICAL_DECISIONS.md:** Add trampoline rationale
4. **docs/INDEX.md:** Update Quick Status
5. **README.md:** Highlight unlimited recursion capability

## Success Metrics

**Must Have:**
- ✅ All 33 existing tests pass
- ✅ No stack overflow on 1000+ deep recursion
- ✅ No memory leaks
- ✅ Clean compilation with no warnings

**Should Have:**
- ✅ Performance within 20% of current
- ✅ Clear error messages
- ✅ Comprehensive test coverage

**Nice to Have:**
- ✅ Performance within 10% of current
- ✅ Frame pooling optimization
- ✅ Tail call optimization

## Risk Mitigation

**Risk 1: Breaking Changes**
- **Mitigation:** Keep old evaluator during development
- **Fallback:** Can revert if critical issues

**Risk 2: Performance Regression**
- **Mitigation:** Benchmark before/after
- **Fallback:** Optimize hot paths if needed

**Risk 3: Complex Bugs**
- **Mitigation:** Test each phase independently
- **Fallback:** Parallel implementation allows comparison

**Risk 4: Memory Leaks**
- **Mitigation:** Run valgrind on all tests
- **Fallback:** Fix leaks before cutover

## References

**Academic:**
- "Definitional Interpreters for Higher-Order Programming Languages" (Reynolds 1972)
- "Three Implementation Models for Scheme" (Clinger 1998)

**Practical:**
- CPython: Uses evaluation stack (Python/ceval.c)
- Lua: Uses CallInfo stack (lua/ldo.c)
- Guile Scheme: Uses VM stack (guile/vm.c)

**Guage:**
- `bootstrap/eval.c` - Current C recursion evaluator
- `bootstrap/cell.{c,h}` - Cell data structures
- `bootstrap/debruijn.{c,h}` - De Bruijn conversion
- Day 46 session notes - Stack overflow analysis

---

**Status:** Architecture designed, ready for implementation
**Next Step:** Phase 1 - Implement data structures (StackFrame, EvalStack)
**Timeline:** 3 days (~16 hours) for complete implementation
