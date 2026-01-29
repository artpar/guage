#pragma clang diagnostic ignored "-Wdeprecated-declarations"

#include "fiber.h"
#include "cell.h"
#include "eval.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Global current fiber pointer */
static Fiber* g_current_fiber = NULL;

/* Entry point for fiber execution */
static void fiber_entry(unsigned int hi, unsigned int lo) {
    /* Reconstruct fiber pointer from two unsigned ints (portable for makecontext) */
    Fiber* fiber = (Fiber*)(((uintptr_t)hi << 32) | (uintptr_t)lo);

    /* Evaluate body */
    Cell* result = eval_internal(fiber->eval_ctx, fiber->body_env, fiber->body);

    /* Store result and mark finished */
    fiber->result = result;
    fiber->state = FIBER_FINISHED;

    /* Return to caller */
    swapcontext(&fiber->ctx, &fiber->caller_ctx);
}

/* Create a new fiber */
Fiber* fiber_create(EvalContext* ctx, Cell* body, Cell* env, size_t stack_size) {
    Fiber* fiber = (Fiber*)calloc(1, sizeof(Fiber));

    fiber->stack_size = stack_size;
    fiber->stack = (char*)malloc(stack_size);
    fiber->state = FIBER_READY;

    /* Store eval info */
    fiber->eval_ctx = ctx;
    fiber->body = body;
    fiber->body_env = env;
    cell_retain(body);
    cell_retain(env);

    /* Initialize context */
    getcontext(&fiber->ctx);
    fiber->ctx.uc_stack.ss_sp = fiber->stack;
    fiber->ctx.uc_stack.ss_size = fiber->stack_size;
    fiber->ctx.uc_link = NULL; /* We handle return via swapcontext */

    /* Split pointer into two unsigned ints for makecontext (which takes ints) */
    uintptr_t ptr = (uintptr_t)fiber;
    unsigned int hi = (unsigned int)(ptr >> 32);
    unsigned int lo = (unsigned int)(ptr & 0xFFFFFFFF);
    makecontext(&fiber->ctx, (void (*)(void))fiber_entry, 2, hi, lo);

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

    free(fiber->stack);
    free(fiber);
}

/* Start a fiber (first resume — transitions READY -> RUNNING) */
void fiber_start(Fiber* fiber) {
    fiber->state = FIBER_RUNNING;
    swapcontext(&fiber->caller_ctx, &fiber->ctx);
}

/* Resume a suspended fiber with a value */
void fiber_resume(Fiber* fiber, Cell* value) {
    if (fiber->resume_value) {
        cell_release(fiber->resume_value);
    }
    fiber->resume_value = value;
    if (value) cell_retain(value);

    fiber->state = FIBER_RUNNING;
    swapcontext(&fiber->caller_ctx, &fiber->ctx);
}

/* Yield from inside a fiber (transitions RUNNING -> SUSPENDED) */
void fiber_yield(Fiber* fiber) {
    fiber->state = FIBER_SUSPENDED;
    swapcontext(&fiber->ctx, &fiber->caller_ctx);
}

/* Get the current fiber */
Fiber* fiber_current(void) {
    return g_current_fiber;
}

/* Set the current fiber */
void fiber_set_current(Fiber* fiber) {
    g_current_fiber = fiber;
}
