#pragma clang diagnostic ignored "-Wdeprecated-declarations"

#include "actor.h"
#include "channel.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Global actor registry */
static Actor* g_actors[MAX_ACTORS];
static int g_actor_count = 0;
static int g_next_actor_id = 1;

/* Current actor (set during scheduler tick) */
static Actor* g_current_actor = NULL;

Actor* actor_current(void) {
    return g_current_actor;
}

void actor_set_current(Actor* actor) {
    g_current_actor = actor;
}

/* Create a new actor from a behavior function.
 * behavior is a λ that takes (self) — the actor cell.
 * The body we give the fiber is (behavior self-cell),
 * but we need to build that as an application.
 * Instead, we wrap the behavior call in a lambda body
 * that the fiber will evaluate. */
Actor* actor_create(EvalContext* ctx, Cell* behavior, Cell* env) {
    if (g_actor_count >= MAX_ACTORS) {
        return NULL;
    }

    Actor* actor = (Actor*)calloc(1, sizeof(Actor));
    actor->id = g_next_actor_id++;
    actor->mailbox_count = 0;
    actor->alive = true;
    actor->result = NULL;

    /* Build application: (behavior self-actor-cell)
     * We create the actor cell first, then build the call expr.
     * The actor cell needs to exist before the fiber starts,
     * so the behavior can reference itself. */

    /* We'll create the CELL_ACTOR externally and pass the call body.
     * For now, create fiber with the behavior body.
     * The caller (prim_spawn) builds the application expression. */
    actor->fiber = fiber_create(ctx, behavior, env, FIBER_DEFAULT_STACK_SIZE);

    /* Register in global registry */
    g_actors[g_actor_count++] = actor;

    return actor;
}

/* Append message to actor's mailbox (FIFO).
 * Uses a simple array-based queue to avoid refcount complexity. */
#define MAILBOX_CAPACITY 1024
static Cell* g_mailbox_storage[MAX_ACTORS][MAILBOX_CAPACITY];
static int g_mailbox_read[MAX_ACTORS];
static int g_mailbox_write[MAX_ACTORS];

/* Get mailbox index for actor */
static int mailbox_idx(Actor* actor) {
    /* Find actor's index in registry */
    for (int i = 0; i < g_actor_count; i++) {
        if (g_actors[i] == actor) return i;
    }
    return -1;
}

void actor_send(Actor* actor, Cell* message) {
    int idx = mailbox_idx(actor);
    if (idx < 0) return;

    int w = g_mailbox_write[idx];
    if (w - g_mailbox_read[idx] >= MAILBOX_CAPACITY) return; /* full */

    cell_retain(message);
    g_mailbox_storage[idx][w % MAILBOX_CAPACITY] = message;
    g_mailbox_write[idx] = w + 1;
    actor->mailbox_count++;
}

Cell* actor_receive(Actor* actor) {
    if (actor->mailbox_count == 0) return NULL;

    int idx = mailbox_idx(actor);
    if (idx < 0) return NULL;

    int r = g_mailbox_read[idx];
    Cell* message = g_mailbox_storage[idx][r % MAILBOX_CAPACITY];
    g_mailbox_storage[idx][r % MAILBOX_CAPACITY] = NULL;
    g_mailbox_read[idx] = r + 1;
    actor->mailbox_count--;

    return message; /* caller owns the ref */
}

/* Destroy actor and free resources */
void actor_destroy(Actor* actor) {
    if (!actor) return;

    /* Release remaining mailbox messages */
    int idx = mailbox_idx(actor);
    if (idx >= 0) {
        while (g_mailbox_read[idx] < g_mailbox_write[idx]) {
            int r = g_mailbox_read[idx] % MAILBOX_CAPACITY;
            if (g_mailbox_storage[idx][r]) {
                cell_release(g_mailbox_storage[idx][r]);
                g_mailbox_storage[idx][r] = NULL;
            }
            g_mailbox_read[idx]++;
        }
    }

    if (actor->result) {
        cell_release(actor->result);
    }

    if (actor->fiber) {
        fiber_destroy(actor->fiber);
    }

    free(actor);
}

/* Lookup actor by ID */
Actor* actor_lookup(int id) {
    for (int i = 0; i < g_actor_count; i++) {
        if (g_actors[i] && g_actors[i]->id == id) {
            return g_actors[i];
        }
    }
    return NULL;
}

/* Cooperative round-robin scheduler.
 * Runs each alive actor once per tick.
 * An actor runs until it yields (←? with empty mailbox) or finishes.
 * Returns number of ticks executed. */
int actor_run_all(int max_ticks) {
    int ticks = 0;

    for (int t = 0; t < max_ticks; t++) {
        bool any_alive = false;
        bool any_ran = false;

        for (int i = 0; i < g_actor_count; i++) {
            Actor* actor = g_actors[i];
            if (!actor || !actor->alive) continue;
            any_alive = true;

            Fiber* fiber = actor->fiber;

            /* Skip suspended actors whose condition isn't met yet */
            if (fiber->state == FIBER_SUSPENDED) {
                switch (fiber->suspend_reason) {
                    case SUSPEND_MAILBOX:
                        if (actor->mailbox_count == 0) continue;
                        break;
                    case SUSPEND_CHAN_RECV: {
                        Channel* chan = channel_lookup(fiber->suspend_channel_id);
                        if (chan && chan->count == 0 && !chan->closed) continue;
                        break;
                    }
                    case SUSPEND_CHAN_SEND: {
                        Channel* chan = channel_lookup(fiber->suspend_channel_id);
                        if (chan && chan->count >= chan->capacity) continue;
                        break;
                    }
                    case SUSPEND_SELECT: {
                        bool any_ready = false;
                        for (int j = 0; j < fiber->suspend_select_count; j++) {
                            Channel* chan = channel_lookup(fiber->suspend_select_ids[j]);
                            if (chan && (chan->count > 0 || chan->closed)) {
                                any_ready = true;
                                break;
                            }
                        }
                        if (!any_ready) continue;
                        break;
                    }
                    case SUSPEND_GENERAL:
                        continue; /* Wait for explicit resume */
                }
            }

            /* Set current actor so ←? can find it */
            Actor* prev_actor = g_current_actor;
            Fiber* prev_fiber = fiber_current();
            g_current_actor = actor;
            fiber_set_current(fiber);

            if (fiber->state == FIBER_READY) {
                /* First run */
                fiber_start(fiber);
                any_ran = true;
            } else if (fiber->state == FIBER_SUSPENDED) {
                Cell* resume_val = NULL;

                switch (fiber->suspend_reason) {
                    case SUSPEND_MAILBOX: {
                        Cell* msg = actor_receive(actor);
                        resume_val = msg ? msg : cell_nil();
                        break;
                    }
                    case SUSPEND_CHAN_RECV: {
                        Channel* chan = channel_lookup(fiber->suspend_channel_id);
                        if (chan) {
                            Cell* val = channel_try_recv(chan);
                            if (val) {
                                resume_val = val;
                            } else if (chan->closed) {
                                resume_val = cell_error("chan-recv-closed", cell_nil());
                            } else {
                                resume_val = cell_nil();
                            }
                        } else {
                            resume_val = cell_error("chan-recv-invalid", cell_nil());
                        }
                        break;
                    }
                    case SUSPEND_CHAN_SEND: {
                        Channel* chan = channel_lookup(fiber->suspend_channel_id);
                        if (chan && fiber->suspend_send_value) {
                            channel_try_send(chan, fiber->suspend_send_value);
                            cell_release(fiber->suspend_send_value);
                            fiber->suspend_send_value = NULL;
                        }
                        resume_val = cell_nil();
                        break;
                    }
                    case SUSPEND_SELECT: {
                        int total = fiber->suspend_select_count;
                        int closed_empty = 0;
                        static int sel_round = 0;
                        int start = sel_round % total;
                        sel_round++;
                        for (int j = 0; j < total; j++) {
                            int idx = (start + j) % total;
                            int ch_id = fiber->suspend_select_ids[idx];
                            Channel* chan = channel_lookup(ch_id);
                            if (!chan || (chan->closed && chan->count == 0)) {
                                closed_empty++;
                                continue;
                            }
                            Cell* val = channel_try_recv(chan);
                            if (val) {
                                resume_val = cell_cons(cell_channel(ch_id), val);
                                cell_release(val);
                                break;
                            }
                        }
                        if (!resume_val) {
                            resume_val = (closed_empty == total)
                                ? cell_error("select-all-closed", cell_nil())
                                : cell_nil();
                        }
                        break;
                    }
                    case SUSPEND_GENERAL:
                        resume_val = cell_nil();
                        break;
                }

                fiber->suspend_reason = SUSPEND_GENERAL;
                fiber_resume(fiber, resume_val);
                if (resume_val) cell_release(resume_val);
                any_ran = true;
            }

            /* Check if actor finished */
            if (fiber->state == FIBER_FINISHED) {
                actor->alive = false;
                actor->result = fiber->result;
                if (actor->result) cell_retain(actor->result);
            }

            /* Restore */
            g_current_actor = prev_actor;
            fiber_set_current(prev_fiber);
        }

        if (any_ran) ticks++;
        if (!any_alive) break;
        /* If no actor ran this tick (all waiting), stop to avoid spin */
        if (!any_ran) break;
    }

    return ticks;
}

/* Reset all actors (for testing) */
void actor_reset_all(void) {
    for (int i = 0; i < g_actor_count; i++) {
        if (g_actors[i]) {
            actor_destroy(g_actors[i]);
            g_actors[i] = NULL;
        }
        g_mailbox_read[i] = 0;
        g_mailbox_write[i] = 0;
        memset(g_mailbox_storage[i], 0, sizeof(g_mailbox_storage[i]));
    }
    g_actor_count = 0;
    g_next_actor_id = 1;
    g_current_actor = NULL;
    channel_reset_all();
}
