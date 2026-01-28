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
bool handle_eval_return(StackFrame* frame, EvalStack* stack) {
    Cell* value = frame->value;

    /* Debug */
    static int return_call = 0;
    return_call++;
    if (return_call <= 20) {
        fprintf(stderr, "DEBUG handle_eval_return #%d: value=%p, stack_empty=%d\n",
                return_call, (void*)value, stack_is_empty(stack));
        fflush(stderr);
    }

    if (stack_is_empty(stack)) {
        /* No parent - this is the final result */
        fprintf(stderr, "DEBUG: Setting final result\n");
        fflush(stderr);
        stack_set_result(stack, value);
        return true;  /* Frame is done */
    }

    /* Get parent frame and store value */
    StackFrame* parent = stack_peek(stack);

    fprintf(stderr, "DEBUG: Storing value in parent frame (state=%s)\n",
            frame_state_name(parent->state));
    fflush(stderr);

    /* Store value in parent's value field */
    if (parent->value) {
        cell_release(parent->value);
    }
    parent->value = value;
    if (value) {
        cell_retain(value);
    }

    /* Parent will process value based on its state */
    return true;  /* Frame is done */
}

/*
 * handle_eval_quote - Return expression without evaluation (⌜)
 *
 * Simply returns the quoted expression as-is.
 */
bool handle_eval_quote(StackFrame* frame, EvalStack* stack) {
    /* Quote - return expression without evaluation */
    StackFrame* ret = frame_create_return(frame->expr);
    stack_push(stack, ret);
    return true;  /* Frame is done */
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
bool handle_eval_expr(StackFrame* frame, EvalStack* stack) {
    Cell* expr = frame->expr;
    Cell* env = frame->env;

    /* Atoms - self-evaluating */
    if (cell_is_number(expr) || cell_is_bool(expr) ||
        cell_is_nil(expr) || cell_is_error(expr) ||
        cell_is_string(expr)) {
        /* Push return frame with the atom */
        StackFrame* ret = frame_create_return(expr);
        stack_push(stack, ret);
        return true;  /* Frame is done */
    }

    /* Keywords - self-evaluating (symbols starting with :) */
    if (cell_is_symbol(expr)) {
        const char* sym_name = cell_get_symbol(expr);
        if (sym_name && sym_name[0] == ':') {
            /* Keyword - self-evaluating */
            StackFrame* ret = frame_create_return(expr);
            stack_push(stack, ret);
            return true;  /* Frame is done */
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
            return true;  /* Frame is done */
        }

        StackFrame* ret = frame_create_return(value);
        stack_push(stack, ret);
        return true;  /* Frame is done */
    }

    /* Pair - function application or special form */
    if (cell_is_pair(expr)) {
        Cell* first = cell_car(expr);
        Cell* rest = cell_cdr(expr);

        /* Check for special forms - PURE SYMBOLS ONLY */
        if (cell_is_symbol(first)) {
            const char* sym = cell_get_symbol(first);

            /* ⌜ - quote */
            if (strcmp(sym, "⌜") == 0) {
                Cell* arg = cell_car(rest);
                StackFrame* quote_frame = frame_create_quote(arg);
                stack_push(stack, quote_frame);
                return true;  /* Frame is done */
            }

            /* ≔ - define */
            if (strcmp(sym, "≔") == 0) {
                Cell* name = cell_car(rest);
                Cell* value_expr = cell_car(cell_cdr(rest));
                StackFrame* define_frame = frame_create_define(name, value_expr, env);
                stack_push(stack, define_frame);
                return true;  /* Frame is done */
            }

            /* ? - conditional */
            if (strcmp(sym, "?") == 0) {
                Cell* cond_expr = cell_car(rest);
                Cell* then_expr = cell_car(cell_cdr(rest));
                Cell* else_expr = cell_car(cell_cdr(cell_cdr(rest)));
                StackFrame* if_frame = frame_create_if(cond_expr, then_expr, else_expr, env);
                stack_push(stack, if_frame);
                return true;  /* Frame is done */
            }

            /* λ - lambda (note: lambda creation is done at parse time with De Bruijn conversion)
             * If we see a raw λ here, it means it wasn't converted yet - this shouldn't happen
             * in the trampoline evaluator as conversion happens before evaluation.
             * For now, treat as an error. */
            if (strcmp(sym, "λ") == 0) {
                Cell* err = cell_error("invalid-lambda", cell_symbol("lambda-not-converted"));
                StackFrame* ret = frame_create_return(err);
                stack_push(stack, ret);
                return true;  /* Frame is done */
            }
        }

        /* Not a special form - function application */
        /* Create EVAL_APPLY frame with full expression */
        /* The handler will take care of evaluating func and args */
        StackFrame* apply_frame = frame_create_apply(NULL, expr, env);
        stack_push(stack, apply_frame);
        return true;  /* Frame is done */
    }

    /* Unknown type - error */
    Cell* err = cell_error("invalid-expr", expr);
    StackFrame* ret = frame_create_return(err);
    stack_push(stack, ret);

    return true;  /* Frame is done */
}

/*
 * handle_eval_apply - Apply function to arguments
 *
 * State machine using value and func fields:
 * 1. func==NULL, value==NULL: need to evaluate function (expr has full expression)
 * 2. func==NULL, value!=NULL: function evaluated, move to func field, eval args
 * 3. func!=NULL, value!=NULL: both ready, apply function
 */
bool handle_eval_apply(StackFrame* frame, EvalStack* stack) {
    Cell* func = frame->func;
    Cell* args_or_expr = frame->expr;
    Cell* env = frame->env;
    Cell* value = frame->value;

    /* Debug */
    static int apply_call = 0;
    apply_call++;
    if (apply_call <= 20) {
        fprintf(stderr, "DEBUG handle_eval_apply #%d: func=%p, value=%p\n",
                apply_call, (void*)func, (void*)value);
        fflush(stderr);
    }

    /* Step 1: Function not evaluated yet */
    if (!func && !value) {
        /* expr contains full expression: (func-expr arg1 arg2 ...) */
        Cell* func_expr = cell_car(args_or_expr);
        Cell* arg_exprs = cell_cdr(args_or_expr);

        /* Update expr to store arg expressions for later */
        if (frame->expr) cell_release(frame->expr);
        frame->expr = arg_exprs;
        if (arg_exprs) cell_retain(arg_exprs);

        /* Evaluate function expression - result will go to value field */
        StackFrame* eval_func = frame_create_eval(func_expr, env);
        stack_push(stack, eval_func);
        return false;  /* Frame needs continuation (wait for function evaluation) */
    }

    /* Step 2: Function evaluated (in value field), move to func and eval args */
    if (!func && value) {
        /* Move evaluated function from value to func field */
        frame->func = value;
        cell_retain(value);
        frame->value = NULL;  /* Clear value for next step */

        /* Evaluate arguments - result will go to value field */
        StackFrame* eval_args = frame_create_args(args_or_expr, env, 0);
        stack_push(stack, eval_args);
        return false;  /* Frame needs continuation (wait for args evaluation) */
    }

    /* Step 3: Both function and evaluated arguments ready - apply! */
    if (func && value) {
        Cell* evaluated_args = value;

        /* Handle primitives (CELL_BUILTIN) */
        if (func->type == CELL_BUILTIN) {
            Cell* (*builtin_fn)(Cell*) = (Cell* (*)(Cell*))func->data.atom.builtin;
            Cell* result = builtin_fn(evaluated_args);
            StackFrame* ret = frame_create_return(result);
            stack_push(stack, ret);
            return true;  /* Frame is done */
        }

        /* Handle lambdas (CELL_LAMBDA) */
        if (func->type == CELL_LAMBDA) {
            Cell* closure_env = func->data.lambda.env;
            Cell* body = func->data.lambda.body;
            int arity = func->data.lambda.arity;

            /* Check argument count */
            int arg_count = list_length(evaluated_args);
            if (arg_count != arity) {
                Cell* expected = cell_number(arity);
                Cell* actual = cell_number(arg_count);
                Cell* data = cell_cons(expected, cell_cons(actual, cell_nil()));
                cell_release(expected);
                cell_release(actual);
                Cell* err = cell_error("arity-mismatch", data);
                StackFrame* ret = frame_create_return(err);
                stack_push(stack, ret);
                return true;  /* Frame is done */
            }

            /* Create new environment: prepend args to closure env */
            Cell* new_env = extend_env(closure_env, evaluated_args);

            /* Evaluate body in new environment */
            StackFrame* eval = frame_create_eval(body, new_env);
            stack_push(stack, eval);

            /* Cleanup */
            cell_release(new_env);
            return true;  /* Frame is done */
        }

        /* Not a function - error */
        cell_retain(func);
        Cell* err = cell_error("not-a-function", func);
        StackFrame* ret = frame_create_return(err);
        stack_push(stack, ret);
        return true;  /* Frame is done */
    }

    /* Should not reach here */
    Cell* err = cell_error("internal-error", cell_symbol("handle-eval-apply"));
    StackFrame* ret = frame_create_return(err);
    stack_push(stack, ret);

    return true;  /* Frame is done */
}

/*
 * handle_eval_args - Evaluate argument list left-to-right
 *
 * Process arguments one at a time, accumulating evaluated results.
 */
bool handle_eval_args(StackFrame* frame, EvalStack* stack) {
    Cell* args = frame->expr;
    Cell* accumulated = frame->accumulated_args;
    Cell* env = frame->env;

    if (cell_is_nil(args)) {
        /* All arguments evaluated - return accumulated list */
        StackFrame* ret = frame_create_return(accumulated);
        stack_push(stack, ret);
        return true;  /* Frame is done */
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

    return true;  /* Frame is done (delegated to continuation) */
}

/*
 * handle_eval_if - Conditional branching (?)
 *
 * Evaluates condition, then chooses then/else branch based on result.
 */
bool handle_eval_if(StackFrame* frame, EvalStack* stack) {
    Cell* cond_value = frame->value;

    if (!cond_value) {
        /* Condition not evaluated yet - evaluate it first */
        StackFrame* eval = frame_create_eval(frame->expr, frame->env);
        stack_push(stack, eval);
        return false;  /* Frame needs continuation (wait for condition evaluation) */
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

    return true;  /* Frame is done */
}

/*
 * handle_eval_define - Global definition (≔)
 *
 * Evaluates value expression, then defines symbol in global environment.
 */
bool handle_eval_define(StackFrame* frame, EvalStack* stack) {
    Cell* value = frame->value;

    if (!value) {
        /* Value not evaluated yet - evaluate it first */
        StackFrame* eval = frame_create_eval(frame->expr, frame->env);
        stack_push(stack, eval);
        return false;  /* Frame needs continuation (wait for value evaluation) */
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

    return true;  /* Frame is done */
}

/* ========== Trampoline Evaluator Entry Point ========== */

/* Forward declaration */
void trampoline_loop(EvalStack* stack);

/*
 * trampoline_eval - Main entry point for trampoline evaluation
 *
 * Evaluates an expression using the trampoline architecture (explicit stack).
 * This replaces the recursive eval() with a loop-based approach that:
 * - Never overflows the C call stack
 * - Enables future features (continuations, time-travel debugging, coroutines)
 * - Makes the evaluation process inspectable and controllable
 *
 * Arguments:
 *   ctx - Global evaluation context (environment, primitives, type registry)
 *   expr - Expression to evaluate
 *
 * Returns:
 *   Evaluated result (caller must release)
 */
Cell* trampoline_eval(EvalContext* ctx, Cell* expr) {
    if (!ctx || !expr) {
        return cell_error("invalid-args", cell_nil());
    }

    /* Create evaluation stack and store context */
    EvalStack* stack = stack_create();
    stack->ctx = (void*)ctx;

    /* Push initial evaluation frame */
    StackFrame* initial = frame_create_eval(expr, ctx->env);
    stack_push(stack, initial);

    /* Run evaluation loop until completion */
    trampoline_loop(stack);

    /* Extract final result */
    Cell* result = stack_get_result(stack);
    if (!result) {
        result = cell_nil();  /* Default to nil if no result */
    }
    cell_retain(result);  /* Caller must release */

    /* Clean up stack */
    stack_destroy(stack);

    return result;
}

/*
 * trampoline_loop - Main evaluation loop
 *
 * Processes frames from the stack until empty, dispatching each frame
 * to its appropriate handler. Handlers push new frames as needed to
 * continue evaluation.
 *
 * The loop terminates when:
 * 1. Stack becomes empty (evaluation complete)
 * 2. An error frame is returned (propagates up)
 *
 * Arguments:
 *   stack - Evaluation stack with initial frame(s)
 */
void trampoline_loop(EvalStack* stack) {
    int iteration = 0;
    while (!stack_is_empty(stack)) {
        /* Pop frame to process */
        StackFrame* frame = stack_pop(stack);

        if (!frame) {
            /* Should never happen, but guard against it */
            fprintf(stderr, "Error: Popped NULL frame from non-empty stack\n");
            break;
        }

        /* Debug: Print frame state every 100 iterations */
        iteration++;
        if (iteration % 100 == 0) {
            fprintf(stderr, "DEBUG: iteration %d, state=%s, stack_size=%d\n",
                    iteration, frame_state_name(frame->state), stack_size(stack));
        }

        /* Remember stack size before handler */
        int size_before = stack_size(stack);

        /* Dispatch to appropriate handler based on frame state */
        /* Handlers return true if frame is done (can destroy), false if needs continuation */
        bool frame_done = false;

        switch (frame->state) {
            case EVAL_EXPR:
                frame_done = handle_eval_expr(frame, stack);
                break;

            case EVAL_APPLY:
                frame_done = handle_eval_apply(frame, stack);
                break;

            case EVAL_ARGS:
                frame_done = handle_eval_args(frame, stack);
                break;

            case EVAL_RETURN:
                frame_done = handle_eval_return(frame, stack);
                break;

            case EVAL_IF:
                frame_done = handle_eval_if(frame, stack);
                break;

            case EVAL_DEFINE:
                frame_done = handle_eval_define(frame, stack);
                break;

            case EVAL_QUOTE:
                frame_done = handle_eval_quote(frame, stack);
                break;

            default:
                /* Unknown state - should never happen */
                fprintf(stderr, "Error: Unknown frame state %d\n", frame->state);
                Cell* err = cell_error("invalid-state", cell_nil());
                StackFrame* ret = frame_create_return(err);
                stack_push(stack, ret);
                frame_done = true;  /* Destroy frame on error */
                break;
        }

        if (frame_done) {
            /* Frame is complete - destroy it */
            frame_destroy(frame);
        } else {
            /* Frame needs continuation - insert it below any sub-frames that were pushed */
            int frames_pushed = stack_size(stack) - size_before;

            if (frames_pushed == 0) {
                /* No sub-frames pushed - just push frame back */
                stack_push(stack, frame);
            } else {
                /* Sub-frames were pushed - insert frame below them */
                /* Temporarily pop the sub-frames */
                StackFrame** temp_frames = malloc(sizeof(StackFrame*) * frames_pushed);
                for (int i = 0; i < frames_pushed; i++) {
                    temp_frames[i] = stack_pop(stack);
                }

                /* Push the continuation frame */
                stack_push(stack, frame);

                /* Push the sub-frames back on top */
                for (int i = frames_pushed - 1; i >= 0; i--) {
                    stack_push(stack, temp_frames[i]);
                }

                free(temp_frames);
            }
        }
    }
}
