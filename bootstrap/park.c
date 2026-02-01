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
