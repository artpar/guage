---
Status: CURRENT
Created: 2026-01-28
Purpose: Document critical bug found during Phase 3 integration testing
---

# Trampoline Phase 3 - Frame Lifecycle Bug

## Problem

When handlers push sub-frames and return, the current frame is destroyed by the evaluation loop. This breaks the state machine because parent frames cannot receive results from child computations.

## Example

Expression: `(⊕ 1 2)`

**Expected Flow:**
1. EVAL_EXPR((⊕ 1 2)) → pushes EVAL_APPLY((⊕ 1 2))
2. EVAL_APPLY (State 1) → pushes EVAL_EXPR(⊕) to evaluate function
3. EVAL_EXPR(⊕) → looks up ⊕ → pushes EVAL_RETURN(builtin)
4. EVAL_RETURN → stores builtin in EVAL_APPLY->value
5. EVAL_APPLY (State 2) → pushes EVAL_ARGS to evaluate arguments
6. ... continue until application complete

**Actual Flow (BUG):**
1. EVAL_EXPR((⊕ 1 2)) → pushes EVAL_APPLY((⊕ 1 2))
2. EVAL_APPLY popped, handler pushes EVAL_EXPR(⊕), then returns
3. **EVAL_APPLY frame is destroyed by loop**
4. EVAL_EXPR(⊕) → looks up ⊕ → pushes EVAL_RETURN(builtin)
5. EVAL_RETURN → stack is empty → sets builtin as final result
6. **Result: builtin function instead of application result**

## Root Cause

The `trampoline_loop` destroys frames after handlers return:

```c
void trampoline_loop(EvalStack* stack) {
    while (!stack_is_empty(stack)) {
        StackFrame* frame = stack_pop(stack);  // Pop frame

        switch (frame->state) {
            // ... dispatch to handler ...
        }

        frame_destroy(frame);  // ← BUG: Destroys frame even if it needs to continue
    }
}
```

## Solution Options

### Option A: Re-push Parent Before Pushing Child (RECOMMENDED)

Handlers re-push themselves before pushing sub-frames:

```c
void handle_eval_apply(StackFrame* frame, EvalStack* stack) {
    if (!func && !value) {
        /* Step 1: Need to evaluate function */

        /* Re-push this frame (will be processed again after function evaluates) */
        stack_push(stack, frame);

        /* Push sub-frame to evaluate function */
        StackFrame* eval_func = frame_create_eval(func_expr, env);
        stack_push(stack, eval_func);

        /* Return WITHOUT destroying frame (it's back on stack) */
        return;
    }
    // ... rest of states ...
}
```

**Pros:**
- Minimal changes to loop
- Clear semantics: frame on stack = needs processing
- Handlers control their own lifecycle

**Cons:**
- Handlers must remember to re-push
- Frame is destroyed and immediately re-pushed (slight overhead)

### Option B: "Continue" Flag

Add a flag to indicate frame should stay on stack:

```c
typedef struct StackFrame {
    // ... existing fields ...
    bool needs_continuation;  /* If true, don't destroy after handler returns */
} StackFrame;

void trampoline_loop(EvalStack* stack) {
    while (!stack_is_empty(stack)) {
        StackFrame* frame = stack_pop(stack);

        switch (frame->state) {
            // ... dispatch ...
        }

        if (!frame->needs_continuation) {
            frame_destroy(frame);
        }
        /* Otherwise frame stays alive and will be re-pushed by next handler */
    }
}
```

**Pros:**
- More efficient (no destroy/recreate)
- Explicit intent

**Cons:**
- More complex lifecycle management
- Frame stays in memory even when not on stack

### Option C: Separate "Push Sub-Frame" Function

Create a helper that handles the pattern:

```c
void stack_push_with_parent(EvalStack* stack, StackFrame* parent, StackFrame* child) {
    stack_push(stack, parent);  /* Re-push parent first */
    stack_push(stack, child);   /* Then push child on top */
}
```

**Pros:**
- Clear intent
- Reduces boilerplate in handlers

**Cons:**
- Still need to track which frames are "done"

## Recommendation

**Use Option A: Re-push Parent Before Pushing Child**

This is the most straightforward and maintains the current loop structure. Each handler is responsible for its own lifecycle:

- If handler is DONE → just return (frame will be destroyed)
- If handler needs CONTINUATION → re-push self, then push sub-frames

## Implementation Plan

1. **Update all handlers that push sub-frames:**
   - `handle_eval_apply` (States 1, 2)
   - `handle_eval_expr` (symbol lookup, pairs)
   - `handle_eval_args` (iterative arg evaluation)
   - `handle_eval_if` (condition evaluation)
   - `handle_eval_define` (value evaluation)

2. **Pattern to follow:**
   ```c
   if (needs_more_work) {
       /* Re-push self (don't retain, frame already on stack) */
       stack_push(stack, frame);

       /* Push sub-frame(s) */
       StackFrame* child = frame_create_whatever(...);
       stack_push(stack, child);

       return;  /* Frame not destroyed - it's back on stack */
   }

   /* Otherwise frame is done - return normally to be destroyed */
   ```

3. **Update loop to handle re-pushed frames:**
   ```c
   void trampoline_loop(EvalStack* stack) {
       while (!stack_is_empty(stack)) {
           StackFrame* frame = stack_pop(stack);

           /* Track if frame was re-pushed by handler */
           int stack_size_before = stack_size(stack);

           // ... dispatch to handler ...

           int stack_size_after = stack_size(stack);

           /* If handler re-pushed frame, it's at stack_size_before position */
           /* In that case, DON'T destroy frame */
           bool frame_was_repushed = (stack_size_after > stack_size_before);

           if (!frame_was_repushed) {
               frame_destroy(frame);
           }
       }
   }
   ```

Actually, simpler approach: check if top of stack is the same frame:

```c
void trampoline_loop(EvalStack* stack) {
    while (!stack_is_empty(stack)) {
        StackFrame* frame = stack_pop(stack);
        bool frame_is_still_on_stack = false;

        /* Check if handler will re-push frame (peek before dispatch) */
        int size_before = stack_size(stack);

        // ... dispatch to handler ...

        /* Check if frame was re-pushed */
        if (stack_size(stack) > size_before) {
            /* Frames were pushed - check if our frame is among them */
            for (int i = size_before; i < stack_size(stack); i++) {
                if (stack->frames[i] == frame) {
                    frame_is_still_on_stack = true;
                    break;
                }
            }
        }

        if (!frame_is_still_on_stack) {
            frame_destroy(frame);
        }
    }
}
```

Even simpler: handlers return a boolean indicating if frame should be destroyed:

```c
bool handle_eval_apply(StackFrame* frame, EvalStack* stack);  /* returns true if frame is done */

void trampoline_loop(EvalStack* stack) {
    while (!stack_is_empty(stack)) {
        StackFrame* frame = stack_pop(stack);

        bool frame_is_done = false;
        switch (frame->state) {
            case EVAL_APPLY:
                frame_is_done = handle_eval_apply(frame, stack);
                break;
            // ...
        }

        if (frame_is_done) {
            frame_destroy(frame);
        }
    }
}
```

## Next Steps

1. Choose approach (recommend: boolean return value from handlers)
2. Update handler signatures
3. Update all handlers to return appropriate value
4. Update loop to respect return value
5. Test all integration tests
6. Verify no memory leaks

## Estimated Time

- 2-3 hours to implement fix
- 1 hour to test and verify
- Total: 3-4 hours

---

**Status:** Bug documented, fix planned for Day 49 continuation
