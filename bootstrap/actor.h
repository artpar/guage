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
#define MAX_TIMERS 256
#define MAX_DICT_ENTRIES 256
#define MAX_ETS_TABLES 64
#define MAX_ETS_ENTRIES 256
#define MAX_APPLICATIONS 16
#define MAX_APP_ENV 64

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

    /* Process dictionary (per-actor key-value store) */
    Cell* dict_keys[MAX_DICT_ENTRIES];
    Cell* dict_values[MAX_DICT_ENTRIES];
    int dict_count;
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

/* Per-child restart types (for DynamicSupervisor) */
typedef enum {
    CHILD_PERMANENT,   /* Always restart on exit */
    CHILD_TRANSIENT,   /* Restart only on error exit */
    CHILD_TEMPORARY    /* Never restart */
} ChildRestartType;

typedef struct Supervisor {
    int id;
    SupervisorStrategy strategy;
    Cell* child_specs[MAX_SUP_CHILDREN];   /* Behavior functions */
    int child_ids[MAX_SUP_CHILDREN];       /* Current child actor IDs */
    ChildRestartType child_restart[MAX_SUP_CHILDREN]; /* Per-child restart type */
    int child_count;
    int restart_count;
    bool is_dynamic;                       /* true for DynamicSupervisor */
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

/* Timers */
typedef struct Timer {
    int id;
    int target_actor_id;
    int remaining_ticks;
    Cell* message;
    bool active;
} Timer;

int   timer_create(int target_actor_id, int ticks, Cell* message);  /* returns timer id */
int   timer_cancel(int timer_id);     /* 0=ok, -1=not found */
bool  timer_active(int timer_id);
bool  timer_tick_all(void);           /* called each scheduler tick, returns true if any fired */
bool  timer_any_pending(void);       /* true if any timers still counting down */
void  timer_reset_all(void);

/* ETS - Erlang Term Storage (shared named tables) */
typedef struct EtsTable {
    const char* name;                     /* Table name (symbol) */
    int owner_actor_id;                   /* -1 if created outside actor */
    Cell* keys[MAX_ETS_ENTRIES];
    Cell* values[MAX_ETS_ENTRIES];
    int count;
    bool active;
} EtsTable;

int   ets_create(const char* name, int owner_actor_id);  /* 0=ok, -1=dup, -2=full */
int   ets_insert(const char* name, Cell* key, Cell* value); /* 0=ok, -1=not found, -2=full */
Cell* ets_lookup(const char* name, Cell* key);            /* value or NULL */
int   ets_delete_key(const char* name, Cell* key);        /* 0=ok, -1=not found table, -2=key not found */
int   ets_delete_table(const char* name);                 /* 0=ok, -1=not found */
int   ets_size(const char* name);                         /* count or -1 */
Cell* ets_all(const char* name);                          /* list of ⟨key value⟩ or NULL */
void  ets_destroy_by_owner(int actor_id);                 /* destroy tables owned by actor */
void  ets_reset_all(void);

/* Application - OTP top-level container */
typedef struct Application {
    const char* name;                    /* Application name (symbol) */
    int supervisor_id;                   /* Top-level supervisor ID */
    Cell* stop_fn;                       /* Optional stop callback (or NULL) */
    Cell* env_keys[MAX_APP_ENV];
    Cell* env_vals[MAX_APP_ENV];
    int env_count;
    bool running;
} Application;

int   app_start(const char* name, int supervisor_id, Cell* stop_fn);  /* 0=ok, -1=dup, -2=full */
int   app_stop(const char* name);                                       /* 0=ok, -1=not found */
Application* app_lookup(const char* name);
Cell* app_which(void);                                                  /* list of name symbols */
Cell* app_get_env(const char* name, Cell* key);                        /* value or NULL */
int   app_set_env(const char* name, Cell* key, Cell* value);           /* 0=ok, -1=not found, -2=full */
void  app_reset_all(void);

/* Agent - functional state wrapper */
#define MAX_AGENTS 64

typedef struct AgentState {
    int id;
    Cell* state;
    bool active;
} AgentState;

int   agent_start(Cell* initial_state);   /* returns agent id, or -1 if full */
Cell* agent_get_state(int id);            /* state or NULL if not found */
int   agent_set_state(int id, Cell* st);  /* 0=ok, -1=not found */
int   agent_stop(int id);                 /* 0=ok, -1=not found */
void  agent_reset_all(void);

/* GenStage - producer-consumer pipelines */
#define MAX_STAGES 64
#define MAX_STAGE_SUBSCRIBERS 16

typedef enum {
    STAGE_PRODUCER,
    STAGE_CONSUMER,
    STAGE_PRODUCER_CONSUMER
} StageMode;

typedef struct GenStage {
    int id;
    StageMode mode;
    Cell* handler;              /* callback function */
    Cell* state;                /* current state */
    int subscribers[MAX_STAGE_SUBSCRIBERS];  /* downstream stage IDs */
    int subscriber_count;
    bool active;
} GenStage;

int        stage_create(StageMode mode, Cell* handler, Cell* state);  /* returns id or -1 */
GenStage*  stage_lookup(int id);
int        stage_subscribe(int consumer_id, int producer_id);  /* 0=ok, -1=not found, -2=full */
int        stage_stop(int id);           /* 0=ok, -1=not found */
void       stage_reset_all(void);

#endif /* GUAGE_ACTOR_H */
