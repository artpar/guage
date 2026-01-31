/* ring.h — Platform-abstracted async I/O ring (Day 126)
 *
 * io_uring (Linux 6.0+) | IOCP (Windows) | kqueue (macOS/BSD)
 *
 * Zero-copy sockets, multishot recv, provided buffer rings,
 * batch submit/complete — HFT-grade networking primitives.
 */

#ifndef GUAGE_RING_H
#define GUAGE_RING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ── Platform detection ───────────────────────────────────── */

#if defined(__linux__)
#  define RING_BACKEND_IOURING 1
#  include <sys/syscall.h>
#  include <linux/io_uring.h>
#  include <sys/mman.h>
#  include <unistd.h>
#  include <errno.h>
#  include <string.h>
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  include <sys/un.h>
#  include <fcntl.h>
#  include <netdb.h>
#elif defined(_WIN32)
#  define RING_BACKEND_IOCP 1
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  include <mswsock.h>
#  pragma comment(lib, "ws2_32.lib")
#else
#  define RING_BACKEND_KQUEUE 1
#  include <sys/event.h>
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <netinet/tcp.h>
#  include <arpa/inet.h>
#  include <sys/un.h>
#  include <unistd.h>
#  include <fcntl.h>
#  include <errno.h>
#  include <string.h>
#  include <netdb.h>
#  include <sys/time.h>
#endif

/* ── Completion flags ─────────────────────────────────────── */

enum {
    RING_CQE_F_MORE   = 0x01,    /* multishot: more completions coming */
    RING_CQE_F_BUFFER = 0x02,    /* buffer_id is valid */
};

/* ── Operation types ──────────────────────────────────────── */

enum RingOp {
    RING_OP_ACCEPT  = 0,
    RING_OP_RECV    = 1,
    RING_OP_SEND    = 2,
    RING_OP_CONNECT = 3,
    RING_OP_CLOSE   = 4,
    RING_OP_RECV_ZC = 5,
    RING_OP_SEND_ZC = 6,
};

/* ── Unified completion event ─────────────────────────────── */

typedef struct {
    int32_t  result;        /* bytes transferred or error */
    uint32_t user_data;     /* operation tag */
    uint16_t buffer_id;     /* provided buffer ID (if applicable) */
    uint8_t  flags;         /* RING_CQE_F_MORE, RING_CQE_F_BUFFER */
    uint8_t  op_type;       /* RingOp enum */
} RingCQE;

/* ── Event ring ───────────────────────────────────────────── */

typedef struct {
#if defined(RING_BACKEND_IOURING)
    int       ring_fd;      /* io_uring fd from io_uring_setup() */
    void*     sq_ring;      /* mmap'd SQ ring */
    void*     cq_ring;      /* mmap'd CQ ring */
    void*     sqes;         /* mmap'd SQE array */
    uint32_t  sq_mask;
    uint32_t  cq_mask;
    uint32_t* sq_head;
    uint32_t* sq_tail;
    uint32_t* sq_array;
    uint32_t* cq_head;
    uint32_t* cq_tail;
    struct io_uring_cqe* cqes_ptr;
#elif defined(RING_BACKEND_IOCP)
    void*     iocp;         /* HANDLE — I/O Completion Port */
#else /* kqueue */
    int       kq_fd;        /* kqueue fd */
    /* Pending changelist for batch submit */
    struct kevent* changelist;
    int            nchanges;
    int            changelist_cap;
    /* Track op metadata for readiness→completion emulation */
    struct RingKqOp* ops;   /* Hash table of pending ops */
    uint32_t         ops_cap;
    uint32_t         ops_size;
#endif
    uint32_t sq_entries;    /* submission queue size (power of 2) */
    uint32_t pending;       /* in-flight operations */
} EventRing;

/* ── kqueue op tracking (macOS only) ─────────────────────── */

#if defined(RING_BACKEND_KQUEUE)

/* Per-operation metadata for kqueue readiness→completion emulation */
typedef struct RingKqOp {
    int          fd;
    uint32_t     user_data;
    uint8_t      op_type;    /* RingOp enum */
    uint16_t     group_id;   /* buffer pool group (for recv_provided) */
    bool         multishot;
    bool         active;
    /* For recv with user buffer */
    void*        buf;
    uint32_t     buf_len;
    /* For send */
    const void*  send_buf;
    uint32_t     send_len;
    /* For connect */
    const struct sockaddr* addr;
    socklen_t    addrlen;
} RingKqOp;

#endif /* RING_BACKEND_KQUEUE */

/* ── Buffer ring (provided buffers) ───────────────────────── */

typedef struct {
    uint8_t*  base;         /* mmap'd / malloc'd buffer pool base */
    uint32_t  buf_count;    /* number of buffers */
    uint32_t  buf_size;     /* bytes per buffer */
    uint16_t  group_id;     /* buffer group ID */
#if defined(RING_BACKEND_IOURING)
    void*     br;           /* struct io_uring_buf_ring* (kernel-shared) */
#else
    uint32_t* free_stack;   /* free buffer index stack */
    uint32_t  free_top;     /* stack pointer */
#endif
} BufferRing;

/* ── Lifecycle ────────────────────────────────────────────── */

int  ring_init(EventRing* ring, uint32_t sq_entries);
void ring_destroy(EventRing* ring);

/* ── Buffer pool ──────────────────────────────────────────── */

int      ring_buf_init(EventRing* ring, BufferRing* br, uint16_t group_id,
                       uint32_t buf_count, uint32_t buf_size);
void     ring_buf_destroy(BufferRing* br);
void     ring_buf_return(BufferRing* br, uint16_t buf_id);
uint8_t* ring_buf_get(BufferRing* br, uint16_t buf_id);

/* ── Submit operations (returns 0 on success, -errno on failure) ── */

int ring_prep_accept(EventRing* ring, int fd, uint32_t user_data, bool multishot);
int ring_prep_recv(EventRing* ring, int fd, void* buf, uint32_t len,
                   uint32_t user_data, int flags);
int ring_prep_recv_provided(EventRing* ring, int fd, uint16_t group_id,
                            uint32_t user_data, bool multishot);
int ring_prep_send(EventRing* ring, int fd, const void* buf, uint32_t len,
                   uint32_t user_data, int flags);
int ring_prep_send_zc(EventRing* ring, int fd, const void* buf, uint32_t len,
                      uint32_t user_data);
int ring_prep_connect(EventRing* ring, int fd, const struct sockaddr* addr,
                      socklen_t addrlen, uint32_t user_data);
int ring_prep_close(EventRing* ring, int fd, uint32_t user_data);

/* ── Flush submissions to kernel ──────────────────────────── */

int ring_submit(EventRing* ring);

/* ── Harvest completions ──────────────────────────────────── */

int ring_complete(EventRing* ring, RingCQE* cqes, uint32_t max_cqes,
                  uint32_t wait_min, uint32_t timeout_ms);

/* ── Global buffer ring registry (for kqueue recv_provided) ─ */

#if defined(RING_BACKEND_KQUEUE)
void ring_register_bufring(uint16_t group_id, BufferRing* br);
BufferRing* ring_lookup_bufring(uint16_t group_id);
#endif

#endif /* GUAGE_RING_H */
