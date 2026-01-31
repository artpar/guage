#if defined(__x86_64__) || defined(_M_X64)

#include "ffi_jit.h"
#include <string.h>

/*
 * x86-64 SysV ABI JIT stub emitter
 *
 * Generated stub signature: Cell* stub(Cell* args)
 *
 * Input: RDI = cons-list of Cell* args
 * Output: RAX = Cell* result
 *
 * Strategy:
 *   1. Save callee-saved regs (RBX, R12-R15 as needed)
 *   2. Walk cons-list, type-check + extract each arg
 *   3. Load args into SysV registers (RDI,RSI,RDX,RCX,R8,R9 / XMM0-7)
 *   4. Call target C function
 *   5. Wrap return value (cell_number, cell_string, etc.)
 *   6. Return
 *
 * SysV integer arg regs: RDI, RSI, RDX, RCX, R8, R9
 * SysV float arg regs: XMM0-XMM7
 *
 * Cell layout offsets (verified from cell.h):
 *   +0:  type (uint32_t CellType)
 *   +32: data.atom.number (double) / data.atom.string (char*) / data.pair.car
 *   +40: data.pair.cdr
 *   +32: data.buffer.bytes (uint8_t*)
 */

/* x86-64 register encoding */
#define RAX 0
#define RCX 1
#define RDX 2
#define RBX 3
#define RSP 4
#define RBP 5
#define RSI 6
#define RDI 7
#define R8  8
#define R9  9
#define R10 10
#define R11 11
#define R12 12
#define R13 13
#define R14 14
#define R15 15

/* REX prefix helpers */
#define REX_W 0x48
#define REX_WR 0x4C
#define REX_WB 0x49
#define REX_WRB 0x4D

/* Integer arg regs in SysV order */
static const int int_arg_regs[] = { RDI, RSI, RDX, RCX, R8, R9 };

/* Helpers for x86-64 encoding */

/* MOV r64, imm64 (movabs) — REX.W + B8+rd + imm64 */
static void emit_mov_imm64(EmitCtx* ctx, int reg, uint64_t imm) {
    if (reg >= 8) {
        emit_u8(ctx, REX_WB);
        emit_u8(ctx, 0xB8 + (reg & 7));
    } else {
        emit_u8(ctx, REX_W);
        emit_u8(ctx, 0xB8 + reg);
    }
    emit_u64(ctx, imm);
}

/* MOV r64, [r64+disp8] */
static void emit_mov_rm_disp8(EmitCtx* ctx, int dst, int src, int8_t disp) {
    uint8_t rex = 0x48;
    if (dst >= 8) rex |= 0x04; /* REX.R */
    if (src >= 8) rex |= 0x01; /* REX.B */
    emit_u8(ctx, rex);
    emit_u8(ctx, 0x8B);
    emit_u8(ctx, 0x40 | ((dst & 7) << 3) | (src & 7));
    emit_u8(ctx, (uint8_t)disp);
}

/* CMP dword [r64], imm8 — compare cell type */
static void emit_cmp_dword_mem_imm8(EmitCtx* ctx, int reg, uint8_t imm) {
    if (reg >= 8) {
        emit_u8(ctx, 0x41); /* REX.B */
    }
    emit_u8(ctx, 0x83);
    emit_u8(ctx, 0x38 | (reg & 7)); /* /7, [reg] */
    emit_u8(ctx, imm);
}

/* JNE rel32 — placeholder, returns patch offset */
static size_t emit_jne_rel32(EmitCtx* ctx) {
    emit_u8(ctx, 0x0F);
    emit_u8(ctx, 0x85);
    size_t patch = ctx->pos;
    emit_u32(ctx, 0); /* placeholder */
    return patch;
}

/* Patch a rel32 jump target */
static void patch_rel32(EmitCtx* ctx, size_t patch_offset) {
    int32_t rel = (int32_t)(ctx->pos - (patch_offset + 4));
    memcpy(&ctx->buf[patch_offset], &rel, 4);
}

/* MOVSD xmmN, [r64+disp8] — load double */
static void emit_movsd_xmm_mem(EmitCtx* ctx, int xmm, int base, int8_t disp) {
    emit_u8(ctx, 0xF2);
    uint8_t rex = 0;
    if (xmm >= 8) rex |= 0x44; /* REX.R */
    if (base >= 8) rex |= 0x41; /* REX.B */
    if (rex) emit_u8(ctx, rex);
    emit_u8(ctx, 0x0F);
    emit_u8(ctx, 0x10);
    emit_u8(ctx, 0x40 | ((xmm & 7) << 3) | (base & 7));
    emit_u8(ctx, (uint8_t)disp);
}

/* CALL r64 */
static void emit_call_reg(EmitCtx* ctx, int reg) {
    if (reg >= 8) {
        emit_u8(ctx, 0x41);
    }
    emit_u8(ctx, 0xFF);
    emit_u8(ctx, 0xD0 | (reg & 7));
}

/* PUSH r64 */
static void emit_push(EmitCtx* ctx, int reg) {
    if (reg >= 8) emit_u8(ctx, 0x41);
    emit_u8(ctx, 0x50 + (reg & 7));
}

/* POP r64 */
static void emit_pop(EmitCtx* ctx, int reg) {
    if (reg >= 8) emit_u8(ctx, 0x41);
    emit_u8(ctx, 0x58 + (reg & 7));
}

/* RET */
static void emit_ret(EmitCtx* ctx) {
    emit_u8(ctx, 0xC3);
}

/* SUB RSP, imm8 */
static void emit_sub_rsp_imm8(EmitCtx* ctx, uint8_t imm) {
    emit_u8(ctx, REX_W);
    emit_u8(ctx, 0x83);
    emit_u8(ctx, 0xEC);
    emit_u8(ctx, imm);
}

/* ADD RSP, imm8 */
static void emit_add_rsp_imm8(EmitCtx* ctx, uint8_t imm) {
    emit_u8(ctx, REX_W);
    emit_u8(ctx, 0x83);
    emit_u8(ctx, 0xC4);
    emit_u8(ctx, imm);
}

/* CVTSS2SD xmm, xmm — float to double promotion */
static void emit_cvtss2sd(EmitCtx* ctx, int xmm_dst, int xmm_src) {
    emit_u8(ctx, 0xF3);
    if (xmm_dst >= 8 || xmm_src >= 8) {
        uint8_t rex = 0x40;
        if (xmm_dst >= 8) rex |= 0x04;
        if (xmm_src >= 8) rex |= 0x01;
        emit_u8(ctx, rex);
    }
    emit_u8(ctx, 0x0F);
    emit_u8(ctx, 0x5A);
    emit_u8(ctx, 0xC0 | ((xmm_dst & 7) << 3) | (xmm_src & 7));
}

/* CVTSD2SI r64, xmm — double to int64 */
static void emit_cvtsd2si(EmitCtx* ctx, int reg, int xmm) {
    emit_u8(ctx, 0xF2);
    uint8_t rex = 0x48;
    if (reg >= 8) rex |= 0x04;
    if (xmm >= 8) rex |= 0x01;
    emit_u8(ctx, rex);
    emit_u8(ctx, 0x0F);
    emit_u8(ctx, 0x2D);
    emit_u8(ctx, 0xC0 | ((reg & 7) << 3) | (xmm & 7));
}

/* CVTSI2SD xmm, r64 — int64 to double */
static void emit_cvtsi2sd(EmitCtx* ctx, int xmm, int reg) {
    emit_u8(ctx, 0xF2);
    uint8_t rex = 0x48;
    if (xmm >= 8) rex |= 0x04;
    if (reg >= 8) rex |= 0x01;
    emit_u8(ctx, rex);
    emit_u8(ctx, 0x0F);
    emit_u8(ctx, 0x2A);
    emit_u8(ctx, 0xC0 | ((xmm & 7) << 3) | (reg & 7));
}

/* CVTSD2SS xmm, xmm — double to float truncation */
static void emit_cvtsd2ss(EmitCtx* ctx, int xmm_dst, int xmm_src) {
    emit_u8(ctx, 0xF2);
    if (xmm_dst >= 8 || xmm_src >= 8) {
        uint8_t rex = 0x40;
        if (xmm_dst >= 8) rex |= 0x04;
        if (xmm_src >= 8) rex |= 0x01;
        emit_u8(ctx, rex);
    }
    emit_u8(ctx, 0x0F);
    emit_u8(ctx, 0x5A);
    emit_u8(ctx, 0xC0 | ((xmm_dst & 7) << 3) | (xmm_src & 7));
}

/* MOV r64, r64 */
static void emit_mov_r64_r64(EmitCtx* ctx, int dst, int src) {
    uint8_t rex = 0x48;
    if (src >= 8) rex |= 0x04;
    if (dst >= 8) rex |= 0x01;
    emit_u8(ctx, rex);
    emit_u8(ctx, 0x89);
    emit_u8(ctx, 0xC0 | ((src & 7) << 3) | (dst & 7));
}

/* Get the CellType constant for a given FFICType */
static int ffi_expected_cell_type(FFICType t) {
    switch (t) {
        case FFI_DOUBLE:
        case FFI_FLOAT:
        case FFI_INT32:
        case FFI_INT64:
        case FFI_UINT32:
        case FFI_UINT64:
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

/* Is this an integer/pointer register arg? */
static bool ffi_is_int_arg(FFICType t) {
    switch (t) {
        case FFI_INT32: case FFI_INT64: case FFI_UINT32: case FFI_UINT64:
        case FFI_PTR: case FFI_CSTRING: case FFI_BOOL: case FFI_SIZE_T:
        case FFI_BUFFER:
            return true;
        default:
            return false;
    }
}

bool emit_x64_stub(EmitCtx* ctx, FFISig* sig) {
    /* Prologue: save callee-saved regs we need */
    emit_push(ctx, RBP);
    emit_mov_r64_r64(ctx, RBP, RSP);
    emit_push(ctx, RBX);
    emit_push(ctx, R12);
    emit_push(ctx, R13);

    /* Align stack to 16 bytes (we pushed 4 regs = 32 bytes, RBP already aligned) */
    /* Save args list pointer in RBX */
    emit_mov_r64_r64(ctx, RBX, RDI); /* RBX = args cons-list */

    /* We'll accumulate extracted values on stack, then load into regs before call.
     * Simpler approach: extract into callee-saved regs and stack slots.
     *
     * For simplicity: save doubles to stack, integers to callee-saved regs.
     * We support up to 8 args total with max 6 int + 8 float.
     */

    /* Reserve stack space for arg values:
     * 8 doubles (64 bytes) + 8 int64s (64 bytes) = 128 bytes + alignment */
    size_t stack_space = 128 + 8; /* extra 8 for 16-byte alignment */
    emit_sub_rsp_imm8(ctx, (uint8_t)stack_space);

    /* R12 = args cursor (walking the cons-list) */
    emit_mov_r64_r64(ctx, R12, RBX);

    /* Track register allocation */
    int int_reg_idx = 0;
    int float_reg_idx = 0;

    /* Per-arg error patch locations */
    size_t error_patches[FFI_MAX_ARGS];

    for (int i = 0; i < sig->n_args; i++) {
        FFICType at = sig->arg_types[i];
        int expected_ct = ffi_expected_cell_type(at);

        /* RAX = car(R12) = [R12 + 32] — the arg cell */
        emit_mov_rm_disp8(ctx, RAX, R12, 32);

        /* Type check: cmp dword [RAX], expected_cell_type */
        emit_cmp_dword_mem_imm8(ctx, RAX, (uint8_t)expected_ct);
        error_patches[i] = emit_jne_rel32(ctx);

        /* Extract value based on FFI type */
        if (at == FFI_DOUBLE) {
            /* movsd xmmN, [RAX+32] — but we need to stash to stack since we
             * can't keep xmm regs across the cons-list walk. Store at [RSP + float_reg_idx*8] */
            emit_movsd_xmm_mem(ctx, 0, RAX, 32); /* XMM0 = double value */
            /* movsd [RSP + offset], xmm0 */
            emit_u8(ctx, 0xF2); emit_u8(ctx, 0x0F); emit_u8(ctx, 0x11);
            emit_u8(ctx, 0x44); emit_u8(ctx, 0x24);
            emit_u8(ctx, (uint8_t)(float_reg_idx * 8));
            float_reg_idx++;
        } else if (at == FFI_FLOAT) {
            /* Load double, will convert to float before call */
            emit_movsd_xmm_mem(ctx, 0, RAX, 32);
            emit_u8(ctx, 0xF2); emit_u8(ctx, 0x0F); emit_u8(ctx, 0x11);
            emit_u8(ctx, 0x44); emit_u8(ctx, 0x24);
            emit_u8(ctx, (uint8_t)(float_reg_idx * 8));
            float_reg_idx++;
        } else if (ffi_is_int_arg(at)) {
            /* mov reg, [RAX+32] — load the value (number as double, then cvt, or pointer/string direct) */
            if (at == FFI_PTR || at == FFI_CSTRING || at == FFI_BUFFER) {
                /* Pointer types: load [RAX+32] directly as pointer */
                emit_mov_rm_disp8(ctx, R13, RAX, 32);
            } else if (at == FFI_BOOL) {
                /* Load boolean from [RAX+32] */
                emit_mov_rm_disp8(ctx, R13, RAX, 32);
            } else {
                /* Integer types: load double from [RAX+32], convert to int64 */
                emit_movsd_xmm_mem(ctx, 0, RAX, 32);
                emit_cvtsd2si(ctx, R13, 0);
            }
            /* Store int value at [RSP + 64 + int_reg_idx*8] */
            /* mov [RSP + offset], R13 */
            uint8_t rex2 = 0x4C; /* REX.WR for R13 */
            emit_u8(ctx, rex2);
            emit_u8(ctx, 0x89);
            emit_u8(ctx, 0x6C); /* R13 = 5, ModRM = 01 101 100 = 0x6C */
            emit_u8(ctx, 0x24);
            emit_u8(ctx, (uint8_t)(64 + int_reg_idx * 8));
            int_reg_idx++;
        }

        /* Advance: R12 = cdr(R12) = [R12 + 40] */
        emit_mov_rm_disp8(ctx, R12, R12, 40);
    }

    /* Now load extracted values into the correct ABI registers.
     * Float args go into XMM0-XMM7 from stack offsets [RSP + 0..].
     * Int args go into RDI,RSI,RDX,RCX,R8,R9 from stack offsets [RSP + 64..]. */

    int_reg_idx = 0;
    float_reg_idx = 0;
    for (int i = 0; i < sig->n_args; i++) {
        FFICType at = sig->arg_types[i];
        if (at == FFI_DOUBLE) {
            /* movsd xmmN, [RSP + float_reg_idx*8] */
            emit_u8(ctx, 0xF2); emit_u8(ctx, 0x0F); emit_u8(ctx, 0x10);
            emit_u8(ctx, 0x44 | ((float_reg_idx & 7) << 3));
            emit_u8(ctx, 0x24);
            emit_u8(ctx, (uint8_t)(float_reg_idx * 8));
            float_reg_idx++;
        } else if (at == FFI_FLOAT) {
            /* Load as double from stack, convert to float */
            emit_u8(ctx, 0xF2); emit_u8(ctx, 0x0F); emit_u8(ctx, 0x10);
            emit_u8(ctx, 0x44 | ((float_reg_idx & 7) << 3));
            emit_u8(ctx, 0x24);
            emit_u8(ctx, (uint8_t)(float_reg_idx * 8));
            emit_cvtsd2ss(ctx, float_reg_idx, float_reg_idx);
            float_reg_idx++;
        } else if (ffi_is_int_arg(at)) {
            int reg = int_arg_regs[int_reg_idx];
            /* mov reg, [RSP + 64 + int_reg_idx*8] */
            uint8_t rex3 = 0x48;
            if (reg >= 8) rex3 |= 0x04;
            emit_u8(ctx, rex3);
            emit_u8(ctx, 0x8B);
            emit_u8(ctx, 0x44 | ((reg & 7) << 3));
            emit_u8(ctx, 0x24);
            emit_u8(ctx, (uint8_t)(64 + int_reg_idx * 8));
            int_reg_idx++;
        }
    }

    /* Call target function: movabs RAX, fn_ptr; call RAX */
    emit_mov_imm64(ctx, RAX, (uint64_t)(uintptr_t)sig->fn_ptr);
    emit_call_reg(ctx, RAX);

    /* Wrap return value */
    switch (sig->ret_type) {
        case FFI_DOUBLE: {
            /* Result in XMM0. Need to call cell_number(double).
             * XMM0 is already the first float arg for SysV. */
            emit_mov_imm64(ctx, RAX, (uint64_t)(uintptr_t)cell_number);
            emit_call_reg(ctx, RAX);
            break;
        }
        case FFI_FLOAT: {
            /* Result in XMM0 as float. Promote to double, then cell_number. */
            emit_cvtss2sd(ctx, 0, 0);
            emit_mov_imm64(ctx, RAX, (uint64_t)(uintptr_t)cell_number);
            emit_call_reg(ctx, RAX);
            break;
        }
        case FFI_INT32:
        case FFI_INT64:
        case FFI_UINT32:
        case FFI_UINT64:
        case FFI_SIZE_T: {
            /* Result in RAX as integer. Convert to double, then cell_number. */
            emit_cvtsi2sd(ctx, 0, RAX);
            emit_mov_imm64(ctx, RAX, (uint64_t)(uintptr_t)cell_number);
            emit_call_reg(ctx, RAX);
            break;
        }
        case FFI_BOOL: {
            /* Result in EAX. Convert to bool, then cell_bool. */
            /* movzx edi, al — zero extend to RDI */
            emit_u8(ctx, 0x0F); emit_u8(ctx, 0xB6); emit_u8(ctx, 0xF8);
            emit_mov_imm64(ctx, RAX, (uint64_t)(uintptr_t)cell_bool);
            emit_call_reg(ctx, RAX);
            break;
        }
        case FFI_CSTRING: {
            /* Result in RAX = char*. Call cell_string(RAX). */
            emit_mov_r64_r64(ctx, RDI, RAX);
            emit_mov_imm64(ctx, RAX, (uint64_t)(uintptr_t)cell_string);
            emit_call_reg(ctx, RAX);
            break;
        }
        case FFI_PTR: {
            /* Result in RAX = void*. Wrap as FFI_PTR cell. */
            /* cell_ffi_ptr(ptr, NULL, "ffi_return") */
            emit_mov_r64_r64(ctx, RDI, RAX);       /* ptr */
            emit_mov_imm64(ctx, RSI, 0);            /* NULL finalizer */
            emit_mov_imm64(ctx, RDX, (uint64_t)(uintptr_t)"ffi_return");
            emit_mov_imm64(ctx, RAX, (uint64_t)(uintptr_t)cell_ffi_ptr);
            emit_call_reg(ctx, RAX);
            break;
        }
        case FFI_VOID: {
            /* No return value. Return nil. */
            emit_mov_imm64(ctx, RAX, (uint64_t)(uintptr_t)cell_nil);
            emit_call_reg(ctx, RAX);
            break;
        }
        default: {
            emit_mov_imm64(ctx, RAX, (uint64_t)(uintptr_t)cell_nil);
            emit_call_reg(ctx, RAX);
            break;
        }
    }

    /* Epilogue */
    emit_add_rsp_imm8(ctx, (uint8_t)stack_space);
    emit_pop(ctx, R13);
    emit_pop(ctx, R12);
    emit_pop(ctx, RBX);
    emit_pop(ctx, RBP);
    emit_ret(ctx);

    /* Error paths — one per arg */
    for (int i = 0; i < sig->n_args; i++) {
        patch_rel32(ctx, error_patches[i]);
        /* Call ffi_type_error(arg_index, expected, got_type) */
        /* Load got_type from [RAX] (the cell type) */
        /* mov edi, i */
        emit_u8(ctx, 0xBF); emit_u32(ctx, (uint32_t)i);
        /* mov esi, expected */
        emit_u8(ctx, 0xBE); emit_u32(ctx, (uint32_t)ffi_expected_cell_type(sig->arg_types[i]));
        /* mov edx, [rax] — cell->type (only if RAX is valid) */
        emit_u8(ctx, 0x8B); emit_u8(ctx, 0x10); /* mov edx, [rax] */
        emit_mov_imm64(ctx, RAX, (uint64_t)(uintptr_t)ffi_type_error);
        emit_call_reg(ctx, RAX);
        /* Clean up and return the error */
        emit_add_rsp_imm8(ctx, (uint8_t)stack_space);
        emit_pop(ctx, R13);
        emit_pop(ctx, R12);
        emit_pop(ctx, RBX);
        emit_pop(ctx, RBP);
        emit_ret(ctx);
    }

    return true;
}

#endif /* __x86_64__ */
