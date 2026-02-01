/* park.c — Platform-adaptive thread parking
 *
 * macOS: __ulock_wait/__ulock_wake (Darwin futex, ~0.5-1μs)
 *   Used by Rust std, libdispatch, libc++
 * Linux: futex(FUTEX_WAIT_PRIVATE) (~1μs)
 *   Direct syscall with PRIVATE flag for process-local
 *
 * Both: 4-byte footprint vs 120+ bytes for pthread_cond
 */

#include "park.h"

#if defined(__linux__)

#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

void guage_park(_Atomic uint32_t* addr, uint32_t expected) {
    syscall(SYS_futex, addr, FUTEX_WAIT_PRIVATE, expected, NULL, NULL, 0);
}

void guage_wake(_Atomic uint32_t* addr) {
    syscall(SYS_futex, addr, FUTEX_WAKE_PRIVATE, 1, NULL, NULL, 0);
}

#elif defined(__APPLE__)

/* Darwin ulock — private API, stable since macOS 10.12
 * Used by Rust std::sync, libdispatch, libc++ */
#define UL_COMPARE_AND_WAIT 1
#define ULF_NO_ERRNO        0x01000000

extern int __ulock_wait(uint32_t op, void* addr, uint64_t value, uint32_t timeout);
extern int __ulock_wake(uint32_t op, void* addr, uint64_t wake_value);

void guage_park(_Atomic uint32_t* addr, uint32_t expected) {
    __ulock_wait(UL_COMPARE_AND_WAIT | ULF_NO_ERRNO, (void*)addr,
                 (uint64_t)expected, 0);
}

void guage_wake(_Atomic uint32_t* addr) {
    __ulock_wake(UL_COMPARE_AND_WAIT | ULF_NO_ERRNO, (void*)addr, 0);
}

#else
/* Fallback: busy spin (should not be used in production) */
#include <sched.h>

void guage_park(_Atomic uint32_t* addr, uint32_t expected) {
    while (atomic_load_explicit(addr, memory_order_acquire) == expected) {
        sched_yield();
    }
}

void guage_wake(_Atomic uint32_t* addr) {
    /* No-op: busy spin in park will eventually see the change */
    (void)addr;
}
#endif

/* ── Tiered park for eventcount ──
 * 3-stage parking optimized for Apple Silicon:
 *   Stage 1: ARM YIELD × 64 (~100ns, pipeline hint)
 *   Stage 2: WFE × 256 (~1-5μs, cache-line monitor, no syscall)
 *   Stage 3: ulock/futex (OS sleep, for long idles)
 *
 * Returns when epoch (upper 32 bits) changes from expected_epoch. */

static inline uint32_t tiered_ec_epoch(uint64_t s) {
    return (uint32_t)(s >> 32);
}

void guage_park_tiered(_Atomic uint64_t* ec_state, uint32_t expected_epoch) {
    /* Stage 1: YIELD spin × 64 (~100ns) */
    for (int i = 0; i < 64; i++) {
        if (tiered_ec_epoch(atomic_load_explicit(ec_state, memory_order_acquire)) != expected_epoch)
            return;
#if defined(__aarch64__)
        __builtin_arm_yield();
#elif defined(__x86_64__)
        __asm__ volatile("pause");
#endif
    }

    /* Stage 2: WFE / pause loop × 256 (~1-5μs) */
    for (int i = 0; i < 256; i++) {
        if (tiered_ec_epoch(atomic_load_explicit(ec_state, memory_order_acquire)) != expected_epoch)
            return;
#if defined(__aarch64__)
        /* WFE monitors the cache line — wakes on any write to ec_state */
        __asm__ volatile("wfe");
#elif defined(__x86_64__)
        __asm__ volatile("pause");
#else
        sched_yield();
#endif
    }

    /* Stage 3: OS sleep (ulock on macOS, futex on Linux) */
    /* Re-check epoch one final time before syscall */
    uint64_t cur = atomic_load_explicit(ec_state, memory_order_acquire);
    if (tiered_ec_epoch(cur) != expected_epoch) return;

#if defined(__APPLE__)
    /* Park on lower 32 bits (waiter count field — changes on wake) */
    uint32_t val = (uint32_t)cur;
    extern int __ulock_wait(uint32_t op, void* addr, uint64_t value, uint32_t timeout);
#define UL_COMPARE_AND_WAIT_T 1
#define ULF_NO_ERRNO_T        0x01000000
    /* Bounded wait (10ms) so threads can re-check termination conditions.
     * Prevents permanent deadlock when all actors are blocked. */
    __ulock_wait(UL_COMPARE_AND_WAIT_T | ULF_NO_ERRNO_T, (void*)ec_state, (uint64_t)val, 10000);
#elif defined(__linux__)
    uint32_t val = (uint32_t)cur;
    syscall(SYS_futex, ec_state, FUTEX_WAIT_PRIVATE, val, NULL, NULL, 0);
#else
    /* Fallback: sched_yield loop */
    while (tiered_ec_epoch(atomic_load_explicit(ec_state, memory_order_acquire)) == expected_epoch) {
        sched_yield();
    }
#endif
}
