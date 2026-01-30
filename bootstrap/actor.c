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

/* Supervision: bidirectional link */
void actor_link(Actor* a, Actor* b) {
    if (!a || !b) return;
    /* Add b to a's links (if not already there) */
    for (int i = 0; i < a->link_count; i++) {
        if (a->links[i] == b->id) goto add_reverse;
    }
    if (a->link_count < MAX_LINKS) {
        a->links[a->link_count++] = b->id;
    }
add_reverse:
    /* Add a to b's links */
    for (int i = 0; i < b->link_count; i++) {
        if (b->links[i] == a->id) return;
    }
    if (b->link_count < MAX_LINKS) {
        b->links[b->link_count++] = a->id;
    }
}

void actor_unlink(Actor* a, Actor* b) {
    if (!a || !b) return;
    /* Remove b from a's links */
    for (int i = 0; i < a->link_count; i++) {
        if (a->links[i] == b->id) {
            a->links[i] = a->links[--a->link_count];
            break;
        }
    }
    /* Remove a from b's links */
    for (int i = 0; i < b->link_count; i++) {
        if (b->links[i] == a->id) {
            b->links[i] = b->links[--b->link_count];
            break;
        }
    }
}

/* Add watcher as a monitor of target */
void actor_add_monitor(Actor* target, Actor* watcher) {
    if (!target || !watcher) return;
    if (target->monitor_count < MAX_MONITORS) {
        target->monitors[target->monitor_count++] = watcher->id;
    }
}

/* Send exit signal to target from sender with reason.
 * If target traps exits → deliver as ⟨:EXIT sender-id reason⟩ message.
 * If target does not trap → kill it. */
void actor_exit_signal(Actor* target, Actor* sender, Cell* reason) {
    if (!target || !target->alive) return;

    if (target->trap_exit) {
        /* Deliver as message: ⟨:EXIT sender-id reason⟩ */
        Cell* msg = cell_cons(
            cell_symbol(":EXIT"),
            cell_cons(
                cell_number(sender ? sender->id : 0),
                cell_cons(reason, cell_nil())));
        actor_send(target, msg);
        cell_release(msg);
    } else {
        /* Kill the target actor */
        target->alive = false;
        target->result = reason;
        if (reason) cell_retain(reason);
        /* Propagate to target's own links/monitors */
        actor_notify_exit(target, reason);
    }
}

/* Called when an actor exits. Notifies links and monitors. */
void actor_notify_exit(Actor* exiting, Cell* reason) {
    if (!exiting) return;

    bool is_error = reason && reason->type == CELL_ERROR;

    /* Check if this actor belongs to a supervisor */
    Supervisor* sup = supervisor_find_for_child(exiting->id);
    if (sup && is_error) {
        supervisor_handle_exit(sup, exiting->id, reason);
    }

    /* Notify monitors: send ⟨:DOWN id reason⟩ message */
    for (int i = 0; i < exiting->monitor_count; i++) {
        Actor* watcher = actor_lookup(exiting->monitors[i]);
        if (watcher && watcher->alive) {
            Cell* exit_reason = is_error ? reason : cell_symbol(":normal");
            Cell* msg = cell_cons(
                cell_symbol(":DOWN"),
                cell_cons(
                    cell_number(exiting->id),
                    cell_cons(exit_reason, cell_nil())));
            actor_send(watcher, msg);
            cell_release(msg);
        }
    }

    /* Notify linked actors */
    for (int i = 0; i < exiting->link_count; i++) {
        Actor* linked = actor_lookup(exiting->links[i]);
        if (linked && linked->alive) {
            if (is_error) {
                /* Error exit → send exit signal */
                actor_exit_signal(linked, exiting, reason);
            } else {
                /* Normal exit → send :EXIT :normal as message if trapping,
                 * otherwise just unlink silently */
                if (linked->trap_exit) {
                    Cell* msg = cell_cons(
                        cell_symbol(":EXIT"),
                        cell_cons(
                            cell_number(exiting->id),
                            cell_cons(cell_symbol(":normal"), cell_nil())));
                    actor_send(linked, msg);
                    cell_release(msg);
                }
                /* Normal exit: don't kill linked actor */
            }
        }
    }
}

/* ─── Supervisor ─── */

static Supervisor* g_supervisors[MAX_SUPERVISORS];
static int g_supervisor_count = 0;
static int g_next_supervisor_id = 1;

Supervisor* supervisor_create(EvalContext* ctx, SupervisorStrategy strategy, Cell* specs) {
    if (g_supervisor_count >= MAX_SUPERVISORS) return NULL;

    Supervisor* sup = (Supervisor*)calloc(1, sizeof(Supervisor));
    sup->id = g_next_supervisor_id++;
    sup->strategy = strategy;
    sup->ctx = ctx;
    sup->restart_count = 0;
    sup->child_count = 0;

    /* Parse child specs from list */
    Cell* cur = specs;
    while (cur && cur->type == CELL_PAIR && sup->child_count < MAX_SUP_CHILDREN) {
        Cell* spec = cell_car(cur);
        cell_retain(spec);
        sup->child_specs[sup->child_count] = spec;
        sup->child_ids[sup->child_count] = 0; /* not yet spawned */
        sup->child_count++;
        cur = cell_cdr(cur);
    }

    g_supervisors[g_supervisor_count++] = sup;
    return sup;
}

/* Spawn a single child at given index, returns actor ID */
int supervisor_spawn_child(Supervisor* sup, int index) {
    if (index < 0 || index >= sup->child_count) return 0;

    Cell* behavior = sup->child_specs[index];
    EvalContext* ctx = sup->ctx;

    /* Replicate prim_spawn logic: create actor, define bindings, build body */
    Cell* placeholder = cell_nil();
    Actor* actor = actor_create(ctx, placeholder, ctx->env);
    cell_release(placeholder);
    if (!actor) return 0;

    char fn_name[64], self_name[64];
    snprintf(fn_name, sizeof(fn_name), "__actor_fn_%d", actor->id);
    snprintf(self_name, sizeof(self_name), "__actor_self_%d", actor->id);

    Cell* self_cell = cell_actor(actor->id);
    eval_define(ctx, fn_name, behavior);
    eval_define(ctx, self_name, self_cell);
    cell_release(self_cell);

    Cell* fn_sym = cell_symbol(fn_name);
    Cell* self_sym = cell_symbol(self_name);
    Cell* body = cell_cons(fn_sym, cell_cons(self_sym, cell_nil()));
    cell_release(fn_sym);
    cell_release(self_sym);

    cell_release(actor->fiber->body);
    actor->fiber->body = body;
    cell_retain(body);
    cell_release(body);

    sup->child_ids[index] = actor->id;
    return actor->id;
}

/* Handle a child's exit — apply restart strategy */
void supervisor_handle_exit(Supervisor* sup, int dead_id, Cell* reason) {
    if (!sup) return;
    if (sup->restart_count >= SUP_MAX_RESTARTS) return;

    /* Find which child index died */
    int dead_index = -1;
    for (int i = 0; i < sup->child_count; i++) {
        if (sup->child_ids[i] == dead_id) {
            dead_index = i;
            break;
        }
    }
    if (dead_index < 0) return;

    sup->restart_count++;

    if (sup->strategy == SUP_ONE_FOR_ONE) {
        /* Restart only the failed child */
        supervisor_spawn_child(sup, dead_index);
    } else if (sup->strategy == SUP_ONE_FOR_ALL) {
        /* Kill all other children, then restart all */
        for (int i = 0; i < sup->child_count; i++) {
            if (i == dead_index) continue;
            Actor* child = actor_lookup(sup->child_ids[i]);
            if (child && child->alive) {
                child->alive = false;
                child->result = cell_symbol(":shutdown");
                cell_retain(child->result);
            }
        }
        /* Respawn all children */
        for (int i = 0; i < sup->child_count; i++) {
            supervisor_spawn_child(sup, i);
        }
    } else if (sup->strategy == SUP_REST_FOR_ONE) {
        /* Kill children after dead_index, then restart from dead_index onward */
        for (int i = dead_index + 1; i < sup->child_count; i++) {
            Actor* child = actor_lookup(sup->child_ids[i]);
            if (child && child->alive) {
                child->alive = false;
                child->result = cell_symbol(":shutdown");
                cell_retain(child->result);
            }
        }
        /* Respawn from dead_index to end */
        for (int i = dead_index; i < sup->child_count; i++) {
            supervisor_spawn_child(sup, i);
        }
    }
}

Supervisor* supervisor_lookup(int id) {
    for (int i = 0; i < g_supervisor_count; i++) {
        if (g_supervisors[i] && g_supervisors[i]->id == id) {
            return g_supervisors[i];
        }
    }
    return NULL;
}

Supervisor* supervisor_find_for_child(int child_id) {
    for (int i = 0; i < g_supervisor_count; i++) {
        Supervisor* sup = g_supervisors[i];
        if (!sup) continue;
        for (int j = 0; j < sup->child_count; j++) {
            if (sup->child_ids[j] == child_id) return sup;
        }
    }
    return NULL;
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
                /* Notify links and monitors */
                actor_notify_exit(actor, actor->result);
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
    /* Reset supervisors first */
    for (int i = 0; i < g_supervisor_count; i++) {
        if (g_supervisors[i]) {
            Supervisor* sup = g_supervisors[i];
            for (int j = 0; j < sup->child_count; j++) {
                if (sup->child_specs[j]) {
                    cell_release(sup->child_specs[j]);
                }
            }
            free(sup);
            g_supervisors[i] = NULL;
        }
    }
    g_supervisor_count = 0;
    g_next_supervisor_id = 1;

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
