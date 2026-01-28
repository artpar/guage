#include "trampoline.h"
#include "eval.h"  /* For EvalContext, eval_lookup, eval_lookup_env */
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
    stack->ctx = NULL;  /* Will be set by caller if needed */

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

/* ========== State Handlers ========== */

/*
 * handle_eval_return - Propagate return value to parent frame or set final result
 *
 * This is the simplest handler - it takes a value and either:
 * 1. Sets it as the final result if stack is empty
 * 2. Passes it to the parent frame if stack has more frames
 */
void handle_eval_return(StackFrame* frame, EvalStack* stack) {
    Cell* value = frame->value;

    if (stack_is_empty(stack)) {
        /* No parent - this is the final result */
        stack_set_result(stack, value);
        return;
    }

    /* Get parent frame and store value */
    StackFrame* parent = stack_peek(stack);

    /* Store value in parent's value field */
    if (parent->value) {
        cell_release(parent->value);
    }
    parent->value = value;
    if (value) {
        cell_retain(value);
    }

    /* Parent will process value based on its state */
}

/*
 * handle_eval_quote - Return expression without evaluation (⌜)
 *
 * Simply returns the quoted expression as-is.
 */
void handle_eval_quote(StackFrame* frame, EvalStack* stack) {
    /* Quote - return expression without evaluation */
    StackFrame* ret = frame_create_return(frame->expr);
    stack_push(stack, ret);
}

/*
 * handle_eval_expr - Evaluate expressions (atoms, symbols, pairs)
 *
 * This is the main dispatcher that handles different expression types:
 * - Atoms (numbers, booleans, nil, errors, strings) -> self-evaluating
 * - Keywords (:foo) -> self-evaluating
 * - Symbols -> lookup in environment
 * - Pairs -> function application or special forms
 */
void handle_eval_expr(StackFrame* frame, EvalStack* stack) {
    Cell* expr = frame->expr;
    Cell* env = frame->env;

    /* Atoms - self-evaluating */
    if (cell_is_number(expr) || cell_is_bool(expr) ||
        cell_is_nil(expr) || cell_is_error(expr) ||
        cell_is_string(expr)) {
        /* Push return frame with the atom */
        StackFrame* ret = frame_create_return(expr);
        stack_push(stack, ret);
        return;
    }

    /* Keywords - self-evaluating (symbols starting with :) */
    if (cell_is_symbol(expr)) {
        const char* sym_name = cell_get_symbol(expr);
        if (sym_name && sym_name[0] == ':') {
            /* Keyword - self-evaluating */
            StackFrame* ret = frame_create_return(expr);
            stack_push(stack, ret);
            return;
        }
    }

    /* Symbol - lookup in environment */
    if (cell_is_symbol(expr)) {
        const char* name = cell_get_symbol(expr);

        /* Look up in local environment (for closures) */
        Cell* value = eval_lookup_env(env, name);

        if (!value) {
            /* Not in local env - try global context */
            if (stack->ctx) {
                EvalContext* ctx = (EvalContext*)stack->ctx;
                value = eval_lookup(ctx, name);
            }
        }

        if (!value) {
            /* Symbol not found - return error */
            Cell* err = cell_error("symbol-not-found", expr);
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
        /* TODO: Check for special forms (λ, ≔, ?, ⌜) */
        /* TODO: Handle function application */

        /* For now, return error */
        Cell* err = cell_error("not-implemented", cell_symbol("pair-eval"));
        StackFrame* ret = frame_create_return(err);
        stack_push(stack, ret);
        return;
    }

    /* Unknown type - error */
    Cell* err = cell_error("invalid-expr", expr);
    StackFrame* ret = frame_create_return(err);
    stack_push(stack, ret);
}

/*
 * handle_eval_apply - Apply function to arguments
 *
 * Handles two cases:
 * 1. Primitives - call C function directly
 * 2. Lambdas - create new environment and evaluate body
 */
void handle_eval_apply(StackFrame* frame, EvalStack* stack) {
    Cell* func = frame->func;
    Cell* args = frame->expr;  /* Unevaluated arguments */
    Cell* env = frame->env;

    /* TODO: Primitives and lambdas need to be implemented */
    /* For now, return error */
    Cell* err = cell_error("not-implemented", cell_symbol("apply"));
    StackFrame* ret = frame_create_return(err);
    stack_push(stack, ret);
}

/*
 * handle_eval_args - Evaluate argument list left-to-right
 *
 * Process arguments one at a time, accumulating evaluated results.
 */
void handle_eval_args(StackFrame* frame, EvalStack* stack) {
    Cell* args = frame->expr;
    Cell* accumulated = frame->accumulated_args;
    Cell* env = frame->env;

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
    StackFrame* cont = frame_create_args(rest, env, frame->arg_index + 1);
    cont->accumulated_args = accumulated;
    if (accumulated) cell_retain(accumulated);
    stack_push(stack, cont);

    /* Evaluate current argument */
    StackFrame* eval = frame_create_eval(arg, env);
    stack_push(stack, eval);
}

/*
 * handle_eval_if - Conditional branching (?)
 *
 * Evaluates condition, then chooses then/else branch based on result.
 */
void handle_eval_if(StackFrame* frame, EvalStack* stack) {
    Cell* cond_value = frame->value;

    if (!cond_value) {
        /* Condition not evaluated yet - evaluate it first */
        StackFrame* eval = frame_create_eval(frame->expr, frame->env);
        stack_push(stack, eval);
        return;
    }

    /* Condition evaluated - choose branch */
    /* In Guage, only #f and nil are false, everything else is true */
    bool is_true = true;
    if (cell_is_bool(cond_value) && !cell_get_bool(cond_value)) {
        is_true = false;
    } else if (cell_is_nil(cond_value)) {
        is_true = false;
    }

    Cell* branch = is_true ? frame->then_branch : frame->else_branch;

    StackFrame* eval = frame_create_eval(branch, frame->env);
    stack_push(stack, eval);
}

/*
 * handle_eval_define - Global definition (≔)
 *
 * Evaluates value expression, then defines symbol in global environment.
 */
void handle_eval_define(StackFrame* frame, EvalStack* stack) {
    Cell* value = frame->value;

    if (!value) {
        /* Value not evaluated yet - evaluate it first */
        StackFrame* eval = frame_create_eval(frame->expr, frame->env);
        stack_push(stack, eval);
        return;
    }

    /* Value evaluated - define in global environment */
    if (stack->ctx) {
        EvalContext* ctx = (EvalContext*)stack->ctx;
        const char* name = cell_get_symbol(frame->symbol);
        eval_define(ctx, name, value);
    }

    /* Return the value */
    StackFrame* ret = frame_create_return(value);
    stack_push(stack, ret);
}
