/*
 * x86-64 Stencils for Copy-and-Patch JIT
 *
 * Pre-compiled machine code templates for hot path operations.
 * Uses System V AMD64 ABI (Linux, macOS).
 *
 * Register conventions:
 *   XMM0-XMM7: Floating point arguments/returns
 *   RAX:       Return value (integer)
 *   RDI, RSI, RDX, RCX, R8, R9: Integer arguments
 *   R10, R11:  Scratch registers
 *   RBX, R12-R15: Callee-saved
 */

#if defined(__x86_64__) || defined(_M_X64)

#include "jit.h"
#include <string.h>

extern void jit_register_stencil(JITOpcode op, const uint8_t* code, size_t size,
                                 const uint32_t* holes, uint8_t n_holes);

/* ============================================================================
 * Arithmetic Stencils (Double - SSE2)
 * ============================================================================ */

/* ADD_DD: XMM0 = XMM0 + XMM1 */
static const uint8_t stencil_add_dd[] = {
    0xF2, 0x0F, 0x58, 0xC1,  /* ADDSD XMM0, XMM1 */
};

/* SUB_DD: XMM0 = XMM0 - XMM1 */
static const uint8_t stencil_sub_dd[] = {
    0xF2, 0x0F, 0x5C, 0xC1,  /* SUBSD XMM0, XMM1 */
};

/* MUL_DD: XMM0 = XMM0 * XMM1 */
static const uint8_t stencil_mul_dd[] = {
    0xF2, 0x0F, 0x59, 0xC1,  /* MULSD XMM0, XMM1 */
};

/* DIV_DD: XMM0 = XMM0 / XMM1 */
static const uint8_t stencil_div_dd[] = {
    0xF2, 0x0F, 0x5E, 0xC1,  /* DIVSD XMM0, XMM1 */
};

/* ============================================================================
 * Arithmetic Stencils (Integer)
 * ============================================================================ */

/* ADD_II: RAX = RAX + RDI */
static const uint8_t stencil_add_ii[] = {
    0x48, 0x01, 0xF8,        /* ADD RAX, RDI */
};

/* SUB_II: RAX = RAX - RDI */
static const uint8_t stencil_sub_ii[] = {
    0x48, 0x29, 0xF8,        /* SUB RAX, RDI */
};

/* MUL_II: RAX = RAX * RDI */
static const uint8_t stencil_mul_ii[] = {
    0x48, 0x0F, 0xAF, 0xC7,  /* IMUL RAX, RDI */
};

/* DIV_II: RAX = RAX / RDI (signed) */
static const uint8_t stencil_div_ii[] = {
    0x48, 0x99,              /* CQO (sign-extend RAX to RDX:RAX) */
    0x48, 0xF7, 0xFF,        /* IDIV RDI */
};

/* NEG_I: RAX = -RAX */
static const uint8_t stencil_neg_i[] = {
    0x48, 0xF7, 0xD8,        /* NEG RAX */
};

/* ============================================================================
 * Comparison Stencils (Double)
 * ============================================================================ */

/* LT_DD: RAX = (XMM0 < XMM1) ? 1 : 0 */
static const uint8_t stencil_lt_dd[] = {
    0x66, 0x0F, 0x2F, 0xC1,  /* COMISD XMM0, XMM1 */
    0x0F, 0x92, 0xC0,        /* SETB AL */
    0x0F, 0xB6, 0xC0,        /* MOVZX EAX, AL */
};

/* LE_DD: RAX = (XMM0 <= XMM1) ? 1 : 0 */
static const uint8_t stencil_le_dd[] = {
    0x66, 0x0F, 0x2F, 0xC1,  /* COMISD XMM0, XMM1 */
    0x0F, 0x96, 0xC0,        /* SETBE AL */
    0x0F, 0xB6, 0xC0,        /* MOVZX EAX, AL */
};

/* GT_DD: RAX = (XMM0 > XMM1) ? 1 : 0 */
static const uint8_t stencil_gt_dd[] = {
    0x66, 0x0F, 0x2F, 0xC1,  /* COMISD XMM0, XMM1 */
    0x0F, 0x97, 0xC0,        /* SETA AL */
    0x0F, 0xB6, 0xC0,        /* MOVZX EAX, AL */
};

/* GE_DD: RAX = (XMM0 >= XMM1) ? 1 : 0 */
static const uint8_t stencil_ge_dd[] = {
    0x66, 0x0F, 0x2F, 0xC1,  /* COMISD XMM0, XMM1 */
    0x0F, 0x93, 0xC0,        /* SETAE AL */
    0x0F, 0xB6, 0xC0,        /* MOVZX EAX, AL */
};

/* EQ_DD: RAX = (XMM0 == XMM1) ? 1 : 0 */
static const uint8_t stencil_eq_dd[] = {
    0x66, 0x0F, 0x2F, 0xC1,  /* COMISD XMM0, XMM1 */
    0x0F, 0x94, 0xC0,        /* SETE AL */
    0x0F, 0xB6, 0xC0,        /* MOVZX EAX, AL */
};

/* ============================================================================
 * Comparison Stencils (Integer)
 * ============================================================================ */

/* LT_II: RAX = (RAX < RDI) ? 1 : 0 */
static const uint8_t stencil_lt_ii[] = {
    0x48, 0x39, 0xF8,        /* CMP RAX, RDI */
    0x0F, 0x9C, 0xC0,        /* SETL AL */
    0x0F, 0xB6, 0xC0,        /* MOVZX EAX, AL */
};

/* LE_II: RAX = (RAX <= RDI) ? 1 : 0 */
static const uint8_t stencil_le_ii[] = {
    0x48, 0x39, 0xF8,        /* CMP RAX, RDI */
    0x0F, 0x9E, 0xC0,        /* SETLE AL */
    0x0F, 0xB6, 0xC0,        /* MOVZX EAX, AL */
};

/* GT_II: RAX = (RAX > RDI) ? 1 : 0 */
static const uint8_t stencil_gt_ii[] = {
    0x48, 0x39, 0xF8,        /* CMP RAX, RDI */
    0x0F, 0x9F, 0xC0,        /* SETG AL */
    0x0F, 0xB6, 0xC0,        /* MOVZX EAX, AL */
};

/* GE_II: RAX = (RAX >= RDI) ? 1 : 0 */
static const uint8_t stencil_ge_ii[] = {
    0x48, 0x39, 0xF8,        /* CMP RAX, RDI */
    0x0F, 0x9D, 0xC0,        /* SETGE AL */
    0x0F, 0xB6, 0xC0,        /* MOVZX EAX, AL */
};

/* EQ_II: RAX = (RAX == RDI) ? 1 : 0 */
static const uint8_t stencil_eq_ii[] = {
    0x48, 0x39, 0xF8,        /* CMP RAX, RDI */
    0x0F, 0x94, 0xC0,        /* SETE AL */
    0x0F, 0xB6, 0xC0,        /* MOVZX EAX, AL */
};

/* ============================================================================
 * Type Conversion Stencils
 * ============================================================================ */

/* I2D: XMM0 = (double)RAX */
static const uint8_t stencil_i2d[] = {
    0xF2, 0x48, 0x0F, 0x2A, 0xC0,  /* CVTSI2SD XMM0, RAX */
};

/* D2I: RAX = (int64_t)XMM0 */
static const uint8_t stencil_d2i[] = {
    0xF2, 0x48, 0x0F, 0x2C, 0xC0,  /* CVTTSD2SI RAX, XMM0 */
};

/* ============================================================================
 * Memory Access Stencils
 * ============================================================================ */

/* LOAD_NUM: XMM0 = [RDI + 40] */
static const uint8_t stencil_load_num[] = {
    0xF2, 0x0F, 0x10, 0x47, 0x28,  /* MOVSD XMM0, [RDI + 40] */
};

/* STORE_NUM: [RDI + 40] = XMM0 */
static const uint8_t stencil_store_num[] = {
    0xF2, 0x0F, 0x11, 0x47, 0x28,  /* MOVSD [RDI + 40], XMM0 */
};

/* LOAD_INT: RAX = [RDI + 40] */
static const uint8_t stencil_load_int[] = {
    0x48, 0x8B, 0x47, 0x28,        /* MOV RAX, [RDI + 40] */
};

/* CAR: RAX = [RDI + 40] */
static const uint8_t stencil_car[] = {
    0x48, 0x8B, 0x47, 0x28,        /* MOV RAX, [RDI + 40] */
};

/* CDR: RAX = [RDI + 48] */
static const uint8_t stencil_cdr[] = {
    0x48, 0x8B, 0x47, 0x30,        /* MOV RAX, [RDI + 48] */
};

/* ============================================================================
 * Constant Loading Stencils
 * ============================================================================ */

/* CONST_INT: RAX = immediate 64-bit */
static const uint8_t stencil_const_int[] = {
    0x48, 0xB8,                    /* MOV RAX, imm64 */
    0x00, 0x00, 0x00, 0x00,        /* imm64 low (hole at offset 2) */
    0x00, 0x00, 0x00, 0x00,        /* imm64 high */
};
static const uint32_t holes_const_int[] = {2};

/* CONST_NUM: XMM0 = immediate double (via RAX) */
static const uint8_t stencil_const_num[] = {
    0x48, 0xB8,                    /* MOV RAX, imm64 */
    0x00, 0x00, 0x00, 0x00,        /* imm64 low (hole at offset 2) */
    0x00, 0x00, 0x00, 0x00,        /* imm64 high */
    0x66, 0x48, 0x0F, 0x6E, 0xC0,  /* MOVQ XMM0, RAX */
};
static const uint32_t holes_const_num[] = {2};

/* ============================================================================
 * Control Flow Stencils
 * ============================================================================ */

/* JUMP: JMP rel32 */
static const uint8_t stencil_jump[] = {
    0xE9, 0x00, 0x00, 0x00, 0x00,  /* JMP rel32 (hole at offset 1) */
};
static const uint32_t holes_jump[] = {1};

/* JUMP_IF: TEST RAX, RAX; JNZ rel32 */
static const uint8_t stencil_jump_if[] = {
    0x48, 0x85, 0xC0,              /* TEST RAX, RAX */
    0x0F, 0x85, 0x00, 0x00, 0x00, 0x00,  /* JNZ rel32 (hole at offset 5) */
};
static const uint32_t holes_jump_if[] = {5};

/* JUMP_UNLESS: TEST RAX, RAX; JZ rel32 */
static const uint8_t stencil_jump_unless[] = {
    0x48, 0x85, 0xC0,              /* TEST RAX, RAX */
    0x0F, 0x84, 0x00, 0x00, 0x00, 0x00,  /* JZ rel32 (hole at offset 5) */
};
static const uint32_t holes_jump_unless[] = {5};

/* RET */
static const uint8_t stencil_ret[] = {
    0xC3,                          /* RET */
};

/* ============================================================================
 * Initialization
 * ============================================================================ */

void jit_stencils_init_x64(void) {
    /* Arithmetic - Double */
    jit_register_stencil(JOP_ADD_DD, stencil_add_dd, sizeof(stencil_add_dd), NULL, 0);
    jit_register_stencil(JOP_SUB_DD, stencil_sub_dd, sizeof(stencil_sub_dd), NULL, 0);
    jit_register_stencil(JOP_MUL_DD, stencil_mul_dd, sizeof(stencil_mul_dd), NULL, 0);
    jit_register_stencil(JOP_DIV_DD, stencil_div_dd, sizeof(stencil_div_dd), NULL, 0);

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
                         holes_const_num, 1);
    jit_register_stencil(JOP_CONST_INT, stencil_const_int, sizeof(stencil_const_int),
                         holes_const_int, 1);

    /* Control Flow */
    jit_register_stencil(JOP_JUMP, stencil_jump, sizeof(stencil_jump), holes_jump, 1);
    jit_register_stencil(JOP_JUMP_IF, stencil_jump_if, sizeof(stencil_jump_if), holes_jump_if, 1);
    jit_register_stencil(JOP_JUMP_UNLESS, stencil_jump_unless, sizeof(stencil_jump_unless),
                         holes_jump_unless, 1);
    jit_register_stencil(JOP_RET, stencil_ret, sizeof(stencil_ret), NULL, 0);
}

#endif /* __x86_64__ */
