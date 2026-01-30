#ifndef GUAGE_FIBER_H
#define GUAGE_FIBER_H

#define _XOPEN_SOURCE
#include <ucontext.h>
#include <stdbool.h>
#include <stddef.h>

/* Forward declarations */
typedef struct Cell Cell;
typedef struct EvalContext EvalContext;

/* Fiber states */
typedef enum {
    FIBER_READY,
    FIBER_RUNNING,
    FIBER_SUSPENDED,
    FIBER_FINISHED
} FiberState;

/* Suspend reasons — why a fiber yielded */
typedef enum {
    SUSPEND_GENERAL,     /* shift/reset, perform */
    SUSPEND_MAILBOX,     /* ←? on empty mailbox */
    SUSPEND_CHAN_RECV,   /* ⟿← on empty channel */
    SUSPEND_CHAN_SEND,   /* ⟿→ on full channel */
    SUSPEND_SELECT,      /* ⟿⊞ waiting on multiple channels */
} SuspendReason;

/* Fiber - lightweight coroutine for delimited continuations */
typedef struct Fiber {
    ucontext_t ctx;            /* Fiber's saved execution context */
    ucontext_t caller_ctx;     /* Caller's context (return point on yield) */
    char* stack;               /* Allocated stack */
    size_t stack_size;
    FiberState state;

    /* Communication slots */
    Cell* resume_value;        /* Value sent TO fiber (from k) */
    Cell* result;              /* Final result when FINISHED */

    /* Perform metadata (set by perform before yielding) */
    const char* perform_eff;
    const char* perform_op;
    Cell* perform_args;

    /* Shift/reset support */
    Cell* shift_handler;       /* Handler fn from shift */
    bool is_shift_yield;       /* true = shift, false = perform */

    /* One-shot tracking */
    bool k_used;               /* true after k has been called once */

    /* Channel/mailbox suspend info */
    SuspendReason suspend_reason;
    int suspend_channel_id;        /* channel ID for CHAN_RECV/CHAN_SEND */
    Cell* suspend_send_value;      /* pending value for CHAN_SEND */

    /* Multi-channel select */
    #define MAX_SELECT_CHANNELS 16
    int suspend_select_ids[MAX_SELECT_CHANNELS];
    int suspend_select_count;

    /* Evaluation context */
    EvalContext* eval_ctx;
    Cell* body;
    Cell* body_env;
} Fiber;

/* Default stack size: 256KB */
#define FIBER_DEFAULT_STACK_SIZE (256 * 1024)

/* Lifecycle */
Fiber* fiber_create(EvalContext* ctx, Cell* body, Cell* env, size_t stack_size);
void   fiber_destroy(Fiber* fiber);

/* Operations */
void   fiber_start(Fiber* fiber);                 /* First resume: starts body eval */
void   fiber_resume(Fiber* fiber, Cell* value);   /* Resume suspended fiber */
void   fiber_yield(Fiber* fiber);                 /* Yield from inside body */

/* Current fiber tracking */
Fiber* fiber_current(void);
void   fiber_set_current(Fiber* fiber);

#endif /* GUAGE_FIBER_H */
