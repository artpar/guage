#ifndef GUAGE_ACTOR_H
#define GUAGE_ACTOR_H

#include <stdbool.h>
#include "cell.h"
#include "fiber.h"
#include "eval.h"

#define MAX_ACTORS 256
#define MAX_LINKS 32
#define MAX_MONITORS 32
#define MAX_SUPERVISORS 64
#define MAX_SUP_CHILDREN 32
#define SUP_MAX_RESTARTS 5
#define MAX_REGISTRY 256

/* Actor - a fiber with a mailbox */
typedef struct Actor {
    int id;                /* Unique actor ID */
    Fiber* fiber;          /* Underlying fiber */
    int mailbox_count;     /* Number of pending messages */
    bool alive;            /* false after actor finishes */
    Cell* result;          /* Final result when finished */

    /* Supervision */
    int links[MAX_LINKS];      /* Bidirectional linked actor IDs */
    int link_count;
    int monitors[MAX_MONITORS]; /* Actor IDs monitoring this actor */
    int monitor_count;
    bool trap_exit;             /* Convert exit signals to messages */
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

/* Supervision */
void   actor_link(Actor* a, Actor* b);
void   actor_unlink(Actor* a, Actor* b);
void   actor_add_monitor(Actor* target, Actor* watcher);
void   actor_exit_signal(Actor* target, Actor* sender, Cell* reason);
void   actor_notify_exit(Actor* exiting, Cell* reason);

/* Supervisor strategies */
typedef enum {
    SUP_ONE_FOR_ONE,
    SUP_ONE_FOR_ALL,
    SUP_REST_FOR_ONE
} SupervisorStrategy;

typedef struct Supervisor {
    int id;
    SupervisorStrategy strategy;
    Cell* child_specs[MAX_SUP_CHILDREN];   /* Behavior functions */
    int child_ids[MAX_SUP_CHILDREN];       /* Current child actor IDs */
    int child_count;
    int restart_count;
    EvalContext* ctx;                       /* For spawning */
} Supervisor;

Supervisor* supervisor_create(EvalContext* ctx, SupervisorStrategy strategy, Cell* specs);
int         supervisor_spawn_child(Supervisor* sup, int index);
void        supervisor_handle_exit(Supervisor* sup, int dead_id, Cell* reason);
Supervisor* supervisor_lookup(int id);
Supervisor* supervisor_find_for_child(int child_id);

/* Named Process Registry */
int  actor_registry_register(const char* name, int actor_id);   /* 0=ok, -1=dup name, -2=dup actor, -3=full, -4=dead */
int  actor_registry_lookup(const char* name);                    /* actor_id or -1 */
int  actor_registry_unregister_name(const char* name);           /* 0=ok, -1=not found */
void actor_registry_unregister_actor(int actor_id);              /* silent if not registered */
Cell* actor_registry_list(void);                                  /* list of name symbols */
void actor_registry_reset(void);

#endif /* GUAGE_ACTOR_H */
