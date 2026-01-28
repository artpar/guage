#ifndef TRAMPOLINE_H
#define TRAMPOLINE_H

#include "cell.h"
#include "eval.h"  /* For EvalContext */
#include <stdbool.h>

/* Frame State - what kind of computation this frame represents */
typedef enum {
    EVAL_EXPR,      /* Evaluate an expression */
    EVAL_APPLY,     /* Apply function to arguments */
    EVAL_ARGS,      /* Evaluate argument list */
    EVAL_RETURN,    /* Return value to parent frame */
    EVAL_IF,        /* Conditional branch */
    EVAL_DEFINE,    /* Global definition */
    EVAL_QUOTE,     /* Quote - return literal */
} FrameState;

/* Stack Frame - represents one step of computation */
typedef struct StackFrame {
    FrameState state;          /* Current state */
    Cell* expr;                /* Expression being evaluated */
    Cell* env;                 /* Environment for evaluation */
    Cell* value;               /* Result value (for EVAL_RETURN) */

    /* For EVAL_ARGS state */
    int arg_index;             /* Which argument we're evaluating */
    Cell* accumulated_args;    /* Evaluated arguments so far */

    /* For EVAL_APPLY state */
    Cell* func;                /* Function to apply */

    /* For EVAL_IF state */
    Cell* then_branch;         /* Then expression */
    Cell* else_branch;         /* Else expression */

    /* For EVAL_DEFINE state */
    Cell* symbol;              /* Symbol to define */
} StackFrame;

/* Evaluation Stack - manages frames */
typedef struct EvalStack {
    StackFrame** frames;       /* Array of frame pointers */
    int capacity;              /* Allocated capacity */
    int size;                  /* Current number of frames */
    Cell* result;              /* Final result */
    void* ctx;                 /* Global evaluation context (EvalContext*) */
} EvalStack;

/* ========== EvalStack Operations ========== */

/* Create new evaluation stack */
EvalStack* stack_create(void);

/* Destroy stack and all frames */
void stack_destroy(EvalStack* stack);

/* Check if stack is empty */
bool stack_is_empty(EvalStack* stack);

/* Get current stack size */
int stack_size(EvalStack* stack);

/* Push new frame onto stack */
void stack_push(EvalStack* stack, StackFrame* frame);

/* Pop top frame from stack (caller must free) */
StackFrame* stack_pop(EvalStack* stack);

/* Peek at top frame without removing */
StackFrame* stack_peek(EvalStack* stack);

/* Set final result */
void stack_set_result(EvalStack* stack, Cell* result);

/* Get final result */
Cell* stack_get_result(EvalStack* stack);

/* ========== StackFrame Creation ========== */

/* Create frame for expression evaluation */
StackFrame* frame_create_eval(Cell* expr, Cell* env);

/* Create frame for function application */
StackFrame* frame_create_apply(Cell* func, Cell* args, Cell* env);

/* Create frame for argument evaluation */
StackFrame* frame_create_args(Cell* args, Cell* env, int start_index);

/* Create frame for return value */
StackFrame* frame_create_return(Cell* value);

/* Create frame for conditional */
StackFrame* frame_create_if(Cell* cond, Cell* then_branch, Cell* else_branch, Cell* env);

/* Create frame for definition */
StackFrame* frame_create_define(Cell* symbol, Cell* value_expr, Cell* env);

/* Create frame for quote */
StackFrame* frame_create_quote(Cell* expr);

/* Destroy frame */
void frame_destroy(StackFrame* frame);

/* ========== State Handlers ========== */

/* Handle EVAL_EXPR state - evaluate expressions (atoms, symbols, pairs)
 * Returns: true if frame is done (can destroy), false if needs continuation */
bool handle_eval_expr(StackFrame* frame, EvalStack* stack);

/* Handle EVAL_APPLY state - apply function to arguments
 * Returns: true if frame is done (can destroy), false if needs continuation */
bool handle_eval_apply(StackFrame* frame, EvalStack* stack);

/* Handle EVAL_ARGS state - evaluate argument list left-to-right
 * Returns: true if frame is done (can destroy), false if needs continuation */
bool handle_eval_args(StackFrame* frame, EvalStack* stack);

/* Handle EVAL_RETURN state - propagate return value to parent
 * Returns: true if frame is done (can destroy), false if needs continuation */
bool handle_eval_return(StackFrame* frame, EvalStack* stack);

/* Handle EVAL_IF state - conditional branching
 * Returns: true if frame is done (can destroy), false if needs continuation */
bool handle_eval_if(StackFrame* frame, EvalStack* stack);

/* Handle EVAL_DEFINE state - global definition
 * Returns: true if frame is done (can destroy), false if needs continuation */
bool handle_eval_define(StackFrame* frame, EvalStack* stack);

/* Handle EVAL_QUOTE state - quote (return literal)
 * Returns: true if frame is done (can destroy), false if needs continuation */
bool handle_eval_quote(StackFrame* frame, EvalStack* stack);

/* ========== Debug Utilities ========== */

/* Get string name for frame state */
const char* frame_state_name(FrameState state);

/* Print stack frame for debugging */
void frame_print(StackFrame* frame);

/* Print entire stack for debugging */
void stack_print(EvalStack* stack);

/* ========== Trampoline Evaluator ========== */

/* Main evaluation loop - processes frames until stack empty */
void trampoline_loop(EvalStack* stack);

/* Trampoline evaluator entry point - evaluates expression using explicit stack */
Cell* trampoline_eval(EvalContext* ctx, Cell* expr);

#endif /* TRAMPOLINE_H */
