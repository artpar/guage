#ifndef GUAGE_CHANNEL_H
#define GUAGE_CHANNEL_H

#include <stdbool.h>
#include <stdatomic.h>
#include <stdint.h>
#include "cell.h"

#define MAX_CHANNELS 256
#define DEFAULT_CHANNEL_CAPACITY 64

/* Cache line size for slot alignment (matches scheduler.h) */
#ifndef CACHE_LINE
#define CACHE_LINE 128
#endif

/* Vyukov MPMC slot — each on its own cache line for zero false sharing.
 * Padding ensures each slot occupies exactly one cache line. */
typedef struct {
    _Atomic uint64_t sequence;     /* Generation counter (8 bytes) */
    Cell* value;                   /* Payload (8 bytes) */
    char _pad[CACHE_LINE - 16];   /* Fill to CACHE_LINE (128 - 16 = 112 bytes) */
} VyukovSlot __attribute__((aligned(CACHE_LINE)));

/* Channel — Vyukov MPMC bounded queue for inter-actor communication
 * 1 CAS per enqueue/dequeue operation, cache-line-aligned slots. */
typedef struct Channel {
    int id;
    uint32_t capacity;           /* Power of 2 */
    uint32_t mask;               /* capacity - 1 */
    _Atomic bool closed;

    /* Separated to own cache lines — single-writer principle */
    _Alignas(CACHE_LINE) _Atomic uint64_t enqueue_pos;
    _Alignas(CACHE_LINE) _Atomic uint64_t dequeue_pos;

    VyukovSlot* buffer;          /* Cache-line-aligned allocation */

    /* Compatibility: approximate count for suspend checks.
     * Not perfectly synchronized — used for heuristic scheduling only. */
    _Atomic int32_t count;

    /* Waiter registration (Day 134): actor IDs blocked on this channel.
     * -1 means no waiter. Single CAS to claim/release waiter slot. */
    _Atomic int recv_waiter;   /* Actor ID waiting to recv, or -1 */
    _Atomic int send_waiter;   /* Actor ID waiting to send, or -1 */
} Channel;

/* Lifecycle */
Channel* channel_create(int capacity);
void     channel_close(Channel* chan);
void     channel_destroy(Channel* chan);

/* Non-blocking buffer operations (return success/failure) */
bool  channel_try_send(Channel* chan, Cell* value);
Cell* channel_try_recv(Channel* chan);  /* NULL if empty */

/* Registry */
Channel* channel_lookup(int id);
void     channel_reset_all(void);

#endif /* GUAGE_CHANNEL_H */
