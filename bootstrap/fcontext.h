/* fcontext.h — Portable fast context switch API
 *
 * Replaces ucontext (which costs ~600ns due to sigprocmask syscall)
 * with custom assembly (~4-20ns, no syscall, saves FPCR).
 *
 * ARM64: saves d8-d15, x19-x28, fp, lr, FPCR (192B frame)
 * x86-64: saves rbx, rbp, r12-r15, MXCSR, x87 CW (64B frame)
 */
#ifndef GUAGE_FCONTEXT_H
#define GUAGE_FCONTEXT_H

#include <stddef.h>

/* An fcontext_t is an opaque saved stack pointer */
typedef void* fcontext_t;

/* Transfer record: returned by fctx_jump, received by new contexts */
typedef struct {
    fcontext_t ctx;   /* The "from" context (caller's saved sp) */
    void*      data;  /* Passthrough data pointer */
} fctx_transfer_t;

/* Jump from current context to 'to', passing 'data'.
 * Saves current registers to stack, switches sp to 'to', restores registers.
 * Returns when someone jumps back to us. */
fctx_transfer_t fctx_jump(fcontext_t to, void* data);

/* Create a new context on 'stack_top' (highest address, stack grows down).
 * When jumped to via fctx_jump, calls fn(transfer) where transfer contains
 * the caller's context and data pointer.
 * fn must never return — it must fctx_jump out. */
fcontext_t fctx_make(void* stack_top, size_t size, void (*fn)(fctx_transfer_t));

#endif /* GUAGE_FCONTEXT_H */
