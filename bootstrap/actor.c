#include "actor.h"
#include "channel.h"
#include "scheduler.h"
#include "log.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Global actor registry */
static Actor* g_actors[MAX_ACTORS];
static int g_actor_count = 0;
static int g_next_actor_id = 1;

/* Striped locks for actor registry (4 stripes, hash by actor ID) */
#define ACTOR_LOCK_STRIPES 4
static pthread_mutex_t g_actor_locks[ACTOR_LOCK_STRIPES];
#define ACTOR_STRIPE(id) (&g_actor_locks[(id) & (ACTOR_LOCK_STRIPES - 1)])

/* ── Per-subsystem locks (Day 132) ── */
static pthread_rwlock_t g_registry_lock = PTHREAD_RWLOCK_INITIALIZER;
static pthread_rwlock_t g_ets_lock      = PTHREAD_RWLOCK_INITIALIZER;
static pthread_mutex_t  g_supervisor_lock;
static pthread_mutex_t  g_timer_lock;
static pthread_mutex_t  g_agent_lock;
static pthread_mutex_t  g_stage_lock;
static pthread_mutex_t  g_flow_lock;
static pthread_mutex_t  g_app_lock;
static pthread_mutex_t  g_flow_reg_lock;

void actor_locks_init(void) {
    for (int i = 0; i < ACTOR_LOCK_STRIPES; i++) {
        pthread_mutex_init(&g_actor_locks[i], NULL);
    }
    pthread_rwlock_init(&g_registry_lock, NULL);
    pthread_rwlock_init(&g_ets_lock, NULL);
    pthread_mutex_init(&g_supervisor_lock, NULL);
    pthread_mutex_init(&g_timer_lock, NULL);
    pthread_mutex_init(&g_agent_lock, NULL);
    pthread_mutex_init(&g_stage_lock, NULL);
    pthread_mutex_init(&g_flow_lock, NULL);
    pthread_mutex_init(&g_app_lock, NULL);
    pthread_mutex_init(&g_flow_reg_lock, NULL);
}

void actor_locks_destroy(void) {
    for (int i = 0; i < ACTOR_LOCK_STRIPES; i++) {
        pthread_mutex_destroy(&g_actor_locks[i]);
    }
    pthread_rwlock_destroy(&g_registry_lock);
    pthread_rwlock_destroy(&g_ets_lock);
    pthread_mutex_destroy(&g_supervisor_lock);
    pthread_mutex_destroy(&g_timer_lock);
    pthread_mutex_destroy(&g_agent_lock);
    pthread_mutex_destroy(&g_stage_lock);
    pthread_mutex_destroy(&g_flow_lock);
    pthread_mutex_destroy(&g_app_lock);
    pthread_mutex_destroy(&g_flow_reg_lock);
}

/* Thread-local current actor (set during scheduler tick) */
static _Thread_local Actor* g_current_actor = NULL;

Actor* actor_current(void) {
    return g_current_actor;
}

void actor_set_current(Actor* actor) {
    g_current_actor = actor;
}

/* Forward declarations for mailbox helpers */
static void mailbox_init(Mailbox* mb, uint32_t capacity);
static void mailbox_destroy(Mailbox* mb);

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
    mailbox_init(&actor->mailbox, MAILBOX_DEFAULT_CAP);
    actor->alive = true;
    actor->result = NULL;
    actor->home_scheduler = (int)tls_scheduler_id;
    atomic_init(&actor->wait_flag, 0);
    atomic_init(&actor->trace_seq, 0);
    actor->trace_origin = 0;
    actor->trace_causal = false;

    /* Build application: (behavior self-actor-cell)
     * We create the actor cell first, then build the call expr.
     * The actor cell needs to exist before the fiber starts,
     * so the behavior can reference itself. */

    /* We'll create the CELL_ACTOR externally and pass the call body.
     * For now, create fiber with the behavior body.
     * The caller (prim_spawn) builds the application expression. */
    actor->fiber = fiber_create(ctx, behavior, env, FIBER_DEFAULT_STACK_SIZE);

    /* Register in global registry (rwlock for thread safety) */
    pthread_rwlock_wrlock(&g_registry_lock);
    g_actors[g_actor_count++] = actor;
    pthread_rwlock_unlock(&g_registry_lock);

    /* Increment alive actor counter for termination detection */
    atomic_fetch_add_explicit(&g_alive_actors, 1, memory_order_relaxed);

    /* Trace: actor spawned */
    {
        Actor* parent = g_current_actor;
        trace_record(TRACE_SPAWN, (uint16_t)actor->id, parent ? (uint16_t)parent->id : 0);
    }

    return actor;
}

/* ── Per-actor Vyukov MPMC mailbox ── */

static void mailbox_init(Mailbox* mb, uint32_t capacity) {
    mb->capacity = capacity;
    mb->mask = capacity - 1;
    atomic_init(&mb->enqueue_pos, 0);
    atomic_init(&mb->dequeue_pos, 0);
    atomic_init(&mb->count, 0);
    mb->slots = (MailboxSlot*)calloc(capacity, sizeof(MailboxSlot));
    for (uint32_t i = 0; i < capacity; i++) {
        atomic_init(&mb->slots[i].sequence, (uint64_t)i);
        mb->slots[i].value = NULL;
    }
}

static void mailbox_destroy(Mailbox* mb) {
    if (!mb->slots) return;
    /* Drain remaining messages */
    uint64_t dq = atomic_load_explicit(&mb->dequeue_pos, memory_order_relaxed);
    uint64_t eq = atomic_load_explicit(&mb->enqueue_pos, memory_order_relaxed);
    for (uint64_t i = dq; i < eq; i++) {
        MailboxSlot* slot = &mb->slots[i & mb->mask];
        if (slot->value) {
            cell_release(slot->value);
            slot->value = NULL;
        }
    }
    free(mb->slots);
    mb->slots = NULL;
}

void actor_send(Actor* actor, Cell* message) {
    if (!actor || !actor->alive) return;
    Mailbox* mb = &actor->mailbox;

    uint64_t pos = atomic_load_explicit(&mb->enqueue_pos, memory_order_relaxed);
    for (;;) {
        MailboxSlot* slot = &mb->slots[pos & mb->mask];
        uint64_t seq = atomic_load_explicit(&slot->sequence, memory_order_acquire);
        int64_t diff = (int64_t)seq - (int64_t)pos;

        if (diff == 0) {
            if (atomic_compare_exchange_weak_explicit(&mb->enqueue_pos, &pos, pos + 1,
                    memory_order_relaxed, memory_order_relaxed)) {
                cell_retain(message);
                slot->value = message;
                atomic_store_explicit(&slot->sequence, pos + 1, memory_order_release);
                atomic_fetch_add_explicit(&mb->count, 1, memory_order_relaxed);

                /* Trace: message sent + causal token propagation */
                {
                    Actor* sender = g_current_actor;
                    trace_record(TRACE_SEND, sender ? (uint16_t)sender->id : 0, (uint16_t)actor->id);
                    if (sender && sender->trace_causal) {
                        atomic_fetch_add_explicit(&sender->trace_seq, 1, memory_order_relaxed);
                        actor->trace_origin = (uint16_t)sender->id;
                    }
                }

                /* Wake actor if blocked on mailbox recv (Day 134) */
                if (atomic_exchange_explicit(&actor->wait_flag, 0, memory_order_acq_rel) == 1) {
                    LOG_DEBUG("send: waking actor %d (was waiting)", actor->id);
                    Scheduler* home = sched_get(actor->home_scheduler);
                    if (home) {
                        sched_enqueue(home, actor);
                    }
                }
                return;
            }
        } else if (diff < 0) {
            return; /* Full — drop message */
        } else {
            pos = atomic_load_explicit(&mb->enqueue_pos, memory_order_relaxed);
        }
    }
}

Cell* actor_receive(Actor* actor) {
    Mailbox* mb = &actor->mailbox;
    if (atomic_load_explicit(&mb->count, memory_order_relaxed) <= 0) return NULL;

    uint64_t pos = atomic_load_explicit(&mb->dequeue_pos, memory_order_relaxed);
    for (;;) {
        MailboxSlot* slot = &mb->slots[pos & mb->mask];
        uint64_t seq = atomic_load_explicit(&slot->sequence, memory_order_acquire);
        int64_t diff = (int64_t)seq - (int64_t)(pos + 1);

        if (diff == 0) {
            if (atomic_compare_exchange_weak_explicit(&mb->dequeue_pos, &pos, pos + 1,
                    memory_order_relaxed, memory_order_relaxed)) {
                Cell* value = slot->value;
                slot->value = NULL;
                atomic_store_explicit(&slot->sequence, pos + mb->capacity, memory_order_release);
                atomic_fetch_sub_explicit(&mb->count, 1, memory_order_relaxed);
                trace_record(TRACE_RECV, (uint16_t)actor->id, 0);
                return value; /* Caller owns the ref */
            }
        } else if (diff < 0) {
            return NULL; /* Empty */
        } else {
            pos = atomic_load_explicit(&mb->dequeue_pos, memory_order_relaxed);
        }
    }
}

/* Destroy actor and free resources */
void actor_destroy(Actor* actor) {
    if (!actor) return;

    /* Release remaining mailbox messages */
    mailbox_destroy(&actor->mailbox);

    /* Release process dictionary entries */
    for (int i = 0; i < actor->dict_count; i++) {
        if (actor->dict_keys[i]) cell_release(actor->dict_keys[i]);
        if (actor->dict_values[i]) cell_release(actor->dict_values[i]);
    }
    actor->dict_count = 0;

    if (actor->result) {
        cell_release_shared(actor->result);
    }

    if (actor->fiber) {
        fiber_destroy(actor->fiber);
    }

    free(actor);
}

/* Supervision: bidirectional link */
void actor_link(Actor* a, Actor* b) {
    if (!a || !b) return;
    /* Lock both stripes (ordered by ID to prevent deadlock) */
    int lo = a->id < b->id ? a->id : b->id;
    int hi = a->id < b->id ? b->id : a->id;
    pthread_mutex_lock(ACTOR_STRIPE(lo));
    if ((lo & (ACTOR_LOCK_STRIPES - 1)) != (hi & (ACTOR_LOCK_STRIPES - 1)))
        pthread_mutex_lock(ACTOR_STRIPE(hi));

    /* Add b to a's links (if not already there) */
    bool found_a = false;
    for (int i = 0; i < a->link_count; i++) {
        if (a->links[i] == b->id) { found_a = true; break; }
    }
    if (!found_a && a->link_count < MAX_LINKS) {
        a->links[a->link_count++] = b->id;
    }
    /* Add a to b's links */
    bool found_b = false;
    for (int i = 0; i < b->link_count; i++) {
        if (b->links[i] == a->id) { found_b = true; break; }
    }
    if (!found_b && b->link_count < MAX_LINKS) {
        b->links[b->link_count++] = a->id;
    }

    trace_record(TRACE_LINK, (uint16_t)a->id, (uint16_t)b->id);

    if ((lo & (ACTOR_LOCK_STRIPES - 1)) != (hi & (ACTOR_LOCK_STRIPES - 1)))
        pthread_mutex_unlock(ACTOR_STRIPE(hi));
    pthread_mutex_unlock(ACTOR_STRIPE(lo));
}

void actor_unlink(Actor* a, Actor* b) {
    if (!a || !b) return;
    int lo = a->id < b->id ? a->id : b->id;
    int hi = a->id < b->id ? b->id : a->id;
    pthread_mutex_lock(ACTOR_STRIPE(lo));
    if ((lo & (ACTOR_LOCK_STRIPES - 1)) != (hi & (ACTOR_LOCK_STRIPES - 1)))
        pthread_mutex_lock(ACTOR_STRIPE(hi));

    for (int i = 0; i < a->link_count; i++) {
        if (a->links[i] == b->id) {
            a->links[i] = a->links[--a->link_count];
            break;
        }
    }
    for (int i = 0; i < b->link_count; i++) {
        if (b->links[i] == a->id) {
            b->links[i] = b->links[--b->link_count];
            break;
        }
    }

    if ((lo & (ACTOR_LOCK_STRIPES - 1)) != (hi & (ACTOR_LOCK_STRIPES - 1)))
        pthread_mutex_unlock(ACTOR_STRIPE(hi));
    pthread_mutex_unlock(ACTOR_STRIPE(lo));
}

/* Add watcher as a monitor of target */
/* Returns true if monitor was added (target alive), false if target already dead.
 * Caller must send :DOWN immediately when false is returned — this closes the
 * TOCTOU race between alive-check and monitor registration that caused deadlocks
 * in multi-scheduler mode. */
bool actor_add_monitor(Actor* target, Actor* watcher) {
    if (!target || !watcher) return false;
    pthread_mutex_lock(ACTOR_STRIPE(target->id));
    if (!target->alive) {
        pthread_mutex_unlock(ACTOR_STRIPE(target->id));
        LOG_DEBUG("add_monitor: target %d already dead, watcher %d gets immediate :DOWN",
            target->id, watcher->id);
        return false;  /* Target died between caller's check and our lock */
    }
    if (target->monitor_count < MAX_MONITORS) {
        target->monitors[target->monitor_count++] = watcher->id;
        LOG_DEBUG("add_monitor: target %d watcher %d (count=%d)",
            target->id, watcher->id, target->monitor_count);
        trace_record(TRACE_MONITOR, (uint16_t)target->id, (uint16_t)watcher->id);
    }
    pthread_mutex_unlock(ACTOR_STRIPE(target->id));
    return true;
}

/* Decrement g_alive_actors and wake all schedulers if it hits 0.
 * Centralises the pattern so every kill path notifies parked schedulers. */
static void alive_dec_and_notify(void) {
    int prev = atomic_fetch_sub_explicit(&g_alive_actors, 1, memory_order_acq_rel);
    if (prev == 1) {
        ec_notify_all(&g_sched_ec);
    }
}

/* Send exit signal to target from sender with reason.
 * If target traps exits → deliver as ⟨:EXIT sender-id reason⟩ message.
 * If target does not trap → kill it. */
void actor_exit_signal(Actor* target, Actor* sender, Cell* reason) {
    if (!target || !target->alive) return;
    trace_record(TRACE_EXIT_SIGNAL, (uint16_t)target->id, sender ? (uint16_t)sender->id : 0);

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
        /* Kill the target actor — use stripe lock to prevent double-kill
         * from concurrent exit signals (e.g., two linked actors dying). */
        pthread_mutex_lock(ACTOR_STRIPE(target->id));
        if (!target->alive) {
            pthread_mutex_unlock(ACTOR_STRIPE(target->id));
            return;  /* Already killed by another thread */
        }
        target->alive = false;
        target->result = reason;
        if (reason) cell_retain_shared(reason);  /* shared: cleanup may run on different thread */
        alive_dec_and_notify();
        pthread_mutex_unlock(ACTOR_STRIPE(target->id));
        /* Propagate to target's own links/monitors (outside lock) */
        actor_notify_exit(target, reason);
    }
}

/* Forward declaration — defined after supervisor_handle_exit */
static void dynsup_remove_child_at(Supervisor* sup, int index);

/* Thread-safe actor finish: atomically checks alive, marks dead, stores result,
 * decrements g_alive_actors, and notifies links/monitors.
 * Returns true if this call killed the actor, false if already dead.
 * Used by both sched_run_one_quantum (fiber finished) and actor_run_all. */
bool actor_finish(Actor* actor, Cell* result) {
    pthread_mutex_lock(ACTOR_STRIPE(actor->id));
    if (!actor->alive) {
        pthread_mutex_unlock(ACTOR_STRIPE(actor->id));
        return false;  /* Already killed by exit signal */
    }
    actor->alive = false;
    actor->result = result;
    if (result) {
        /* Transfer fiber->result's biased ref to shared domain.
         * This ensures any thread can properly release actor->result during cleanup,
         * even if the owning worker thread has exited. */
        cell_transfer_to_shared(result);
    }
    /* Break fiber->result alias to prevent double-release in fiber_destroy */
    if (actor->fiber) actor->fiber->result = NULL;
    alive_dec_and_notify();
    LOG_DEBUG("actor_finish: actor %d dead, alive now decremented", actor->id);
    pthread_mutex_unlock(ACTOR_STRIPE(actor->id));

    /* Notify links/monitors outside lock */
    actor_notify_exit(actor, result);

    /* Wake actors suspended via SUSPEND_TASK_AWAIT on this actor.
     * Read g_actors[] under registry read-lock to prevent use-after-free
     * if actor_reset_all runs concurrently. */
    pthread_rwlock_rdlock(&g_registry_lock);
    int count = g_actor_count;
    pthread_rwlock_unlock(&g_registry_lock);
    for (int i = 0; i < count; i++) {
        Actor* waiter = g_actors[i];
        if (!waiter || !waiter->alive || !waiter->fiber) continue;
        if (waiter->fiber->state == FIBER_SUSPENDED &&
            waiter->fiber->suspend_reason == SUSPEND_TASK_AWAIT &&
            waiter->fiber->suspend_await_actor_id == actor->id) {
            if (atomic_exchange_explicit(&waiter->wait_flag, 0, memory_order_acq_rel) == 1) {
                Scheduler* home = sched_get(waiter->home_scheduler);
                if (home) sched_enqueue(home, waiter);
            }
        }
    }

    return true;
}

/* Called when an actor exits. Notifies links and monitors.
 * Thread-safe (Day 134): copies link/monitor arrays under stripe lock,
 * then sends signals outside the lock to avoid deadlock. */
void actor_notify_exit(Actor* exiting, Cell* reason) {
    if (!exiting) return;

    {
        bool is_err = reason && reason->type == CELL_ERROR;
        trace_record(TRACE_DIE, (uint16_t)exiting->id, is_err ? 1 : 0);
    }

    /* Destroy ETS tables owned by this actor */
    ets_destroy_by_owner(exiting->id);

    /* Auto-deregister from named registry */
    actor_registry_unregister_actor(exiting->id);

    bool is_error = reason && reason->type == CELL_ERROR;

    /* Check if this actor belongs to a supervisor */
    Supervisor* sup = supervisor_find_for_child(exiting->id);
    if (sup) {
        if (is_error) {
            supervisor_handle_exit(sup, exiting->id, reason);
        } else if (sup->is_dynamic) {
            int idx = -1;
            for (int i = 0; i < sup->child_count; i++) {
                if (sup->child_ids[i] == exiting->id) { idx = i; break; }
            }
            if (idx >= 0) {
                ChildRestartType rt = sup->child_restart[idx];
                if (rt == CHILD_TEMPORARY || rt == CHILD_TRANSIENT) {
                    dynsup_remove_child_at(sup, idx);
                }
            }
        }
    }

    /* Copy link/monitor arrays under stripe lock to avoid races */
    int monitor_ids[MAX_MONITORS];
    int monitor_count;
    int link_ids[MAX_LINKS];
    int link_count;

    pthread_mutex_lock(ACTOR_STRIPE(exiting->id));
    monitor_count = exiting->monitor_count;
    memcpy(monitor_ids, exiting->monitors, monitor_count * sizeof(int));
    link_count = exiting->link_count;
    memcpy(link_ids, exiting->links, link_count * sizeof(int));
    exiting->monitor_count = 0;
    exiting->link_count = 0;
    pthread_mutex_unlock(ACTOR_STRIPE(exiting->id));

    /* Notify monitors: send ⟨:DOWN id reason⟩ message (outside lock) */
    for (int i = 0; i < monitor_count; i++) {
        Actor* watcher = actor_lookup(monitor_ids[i]);
        LOG_DEBUG("notify_exit: actor %d -> monitor watcher %d (found=%d alive=%d)",
            exiting->id, monitor_ids[i], watcher != NULL, watcher ? watcher->alive : 0);
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

    /* Notify linked actors (outside lock) */
    for (int i = 0; i < link_count; i++) {
        Actor* linked = actor_lookup(link_ids[i]);
        if (linked && linked->alive) {
            if (is_error) {
                actor_exit_signal(linked, exiting, reason);
            } else {
                if (linked->trap_exit) {
                    Cell* msg = cell_cons(
                        cell_symbol(":EXIT"),
                        cell_cons(
                            cell_number(exiting->id),
                            cell_cons(cell_symbol(":normal"), cell_nil())));
                    actor_send(linked, msg);
                    cell_release(msg);
                }
            }
        }
    }
}

/* ─── Supervisor ─── */

static Supervisor* g_supervisors[MAX_SUPERVISORS];
static int g_supervisor_count = 0;
static int g_next_supervisor_id = 1;

Supervisor* supervisor_create(EvalContext* ctx, SupervisorStrategy strategy, Cell* specs) {
    pthread_mutex_lock(&g_supervisor_lock);
    if (g_supervisor_count >= MAX_SUPERVISORS) { pthread_mutex_unlock(&g_supervisor_lock); return NULL; }

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
    pthread_mutex_unlock(&g_supervisor_lock);
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
            if (!child) continue;
            pthread_mutex_lock(ACTOR_STRIPE(child->id));
            if (child->alive) {
                child->alive = false;
                child->result = cell_symbol(":shutdown");
                cell_transfer_to_shared(child->result);
                alive_dec_and_notify();
                pthread_mutex_unlock(ACTOR_STRIPE(child->id));
                actor_notify_exit(child, child->result);
            } else {
                pthread_mutex_unlock(ACTOR_STRIPE(child->id));
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
            if (!child) continue;
            pthread_mutex_lock(ACTOR_STRIPE(child->id));
            if (child->alive) {
                child->alive = false;
                child->result = cell_symbol(":shutdown");
                cell_transfer_to_shared(child->result);
                alive_dec_and_notify();
                pthread_mutex_unlock(ACTOR_STRIPE(child->id));
                actor_notify_exit(child, child->result);
            } else {
                pthread_mutex_unlock(ACTOR_STRIPE(child->id));
            }
        }
        /* Respawn from dead_index to end */
        for (int i = dead_index; i < sup->child_count; i++) {
            supervisor_spawn_child(sup, i);
        }
    }
}

Supervisor* supervisor_lookup(int id) {
    pthread_mutex_lock(&g_supervisor_lock);
    for (int i = 0; i < g_supervisor_count; i++) {
        if (g_supervisors[i] && g_supervisors[i]->id == id) {
            Supervisor* s = g_supervisors[i];
            pthread_mutex_unlock(&g_supervisor_lock);
            return s;
        }
    }
    pthread_mutex_unlock(&g_supervisor_lock);
    return NULL;
}

Supervisor* supervisor_find_for_child(int child_id) {
    pthread_mutex_lock(&g_supervisor_lock);
    for (int i = 0; i < g_supervisor_count; i++) {
        Supervisor* sup = g_supervisors[i];
        if (!sup) continue;
        for (int j = 0; j < sup->child_count; j++) {
            if (sup->child_ids[j] == child_id) {
                pthread_mutex_unlock(&g_supervisor_lock);
                return sup;
            }
        }
    }
    pthread_mutex_unlock(&g_supervisor_lock);
    return NULL;
}

/* Lookup actor by ID (thread-safe via rwlock) */
Actor* actor_lookup(int id) {
    pthread_rwlock_rdlock(&g_registry_lock);
    for (int i = 0; i < g_actor_count; i++) {
        if (g_actors[i] && g_actors[i]->id == id) {
            Actor* a = g_actors[i];
            pthread_rwlock_unlock(&g_registry_lock);
            return a;
        }
    }
    pthread_rwlock_unlock(&g_registry_lock);
    return NULL;
}

/* Lookup actor by array index (for scheduler distribution) */
Actor* actor_lookup_by_index(int index) {
    if (index < 0 || index >= g_actor_count) return NULL;
    return g_actors[index];
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
                        if (atomic_load_explicit(&actor->mailbox.count, memory_order_relaxed) == 0) continue;
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
                    case SUSPEND_REDUCTION:
                        break; /* Immediately runnable — reduction budget expired */
                }
            }

            /* Set current actor so ←? can find it */
            Actor* prev_actor = g_current_actor;
            Fiber* prev_fiber = fiber_current();
            g_current_actor = actor;
            fiber_set_current(fiber);

            if (fiber->state == FIBER_READY) {
                /* First run — set reduction budget */
                fiber->eval_ctx->reductions_left = CONTEXT_REDS;
                trace_record(TRACE_RESUME, (uint16_t)actor->id, 0);
                fiber_start(fiber);
                any_ran = true;
            } else if (fiber->state == FIBER_SUSPENDED) {
                /* Clear wait_flag — we're resuming this actor directly */
                atomic_store_explicit(&actor->wait_flag, 0, memory_order_relaxed);
                /* Clear select waiter registrations before resume */
                if (fiber->suspend_reason == SUSPEND_SELECT) {
                    for (int j = 0; j < fiber->suspend_select_count; j++) {
                        Channel* ch = channel_lookup(fiber->suspend_select_ids[j]);
                        if (ch) {
                            int expected = actor->id;
                            atomic_compare_exchange_strong_explicit(
                                &ch->recv_waiter, &expected, -1,
                                memory_order_relaxed, memory_order_relaxed);
                        }
                    }
                }
                Cell* resume_val = NULL;
                trace_record(TRACE_RESUME, (uint16_t)actor->id, (uint16_t)fiber->suspend_reason);

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
                        int start = fiber->select_round % total;
                        fiber->select_round++;
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
                    case SUSPEND_REDUCTION:
                        /* Reduction budget expired — resume with nil, fiber_entry
                         * will re-evaluate from saved continuation */
                        resume_val = cell_nil();
                        break;
                }

                /* Clear stale suspend metadata after computing resume value */
                fiber->suspend_channel_id = -1;
                fiber->suspend_await_actor_id = -1;
                fiber->suspend_select_count = 0;
                fiber->suspend_reason = SUSPEND_GENERAL;
                /* Reset reduction budget before resuming */
                fiber->eval_ctx->reductions_left = CONTEXT_REDS;
                fiber_resume(fiber, resume_val);
                if (resume_val) cell_release(resume_val);
                any_ran = true;
            }

            /* Check if actor finished */
            if (fiber->state == FIBER_FINISHED) {
                actor_finish(actor, fiber->result);
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
    pthread_rwlock_wrlock(&g_registry_lock);
    if (g_registry_count >= MAX_REGISTRY) { pthread_rwlock_unlock(&g_registry_lock); return -3; }

    Actor* actor = actor_lookup(actor_id);
    if (!actor || !actor->alive) { pthread_rwlock_unlock(&g_registry_lock); return -4; }

    for (int i = 0; i < g_registry_count; i++) {
        if (strcmp(g_registry_names[i], name) == 0) { pthread_rwlock_unlock(&g_registry_lock); return -1; }
        if (g_registry_ids[i] == actor_id) { pthread_rwlock_unlock(&g_registry_lock); return -2; }
    }

    g_registry_names[g_registry_count] = strdup(name);
    g_registry_ids[g_registry_count] = actor_id;
    g_registry_count++;
    pthread_rwlock_unlock(&g_registry_lock);
    return 0;
}

int actor_registry_lookup(const char* name) {
    pthread_rwlock_rdlock(&g_registry_lock);
    for (int i = 0; i < g_registry_count; i++) {
        if (strcmp(g_registry_names[i], name) == 0) {
            int id = g_registry_ids[i];
            pthread_rwlock_unlock(&g_registry_lock);
            return id;
        }
    }
    pthread_rwlock_unlock(&g_registry_lock);
    return -1;
}

int actor_registry_unregister_name(const char* name) {
    pthread_rwlock_wrlock(&g_registry_lock);
    for (int i = 0; i < g_registry_count; i++) {
        if (strcmp(g_registry_names[i], name) == 0) {
            free(g_registry_names[i]);
            g_registry_names[i] = g_registry_names[--g_registry_count];
            g_registry_ids[i] = g_registry_ids[g_registry_count];
            pthread_rwlock_unlock(&g_registry_lock);
            return 0;
        }
    }
    pthread_rwlock_unlock(&g_registry_lock);
    return -1;
}

void actor_registry_unregister_actor(int actor_id) {
    pthread_rwlock_wrlock(&g_registry_lock);
    for (int i = 0; i < g_registry_count; i++) {
        if (g_registry_ids[i] == actor_id) {
            free(g_registry_names[i]);
            g_registry_names[i] = g_registry_names[--g_registry_count];
            g_registry_ids[i] = g_registry_ids[g_registry_count];
            pthread_rwlock_unlock(&g_registry_lock);
            return;
        }
    }
    pthread_rwlock_unlock(&g_registry_lock);
}

Cell* actor_registry_list(void) {
    pthread_rwlock_rdlock(&g_registry_lock);
    Cell* list = cell_nil();
    for (int i = g_registry_count - 1; i >= 0; i--) {
        Cell* sym = cell_symbol(g_registry_names[i]);
        Cell* new_list = cell_cons(sym, list);
        cell_release(sym);
        cell_release(list);
        list = new_list;
    }
    pthread_rwlock_unlock(&g_registry_lock);
    return list;
}

void actor_registry_reset(void) {
    pthread_rwlock_wrlock(&g_registry_lock);
    for (int i = 0; i < g_registry_count; i++) {
        free(g_registry_names[i]);
        g_registry_names[i] = NULL;
    }
    g_registry_count = 0;
    pthread_rwlock_unlock(&g_registry_lock);
}

/* Reset all actors (for testing) */
/* ─── Timers ─── */

static Timer g_timers[MAX_TIMERS];
static int g_timer_count = 0;
static int g_next_timer_id = 1;

int timer_create(int target_actor_id, int ticks, Cell* message) {
    pthread_mutex_lock(&g_timer_lock);
    if (g_timer_count >= MAX_TIMERS) { pthread_mutex_unlock(&g_timer_lock); return -1; }
    int id = g_next_timer_id++;
    Timer* t = &g_timers[g_timer_count++];
    t->id = id;
    t->target_actor_id = target_actor_id;
    t->remaining_ticks = ticks;
    t->message = message;
    if (message) cell_retain(message);
    t->active = true;
    pthread_mutex_unlock(&g_timer_lock);
    return id;
}

int timer_cancel(int timer_id) {
    pthread_mutex_lock(&g_timer_lock);
    for (int i = 0; i < g_timer_count; i++) {
        if (g_timers[i].id == timer_id && g_timers[i].active) {
            g_timers[i].active = false;
            if (g_timers[i].message) {
                cell_release(g_timers[i].message);
                g_timers[i].message = NULL;
            }
            pthread_mutex_unlock(&g_timer_lock);
            return 0;
        }
    }
    pthread_mutex_unlock(&g_timer_lock);
    return -1;
}

bool timer_active(int timer_id) {
    pthread_mutex_lock(&g_timer_lock);
    for (int i = 0; i < g_timer_count; i++) {
        if (g_timers[i].id == timer_id) {
            bool result = g_timers[i].active;
            pthread_mutex_unlock(&g_timer_lock);
            return result;
        }
    }
    pthread_mutex_unlock(&g_timer_lock);
    return false;
}

bool timer_tick_all(void) {
    pthread_mutex_lock(&g_timer_lock);
    bool any_fired = false;
    for (int i = 0; i < g_timer_count; i++) {
        Timer* t = &g_timers[i];
        if (!t->active) continue;
        if (t->remaining_ticks > 0) {
            t->remaining_ticks--;
            continue;
        }
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
    pthread_mutex_unlock(&g_timer_lock);
    return any_fired;
}

bool timer_any_pending(void) {
    pthread_mutex_lock(&g_timer_lock);
    for (int i = 0; i < g_timer_count; i++) {
        if (g_timers[i].active) { pthread_mutex_unlock(&g_timer_lock); return true; }
    }
    pthread_mutex_unlock(&g_timer_lock);
    return false;
}

void timer_reset_all(void) {
    pthread_mutex_lock(&g_timer_lock);
    for (int i = 0; i < g_timer_count; i++) {
        if (g_timers[i].message) {
            cell_release(g_timers[i].message);
            g_timers[i].message = NULL;
        }
        g_timers[i].active = false;
    }
    g_timer_count = 0;
    g_next_timer_id = 1;
    pthread_mutex_unlock(&g_timer_lock);
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
    pthread_rwlock_wrlock(&g_ets_lock);
    if (ets_find(name)) { pthread_rwlock_unlock(&g_ets_lock); return -1; }
    if (g_ets_count >= MAX_ETS_TABLES) { pthread_rwlock_unlock(&g_ets_lock); return -2; }

    EtsTable* t = &g_ets_tables[g_ets_count++];
    t->name = strdup(name);
    t->owner_actor_id = owner_actor_id;
    t->count = 0;
    t->active = true;
    pthread_rwlock_unlock(&g_ets_lock);
    return 0;
}

int ets_insert(const char* name, Cell* key, Cell* value) {
    pthread_rwlock_wrlock(&g_ets_lock);
    EtsTable* t = ets_find(name);
    if (!t) { pthread_rwlock_unlock(&g_ets_lock); return -1; }

    for (int i = 0; i < t->count; i++) {
        if (cell_equal(t->keys[i], key)) {
            Cell* old = t->values[i];
            cell_retain(value);
            t->values[i] = value;
            cell_release(old);
            pthread_rwlock_unlock(&g_ets_lock);
            return 0;
        }
    }

    if (t->count >= MAX_ETS_ENTRIES) { pthread_rwlock_unlock(&g_ets_lock); return -2; }
    cell_retain(key);
    cell_retain(value);
    t->keys[t->count] = key;
    t->values[t->count] = value;
    t->count++;
    pthread_rwlock_unlock(&g_ets_lock);
    return 0;
}

Cell* ets_lookup(const char* name, Cell* key) {
    pthread_rwlock_rdlock(&g_ets_lock);
    EtsTable* t = ets_find(name);
    if (!t) { pthread_rwlock_unlock(&g_ets_lock); return NULL; }

    for (int i = 0; i < t->count; i++) {
        if (cell_equal(t->keys[i], key)) {
            cell_retain(t->values[i]);
            Cell* result = t->values[i];
            pthread_rwlock_unlock(&g_ets_lock);
            return result;
        }
    }
    pthread_rwlock_unlock(&g_ets_lock);
    return NULL;
}

int ets_delete_key(const char* name, Cell* key) {
    pthread_rwlock_wrlock(&g_ets_lock);
    EtsTable* t = ets_find(name);
    if (!t) { pthread_rwlock_unlock(&g_ets_lock); return -1; }

    for (int i = 0; i < t->count; i++) {
        if (cell_equal(t->keys[i], key)) {
            cell_release(t->keys[i]);
            cell_release(t->values[i]);
            t->count--;
            if (i < t->count) {
                t->keys[i] = t->keys[t->count];
                t->values[i] = t->values[t->count];
            }
            pthread_rwlock_unlock(&g_ets_lock);
            return 0;
        }
    }
    pthread_rwlock_unlock(&g_ets_lock);
    return -2;
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
    pthread_rwlock_wrlock(&g_ets_lock);
    EtsTable* t = ets_find(name);
    if (!t) { pthread_rwlock_unlock(&g_ets_lock); return -1; }
    ets_destroy_table(t);
    pthread_rwlock_unlock(&g_ets_lock);
    return 0;
}

int ets_size(const char* name) {
    pthread_rwlock_rdlock(&g_ets_lock);
    EtsTable* t = ets_find(name);
    int result = t ? t->count : -1;
    pthread_rwlock_unlock(&g_ets_lock);
    return result;
}

Cell* ets_all(const char* name) {
    pthread_rwlock_rdlock(&g_ets_lock);
    EtsTable* t = ets_find(name);
    if (!t) { pthread_rwlock_unlock(&g_ets_lock); return NULL; }

    Cell* list = cell_nil();
    for (int i = t->count - 1; i >= 0; i--) {
        Cell* pair = cell_cons(t->keys[i], t->values[i]);
        Cell* new_list = cell_cons(pair, list);
        cell_release(pair);
        cell_release(list);
        list = new_list;
    }
    pthread_rwlock_unlock(&g_ets_lock);
    return list;
}

void ets_destroy_by_owner(int actor_id) {
    pthread_rwlock_wrlock(&g_ets_lock);
    for (int i = 0; i < g_ets_count; i++) {
        if (g_ets_tables[i].active && g_ets_tables[i].owner_actor_id == actor_id) {
            ets_destroy_table(&g_ets_tables[i]);
        }
    }
    pthread_rwlock_unlock(&g_ets_lock);
}

void ets_reset_all(void) {
    pthread_rwlock_wrlock(&g_ets_lock);
    for (int i = 0; i < g_ets_count; i++) {
        if (g_ets_tables[i].active) {
            ets_destroy_table(&g_ets_tables[i]);
        }
    }
    g_ets_count = 0;
    pthread_rwlock_unlock(&g_ets_lock);
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
    }
    g_actor_count = 0;
    g_next_actor_id = 1;
    g_current_actor = NULL;
    /* Reset alive counter — actor_destroy does NOT decrement g_alive_actors
     * (only actor_finish does). Without this, stale counts leak across
     * sched_run_all calls and break termination detection. */
    atomic_store_explicit(&g_alive_actors, 0, memory_order_relaxed);
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
    pthread_mutex_lock(&g_app_lock);
    for (int i = 0; i < g_app_count; i++) {
        if (g_applications[i].running && strcmp(g_applications[i].name, name) == 0) {
            pthread_mutex_unlock(&g_app_lock);
            return -1;
        }
    }
    if (g_app_count >= MAX_APPLICATIONS) { pthread_mutex_unlock(&g_app_lock); return -2; }

    Application* app = &g_applications[g_app_count++];
    app->name = name;
    app->supervisor_id = supervisor_id;
    app->stop_fn = stop_fn;
    if (stop_fn) cell_retain(stop_fn);
    app->env_count = 0;
    app->running = true;
    pthread_mutex_unlock(&g_app_lock);
    return 0;
}

int app_stop(const char* name) {
    pthread_mutex_lock(&g_app_lock);
    for (int i = 0; i < g_app_count; i++) {
        if (g_applications[i].running && strcmp(g_applications[i].name, name) == 0) {
            Application* app = &g_applications[i];
            app->running = false;
            for (int j = 0; j < app->env_count; j++) {
                if (app->env_keys[j]) cell_release(app->env_keys[j]);
                if (app->env_vals[j]) cell_release(app->env_vals[j]);
                app->env_keys[j] = NULL;
                app->env_vals[j] = NULL;
            }
            app->env_count = 0;
            if (app->stop_fn) {
                cell_release(app->stop_fn);
                app->stop_fn = NULL;
            }
            pthread_mutex_unlock(&g_app_lock);
            return 0;
        }
    }
    pthread_mutex_unlock(&g_app_lock);
    return -1;
}

Application* app_lookup(const char* name) {
    pthread_mutex_lock(&g_app_lock);
    for (int i = 0; i < g_app_count; i++) {
        if (g_applications[i].running && strcmp(g_applications[i].name, name) == 0) {
            Application* result = &g_applications[i];
            pthread_mutex_unlock(&g_app_lock);
            return result;
        }
    }
    pthread_mutex_unlock(&g_app_lock);
    return NULL;
}

Cell* app_which(void) {
    pthread_mutex_lock(&g_app_lock);
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
    pthread_mutex_unlock(&g_app_lock);
    return result;
}

Cell* app_get_env(const char* name, Cell* key) {
    pthread_mutex_lock(&g_app_lock);
    Application* app = NULL;
    for (int i = 0; i < g_app_count; i++) {
        if (g_applications[i].running && strcmp(g_applications[i].name, name) == 0) {
            app = &g_applications[i];
            break;
        }
    }
    if (!app) { pthread_mutex_unlock(&g_app_lock); return NULL; }
    for (int i = 0; i < app->env_count; i++) {
        if (cell_equal(app->env_keys[i], key)) {
            cell_retain(app->env_vals[i]);
            Cell* result = app->env_vals[i];
            pthread_mutex_unlock(&g_app_lock);
            return result;
        }
    }
    pthread_mutex_unlock(&g_app_lock);
    return NULL;
}

int app_set_env(const char* name, Cell* key, Cell* value) {
    pthread_mutex_lock(&g_app_lock);
    Application* app = NULL;
    for (int i = 0; i < g_app_count; i++) {
        if (g_applications[i].running && strcmp(g_applications[i].name, name) == 0) {
            app = &g_applications[i];
            break;
        }
    }
    if (!app) { pthread_mutex_unlock(&g_app_lock); return -1; }
    for (int i = 0; i < app->env_count; i++) {
        if (cell_equal(app->env_keys[i], key)) {
            cell_release(app->env_vals[i]);
            cell_retain(value);
            app->env_vals[i] = value;
            pthread_mutex_unlock(&g_app_lock);
            return 0;
        }
    }
    if (app->env_count >= MAX_APP_ENV) { pthread_mutex_unlock(&g_app_lock); return -2; }
    cell_retain(key);
    cell_retain(value);
    app->env_keys[app->env_count] = key;
    app->env_vals[app->env_count] = value;
    app->env_count++;
    pthread_mutex_unlock(&g_app_lock);
    return 0;
}

void app_reset_all(void) {
    pthread_mutex_lock(&g_app_lock);
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
    pthread_mutex_unlock(&g_app_lock);
}

/* ============ Agent (functional state wrapper) ============ */

static AgentState g_agents[MAX_AGENTS];
static int g_agent_count = 0;
static int g_next_agent_id = 0;

int agent_start(Cell* initial_state) {
    pthread_mutex_lock(&g_agent_lock);
    if (g_agent_count >= MAX_AGENTS) { pthread_mutex_unlock(&g_agent_lock); return -1; }
    for (int i = 0; i < MAX_AGENTS; i++) {
        if (!g_agents[i].active) {
            g_agents[i].id = g_next_agent_id++;
            g_agents[i].state = initial_state;
            cell_retain(initial_state);
            g_agents[i].active = true;
            g_agent_count++;
            int id = g_agents[i].id;
            pthread_mutex_unlock(&g_agent_lock);
            return id;
        }
    }
    pthread_mutex_unlock(&g_agent_lock);
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
    pthread_mutex_lock(&g_agent_lock);
    AgentState* ag = agent_lookup(id);
    Cell* result = ag ? ag->state : NULL;
    pthread_mutex_unlock(&g_agent_lock);
    return result;
}

int agent_set_state(int id, Cell* st) {
    pthread_mutex_lock(&g_agent_lock);
    AgentState* ag = agent_lookup(id);
    if (!ag) { pthread_mutex_unlock(&g_agent_lock); return -1; }
    cell_retain(st);
    cell_release(ag->state);
    ag->state = st;
    pthread_mutex_unlock(&g_agent_lock);
    return 0;
}

int agent_stop(int id) {
    pthread_mutex_lock(&g_agent_lock);
    AgentState* ag = agent_lookup(id);
    if (!ag) { pthread_mutex_unlock(&g_agent_lock); return -1; }
    cell_release(ag->state);
    ag->state = NULL;
    ag->active = false;
    g_agent_count--;
    pthread_mutex_unlock(&g_agent_lock);
    return 0;
}

void agent_reset_all(void) {
    pthread_mutex_lock(&g_agent_lock);
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
    pthread_mutex_unlock(&g_agent_lock);
}

/* ============ GenStage (Producer-Consumer Pipelines) ============ */

static GenStage g_stages[MAX_STAGES];
static int g_stage_count = 0;
static int g_next_stage_id = 0;

int stage_create(StageMode mode, Cell* handler, Cell* state) {
    pthread_mutex_lock(&g_stage_lock);
    if (g_stage_count >= MAX_STAGES) { pthread_mutex_unlock(&g_stage_lock); return -1; }
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
            int id = g_stages[i].id;
            pthread_mutex_unlock(&g_stage_lock);
            return id;
        }
    }
    pthread_mutex_unlock(&g_stage_lock);
    return -1;
}

GenStage* stage_lookup(int id) {
    pthread_mutex_lock(&g_stage_lock);
    for (int i = 0; i < MAX_STAGES; i++) {
        if (g_stages[i].active && g_stages[i].id == id) {
            GenStage* s = &g_stages[i];
            pthread_mutex_unlock(&g_stage_lock);
            return s;
        }
    }
    pthread_mutex_unlock(&g_stage_lock);
    return NULL;
}

int stage_subscribe(int consumer_id, int producer_id) {
    pthread_mutex_lock(&g_stage_lock);
    GenStage* producer = NULL;
    GenStage* consumer = NULL;
    for (int i = 0; i < MAX_STAGES; i++) {
        if (g_stages[i].active && g_stages[i].id == producer_id) producer = &g_stages[i];
        if (g_stages[i].active && g_stages[i].id == consumer_id) consumer = &g_stages[i];
    }
    if (!producer || !consumer) { pthread_mutex_unlock(&g_stage_lock); return -1; }
    if (producer->subscriber_count >= MAX_STAGE_SUBSCRIBERS) { pthread_mutex_unlock(&g_stage_lock); return -2; }
    producer->subscribers[producer->subscriber_count++] = consumer_id;
    pthread_mutex_unlock(&g_stage_lock);
    return 0;
}

int stage_stop(int id) {
    pthread_mutex_lock(&g_stage_lock);
    GenStage* s = NULL;
    for (int i = 0; i < MAX_STAGES; i++) {
        if (g_stages[i].active && g_stages[i].id == id) { s = &g_stages[i]; break; }
    }
    if (!s) { pthread_mutex_unlock(&g_stage_lock); return -1; }
    if (s->handler) cell_release(s->handler);
    if (s->state) cell_release(s->state);
    s->handler = NULL;
    s->state = NULL;
    s->active = false;
    g_stage_count--;
    pthread_mutex_unlock(&g_stage_lock);
    return 0;
}

void stage_reset_all(void) {
    pthread_mutex_lock(&g_stage_lock);
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
    pthread_mutex_unlock(&g_stage_lock);
}

/* ============ Flow (lazy computation pipelines) ============ */

static Flow g_flows[MAX_FLOWS];
static int g_flow_count = 0;
static int g_next_flow_id = 0;

int flow_create(Cell* source) {
    pthread_mutex_lock(&g_flow_lock);
    if (g_flow_count >= MAX_FLOWS) { pthread_mutex_unlock(&g_flow_lock); return -1; }
    for (int i = 0; i < MAX_FLOWS; i++) {
        if (!g_flows[i].active) {
            g_flows[i].id = g_next_flow_id++;
            g_flows[i].source = source;
            if (source) cell_retain(source);
            g_flows[i].step_count = 0;
            g_flows[i].active = true;
            g_flow_count++;
            int id = g_flows[i].id;
            pthread_mutex_unlock(&g_flow_lock);
            return id;
        }
    }
    pthread_mutex_unlock(&g_flow_lock);
    return -1;
}

Flow* flow_lookup(int id) {
    pthread_mutex_lock(&g_flow_lock);
    for (int i = 0; i < MAX_FLOWS; i++) {
        if (g_flows[i].active && g_flows[i].id == id) {
            Flow* f = &g_flows[i];
            pthread_mutex_unlock(&g_flow_lock);
            return f;
        }
    }
    pthread_mutex_unlock(&g_flow_lock);
    return NULL;
}

int flow_add_step(int id, FlowStepType type, Cell* fn, Cell* init) {
    pthread_mutex_lock(&g_flow_lock);
    Flow* f = NULL;
    for (int i = 0; i < MAX_FLOWS; i++) {
        if (g_flows[i].active && g_flows[i].id == id) { f = &g_flows[i]; break; }
    }
    if (!f) { pthread_mutex_unlock(&g_flow_lock); return -1; }
    if (f->step_count >= MAX_FLOW_STEPS) { pthread_mutex_unlock(&g_flow_lock); return -2; }
    int idx = f->step_count++;
    f->steps[idx].type = type;
    f->steps[idx].fn = fn;
    if (fn) cell_retain(fn);
    f->steps[idx].init = init;
    if (init) cell_retain(init);
    pthread_mutex_unlock(&g_flow_lock);
    return 0;
}

void flow_reset_all(void) {
    pthread_mutex_lock(&g_flow_lock);
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
    pthread_mutex_unlock(&g_flow_lock);
}

/* ============ Named Flow Registry ============ */

static char* g_flow_registry_names[MAX_FLOW_REGISTRY];
static int   g_flow_registry_ids[MAX_FLOW_REGISTRY];
static int   g_flow_registry_count = 0;

int flow_registry_register(const char* name, int flow_id) {
    pthread_mutex_lock(&g_flow_reg_lock);
    if (g_flow_registry_count >= MAX_FLOW_REGISTRY) { pthread_mutex_unlock(&g_flow_reg_lock); return -3; }

    Flow* f = flow_lookup(flow_id);
    if (!f) { pthread_mutex_unlock(&g_flow_reg_lock); return -4; }

    for (int i = 0; i < g_flow_registry_count; i++) {
        if (strcmp(g_flow_registry_names[i], name) == 0) { pthread_mutex_unlock(&g_flow_reg_lock); return -1; }
        if (g_flow_registry_ids[i] == flow_id) { pthread_mutex_unlock(&g_flow_reg_lock); return -2; }
    }

    g_flow_registry_names[g_flow_registry_count] = strdup(name);
    g_flow_registry_ids[g_flow_registry_count] = flow_id;
    g_flow_registry_count++;
    pthread_mutex_unlock(&g_flow_reg_lock);
    return 0;
}

int flow_registry_lookup(const char* name) {
    pthread_mutex_lock(&g_flow_reg_lock);
    for (int i = 0; i < g_flow_registry_count; i++) {
        if (strcmp(g_flow_registry_names[i], name) == 0) {
            int id = g_flow_registry_ids[i];
            pthread_mutex_unlock(&g_flow_reg_lock);
            return id;
        }
    }
    pthread_mutex_unlock(&g_flow_reg_lock);
    return -1;
}

int flow_registry_unregister_name(const char* name) {
    pthread_mutex_lock(&g_flow_reg_lock);
    for (int i = 0; i < g_flow_registry_count; i++) {
        if (strcmp(g_flow_registry_names[i], name) == 0) {
            free(g_flow_registry_names[i]);
            g_flow_registry_names[i] = g_flow_registry_names[--g_flow_registry_count];
            g_flow_registry_ids[i] = g_flow_registry_ids[g_flow_registry_count];
            pthread_mutex_unlock(&g_flow_reg_lock);
            return 0;
        }
    }
    pthread_mutex_unlock(&g_flow_reg_lock);
    return -1;
}

Cell* flow_registry_list(void) {
    pthread_mutex_lock(&g_flow_reg_lock);
    Cell* list = cell_nil();
    for (int i = g_flow_registry_count - 1; i >= 0; i--) {
        Cell* sym = cell_symbol(g_flow_registry_names[i]);
        Cell* new_list = cell_cons(sym, list);
        cell_release(sym);
        cell_release(list);
        list = new_list;
    }
    pthread_mutex_unlock(&g_flow_reg_lock);
    return list;
}

void flow_registry_reset(void) {
    pthread_mutex_lock(&g_flow_reg_lock);
    for (int i = 0; i < g_flow_registry_count; i++) {
        free(g_flow_registry_names[i]);
        g_flow_registry_names[i] = NULL;
    }
    g_flow_registry_count = 0;
    pthread_mutex_unlock(&g_flow_reg_lock);
}
