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

    /* Release process dictionary entries */
    for (int i = 0; i < actor->dict_count; i++) {
        if (actor->dict_keys[i]) cell_release(actor->dict_keys[i]);
        if (actor->dict_values[i]) cell_release(actor->dict_values[i]);
    }
    actor->dict_count = 0;

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

/* Forward declaration — defined after supervisor_handle_exit */
static void dynsup_remove_child_at(Supervisor* sup, int index);

/* Called when an actor exits. Notifies links and monitors. */
void actor_notify_exit(Actor* exiting, Cell* reason) {
    if (!exiting) return;

    /* Destroy ETS tables owned by this actor */
    ets_destroy_by_owner(exiting->id);

    /* Auto-deregister from named registry */
    actor_registry_unregister_actor(exiting->id);

    bool is_error = reason && reason->type == CELL_ERROR;

    /* Check if this actor belongs to a supervisor */
    Supervisor* sup = supervisor_find_for_child(exiting->id);
    if (sup) {
        if (is_error) {
            /* Error exit — always notify supervisor */
            supervisor_handle_exit(sup, exiting->id, reason);
        } else if (sup->is_dynamic) {
            /* Normal exit on dynamic supervisor — remove temporary/transient children */
            int idx = -1;
            for (int i = 0; i < sup->child_count; i++) {
                if (sup->child_ids[i] == exiting->id) { idx = i; break; }
            }
            if (idx >= 0) {
                ChildRestartType rt = sup->child_restart[idx];
                if (rt == CHILD_TEMPORARY || rt == CHILD_TRANSIENT) {
                    /* Both temporary and transient are removed on normal exit */
                    dynsup_remove_child_at(sup, idx);
                }
                /* CHILD_PERMANENT on normal exit: not restarted (normal is fine) */
            }
        }
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
    sup->is_dynamic = false;

    /* Parse child specs from list */
    Cell* cur = specs;
    while (cur && cur->type == CELL_PAIR && sup->child_count < MAX_SUP_CHILDREN) {
        Cell* spec = cell_car(cur);
        cell_retain(spec);
        sup->child_specs[sup->child_count] = spec;
        sup->child_ids[sup->child_count] = 0; /* not yet spawned */
        sup->child_restart[sup->child_count] = CHILD_PERMANENT; /* default */
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

/* Remove a dynamic child at given index (shift arrays down) */
static void dynsup_remove_child_at(Supervisor* sup, int index) {
    if (sup->child_specs[index]) {
        cell_release(sup->child_specs[index]);
    }
    for (int i = index; i < sup->child_count - 1; i++) {
        sup->child_specs[i] = sup->child_specs[i + 1];
        sup->child_ids[i] = sup->child_ids[i + 1];
        sup->child_restart[i] = sup->child_restart[i + 1];
    }
    sup->child_count--;
    sup->child_specs[sup->child_count] = NULL;
    sup->child_ids[sup->child_count] = 0;
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

    /* For dynamic supervisors, check per-child restart type */
    if (sup->is_dynamic) {
        ChildRestartType rt = sup->child_restart[dead_index];
        if (rt == CHILD_TEMPORARY) {
            /* Never restart — remove child */
            dynsup_remove_child_at(sup, dead_index);
            return;
        }
        /* CHILD_PERMANENT always restarts (handled below as one-for-one) */
        /* CHILD_TRANSIENT restarts only on error — caller already checked is_error,
         * but we also handle normal exit for transient in actor_notify_exit */
    }

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
                    case SUSPEND_TASK_AWAIT: {
                        Actor* awaited = actor_lookup(fiber->suspend_await_actor_id);
                        if (awaited && awaited->alive) continue;
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
                    case SUSPEND_TASK_AWAIT: {
                        Actor* awaited = actor_lookup(fiber->suspend_await_actor_id);
                        if (awaited && awaited->result) {
                            cell_retain(awaited->result);
                            resume_val = awaited->result;
                        } else {
                            resume_val = cell_nil();
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

        /* Tick timers each scheduler round */
        bool timer_fired = timer_tick_all();

        if (any_ran || timer_fired) ticks++;
        if (!any_alive) break;
        /* If no actor ran and no timer fired, check for pending timers */
        if (!any_ran && !timer_fired) {
            /* Keep spinning if timers are counting down */
            if (!timer_any_pending()) break;
        }
    }

    return ticks;
}

/* ─── Named Process Registry ─── */

static char* g_registry_names[MAX_REGISTRY];
static int   g_registry_ids[MAX_REGISTRY];
static int   g_registry_count = 0;

int actor_registry_register(const char* name, int actor_id) {
    if (g_registry_count >= MAX_REGISTRY) return -3;

    /* Check actor is alive */
    Actor* actor = actor_lookup(actor_id);
    if (!actor || !actor->alive) return -4;

    for (int i = 0; i < g_registry_count; i++) {
        if (strcmp(g_registry_names[i], name) == 0) return -1; /* dup name */
        if (g_registry_ids[i] == actor_id) return -2;          /* dup actor */
    }

    g_registry_names[g_registry_count] = strdup(name);
    g_registry_ids[g_registry_count] = actor_id;
    g_registry_count++;
    return 0;
}

int actor_registry_lookup(const char* name) {
    for (int i = 0; i < g_registry_count; i++) {
        if (strcmp(g_registry_names[i], name) == 0) {
            return g_registry_ids[i];
        }
    }
    return -1;
}

int actor_registry_unregister_name(const char* name) {
    for (int i = 0; i < g_registry_count; i++) {
        if (strcmp(g_registry_names[i], name) == 0) {
            free(g_registry_names[i]);
            g_registry_names[i] = g_registry_names[--g_registry_count];
            g_registry_ids[i] = g_registry_ids[g_registry_count];
            return 0;
        }
    }
    return -1;
}

void actor_registry_unregister_actor(int actor_id) {
    for (int i = 0; i < g_registry_count; i++) {
        if (g_registry_ids[i] == actor_id) {
            free(g_registry_names[i]);
            g_registry_names[i] = g_registry_names[--g_registry_count];
            g_registry_ids[i] = g_registry_ids[g_registry_count];
            return;
        }
    }
}

Cell* actor_registry_list(void) {
    Cell* list = cell_nil();
    for (int i = g_registry_count - 1; i >= 0; i--) {
        Cell* sym = cell_symbol(g_registry_names[i]);
        Cell* new_list = cell_cons(sym, list);
        cell_release(sym);
        cell_release(list);
        list = new_list;
    }
    return list;
}

void actor_registry_reset(void) {
    for (int i = 0; i < g_registry_count; i++) {
        free(g_registry_names[i]);
        g_registry_names[i] = NULL;
    }
    g_registry_count = 0;
}

/* Reset all actors (for testing) */
/* ─── Timers ─── */

static Timer g_timers[MAX_TIMERS];
static int g_timer_count = 0;
static int g_next_timer_id = 1;

int timer_create(int target_actor_id, int ticks, Cell* message) {
    if (g_timer_count >= MAX_TIMERS) return -1;
    int id = g_next_timer_id++;
    Timer* t = &g_timers[g_timer_count++];
    t->id = id;
    t->target_actor_id = target_actor_id;
    t->remaining_ticks = ticks;
    t->message = message;
    if (message) cell_retain(message);
    t->active = true;
    return id;
}

int timer_cancel(int timer_id) {
    for (int i = 0; i < g_timer_count; i++) {
        if (g_timers[i].id == timer_id && g_timers[i].active) {
            g_timers[i].active = false;
            if (g_timers[i].message) {
                cell_release(g_timers[i].message);
                g_timers[i].message = NULL;
            }
            return 0;
        }
    }
    return -1;
}

bool timer_active(int timer_id) {
    for (int i = 0; i < g_timer_count; i++) {
        if (g_timers[i].id == timer_id) {
            return g_timers[i].active;
        }
    }
    return false;
}

bool timer_tick_all(void) {
    bool any_fired = false;
    for (int i = 0; i < g_timer_count; i++) {
        Timer* t = &g_timers[i];
        if (!t->active) continue;
        if (t->remaining_ticks > 0) {
            t->remaining_ticks--;
            continue;
        }
        /* Timer fired */
        any_fired = true;
        Actor* target = actor_lookup(t->target_actor_id);
        if (target && target->alive && t->message) {
            actor_send(target, t->message);
        }
        t->active = false;
        if (t->message) {
            cell_release(t->message);
            t->message = NULL;
        }
    }
    return any_fired;
}

bool timer_any_pending(void) {
    for (int i = 0; i < g_timer_count; i++) {
        if (g_timers[i].active) return true;
    }
    return false;
}

void timer_reset_all(void) {
    for (int i = 0; i < g_timer_count; i++) {
        if (g_timers[i].message) {
            cell_release(g_timers[i].message);
            g_timers[i].message = NULL;
        }
        g_timers[i].active = false;
    }
    g_timer_count = 0;
    g_next_timer_id = 1;
}

/* ============ ETS - Erlang Term Storage ============ */

static EtsTable g_ets_tables[MAX_ETS_TABLES];
static int g_ets_count = 0;

static EtsTable* ets_find(const char* name) {
    for (int i = 0; i < g_ets_count; i++) {
        if (g_ets_tables[i].active && strcmp(g_ets_tables[i].name, name) == 0) {
            return &g_ets_tables[i];
        }
    }
    return NULL;
}

int ets_create(const char* name, int owner_actor_id) {
    if (ets_find(name)) return -1;  /* duplicate name */
    if (g_ets_count >= MAX_ETS_TABLES) return -2;  /* full */

    EtsTable* t = &g_ets_tables[g_ets_count++];
    t->name = strdup(name);
    t->owner_actor_id = owner_actor_id;
    t->count = 0;
    t->active = true;
    return 0;
}

int ets_insert(const char* name, Cell* key, Cell* value) {
    EtsTable* t = ets_find(name);
    if (!t) return -1;

    /* Search for existing key */
    for (int i = 0; i < t->count; i++) {
        if (cell_equal(t->keys[i], key)) {
            Cell* old = t->values[i];
            cell_retain(value);
            t->values[i] = value;
            cell_release(old);
            return 0;
        }
    }

    /* New entry */
    if (t->count >= MAX_ETS_ENTRIES) return -2;
    cell_retain(key);
    cell_retain(value);
    t->keys[t->count] = key;
    t->values[t->count] = value;
    t->count++;
    return 0;
}

Cell* ets_lookup(const char* name, Cell* key) {
    EtsTable* t = ets_find(name);
    if (!t) return NULL;

    for (int i = 0; i < t->count; i++) {
        if (cell_equal(t->keys[i], key)) {
            cell_retain(t->values[i]);
            return t->values[i];
        }
    }
    return NULL;  /* key not found */
}

int ets_delete_key(const char* name, Cell* key) {
    EtsTable* t = ets_find(name);
    if (!t) return -1;

    for (int i = 0; i < t->count; i++) {
        if (cell_equal(t->keys[i], key)) {
            cell_release(t->keys[i]);
            cell_release(t->values[i]);
            t->count--;
            if (i < t->count) {
                t->keys[i] = t->keys[t->count];
                t->values[i] = t->values[t->count];
            }
            return 0;
        }
    }
    return -2;  /* key not found */
}

static void ets_destroy_table(EtsTable* t) {
    for (int i = 0; i < t->count; i++) {
        if (t->keys[i]) cell_release(t->keys[i]);
        if (t->values[i]) cell_release(t->values[i]);
    }
    free((void*)t->name);
    t->name = NULL;
    t->count = 0;
    t->active = false;
}

int ets_delete_table(const char* name) {
    EtsTable* t = ets_find(name);
    if (!t) return -1;
    ets_destroy_table(t);
    return 0;
}

int ets_size(const char* name) {
    EtsTable* t = ets_find(name);
    if (!t) return -1;
    return t->count;
}

Cell* ets_all(const char* name) {
    EtsTable* t = ets_find(name);
    if (!t) return NULL;

    Cell* list = cell_nil();
    for (int i = t->count - 1; i >= 0; i--) {
        Cell* pair = cell_cons(t->keys[i], t->values[i]);
        Cell* new_list = cell_cons(pair, list);
        cell_release(pair);
        cell_release(list);
        list = new_list;
    }
    return list;
}

void ets_destroy_by_owner(int actor_id) {
    for (int i = 0; i < g_ets_count; i++) {
        if (g_ets_tables[i].active && g_ets_tables[i].owner_actor_id == actor_id) {
            ets_destroy_table(&g_ets_tables[i]);
        }
    }
}

void ets_reset_all(void) {
    for (int i = 0; i < g_ets_count; i++) {
        if (g_ets_tables[i].active) {
            ets_destroy_table(&g_ets_tables[i]);
        }
    }
    g_ets_count = 0;
}

void actor_reset_all(void) {
    /* Reset ETS tables */
    ets_reset_all();

    /* Reset timers */
    timer_reset_all();

    /* Reset named registry */
    actor_registry_reset();

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

    /* Reset applications */
    app_reset_all();

    /* Reset agents */
    agent_reset_all();

    /* Reset stages */
    stage_reset_all();

    /* Reset flows */
    flow_reset_all();

    /* Reset flow registry */
    flow_registry_reset();
}

/* ============ Application ============ */

static Application g_applications[MAX_APPLICATIONS];
static int g_app_count = 0;

int app_start(const char* name, int supervisor_id, Cell* stop_fn) {
    /* Check duplicate name */
    for (int i = 0; i < g_app_count; i++) {
        if (g_applications[i].running && strcmp(g_applications[i].name, name) == 0) {
            return -1; /* duplicate */
        }
    }
    if (g_app_count >= MAX_APPLICATIONS) return -2; /* full */

    Application* app = &g_applications[g_app_count++];
    app->name = name;
    app->supervisor_id = supervisor_id;
    app->stop_fn = stop_fn;
    if (stop_fn) cell_retain(stop_fn);
    app->env_count = 0;
    app->running = true;
    return 0;
}

int app_stop(const char* name) {
    for (int i = 0; i < g_app_count; i++) {
        if (g_applications[i].running && strcmp(g_applications[i].name, name) == 0) {
            Application* app = &g_applications[i];
            app->running = false;

            /* Release env */
            for (int j = 0; j < app->env_count; j++) {
                if (app->env_keys[j]) cell_release(app->env_keys[j]);
                if (app->env_vals[j]) cell_release(app->env_vals[j]);
                app->env_keys[j] = NULL;
                app->env_vals[j] = NULL;
            }
            app->env_count = 0;

            /* Release stop_fn */
            if (app->stop_fn) {
                cell_release(app->stop_fn);
                app->stop_fn = NULL;
            }

            return 0;
        }
    }
    return -1; /* not found */
}

Application* app_lookup(const char* name) {
    for (int i = 0; i < g_app_count; i++) {
        if (g_applications[i].running && strcmp(g_applications[i].name, name) == 0) {
            return &g_applications[i];
        }
    }
    return NULL;
}

Cell* app_which(void) {
    Cell* result = cell_nil();
    for (int i = g_app_count - 1; i >= 0; i--) {
        if (g_applications[i].running) {
            Cell* name_sym = cell_symbol(g_applications[i].name);
            Cell* new_result = cell_cons(name_sym, result);
            cell_release(name_sym);
            cell_release(result);
            result = new_result;
        }
    }
    return result;
}

Cell* app_get_env(const char* name, Cell* key) {
    Application* app = app_lookup(name);
    if (!app) return NULL;

    for (int i = 0; i < app->env_count; i++) {
        if (cell_equal(app->env_keys[i], key)) {
            cell_retain(app->env_vals[i]);
            return app->env_vals[i];
        }
    }
    return NULL;
}

int app_set_env(const char* name, Cell* key, Cell* value) {
    Application* app = app_lookup(name);
    if (!app) return -1;

    /* Overwrite existing key */
    for (int i = 0; i < app->env_count; i++) {
        if (cell_equal(app->env_keys[i], key)) {
            cell_release(app->env_vals[i]);
            cell_retain(value);
            app->env_vals[i] = value;
            return 0;
        }
    }

    /* Insert new */
    if (app->env_count >= MAX_APP_ENV) return -2;
    cell_retain(key);
    cell_retain(value);
    app->env_keys[app->env_count] = key;
    app->env_vals[app->env_count] = value;
    app->env_count++;
    return 0;
}

void app_reset_all(void) {
    for (int i = 0; i < g_app_count; i++) {
        Application* app = &g_applications[i];
        if (app->running) {
            for (int j = 0; j < app->env_count; j++) {
                if (app->env_keys[j]) cell_release(app->env_keys[j]);
                if (app->env_vals[j]) cell_release(app->env_vals[j]);
            }
            if (app->stop_fn) cell_release(app->stop_fn);
        }
        memset(app, 0, sizeof(Application));
    }
    g_app_count = 0;
}

/* ============ Agent (functional state wrapper) ============ */

static AgentState g_agents[MAX_AGENTS];
static int g_agent_count = 0;
static int g_next_agent_id = 0;

int agent_start(Cell* initial_state) {
    if (g_agent_count >= MAX_AGENTS) return -1;
    for (int i = 0; i < MAX_AGENTS; i++) {
        if (!g_agents[i].active) {
            g_agents[i].id = g_next_agent_id++;
            g_agents[i].state = initial_state;
            cell_retain(initial_state);
            g_agents[i].active = true;
            g_agent_count++;
            return g_agents[i].id;
        }
    }
    return -1;
}

static AgentState* agent_lookup(int id) {
    for (int i = 0; i < MAX_AGENTS; i++) {
        if (g_agents[i].active && g_agents[i].id == id) {
            return &g_agents[i];
        }
    }
    return NULL;
}

Cell* agent_get_state(int id) {
    AgentState* ag = agent_lookup(id);
    if (!ag) return NULL;
    return ag->state;
}

int agent_set_state(int id, Cell* st) {
    AgentState* ag = agent_lookup(id);
    if (!ag) return -1;
    cell_retain(st);
    cell_release(ag->state);
    ag->state = st;
    return 0;
}

int agent_stop(int id) {
    AgentState* ag = agent_lookup(id);
    if (!ag) return -1;
    cell_release(ag->state);
    ag->state = NULL;
    ag->active = false;
    g_agent_count--;
    return 0;
}

void agent_reset_all(void) {
    for (int i = 0; i < MAX_AGENTS; i++) {
        if (g_agents[i].active) {
            if (g_agents[i].state) cell_release(g_agents[i].state);
        }
        g_agents[i].active = false;
        g_agents[i].state = NULL;
        g_agents[i].id = 0;
    }
    g_agent_count = 0;
    g_next_agent_id = 0;
}

/* ============ GenStage (Producer-Consumer Pipelines) ============ */

static GenStage g_stages[MAX_STAGES];
static int g_stage_count = 0;
static int g_next_stage_id = 0;

int stage_create(StageMode mode, Cell* handler, Cell* state) {
    if (g_stage_count >= MAX_STAGES) return -1;
    for (int i = 0; i < MAX_STAGES; i++) {
        if (!g_stages[i].active) {
            g_stages[i].id = g_next_stage_id++;
            g_stages[i].mode = mode;
            g_stages[i].handler = handler;
            cell_retain(handler);
            g_stages[i].state = state;
            cell_retain(state);
            g_stages[i].subscriber_count = 0;
            g_stages[i].active = true;
            g_stage_count++;
            return g_stages[i].id;
        }
    }
    return -1;
}

GenStage* stage_lookup(int id) {
    for (int i = 0; i < MAX_STAGES; i++) {
        if (g_stages[i].active && g_stages[i].id == id) {
            return &g_stages[i];
        }
    }
    return NULL;
}

int stage_subscribe(int consumer_id, int producer_id) {
    GenStage* producer = stage_lookup(producer_id);
    if (!producer) return -1;
    GenStage* consumer = stage_lookup(consumer_id);
    if (!consumer) return -1;
    if (producer->subscriber_count >= MAX_STAGE_SUBSCRIBERS) return -2;
    producer->subscribers[producer->subscriber_count++] = consumer_id;
    return 0;
}

int stage_stop(int id) {
    GenStage* s = stage_lookup(id);
    if (!s) return -1;
    if (s->handler) cell_release(s->handler);
    if (s->state) cell_release(s->state);
    s->handler = NULL;
    s->state = NULL;
    s->active = false;
    g_stage_count--;
    return 0;
}

void stage_reset_all(void) {
    for (int i = 0; i < MAX_STAGES; i++) {
        if (g_stages[i].active) {
            if (g_stages[i].handler) cell_release(g_stages[i].handler);
            if (g_stages[i].state) cell_release(g_stages[i].state);
        }
        g_stages[i].active = false;
        g_stages[i].handler = NULL;
        g_stages[i].state = NULL;
        g_stages[i].id = 0;
        g_stages[i].subscriber_count = 0;
    }
    g_stage_count = 0;
    g_next_stage_id = 0;
}

/* ============ Flow (lazy computation pipelines) ============ */

static Flow g_flows[MAX_FLOWS];
static int g_flow_count = 0;
static int g_next_flow_id = 0;

int flow_create(Cell* source) {
    if (g_flow_count >= MAX_FLOWS) return -1;
    for (int i = 0; i < MAX_FLOWS; i++) {
        if (!g_flows[i].active) {
            g_flows[i].id = g_next_flow_id++;
            g_flows[i].source = source;
            if (source) cell_retain(source);
            g_flows[i].step_count = 0;
            g_flows[i].active = true;
            g_flow_count++;
            return g_flows[i].id;
        }
    }
    return -1;
}

Flow* flow_lookup(int id) {
    for (int i = 0; i < MAX_FLOWS; i++) {
        if (g_flows[i].active && g_flows[i].id == id) {
            return &g_flows[i];
        }
    }
    return NULL;
}

int flow_add_step(int id, FlowStepType type, Cell* fn, Cell* init) {
    Flow* f = flow_lookup(id);
    if (!f) return -1;
    if (f->step_count >= MAX_FLOW_STEPS) return -2;
    int idx = f->step_count++;
    f->steps[idx].type = type;
    f->steps[idx].fn = fn;
    if (fn) cell_retain(fn);
    f->steps[idx].init = init;
    if (init) cell_retain(init);
    return 0;
}

void flow_reset_all(void) {
    for (int i = 0; i < MAX_FLOWS; i++) {
        if (g_flows[i].active) {
            if (g_flows[i].source) cell_release(g_flows[i].source);
            for (int j = 0; j < g_flows[i].step_count; j++) {
                if (g_flows[i].steps[j].fn) cell_release(g_flows[i].steps[j].fn);
                if (g_flows[i].steps[j].init) cell_release(g_flows[i].steps[j].init);
            }
        }
        g_flows[i].active = false;
        g_flows[i].source = NULL;
        g_flows[i].step_count = 0;
        g_flows[i].id = 0;
    }
    g_flow_count = 0;
    g_next_flow_id = 0;
}

/* ============ Named Flow Registry ============ */

static char* g_flow_registry_names[MAX_FLOW_REGISTRY];
static int   g_flow_registry_ids[MAX_FLOW_REGISTRY];
static int   g_flow_registry_count = 0;

int flow_registry_register(const char* name, int flow_id) {
    if (g_flow_registry_count >= MAX_FLOW_REGISTRY) return -3;

    /* Check flow exists */
    Flow* f = flow_lookup(flow_id);
    if (!f) return -4;

    for (int i = 0; i < g_flow_registry_count; i++) {
        if (strcmp(g_flow_registry_names[i], name) == 0) return -1; /* dup name */
        if (g_flow_registry_ids[i] == flow_id) return -2;           /* dup flow */
    }

    g_flow_registry_names[g_flow_registry_count] = strdup(name);
    g_flow_registry_ids[g_flow_registry_count] = flow_id;
    g_flow_registry_count++;
    return 0;
}

int flow_registry_lookup(const char* name) {
    for (int i = 0; i < g_flow_registry_count; i++) {
        if (strcmp(g_flow_registry_names[i], name) == 0) {
            return g_flow_registry_ids[i];
        }
    }
    return -1;
}

int flow_registry_unregister_name(const char* name) {
    for (int i = 0; i < g_flow_registry_count; i++) {
        if (strcmp(g_flow_registry_names[i], name) == 0) {
            free(g_flow_registry_names[i]);
            g_flow_registry_names[i] = g_flow_registry_names[--g_flow_registry_count];
            g_flow_registry_ids[i] = g_flow_registry_ids[g_flow_registry_count];
            return 0;
        }
    }
    return -1;
}

Cell* flow_registry_list(void) {
    Cell* list = cell_nil();
    for (int i = g_flow_registry_count - 1; i >= 0; i--) {
        Cell* sym = cell_symbol(g_flow_registry_names[i]);
        Cell* new_list = cell_cons(sym, list);
        cell_release(sym);
        cell_release(list);
        list = new_list;
    }
    return list;
}

void flow_registry_reset(void) {
    for (int i = 0; i < g_flow_registry_count; i++) {
        free(g_flow_registry_names[i]);
        g_flow_registry_names[i] = NULL;
    }
    g_flow_registry_count = 0;
}
