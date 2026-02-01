#if defined(__aarch64__) || defined(_M_ARM64)

#include "ffi_jit.h"
#include <string.h>

/*
 * ARM64 AAPCS64 JIT stub emitter
 *
 * Generated stub signature: Cell* stub(Cell* args)
 *
 * Input: X0 = cons-list of Cell* args
 * Output: X0 = Cell* result
 *
 * Frame layout (grows downward):
 *   [FP+0]  = saved FP
 *   [FP+8]  = saved LR
 *   [FP-8]  = saved X19
 *   [FP-16] = saved X20
 *   [FP-24] = saved X21
 *   [FP-32] = saved X22
 *   [FP-40..FP-96]  = double scratch (8 slots)
 *   [FP-104..FP-160] = int scratch (8 slots)
 *
 * Cell offsets (with BiasedRC):
 *   +0:  type (uint32_t)
 *   +40: data union start / pair.car / atom.number / atom.builtin
 *   +48: pair.cdr
 */
#define CELL_OFF_DATA 40
#define CELL_OFF_CDR  48

static void emit_inst(EmitCtx* ctx, uint32_t inst) {
    emit_u32(ctx, inst);
}

/* === Instruction encodings === */

/* STP Xt1, Xt2, [Xn, #offset]! (pre-indexed, 64-bit) */
static void emit_stp_pre64(EmitCtx* ctx, int rt1, int rt2, int rn, int offset) {
    /* opc=10, V=0, L=0, imm7=offset/8, Rt2, Rn, Rt1 */
    int imm7 = (offset / 8) & 0x7F;
    uint32_t inst = 0xA9800000
        | ((uint32_t)imm7 << 15)
        | ((uint32_t)(rt2 & 0x1F) << 10)
        | ((uint32_t)(rn & 0x1F) << 5)
        | (uint32_t)(rt1 & 0x1F);
    emit_inst(ctx, inst);
}

/* LDP Xt1, Xt2, [Xn], #offset (post-indexed, 64-bit) */
static void emit_ldp_post64(EmitCtx* ctx, int rt1, int rt2, int rn, int offset) {
    int imm7 = (offset / 8) & 0x7F;
    uint32_t inst = 0xA8C00000
        | ((uint32_t)imm7 << 15)
        | ((uint32_t)(rt2 & 0x1F) << 10)
        | ((uint32_t)(rn & 0x1F) << 5)
        | (uint32_t)(rt1 & 0x1F);
    emit_inst(ctx, inst);
}

/* STP Xt1, Xt2, [Xn, #offset] (signed offset, 64-bit, no writeback) */
static void emit_stp_off64(EmitCtx* ctx, int rt1, int rt2, int rn, int offset) {
    int imm7 = (offset / 8) & 0x7F;
    uint32_t inst = 0xA9000000
        | ((uint32_t)imm7 << 15)
        | ((uint32_t)(rt2 & 0x1F) << 10)
        | ((uint32_t)(rn & 0x1F) << 5)
        | (uint32_t)(rt1 & 0x1F);
    emit_inst(ctx, inst);
}

/* LDP Xt1, Xt2, [Xn, #offset] (signed offset, 64-bit, no writeback) */
static void emit_ldp_off64(EmitCtx* ctx, int rt1, int rt2, int rn, int offset) {
    int imm7 = (offset / 8) & 0x7F;
    uint32_t inst = 0xA9400000
        | ((uint32_t)imm7 << 15)
        | ((uint32_t)(rt2 & 0x1F) << 10)
        | ((uint32_t)(rn & 0x1F) << 5)
        | (uint32_t)(rt1 & 0x1F);
    emit_inst(ctx, inst);
}

/* STR Xt, [Xn, #imm] (unsigned offset, imm is offset/8) */
static void emit_str_x(EmitCtx* ctx, int rt, int rn, int offset) {
    uint32_t imm12 = ((uint32_t)offset / 8) & 0xFFF;
    uint32_t inst = 0xF9000000 | (imm12 << 10) | ((uint32_t)(rn & 0x1F) << 5) | (uint32_t)(rt & 0x1F);
    emit_inst(ctx, inst);
}

/* LDR Xt, [Xn, #imm] */
static void emit_ldr_x(EmitCtx* ctx, int rt, int rn, int offset) {
    uint32_t imm12 = ((uint32_t)offset / 8) & 0xFFF;
    uint32_t inst = 0xF9400000 | (imm12 << 10) | ((uint32_t)(rn & 0x1F) << 5) | (uint32_t)(rt & 0x1F);
    emit_inst(ctx, inst);
}

/* LDR Wt, [Xn, #imm] (32-bit load, unsigned offset, imm is offset/4) */
static void emit_ldr_w(EmitCtx* ctx, int rt, int rn, int offset) {
    uint32_t imm12 = ((uint32_t)offset / 4) & 0xFFF;
    uint32_t inst = 0xB9400000 | (imm12 << 10) | ((uint32_t)(rn & 0x1F) << 5) | (uint32_t)(rt & 0x1F);
    emit_inst(ctx, inst);
}

/* LDR Dt, [Xn, #imm] (64-bit FP load) */
static void emit_ldr_d(EmitCtx* ctx, int dt, int rn, int offset) {
    uint32_t imm12 = ((uint32_t)offset / 8) & 0xFFF;
    uint32_t inst = 0xFD400000 | (imm12 << 10) | ((uint32_t)(rn & 0x1F) << 5) | (uint32_t)(dt & 0x1F);
    emit_inst(ctx, inst);
}

/* STR Dt, [Xn, #imm] (64-bit FP store) */
static void emit_str_d(EmitCtx* ctx, int dt, int rn, int offset) {
    uint32_t imm12 = ((uint32_t)offset / 8) & 0xFFF;
    uint32_t inst = 0xFD000000 | (imm12 << 10) | ((uint32_t)(rn & 0x1F) << 5) | (uint32_t)(dt & 0x1F);
    emit_inst(ctx, inst);
}

/* MOV Xd, Xn (via ORR Xd, XZR, Xn) */
static void emit_mov_x(EmitCtx* ctx, int rd, int rn) {
    uint32_t inst = 0xAA0003E0 | ((uint32_t)(rn & 0x1F) << 16) | (uint32_t)(rd & 0x1F);
    emit_inst(ctx, inst);
}

/* MOV Xd, SP (ADD Xd, SP, #0) — special because SP is not XZR in this context */
static void emit_mov_from_sp(EmitCtx* ctx, int rd) {
    /* ADD Xd, SP, #0 */
    uint32_t inst = 0x910003E0 | (uint32_t)(rd & 0x1F);
    emit_inst(ctx, inst);
}

/* MOV SP, Xn (ADD SP, Xn, #0) */
static void emit_mov_to_sp(EmitCtx* ctx, int rn) {
    uint32_t inst = 0x9100001F | ((uint32_t)(rn & 0x1F) << 5);
    emit_inst(ctx, inst);
}

/* SUB SP, SP, #imm12 */
static void emit_sub_sp(EmitCtx* ctx, int imm) {
    uint32_t inst = 0xD10003FF | (((uint32_t)imm & 0xFFF) << 10);
    emit_inst(ctx, inst);
}

/* ADD SP, SP, #imm12 */
static void emit_add_sp(EmitCtx* ctx, int imm) {
    uint32_t inst = 0x910003FF | (((uint32_t)imm & 0xFFF) << 10);
    emit_inst(ctx, inst);
}

/* CMP Wn, #imm12 (SUBS WZR, Wn, #imm12) */
static void emit_cmp_w_imm(EmitCtx* ctx, int rn, int imm) {
    uint32_t inst = 0x7100001F | (((uint32_t)imm & 0xFFF) << 10) | ((uint32_t)(rn & 0x1F) << 5);
    emit_inst(ctx, inst);
}

/* B.NE — returns offset for patching */
static size_t emit_bne(EmitCtx* ctx) {
    size_t off = ctx->pos;
    emit_inst(ctx, 0x54000001); /* b.ne +0 (placeholder) */
    return off;
}

/* Patch a conditional branch */
static void patch_bcond(EmitCtx* ctx, size_t off) {
    int32_t delta = (int32_t)(ctx->pos - off) / 4;
    uint32_t imm19 = (uint32_t)delta & 0x7FFFF;
    uint32_t inst = 0x54000001 | (imm19 << 5); /* b.ne */
    memcpy(&ctx->buf[off], &inst, 4);
}

/* MOVZ Xd, #imm16, LSL #shift */
static void emit_movz(EmitCtx* ctx, int rd, uint16_t imm16, int shift) {
    uint32_t hw = (uint32_t)(shift / 16) & 3;
    uint32_t inst = 0xD2800000 | (hw << 21) | ((uint32_t)imm16 << 5) | (uint32_t)(rd & 0x1F);
    emit_inst(ctx, inst);
}

/* MOVK Xd, #imm16, LSL #shift */
static void emit_movk(EmitCtx* ctx, int rd, uint16_t imm16, int shift) {
    uint32_t hw = (uint32_t)(shift / 16) & 3;
    uint32_t inst = 0xF2800000 | (hw << 21) | ((uint32_t)imm16 << 5) | (uint32_t)(rd & 0x1F);
    emit_inst(ctx, inst);
}

/* Load 64-bit immediate into register */
static void emit_imm64(EmitCtx* ctx, int rd, uint64_t imm) {
    emit_movz(ctx, rd, (uint16_t)(imm & 0xFFFF), 0);
    if ((imm >> 16) & 0xFFFF)
        emit_movk(ctx, rd, (uint16_t)((imm >> 16) & 0xFFFF), 16);
    if ((imm >> 32) & 0xFFFF)
        emit_movk(ctx, rd, (uint16_t)((imm >> 32) & 0xFFFF), 32);
    if ((imm >> 48) & 0xFFFF)
        emit_movk(ctx, rd, (uint16_t)((imm >> 48) & 0xFFFF), 48);
}

/* MOVZ Wd, #imm16 (32-bit) */
static void emit_movz_w(EmitCtx* ctx, int rd, uint16_t imm16) {
    uint32_t inst = 0x52800000 | ((uint32_t)imm16 << 5) | (uint32_t)(rd & 0x1F);
    emit_inst(ctx, inst);
}

/* BLR Xn */
static void emit_blr(EmitCtx* ctx, int rn) {
    uint32_t inst = 0xD63F0000 | ((uint32_t)(rn & 0x1F) << 5);
    emit_inst(ctx, inst);
}

/* RET */
static void emit_ret(EmitCtx* ctx) {
    emit_inst(ctx, 0xD65F03C0);
}

/* FCVTZS Xd, Dn — double to int64 */
static void emit_fcvtzs_x_d(EmitCtx* ctx, int xd, int dn) {
    uint32_t inst = 0x9E780000 | ((uint32_t)(dn & 0x1F) << 5) | (uint32_t)(xd & 0x1F);
    emit_inst(ctx, inst);
}

/* SCVTF Dd, Xn — int64 to double */
static void emit_scvtf_d_x(EmitCtx* ctx, int dd, int xn) {
    uint32_t inst = 0x9E620000 | ((uint32_t)(xn & 0x1F) << 5) | (uint32_t)(dd & 0x1F);
    emit_inst(ctx, inst);
}

/* FCVT Sd, Dn — double to single */
static void emit_fcvt_s_d(EmitCtx* ctx, int sd, int dn) {
    uint32_t inst = 0x1E624000 | ((uint32_t)(dn & 0x1F) << 5) | (uint32_t)(sd & 0x1F);
    emit_inst(ctx, inst);
}

/* FCVT Dd, Sn — single to double */
static void emit_fcvt_d_s(EmitCtx* ctx, int dd, int sn) {
    uint32_t inst = 0x1E22C000 | ((uint32_t)(sn & 0x1F) << 5) | (uint32_t)(dd & 0x1F);
    emit_inst(ctx, inst);
}

/* AND Xd, Xn, #1 (bitmask immediate) */
static void emit_and_imm1(EmitCtx* ctx, int rd, int rn) {
    /* AND X0, X0, #1: sf=1, opc=00, N=1, immr=0, imms=0, Rn, Rd */
    uint32_t inst = 0x92400000 | ((uint32_t)(rn & 0x1F) << 5) | (uint32_t)(rd & 0x1F);
    emit_inst(ctx, inst);
}

/* === Type helpers === */

static int ffi_expected_cell_type_a64(FFICType t) {
    switch (t) {
        case FFI_DOUBLE: case FFI_FLOAT:
        case FFI_INT32: case FFI_INT64: case FFI_UINT32: case FFI_UINT64:
        case FFI_SIZE_T:
            return CELL_ATOM_NUMBER;
        case FFI_CSTRING:
            return CELL_ATOM_STRING;
        case FFI_BOOL:
            return CELL_ATOM_BOOL;
        case FFI_PTR:
            return CELL_FFI_PTR;
        case FFI_BUFFER:
            return CELL_BUFFER;
        default:
            return CELL_ATOM_NIL;
    }
}

static bool ffi_is_int_arg_a64(FFICType t) {
    switch (t) {
        case FFI_INT32: case FFI_INT64: case FFI_UINT32: case FFI_UINT64:
        case FFI_PTR: case FFI_CSTRING: case FFI_BOOL: case FFI_SIZE_T:
        case FFI_BUFFER:
            return true;
        default:
            return false;
    }
}

/* B.EQ — returns offset for patching */
static size_t emit_beq(EmitCtx* ctx) {
    size_t off = ctx->pos;
    emit_inst(ctx, 0x54000000); /* b.eq +0 (placeholder) */
    return off;
}

/* Patch B.EQ */
static void patch_beq(EmitCtx* ctx, size_t off) {
    int32_t delta = (int32_t)(ctx->pos - off) / 4;
    uint32_t imm19 = (uint32_t)delta & 0x7FFFF;
    uint32_t inst = 0x54000000 | (imm19 << 5);
    memcpy(&ctx->buf[off], &inst, 4);
}

/* B (unconditional) — returns offset for patching */
static size_t emit_b(EmitCtx* ctx) {
    size_t off = ctx->pos;
    emit_inst(ctx, 0x14000000); /* b +0 (placeholder) */
    return off;
}

/* Patch B (unconditional) */
static void patch_b(EmitCtx* ctx, size_t off) {
    int32_t delta = (int32_t)(ctx->pos - off) / 4;
    uint32_t imm26 = (uint32_t)delta & 0x3FFFFFF;
    uint32_t inst = 0x14000000 | imm26;
    memcpy(&ctx->buf[off], &inst, 4);
}

/* Is this a numeric FFI type that can accept both NUMBER and INTEGER cells? */
static bool ffi_is_numeric_type_a64(FFICType t) {
    switch (t) {
        case FFI_DOUBLE: case FFI_FLOAT:
        case FFI_INT32: case FFI_INT64: case FFI_UINT32: case FFI_UINT64:
        case FFI_SIZE_T:
            return true;
        default:
            return false;
    }
}

/* Total frame size: 16 (FP+LR) + 32 (X19-X22) + 64 (doubles) + 64 (ints) = 176, round to 176 */
#define FRAME_SIZE 176
#define SAVE_X19_OFF 16
#define SAVE_X20_OFF 24
#define SAVE_X21_OFF 32
#define SAVE_X22_OFF 40
#define DOUBLE_BASE  48    /* 8 double slots at [SP+48..SP+112) */
#define INT_BASE     112   /* 8 int64 slots at [SP+112..SP+176) */

bool emit_a64_stub(EmitCtx* ctx, FFISig* sig) {
    /*
     * Prologue:
     *   STP X29, X30, [SP, #-FRAME_SIZE]!   ; save FP, LR, allocate frame
     *   MOV X29, SP                           ; set frame pointer
     *   STP X19, X20, [SP, #SAVE_X19_OFF]
     *   STP X21, X22, [SP, #SAVE_X21_OFF]
     */
    emit_stp_pre64(ctx, 29, 30, 31, -FRAME_SIZE);
    emit_mov_from_sp(ctx, 29);  /* MOV X29, SP */
    emit_stp_off64(ctx, 19, 20, 31, SAVE_X19_OFF);
    emit_stp_off64(ctx, 21, 22, 31, SAVE_X21_OFF);

    /* X19 = args list cursor */
    emit_mov_x(ctx, 19, 0);

    /* X20 = SP (base for scratch storage) */
    emit_mov_from_sp(ctx, 20);

    int int_idx = 0;
    int fp_idx = 0;
    size_t error_patches[FFI_MAX_ARGS];

    for (int i = 0; i < sig->n_args; i++) {
        FFICType at = sig->arg_types[i];

        /* X21 = car(X19) = [X19 + CELL_OFF_DATA] */
        emit_ldr_x(ctx, 21, 19, CELL_OFF_DATA);

        /* W22 = [X21 + 0] (cell->type) */
        emit_ldr_w(ctx, 22, 21, 0);

        if (ffi_is_numeric_type_a64(at)) {
            /* Dual type check: INTEGER (zero-conversion fast path) or NUMBER */
            emit_cmp_w_imm(ctx, 22, CELL_ATOM_INTEGER);
            size_t int_patch = emit_beq(ctx);

            /* Not INTEGER — check NUMBER */
            emit_cmp_w_imm(ctx, 22, CELL_ATOM_NUMBER);
            error_patches[i] = emit_bne(ctx);

            /* === NUMBER path === */
            if (at == FFI_DOUBLE || at == FFI_FLOAT) {
                emit_ldr_d(ctx, 0, 21, CELL_OFF_DATA);
                emit_str_d(ctx, 0, 20, DOUBLE_BASE + fp_idx * 8);
            } else {
                /* Load double from NUMBER, convert to int64 via FCVTZS */
                emit_ldr_d(ctx, 0, 21, CELL_OFF_DATA);
                emit_fcvtzs_x_d(ctx, 22, 0);
                emit_str_x(ctx, 22, 20, INT_BASE + int_idx * 8);
            }
            size_t done_patch = emit_b(ctx);

            /* === INTEGER path (zero-conversion) === */
            patch_beq(ctx, int_patch);
            if (at == FFI_DOUBLE || at == FFI_FLOAT) {
                /* Load int64 from INTEGER cell, promote to double via SCVTF */
                emit_ldr_x(ctx, 22, 21, CELL_OFF_DATA);
                emit_scvtf_d_x(ctx, 0, 22);
                emit_str_d(ctx, 0, 20, DOUBLE_BASE + fp_idx * 8);
            } else {
                /* Zero-conversion: load int64 directly — no FP round-trip */
                emit_ldr_x(ctx, 22, 21, CELL_OFF_DATA);
                emit_str_x(ctx, 22, 20, INT_BASE + int_idx * 8);
            }
            patch_b(ctx, done_patch);

            if (at == FFI_DOUBLE || at == FFI_FLOAT) fp_idx++;
            else int_idx++;
        } else {
            /* Non-numeric: single type check */
            int expected = ffi_expected_cell_type_a64(at);
            emit_cmp_w_imm(ctx, 22, expected);
            error_patches[i] = emit_bne(ctx);

            if (at == FFI_PTR || at == FFI_CSTRING || at == FFI_BUFFER || at == FFI_BOOL) {
                emit_ldr_x(ctx, 22, 21, CELL_OFF_DATA);
            }
            emit_str_x(ctx, 22, 20, INT_BASE + int_idx * 8);
            int_idx++;
        }

        /* Advance: X19 = [X19 + CELL_OFF_CDR] (cdr) */
        emit_ldr_x(ctx, 19, 19, CELL_OFF_CDR);
    }

    /* Load values into ABI registers */
    int_idx = 0;
    fp_idx = 0;
    for (int i = 0; i < sig->n_args; i++) {
        FFICType at = sig->arg_types[i];
        if (at == FFI_DOUBLE) {
            emit_ldr_d(ctx, fp_idx, 20, DOUBLE_BASE + fp_idx * 8);
            fp_idx++;
        } else if (at == FFI_FLOAT) {
            emit_ldr_d(ctx, fp_idx, 20, DOUBLE_BASE + fp_idx * 8);
            emit_fcvt_s_d(ctx, fp_idx, fp_idx);
            fp_idx++;
        } else if (ffi_is_int_arg_a64(at)) {
            emit_ldr_x(ctx, int_idx, 20, INT_BASE + int_idx * 8);
            int_idx++;
        }
    }

    /* Call target: X8 = fn_ptr, BLR X8 */
    emit_imm64(ctx, 8, (uint64_t)(uintptr_t)sig->fn_ptr);
    emit_blr(ctx, 8);

    /* Wrap return value */
    switch (sig->ret_type) {
        case FFI_DOUBLE: {
            /* D0 already has result, call cell_number(double) */
            emit_imm64(ctx, 8, (uint64_t)(uintptr_t)cell_number);
            emit_blr(ctx, 8);
            break;
        }
        case FFI_FLOAT: {
            emit_fcvt_d_s(ctx, 0, 0);
            emit_imm64(ctx, 8, (uint64_t)(uintptr_t)cell_number);
            emit_blr(ctx, 8);
            break;
        }
        case FFI_INT32: case FFI_INT64: case FFI_UINT32: case FFI_UINT64:
        case FFI_SIZE_T: {
            /* X0 already has int64 result. Return as native integer cell. */
            emit_imm64(ctx, 8, (uint64_t)(uintptr_t)cell_integer);
            emit_blr(ctx, 8);
            break;
        }
        case FFI_BOOL: {
            emit_and_imm1(ctx, 0, 0);
            emit_imm64(ctx, 8, (uint64_t)(uintptr_t)cell_bool);
            emit_blr(ctx, 8);
            break;
        }
        case FFI_CSTRING: {
            /* X0 = char*, call cell_string */
            emit_imm64(ctx, 8, (uint64_t)(uintptr_t)cell_string);
            emit_blr(ctx, 8);
            break;
        }
        case FFI_PTR: {
            /* X0 = void*. Save in X19, then call cell_ffi_ptr(X19, 0, "ffi_return") */
            emit_mov_x(ctx, 19, 0);
            emit_mov_x(ctx, 0, 19);
            emit_movz(ctx, 1, 0, 0); /* NULL finalizer */
            emit_imm64(ctx, 2, (uint64_t)(uintptr_t)"ffi_return");
            emit_imm64(ctx, 8, (uint64_t)(uintptr_t)cell_ffi_ptr);
            emit_blr(ctx, 8);
            break;
        }
        case FFI_VOID:
        default: {
            emit_imm64(ctx, 8, (uint64_t)(uintptr_t)cell_nil);
            emit_blr(ctx, 8);
            break;
        }
    }

    /*
     * Epilogue:
     *   LDP X19, X20, [SP, #SAVE_X19_OFF]
     *   LDP X21, X22, [SP, #SAVE_X21_OFF]
     *   LDP X29, X30, [SP], #FRAME_SIZE
     *   RET
     */
    emit_ldp_off64(ctx, 19, 20, 31, SAVE_X19_OFF);
    emit_ldp_off64(ctx, 21, 22, 31, SAVE_X21_OFF);
    emit_ldp_post64(ctx, 29, 30, 31, FRAME_SIZE);
    emit_ret(ctx);

    /* Error paths */
    for (int i = 0; i < sig->n_args; i++) {
        patch_bcond(ctx, error_patches[i]);
        /* ffi_type_error(arg_index, expected, got_type) */
        emit_movz_w(ctx, 0, (uint16_t)i);
        emit_movz_w(ctx, 1, (uint16_t)ffi_expected_cell_type_a64(sig->arg_types[i]));
        emit_ldr_w(ctx, 2, 21, 0);
        emit_imm64(ctx, 8, (uint64_t)(uintptr_t)ffi_type_error);
        emit_blr(ctx, 8);
        /* Return error */
        emit_ldp_off64(ctx, 19, 20, 31, SAVE_X19_OFF);
        emit_ldp_off64(ctx, 21, 22, 31, SAVE_X21_OFF);
        emit_ldp_post64(ctx, 29, 30, 31, FRAME_SIZE);
        emit_ret(ctx);
    }

    return true;
}

#endif /* __aarch64__ */
