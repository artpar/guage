/* park.h — Platform-adaptive thread parking
 *
 * macOS: __ulock_wait/__ulock_wake (~0.5-1μs, 4-byte footprint)
 *   Used by Rust std, libdispatch, libc++
 * Linux: futex(FUTEX_WAIT_PRIVATE) (~1μs)
 *   Direct syscall, PRIVATE flag for process-local
 *
 * Replaces pthread_cond (~2-5μs, 120+ bytes)
 */
#ifndef GUAGE_PARK_H
#define GUAGE_PARK_H

#include <stdint.h>
#include <stdatomic.h>

/* Park: sleep until *addr changes from expected, or explicitly woken */
void guage_park(_Atomic uint32_t* addr, uint32_t expected);

/* Wake: wake one waiter blocked on addr */
void guage_wake(_Atomic uint32_t* addr);

#endif /* GUAGE_PARK_H */
