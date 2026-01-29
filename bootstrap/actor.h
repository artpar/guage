#ifndef GUAGE_ACTOR_H
#define GUAGE_ACTOR_H

#include <stdbool.h>
#include "cell.h"
#include "fiber.h"
#include "eval.h"

#define MAX_ACTORS 256

/* Actor - a fiber with a mailbox */
typedef struct Actor {
    int id;                /* Unique actor ID */
    Fiber* fiber;          /* Underlying fiber */
    int mailbox_count;     /* Number of pending messages */
    bool alive;            /* false after actor finishes */
    Cell* result;          /* Final result when finished */
} Actor;

/* Lifecycle */
Actor* actor_create(EvalContext* ctx, Cell* behavior, Cell* env);
void   actor_send(Actor* actor, Cell* message);
Cell*  actor_receive(Actor* actor);
void   actor_destroy(Actor* actor);

/* Registry */
Actor* actor_lookup(int id);
int    actor_run_all(int max_ticks);
void   actor_reset_all(void);

/* Current actor tracking (for ←? to find its actor) */
Actor* actor_current(void);
void   actor_set_current(Actor* actor);

#endif /* GUAGE_ACTOR_H */
