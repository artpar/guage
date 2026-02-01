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

/* Tiered park for eventcount: YIELD → WFE → ulock/futex
 * Parks until the epoch (upper 32 bits of *ec_state) changes from expected_epoch.
 * Stage 1: ARM YIELD × 64 (~100ns)
 * Stage 2: WFE × 256 (~1-5μs, no syscall, Apple Silicon native)
 * Stage 3: ulock/futex (OS sleep, for long idles) */
void guage_park_tiered(_Atomic uint64_t* ec_state, uint32_t expected_epoch);

#endif /* GUAGE_PARK_H */
