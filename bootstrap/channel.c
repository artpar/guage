#include "channel.h"
#include "actor.h"
#include "scheduler.h"
#include <stdlib.h>
#include <string.h>

/* Global channel registry */
static Channel* g_channels[MAX_CHANNELS];
static int g_channel_count = 0;
static int g_next_channel_id = 1;

/* Round up to next power of 2 */
static uint32_t next_pow2(uint32_t v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v < 2 ? 2 : v;
}

Channel* channel_create(int capacity) {
    if (g_channel_count >= MAX_CHANNELS) return NULL;
    if (capacity <= 0) capacity = DEFAULT_CHANNEL_CAPACITY;

    /* Round capacity to power of 2 for mask-based indexing */
    uint32_t cap = next_pow2((uint32_t)capacity);

    Channel* chan = (Channel*)calloc(1, sizeof(Channel));
    chan->id = g_next_channel_id++;
    chan->capacity = cap;
    chan->mask = cap - 1;
    atomic_init(&chan->closed, false);
    atomic_init(&chan->enqueue_pos, 0);
    atomic_init(&chan->dequeue_pos, 0);
    atomic_init(&chan->count, 0);
    atomic_init(&chan->recv_waiter, -1);
    atomic_init(&chan->send_waiter, -1);

    /* Allocate cache-line-aligned slot buffer */
    chan->buffer = (VyukovSlot*)aligned_alloc(CACHE_LINE, cap * sizeof(VyukovSlot));
    memset(chan->buffer, 0, cap * sizeof(VyukovSlot));

    /* Initialize generation counters */
    for (uint32_t i = 0; i < cap; i++) {
        atomic_init(&chan->buffer[i].sequence, (uint64_t)i);
        chan->buffer[i].value = NULL;
    }

    g_channels[g_channel_count++] = chan;
    return chan;
}

void channel_close(Channel* chan) {
    if (!chan) return;
    atomic_store_explicit(&chan->closed, true, memory_order_release);
    {
        Actor* cur = actor_current();
        trace_record(TRACE_CHAN_CLOSE, cur ? (uint16_t)cur->id : 0, (uint16_t)chan->id);
    }
}

void channel_destroy(Channel* chan) {
    if (!chan) return;

    /* Drain and release any buffered values */
    uint64_t dq = atomic_load_explicit(&chan->dequeue_pos, memory_order_relaxed);
    uint64_t eq = atomic_load_explicit(&chan->enqueue_pos, memory_order_relaxed);
    for (uint64_t i = dq; i < eq; i++) {
        VyukovSlot* slot = &chan->buffer[i & chan->mask];
        if (slot->value) {
            cell_release(slot->value);
            slot->value = NULL;
        }
    }

    free(chan->buffer);
    free(chan);
}

/* Wake a blocked actor: clear wait_flag, enqueue to home scheduler, unpark.
 * Only wakes if wait_flag was 1 (atomically exchanged to 0). */
static void channel_wake_actor(int actor_id) {
    if (actor_id < 0) return;
    Actor* a = actor_lookup(actor_id);
    if (!a || !a->alive) return;
    if (atomic_exchange_explicit(&a->wait_flag, 0, memory_order_acq_rel) == 1) {
        Scheduler* home = sched_get(a->home_scheduler);
        if (home) {
            sched_enqueue(home, a);
        }
    }
}

/* Vyukov MPMC enqueue — 1 CAS per operation */
bool channel_try_send(Channel* chan, Cell* value) {
    if (!chan || atomic_load_explicit(&chan->closed, memory_order_acquire)) return false;

    uint64_t pos = atomic_load_explicit(&chan->enqueue_pos, memory_order_relaxed);
    for (;;) {
        VyukovSlot* slot = &chan->buffer[pos & chan->mask];
        uint64_t seq = atomic_load_explicit(&slot->sequence, memory_order_acquire);
        int64_t diff = (int64_t)seq - (int64_t)pos;

        if (diff == 0) {
            /* Slot is available — try to claim it */
            if (atomic_compare_exchange_weak_explicit(&chan->enqueue_pos, &pos, pos + 1,
                    memory_order_relaxed, memory_order_relaxed)) {
                cell_retain(value);
                slot->value = value;
                atomic_store_explicit(&slot->sequence, pos + 1, memory_order_release);
                atomic_fetch_add_explicit(&chan->count, 1, memory_order_relaxed);
                {
                    Actor* cur = actor_current();
                    trace_record(TRACE_CHAN_SEND, cur ? (uint16_t)cur->id : 0, (uint16_t)chan->id);
                }
                /* Wake actor blocked on recv for this channel */
                int waiter = atomic_exchange_explicit(&chan->recv_waiter, -1, memory_order_acq_rel);
                if (waiter >= 0) channel_wake_actor(waiter);
                return true;
            }
        } else if (diff < 0) {
            /* Queue is full */
            return false;
        } else {
            /* Another producer claimed this slot — reload position */
            pos = atomic_load_explicit(&chan->enqueue_pos, memory_order_relaxed);
        }
    }
}

/* Vyukov MPMC dequeue — 1 CAS per operation */
Cell* channel_try_recv(Channel* chan) {
    if (!chan) return NULL;

    uint64_t pos = atomic_load_explicit(&chan->dequeue_pos, memory_order_relaxed);
    for (;;) {
        VyukovSlot* slot = &chan->buffer[pos & chan->mask];
        uint64_t seq = atomic_load_explicit(&slot->sequence, memory_order_acquire);
        int64_t diff = (int64_t)seq - (int64_t)(pos + 1);

        if (diff == 0) {
            /* Slot has data — try to claim it */
            if (atomic_compare_exchange_weak_explicit(&chan->dequeue_pos, &pos, pos + 1,
                    memory_order_relaxed, memory_order_relaxed)) {
                Cell* value = slot->value;
                slot->value = NULL;
                atomic_store_explicit(&slot->sequence, pos + chan->capacity, memory_order_release);
                atomic_fetch_sub_explicit(&chan->count, 1, memory_order_relaxed);
                {
                    Actor* cur = actor_current();
                    trace_record(TRACE_CHAN_RECV, cur ? (uint16_t)cur->id : 0, (uint16_t)chan->id);
                }
                /* Wake actor blocked on send for this channel */
                int waiter = atomic_exchange_explicit(&chan->send_waiter, -1, memory_order_acq_rel);
                if (waiter >= 0) channel_wake_actor(waiter);
                return value; /* Caller owns the ref */
            }
        } else if (diff < 0) {
            /* Queue is empty */
            return NULL;
        } else {
            /* Another consumer claimed this slot — reload position */
            pos = atomic_load_explicit(&chan->dequeue_pos, memory_order_relaxed);
        }
    }
}

Channel* channel_lookup(int id) {
    for (int i = 0; i < g_channel_count; i++) {
        if (g_channels[i] && g_channels[i]->id == id) {
            return g_channels[i];
        }
    }
    return NULL;
}

void channel_reset_all(void) {
    for (int i = 0; i < g_channel_count; i++) {
        if (g_channels[i]) {
            channel_destroy(g_channels[i]);
            g_channels[i] = NULL;
        }
    }
    g_channel_count = 0;
    g_next_channel_id = 1;
}
