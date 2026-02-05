/*
 * ARM64 Stencils for Copy-and-Patch JIT
 *
 * Pre-compiled machine code templates for hot path operations.
 * These operate on raw values (double/int64) without boxing overhead.
 *
 * Register conventions:
 *   D0-D7:  Double registers (floating point)
 *   X0-X7:  Integer/pointer registers
 *   X8:     Scratch register
 *   X9-X15: More scratch
 *   X19-X28: Callee-saved (we preserve these)
 *   X29:    Frame pointer
 *   X30:    Link register
 *
 * Stencil format:
 *   - Each stencil is self-contained machine code
 *   - "Holes" are 8-byte slots that get patched at JIT time
 *   - Holes are aligned to 8 bytes for easy patching
 */

#if defined(__aarch64__) || defined(_M_ARM64)

#include "jit.h"
#include <string.h>

/* Helper to register a stencil */
extern void jit_register_stencil(JITOpcode op, const uint8_t* code, size_t size,
                                 const uint32_t* holes, uint8_t n_holes);

/* ============================================================================
 * Arithmetic Stencils (Double)
 * ============================================================================ */

/* ADD_DD: D0 = D1 + D2 */
static const uint8_t stencil_add_dd[] = {
    0x00, 0x28, 0x62, 0x1E,  /* FADD D0, D0, D2 */
};

/* SUB_DD: D0 = D1 - D2 */
static const uint8_t stencil_sub_dd[] = {
    0x00, 0x38, 0x62, 0x1E,  /* FSUB D0, D0, D2 */
};

/* MUL_DD: D0 = D1 * D2 */
static const uint8_t stencil_mul_dd[] = {
    0x00, 0x08, 0x62, 0x1E,  /* FMUL D0, D0, D2 */
};

/* DIV_DD: D0 = D1 / D2 */
static const uint8_t stencil_div_dd[] = {
    0x00, 0x18, 0x62, 0x1E,  /* FDIV D0, D0, D2 */
};

/* NEG_D: D0 = -D1 */
static const uint8_t stencil_neg_d[] = {
    0x00, 0x40, 0x61, 0x1E,  /* FNEG D0, D0 */
};

/* ============================================================================
 * Arithmetic Stencils (Integer)
 * ============================================================================ */

/* ADD_II: X0 = X0 + X1 */
static const uint8_t stencil_add_ii[] = {
    0x00, 0x00, 0x01, 0x8B,  /* ADD X0, X0, X1 */
};

/* SUB_II: X0 = X0 - X1 */
static const uint8_t stencil_sub_ii[] = {
    0x00, 0x00, 0x01, 0xCB,  /* SUB X0, X0, X1 */
};

/* MUL_II: X0 = X0 * X1 */
static const uint8_t stencil_mul_ii[] = {
    0x00, 0x7C, 0x01, 0x9B,  /* MUL X0, X0, X1 */
};

/* DIV_II: X0 = X0 / X1 (signed) */
static const uint8_t stencil_div_ii[] = {
    0x00, 0x0C, 0xC1, 0x9A,  /* SDIV X0, X0, X1 */
};

/* NEG_I: X0 = -X0 */
static const uint8_t stencil_neg_i[] = {
    0x00, 0x00, 0x00, 0xCB,  /* NEG X0, X0 (SUB X0, XZR, X0) */
};

/* ============================================================================
 * Comparison Stencils (Double)
 * ============================================================================ */

/* LT_DD: X0 = (D0 < D1) ? 1 : 0 */
static const uint8_t stencil_lt_dd[] = {
    0x00, 0x20, 0x61, 0x1E,  /* FCMP D0, D1 */
    0xE0, 0x47, 0x9F, 0x9A,  /* CSET X0, MI (less than) */
};

/* LE_DD: X0 = (D0 <= D1) ? 1 : 0 */
static const uint8_t stencil_le_dd[] = {
    0x00, 0x20, 0x61, 0x1E,  /* FCMP D0, D1 */
    0xE0, 0xC7, 0x9F, 0x9A,  /* CSET X0, LS (less or same) */
};

/* GT_DD: X0 = (D0 > D1) ? 1 : 0 */
static const uint8_t stencil_gt_dd[] = {
    0x00, 0x20, 0x61, 0x1E,  /* FCMP D0, D1 */
    0xE0, 0xC7, 0x9F, 0x9A,  /* CSET X0, GT */
};

/* GE_DD: X0 = (D0 >= D1) ? 1 : 0 */
static const uint8_t stencil_ge_dd[] = {
    0x00, 0x20, 0x61, 0x1E,  /* FCMP D0, D1 */
    0xE0, 0xA7, 0x9F, 0x9A,  /* CSET X0, GE */
};

/* EQ_DD: X0 = (D0 == D1) ? 1 : 0 */
static const uint8_t stencil_eq_dd[] = {
    0x00, 0x20, 0x61, 0x1E,  /* FCMP D0, D1 */
    0xE0, 0x07, 0x9F, 0x9A,  /* CSET X0, EQ */
};

/* ============================================================================
 * Comparison Stencils (Integer)
 * ============================================================================ */

/* LT_II: X0 = (X0 < X1) ? 1 : 0 (signed) */
static const uint8_t stencil_lt_ii[] = {
    0x1F, 0x00, 0x01, 0xEB,  /* CMP X0, X1 */
    0xE0, 0xB7, 0x9F, 0x9A,  /* CSET X0, LT */
};

/* LE_II: X0 = (X0 <= X1) ? 1 : 0 */
static const uint8_t stencil_le_ii[] = {
    0x1F, 0x00, 0x01, 0xEB,  /* CMP X0, X1 */
    0xE0, 0xD7, 0x9F, 0x9A,  /* CSET X0, LE */
};

/* GT_II: X0 = (X0 > X1) ? 1 : 0 */
static const uint8_t stencil_gt_ii[] = {
    0x1F, 0x00, 0x01, 0xEB,  /* CMP X0, X1 */
    0xE0, 0xC7, 0x9F, 0x9A,  /* CSET X0, GT */
};

/* GE_II: X0 = (X0 >= X1) ? 1 : 0 */
static const uint8_t stencil_ge_ii[] = {
    0x1F, 0x00, 0x01, 0xEB,  /* CMP X0, X1 */
    0xE0, 0xA7, 0x9F, 0x9A,  /* CSET X0, GE */
};

/* EQ_II: X0 = (X0 == X1) ? 1 : 0 */
static const uint8_t stencil_eq_ii[] = {
    0x1F, 0x00, 0x01, 0xEB,  /* CMP X0, X1 */
    0xE0, 0x07, 0x9F, 0x9A,  /* CSET X0, EQ */
};

/* ============================================================================
 * Type Conversion Stencils
 * ============================================================================ */

/* I2D: D0 = (double)X0 */
static const uint8_t stencil_i2d[] = {
    0x00, 0x00, 0x62, 0x9E,  /* SCVTF D0, X0 */
};

/* D2I: X0 = (int64_t)D0 */
static const uint8_t stencil_d2i[] = {
    0x00, 0x00, 0x78, 0x9E,  /* FCVTZS X0, D0 */
};

/* ============================================================================
 * Memory Access Stencils
 * ============================================================================ */

/* LOAD_NUM: D0 = [X0 + 40] (cell->number, offset 40 for data union) */
static const uint8_t stencil_load_num[] = {
    0x00, 0x14, 0x40, 0xFD,  /* LDR D0, [X0, #40] */
};

/* STORE_NUM: [X0 + 40] = D0 */
static const uint8_t stencil_store_num[] = {
    0x00, 0x14, 0x00, 0xFD,  /* STR D0, [X0, #40] */
};

/* LOAD_INT: X1 = [X0 + 40] */
static const uint8_t stencil_load_int[] = {
    0x01, 0x14, 0x40, 0xF9,  /* LDR X1, [X0, #40] */
};

/* CAR: X0 = [X0 + 40] (pair->car at offset 40) */
static const uint8_t stencil_car[] = {
    0x00, 0x14, 0x40, 0xF9,  /* LDR X0, [X0, #40] */
};

/* CDR: X0 = [X0 + 48] (pair->cdr at offset 48) */
static const uint8_t stencil_cdr[] = {
    0x00, 0x18, 0x40, 0xF9,  /* LDR X0, [X0, #48] */
};

/* ============================================================================
 * Constant Loading Stencils
 * ============================================================================ */

/* CONST_NUM: D0 = immediate double (hole at offset 0) */
static const uint8_t stencil_const_num[] = {
    /* Load 64-bit immediate into X8, then FMOV to D0 */
    0x08, 0x00, 0x80, 0xD2,  /* MOVZ X8, #imm16_0 (hole: 2) */
    0x08, 0x00, 0xA0, 0xF2,  /* MOVK X8, #imm16_1, LSL #16 (hole: 6) */
    0x08, 0x00, 0xC0, 0xF2,  /* MOVK X8, #imm16_2, LSL #32 (hole: 10) */
    0x08, 0x00, 0xE0, 0xF2,  /* MOVK X8, #imm16_3, LSL #48 (hole: 14) */
    0x00, 0x01, 0x67, 0x9E,  /* FMOV D0, X8 */
};
static const uint32_t holes_const_num[] = {2, 6, 10, 14};

/* CONST_INT: X0 = immediate int64 */
static const uint8_t stencil_const_int[] = {
    0x00, 0x00, 0x80, 0xD2,  /* MOVZ X0, #imm16_0 (hole: 2) */
    0x00, 0x00, 0xA0, 0xF2,  /* MOVK X0, #imm16_1, LSL #16 (hole: 6) */
    0x00, 0x00, 0xC0, 0xF2,  /* MOVK X0, #imm16_2, LSL #32 (hole: 10) */
    0x00, 0x00, 0xE0, 0xF2,  /* MOVK X0, #imm16_3, LSL #48 (hole: 14) */
};
static const uint32_t holes_const_int[] = {2, 6, 10, 14};

/* ============================================================================
 * Control Flow Stencils
 * ============================================================================ */

/* JUMP: unconditional branch (hole: offset at 0) */
static const uint8_t stencil_jump[] = {
    0x00, 0x00, 0x00, 0x14,  /* B #offset (hole: 0) */
};
static const uint32_t holes_jump[] = {0};

/* JUMP_IF: branch if X0 != 0 */
static const uint8_t stencil_jump_if[] = {
    0x00, 0x00, 0x00, 0xB5,  /* CBNZ X0, #offset (hole: 0) */
};
static const uint32_t holes_jump_if[] = {0};

/* JUMP_UNLESS: branch if X0 == 0 */
static const uint8_t stencil_jump_unless[] = {
    0x00, 0x00, 0x00, 0xB4,  /* CBZ X0, #offset (hole: 0) */
};
static const uint32_t holes_jump_unless[] = {0};

/* RET: return X0 */
static const uint8_t stencil_ret[] = {
    0xC0, 0x03, 0x5F, 0xD6,  /* RET */
};

/* ============================================================================
 * Initialization
 * ============================================================================ */

void jit_stencils_init_a64(void) {
    /* Arithmetic - Double */
    jit_register_stencil(JOP_ADD_DD, stencil_add_dd, sizeof(stencil_add_dd), NULL, 0);
    jit_register_stencil(JOP_SUB_DD, stencil_sub_dd, sizeof(stencil_sub_dd), NULL, 0);
    jit_register_stencil(JOP_MUL_DD, stencil_mul_dd, sizeof(stencil_mul_dd), NULL, 0);
    jit_register_stencil(JOP_DIV_DD, stencil_div_dd, sizeof(stencil_div_dd), NULL, 0);
    jit_register_stencil(JOP_NEG_D, stencil_neg_d, sizeof(stencil_neg_d), NULL, 0);

    /* Arithmetic - Integer */
    jit_register_stencil(JOP_ADD_II, stencil_add_ii, sizeof(stencil_add_ii), NULL, 0);
    jit_register_stencil(JOP_SUB_II, stencil_sub_ii, sizeof(stencil_sub_ii), NULL, 0);
    jit_register_stencil(JOP_MUL_II, stencil_mul_ii, sizeof(stencil_mul_ii), NULL, 0);
    jit_register_stencil(JOP_DIV_II, stencil_div_ii, sizeof(stencil_div_ii), NULL, 0);
    jit_register_stencil(JOP_NEG_I, stencil_neg_i, sizeof(stencil_neg_i), NULL, 0);

    /* Comparisons - Double */
    jit_register_stencil(JOP_LT_DD, stencil_lt_dd, sizeof(stencil_lt_dd), NULL, 0);
    jit_register_stencil(JOP_LE_DD, stencil_le_dd, sizeof(stencil_le_dd), NULL, 0);
    jit_register_stencil(JOP_GT_DD, stencil_gt_dd, sizeof(stencil_gt_dd), NULL, 0);
    jit_register_stencil(JOP_GE_DD, stencil_ge_dd, sizeof(stencil_ge_dd), NULL, 0);
    jit_register_stencil(JOP_EQ_DD, stencil_eq_dd, sizeof(stencil_eq_dd), NULL, 0);

    /* Comparisons - Integer */
    jit_register_stencil(JOP_LT_II, stencil_lt_ii, sizeof(stencil_lt_ii), NULL, 0);
    jit_register_stencil(JOP_LE_II, stencil_le_ii, sizeof(stencil_le_ii), NULL, 0);
    jit_register_stencil(JOP_GT_II, stencil_gt_ii, sizeof(stencil_gt_ii), NULL, 0);
    jit_register_stencil(JOP_GE_II, stencil_ge_ii, sizeof(stencil_ge_ii), NULL, 0);
    jit_register_stencil(JOP_EQ_II, stencil_eq_ii, sizeof(stencil_eq_ii), NULL, 0);

    /* Type Conversions */
    jit_register_stencil(JOP_I2D, stencil_i2d, sizeof(stencil_i2d), NULL, 0);
    jit_register_stencil(JOP_D2I, stencil_d2i, sizeof(stencil_d2i), NULL, 0);

    /* Memory Access */
    jit_register_stencil(JOP_LOAD_NUM, stencil_load_num, sizeof(stencil_load_num), NULL, 0);
    jit_register_stencil(JOP_STORE_NUM, stencil_store_num, sizeof(stencil_store_num), NULL, 0);
    jit_register_stencil(JOP_LOAD_INT, stencil_load_int, sizeof(stencil_load_int), NULL, 0);
    jit_register_stencil(JOP_CAR, stencil_car, sizeof(stencil_car), NULL, 0);
    jit_register_stencil(JOP_CDR, stencil_cdr, sizeof(stencil_cdr), NULL, 0);

    /* Constants */
    jit_register_stencil(JOP_CONST_NUM, stencil_const_num, sizeof(stencil_const_num),
                         holes_const_num, 4);
    jit_register_stencil(JOP_CONST_INT, stencil_const_int, sizeof(stencil_const_int),
                         holes_const_int, 4);

    /* Control Flow */
    jit_register_stencil(JOP_JUMP, stencil_jump, sizeof(stencil_jump), holes_jump, 1);
    jit_register_stencil(JOP_JUMP_IF, stencil_jump_if, sizeof(stencil_jump_if), holes_jump_if, 1);
    jit_register_stencil(JOP_JUMP_UNLESS, stencil_jump_unless, sizeof(stencil_jump_unless),
                         holes_jump_unless, 1);
    jit_register_stencil(JOP_RET, stencil_ret, sizeof(stencil_ret), NULL, 0);
}

#endif /* __aarch64__ */
