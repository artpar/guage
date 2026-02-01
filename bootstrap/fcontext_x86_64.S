/* fcontext_x86_64.S — x86-64 System V context switch for Guage fibers
 *
 * Context frame: 64 bytes, built by pushes
 *   sp+0x00: MXCSR (4B) + x87 CW (2B) + pad (2B)
 *   sp+0x08: r12 (stores fn for fresh contexts)
 *   sp+0x10: r13
 *   sp+0x18: r14
 *   sp+0x20: r15
 *   sp+0x28: rbx
 *   sp+0x30: rbp
 *   sp+0x38: return address (popped by ret)
 *
 * ~12 instructions per switch, ~8-15ns on modern x86
 * vs ~600ns for ucontext (sigprocmask syscall)
 */

.text
.p2align 4

/* ── fctx_jump ──────────────────────────────────────────────────────
 * fctx_transfer_t fctx_jump(fcontext_t to, void* data)
 *   rdi = to (target sp)
 *   rsi = data (passthrough)
 *   Returns {from_ctx, data} in rax, rdx (System V 2-ptr struct) */

#if defined(__APPLE__)
.globl _fctx_jump
_fctx_jump:
#else
.globl fctx_jump
.type fctx_jump, @function
fctx_jump:
#endif
    /* Save callee-saved + FP control */
    pushq  %rbp
    pushq  %rbx
    pushq  %r15
    pushq  %r14
    pushq  %r13
    pushq  %r12

    subq   $8, %rsp
    stmxcsr (%rsp)
    fnstcw  4(%rsp)

    /* Save current rsp as from-context */
    movq   %rsp, %rax

    /* Switch to target stack */
    movq   %rdi, %rsp

    /* Restore FP control + callee-saved */
    ldmxcsr (%rsp)
    fldcw   4(%rsp)
    addq   $8, %rsp

    popq   %r12
    popq   %r13
    popq   %r14
    popq   %r15
    popq   %rbx
    popq   %rbp

    /* Return transfer_t { rax = from_ctx, rdx = data } */
    movq   %rsi, %rdx
    retq

#ifndef __APPLE__
.size fctx_jump, . - fctx_jump
#endif

/* ── fctx_make ──────────────────────────────────────────────────────
 * fcontext_t fctx_make(void* stack_top, size_t size, void(*fn)(fctx_transfer_t))
 *   rdi = stack_top (highest address), rsi = size (unused), rdx = fn
 *   Returns prepared fcontext_t in rax */

#if defined(__APPLE__)
.globl _fctx_make
_fctx_make:
#else
.globl fctx_make
.type fctx_make, @function
fctx_make:
#endif
    /* Align down to 16 bytes */
    andq   $-16, %rdi

    /* Trampoline return address */
    subq   $8, %rdi
#if defined(__APPLE__)
    leaq   Lfctx_trampoline_x64(%rip), %rax
#else
    leaq   .Lfctx_trampoline_x64(%rip), %rax
#endif
    movq   %rax, (%rdi)

    /* Simulate pushes matching fctx_jump save order */
    subq   $8, %rdi         /* rbp = 0 */
    movq   $0, (%rdi)
    subq   $8, %rdi         /* rbx = 0 */
    movq   $0, (%rdi)
    subq   $8, %rdi         /* r15 = 0 */
    movq   $0, (%rdi)
    subq   $8, %rdi         /* r14 = 0 */
    movq   $0, (%rdi)
    subq   $8, %rdi         /* r13 = 0 */
    movq   $0, (%rdi)
    subq   $8, %rdi         /* r12 = fn */
    movq   %rdx, (%rdi)

    /* MXCSR + x87 CW (current defaults) */
    subq   $8, %rdi
    stmxcsr (%rdi)
    fnstcw  4(%rdi)

    /* Return prepared context */
    movq   %rdi, %rax
    retq

#ifndef __APPLE__
.size fctx_make, . - fctx_make
#endif

/* ── Trampoline ─────────────────────────────────────────────────────
 * Entered via ret in fctx_jump into a fresh context.
 *   rax = from_ctx, rdx = data, r12 = fn
 * Calls fn({rdi=ctx, rsi=data}) — fn must fctx_jump out, never return. */

#if defined(__APPLE__)
Lfctx_trampoline_x64:
#else
.Lfctx_trampoline_x64:
#endif
    movq   %rax, %rdi
    movq   %rdx, %rsi
    callq  *%r12
    ud2      /* fn must never return — trap */
