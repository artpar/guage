/*
 * Guage Copy-and-Patch JIT Compiler
 *
 * HFT-grade performance through tiered compilation:
 *   Tier 0: Interpreter (cold code)
 *   Tier 1: Copy-and-Patch JIT (hot paths)
 *   Tier 2: LLVM/Cranelift (future - peak performance)
 *
 * Design principles:
 *   - Zero allocation in hot paths
 *   - Inline caching for type stability
 *   - Deoptimization guards for type changes
 *   - Arena-based memory for JIT artifacts
 */

#ifndef GUAGE_JIT_H
#define GUAGE_JIT_H

#include "cell.h"
#include "ffi_jit.h"
#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 * JIT Configuration
 * ============================================================================ */

#define JIT_HOT_THRESHOLD     100    /* Calls before JIT compilation */
#define JIT_TRACE_MAX_LEN     1024   /* Max instructions per trace */
#define JIT_MAX_INLINE_DEPTH  8      /* Max inlining depth */
#define JIT_CODE_ARENA_SIZE   (16 * 1024 * 1024)  /* 16MB code arena */
#define JIT_MAX_CONSTANTS     256    /* Constants pool size */

/* ============================================================================
 * Opcode Definitions (Internal IR)
 * ============================================================================ */

typedef enum {
    /* Arithmetic - operate on raw doubles, no boxing */
    JOP_ADD_DD,         /* D0 = D1 + D2 */
    JOP_SUB_DD,         /* D0 = D1 - D2 */
    JOP_MUL_DD,         /* D0 = D1 * D2 */
    JOP_DIV_DD,         /* D0 = D1 / D2 */
    JOP_MOD_DD,         /* D0 = fmod(D1, D2) */
    JOP_NEG_D,          /* D0 = -D1 */

    /* Integer arithmetic - operate on raw int64, no boxing */
    JOP_ADD_II,         /* I0 = I1 + I2 */
    JOP_SUB_II,         /* I0 = I1 - I2 */
    JOP_MUL_II,         /* I0 = I1 * I2 */
    JOP_DIV_II,         /* I0 = I1 / I2 (signed) */
    JOP_MOD_II,         /* I0 = I1 % I2 */
    JOP_NEG_I,          /* I0 = -I1 */

    /* Comparisons - return raw bool (0/1) */
    JOP_LT_DD,          /* B0 = D1 < D2 */
    JOP_LE_DD,          /* B0 = D1 <= D2 */
    JOP_GT_DD,          /* B0 = D1 > D2 */
    JOP_GE_DD,          /* B0 = D1 >= D2 */
    JOP_EQ_DD,          /* B0 = D1 == D2 */
    JOP_LT_II,          /* B0 = I1 < I2 */
    JOP_LE_II,          /* B0 = I1 <= I2 */
    JOP_GT_II,          /* B0 = I1 > I2 */
    JOP_GE_II,          /* B0 = I1 >= I2 */
    JOP_EQ_II,          /* B0 = I1 == I2 */

    /* Type conversions */
    JOP_I2D,            /* D0 = (double)I1 */
    JOP_D2I,            /* I0 = (int64_t)D1 */

    /* Memory operations - zero-copy Cell access */
    JOP_LOAD_NUM,       /* D0 = cell->number (unbox) */
    JOP_LOAD_INT,       /* I0 = cell->integer (unbox) */
    JOP_LOAD_BOOL,      /* B0 = cell->bool_val */
    JOP_STORE_NUM,      /* cell->number = D0 (box) */
    JOP_STORE_INT,      /* cell->integer = I0 (box) */
    JOP_STORE_BOOL,     /* cell->bool_val = B0 */

    /* Type guards - deopt on failure */
    JOP_GUARD_NUM,      /* deopt if cell->type != NUMBER */
    JOP_GUARD_INT,      /* deopt if cell->type != INTEGER */
    JOP_GUARD_BOOL,     /* deopt if cell->type != BOOL */
    JOP_GUARD_PAIR,     /* deopt if cell->type != PAIR */
    JOP_GUARD_NIL,      /* deopt if cell != nil */

    /* Pair operations */
    JOP_CAR,            /* P0 = cell->car */
    JOP_CDR,            /* P0 = cell->cdr */
    JOP_CONS,           /* P0 = cons(P1, P2) */

    /* Control flow */
    JOP_JUMP,           /* unconditional jump */
    JOP_JUMP_IF,        /* jump if B0 true */
    JOP_JUMP_UNLESS,    /* jump if B0 false */
    JOP_CALL,           /* call JIT'd function */
    JOP_CALL_INTERP,    /* fall back to interpreter */
    JOP_RET,            /* return from JIT'd function */
    JOP_DEOPT,          /* deoptimize and return to interpreter */

    /* Environment access (De Bruijn) */
    JOP_ENV_LOAD,       /* P0 = env[depth][index] */
    JOP_ENV_STORE,      /* env[depth][index] = P0 */

    /* Constants */
    JOP_CONST_NUM,      /* D0 = immediate double */
    JOP_CONST_INT,      /* I0 = immediate int64 */
    JOP_CONST_BOOL,     /* B0 = immediate bool */
    JOP_CONST_NIL,      /* P0 = nil */
    JOP_CONST_CELL,     /* P0 = cell from constant pool */

    /* Box operations (mutable state) */
    JOP_BOX_NEW,        /* P0 = new box(P1) */
    JOP_BOX_READ,       /* P0 = unbox(P1) */
    JOP_BOX_WRITE,      /* box_set!(P0, P1) */

    JOP_COUNT
} JITOpcode;

/* ============================================================================
 * JIT Instruction
 * ============================================================================ */

typedef struct {
    JITOpcode op;
    uint8_t   dst;      /* Destination register (0-15) */
    uint8_t   src1;     /* Source register 1 */
    uint8_t   src2;     /* Source register 2 */
    uint8_t   flags;    /* Instruction flags */
    union {
        double    num_const;
        int64_t   int_const;
        uint32_t  jump_offset;
        uint32_t  const_index;
        struct {
            uint8_t depth;
            uint8_t index;
        } env;
    } imm;
} JITInst;

/* ============================================================================
 * JIT Trace - A compiled code path
 * ============================================================================ */

typedef struct JITTrace {
    JITInst*  insts;        /* Instruction array */
    uint32_t  n_insts;      /* Number of instructions */
    uint32_t  capacity;     /* Allocated capacity */

    void*     native_code;  /* JIT'd machine code */
    size_t    code_size;    /* Machine code size */

    Cell**    constants;    /* Constant pool */
    uint32_t  n_constants;

    Cell*     root_expr;    /* Original expression (for deopt) */
    uint32_t  exec_count;   /* Execution counter */

    struct JITTrace* next;  /* Linked list for cleanup */
} JITTrace;

/* ============================================================================
 * JIT Function Signature
 * ============================================================================ */

/* JIT'd function: takes environment, returns Cell* result */
typedef Cell* (*JITFunction)(Cell* env);

/* ============================================================================
 * JIT Compiler State
 * ============================================================================ */

typedef struct {
    /* Code arena - bump allocator for JIT code */
    uint8_t*  code_arena;
    size_t    code_arena_pos;
    size_t    code_arena_size;

    /* Trace storage */
    JITTrace* traces;
    uint32_t  trace_count;

    /* Hot function tracking */
    struct {
        Cell*     expr;
        uint32_t  count;
        JITTrace* trace;
    } hot_functions[1024];
    uint32_t hot_count;

    /* Statistics */
    uint64_t  total_compiles;
    uint64_t  total_deopts;
    uint64_t  total_native_calls;

    /* Enabled flag */
    bool      enabled;
} JITCompiler;

/* Global JIT compiler instance */
extern JITCompiler g_jit_compiler;

/* ============================================================================
 * JIT API
 * ============================================================================ */

/* Initialize/shutdown */
void jit_init(void);
void jit_shutdown(void);

/* Enable/disable JIT */
void jit_enable(void);
void jit_disable(void);
bool jit_is_enabled(void);

/* Compile expression to JIT trace */
JITTrace* jit_compile(Cell* expr);

/* Execute JIT trace */
Cell* jit_execute(JITTrace* trace, Cell* env);

/* Record function call for hot tracking */
void jit_record_call(Cell* expr);

/* Get or compile JIT trace for expression */
JITTrace* jit_get_trace(Cell* expr);

/* Deoptimize and return to interpreter */
Cell* jit_deopt(JITTrace* trace, Cell* env, uint32_t pc);

/* ============================================================================
 * Stencil API (platform-specific)
 * ============================================================================ */

/* Stencil descriptor */
typedef struct {
    const uint8_t* code;      /* Pre-compiled machine code */
    size_t         size;      /* Code size in bytes */
    uint32_t       holes[8];  /* Offsets of holes to patch */
    uint8_t        n_holes;   /* Number of holes */
} JITStencil;

/* Get stencil for opcode */
const JITStencil* jit_get_stencil(JITOpcode op);

/* Emit stencil to code buffer, patching holes */
size_t jit_emit_stencil(EmitCtx* ctx, const JITStencil* stencil,
                        uint64_t* patch_values);

/* Platform-specific stencil initialization */
void jit_stencils_init_a64(void);
void jit_stencils_init_x64(void);

/* ============================================================================
 * Statistics
 * ============================================================================ */

typedef struct {
    uint64_t compiles;
    uint64_t deopts;
    uint64_t native_calls;
    uint64_t interp_calls;
    uint64_t code_bytes;
    uint64_t traces;
} JITStats;

void jit_get_stats(JITStats* stats);
void jit_print_stats(void);

#endif /* GUAGE_JIT_H */
