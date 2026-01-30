#ifndef GUAGE_DEBUG_H
#define GUAGE_DEBUG_H

#include "cell.h"
#include "eval.h"

/* Debugging & Error Handling
 *
 * First-class support for errors, assertions, breakpoints, and tracing.
 */

/* Call stack frame for debugging */
typedef struct StackFrame {
    const char* function_name;  /* Name of function being called */
    Cell* args;                 /* Arguments passed */
    const char* file;           /* Source file (future) */
    int line;                   /* Line number (future) */
    struct StackFrame* parent;  /* Caller frame */
} StackFrame;

/* Debug context (extends EvalContext) */
typedef struct {
    StackFrame* stack;          /* Current call stack */
    bool trace_enabled;         /* Global trace flag */
    bool break_on_error;        /* Break on error */
    int breakpoint_count;       /* Number of breakpoints hit */
} DebugContext;

/* Initialize debug context */
DebugContext* debug_context_new(void);

/* Free debug context */
void debug_context_free(DebugContext* ctx);

/* Push stack frame */
void debug_push_frame(DebugContext* ctx, const char* name, Cell* args);

/* Pop stack frame */
void debug_pop_frame(DebugContext* ctx);

/* Print stack trace */
void debug_print_stack(DebugContext* ctx);

/* cell_is_error is now inline in cell.h */
/* cell_error_message and cell_error_data declared in cell.h */

#endif /* GUAGE_DEBUG_H */
