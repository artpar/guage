/* ring.c — Platform-abstracted async I/O ring (Day 126)
 *
 * Backends:
 *   Linux  — io_uring (direct syscalls, no liburing)
 *   macOS  — kqueue (readiness → completion emulation)
 *   Windows — IOCP (not yet — placeholder)
 */

#include "ring.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

/* ════════════════════════════════════════════════════════════
 *  kqueue backend (macOS / BSD)
 * ════════════════════════════════════════════════════════════ */

#if defined(RING_BACKEND_KQUEUE)

/* ── Global buffer ring registry ─────────────────────────── */

#define MAX_BUFRINGS 64
static BufferRing* g_bufrings[MAX_BUFRINGS];
static uint16_t    g_bufring_ids[MAX_BUFRINGS];
static int         g_bufring_count = 0;

void ring_register_bufring(uint16_t group_id, BufferRing* br) {
    if (g_bufring_count < MAX_BUFRINGS) {
        g_bufring_ids[g_bufring_count] = group_id;
        g_bufrings[g_bufring_count] = br;
        g_bufring_count++;
    }
}

BufferRing* ring_lookup_bufring(uint16_t group_id) {
    for (int i = 0; i < g_bufring_count; i++) {
        if (g_bufring_ids[i] == group_id) return g_bufrings[i];
    }
    return NULL;
}

/* ── Lifecycle ────────────────────────────────────────────── */

int ring_init(EventRing* ring, uint32_t sq_entries) {
    memset(ring, 0, sizeof(*ring));
    ring->kq_fd = kqueue();
    if (ring->kq_fd < 0) return -errno;

    ring->sq_entries = sq_entries;
    ring->pending = 0;

    /* Changelist for batching kevent submissions */
    ring->changelist_cap = sq_entries > 64 ? sq_entries : 64;
    ring->changelist = calloc(ring->changelist_cap, sizeof(struct kevent));
    ring->nchanges = 0;

    /* Op tracking hash table (open addressing, 2x oversize) */
    ring->ops_cap = sq_entries * 2;
    if (ring->ops_cap < 128) ring->ops_cap = 128;
    ring->ops = calloc(ring->ops_cap, sizeof(RingKqOp));
    ring->ops_size = 0;

    return 0;
}

void ring_destroy(EventRing* ring) {
    if (ring->kq_fd >= 0) close(ring->kq_fd);
    free(ring->changelist);
    free(ring->ops);
    memset(ring, 0, sizeof(*ring));
    ring->kq_fd = -1;
}

/* ── Op tracking helpers ─────────────────────────────────── */

/* Simple hash for (fd, op_type, user_data) */
static uint32_t op_hash(int fd, uint32_t user_data) {
    uint32_t h = (uint32_t)fd * 2654435761u;
    h ^= user_data * 2246822519u;
    return h;
}

static RingKqOp* op_find_slot(EventRing* ring, int fd, uint32_t user_data) {
    uint32_t h = op_hash(fd, user_data) & (ring->ops_cap - 1);
    for (uint32_t i = 0; i < ring->ops_cap; i++) {
        uint32_t idx = (h + i) & (ring->ops_cap - 1);
        RingKqOp* op = &ring->ops[idx];
        if (!op->active) return op;
        if (op->fd == fd && op->user_data == user_data) return op;
    }
    return NULL; /* full — shouldn't happen with 2x oversize */
}

static RingKqOp* op_lookup(EventRing* ring, int fd, uint32_t user_data) {
    uint32_t h = op_hash(fd, user_data) & (ring->ops_cap - 1);
    for (uint32_t i = 0; i < ring->ops_cap; i++) {
        uint32_t idx = (h + i) & (ring->ops_cap - 1);
        RingKqOp* op = &ring->ops[idx];
        if (!op->active) return NULL;
        if (op->fd == fd && op->user_data == user_data) return op;
    }
    return NULL;
}

/* Find op by fd + filter type (for kevent completion) */
static RingKqOp* op_lookup_by_fd_filter(EventRing* ring, int fd, int16_t filter) {
    /* Linear scan — ops table is small enough */
    for (uint32_t i = 0; i < ring->ops_cap; i++) {
        RingKqOp* op = &ring->ops[i];
        if (!op->active) continue;
        if (op->fd != fd) continue;
        /* Match filter to op type */
        if (filter == EVFILT_READ &&
            (op->op_type == RING_OP_ACCEPT || op->op_type == RING_OP_RECV)) {
            return op;
        }
        if (filter == EVFILT_WRITE &&
            (op->op_type == RING_OP_SEND || op->op_type == RING_OP_SEND_ZC ||
             op->op_type == RING_OP_CONNECT)) {
            return op;
        }
    }
    return NULL;
}

static void op_store(EventRing* ring, int fd, uint32_t user_data, uint8_t op_type) {
    RingKqOp* slot = op_find_slot(ring, fd, user_data);
    if (!slot) return;
    slot->fd = fd;
    slot->user_data = user_data;
    slot->op_type = op_type;
    slot->multishot = false;
    slot->active = true;
    slot->group_id = 0;
    slot->buf = NULL;
    slot->buf_len = 0;
    slot->send_buf = NULL;
    slot->send_len = 0;
    slot->addr = NULL;
    slot->addrlen = 0;
    ring->ops_size++;
}

static void op_remove(RingKqOp* op) {
    op->active = false;
}

/* ── kevent helpers ───────────────────────────────────────── */

static void kq_add_change(EventRing* ring, struct kevent* ev) {
    if (ring->nchanges >= ring->changelist_cap) {
        ring->changelist_cap *= 2;
        ring->changelist = realloc(ring->changelist,
                                   ring->changelist_cap * sizeof(struct kevent));
    }
    ring->changelist[ring->nchanges++] = *ev;
}

/* ── Buffer pool ──────────────────────────────────────────── */

int ring_buf_init(EventRing* ring, BufferRing* br, uint16_t group_id,
                  uint32_t buf_count, uint32_t buf_size) {
    (void)ring;
    memset(br, 0, sizeof(*br));
    br->buf_count = buf_count;
    br->buf_size = buf_size;
    br->group_id = group_id;

    /* Allocate contiguous buffer pool */
    size_t total = (size_t)buf_count * buf_size;
    br->base = malloc(total);
    if (!br->base) return -ENOMEM;
    memset(br->base, 0, total);

    /* Free stack: all buffers initially free */
    br->free_stack = malloc(buf_count * sizeof(uint32_t));
    if (!br->free_stack) { free(br->base); return -ENOMEM; }
    for (uint32_t i = 0; i < buf_count; i++) {
        br->free_stack[i] = buf_count - 1 - i; /* top = 0 */
    }
    br->free_top = buf_count;

    ring_register_bufring(group_id, br);
    return 0;
}

void ring_buf_destroy(BufferRing* br) {
    free(br->base);
    free(br->free_stack);
    memset(br, 0, sizeof(*br));
}

void ring_buf_return(BufferRing* br, uint16_t buf_id) {
    if (buf_id >= br->buf_count) return;
    if (br->free_top < br->buf_count) {
        br->free_stack[br->free_top++] = buf_id;
    }
}

uint8_t* ring_buf_get(BufferRing* br, uint16_t buf_id) {
    if (buf_id >= br->buf_count) return NULL;
    return br->base + (size_t)buf_id * br->buf_size;
}

/* Allocate a buffer from the pool (returns -1 if empty) */
static int32_t buf_alloc(BufferRing* br) {
    if (br->free_top == 0) return -1;
    return (int32_t)br->free_stack[--br->free_top];
}

/* ── Submit operations ────────────────────────────────────── */

int ring_prep_accept(EventRing* ring, int fd, uint32_t user_data, bool multishot) {
    struct kevent ev;
    EV_SET(&ev, fd, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, (void*)(uintptr_t)user_data);
    kq_add_change(ring, &ev);

    op_store(ring, fd, user_data, RING_OP_ACCEPT);
    RingKqOp* op = op_lookup(ring, fd, user_data);
    if (op) op->multishot = multishot;

    ring->pending++;
    return 0;
}

int ring_prep_recv(EventRing* ring, int fd, void* buf, uint32_t len,
                   uint32_t user_data, int flags) {
    (void)flags;
    struct kevent ev;
    EV_SET(&ev, fd, EVFILT_READ, EV_ADD | EV_ONESHOT, 0, 0, (void*)(uintptr_t)user_data);
    kq_add_change(ring, &ev);

    op_store(ring, fd, user_data, RING_OP_RECV);
    RingKqOp* op = op_lookup(ring, fd, user_data);
    if (op) { op->buf = buf; op->buf_len = len; }

    ring->pending++;
    return 0;
}

int ring_prep_recv_provided(EventRing* ring, int fd, uint16_t group_id,
                            uint32_t user_data, bool multishot) {
    struct kevent ev;
    uint16_t flags = EV_ADD | EV_CLEAR;
    if (!multishot) flags |= EV_ONESHOT;
    EV_SET(&ev, fd, EVFILT_READ, flags, 0, 0, (void*)(uintptr_t)user_data);
    kq_add_change(ring, &ev);

    op_store(ring, fd, user_data, RING_OP_RECV);
    RingKqOp* op = op_lookup(ring, fd, user_data);
    if (op) {
        op->group_id = group_id;
        op->multishot = multishot;
    }

    ring->pending++;
    return 0;
}

int ring_prep_send(EventRing* ring, int fd, const void* buf, uint32_t len,
                   uint32_t user_data, int flags) {
    (void)flags;
    struct kevent ev;
    EV_SET(&ev, fd, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, (void*)(uintptr_t)user_data);
    kq_add_change(ring, &ev);

    op_store(ring, fd, user_data, RING_OP_SEND);
    RingKqOp* op = op_lookup(ring, fd, user_data);
    if (op) { op->send_buf = buf; op->send_len = len; }

    ring->pending++;
    return 0;
}

int ring_prep_send_zc(EventRing* ring, int fd, const void* buf, uint32_t len,
                      uint32_t user_data) {
    /* macOS: no zero-copy send, fall back to regular send */
    return ring_prep_send(ring, fd, buf, len, user_data, 0);
}

int ring_prep_connect(EventRing* ring, int fd, const struct sockaddr* addr,
                      socklen_t addrlen, uint32_t user_data) {
    /* Start non-blocking connect, then watch for write readiness */
    int flags = fcntl(fd, F_GETFL, 0);
    if (!(flags & O_NONBLOCK)) fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    int rc = connect(fd, addr, addrlen);
    if (rc == 0) {
        /* Connected immediately — synthesize completion */
        /* Store as pending so ring_complete can return it */
        op_store(ring, fd, user_data, RING_OP_CONNECT);
        RingKqOp* op = op_lookup(ring, fd, user_data);
        if (op) { op->addr = NULL; op->addrlen = 0; } /* mark done */

        struct kevent ev;
        EV_SET(&ev, fd, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, (void*)(uintptr_t)user_data);
        kq_add_change(ring, &ev);
        ring->pending++;
        return 0;
    }
    if (errno != EINPROGRESS) return -errno;

    struct kevent ev;
    EV_SET(&ev, fd, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, (void*)(uintptr_t)user_data);
    kq_add_change(ring, &ev);

    op_store(ring, fd, user_data, RING_OP_CONNECT);
    RingKqOp* op = op_lookup(ring, fd, user_data);
    if (op) { op->addr = addr; op->addrlen = addrlen; }

    ring->pending++;
    return 0;
}

int ring_prep_close(EventRing* ring, int fd, uint32_t user_data) {
    /* Close is synchronous on kqueue — just do it */
    op_store(ring, fd, user_data, RING_OP_CLOSE);
    ring->pending++;

    struct kevent ev;
    EV_SET(&ev, fd, EVFILT_READ, EV_ADD | EV_ONESHOT, 0, 0, (void*)(uintptr_t)user_data);
    kq_add_change(ring, &ev);
    return 0;
}

/* ── Flush submissions ────────────────────────────────────── */

int ring_submit(EventRing* ring) {
    if (ring->nchanges == 0) return 0;

    /* Submit all pending changes (no harvesting here) */
    int rc = kevent(ring->kq_fd, ring->changelist, ring->nchanges, NULL, 0, NULL);
    int submitted = ring->nchanges;
    ring->nchanges = 0;

    if (rc < 0) return -errno;
    return submitted;
}

/* ── Harvest completions ──────────────────────────────────── */

int ring_complete(EventRing* ring, RingCQE* cqes, uint32_t max_cqes,
                  uint32_t wait_min, uint32_t timeout_ms) {
    struct kevent events[64];
    int nevents = max_cqes > 64 ? 64 : (int)max_cqes;

    struct timespec ts;
    struct timespec* tsp = NULL;
    if (timeout_ms > 0 || wait_min == 0) {
        ts.tv_sec = timeout_ms / 1000;
        ts.tv_nsec = (timeout_ms % 1000) * 1000000L;
        tsp = &ts;
    }

    /* Also flush any pending changes while harvesting */
    int rc = kevent(ring->kq_fd,
                    ring->changelist, ring->nchanges,
                    events, nevents, tsp);
    ring->nchanges = 0;

    if (rc < 0) return -errno;

    int count = 0;
    for (int i = 0; i < rc && count < (int)max_cqes; i++) {
        struct kevent* kev = &events[i];
        int fd = (int)kev->ident;
        uint32_t ud = (uint32_t)(uintptr_t)kev->udata;

        /* Find the op metadata */
        RingKqOp* op = op_lookup_by_fd_filter(ring, fd, kev->filter);
        if (!op) continue;

        RingCQE* cqe = &cqes[count];
        cqe->user_data = op->user_data;
        cqe->buffer_id = 0;
        cqe->flags = 0;
        cqe->op_type = op->op_type;
        (void)ud;

        switch (op->op_type) {
        case RING_OP_ACCEPT: {
            struct sockaddr_storage sa;
            socklen_t salen = sizeof(sa);
            int client = accept(fd, (struct sockaddr*)&sa, &salen);
            if (client < 0) {
                cqe->result = -errno;
            } else {
                cqe->result = client;
            }
            if (op->multishot) {
                cqe->flags |= RING_CQE_F_MORE;
                /* Re-arm: EV_CLEAR keeps it active */
            } else {
                op_remove(op);
                ring->pending--;
            }
            count++;
            break;
        }
        case RING_OP_RECV: {
            int32_t bytes;
            uint16_t bid = 0;

            if (op->group_id) {
                /* Provided buffer recv */
                BufferRing* br = ring_lookup_bufring(op->group_id);
                if (!br) { cqe->result = -ENOMEM; count++; break; }
                int32_t slot = buf_alloc(br);
                if (slot < 0) { cqe->result = -ENOMEM; count++; break; }
                bid = (uint16_t)slot;
                uint8_t* ptr = ring_buf_get(br, bid);
                bytes = (int32_t)recv(fd, ptr, br->buf_size, 0);
                cqe->buffer_id = bid;
                cqe->flags |= RING_CQE_F_BUFFER;
            } else if (op->buf) {
                bytes = (int32_t)recv(fd, op->buf, op->buf_len, 0);
            } else {
                bytes = -EINVAL;
            }

            if (bytes < 0) cqe->result = -errno;
            else cqe->result = bytes;

            if (op->multishot && bytes > 0) {
                cqe->flags |= RING_CQE_F_MORE;
            } else {
                op_remove(op);
                ring->pending--;
            }
            count++;
            break;
        }
        case RING_OP_SEND:
        case RING_OP_SEND_ZC: {
            if (!op->send_buf) { cqe->result = -EINVAL; count++; break; }
            ssize_t bytes = send(fd, op->send_buf, op->send_len, 0);
            cqe->result = bytes < 0 ? -errno : (int32_t)bytes;
            op_remove(op);
            ring->pending--;
            count++;
            break;
        }
        case RING_OP_CONNECT: {
            /* Check if connect succeeded */
            int err = 0;
            socklen_t errlen = sizeof(err);
            getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &errlen);
            cqe->result = err ? -err : 0;
            op_remove(op);
            ring->pending--;
            count++;
            break;
        }
        case RING_OP_CLOSE: {
            int ret = close(fd);
            cqe->result = ret < 0 ? -errno : 0;
            op_remove(op);
            ring->pending--;
            count++;
            break;
        }
        default:
            break;
        }
    }

    return count;
}

/* ════════════════════════════════════════════════════════════
 *  io_uring backend (Linux)
 * ════════════════════════════════════════════════════════════ */

#elif defined(RING_BACKEND_IOURING)

/* ── Inline syscall wrappers (no liburing) ────────────────── */

static int io_uring_setup(uint32_t entries, struct io_uring_params* p) {
    return (int)syscall(__NR_io_uring_setup, entries, p);
}

static int io_uring_enter(int fd, uint32_t to_submit, uint32_t min_complete,
                          uint32_t flags, void* sig) {
    return (int)syscall(__NR_io_uring_enter, fd, to_submit, min_complete, flags, sig, 0);
}

static int io_uring_register_fn(int fd, uint32_t opcode, void* arg, uint32_t nr_args) {
    return (int)syscall(__NR_io_uring_register, fd, opcode, arg, nr_args);
}

/* ── SQE helpers ──────────────────────────────────────────── */

static struct io_uring_sqe* get_sqe(EventRing* ring) {
    uint32_t head = __atomic_load_n(ring->sq_head, __ATOMIC_ACQUIRE);
    uint32_t tail = *ring->sq_tail;
    if (tail - head >= ring->sq_entries) return NULL; /* full */
    struct io_uring_sqe* sqe = &((struct io_uring_sqe*)ring->sqes)[tail & ring->sq_mask];
    memset(sqe, 0, sizeof(*sqe));
    return sqe;
}

static void submit_sqe(EventRing* ring) {
    uint32_t tail = *ring->sq_tail;
    ring->sq_array[tail & ring->sq_mask] = tail & ring->sq_mask;
    __atomic_store_n(ring->sq_tail, tail + 1, __ATOMIC_RELEASE);
}

/* ── Lifecycle ────────────────────────────────────────────── */

int ring_init(EventRing* ring, uint32_t sq_entries) {
    memset(ring, 0, sizeof(*ring));

    struct io_uring_params params;
    memset(&params, 0, sizeof(params));

    ring->ring_fd = io_uring_setup(sq_entries, &params);
    if (ring->ring_fd < 0) return -errno;

    ring->sq_entries = params.sq_entries;
    ring->sq_mask = params.sq_entries - 1;
    ring->cq_mask = params.cq_entries - 1;

    /* mmap SQ ring */
    size_t sq_ring_sz = params.sq_off.array + params.sq_entries * sizeof(uint32_t);
    ring->sq_ring = mmap(NULL, sq_ring_sz, PROT_READ | PROT_WRITE,
                         MAP_SHARED | MAP_POPULATE, ring->ring_fd,
                         IORING_OFF_SQ_RING);
    if (ring->sq_ring == MAP_FAILED) { close(ring->ring_fd); return -errno; }

    ring->sq_head = (uint32_t*)((char*)ring->sq_ring + params.sq_off.head);
    ring->sq_tail = (uint32_t*)((char*)ring->sq_ring + params.sq_off.tail);
    ring->sq_array = (uint32_t*)((char*)ring->sq_ring + params.sq_off.array);

    /* mmap CQ ring */
    size_t cq_ring_sz = params.cq_off.cqes + params.cq_entries * sizeof(struct io_uring_cqe);
    ring->cq_ring = mmap(NULL, cq_ring_sz, PROT_READ | PROT_WRITE,
                         MAP_SHARED | MAP_POPULATE, ring->ring_fd,
                         IORING_OFF_CQ_RING);
    if (ring->cq_ring == MAP_FAILED) { close(ring->ring_fd); return -errno; }

    ring->cq_head = (uint32_t*)((char*)ring->cq_ring + params.cq_off.head);
    ring->cq_tail = (uint32_t*)((char*)ring->cq_ring + params.cq_off.tail);
    ring->cqes_ptr = (struct io_uring_cqe*)((char*)ring->cq_ring + params.cq_off.cqes);

    /* mmap SQE array */
    size_t sqes_sz = params.sq_entries * sizeof(struct io_uring_sqe);
    ring->sqes = mmap(NULL, sqes_sz, PROT_READ | PROT_WRITE,
                      MAP_SHARED | MAP_POPULATE, ring->ring_fd,
                      IORING_OFF_SQES);
    if (ring->sqes == MAP_FAILED) { close(ring->ring_fd); return -errno; }

    return 0;
}

void ring_destroy(EventRing* ring) {
    if (ring->ring_fd >= 0) close(ring->ring_fd);
    /* Note: munmap should be called for the mmap'd regions
       but we keep it simple — OS reclaims on process exit */
    memset(ring, 0, sizeof(*ring));
    ring->ring_fd = -1;
}

/* ── Buffer pool ──────────────────────────────────────────── */

int ring_buf_init(EventRing* ring, BufferRing* br, uint16_t group_id,
                  uint32_t buf_count, uint32_t buf_size) {
    memset(br, 0, sizeof(*br));
    br->buf_count = buf_count;
    br->buf_size = buf_size;
    br->group_id = group_id;

    /* Allocate contiguous buffer pool */
    size_t total = (size_t)buf_count * buf_size;
    br->base = mmap(NULL, total, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (br->base == MAP_FAILED) return -errno;

    /* Allocate provided buffer ring (shared with kernel) */
    size_t ring_sz = sizeof(struct io_uring_buf) * buf_count;
    /* Page-align for kernel registration */
    size_t page = 4096;
    ring_sz = (ring_sz + page - 1) & ~(page - 1);
    br->br = mmap(NULL, ring_sz, PROT_READ | PROT_WRITE,
                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (br->br == MAP_FAILED) { munmap(br->base, total); return -errno; }

    /* Fill buffer ring entries */
    struct io_uring_buf_ring* bring = (struct io_uring_buf_ring*)br->br;
    /* Initialize the ring tail to 0 */
    __atomic_store_n(&bring->tail, 0, __ATOMIC_RELEASE);
    for (uint32_t i = 0; i < buf_count; i++) {
        struct io_uring_buf* buf_entry = &bring->bufs[i];
        buf_entry->addr = (uint64_t)(uintptr_t)(br->base + (size_t)i * buf_size);
        buf_entry->len = buf_size;
        buf_entry->bid = i;
    }
    __atomic_store_n(&bring->tail, buf_count, __ATOMIC_RELEASE);

    /* Register with kernel */
    struct io_uring_buf_reg reg;
    memset(&reg, 0, sizeof(reg));
    reg.ring_addr = (uint64_t)(uintptr_t)br->br;
    reg.ring_entries = buf_count;
    reg.bgid = group_id;

    int ret = io_uring_register_fn(ring->ring_fd, IORING_REGISTER_PBUF_RING, &reg, 1);
    if (ret < 0) {
        munmap(br->base, total);
        munmap(br->br, ring_sz);
        return ret;
    }

    return 0;
}

void ring_buf_destroy(BufferRing* br) {
    if (br->base && br->base != MAP_FAILED) {
        munmap(br->base, (size_t)br->buf_count * br->buf_size);
    }
    if (br->br && br->br != MAP_FAILED) {
        size_t ring_sz = sizeof(struct io_uring_buf) * br->buf_count;
        size_t page = 4096;
        ring_sz = (ring_sz + page - 1) & ~(page - 1);
        munmap(br->br, ring_sz);
    }
    memset(br, 0, sizeof(*br));
}

void ring_buf_return(BufferRing* br, uint16_t buf_id) {
    if (!br->br || buf_id >= br->buf_count) return;
    struct io_uring_buf_ring* bring = (struct io_uring_buf_ring*)br->br;
    uint32_t tail = __atomic_load_n(&bring->tail, __ATOMIC_ACQUIRE);
    struct io_uring_buf* entry = &bring->bufs[tail & (br->buf_count - 1)];
    entry->addr = (uint64_t)(uintptr_t)(br->base + (size_t)buf_id * br->buf_size);
    entry->len = br->buf_size;
    entry->bid = buf_id;
    __atomic_store_n(&bring->tail, tail + 1, __ATOMIC_RELEASE);
}

uint8_t* ring_buf_get(BufferRing* br, uint16_t buf_id) {
    if (buf_id >= br->buf_count) return NULL;
    return br->base + (size_t)buf_id * br->buf_size;
}

/* ── Submit operations ────────────────────────────────────── */

int ring_prep_accept(EventRing* ring, int fd, uint32_t user_data, bool multishot) {
    struct io_uring_sqe* sqe = get_sqe(ring);
    if (!sqe) return -ENOSPC;

    sqe->opcode = IORING_OP_ACCEPT;
    sqe->fd = fd;
    sqe->user_data = user_data;
    sqe->addr = 0;
    sqe->addr2 = 0;
    if (multishot) {
        /* IORING_ACCEPT_MULTISHOT = 1 << 0, stored in ioprio for accept */
        sqe->ioprio = 1; /* IORING_ACCEPT_MULTISHOT */
    }

    submit_sqe(ring);
    ring->pending++;
    return 0;
}

int ring_prep_recv(EventRing* ring, int fd, void* buf, uint32_t len,
                   uint32_t user_data, int flags) {
    struct io_uring_sqe* sqe = get_sqe(ring);
    if (!sqe) return -ENOSPC;

    sqe->opcode = IORING_OP_RECV;
    sqe->fd = fd;
    sqe->addr = (uint64_t)(uintptr_t)buf;
    sqe->len = len;
    sqe->user_data = user_data;
    sqe->msg_flags = (uint32_t)flags;

    submit_sqe(ring);
    ring->pending++;
    return 0;
}

int ring_prep_recv_provided(EventRing* ring, int fd, uint16_t group_id,
                            uint32_t user_data, bool multishot) {
    struct io_uring_sqe* sqe = get_sqe(ring);
    if (!sqe) return -ENOSPC;

    sqe->opcode = IORING_OP_RECV;
    sqe->fd = fd;
    sqe->len = 0; /* kernel picks from provided buffer */
    sqe->user_data = user_data;
    sqe->buf_group = group_id;
    sqe->flags = IOSQE_BUFFER_SELECT;
    if (multishot) {
        /* Use multishot recv if available (kernel 6.0+) */
        sqe->ioprio |= 1; /* IORING_RECV_MULTISHOT */
    }

    submit_sqe(ring);
    ring->pending++;
    return 0;
}

int ring_prep_send(EventRing* ring, int fd, const void* buf, uint32_t len,
                   uint32_t user_data, int flags) {
    struct io_uring_sqe* sqe = get_sqe(ring);
    if (!sqe) return -ENOSPC;

    sqe->opcode = IORING_OP_SEND;
    sqe->fd = fd;
    sqe->addr = (uint64_t)(uintptr_t)buf;
    sqe->len = len;
    sqe->user_data = user_data;
    sqe->msg_flags = (uint32_t)flags;

    submit_sqe(ring);
    ring->pending++;
    return 0;
}

int ring_prep_send_zc(EventRing* ring, int fd, const void* buf, uint32_t len,
                      uint32_t user_data) {
    struct io_uring_sqe* sqe = get_sqe(ring);
    if (!sqe) return -ENOSPC;

    sqe->opcode = IORING_OP_SEND_ZC;
    sqe->fd = fd;
    sqe->addr = (uint64_t)(uintptr_t)buf;
    sqe->len = len;
    sqe->user_data = user_data;

    submit_sqe(ring);
    ring->pending++;
    return 0;
}

int ring_prep_connect(EventRing* ring, int fd, const struct sockaddr* addr,
                      socklen_t addrlen, uint32_t user_data) {
    struct io_uring_sqe* sqe = get_sqe(ring);
    if (!sqe) return -ENOSPC;

    sqe->opcode = IORING_OP_CONNECT;
    sqe->fd = fd;
    sqe->addr = (uint64_t)(uintptr_t)addr;
    sqe->off = addrlen;
    sqe->user_data = user_data;

    submit_sqe(ring);
    ring->pending++;
    return 0;
}

int ring_prep_close(EventRing* ring, int fd, uint32_t user_data) {
    struct io_uring_sqe* sqe = get_sqe(ring);
    if (!sqe) return -ENOSPC;

    sqe->opcode = IORING_OP_CLOSE;
    sqe->fd = fd;
    sqe->user_data = user_data;

    submit_sqe(ring);
    ring->pending++;
    return 0;
}

/* ── Flush submissions ────────────────────────────────────── */

int ring_submit(EventRing* ring) {
    int ret = io_uring_enter(ring->ring_fd, ring->pending, 0, 0, NULL);
    if (ret < 0) return -errno;
    return ret;
}

/* ── Harvest completions ──────────────────────────────────── */

int ring_complete(EventRing* ring, RingCQE* cqes, uint32_t max_cqes,
                  uint32_t wait_min, uint32_t timeout_ms) {
    (void)timeout_ms; /* TODO: use IORING_ENTER_EXT_ARG for timeout */

    if (wait_min > 0) {
        int ret = io_uring_enter(ring->ring_fd, 0, wait_min,
                                 IORING_ENTER_GETEVENTS, NULL);
        if (ret < 0) return -errno;
    }

    uint32_t head = __atomic_load_n(ring->cq_head, __ATOMIC_ACQUIRE);
    uint32_t tail = __atomic_load_n(ring->cq_tail, __ATOMIC_ACQUIRE);
    uint32_t count = 0;

    while (head != tail && count < max_cqes) {
        struct io_uring_cqe* cqe = &ring->cqes_ptr[head & ring->cq_mask];

        cqes[count].result = cqe->res;
        cqes[count].user_data = (uint32_t)cqe->user_data;
        cqes[count].flags = 0;
        cqes[count].buffer_id = 0;
        cqes[count].op_type = 0; /* io_uring doesn't track op type in CQE */

        if (cqe->flags & IORING_CQE_F_MORE)
            cqes[count].flags |= RING_CQE_F_MORE;
        if (cqe->flags & IORING_CQE_F_BUFFER) {
            cqes[count].flags |= RING_CQE_F_BUFFER;
            cqes[count].buffer_id = cqe->flags >> IORING_CQE_BUFFER_SHIFT;
        }

        head++;
        count++;
    }

    __atomic_store_n(ring->cq_head, head, __ATOMIC_RELEASE);

    if (count > 0 && ring->pending >= count)
        ring->pending -= count;

    return (int)count;
}

/* ════════════════════════════════════════════════════════════
 *  IOCP backend (Windows) — placeholder
 * ════════════════════════════════════════════════════════════ */

#elif defined(RING_BACKEND_IOCP)

/* TODO: Windows IOCP backend */

int ring_init(EventRing* ring, uint32_t sq_entries) {
    (void)ring; (void)sq_entries;
    return -1; /* Not yet implemented */
}

void ring_destroy(EventRing* ring) { (void)ring; }

int ring_buf_init(EventRing* ring, BufferRing* br, uint16_t group_id,
                  uint32_t buf_count, uint32_t buf_size) {
    (void)ring; (void)br; (void)group_id; (void)buf_count; (void)buf_size;
    return -1;
}
void ring_buf_destroy(BufferRing* br) { (void)br; }
void ring_buf_return(BufferRing* br, uint16_t buf_id) { (void)br; (void)buf_id; }
uint8_t* ring_buf_get(BufferRing* br, uint16_t buf_id) { (void)br; (void)buf_id; return NULL; }

int ring_prep_accept(EventRing* r, int fd, uint32_t ud, bool ms) {
    (void)r;(void)fd;(void)ud;(void)ms; return -1; }
int ring_prep_recv(EventRing* r, int fd, void* b, uint32_t l, uint32_t ud, int f) {
    (void)r;(void)fd;(void)b;(void)l;(void)ud;(void)f; return -1; }
int ring_prep_recv_provided(EventRing* r, int fd, uint16_t g, uint32_t ud, bool ms) {
    (void)r;(void)fd;(void)g;(void)ud;(void)ms; return -1; }
int ring_prep_send(EventRing* r, int fd, const void* b, uint32_t l, uint32_t ud, int f) {
    (void)r;(void)fd;(void)b;(void)l;(void)ud;(void)f; return -1; }
int ring_prep_send_zc(EventRing* r, int fd, const void* b, uint32_t l, uint32_t ud) {
    (void)r;(void)fd;(void)b;(void)l;(void)ud; return -1; }
int ring_prep_connect(EventRing* r, int fd, const struct sockaddr* a, socklen_t al, uint32_t ud) {
    (void)r;(void)fd;(void)a;(void)al;(void)ud; return -1; }
int ring_prep_close(EventRing* r, int fd, uint32_t ud) {
    (void)r;(void)fd;(void)ud; return -1; }
int ring_submit(EventRing* r) { (void)r; return -1; }
int ring_complete(EventRing* r, RingCQE* c, uint32_t m, uint32_t w, uint32_t t) {
    (void)r;(void)c;(void)m;(void)w;(void)t; return -1; }

#endif /* backend selection */
