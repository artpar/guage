/* fcontext_arm64.S — ARM64 (AArch64) context switch for Guage fibers
 *
 * Context frame: 192 bytes (0xC0), 16-byte aligned
 *   0x00-0x3F: d8-d15   (8 NEON callee-saved, 64 bytes)
 *   0x40-0x8F: x19-x28  (10 GPR callee-saved, 80 bytes)
 *   0x90-0x9F: fp, lr    (16 bytes)
 *   0xA0-0xA7: FPCR      (8 bytes — Boost.Context omits this, we don't)
 *   0xA8-0xBF: padding    (24 bytes for 16-byte alignment)
 *
 * ~14 instructions per switch, ~4-20ns on Apple Silicon
 * vs ~600ns for ucontext (sigprocmask syscall)
 */

.text
.p2align 2

/* ── fctx_jump ──────────────────────────────────────────────────────
 * fctx_transfer_t fctx_jump(fcontext_t to, void* data)
 *   x0 = to (target sp)
 *   x1 = data (passthrough)
 *   Returns {from_ctx, data} in x0, x1 (AAPCS64 2-ptr struct) */

#if defined(__APPLE__)
.globl _fctx_jump
_fctx_jump:
#else
.globl fctx_jump
.type fctx_jump, %function
fctx_jump:
#endif
    /* Save context to current stack */
    sub  sp,  sp, #0xC0
    stp  d8,  d9,  [sp, #0x00]
    stp  d10, d11, [sp, #0x10]
    stp  d12, d13, [sp, #0x20]
    stp  d14, d15, [sp, #0x30]
    stp  x19, x20, [sp, #0x40]
    stp  x21, x22, [sp, #0x50]
    stp  x23, x24, [sp, #0x60]
    stp  x25, x26, [sp, #0x70]
    stp  x27, x28, [sp, #0x80]
    stp  fp,  lr,  [sp, #0x90]
    mrs  x9,  fpcr
    str  x9,  [sp, #0xA0]

    /* x2 = our saved sp (from context) */
    mov  x2,  sp

    /* Switch to target stack */
    mov  sp,  x0

    /* Restore context from target stack */
    ldp  d8,  d9,  [sp, #0x00]
    ldp  d10, d11, [sp, #0x10]
    ldp  d12, d13, [sp, #0x20]
    ldp  d14, d15, [sp, #0x30]
    ldp  x19, x20, [sp, #0x40]
    ldp  x21, x22, [sp, #0x50]
    ldp  x23, x24, [sp, #0x60]
    ldp  x25, x26, [sp, #0x70]
    ldp  x27, x28, [sp, #0x80]
    ldp  fp,  lr,  [sp, #0x90]
    ldr  x9,  [sp, #0xA0]
    msr  fpcr, x9
    add  sp,  sp, #0xC0

    /* Return transfer_t { .ctx = from, .data = data } */
    mov  x0,  x2
    /* x1 = data (untouched) */
    ret

#ifndef __APPLE__
.size fctx_jump, . - fctx_jump
#endif

/* ── fctx_make ──────────────────────────────────────────────────────
 * fcontext_t fctx_make(void* stack_top, size_t size, void(*fn)(fctx_transfer_t))
 *   x0 = stack_top (highest address), x1 = size (unused), x2 = fn
 *   Returns prepared fcontext_t in x0 */

#if defined(__APPLE__)
.globl _fctx_make
_fctx_make:
#else
.globl fctx_make
.type fctx_make, %function
fctx_make:
#endif
    /* Align down to 16 bytes */
    and  x0,  x0, #~0xF

    /* Allocate context frame */
    sub  x0,  x0, #0xC0

    /* Zero NEON callee-saved (d8-d15) */
    stp  xzr, xzr, [x0, #0x00]
    stp  xzr, xzr, [x0, #0x10]
    stp  xzr, xzr, [x0, #0x20]
    stp  xzr, xzr, [x0, #0x30]

    /* x19 = fn (restored after fctx_jump, available in trampoline) */
    str  x2,  [x0, #0x40]
    /* Zero x20-x28 */
    str  xzr, [x0, #0x48]
    stp  xzr, xzr, [x0, #0x50]
    stp  xzr, xzr, [x0, #0x60]
    stp  xzr, xzr, [x0, #0x70]
    stp  xzr, xzr, [x0, #0x80]

    /* fp = 0 (end of frame chain), lr = trampoline */
#if defined(__APPLE__)
    adr  x3,  Lfctx_trampoline
#else
    adr  x3,  .Lfctx_trampoline
#endif
    stp  xzr, x3,  [x0, #0x90]

    /* Zero FPCR + padding */
    str  xzr, [x0, #0xA0]
    stp  xzr, xzr, [x0, #0xA8]

    /* Return prepared context (sp value) */
    ret

#ifndef __APPLE__
.size fctx_make, . - fctx_make
#endif

/* ── Trampoline ─────────────────────────────────────────────────────
 * Entered via fctx_jump into a fresh context.
 *   x0 = from context (caller's saved sp)
 *   x1 = data (void* passthrough)
 *   x19 = fn (restored from saved frame)
 * Calls fn({x0, x1}) — fn must fctx_jump out, never return. */

#if defined(__APPLE__)
Lfctx_trampoline:
#else
.Lfctx_trampoline:
#endif
    blr  x19
    brk  #1     /* fn must never return — trap */
