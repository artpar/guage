#include "trampoline.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Initial capacity for stack */
#define INITIAL_STACK_CAPACITY 64

/* ========== EvalStack Implementation ========== */

EvalStack* stack_create(void) {
    EvalStack* stack = malloc(sizeof(EvalStack));
    if (!stack) {
        fprintf(stderr, "Failed to allocate EvalStack\n");
        exit(1);
    }

    stack->frames = malloc(sizeof(StackFrame*) * INITIAL_STACK_CAPACITY);
    if (!stack->frames) {
        fprintf(stderr, "Failed to allocate stack frames array\n");
        exit(1);
    }

    stack->capacity = INITIAL_STACK_CAPACITY;
    stack->size = 0;
    stack->result = NULL;

    return stack;
}

void stack_destroy(EvalStack* stack) {
    if (!stack) return;

    /* Destroy all remaining frames */
    for (int i = 0; i < stack->size; i++) {
        frame_destroy(stack->frames[i]);
    }

    /* Release result if set */
    if (stack->result) {
        cell_release(stack->result);
    }

    free(stack->frames);
    free(stack);
}

bool stack_is_empty(EvalStack* stack) {
    return stack->size == 0;
}

int stack_size(EvalStack* stack) {
    return stack->size;
}

void stack_push(EvalStack* stack, StackFrame* frame) {
    /* Grow if needed */
    if (stack->size >= stack->capacity) {
        int new_capacity = stack->capacity * 2;
        StackFrame** new_frames = realloc(stack->frames,
                                          sizeof(StackFrame*) * new_capacity);
        if (!new_frames) {
            fprintf(stderr, "Failed to grow stack (capacity %d -> %d)\n",
                    stack->capacity, new_capacity);
            exit(1);
        }
        stack->frames = new_frames;
        stack->capacity = new_capacity;
    }

    stack->frames[stack->size++] = frame;
}

StackFrame* stack_pop(EvalStack* stack) {
    if (stack_is_empty(stack)) {
        fprintf(stderr, "Stack underflow: cannot pop from empty stack\n");
        exit(1);
    }

    return stack->frames[--stack->size];
}

StackFrame* stack_peek(EvalStack* stack) {
    if (stack_is_empty(stack)) {
        return NULL;
    }

    return stack->frames[stack->size - 1];
}

void stack_set_result(EvalStack* stack, Cell* result) {
    if (stack->result) {
        cell_release(stack->result);
    }
    stack->result = result;
    if (result) {
        cell_retain(result);
    }
}

Cell* stack_get_result(EvalStack* stack) {
    return stack->result;
}

/* ========== StackFrame Implementation ========== */

static StackFrame* frame_create_base(FrameState state) {
    StackFrame* frame = malloc(sizeof(StackFrame));
    if (!frame) {
        fprintf(stderr, "Failed to allocate StackFrame\n");
        exit(1);
    }

    memset(frame, 0, sizeof(StackFrame));
    frame->state = state;
    return frame;
}

StackFrame* frame_create_eval(Cell* expr, Cell* env) {
    StackFrame* frame = frame_create_base(EVAL_EXPR);
    frame->expr = expr;
    frame->env = env;

    /* Retain references */
    if (expr) cell_retain(expr);
    if (env) cell_retain(env);

    return frame;
}

StackFrame* frame_create_apply(Cell* func, Cell* args, Cell* env) {
    StackFrame* frame = frame_create_base(EVAL_APPLY);
    frame->func = func;
    frame->expr = args;  /* args stored in expr field */
    frame->env = env;

    /* Retain references */
    if (func) cell_retain(func);
    if (args) cell_retain(args);
    if (env) cell_retain(env);

    return frame;
}

StackFrame* frame_create_args(Cell* args, Cell* env, int start_index) {
    StackFrame* frame = frame_create_base(EVAL_ARGS);
    frame->expr = args;
    frame->env = env;
    frame->arg_index = start_index;
    frame->accumulated_args = cell_nil();  /* Start with empty list */

    /* Retain references */
    if (args) cell_retain(args);
    if (env) cell_retain(env);
    cell_retain(frame->accumulated_args);

    return frame;
}

StackFrame* frame_create_return(Cell* value) {
    StackFrame* frame = frame_create_base(EVAL_RETURN);
    frame->value = value;

    /* Retain reference */
    if (value) cell_retain(value);

    return frame;
}

StackFrame* frame_create_if(Cell* cond, Cell* then_branch, Cell* else_branch, Cell* env) {
    StackFrame* frame = frame_create_base(EVAL_IF);
    frame->expr = cond;
    frame->then_branch = then_branch;
    frame->else_branch = else_branch;
    frame->env = env;

    /* Retain references */
    if (cond) cell_retain(cond);
    if (then_branch) cell_retain(then_branch);
    if (else_branch) cell_retain(else_branch);
    if (env) cell_retain(env);

    return frame;
}

StackFrame* frame_create_define(Cell* symbol, Cell* value_expr, Cell* env) {
    StackFrame* frame = frame_create_base(EVAL_DEFINE);
    frame->symbol = symbol;
    frame->expr = value_expr;
    frame->env = env;

    /* Retain references */
    if (symbol) cell_retain(symbol);
    if (value_expr) cell_retain(value_expr);
    if (env) cell_retain(env);

    return frame;
}

StackFrame* frame_create_quote(Cell* expr) {
    StackFrame* frame = frame_create_base(EVAL_QUOTE);
    frame->expr = expr;

    /* Retain reference */
    if (expr) cell_retain(expr);

    return frame;
}

void frame_destroy(StackFrame* frame) {
    if (!frame) return;

    /* Release all cell references */
    if (frame->expr) cell_release(frame->expr);
    if (frame->env) cell_release(frame->env);
    if (frame->value) cell_release(frame->value);
    if (frame->accumulated_args) cell_release(frame->accumulated_args);
    if (frame->func) cell_release(frame->func);
    if (frame->then_branch) cell_release(frame->then_branch);
    if (frame->else_branch) cell_release(frame->else_branch);
    if (frame->symbol) cell_release(frame->symbol);

    free(frame);
}

/* ========== Debug Utilities ========== */

const char* frame_state_name(FrameState state) {
    switch (state) {
        case EVAL_EXPR:   return "EVAL_EXPR";
        case EVAL_APPLY:  return "EVAL_APPLY";
        case EVAL_ARGS:   return "EVAL_ARGS";
        case EVAL_RETURN: return "EVAL_RETURN";
        case EVAL_IF:     return "EVAL_IF";
        case EVAL_DEFINE: return "EVAL_DEFINE";
        case EVAL_QUOTE:  return "EVAL_QUOTE";
        default:          return "UNKNOWN";
    }
}

void frame_print(StackFrame* frame) {
    if (!frame) {
        printf("Frame: NULL\n");
        return;
    }

    printf("Frame[%s]", frame_state_name(frame->state));

    if (frame->expr) {
        printf(" expr=");
        cell_print(frame->expr);
    }

    if (frame->value) {
        printf(" value=");
        cell_print(frame->value);
    }

    if (frame->func) {
        printf(" func=");
        cell_print(frame->func);
    }

    if (frame->state == EVAL_ARGS) {
        printf(" arg_index=%d", frame->arg_index);
        if (frame->accumulated_args) {
            printf(" accumulated=");
            cell_print(frame->accumulated_args);
        }
    }

    printf("\n");
}

void stack_print(EvalStack* stack) {
    if (!stack) {
        printf("Stack: NULL\n");
        return;
    }

    printf("Stack (size=%d, capacity=%d):\n", stack->size, stack->capacity);

    for (int i = stack->size - 1; i >= 0; i--) {
        printf("  [%d] ", i);
        frame_print(stack->frames[i]);
    }

    if (stack->result) {
        printf("  Result: ");
        cell_print(stack->result);
        printf("\n");
    }
}
