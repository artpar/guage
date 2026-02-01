#ifndef GUAGE_SCHEDULER_H
#define GUAGE_SCHEDULER_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include "cell.h"
#include "eval.h"
#include "park.h"
#include "eventcount.h"

/* Cache line size — 128B for Apple Silicon SoC safety, also safe on x86 (64B) */
#define CACHE_LINE 128

/* BEAM-style reduction quantum — reductions per actor before yield */
#define CONTEXT_REDS 4000

/* Maximum schedulers (one per core) */
#define MAX_SCHEDULERS 16

/* BWoS tuning — NVIDIA stdexec production defaults */
#define BWOS_NE        32    /* Entries per block */
#define BWOS_B         8     /* Blocks in ring (power of 2) */
#define BWOS_B_MASK    (BWOS_B - 1)
#define BWOS_SENTINEL  BWOS_NE  /* steal_tail sentinel: "not stealable" */

/* Stack pool max per scheduler */
#define STACK_POOL_MAX 64

/* ── QSBR (Quiescent-State-Based Reclamation) ──
 * PPoPP 2024 amortized-free pattern. Zero read-path overhead.
 * DPDK-style design adapted for actor scheduling. */
#define RETIRE_CAP 256   /* Power of 2 — per-scheduler retire ring */
#define RETIRE_MASK (RETIRE_CAP - 1)

/* Global overflow queue capacity (Vyukov MPMC) */
#define GLOBAL_QUEUE_CAP 1024

/* Yield sentinel — returned by eval when reduction count expires */
extern Cell* CELL_YIELD_SENTINEL;

/* ── Thread-local scheduler identity ── */
extern _Thread_local uint16_t tls_scheduler_id;

/* Forward declarations */
typedef struct Scheduler Scheduler;
typedef struct Actor Actor;
struct TraceEvent_s;  /* Forward declaration — defined below */

/* ── BWoS Work-Stealing Deque (OSDI 2023, NVIDIA stdexec-derived) ──
 * 4-cursor block design: owner and thieves operate on DIFFERENT blocks.
 * Zero seq_cst on owner fast path. ~15x better throughput under steal pressure. */
typedef struct {
    _Alignas(CACHE_LINE) _Atomic uint64_t head;        /* Owner read position */
    _Alignas(CACHE_LINE) _Atomic uint64_t tail;        /* Owner write position */
    _Alignas(CACHE_LINE) _Atomic uint64_t steal_tail;  /* Thief reservation cursor (CAS) */
    _Alignas(CACHE_LINE) _Atomic uint64_t steal_head;  /* Thief commit counter (FAA) */
    Actor* entries[BWOS_NE];
} BWoSBlock;

typedef struct {
    _Alignas(CACHE_LINE) _Atomic uint64_t owner_block;  /* Monotonic epoch (& MASK for index) */
    _Alignas(CACHE_LINE) _Atomic uint64_t thief_block;  /* Monotonic epoch (& MASK for index) */
    BWoSBlock blocks[BWOS_B];
} WSDeque;  /* Name kept for API compat */

void    ws_init(WSDeque* dq);
void    ws_destroy(WSDeque* dq);
void    ws_push(WSDeque* dq, Actor* actor);
Actor*  ws_pop(WSDeque* dq);
Actor*  ws_steal(WSDeque* dq);
int64_t ws_size(WSDeque* dq);

/* ── QSBR per-thread state (cache-line aligned to prevent false sharing) ── */
typedef struct {
    _Alignas(CACHE_LINE) _Atomic uint64_t epoch;   /* Last observed global epoch */
    _Atomic bool online;                             /* false when parked */
} QsbrThread;

typedef struct {
    _Alignas(CACHE_LINE) _Atomic uint64_t global_epoch;
    QsbrThread threads[MAX_SCHEDULERS];
    int thread_count;
} QsbrState;

/* Per-scheduler retire ring with amortized free (PPoPP 2024 pattern) */
typedef struct {
    Actor*   actors[RETIRE_CAP];
    uint64_t epochs[RETIRE_CAP];   /* Global epoch at retire time */
    uint32_t head;                  /* Oldest pending (consumer) */
    uint32_t tail;                  /* Next write slot (producer) */
} RetireRing;

/* ── Scheduler (128B-aligned, no false sharing) ── */
struct Scheduler {
    /* ── Hot: touched every scheduling decision ── */
    int id;
    _Atomic bool running;
    _Atomic bool should_stop;

    WSDeque deque;                  /* Chase-Lev work-stealing deque */

    /* LIFO slot — hottest path, cache-warm, NOT stealable.
     * Tokio/Go runnext pattern. Starvation guard after 3 consecutive uses. */
    _Atomic(Actor*) runnext;
    uint8_t runnext_consecutive;

    /* Thread identity */
    pthread_t thread;
    EvalContext eval_ctx;

    /* RNG for steal victim selection */
    uint32_t rng;

    /* ── Cold: statistics (own cache line) ── */
    _Alignas(CACHE_LINE) uint64_t stat_reductions;
    uint64_t stat_context_switches;
    uint64_t stat_steals;
    uint64_t stat_actors_run;

    /* ── Park/wake — eventcount-based (own cache line) ── */
    _Alignas(CACHE_LINE) _Atomic bool parked;  /* true when committed to eventcount sleep */

    /* ── Stack pool (own cache line) ── */
    _Alignas(CACHE_LINE) char* stack_pool[STACK_POOL_MAX];
    int stack_pool_count;

    /* ── QSBR retire ring (own cache line) ── */
    _Alignas(CACHE_LINE) RetireRing retire_ring;

    /* ── Trace buffer pointers (for cross-scheduler merge) ── */
    struct TraceEvent_s* trace_buf_ptr;  /* Points to worker's tls_trace_buf */
    uint32_t*   trace_pos_ptr;           /* Points to worker's tls_trace_pos */
};

/* ── Scheduler API ── */

/* Initialize scheduler subsystem with N schedulers */
void sched_init(int num_schedulers);

/* Shutdown all schedulers */
void sched_shutdown(void);

/* Deterministic scheduling mode for reproducible tests */
void sched_set_deterministic(int enabled, uint32_t seed);
uint32_t sched_hash_seed(const char* str);

/* Enqueue actor onto scheduler's local deque (LIFO slot first) */
void sched_enqueue(Scheduler* sched, Actor* actor);

/* Enqueue a newly-created actor (no-op in single-scheduler mode) */
void sched_enqueue_new_actor(Actor* actor);

/* Try to steal actor(s) from another scheduler (steal-half policy) */
Actor* sched_try_steal(Scheduler* thief);


/* Run all actors (single-scheduler delegates to actor_run_all;
 * multi-scheduler spawns worker threads) */
int sched_run_all(int max_ticks);

/* Get scheduler by ID */
Scheduler* sched_get(int id);

/* Get number of active schedulers */
int sched_count(void);

/* Set number of schedulers (for runtime adjustment) */
void sched_set_count(int n);

/* ── Stack pool (mmap + guard page + pre-fault) ── */
char* sched_stack_alloc(Scheduler* s, size_t stack_size);
void  sched_stack_free(Scheduler* s, char* stack, size_t stack_size);

/* ── Global overflow queue (Vyukov MPMC) ── */
bool   global_queue_push(Actor* actor);
Actor* global_queue_pop(void);
int    global_queue_size(void);

/* ── Scheduler-driven actor execution ── */

/* Run one actor for one quantum (CONTEXT_REDS reductions).
 * Sets up thread-local context, runs fiber, handles yield/finish.
 * Returns: 1=alive (did work), 0=dead, -1=alive but blocked (no work done). */
int sched_run_one_quantum(Scheduler* sched, Actor* actor);

/* Check if an actor's suspension condition is met so it can be resumed.
 * Returns true if the actor can run, false if still blocked. */
bool sched_actor_runnable(Actor* actor);

/* Prepare resume value for a suspended actor whose condition is met */
Cell* sched_prepare_resume(Actor* actor);

/* ── Alive actor counter (atomic, for termination detection) ── */
extern _Atomic int g_alive_actors;

/* ── Eventcount-based parking (HTF-grade scheduler) ── */
extern EventCount g_sched_ec;               /* Global eventcount (all workers share) */
extern _Atomic uint32_t g_num_searching;    /* Workers actively scanning for work */

/* ── QSBR API ── */

/* Initialize QSBR subsystem — called from sched_init() */
void qsbr_init(int num_schedulers);

/* Thread lifecycle — called from worker_main */
void qsbr_thread_online(int sched_id);
void qsbr_thread_offline(int sched_id);

/* Retire an actor — add to per-scheduler ring (call instead of immediate destroy) */
void qsbr_retire(Scheduler* s, Actor* actor);

/* Amortized reclaim — free at most 2 actors per call (PPoPP 2024 pattern) */
void qsbr_reclaim_amortized(Scheduler* s);

/* Drain all retire rings — called at end of sched_run_all after workers joined */
void qsbr_drain_all(void);

/* Global QSBR state (defined in scheduler.c) */
extern QsbrState g_qsbr;

/* Quiescent checkpoint — THE hot path, ~10ns.
 * Single relaxed load + release store. No barrier on x86 TSO. */
static inline void qsbr_quiescent(int sched_id) {
    uint64_t ge = atomic_load_explicit(&g_qsbr.global_epoch,
                                        memory_order_relaxed);
    atomic_store_explicit(&g_qsbr.threads[sched_id].epoch,
                          ge, memory_order_release);
}

/* Advance global epoch — called by scheduler 0 periodically */
static inline void qsbr_advance_epoch(void) {
    atomic_fetch_add_explicit(&g_qsbr.global_epoch, 1,
                              memory_order_release);
}

/* Grace period check — is it safe to free actors retired at retire_epoch? */
static inline bool qsbr_safe(uint64_t retire_epoch) {
    for (int i = 0; i < g_qsbr.thread_count; i++) {
        if (!atomic_load_explicit(&g_qsbr.threads[i].online,
                                   memory_order_acquire))
            continue;  /* Parked — doesn't block */
        if (atomic_load_explicit(&g_qsbr.threads[i].epoch,
                                  memory_order_acquire) <= retire_epoch)
            return false;  /* This thread hasn't passed quiescent state */
    }
    return true;
}

/* ── Trace Event Ring Buffer (Day 135) ──
 * Thread-local, zero-synchronization trace events. */

typedef enum {
    TRACE_SPAWN,        /* Actor spawned — detail: parent actor ID (0 if none) */
    TRACE_SEND,         /* Message sent — detail: target actor ID */
    TRACE_RECV,         /* Message received — detail: 0 */
    TRACE_DIE,          /* Actor exited — detail: 0=normal, 1=error */
    TRACE_STEAL,        /* Work stolen — detail: victim scheduler ID */
    TRACE_YIELD,        /* Reduction yield — detail: CONTEXT_REDS */
    TRACE_WAKE,         /* Actor woken — detail: 0 */
    TRACE_RESUME,       /* Actor resumed — detail: SuspendReason enum */
    TRACE_LINK,         /* Link created — detail: other actor ID */
    TRACE_MONITOR,      /* Monitor added — detail: watcher actor ID */
    TRACE_EXIT_SIGNAL,  /* Exit signal — detail: sender actor ID (0 if none) */
    TRACE_TIMER_FIRE,   /* Timer fired — detail: 0 */
    TRACE_CHAN_SEND,    /* Channel send — detail: channel ID */
    TRACE_CHAN_RECV,    /* Channel recv — detail: channel ID */
    TRACE_CHAN_CLOSE,   /* Channel closed — detail: channel ID */
} TraceEventKind;

typedef struct TraceEvent_s {
    uint64_t timestamp;     /* rdtscp/cntvct for ns resolution */
    uint16_t scheduler_id;
    uint16_t actor_id;
    uint8_t  kind;          /* TraceEventKind */
    uint8_t  _pad;
    uint16_t detail;        /* Kind-specific detail field */
} TraceEvent;  /* Exactly 16 bytes — 4 events per x86 cache line */

#define TRACE_BUF_CAP 4096

extern _Thread_local TraceEvent tls_trace_buf[TRACE_BUF_CAP];
extern _Thread_local uint32_t tls_trace_pos;

/* Global trace toggle — default off, ~0.3ns when disabled (predicted branch) */
extern _Atomic bool g_trace_enabled;

/* TSC-to-nanosecond calibration (TSCNS pattern) */
typedef struct {
    uint64_t tsc_base;
    uint64_t ns_base;
    double   tsc_to_ns;     /* multiplier: nanos = (tsc - tsc_base) * tsc_to_ns + ns_base */
} TscCalibration;

extern TscCalibration g_tsc_cal;

/* Calibrate TSC against wall clock — call once from sched_init */
void tsc_calibrate(void);

/* Convert raw TSC to nanoseconds (read-path only, cold) */
uint64_t tsc_to_nanos(uint64_t tsc);

/* Record a trace event (inlined, ~4-7ns overhead when enabled)
 * Uses ISB+cntvct on ARM64, rdtscp on x86-64 for serialized timestamps */
static inline void trace_record(TraceEventKind kind, uint16_t actor_id, uint16_t detail) {
    if (__builtin_expect(!atomic_load_explicit(&g_trace_enabled, memory_order_relaxed), 1)) return;
#if defined(__aarch64__)
    uint64_t ts;
    __asm__ volatile("isb\nmrs %0, cntvct_el0" : "=r"(ts));  /* ISB serializes */
#elif defined(__x86_64__)
    uint32_t lo, hi;
    __asm__ volatile("rdtscp" : "=a"(lo), "=d"(hi) : : "ecx");  /* ecx clobbered */
    uint64_t ts = ((uint64_t)hi << 32) | lo;
#else
    uint64_t ts = 0;
#endif
    uint32_t idx = tls_trace_pos & (TRACE_BUF_CAP - 1);
    tls_trace_buf[idx].timestamp = ts;
    tls_trace_buf[idx].scheduler_id = tls_scheduler_id;
    tls_trace_buf[idx].actor_id = actor_id;
    tls_trace_buf[idx].kind = (uint8_t)kind;
    tls_trace_buf[idx].detail = detail;
    tls_trace_pos++;
}

/* ── Global trace merge API ── */

/* K-way merge by timestamp across all scheduler trace buffers.
 * Only safe when workers are parked/joined. Returns event count. */
int sched_trace_merge(TraceEvent* out, int out_cap, int filter_kind);

#endif /* GUAGE_SCHEDULER_H */
