#include "fiber.h"
#include "cell.h"
#include "eval.h"
#include "scheduler.h"
#include "actor.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Thread-local current fiber pointer */
static _Thread_local Fiber* g_current_fiber = NULL;

/* Entry point for fiber execution — called by fcontext trampoline.
 * Receives transfer_t with {caller_ctx, data=Fiber*}.
 * Must NEVER return — must fctx_jump out. */
static void fiber_entry_wrapper(fctx_transfer_t transfer) {
    Fiber* fiber = (Fiber*)transfer.data;
    fiber->caller_ctx = transfer.ctx;

    /* Evaluate body — may yield via CELL_YIELD_SENTINEL (reduction budget) */
    EvalContext* ctx = fiber->eval_ctx;
    Cell* result = eval_internal(ctx, fiber->body_env, fiber->body);

    /* Handle reduction-budget yield loop */
    while (result == CELL_YIELD_SENTINEL) {
        /* Yield back to scheduler — immediately re-runnable */
        fiber->state = FIBER_SUSPENDED;
        fiber->suspend_reason = SUSPEND_REDUCTION;
        {
            Actor* cur = actor_current();
            trace_record(TRACE_YIELD, cur ? (uint16_t)cur->id : 0, CONTEXT_REDS);
        }
        fctx_transfer_t t = fctx_jump(fiber->caller_ctx, fiber);
        fiber->caller_ctx = t.ctx;

        /* Resumed by scheduler — reset reduction budget and continue */
        ctx->reductions_left = CONTEXT_REDS;
        if (ctx->continuation) {
            result = eval_internal(ctx, ctx->continuation_env, ctx->continuation);
            ctx->continuation = NULL;
            ctx->continuation_env = NULL;
        } else {
            /* No continuation saved — shouldn't happen, but handle gracefully */
            result = cell_nil();
            break;
        }
    }

    /* Store result and mark finished */
    fiber->result = result;
    fiber->state = FIBER_FINISHED;

    /* Return to caller — MUST NOT return from this function */
    fctx_jump(fiber->caller_ctx, fiber);
    /* Unreachable — trampoline has ud2/brk safety trap */
}

/* Create a new fiber */
Fiber* fiber_create(EvalContext* ctx, Cell* body, Cell* env, size_t stack_size) {
    Fiber* fiber = (Fiber*)calloc(1, sizeof(Fiber));

    fiber->stack_size = stack_size;

    /* Try scheduler stack pool (mmap + guard page + pre-fault),
     * fall back to malloc if no scheduler context */
    Scheduler* sched = sched_get((int)tls_scheduler_id);
    fiber->stack = sched ? sched_stack_alloc(sched, stack_size)
                         : (char*)malloc(stack_size);
    fiber->state = FIBER_READY;

    /* Store eval info */
    fiber->eval_ctx = ctx;
    fiber->body = body;
    fiber->body_env = env;
    cell_retain(body);
    cell_retain(env);

    /* Create fcontext on stack (stack grows down: top = base + size) */
    void* stack_top = fiber->stack + fiber->stack_size;
    fiber->ctx = fctx_make(stack_top, fiber->stack_size, fiber_entry_wrapper);

    return fiber;
}

/* Destroy a fiber and free resources */
void fiber_destroy(Fiber* fiber) {
    if (!fiber) return;

    if (fiber->body) cell_release(fiber->body);
    if (fiber->body_env) cell_release(fiber->body_env);
    if (fiber->result) cell_release(fiber->result);
    if (fiber->resume_value) cell_release(fiber->resume_value);
    if (fiber->perform_args) cell_release(fiber->perform_args);
    if (fiber->shift_handler) cell_release(fiber->shift_handler);
    if (fiber->saved_continuation) cell_release(fiber->saved_continuation);
    if (fiber->saved_continuation_env) cell_release(fiber->saved_continuation_env);

    /* Return stack to pool or free */
    Scheduler* sched = sched_get((int)tls_scheduler_id);
    if (sched) {
        sched_stack_free(sched, fiber->stack, fiber->stack_size);
    } else {
        free(fiber->stack);
    }
    free(fiber);
}

/* Start a fiber (first resume — transitions READY -> RUNNING) */
void fiber_start(Fiber* fiber) {
    fiber->state = FIBER_RUNNING;
    fctx_transfer_t t = fctx_jump(fiber->ctx, fiber);
    fiber->ctx = t.ctx;  /* Save fiber's updated context for next resume */
}

/* Resume a suspended fiber with a value */
void fiber_resume(Fiber* fiber, Cell* value) {
    if (fiber->resume_value) {
        cell_release(fiber->resume_value);
    }
    fiber->resume_value = value;
    if (value) cell_retain(value);

    fiber->state = FIBER_RUNNING;
    fctx_transfer_t t = fctx_jump(fiber->ctx, fiber);
    fiber->ctx = t.ctx;  /* Save fiber's updated context for next resume */
}

/* Yield from inside a fiber (transitions RUNNING -> SUSPENDED) */
void fiber_yield(Fiber* fiber) {
    fiber->state = FIBER_SUSPENDED;
    fctx_transfer_t t = fctx_jump(fiber->caller_ctx, fiber);
    fiber->caller_ctx = t.ctx;  /* Update caller context on resume */
}

/* Get the current fiber */
Fiber* fiber_current(void) {
    return g_current_fiber;
}

/* Set the current fiber */
void fiber_set_current(Fiber* fiber) {
    g_current_fiber = fiber;
}
