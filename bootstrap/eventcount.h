/* eventcount.h — Folly-derived eventcount for race-free parking
 *
 * Packed into a single _Atomic uint64_t:
 *   Bits [63:32] = epoch   (monotonic version counter)
 *   Bits [31:0]  = waiters (threads in prepare-to-park state)
 *
 * Protocol (2-phase commit):
 *   1. ec_prepare_wait  — increment waiters, snapshot epoch
 *   2. Check condition (work available? shutdown?)
 *   3a. ec_commit_wait  — if epoch unchanged, tiered park
 *   3b. ec_cancel_wait  — decrement waiters, bail out
 *
 * Producers call ec_notify_one/all after making work available.
 * Epoch bump between prepare and commit causes immediate return.
 * NO LOST WAKEUPS by construction.
 */
#ifndef GUAGE_EVENTCOUNT_H
#define GUAGE_EVENTCOUNT_H

#include <stdint.h>
#include <stdatomic.h>

#ifndef CACHE_LINE
#define CACHE_LINE 128
#endif

typedef struct {
    _Alignas(CACHE_LINE) _Atomic uint64_t state;
} EventCount;

/* Extract epoch (upper 32 bits) */
static inline uint32_t ec_epoch(uint64_t s) {
    return (uint32_t)(s >> 32);
}

/* Extract waiter count (lower 32 bits) */
static inline uint32_t ec_waiters(uint64_t s) {
    return (uint32_t)(s & 0xFFFFFFFF);
}

static inline void ec_init(EventCount* ec) {
    atomic_init(&ec->state, 0);
}

/* Phase 1: register as waiter, return current epoch */
static inline uint32_t ec_prepare_wait(EventCount* ec) {
    uint64_t prev = atomic_fetch_add_explicit(&ec->state, 1, memory_order_acq_rel);
    return ec_epoch(prev);
}

/* Cancel: unregister as waiter (seq_cst prevents reordering past condition check) */
static inline void ec_cancel_wait(EventCount* ec) {
    atomic_fetch_sub_explicit(&ec->state, 1, memory_order_seq_cst);
}

/* Phase 2: if epoch still matches, park via tiered sleep.
 * Declared extern — implemented in park.c */
void guage_park_tiered(_Atomic uint64_t* ec_state, uint32_t expected_epoch);

static inline void ec_commit_wait(EventCount* ec, uint32_t epoch) {
    /* Check epoch before expensive park */
    uint64_t cur = atomic_load_explicit(&ec->state, memory_order_acquire);
    if (ec_epoch(cur) != epoch) {
        /* Epoch changed — producer notified between prepare and commit */
        atomic_fetch_sub_explicit(&ec->state, 1, memory_order_release);
        return;
    }
    /* Park — will return when epoch changes or spuriously */
    guage_park_tiered(&ec->state, epoch);
    /* Unregister as waiter after wake */
    atomic_fetch_sub_explicit(&ec->state, 1, memory_order_release);
}

/* Wake one parked worker: bump epoch */
static inline void ec_notify_one(EventCount* ec) {
    uint64_t prev = atomic_fetch_add_explicit(&ec->state, (uint64_t)1 << 32, memory_order_acq_rel);
    if (ec_waiters(prev) > 0) {
        /* At least one waiter — issue platform wake */
        extern void guage_wake(_Atomic uint32_t* addr);
        guage_wake((_Atomic uint32_t*)&ec->state);
    }
}

/* Wake all parked workers: bump epoch */
static inline void ec_notify_all(EventCount* ec) {
    uint64_t prev = atomic_fetch_add_explicit(&ec->state, (uint64_t)1 << 32, memory_order_acq_rel);
    if (ec_waiters(prev) > 0) {
        /* Wake all — platform-specific broadcast */
#if defined(__APPLE__)
        extern int __ulock_wake(uint32_t op, void* addr, uint64_t wake_value);
#define UL_COMPARE_AND_WAIT_EC 1
#define ULF_NO_ERRNO_EC        0x01000000
#define ULF_WAKE_ALL_EC        0x00000100
        __ulock_wake(UL_COMPARE_AND_WAIT_EC | ULF_NO_ERRNO_EC | ULF_WAKE_ALL_EC,
                     (void*)&ec->state, 0);
#elif defined(__linux__)
        extern long syscall(long number, ...);
#include <linux/futex.h>
#include <sys/syscall.h>
        syscall(SYS_futex, &ec->state, FUTEX_WAKE_PRIVATE, INT32_MAX, NULL, NULL, 0);
#else
        /* Fallback: single wake (best effort) */
        extern void guage_wake(_Atomic uint32_t* addr);
        guage_wake((_Atomic uint32_t*)&ec->state);
#endif
    }
}

#endif /* GUAGE_EVENTCOUNT_H */
