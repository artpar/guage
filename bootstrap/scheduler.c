#include "scheduler.h"
#include "actor.h"
#include "channel.h"
#include "log.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <sched.h>

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

/* ── Thread-local scheduler ID ──
 * Defined in cell.c (shared with BRC).
 * extern _Thread_local uint16_t tls_scheduler_id;
 */

/* Yield sentinel — singleton, never freed */
static Cell _yield_sentinel_storage;
Cell* CELL_YIELD_SENTINEL = &_yield_sentinel_storage;

/* ── Alive actor counter (atomic, for termination detection) ── */
_Atomic int g_alive_actors = 0;

/* ── Currently-executing actor counter (for idle detection) ── */
static _Atomic int g_running_actors = 0;

/* ── Eventcount-based parking (HTF-grade scheduler) ── */
EventCount g_sched_ec;
_Atomic uint32_t g_num_searching = 0;

/* ── Trace event ring buffer (Day 135) ── */
_Thread_local TraceEvent tls_trace_buf[TRACE_BUF_CAP];
_Thread_local uint32_t tls_trace_pos = 0;

/* ── Global trace toggle (Day 136) ── */
_Atomic bool g_trace_enabled = false;

/* ── TSC calibration (Day 136) ── */
TscCalibration g_tsc_cal = {0, 0, 1.0};

void tsc_calibrate(void) {
    struct timespec ts1, ts2;
    uint64_t tsc1, tsc2;

    clock_gettime(CLOCK_MONOTONIC, &ts1);
#if defined(__aarch64__)
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(tsc1));
#elif defined(__x86_64__)
    { uint32_t lo, hi; __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi)); tsc1 = ((uint64_t)hi << 32) | lo; }
#else
    tsc1 = 0;
#endif

    /* Spin ~10ms for calibration accuracy */
    volatile int spin = 0;
    for (int i = 0; i < 1000000; i++) spin++;

    clock_gettime(CLOCK_MONOTONIC, &ts2);
#if defined(__aarch64__)
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(tsc2));
#elif defined(__x86_64__)
    { uint32_t lo, hi; __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi)); tsc2 = ((uint64_t)hi << 32) | lo; }
#else
    tsc2 = 0;
#endif

    uint64_t ns1 = (uint64_t)ts1.tv_sec * 1000000000ULL + (uint64_t)ts1.tv_nsec;
    uint64_t ns2 = (uint64_t)ts2.tv_sec * 1000000000ULL + (uint64_t)ts2.tv_nsec;
    uint64_t delta_ns = ns2 - ns1;
    uint64_t delta_tsc = tsc2 - tsc1;

    g_tsc_cal.tsc_base = tsc1;
    g_tsc_cal.ns_base = ns1;
    g_tsc_cal.tsc_to_ns = (delta_tsc > 0) ? (double)delta_ns / (double)delta_tsc : 1.0;
}

uint64_t tsc_to_nanos(uint64_t tsc) {
    return (uint64_t)((double)(tsc - g_tsc_cal.tsc_base) * g_tsc_cal.tsc_to_ns) + g_tsc_cal.ns_base;
}

/* ── Global scheduler state ── */
static Scheduler g_schedulers[MAX_SCHEDULERS] __attribute__((aligned(CACHE_LINE)));
static int g_num_schedulers = 1;
static _Atomic bool g_sched_initialized = false;
static EvalContext* g_shared_eval_ctx = NULL;
static size_t g_page_size = 0;

/* ── QSBR global state (Day 140) ── */
QsbrState g_qsbr;

/* ── BWoS Work-Stealing Deque (OSDI 2023, NVIDIA stdexec-derived) ──
 * 4-cursor block design. Owner and thieves operate on DIFFERENT blocks.
 * Zero seq_cst on owner fast path. Two-phase steal: reserve (CAS) + commit (FAA). */

void ws_init(WSDeque* dq) {
    memset(dq, 0, sizeof(WSDeque));
    /* All blocks start as not-stealable (sentinel) */
    for (int i = 0; i < BWOS_B; i++) {
        atomic_init(&dq->blocks[i].head, 0);
        atomic_init(&dq->blocks[i].tail, 0);
        atomic_init(&dq->blocks[i].steal_tail, (uint64_t)BWOS_SENTINEL);
        atomic_init(&dq->blocks[i].steal_head, 0);
    }
    /* Owner starts at block 1, thief_block at 0.
     * Block 0 is the initial "thief block" (empty, no items to steal).
     * Block 1 is where owner pushes. */
    atomic_init(&dq->owner_block, 1);
    atomic_init(&dq->thief_block, 0);
}

void ws_destroy(WSDeque* dq) {
    /* No-op: all storage is inline */
    (void)dq;
}

void ws_push(WSDeque* dq, Actor* actor) {
    for (;;) {
        uint64_t ob = atomic_load_explicit(&dq->owner_block, memory_order_relaxed);
        BWoSBlock* blk = &dq->blocks[ob & BWOS_B_MASK];
        uint64_t t = atomic_load_explicit(&blk->tail, memory_order_relaxed);

        if (t < BWOS_NE) {
            /* Fast path: space in current block — plain store + release publish */
            blk->entries[t] = actor;
            atomic_store_explicit(&blk->tail, t + 1, memory_order_release);
            return;
        }

        /* Block full → advance to next block */
        uint64_t next_epoch = ob + 1;
        uint64_t tb = atomic_load_explicit(&dq->thief_block, memory_order_acquire);

        if (next_epoch - tb >= BWOS_B) {
            /* All blocks in use — overflow to global queue */
            global_queue_push(actor);
            return;
        }

        BWoSBlock* next_blk = &dq->blocks[next_epoch & BWOS_B_MASK];

        /* GRANT current block to thieves:
         * Set steal_tail = current head to expose remaining items for stealing.
         * head may be >0 if a fallback thief already stole from this block
         * while it was still owner-active (via CAS on head). */
        uint64_t old_tail = atomic_load_explicit(&blk->tail, memory_order_relaxed);
        uint64_t cur_head = atomic_load_explicit(&blk->head, memory_order_relaxed);
        atomic_store_explicit(&blk->head, old_tail, memory_order_relaxed);  /* close owner side */
        atomic_store_explicit(&blk->steal_tail, cur_head, memory_order_release);  /* open for thieves from cur_head */

        /* RECLAIM next block: spin until thieves finish with it.
         * steal_head must equal the number of items that were exposed.
         * For a freshly-sentinel block, steal_head is already 0 and
         * we expect 0 steals (SENTINEL was set, so no items exposed). */
        uint64_t expected_steals = 0;
        uint64_t st = atomic_load_explicit(&next_blk->steal_tail, memory_order_acquire);
        if (st != BWOS_SENTINEL) {
            /* This block was granted before — thieves may be active.
             * Expected completions = the old tail value that was set when granted. */
            expected_steals = atomic_load_explicit(&next_blk->tail, memory_order_relaxed);
            /* Spin until all steals complete */
            while (atomic_load_explicit(&next_blk->steal_head, memory_order_acquire) < expected_steals) {
                #if defined(__aarch64__)
                __builtin_arm_yield();
                #elif defined(__x86_64__)
                __asm__ volatile("pause");
                #endif
            }
        }

        /* Reset next block for owner use */
        atomic_store_explicit(&next_blk->head, 0, memory_order_relaxed);
        atomic_store_explicit(&next_blk->tail, 0, memory_order_relaxed);
        atomic_store_explicit(&next_blk->steal_head, 0, memory_order_relaxed);
        atomic_store_explicit(&next_blk->steal_tail, (uint64_t)BWOS_SENTINEL, memory_order_relaxed);

        atomic_store_explicit(&dq->owner_block, next_epoch, memory_order_relaxed);
        /* Loop back to push into fresh block */
    }
}

Actor* ws_pop(WSDeque* dq) {
    for (;;) {
        uint64_t ob = atomic_load_explicit(&dq->owner_block, memory_order_relaxed);
        BWoSBlock* blk = &dq->blocks[ob & BWOS_B_MASK];
        uint64_t t = atomic_load_explicit(&blk->tail, memory_order_relaxed);
        uint64_t h = atomic_load_explicit(&blk->head, memory_order_acquire);

        if (t > h + 1) {
            /* Multiple items: no contention with thief (different entries) */
            Actor* actor = blk->entries[t - 1];
            atomic_store_explicit(&blk->tail, t - 1, memory_order_release);
            return actor;
        }
        if (t == h + 1) {
            /* Last item: both owner and thief want entry[h].
             * Resolution: both use CAS on head. Winner gets the item.
             * Owner first sets tail=h (makes block look empty to late thieves). */
            atomic_store_explicit(&blk->tail, h, memory_order_seq_cst);
            uint64_t expected = h;
            bool won = atomic_compare_exchange_strong_explicit(&blk->head, &expected, h + 1,
                memory_order_seq_cst, memory_order_seq_cst);
            if (won) {
                Actor* actor = blk->entries[h];
                /* Reset block after claiming entry */
                atomic_store_explicit(&blk->head, 0, memory_order_release);
                atomic_store_explicit(&blk->tail, 0, memory_order_release);
                return actor;
            }
            /* Thief won — reset block */
            atomic_store_explicit(&blk->head, 0, memory_order_release);
            atomic_store_explicit(&blk->tail, 0, memory_order_release);
            /* Fall through to retreat */
        }

        /* Current block empty from owner side → retreat to previous block */
        uint64_t tb = atomic_load_explicit(&dq->thief_block, memory_order_acquire);
        if (ob <= tb) {
            /* No previous blocks available — deque is empty */
            return NULL;
        }

        uint64_t prev_epoch = ob - 1;
        BWoSBlock* prev_blk = &dq->blocks[prev_epoch & BWOS_B_MASK];

        /* TAKEOVER: atomically reclaim the previous block from thieves.
         * Exchange steal_tail to SENTINEL — returns where thieves were up to. */
        uint64_t steal_pos = atomic_exchange_explicit(&prev_blk->steal_tail,
            (uint64_t)BWOS_SENTINEL, memory_order_acquire);

        if (steal_pos == BWOS_SENTINEL) {
            /* Block wasn't granted or already reclaimed — try further back */
            /* But we also need to update owner_block to retreat */
            atomic_store_explicit(&dq->owner_block, prev_epoch, memory_order_relaxed);
            continue;
        }

        uint64_t prev_tail = atomic_load_explicit(&prev_blk->tail, memory_order_relaxed);
        if (steal_pos < prev_tail) {
            /* Items remain: thieves reserved [0..steal_pos), owner gets [steal_pos..tail) */
            /* Wait for in-flight steals to complete before we touch the block */
            while (atomic_load_explicit(&prev_blk->steal_head, memory_order_acquire) < steal_pos) {
                #if defined(__aarch64__)
                __builtin_arm_yield();
                #elif defined(__x86_64__)
                __asm__ volatile("pause");
                #endif
            }
            atomic_store_explicit(&prev_blk->head, steal_pos, memory_order_relaxed);
            atomic_store_explicit(&prev_blk->steal_head, 0, memory_order_relaxed);
            atomic_store_explicit(&dq->owner_block, prev_epoch, memory_order_relaxed);
            continue;  /* Retry pop from this block */
        }

        /* Block fully stolen — try further back */
        /* Wait for steals to finish */
        while (atomic_load_explicit(&prev_blk->steal_head, memory_order_acquire) < steal_pos) {
            #if defined(__aarch64__)
            __builtin_arm_yield();
            #elif defined(__x86_64__)
            __asm__ volatile("pause");
            #endif
        }
        atomic_store_explicit(&prev_blk->steal_head, 0, memory_order_relaxed);
        atomic_store_explicit(&dq->owner_block, prev_epoch, memory_order_relaxed);

        /* Advance thief_block past this fully-drained block */
        uint64_t old_tb = tb;
        while (old_tb <= prev_epoch) {
            if (!atomic_compare_exchange_weak_explicit(&dq->thief_block, &old_tb, prev_epoch + 1,
                    memory_order_release, memory_order_relaxed))
                break;  /* Someone else advanced it */
            break;
        }

        continue;
    }
}

Actor* ws_steal(WSDeque* dq) {
    uint64_t tb = atomic_load_explicit(&dq->thief_block, memory_order_acquire);
    uint64_t ob = atomic_load_explicit(&dq->owner_block, memory_order_relaxed);

    /* Scan blocks from thief_block up to (but not including) owner_block.
     * Owner's current block has steal_tail == SENTINEL, so we'd skip it anyway. */
    for (uint64_t epoch = tb; epoch < ob; epoch++) {
        BWoSBlock* blk = &dq->blocks[epoch & BWOS_B_MASK];
        uint64_t st = atomic_load_explicit(&blk->steal_tail, memory_order_acquire);

        if (st == BWOS_SENTINEL) continue;  /* Owner-active or not granted */

        uint64_t blk_tail = atomic_load_explicit(&blk->tail, memory_order_relaxed);
        if (st >= blk_tail) continue;  /* Fully reserved */

        /* Try to reserve one entry: CAS steal_tail from st to st+1 */
        if (atomic_compare_exchange_strong_explicit(&blk->steal_tail, &st, st + 1,
                memory_order_acq_rel, memory_order_relaxed)) {
            /* Reserved slot st — read the entry */
            Actor* actor = blk->entries[st];
            /* Commit: tell owner this steal is complete */
            atomic_fetch_add_explicit(&blk->steal_head, 1, memory_order_release);

            /* If this block is now fully drained, try to advance thief_block */
            if (st + 1 >= blk_tail) {
                uint64_t old_tb = tb;
                atomic_compare_exchange_weak_explicit(&dq->thief_block, &old_tb, epoch + 1,
                    memory_order_release, memory_order_relaxed);
            }

            return actor;
        }
        /* CAS failed — another thief got there. Try same block again with refreshed st. */
        /* Actually just move on — the for loop will increment epoch.
         * For high contention we could retry, but scanning forward is simpler. */
    }

    /* Fallback: steal from the owner's active block (steal_tail == SENTINEL).
     * Owner pops LIFO (tail--), thief steals FIFO (head++ via CAS).
     * Safe when t - h > 1: owner and thief touch different entries.
     * When t - h == 1: thief attempts CAS; owner's pop uses seq_cst to detect.
     *
     * CRITICAL: After CAS succeeds, re-check steal_tail. If the owner
     * granted this block between our SENTINEL check and the CAS, the block
     * is now in granted mode. We must account for our steal via steal_head FAA
     * so the reclaim spin-wait doesn't deadlock. */
    {
        BWoSBlock* oblk = &dq->blocks[ob & BWOS_B_MASK];
        uint64_t st = atomic_load_explicit(&oblk->steal_tail, memory_order_acquire);
        if (st != BWOS_SENTINEL) goto done;  /* Granted block, handled above */

        uint64_t h = atomic_load_explicit(&oblk->head, memory_order_acquire);
        uint64_t t = atomic_load_explicit(&oblk->tail, memory_order_acquire);

        if (h >= t) goto done;  /* Empty */

        /* CAS head upward to claim the bottom item */
        if (atomic_compare_exchange_strong_explicit(&oblk->head, &h, h + 1,
                memory_order_seq_cst, memory_order_relaxed)) {
            Actor* stolen = oblk->entries[h];

            /* Check if block was granted while we were stealing.
             * If steal_tail transitioned from SENTINEL to a real value,
             * the owner granted this block and is tracking steals via
             * steal_head. We must commit our steal so reclaim doesn't hang. */
            uint64_t st2 = atomic_load_explicit(&oblk->steal_tail, memory_order_acquire);
            if (st2 != BWOS_SENTINEL) {
                /* Block was granted — commit via FAA on steal_head */
                atomic_fetch_add_explicit(&oblk->steal_head, 1, memory_order_release);
            }

            return stolen;
        }
    }
done:
    return NULL;
}

int64_t ws_size(WSDeque* dq) {
    int64_t total = 0;
    uint64_t ob = atomic_load_explicit(&dq->owner_block, memory_order_relaxed);
    uint64_t tb = atomic_load_explicit(&dq->thief_block, memory_order_relaxed);

    /* Owner's current block */
    BWoSBlock* owner_blk = &dq->blocks[ob & BWOS_B_MASK];
    uint64_t ot = atomic_load_explicit(&owner_blk->tail, memory_order_relaxed);
    uint64_t oh = atomic_load_explicit(&owner_blk->head, memory_order_relaxed);
    total += (int64_t)(ot - oh);

    /* Granted blocks (available for stealing) */
    for (uint64_t epoch = tb; epoch < ob; epoch++) {
        BWoSBlock* blk = &dq->blocks[epoch & BWOS_B_MASK];
        uint64_t st = atomic_load_explicit(&blk->steal_tail, memory_order_relaxed);
        uint64_t sh = atomic_load_explicit(&blk->steal_head, memory_order_relaxed);
        uint64_t bt = atomic_load_explicit(&blk->tail, memory_order_relaxed);
        if (st != BWOS_SENTINEL && st < bt) {
            total += (int64_t)(bt - st);  /* Unreserved items */
        }
        (void)sh;  /* steal_head tracks completed steals, not remaining */
    }

    return total > 0 ? total : 0;
}


/* ── XorShift32 RNG for steal victim selection ── */
static uint32_t xorshift32(uint32_t* state) {
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

/* ── Global Overflow Queue (Vyukov bounded MPMC) ── */

typedef struct {
    _Atomic uint64_t seq;
    Actor* data;
} GlobalSlot;

static GlobalSlot g_global_queue[GLOBAL_QUEUE_CAP] __attribute__((aligned(CACHE_LINE)));
static _Atomic uint64_t g_gq_head __attribute__((aligned(CACHE_LINE)));
static _Atomic uint64_t g_gq_tail __attribute__((aligned(CACHE_LINE)));

static void global_queue_init(void) {
    atomic_init(&g_gq_head, 0);
    atomic_init(&g_gq_tail, 0);
    for (int i = 0; i < GLOBAL_QUEUE_CAP; i++) {
        atomic_init(&g_global_queue[i].seq, (uint64_t)i);
        g_global_queue[i].data = NULL;
    }
}

bool global_queue_push(Actor* actor) {
    uint64_t pos = atomic_load_explicit(&g_gq_head, memory_order_relaxed);
    for (;;) {
        GlobalSlot* slot = &g_global_queue[pos & (GLOBAL_QUEUE_CAP - 1)];
        uint64_t seq = atomic_load_explicit(&slot->seq, memory_order_acquire);
        int64_t diff = (int64_t)seq - (int64_t)pos;
        if (diff == 0) {
            if (atomic_compare_exchange_weak_explicit(&g_gq_head, &pos, pos + 1,
                    memory_order_relaxed, memory_order_relaxed)) {
                slot->data = actor;
                atomic_store_explicit(&slot->seq, pos + 1, memory_order_release);
                return true;
            }
        } else if (diff < 0) {
            return false; /* Full */
        } else {
            pos = atomic_load_explicit(&g_gq_head, memory_order_relaxed);
        }
    }
}

Actor* global_queue_pop(void) {
    uint64_t pos = atomic_load_explicit(&g_gq_tail, memory_order_relaxed);
    for (;;) {
        GlobalSlot* slot = &g_global_queue[pos & (GLOBAL_QUEUE_CAP - 1)];
        uint64_t seq = atomic_load_explicit(&slot->seq, memory_order_acquire);
        int64_t diff = (int64_t)seq - (int64_t)(pos + 1);
        if (diff == 0) {
            if (atomic_compare_exchange_weak_explicit(&g_gq_tail, &pos, pos + 1,
                    memory_order_relaxed, memory_order_relaxed)) {
                Actor* actor = slot->data;
                slot->data = NULL;
                atomic_store_explicit(&slot->seq, pos + GLOBAL_QUEUE_CAP, memory_order_release);
                return actor;
            }
        } else if (diff < 0) {
            return NULL; /* Empty */
        } else {
            pos = atomic_load_explicit(&g_gq_tail, memory_order_relaxed);
        }
    }
}

int global_queue_size(void) {
    uint64_t h = atomic_load_explicit(&g_gq_head, memory_order_relaxed);
    uint64_t t = atomic_load_explicit(&g_gq_tail, memory_order_relaxed);
    return (int)(h - t);
}

/* ── Stack Pool (mmap + guard page + pre-fault) ── */

char* sched_stack_alloc(Scheduler* s, size_t stack_size) {
    if (!g_page_size) g_page_size = (size_t)sysconf(_SC_PAGESIZE);

    /* Try pool first */
    if (s && s->stack_pool_count > 0) {
        return s->stack_pool[--s->stack_pool_count];
    }

    /* mmap with guard page at bottom (stack grows down) */
    size_t total = stack_size + g_page_size;
    void* base = mmap(NULL, total, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (base == MAP_FAILED) return (char*)malloc(stack_size); /* fallback */

    /* Guard page: PROT_NONE at lowest address */
    mprotect(base, g_page_size, PROT_NONE);

    /* Pre-fault every page (MAP_POPULATE doesn't exist on macOS) */
    volatile char* p = (char*)base + g_page_size;
    for (size_t i = 0; i < stack_size; i += g_page_size) {
        p[i] = 0;
    }

    return (char*)base + g_page_size; /* usable region starts after guard */
}

void sched_stack_free(Scheduler* s, char* stack, size_t stack_size) {
    if (!stack) return;
    if (!g_page_size) g_page_size = (size_t)sysconf(_SC_PAGESIZE);

    /* Check if this is an mmap'd stack (address will be page-aligned - guard_size) */
    if (s && s->stack_pool_count < STACK_POOL_MAX) {
        s->stack_pool[s->stack_pool_count++] = stack;
        return;
    }

    /* Pool full — munmap (or free if malloc fallback) */
    void* base = stack - g_page_size;
    /* Check if it looks like an mmap'd region (page-aligned base) */
    if (((uintptr_t)base & (g_page_size - 1)) == 0) {
        munmap(base, stack_size + g_page_size);
    } else {
        free(stack);
    }
}

/* ── QSBR Implementation (Day 140 — PPoPP 2024 amortized-free) ── */

void qsbr_init(int num_schedulers) {
    atomic_init(&g_qsbr.global_epoch, 1);  /* Start at 1 so epoch 0 means "never seen" */
    g_qsbr.thread_count = num_schedulers;
    for (int i = 0; i < num_schedulers; i++) {
        atomic_init(&g_qsbr.threads[i].epoch, 0);
        atomic_init(&g_qsbr.threads[i].online, false);
    }
}

void qsbr_thread_online(int sched_id) {
    /* Publish current epoch so we don't block grace periods immediately */
    uint64_t ge = atomic_load_explicit(&g_qsbr.global_epoch, memory_order_relaxed);
    atomic_store_explicit(&g_qsbr.threads[sched_id].epoch, ge, memory_order_relaxed);
    atomic_store_explicit(&g_qsbr.threads[sched_id].online, true, memory_order_release);
}

void qsbr_thread_offline(int sched_id) {
    atomic_store_explicit(&g_qsbr.threads[sched_id].online, false, memory_order_release);
}

void qsbr_retire(Scheduler* s, Actor* actor) {
    RetireRing* r = &s->retire_ring;
    uint32_t idx = r->tail & RETIRE_MASK;
    r->actors[idx] = actor;
    r->epochs[idx] = atomic_load_explicit(&g_qsbr.global_epoch, memory_order_relaxed);
    r->tail++;

    /* If ring is full, force-drain oldest entry (shouldn't happen in practice
     * with RETIRE_CAP=256 and amortized reclaim every quantum) */
    if (r->tail - r->head >= RETIRE_CAP) {
        uint32_t hi = r->head & RETIRE_MASK;
        r->actors[hi] = NULL;
        r->head++;
    }
}

void qsbr_reclaim_amortized(Scheduler* s) {
    RetireRing* r = &s->retire_ring;
    /* Drip-reclaim at most 2 actors per call (PPoPP 2024 amortized pattern).
     * Once safe, the actor pointer is guaranteed not to be dereferenced by
     * any scheduler's deque/runnext. Actor memory stays in the registry
     * for result queries — actual free is deferred to actor_reset_all. */
    int freed = 0;
    while (r->head != r->tail && freed < 2) {
        uint32_t hi = r->head & RETIRE_MASK;
        if (!qsbr_safe(r->epochs[hi])) break;  /* Not yet safe */
        r->actors[hi] = NULL;
        r->head++;
        freed++;
    }
}

void qsbr_drain_all(void) {
    /* Called after all workers joined — single-threaded.
     * Clear retire rings. Actor memory stays in registry for result queries;
     * actual free is deferred to actor_reset_all. */
    for (int i = 0; i < g_num_schedulers; i++) {
        RetireRing* r = &g_schedulers[i].retire_ring;
        while (r->head != r->tail) {
            uint32_t hi = r->head & RETIRE_MASK;
            r->actors[hi] = NULL;
            r->head++;
        }
    }
}

/* ── Scheduler initialization ── */

void sched_init(int num_schedulers) {
    if (num_schedulers < 1) num_schedulers = 1;
    if (num_schedulers > MAX_SCHEDULERS) num_schedulers = MAX_SCHEDULERS;

    g_num_schedulers = num_schedulers;
    if (!g_page_size) g_page_size = (size_t)sysconf(_SC_PAGESIZE);

    /* Initialize yield sentinel */
    memset(&_yield_sentinel_storage, 0, sizeof(Cell));
    _yield_sentinel_storage.type = CELL_ATOM_SYMBOL;
    _yield_sentinel_storage.rc.biased = BRC_IMMORTAL;

    /* Initialize actor striped locks */
    actor_locks_init();

    /* Initialize global overflow queue */
    global_queue_init();

    for (int i = 0; i < num_schedulers; i++) {
        Scheduler* s = &g_schedulers[i];
        memset(s, 0, sizeof(Scheduler));
        s->id = i;
        atomic_init(&s->running, false);
        atomic_init(&s->should_stop, false);
        atomic_init(&s->runnext, (Actor*)NULL);
        s->runnext_consecutive = 0;
        s->rng = (uint32_t)(i + 1) * 2654435761u; /* Knuth multiplicative hash */

        ws_init(&s->deque);

        /* Eventcount-based parking */
        atomic_init(&s->parked, false);

        /* Stack pool starts empty */
        s->stack_pool_count = 0;
    }

    /* Initialize global eventcount + searching state */
    ec_init(&g_sched_ec);
    atomic_init(&g_num_searching, 0);

    /* Initialize QSBR subsystem */
    qsbr_init(num_schedulers);

    /* Publish scheduler 0 trace pointers (main thread — always available) */
    g_schedulers[0].trace_buf_ptr = tls_trace_buf;
    g_schedulers[0].trace_pos_ptr = &tls_trace_pos;

    /* Calibrate TSC for trace timestamp conversion */
    tsc_calibrate();

    atomic_store(&g_sched_initialized, true);
}

void sched_shutdown(void) {
    for (int i = 0; i < g_num_schedulers; i++) {
        Scheduler* s = &g_schedulers[i];
        atomic_store_explicit(&s->should_stop, true, memory_order_release);
    }
    ec_notify_all(&g_sched_ec);

    /* Wait for worker threads (not scheduler 0 = main thread) */
    for (int i = 1; i < g_num_schedulers; i++) {
        if (atomic_load_explicit(&g_schedulers[i].running, memory_order_acquire)) {
            pthread_join(g_schedulers[i].thread, NULL);
        }
    }

    for (int i = 0; i < g_num_schedulers; i++) {
        Scheduler* s = &g_schedulers[i];
        ws_destroy(&s->deque);

        /* Free pooled stacks */
        for (int j = 0; j < s->stack_pool_count; j++) {
            if (s->stack_pool[j]) {
                void* base = s->stack_pool[j] - g_page_size;
                if (((uintptr_t)base & (g_page_size - 1)) == 0) {
                    munmap(base, FIBER_DEFAULT_STACK_SIZE + g_page_size);
                } else {
                    free(s->stack_pool[j]);
                }
            }
        }
        s->stack_pool_count = 0;
    }

    actor_locks_destroy();
    atomic_store(&g_sched_initialized, false);
}

/* ── LIFO slot + deque enqueue ── */
void sched_enqueue(Scheduler* sched, Actor* actor) {
    bool is_owner = (sched->id == tls_scheduler_id);

    /* Try LIFO slot first (atomic exchange — safe from any thread) */
    Actor* prev = atomic_exchange_explicit(&sched->runnext, actor, memory_order_acq_rel);
    if (prev) {
        if (is_owner) {
            /* Owner thread: push to own deque */
            ws_push(&sched->deque, prev);
        } else {
            /* Cross-thread enqueue: deque push is owner-only.
             * Route overflow to the global MPMC queue instead. */
            global_queue_push(prev);
        }
    }

    /* ws_steal has a fallback path that steals directly from the
     * owner's active block, so no force_grant needed per-push. */

    /* Notify ALL parked schedulers via eventcount.
     * Must wake all: runnext is owner-only (not stealable by others).
     * If we only wake one thread and it's not the runnext owner,
     * the owner stays parked and never sees its work → deadlock.
     * With ≤16 schedulers, thundering herd cost is negligible. */
    if (g_num_schedulers > 1) {
        ec_notify_all(&g_sched_ec);
    }
}

/* ── Steal-half policy ── */
Actor* sched_try_steal(Scheduler* thief) {
    if (g_num_schedulers <= 1) return NULL;

    uint32_t start = xorshift32(&thief->rng) % g_num_schedulers;
    for (int i = 0; i < g_num_schedulers; i++) {
        int victim = (start + i) % g_num_schedulers;
        if (victim == thief->id) continue;

        /* Steal first item */
        Actor* first = ws_steal(&g_schedulers[victim].deque);
        if (!first) continue;

        thief->stat_steals++;
        trace_record(TRACE_STEAL, (uint16_t)first->id, (uint16_t)victim);

        /* Steal-half: steal up to half of remaining */
        int64_t victim_size = ws_size(&g_schedulers[victim].deque);
        int to_steal = (int)(victim_size / 2);
        if (to_steal > 16) to_steal = 16; /* Cap per steal batch */
        for (int j = 0; j < to_steal; j++) {
            Actor* a = ws_steal(&g_schedulers[victim].deque);
            if (!a) break;
            ws_push(&thief->deque, a);
        }

        return first;
    }
    return NULL;
}


/* ── Scheduler-driven actor execution ── */

/* Check if an actor's suspension condition is met.
 * Extracted from actor_run_all for reuse by multi-scheduler. */
bool sched_actor_runnable(Actor* actor) {
    Fiber* fiber = actor->fiber;
    if (fiber->state != FIBER_SUSPENDED) return true; /* READY or RUNNING */

    switch (fiber->suspend_reason) {
        case SUSPEND_MAILBOX:
            return atomic_load_explicit(&actor->mailbox.count, memory_order_relaxed) > 0;
        case SUSPEND_CHAN_RECV: {
            Channel* chan = channel_lookup(fiber->suspend_channel_id);
            return !chan || chan->count > 0 || chan->closed;
        }
        case SUSPEND_CHAN_SEND: {
            Channel* chan = channel_lookup(fiber->suspend_channel_id);
            return !chan || chan->count < chan->capacity;
        }
        case SUSPEND_SELECT: {
            for (int j = 0; j < fiber->suspend_select_count; j++) {
                Channel* chan = channel_lookup(fiber->suspend_select_ids[j]);
                if (chan && (chan->count > 0 || chan->closed)) return true;
            }
            return false;
        }
        case SUSPEND_TASK_AWAIT: {
            Actor* awaited = actor_lookup(fiber->suspend_await_actor_id);
            return !awaited || !awaited->alive;
        }
        case SUSPEND_GENERAL:
            return false; /* Wait for explicit resume */
        case SUSPEND_REDUCTION:
            return true; /* Immediately runnable */
    }
    return false;
}

/* Prepare resume value for a suspended actor whose condition is met.
 * Extracted from actor_run_all for reuse by multi-scheduler. */
Cell* sched_prepare_resume(Actor* actor) {
    Fiber* fiber = actor->fiber;
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
            resume_val = cell_nil();
            break;
    }

    return resume_val;
}

/* Run one actor for one quantum (CONTEXT_REDS reductions).
 * Sets up thread-local context, runs fiber, handles yield/finish.
 * Returns: 1=alive (did work), 0=dead, -1=alive but blocked (no work done). */
int sched_run_one_quantum(Scheduler* sched, Actor* actor) {
    /* Check for externally-killed actor (exit signal from another thread) */
    if (!actor->alive) {
        return 0; /* Already dead — don't run, don't re-enqueue */
    }

    Fiber* fiber = actor->fiber;

    /* Check if suspended actor is runnable */
    if (fiber->state == FIBER_SUSPENDED && !sched_actor_runnable(actor)) {
        return -1; /* Still alive but blocked — re-enqueue, no tick consumed */
    }

    /* Set thread-local actor/fiber context */
    Actor* prev_actor = actor_current();
    Fiber* prev_fiber = fiber_current();
    actor_set_current(actor);
    fiber_set_current(fiber);

    /* Reassign fiber's eval context to this scheduler's context
     * (actor may have been stolen from another scheduler) */
    fiber->eval_ctx = &sched->eval_ctx;

    /* Save/restore per-fiber continuation for multi-scheduler correctness:
     * the shared EvalContext's continuation fields are per-scheduler,
     * but each fiber needs its own continuation state. */
    if (fiber->saved_continuation) {
        sched->eval_ctx.continuation = fiber->saved_continuation;
        sched->eval_ctx.continuation_env = fiber->saved_continuation_env;
        fiber->saved_continuation = NULL;
        fiber->saved_continuation_env = NULL;
    }

    if (fiber->state == FIBER_READY) {
        /* First run — set reduction budget */
        sched->eval_ctx.reductions_left = CONTEXT_REDS;
        trace_record(TRACE_RESUME, (uint16_t)actor->id, 0);
        fiber_start(fiber);
    } else if (fiber->state == FIBER_SUSPENDED) {
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
        Cell* resume_val = sched_prepare_resume(actor);
        trace_record(TRACE_RESUME, (uint16_t)actor->id, (uint16_t)fiber->suspend_reason);

        /* Clear stale suspend metadata after computing resume value */
        fiber->suspend_channel_id = -1;
        fiber->suspend_await_actor_id = -1;
        fiber->suspend_select_count = 0;
        fiber->suspend_reason = SUSPEND_GENERAL;
        sched->eval_ctx.reductions_left = CONTEXT_REDS;
        fiber_resume(fiber, resume_val);
        if (resume_val) cell_release(resume_val);
    }

    /* Save continuation back to fiber for next quantum */
    if (sched->eval_ctx.continuation) {
        fiber->saved_continuation = sched->eval_ctx.continuation;
        fiber->saved_continuation_env = sched->eval_ctx.continuation_env;
        sched->eval_ctx.continuation = NULL;
        sched->eval_ctx.continuation_env = NULL;
    }

    sched->stat_reductions += CONTEXT_REDS;
    sched->stat_actors_run++;

    /* Check if actor finished */
    if (fiber->state == FIBER_FINISHED) {
        /* actor_finish atomically checks alive + marks dead + notifies.
         * Returns false if actor_exit_signal already killed it. */
        actor_finish(actor, fiber->result);

        /* QSBR: retire actor for deferred destroy (other schedulers may
         * still hold stale pointers in deques/runnext). */
        qsbr_retire(sched, actor);

        /* Restore */
        actor_set_current(prev_actor);
        fiber_set_current(prev_fiber);
        return 0; /* Dead */
    }

    sched->stat_context_switches++;

    /* Restore */
    actor_set_current(prev_actor);
    fiber_set_current(prev_fiber);
    return 1; /* Still alive, did work */
}

/* ── Worker thread main loop ── */

static void* scheduler_worker_main(void* arg) {
    Scheduler* sched = (Scheduler*)arg;
    tls_scheduler_id = (uint16_t)sched->id;

    /* Copy shared eval context into per-scheduler context */
    if (g_shared_eval_ctx) {
        sched->eval_ctx = *g_shared_eval_ctx;
        sched->eval_ctx.reductions_left = CONTEXT_REDS;
        sched->eval_ctx.continuation = NULL;
        sched->eval_ctx.continuation_env = NULL;
    }
    eval_set_current_context(&sched->eval_ctx);

    atomic_store_explicit(&sched->running, true, memory_order_release);

    /* Publish trace buffer pointers for cross-scheduler merge */
    sched->trace_buf_ptr = tls_trace_buf;
    sched->trace_pos_ptr = &tls_trace_pos;

    /* QSBR: declare this thread online */
    qsbr_thread_online(sched->id);

    int idle_spins = 0;
    bool was_searching = false;
    while (!atomic_load_explicit(&sched->should_stop, memory_order_acquire)) {
        Actor* actor = NULL;

        /* 1. LIFO slot (hottest, cache-local, not stealable) */
        if (sched->runnext_consecutive < 3) {
            actor = atomic_exchange_explicit(&sched->runnext, NULL, memory_order_acquire);
            if (actor) { sched->runnext_consecutive++; goto run; }
        } else {
            sched->runnext_consecutive = 0;
        }

        /* 2. Own deque (Chase-Lev pop — owner path) */
        actor = ws_pop(&sched->deque);
        if (actor) { sched->runnext_consecutive = 0; goto run; }

        /* 3. Global overflow queue */
        actor = global_queue_pop();
        if (actor) goto run;

        /* 4. Steal half from random victim */
        actor = sched_try_steal(sched);
        if (actor) goto run;

        /* 5. Adaptive idle: spin → eventcount → tiered park */
        idle_spins++;
        if (idle_spins < 64) {
            /* Stage 1: spin with YIELD hint */
#if defined(__aarch64__)
            __builtin_arm_yield();
#elif defined(__x86_64__)
            __asm__ volatile("pause");
#endif
        } else if (idle_spins < 128) {
            sched_yield();
        } else {
            /* Transition: searching → preparing to park.
             * Decrement num_searching. If we were the last searcher,
             * re-scan all queues one final time (Go/Tokio invariant). */
            if (was_searching) {
                uint32_t prev = atomic_fetch_sub_explicit(&g_num_searching, 1, memory_order_acq_rel);
                was_searching = false;
                if (prev == 1) {
                    /* Last searcher: re-scan before committing to sleep */
                    actor = ws_pop(&sched->deque);
                    if (!actor) actor = global_queue_pop();
                    if (!actor) actor = sched_try_steal(sched);
                    if (actor) { idle_spins = 0; goto run; }
                }
            }

            /* Eventcount 2-phase commit */
            uint32_t epoch = ec_prepare_wait(&g_sched_ec);

            /* Between prepare and commit: check ALL termination/work conditions.
             * Any notification between prepare and commit bumps the epoch,
             * causing commit_wait to return immediately. NO LOST WAKEUPS. */
            if (atomic_load_explicit(&sched->should_stop, memory_order_acquire) ||
                atomic_load_explicit(&g_alive_actors, memory_order_acquire) <= 0) {
                ec_cancel_wait(&g_sched_ec);
                break;
            }

            /* One final check for ALL work sources (including runnext!) */
            actor = atomic_exchange_explicit(&sched->runnext, NULL, memory_order_acquire);
            if (!actor) actor = ws_pop(&sched->deque);
            if (!actor) actor = global_queue_pop();
            if (!actor) actor = sched_try_steal(sched);
            if (actor) {
                ec_cancel_wait(&g_sched_ec);
                idle_spins = 0;
                sched->runnext_consecutive = 0;
                goto run;
            }

            /* Commit to sleep — tiered park (YIELD → WFE → ulock) */
            qsbr_thread_offline(sched->id);
            atomic_store_explicit(&sched->parked, true, memory_order_release);
            ec_commit_wait(&g_sched_ec, epoch);
            atomic_store_explicit(&sched->parked, false, memory_order_release);
            qsbr_thread_online(sched->id);
            idle_spins = 0;
        }
        continue;

    run:
        idle_spins = 0;
        /* If we just found work and aren't marked searching, try to become a searcher.
         * Cap at num_schedulers/2 to prevent contention. */
        if (!was_searching &&
            atomic_load_explicit(&g_num_searching, memory_order_relaxed) < (uint32_t)(g_num_schedulers / 2)) {
            atomic_fetch_add_explicit(&g_num_searching, 1, memory_order_relaxed);
            was_searching = true;
        }
        atomic_fetch_add_explicit(&g_running_actors, 1, memory_order_relaxed);
        int result = sched_run_one_quantum(sched, actor);
        if (result == 1) {
            /* Did work — always re-enqueue */
            sched_enqueue(sched, actor);
        }
        /* result == -1 (blocked): do NOT re-enqueue.
         * All suspend paths with wake protocol (MAILBOX, CHAN_RECV,
         * CHAN_SEND, SELECT, TASK_AWAIT) set wait_flag=1 and have
         * sender-side wake that CAS's wait_flag 1→0 + sched_enqueue.
         * Re-enqueueing here causes double-enqueue races → double-run
         * → Bus error / fiber stack corruption.
         * SUSPEND_GENERAL also not re-enqueued — it requires explicit
         * resume (not poll-based). */
        /* Decrement AFTER enqueue so the actor is never in a gap state
         * (not running, not queued) which would fool idle detection. */
        atomic_fetch_sub_explicit(&g_running_actors, 1, memory_order_release);

        /* QSBR: declare quiescent state + drip-free retired actors */
        qsbr_quiescent(sched->id);
        qsbr_reclaim_amortized(sched);
    }

    /* Clean up searching state on exit */
    if (was_searching) {
        atomic_fetch_sub_explicit(&g_num_searching, 1, memory_order_relaxed);
    }

    qsbr_thread_offline(sched->id);
    atomic_store_explicit(&sched->running, false, memory_order_release);
    return NULL;
}

/* ── Distribute actors across schedulers (round-robin) ── */
static void sched_distribute_actors(void) {
    /* Get all alive actors from the registry and distribute them */
    for (int i = 0; i < MAX_ACTORS; i++) {
        Actor* a = actor_lookup_by_index(i);
        if (a && a->alive) {
            int target = a->home_scheduler % g_num_schedulers;
            sched_enqueue(&g_schedulers[target], a);
        }
    }
}

/* Check if all schedulers are idle (no work anywhere) */
static bool sched_all_idle(void) {
    for (int i = 0; i < g_num_schedulers; i++) {
        Scheduler* s = &g_schedulers[i];
        if (atomic_load_explicit(&s->runnext, memory_order_relaxed) != NULL) return false;
        if (ws_size(&s->deque) > 0) return false;
    }
    if (global_queue_size() > 0) return false;
    return true;
}

/* ── Main scheduler entry point ── */

int sched_run_all(int max_ticks) {
    /* Single-scheduler: delegate to existing actor_run_all for compatibility */
    if (g_num_schedulers <= 1) {
        Scheduler* s = &g_schedulers[0];
        int ticks = actor_run_all(max_ticks);
        s->stat_actors_run += (uint64_t)ticks;
        return ticks;
    }

    /* ── Multi-scheduler path ── */
    LOG_DEBUG("sched_run_all: multi-sched max_ticks=%d alive=%d",
        max_ticks, atomic_load_explicit(&g_alive_actors, memory_order_relaxed));
    g_shared_eval_ctx = eval_get_current_context();

    /* Set up scheduler 0's eval context */
    g_schedulers[0].eval_ctx = *g_shared_eval_ctx;
    g_schedulers[0].eval_ctx.reductions_left = CONTEXT_REDS;
    g_schedulers[0].eval_ctx.continuation = NULL;
    g_schedulers[0].eval_ctx.continuation_env = NULL;
    eval_set_current_context(&g_schedulers[0].eval_ctx);

    /* Distribute actors to schedulers */
    sched_distribute_actors();

    /* Reset global state for this run */
    ec_init(&g_sched_ec);
    atomic_store_explicit(&g_num_searching, 0, memory_order_relaxed);
    atomic_store_explicit(&g_running_actors, 0, memory_order_relaxed);

    /* Spawn workers 1..N-1 */
    for (int i = 1; i < g_num_schedulers; i++) {
        atomic_store_explicit(&g_schedulers[i].should_stop, false, memory_order_release);
        atomic_store_explicit(&g_schedulers[i].parked, false, memory_order_release);
        pthread_create(&g_schedulers[i].thread, NULL, scheduler_worker_main, &g_schedulers[i]);
    }

    /* Scheduler 0 = main thread (worker loop + timer_tick_all) */
    Scheduler* s0 = &g_schedulers[0];
    tls_scheduler_id = 0;
    atomic_store_explicit(&s0->running, true, memory_order_release);

    /* Publish trace buffer pointers for scheduler 0 */
    s0->trace_buf_ptr = tls_trace_buf;
    s0->trace_pos_ptr = &tls_trace_pos;

    /* QSBR: scheduler 0 online */
    qsbr_thread_online(0);

    int ticks = 0;
    int idle_spins = 0;
    int epoch_counter = 0;
    while (ticks < max_ticks) {
        Actor* actor = NULL;

        /* Pop order: LIFO slot → deque → global queue → steal */
        if (s0->runnext_consecutive < 3) {
            actor = atomic_exchange_explicit(&s0->runnext, NULL, memory_order_acquire);
            if (actor) s0->runnext_consecutive++;
        } else {
            s0->runnext_consecutive = 0;
        }
        if (!actor) { actor = ws_pop(&s0->deque); s0->runnext_consecutive = 0; }
        if (!actor) actor = global_queue_pop();
        if (!actor) actor = sched_try_steal(s0);

        if (actor) {
            idle_spins = 0;
            atomic_fetch_add_explicit(&g_running_actors, 1, memory_order_relaxed);
            int result = sched_run_one_quantum(s0, actor);
            if (result == 1) sched_enqueue(s0, actor);  /* did work — re-enqueue */
            /* result == -1 (blocked): NOT re-enqueued.
             * Wake path (actor_send, channel_wake_actor, actor_finish)
             * owns re-enqueue via wait_flag CAS. */
            atomic_fetch_sub_explicit(&g_running_actors, 1, memory_order_release);
            ticks++;
        } else {
            idle_spins++;
            bool no_running = atomic_load_explicit(&g_running_actors, memory_order_acquire) <= 0;
            int alive = atomic_load_explicit(&g_alive_actors, memory_order_acquire);
            /* Termination: all actors dead, nothing in flight or queued */
            LOG_DEBUG("S0 idle=%d no_run=%d alive=%d g_run=%d all_idle=%d",
                idle_spins, no_running, alive,
                atomic_load(&g_running_actors), sched_all_idle());
            if (idle_spins > 10 && no_running && alive <= 0 && sched_all_idle()) {
                LOG_DEBUG("S0 terminating: alive=%d", alive);
                ec_notify_all(&g_sched_ec);
                break;
            }
            /* Brief park: prepare-wait on eventcount so we wake immediately
             * when any worker enqueues work (ec_notify_one in sched_enqueue).
             * Avoids burning CPU and gives workers time to complete. */
            if (idle_spins > 32) {
                uint32_t epoch = ec_prepare_wait(&g_sched_ec);
                /* Re-check ALL sources before committing (including runnext!) */
                actor = atomic_exchange_explicit(&s0->runnext, NULL, memory_order_acquire);
                if (!actor) actor = ws_pop(&s0->deque);
                if (!actor) actor = global_queue_pop();
                if (!actor) actor = sched_try_steal(s0);
                if (actor || atomic_load_explicit(&g_alive_actors, memory_order_acquire) <= 0) {
                    ec_cancel_wait(&g_sched_ec);
                    if (actor) { idle_spins = 0; s0->runnext_consecutive = 0; continue; }
                    break;
                }
                /* Park until woken — bounded by tiered park (YIELD→WFE→ulock) */
                LOG_DEBUG("S0 parking, alive=%d epoch=%u",
                    atomic_load_explicit(&g_alive_actors, memory_order_relaxed), epoch);
                qsbr_thread_offline(0);
                ec_commit_wait(&g_sched_ec, epoch);
                qsbr_thread_online(0);
                LOG_DEBUG("S0 woke from park");
                idle_spins = 0;
            }
        }

        /* QSBR: declare quiescent + amortized reclaim */
        qsbr_quiescent(0);
        qsbr_reclaim_amortized(s0);

        /* Advance global QSBR epoch every ~100 ticks (scheduler 0 only) */
        if (++epoch_counter >= 100) {
            qsbr_advance_epoch();
            epoch_counter = 0;
        }

        timer_tick_all();
    }

    qsbr_thread_offline(0);

    /* Shutdown workers — set should_stop THEN wake all via eventcount.
     * The epoch bump ensures any worker in prepare/commit sees the change. */
    for (int i = 1; i < g_num_schedulers; i++) {
        atomic_store_explicit(&g_schedulers[i].should_stop, true, memory_order_release);
    }
    ec_notify_all(&g_sched_ec);
    for (int i = 1; i < g_num_schedulers; i++) {
        pthread_join(g_schedulers[i].thread, NULL);
    }

    atomic_store_explicit(&s0->running, false, memory_order_release);

    /* QSBR: drain all retire rings now that workers are joined */
    qsbr_drain_all();

    /* Drain all queues so no stale actor pointers survive across runs.
     * Workers are joined, so this is single-threaded and safe.
     * Re-init BWoS deques to reset epoch counters and block cursors. */
    for (int i = 0; i < g_num_schedulers; i++) {
        Scheduler* s = &g_schedulers[i];
        atomic_store_explicit(&s->runnext, NULL, memory_order_relaxed);
        s->runnext_consecutive = 0;
        /* Reset deque — BWoS epoch counters must start fresh each run */
        ws_init(&s->deque);
    }
    /* Drain global queue */
    while (global_queue_pop() != NULL) {}

    /* Restore original eval context */
    eval_set_current_context(g_shared_eval_ctx);

    return ticks;
}

Scheduler* sched_get(int id) {
    if (id < 0 || id >= g_num_schedulers) return NULL;
    return &g_schedulers[id];
}

int sched_count(void) {
    return g_num_schedulers;
}

void sched_set_count(int n) {
    /* Only before multi-thread activation */
    if (n < 1) n = 1;
    if (n > MAX_SCHEDULERS) n = MAX_SCHEDULERS;

    /* Initialize new schedulers if growing */
    for (int i = g_num_schedulers; i < n; i++) {
        Scheduler* s = &g_schedulers[i];
        memset(s, 0, sizeof(Scheduler));
        s->id = i;
        atomic_init(&s->running, false);
        atomic_init(&s->should_stop, false);
        atomic_init(&s->runnext, (Actor*)NULL);
        s->runnext_consecutive = 0;
        s->rng = (uint32_t)(i + 1) * 2654435761u;
        ws_init(&s->deque);
        atomic_init(&s->parked, false);
        s->stack_pool_count = 0;
    }

    g_num_schedulers = n;

    /* Update QSBR thread count to match new scheduler count */
    g_qsbr.thread_count = n;
    /* Initialize QSBR state for new threads */
    for (int i = 0; i < n; i++) {
        atomic_store_explicit(&g_qsbr.threads[i].online, false, memory_order_relaxed);
        atomic_store_explicit(&g_qsbr.threads[i].epoch, 0, memory_order_relaxed);
    }
}

/* ── Global trace merge (Day 140) ──
 * K-way merge by timestamp across all scheduler trace buffers.
 * Only safe when workers are parked/joined (post-sched_run_all). */

int sched_trace_merge(TraceEvent* out, int out_cap, int filter_kind) {
    if (!out || out_cap <= 0) return 0;

    /* Per-scheduler iteration state */
    uint32_t counts[MAX_SCHEDULERS];
    uint32_t starts[MAX_SCHEDULERS];
    uint32_t cursors[MAX_SCHEDULERS];
    TraceEvent* bufs[MAX_SCHEDULERS];
    int active = 0;

    for (int i = 0; i < g_num_schedulers; i++) {
        Scheduler* s = &g_schedulers[i];
        if (!s->trace_buf_ptr || !s->trace_pos_ptr) continue;

        uint32_t pos = *s->trace_pos_ptr;
        uint32_t cnt = pos < TRACE_BUF_CAP ? pos : TRACE_BUF_CAP;
        if (cnt == 0) continue;

        bufs[active] = s->trace_buf_ptr;
        counts[active] = cnt;
        starts[active] = pos > TRACE_BUF_CAP ? (pos - TRACE_BUF_CAP) : 0;
        cursors[active] = 0;
        active++;
    }

    if (active == 0) return 0;

    /* K-way merge: repeatedly pick the event with smallest timestamp */
    int written = 0;
    while (written < out_cap) {
        int best = -1;
        uint64_t best_ts = UINT64_MAX;

        for (int i = 0; i < active; i++) {
            if (cursors[i] >= counts[i]) continue;
            uint32_t idx = (starts[i] + cursors[i]) & (TRACE_BUF_CAP - 1);
            TraceEvent* ev = &bufs[i][idx];
            if (filter_kind >= 0 && ev->kind != (uint8_t)filter_kind) {
                cursors[i]++;
                i--;  /* Re-check this scheduler with next event */
                continue;
            }
            if (ev->timestamp < best_ts) {
                best_ts = ev->timestamp;
                best = i;
            }
        }

        if (best < 0) break;  /* All exhausted */

        uint32_t idx = (starts[best] + cursors[best]) & (TRACE_BUF_CAP - 1);
        out[written++] = bufs[best][idx];
        cursors[best]++;
    }

    return written;
}
