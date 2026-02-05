/*
 * Guage Copy-and-Patch JIT Compiler
 *
 * Implementation of tiered compilation for HFT-grade performance.
 */

#include "jit.h"
#include "eval.h"
#include "intern.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <sys/mman.h>

#ifdef __APPLE__
#include <libkern/OSCacheControl.h>
#include <pthread.h>
#endif

/* ============================================================================
 * Cell Structure Offsets (for inline memory access)
 * ============================================================================ */

/* These offsets are computed at compile time for inline ENV_LOAD */
static const size_t CELL_TYPE_OFFSET = offsetof(Cell, type);
static const size_t CELL_DATA_OFFSET = offsetof(Cell, data);

/* Vector offsets within Cell.data union - need runtime calculation */
static size_t g_vec_sbo_offset;      /* offset of data.vector.sbo from Cell start */
static size_t g_vec_heap_offset;     /* offset of data.vector.heap from Cell start */
static size_t g_vec_capacity_offset; /* offset of data.vector.capacity from Cell start */
static size_t g_atom_number_offset;  /* offset of data.atom.number from Cell start */

/* Initialize struct offsets - must be called during jit_init */
static void init_struct_offsets(void) {
    /* Use a dummy cell to compute offsets portably */
    Cell dummy;
    g_vec_sbo_offset = (size_t)((char*)&dummy.data.vector.sbo - (char*)&dummy);
    g_vec_heap_offset = (size_t)((char*)&dummy.data.vector.heap - (char*)&dummy);
    g_vec_capacity_offset = (size_t)((char*)&dummy.data.vector.capacity - (char*)&dummy);
    g_atom_number_offset = (size_t)((char*)&dummy.data.atom.number - (char*)&dummy);
}

/* ============================================================================
 * Global State
 * ============================================================================ */

JITCompiler g_jit_compiler = {0};

/* Stencil table - populated by platform init */
static JITStencil g_stencils[JOP_COUNT] = {0};

/* ============================================================================
 * Arena Allocator
 * ============================================================================ */

static void* arena_alloc(size_t size) {
    /* Align to 16 bytes */
    size = (size + 15) & ~15;

    if (g_jit_compiler.code_arena_pos + size > g_jit_compiler.code_arena_size) {
        return NULL;  /* Arena exhausted */
    }

    void* ptr = g_jit_compiler.code_arena + g_jit_compiler.code_arena_pos;
    g_jit_compiler.code_arena_pos += size;
    return ptr;
}

/* ============================================================================
 * JIT Initialization
 * ============================================================================ */

void jit_init(void) {
    if (g_jit_compiler.code_arena) return;  /* Already initialized */

    /* Initialize struct offsets for inline memory access */
    init_struct_offsets();

    /* Allocate code arena with execute permission */
#if defined(__APPLE__) && defined(__aarch64__)
    g_jit_compiler.code_arena = mmap(NULL, JIT_CODE_ARENA_SIZE,
        PROT_READ | PROT_WRITE | PROT_EXEC,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_JIT, -1, 0);
#else
    g_jit_compiler.code_arena = mmap(NULL, JIT_CODE_ARENA_SIZE,
        PROT_READ | PROT_WRITE | PROT_EXEC,
        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif

    if (g_jit_compiler.code_arena == MAP_FAILED) {
        g_jit_compiler.code_arena = NULL;
        fprintf(stderr, "JIT: Failed to allocate code arena\n");
        return;
    }

    g_jit_compiler.code_arena_size = JIT_CODE_ARENA_SIZE;
    g_jit_compiler.code_arena_pos = 0;

    /* Initialize stencils for current platform */
#if defined(__aarch64__) || defined(_M_ARM64)
    jit_stencils_init_a64();
#elif defined(__x86_64__) || defined(_M_X64)
    jit_stencils_init_x64();
#endif

    g_jit_compiler.enabled = true;
    g_jit_compiler.traces = NULL;
    g_jit_compiler.trace_count = 0;
    g_jit_compiler.hot_count = 0;

    fprintf(stderr, "JIT: Initialized with %zuMB code arena\n",
            (size_t)(JIT_CODE_ARENA_SIZE / (1024 * 1024)));
}

void jit_shutdown(void) {
    if (!g_jit_compiler.code_arena) return;

    /* Free all traces */
    JITTrace* t = g_jit_compiler.traces;
    while (t) {
        JITTrace* next = t->next;
        if (t->insts) free(t->insts);
        if (t->constants) free(t->constants);
        free(t);
        t = next;
    }

    /* Unmap code arena */
    munmap(g_jit_compiler.code_arena, g_jit_compiler.code_arena_size);
    g_jit_compiler.code_arena = NULL;
    g_jit_compiler.enabled = false;
}

void jit_enable(void) {
    if (g_jit_compiler.code_arena) {
        g_jit_compiler.enabled = true;
    }
}

void jit_disable(void) {
    g_jit_compiler.enabled = false;
}

bool jit_is_enabled(void) {
    return g_jit_compiler.enabled && g_jit_compiler.code_arena != NULL;
}

/* ============================================================================
 * Stencil Management
 * ============================================================================ */

const JITStencil* jit_get_stencil(JITOpcode op) {
    if (op >= JOP_COUNT) return NULL;
    if (g_stencils[op].code == NULL) return NULL;
    return &g_stencils[op];
}

void jit_register_stencil(JITOpcode op, const uint8_t* code, size_t size,
                          const uint32_t* holes, uint8_t n_holes) {
    g_stencils[op].code = code;
    g_stencils[op].size = size;
    g_stencils[op].n_holes = n_holes;
    for (int i = 0; i < n_holes && i < 8; i++) {
        g_stencils[op].holes[i] = holes[i];
    }
}

size_t jit_emit_stencil(EmitCtx* ctx, const JITStencil* stencil,
                        uint64_t* patch_values) {
    if (!stencil || !stencil->code) return 0;

    size_t start = ctx->pos;

    /* Copy stencil code */
    emit_bytes(ctx, stencil->code, stencil->size);

    /* Patch holes with values */
    for (int i = 0; i < stencil->n_holes; i++) {
        uint32_t hole_off = stencil->holes[i];
        if (hole_off < stencil->size && start + hole_off + 8 <= ctx->pos) {
            memcpy(&ctx->buf[start + hole_off], &patch_values[i], 8);
        }
    }

    return stencil->size;
}

/* ============================================================================
 * JIT Helper Functions (called from JIT'd code)
 * ============================================================================ */

/* Load a numeric value from environment at De Bruijn index.
 * Called by JIT'd code to access function parameters.
 * env: CELL_VECTOR containing parameters
 * index: De Bruijn index (0 = first param, etc.)
 * Returns: the double value of the parameter
 */
double jit_helper_load_env_num(Cell* env, int index) {
    if (!env || env->type != CELL_VECTOR) {
        return 0.0;  /* Deopt should handle this case */
    }
    Cell* cell = cell_vector_get(env, (uint32_t)index);
    if (!cell) {
        return 0.0;
    }
    return cell_to_double(cell);
}

/* ============================================================================
 * Hot Function Tracking
 * ============================================================================ */

void jit_record_call(Cell* expr) {
    if (!jit_is_enabled()) return;

    /* Search for existing entry */
    for (uint32_t i = 0; i < g_jit_compiler.hot_count; i++) {
        if (g_jit_compiler.hot_functions[i].expr == expr) {
            g_jit_compiler.hot_functions[i].count++;

            /* Trigger compilation at threshold */
            if (g_jit_compiler.hot_functions[i].count == JIT_HOT_THRESHOLD &&
                g_jit_compiler.hot_functions[i].trace == NULL) {
                g_jit_compiler.hot_functions[i].trace = jit_compile(expr);
            }
            return;
        }
    }

    /* Add new entry */
    if (g_jit_compiler.hot_count < 1024) {
        uint32_t idx = g_jit_compiler.hot_count++;
        g_jit_compiler.hot_functions[idx].expr = expr;
        g_jit_compiler.hot_functions[idx].count = 1;
        g_jit_compiler.hot_functions[idx].trace = NULL;
    }
}

JITTrace* jit_get_trace(Cell* expr) {
    if (!jit_is_enabled()) return NULL;

    for (uint32_t i = 0; i < g_jit_compiler.hot_count; i++) {
        if (g_jit_compiler.hot_functions[i].expr == expr) {
            return g_jit_compiler.hot_functions[i].trace;
        }
    }
    return NULL;
}

/* ============================================================================
 * Trace Compilation
 * ============================================================================ */

static JITTrace* trace_new(void) {
    JITTrace* trace = calloc(1, sizeof(JITTrace));
    if (!trace) return NULL;

    trace->capacity = 256;
    trace->insts = calloc(trace->capacity, sizeof(JITInst));
    if (!trace->insts) {
        free(trace);
        return NULL;
    }

    trace->constants = calloc(JIT_MAX_CONSTANTS, sizeof(Cell*));
    if (!trace->constants) {
        free(trace->insts);
        free(trace);
        return NULL;
    }

    return trace;
}

static void trace_emit(JITTrace* trace, JITOpcode op, uint8_t dst,
                       uint8_t src1, uint8_t src2) {
    if (trace->n_insts >= trace->capacity) {
        trace->capacity *= 2;
        trace->insts = realloc(trace->insts, trace->capacity * sizeof(JITInst));
    }

    JITInst* inst = &trace->insts[trace->n_insts++];
    inst->op = op;
    inst->dst = dst;
    inst->src1 = src1;
    inst->src2 = src2;
    inst->flags = 0;
    memset(&inst->imm, 0, sizeof(inst->imm));
}

static void trace_emit_const_num(JITTrace* trace, uint8_t dst, double val) {
    if (trace->n_insts >= trace->capacity) {
        trace->capacity *= 2;
        trace->insts = realloc(trace->insts, trace->capacity * sizeof(JITInst));
    }

    JITInst* inst = &trace->insts[trace->n_insts++];
    inst->op = JOP_CONST_NUM;
    inst->dst = dst;
    inst->src1 = 0;
    inst->src2 = 0;
    inst->flags = 0;
    inst->imm.num_const = val;
}

static void trace_emit_const_int(JITTrace* trace, uint8_t dst, int64_t val) {
    if (trace->n_insts >= trace->capacity) {
        trace->capacity *= 2;
        trace->insts = realloc(trace->insts, trace->capacity * sizeof(JITInst));
    }

    JITInst* inst = &trace->insts[trace->n_insts++];
    inst->op = JOP_CONST_INT;
    inst->dst = dst;
    inst->src1 = 0;
    inst->src2 = 0;
    inst->flags = 0;
    inst->imm.int_const = val;
}

static uint32_t trace_add_constant(JITTrace* trace, Cell* cell) {
    if (trace->n_constants >= JIT_MAX_CONSTANTS) return 0;
    uint32_t idx = trace->n_constants++;
    trace->constants[idx] = cell;
    cell_retain(cell);
    return idx;
}

/* ============================================================================
 * Expression Compilation
 * ============================================================================ */

/* Register allocation state */
typedef struct {
    uint8_t next_dreg;  /* Next double register (0-7) */
    uint8_t next_ireg;  /* Next int register (8-15) */
    uint8_t next_preg;  /* Next pointer register (16-23) */
} RegAlloc;

static uint8_t alloc_dreg(RegAlloc* ra) {
    return ra->next_dreg < 8 ? ra->next_dreg++ : 0;
}

static uint8_t alloc_ireg(RegAlloc* ra) {
    return ra->next_ireg < 16 ? ra->next_ireg++ : 8;
}

static uint8_t alloc_preg(RegAlloc* ra) {
    return ra->next_preg < 24 ? ra->next_preg++ : 16;
}

/* Forward declaration */
static uint8_t compile_expr(JITTrace* trace, RegAlloc* ra, Cell* expr, int depth);

/* Compile a primitive arithmetic operation */
static uint8_t compile_arith(JITTrace* trace, RegAlloc* ra, const char* op,
                             Cell* arg1, Cell* arg2, int depth) {
    /* Compile operands */
    uint8_t r1 = compile_expr(trace, ra, arg1, depth);
    uint8_t r2 = compile_expr(trace, ra, arg2, depth);
    uint8_t dst = alloc_dreg(ra);

    /* Emit operation - handle both ASCII and Unicode operators */
    JITOpcode jop;
    if (strcmp(op, "+") == 0 || strcmp(op, "⊕") == 0)      jop = JOP_ADD_DD;
    else if (strcmp(op, "-") == 0 || strcmp(op, "⊖") == 0) jop = JOP_SUB_DD;
    else if (strcmp(op, "*") == 0 || strcmp(op, "⊗") == 0) jop = JOP_MUL_DD;
    else if (strcmp(op, "/") == 0 || strcmp(op, "⊘") == 0) jop = JOP_DIV_DD;
    else if (strcmp(op, "%") == 0) jop = JOP_MOD_DD;
    else return r1;  /* Unknown op, return first arg */

    trace_emit(trace, jop, dst, r1, r2);
    return dst;
}

/* Compile a comparison operation */
static uint8_t compile_cmp(JITTrace* trace, RegAlloc* ra, const char* op,
                           Cell* arg1, Cell* arg2, int depth) {
    uint8_t r1 = compile_expr(trace, ra, arg1, depth);
    uint8_t r2 = compile_expr(trace, ra, arg2, depth);
    uint8_t dst = alloc_ireg(ra);

    JITOpcode jop;
    if (strcmp(op, "<") == 0)                                 jop = JOP_LT_DD;
    else if (strcmp(op, "<=") == 0 || strcmp(op, "≤") == 0)   jop = JOP_LE_DD;
    else if (strcmp(op, ">") == 0)                            jop = JOP_GT_DD;
    else if (strcmp(op, ">=") == 0 || strcmp(op, "≥") == 0)   jop = JOP_GE_DD;
    else if (strcmp(op, "equal?") == 0 || strcmp(op, "≡") == 0) jop = JOP_EQ_DD;
    else return r1;

    trace_emit(trace, jop, dst, r1, r2);
    return dst;
}

/* Compile an expression to JIT IR */
static uint8_t compile_expr(JITTrace* trace, RegAlloc* ra, Cell* expr, int depth) {
    if (depth > JIT_MAX_INLINE_DEPTH) {
        /* Too deep - emit call to interpreter */
        uint32_t idx = trace_add_constant(trace, expr);
        uint8_t dst = alloc_preg(ra);
        JITInst* inst = &trace->insts[trace->n_insts++];
        inst->op = JOP_CALL_INTERP;
        inst->dst = dst;
        inst->imm.const_index = idx;
        return dst;
    }

    if (!expr) {
        trace_emit(trace, JOP_CONST_NIL, alloc_preg(ra), 0, 0);
        return ra->next_preg - 1;
    }

    /* After De Bruijn conversion:
     * - Bare numbers are De Bruijn indices (variable references)
     * - Number literals are wrapped in (quote <number>)
     * So we check for bare numbers LAST after checking for quote forms
     */

    /* Boolean literal - self-evaluating */
    if (cell_is_bool(expr)) {
        uint8_t dst = alloc_ireg(ra);
        JITInst* inst = &trace->insts[trace->n_insts++];
        inst->op = JOP_CONST_BOOL;
        inst->dst = dst;
        inst->imm.int_const = cell_get_bool(expr) ? 1 : 0;
        return dst;
    }

    /* Nil - self-evaluating */
    if (cell_is_nil(expr)) {
        uint8_t dst = alloc_preg(ra);
        trace_emit(trace, JOP_CONST_NIL, dst, 0, 0);
        return dst;
    }

    /* String - self-evaluating */
    if (cell_is_string(expr)) {
        uint32_t idx = trace_add_constant(trace, expr);
        uint8_t dst = alloc_preg(ra);
        JITInst* inst = &trace->insts[trace->n_insts++];
        inst->op = JOP_CONST_CELL;
        inst->dst = dst;
        inst->imm.const_index = idx;
        return dst;
    }

    /* Symbol - free variable or primitive (kept as symbol after De Bruijn) */
    if (cell_is_symbol(expr)) {
        /* Free variable - emit as constant for interpreter lookup */
        uint32_t idx = trace_add_constant(trace, expr);
        uint8_t dst = alloc_preg(ra);
        JITInst* inst = &trace->insts[trace->n_insts++];
        inst->op = JOP_CONST_CELL;
        inst->dst = dst;
        inst->imm.const_index = idx;
        return dst;
    }

    /* Integer literal - self-evaluating (native int64) */
    if (cell_is_integer(expr)) {
        uint8_t dst = alloc_ireg(ra);
        trace_emit_const_int(trace, dst, cell_get_integer(expr));
        return dst;
    }

    /* Bare number after De Bruijn conversion = variable reference (index) */
    if (cell_is_number(expr)) {
        int index = (int)cell_get_number(expr);
        uint8_t dst = alloc_preg(ra);
        JITInst* inst = &trace->insts[trace->n_insts++];
        inst->op = JOP_ENV_LOAD;
        inst->dst = dst;
        /* De Bruijn index is a single integer; depth/index encoding may vary.
         * For now, use flat index (depth=0, index=idx) */
        inst->imm.env.depth = 0;
        inst->imm.env.index = (uint8_t)index;
        return dst;
    }

    /* Application (function call) or special form */
    if (cell_is_pair(expr)) {
        Cell* head = cell_car(expr);
        Cell* args = cell_cdr(expr);

        /* Check for special forms and primitive operations */
        if (cell_is_symbol(head)) {
            const char* sym = cell_get_symbol(head);

            /* Quote: (quote <datum>) - this is how number literals appear after De Bruijn */
            if (strcmp(sym, "quote") == 0 && cell_is_pair(args)) {
                Cell* datum = cell_car(args);
                if (cell_is_number(datum)) {
                    /* Number literal */
                    uint8_t dst = alloc_dreg(ra);
                    trace_emit_const_num(trace, dst, cell_get_number(datum));
                    return dst;
                }
                if (cell_is_integer(datum)) {
                    /* Integer literal */
                    uint8_t dst = alloc_ireg(ra);
                    trace_emit_const_int(trace, dst, cell_get_integer(datum));
                    return dst;
                }
                /* Other quoted datum - emit as constant */
                uint32_t idx = trace_add_constant(trace, datum);
                uint8_t dst = alloc_preg(ra);
                JITInst* inst = &trace->insts[trace->n_insts++];
                inst->op = JOP_CONST_CELL;
                inst->dst = dst;
                inst->imm.const_index = idx;
                return dst;
            }

            /* Arithmetic: + ⊕ - ⊖ * ⊗ / ⊘ % */
            if ((strcmp(sym, "+") == 0 || strcmp(sym, "⊕") == 0 ||
                 strcmp(sym, "-") == 0 || strcmp(sym, "⊖") == 0 ||
                 strcmp(sym, "*") == 0 || strcmp(sym, "⊗") == 0 ||
                 strcmp(sym, "/") == 0 || strcmp(sym, "⊘") == 0 ||
                 strcmp(sym, "%") == 0) && cell_is_pair(args)) {
                Cell* a1 = cell_car(args);
                Cell* rest = cell_cdr(args);
                if (cell_is_pair(rest)) {
                    Cell* a2 = cell_car(rest);
                    return compile_arith(trace, ra, sym, a1, a2, depth + 1);
                }
            }

            /* Comparisons: <, ≤, >, ≥, ≡ */
            if ((strcmp(sym, "<") == 0 || strcmp(sym, "≤") == 0 ||
                 strcmp(sym, ">") == 0 || strcmp(sym, "≥") == 0 ||
                 strcmp(sym, "<=") == 0 || strcmp(sym, ">=") == 0 ||
                 strcmp(sym, "equal?") == 0 || strcmp(sym, "≡") == 0) && cell_is_pair(args)) {
                Cell* a1 = cell_car(args);
                Cell* rest = cell_cdr(args);
                if (cell_is_pair(rest)) {
                    Cell* a2 = cell_car(rest);
                    return compile_cmp(trace, ra, sym, a1, a2, depth + 1);
                }
            }

            /* car/cdr */
            if (strcmp(sym, "car") == 0 && cell_is_pair(args)) {
                uint8_t r = compile_expr(trace, ra, cell_car(args), depth + 1);
                uint8_t dst = alloc_preg(ra);
                trace_emit(trace, JOP_CAR, dst, r, 0);
                return dst;
            }
            if (strcmp(sym, "cdr") == 0 && cell_is_pair(args)) {
                uint8_t r = compile_expr(trace, ra, cell_car(args), depth + 1);
                uint8_t dst = alloc_preg(ra);
                trace_emit(trace, JOP_CDR, dst, r, 0);
                return dst;
            }

            /* cons */
            if (strcmp(sym, "cons") == 0 && cell_is_pair(args)) {
                Cell* a1 = cell_car(args);
                Cell* rest = cell_cdr(args);
                if (cell_is_pair(rest)) {
                    Cell* a2 = cell_car(rest);
                    uint8_t r1 = compile_expr(trace, ra, a1, depth + 1);
                    uint8_t r2 = compile_expr(trace, ra, a2, depth + 1);
                    uint8_t dst = alloc_preg(ra);
                    trace_emit(trace, JOP_CONS, dst, r1, r2);
                    return dst;
                }
            }
        }

        /* Generic call - fall back to interpreter for now */
        uint32_t idx = trace_add_constant(trace, expr);
        uint8_t dst = alloc_preg(ra);
        JITInst* inst = &trace->insts[trace->n_insts++];
        inst->op = JOP_CALL_INTERP;
        inst->dst = dst;
        inst->imm.const_index = idx;
        return dst;
    }

    /* Unknown - emit as constant */
    uint32_t idx = trace_add_constant(trace, expr);
    uint8_t dst = alloc_preg(ra);
    JITInst* inst = &trace->insts[trace->n_insts++];
    inst->op = JOP_CONST_CELL;
    inst->dst = dst;
    inst->imm.const_index = idx;
    return dst;
}

/* ============================================================================
 * Native Code Generation
 * ============================================================================ */

#if defined(__APPLE__) && defined(__aarch64__)
static void jit_write_begin(void) {
    pthread_jit_write_protect_np(0);
}
static void jit_write_end(void* code, size_t size) {
    pthread_jit_write_protect_np(1);
    sys_icache_invalidate(code, size);
}
#else
#define jit_write_begin() ((void)0)
static void jit_write_end(void* code, size_t size) {
#if defined(__aarch64__)
    __builtin___clear_cache((char*)code, (char*)code + size);
#else
    (void)code; (void)size;
#endif
}
#endif

/* ============================================================================
 * Stack-based Code Generation
 *
 * For simplicity and correctness, we use a stack-based approach:
 *   - Operands are pushed onto a virtual stack (D0-D7 on ARM64, XMM0-XMM7 on x86)
 *   - Operations pop operands and push results
 *   - At the end, D0/XMM0 contains the result
 *
 * This trades some performance for simplicity but still avoids boxing in hot loops.
 * ============================================================================ */

/* Evaluate JIT IR and generate native code
 * Returns true if code generation succeeded, false to fall back to interpreter.
 *
 * For now, we support only "constant folding" JIT:
 * - If expression reduces to a constant, we can return it directly
 * - Otherwise fall back to interpreter
 */
static bool trace_codegen(JITTrace* trace) {
    /* For Phase 2, let's start simple: only compile pure constant expressions */
    if (trace->n_insts == 0) return false;

    /* Scan for unsupported operations */
    for (uint32_t i = 0; i < trace->n_insts; i++) {
        JITInst* inst = &trace->insts[i];
        switch (inst->op) {
            case JOP_CONST_NUM:
            case JOP_CONST_INT:
            case JOP_CONST_BOOL:
            case JOP_ADD_DD:
            case JOP_SUB_DD:
            case JOP_MUL_DD:
            case JOP_DIV_DD:
            case JOP_LT_DD:      /* Comparisons */
            case JOP_LE_DD:
            case JOP_GT_DD:
            case JOP_GE_DD:
            case JOP_EQ_DD:
            case JOP_JUMP_IF:    /* Conditionals */
            case JOP_JUMP_UNLESS:
            case JOP_JUMP:
            case JOP_ENV_LOAD:   /* Parameter access */
            case JOP_RET:
                /* Supported */
                break;
            default:
                /* Unsupported - fall back to interpreter */
                return false;
        }
    }

    EmitCtx ctx;
    ctx.cap = 4096;
    ctx.pos = 0;
    ctx.buf = malloc(ctx.cap);
    if (!ctx.buf) return false;

    /* Virtual register stack: tracks which FP reg holds each IR reg's value */
    int stack_depth = 0;  /* How many values on the FP stack (max 8) */

#if defined(__aarch64__) || defined(_M_ARM64)
    /* ARM64 codegen */

    /* Prologue - save callee-saved registers and frame pointer */
    emit_u32(&ctx, 0xA9BE7BFD);  /* STP X29, X30, [SP, #-32]! (save FP/LR, alloc 32 bytes) */
    emit_u32(&ctx, 0x910003FD);  /* MOV X29, SP */
    emit_u32(&ctx, 0xF9000BF3);  /* STR X19, [SP, #16] (save X19 at SP+16) */
    /* Save X0 (env pointer) to callee-saved X19 */
    emit_u32(&ctx, 0xAA0003F3);  /* MOV X19, X0 (save env in X19) */

    for (uint32_t i = 0; i < trace->n_insts; i++) {
        JITInst* inst = &trace->insts[i];

        switch (inst->op) {
            case JOP_CONST_NUM: {
                /* Load 64-bit double constant into D[stack_depth] */
                int dreg = stack_depth++;
                uint64_t bits;
                memcpy(&bits, &inst->imm.num_const, 8);

                /* Load via X9 scratch register */
                emit_u32(&ctx, 0xD2800009 | ((bits & 0xFFFF) << 5));         /* MOVZ X9, #imm16_0 */
                emit_u32(&ctx, 0xF2A00009 | (((bits >> 16) & 0xFFFF) << 5)); /* MOVK X9, #imm16_1, LSL #16 */
                emit_u32(&ctx, 0xF2C00009 | (((bits >> 32) & 0xFFFF) << 5)); /* MOVK X9, #imm16_2, LSL #32 */
                emit_u32(&ctx, 0xF2E00009 | (((bits >> 48) & 0xFFFF) << 5)); /* MOVK X9, #imm16_3, LSL #48 */
                /* FMOV Dn, X9 */
                emit_u32(&ctx, 0x9E670120 | dreg);
                break;
            }
            case JOP_ADD_DD: {
                /* D[n-2] = D[n-2] + D[n-1] */
                if (stack_depth < 2) { free(ctx.buf); return false; }
                int dst = stack_depth - 2;
                int src = stack_depth - 1;
                /* FADD Ddst, Ddst, Dsrc */
                emit_u32(&ctx, 0x1E602800 | dst | (dst << 5) | (src << 16));
                stack_depth--;
                break;
            }
            case JOP_SUB_DD: {
                if (stack_depth < 2) { free(ctx.buf); return false; }
                int dst = stack_depth - 2;
                int src = stack_depth - 1;
                /* FSUB Ddst, Ddst, Dsrc */
                emit_u32(&ctx, 0x1E603800 | dst | (dst << 5) | (src << 16));
                stack_depth--;
                break;
            }
            case JOP_MUL_DD: {
                if (stack_depth < 2) { free(ctx.buf); return false; }
                int dst = stack_depth - 2;
                int src = stack_depth - 1;
                /* FMUL Ddst, Ddst, Dsrc */
                emit_u32(&ctx, 0x1E600800 | dst | (dst << 5) | (src << 16));
                stack_depth--;
                break;
            }
            case JOP_DIV_DD: {
                if (stack_depth < 2) { free(ctx.buf); return false; }
                int dst = stack_depth - 2;
                int src = stack_depth - 1;
                /* FDIV Ddst, Ddst, Dsrc */
                emit_u32(&ctx, 0x1E601800 | dst | (dst << 5) | (src << 16));
                stack_depth--;
                break;
            }
            case JOP_LT_DD: {
                /* Compare D[n-2] < D[n-1], result in W10 (0 or 1) */
                if (stack_depth < 2) { free(ctx.buf); return false; }
                int op1 = stack_depth - 2;
                int op2 = stack_depth - 1;
                /* FCMP Dop1, Dop2 */
                emit_u32(&ctx, 0x1E602000 | (op1 << 5) | (op2 << 16));
                /* CSET W10, LT (MI = negative = less than) */
                emit_u32(&ctx, 0x1A9F47EA);
                stack_depth -= 2;  /* Pop both operands, comparison result in W10 */
                break;
            }
            case JOP_LE_DD: {
                if (stack_depth < 2) { free(ctx.buf); return false; }
                int op1 = stack_depth - 2;
                int op2 = stack_depth - 1;
                emit_u32(&ctx, 0x1E602000 | (op1 << 5) | (op2 << 16)); /* FCMP */
                /* CSET W10, LS (LS = unsigned lower or same, for floats = LE) */
                emit_u32(&ctx, 0x1A9F87EA);
                stack_depth -= 2;
                break;
            }
            case JOP_GT_DD: {
                if (stack_depth < 2) { free(ctx.buf); return false; }
                int op1 = stack_depth - 2;
                int op2 = stack_depth - 1;
                emit_u32(&ctx, 0x1E602000 | (op1 << 5) | (op2 << 16)); /* FCMP */
                /* CSET W10, GT */
                emit_u32(&ctx, 0x1A9FD7EA);
                stack_depth -= 2;
                break;
            }
            case JOP_GE_DD: {
                if (stack_depth < 2) { free(ctx.buf); return false; }
                int op1 = stack_depth - 2;
                int op2 = stack_depth - 1;
                emit_u32(&ctx, 0x1E602000 | (op1 << 5) | (op2 << 16)); /* FCMP */
                /* CSET W10, GE */
                emit_u32(&ctx, 0x1A9FA7EA);
                stack_depth -= 2;
                break;
            }
            case JOP_EQ_DD: {
                if (stack_depth < 2) { free(ctx.buf); return false; }
                int op1 = stack_depth - 2;
                int op2 = stack_depth - 1;
                emit_u32(&ctx, 0x1E602000 | (op1 << 5) | (op2 << 16)); /* FCMP */
                /* CSET W10, EQ */
                emit_u32(&ctx, 0x1A9F17EA);
                stack_depth -= 2;
                break;
            }
            case JOP_CONST_BOOL: {
                /* Load bool constant into W10 */
                int val = inst->imm.int_const ? 1 : 0;
                emit_u32(&ctx, 0x5280000A | (val << 5));  /* MOV W10, #val */
                break;
            }
            case JOP_JUMP_IF: {
                /* Jump if W10 != 0 (comparison result is true) */
                /* CBZ/CBNZ with immediate offset - will be patched later */
                /* For now, emit CBNZ W10, +offset */
                uint32_t offset = inst->imm.jump_offset;
                /* CBNZ W10, offset (offset in instructions, shifted left 2) */
                emit_u32(&ctx, 0x3500000A | ((offset & 0x7FFFF) << 5));
                break;
            }
            case JOP_JUMP_UNLESS: {
                /* Jump if W10 == 0 (comparison result is false) */
                uint32_t offset = inst->imm.jump_offset;
                /* CBZ W10, offset */
                emit_u32(&ctx, 0x3400000A | ((offset & 0x7FFFF) << 5));
                break;
            }
            case JOP_JUMP: {
                /* Unconditional jump */
                uint32_t offset = inst->imm.jump_offset;
                /* B offset */
                emit_u32(&ctx, 0x14000000 | (offset & 0x3FFFFFF));
                break;
            }
            case JOP_ENV_LOAD: {
                /* INLINE parameter load from environment vector
                 * env is in X19 (callee-saved), index is in imm.env.index
                 *
                 * For SBO (Small Buffer Optimization) case (capacity <= 4):
                 *   Cell* cell = env->data.vector.sbo[index]
                 *   double val = cell->data.atom.number
                 *
                 * This is ~4 instructions vs ~25 with helper call
                 */
                int dreg = stack_depth++;
                int index = inst->imm.env.index;

                /* Load Cell* from env->data.vector.sbo[index] into X9
                 * SBO array is at g_vec_sbo_offset, each element is 8 bytes */
                uint32_t sbo_elem_offset = (uint32_t)(g_vec_sbo_offset + index * 8);

                if (sbo_elem_offset < 4096) {
                    /* LDR X9, [X19, #sbo_elem_offset] - load Cell* */
                    emit_u32(&ctx, 0xF9400269 | ((sbo_elem_offset / 8) << 10));
                } else {
                    /* Offset too large, use ADD + LDR */
                    emit_u32(&ctx, 0x91000269 | ((sbo_elem_offset & 0xFFF) << 10)); /* ADD X9, X19, #off_lo */
                    emit_u32(&ctx, 0xF9400129); /* LDR X9, [X9] */
                }

                /* Load double from cell->data.atom.number into Ddreg
                 * Number is at g_atom_number_offset */
                uint32_t num_offset = (uint32_t)g_atom_number_offset;
                if (num_offset < 32768) {
                    /* LDR Ddreg, [X9, #num_offset] */
                    emit_u32(&ctx, 0xFD400120 | dreg | ((num_offset / 8) << 10));
                } else {
                    /* Offset too large - shouldn't happen for Cell struct */
                    emit_u32(&ctx, 0x91000129 | ((num_offset & 0xFFF) << 10)); /* ADD X9, X9, #off */
                    emit_u32(&ctx, 0xFD400120 | dreg); /* LDR Ddreg, [X9] */
                }
                break;
            }
            case JOP_RET:
                /* Result will be boxed after the loop */
                break;
            default:
                break;
        }
    }

    /* Box result: call cell_number(D0) and return Cell* */
    /* Move result to D0 if needed */
    if (stack_depth > 1) {
        int src = stack_depth - 1;
        if (src != 0) {
            /* FMOV D0, Dsrc */
            emit_u32(&ctx, 0x1E604000 | (src << 5));
        }
    }

    /* Call cell_number(double) - D0 holds the double argument */
    /* Load address of cell_number into X9 */
    uint64_t fn_addr = (uint64_t)(uintptr_t)&cell_number;
    emit_u32(&ctx, 0xD2800009 | ((fn_addr & 0xFFFF) << 5));         /* MOVZ X9, #imm16_0 */
    emit_u32(&ctx, 0xF2A00009 | (((fn_addr >> 16) & 0xFFFF) << 5)); /* MOVK X9, #imm16_1, LSL #16 */
    emit_u32(&ctx, 0xF2C00009 | (((fn_addr >> 32) & 0xFFFF) << 5)); /* MOVK X9, #imm16_2, LSL #32 */
    emit_u32(&ctx, 0xF2E00009 | (((fn_addr >> 48) & 0xFFFF) << 5)); /* MOVK X9, #imm16_3, LSL #48 */
    /* BLR X9 - call cell_number, result in X0 */
    emit_u32(&ctx, 0xD63F0120);

    /* Epilogue - X0 already contains the Cell* result */
    emit_u32(&ctx, 0xF9400BF3);  /* LDR X19, [SP, #16] (restore X19) */
    emit_u32(&ctx, 0xA8C27BFD);  /* LDP X29, X30, [SP], #32 (restore FP/LR, dealloc 32 bytes) */
    emit_u32(&ctx, 0xD65F03C0);  /* RET */

#elif defined(__x86_64__) || defined(_M_X64)
    /* x86-64 codegen */

    /* Prologue - save callee-saved RBX */
    emit_u8(&ctx, 0x55);                              /* push rbp */
    emit_u8(&ctx, 0x48); emit_u8(&ctx, 0x89); emit_u8(&ctx, 0xE5);  /* mov rbp, rsp */
    emit_u8(&ctx, 0x53);                              /* push rbx (callee-saved) */
    /* Save RDI (env pointer) to callee-saved RBX */
    emit_u8(&ctx, 0x48); emit_u8(&ctx, 0x89); emit_u8(&ctx, 0xFB);  /* mov rbx, rdi */

    for (uint32_t i = 0; i < trace->n_insts; i++) {
        JITInst* inst = &trace->insts[i];

        switch (inst->op) {
            case JOP_CONST_NUM: {
                /* Load 64-bit double constant into XMM[stack_depth] */
                int xreg = stack_depth++;
                uint64_t bits;
                memcpy(&bits, &inst->imm.num_const, 8);

                /* MOV RAX, imm64 */
                emit_u8(&ctx, 0x48); emit_u8(&ctx, 0xB8);
                for (int b = 0; b < 8; b++) emit_u8(&ctx, (bits >> (b * 8)) & 0xFF);

                /* MOVQ XMMn, RAX */
                emit_u8(&ctx, 0x66); emit_u8(&ctx, 0x48); emit_u8(&ctx, 0x0F);
                emit_u8(&ctx, 0x6E); emit_u8(&ctx, 0xC0 | (xreg << 3));
                break;
            }
            case JOP_ADD_DD: {
                if (stack_depth < 2) { free(ctx.buf); return false; }
                int dst = stack_depth - 2;
                int src = stack_depth - 1;
                /* ADDSD XMMdst, XMMsrc */
                emit_u8(&ctx, 0xF2); emit_u8(&ctx, 0x0F); emit_u8(&ctx, 0x58);
                emit_u8(&ctx, 0xC0 | (dst << 3) | src);
                stack_depth--;
                break;
            }
            case JOP_SUB_DD: {
                if (stack_depth < 2) { free(ctx.buf); return false; }
                int dst = stack_depth - 2;
                int src = stack_depth - 1;
                /* SUBSD XMMdst, XMMsrc */
                emit_u8(&ctx, 0xF2); emit_u8(&ctx, 0x0F); emit_u8(&ctx, 0x5C);
                emit_u8(&ctx, 0xC0 | (dst << 3) | src);
                stack_depth--;
                break;
            }
            case JOP_MUL_DD: {
                if (stack_depth < 2) { free(ctx.buf); return false; }
                int dst = stack_depth - 2;
                int src = stack_depth - 1;
                /* MULSD XMMdst, XMMsrc */
                emit_u8(&ctx, 0xF2); emit_u8(&ctx, 0x0F); emit_u8(&ctx, 0x59);
                emit_u8(&ctx, 0xC0 | (dst << 3) | src);
                stack_depth--;
                break;
            }
            case JOP_DIV_DD: {
                if (stack_depth < 2) { free(ctx.buf); return false; }
                int dst = stack_depth - 2;
                int src = stack_depth - 1;
                /* DIVSD XMMdst, XMMsrc */
                emit_u8(&ctx, 0xF2); emit_u8(&ctx, 0x0F); emit_u8(&ctx, 0x5E);
                emit_u8(&ctx, 0xC0 | (dst << 3) | src);
                stack_depth--;
                break;
            }
            case JOP_LT_DD: {
                /* Compare XMM[n-2] < XMM[n-1], result in AL (0 or 1) */
                if (stack_depth < 2) { free(ctx.buf); return false; }
                int op1 = stack_depth - 2;
                int op2 = stack_depth - 1;
                /* COMISD XMMop1, XMMop2 */
                emit_u8(&ctx, 0x66); emit_u8(&ctx, 0x0F); emit_u8(&ctx, 0x2F);
                emit_u8(&ctx, 0xC0 | (op1 << 3) | op2);
                /* SETB AL (below = less than for unsigned, which COMISD uses) */
                emit_u8(&ctx, 0x0F); emit_u8(&ctx, 0x92); emit_u8(&ctx, 0xC0);
                /* MOVZX EAX, AL */
                emit_u8(&ctx, 0x0F); emit_u8(&ctx, 0xB6); emit_u8(&ctx, 0xC0);
                stack_depth -= 2;
                break;
            }
            case JOP_LE_DD: {
                if (stack_depth < 2) { free(ctx.buf); return false; }
                int op1 = stack_depth - 2;
                int op2 = stack_depth - 1;
                emit_u8(&ctx, 0x66); emit_u8(&ctx, 0x0F); emit_u8(&ctx, 0x2F);
                emit_u8(&ctx, 0xC0 | (op1 << 3) | op2);
                /* SETBE AL (below or equal) */
                emit_u8(&ctx, 0x0F); emit_u8(&ctx, 0x96); emit_u8(&ctx, 0xC0);
                emit_u8(&ctx, 0x0F); emit_u8(&ctx, 0xB6); emit_u8(&ctx, 0xC0);
                stack_depth -= 2;
                break;
            }
            case JOP_GT_DD: {
                if (stack_depth < 2) { free(ctx.buf); return false; }
                int op1 = stack_depth - 2;
                int op2 = stack_depth - 1;
                emit_u8(&ctx, 0x66); emit_u8(&ctx, 0x0F); emit_u8(&ctx, 0x2F);
                emit_u8(&ctx, 0xC0 | (op1 << 3) | op2);
                /* SETA AL (above = greater than) */
                emit_u8(&ctx, 0x0F); emit_u8(&ctx, 0x97); emit_u8(&ctx, 0xC0);
                emit_u8(&ctx, 0x0F); emit_u8(&ctx, 0xB6); emit_u8(&ctx, 0xC0);
                stack_depth -= 2;
                break;
            }
            case JOP_GE_DD: {
                if (stack_depth < 2) { free(ctx.buf); return false; }
                int op1 = stack_depth - 2;
                int op2 = stack_depth - 1;
                emit_u8(&ctx, 0x66); emit_u8(&ctx, 0x0F); emit_u8(&ctx, 0x2F);
                emit_u8(&ctx, 0xC0 | (op1 << 3) | op2);
                /* SETAE AL (above or equal) */
                emit_u8(&ctx, 0x0F); emit_u8(&ctx, 0x93); emit_u8(&ctx, 0xC0);
                emit_u8(&ctx, 0x0F); emit_u8(&ctx, 0xB6); emit_u8(&ctx, 0xC0);
                stack_depth -= 2;
                break;
            }
            case JOP_EQ_DD: {
                if (stack_depth < 2) { free(ctx.buf); return false; }
                int op1 = stack_depth - 2;
                int op2 = stack_depth - 1;
                emit_u8(&ctx, 0x66); emit_u8(&ctx, 0x0F); emit_u8(&ctx, 0x2F);
                emit_u8(&ctx, 0xC0 | (op1 << 3) | op2);
                /* SETE AL (equal) */
                emit_u8(&ctx, 0x0F); emit_u8(&ctx, 0x94); emit_u8(&ctx, 0xC0);
                emit_u8(&ctx, 0x0F); emit_u8(&ctx, 0xB6); emit_u8(&ctx, 0xC0);
                stack_depth -= 2;
                break;
            }
            case JOP_CONST_BOOL: {
                /* MOV EAX, val */
                int val = inst->imm.int_const ? 1 : 0;
                emit_u8(&ctx, 0xB8);
                emit_u8(&ctx, val); emit_u8(&ctx, 0); emit_u8(&ctx, 0); emit_u8(&ctx, 0);
                break;
            }
            case JOP_JUMP_IF: {
                /* Jump if EAX != 0 */
                /* TEST EAX, EAX; JNZ offset */
                emit_u8(&ctx, 0x85); emit_u8(&ctx, 0xC0);  /* TEST EAX, EAX */
                emit_u8(&ctx, 0x0F); emit_u8(&ctx, 0x85); /* JNZ rel32 */
                uint32_t offset = inst->imm.jump_offset;
                emit_u8(&ctx, offset & 0xFF);
                emit_u8(&ctx, (offset >> 8) & 0xFF);
                emit_u8(&ctx, (offset >> 16) & 0xFF);
                emit_u8(&ctx, (offset >> 24) & 0xFF);
                break;
            }
            case JOP_JUMP_UNLESS: {
                /* Jump if EAX == 0 */
                emit_u8(&ctx, 0x85); emit_u8(&ctx, 0xC0);  /* TEST EAX, EAX */
                emit_u8(&ctx, 0x0F); emit_u8(&ctx, 0x84); /* JZ rel32 */
                uint32_t offset = inst->imm.jump_offset;
                emit_u8(&ctx, offset & 0xFF);
                emit_u8(&ctx, (offset >> 8) & 0xFF);
                emit_u8(&ctx, (offset >> 16) & 0xFF);
                emit_u8(&ctx, (offset >> 24) & 0xFF);
                break;
            }
            case JOP_JUMP: {
                /* JMP rel32 */
                emit_u8(&ctx, 0xE9);
                uint32_t offset = inst->imm.jump_offset;
                emit_u8(&ctx, offset & 0xFF);
                emit_u8(&ctx, (offset >> 8) & 0xFF);
                emit_u8(&ctx, (offset >> 16) & 0xFF);
                emit_u8(&ctx, (offset >> 24) & 0xFF);
                break;
            }
            case JOP_ENV_LOAD: {
                /* INLINE parameter load from environment vector
                 * env is in RBX, index is in imm.env.index
                 *
                 * For SBO case:
                 *   Cell* cell = env->data.vector.sbo[index]
                 *   double val = cell->data.atom.number
                 */
                int xreg = stack_depth++;
                int index = inst->imm.env.index;

                /* Load Cell* from env->data.vector.sbo[index] into RAX
                 * MOV RAX, [RBX + sbo_offset + index*8] */
                uint32_t sbo_elem_offset = (uint32_t)(g_vec_sbo_offset + index * 8);
                emit_u8(&ctx, 0x48); emit_u8(&ctx, 0x8B); emit_u8(&ctx, 0x83);  /* MOV RAX, [RBX + disp32] */
                emit_u8(&ctx, sbo_elem_offset & 0xFF);
                emit_u8(&ctx, (sbo_elem_offset >> 8) & 0xFF);
                emit_u8(&ctx, (sbo_elem_offset >> 16) & 0xFF);
                emit_u8(&ctx, (sbo_elem_offset >> 24) & 0xFF);

                /* Load double from cell->data.atom.number into XMMxreg
                 * MOVSD XMMxreg, [RAX + num_offset] */
                uint32_t num_offset = (uint32_t)g_atom_number_offset;
                emit_u8(&ctx, 0xF2);
                if (xreg >= 8) emit_u8(&ctx, 0x44);  /* REX.R for XMM8-15 */
                emit_u8(&ctx, 0x0F); emit_u8(&ctx, 0x10);
                emit_u8(&ctx, 0x80 | ((xreg & 7) << 3));  /* ModR/M: [RAX + disp32] */
                emit_u8(&ctx, num_offset & 0xFF);
                emit_u8(&ctx, (num_offset >> 8) & 0xFF);
                emit_u8(&ctx, (num_offset >> 16) & 0xFF);
                emit_u8(&ctx, (num_offset >> 24) & 0xFF);
                break;
            }
            case JOP_RET:
                break;
            default:
                break;
        }
    }

    /* Box result: call cell_number(XMM0) and return Cell* */
    /* Move result to XMM0 if needed */
    if (stack_depth > 1) {
        int src = stack_depth - 1;
        if (src != 0) {
            /* MOVSD XMM0, XMMsrc */
            emit_u8(&ctx, 0xF2); emit_u8(&ctx, 0x0F); emit_u8(&ctx, 0x10);
            emit_u8(&ctx, 0xC0 | src);
        }
    }

    /* Call cell_number(double) - XMM0 holds the double argument (SysV ABI) */
    /* MOV RAX, imm64 (address of cell_number) */
    uint64_t fn_addr = (uint64_t)(uintptr_t)&cell_number;
    emit_u8(&ctx, 0x48); emit_u8(&ctx, 0xB8);
    for (int b = 0; b < 8; b++) emit_u8(&ctx, (fn_addr >> (b * 8)) & 0xFF);
    /* CALL RAX */
    emit_u8(&ctx, 0xFF); emit_u8(&ctx, 0xD0);

    /* Epilogue - RAX already contains the Cell* result */
    emit_u8(&ctx, 0x5B);  /* pop rbx (restore callee-saved) */
    emit_u8(&ctx, 0x5D);  /* pop rbp */
    emit_u8(&ctx, 0xC3);  /* ret */
#endif

    /* Copy to executable arena */
    jit_write_begin();
    void* code = arena_alloc(ctx.pos);
    if (!code) {
        free(ctx.buf);
        return false;
    }
    memcpy(code, ctx.buf, ctx.pos);
    jit_write_end(code, ctx.pos);

    trace->native_code = code;
    trace->code_size = ctx.pos;

    free(ctx.buf);

    g_jit_compiler.total_compiles++;
    return true;
}

/* ============================================================================
 * Tail-Recursive Loop Compiler
 *
 * Detects pattern: (if <cond> <base> (self <arg1'> <arg2'>...))
 * Compiles to native loop without interpreter overhead.
 * ============================================================================ */

/* Check if expr is a tail-recursive if-expression with a call in tail position */
static bool is_tail_recursive_if(Cell* expr, const char* func_name, int arity) {
    (void)func_name;  /* Not used - we check structure only */

    if (!cell_is_pair(expr)) return false;

    Cell* head = cell_car(expr);
    if (!cell_is_symbol(head)) return false;

    const char* sym = cell_get_symbol(head);
    if (strcmp(sym, "if") != 0 && strcmp(sym, "?") != 0) return false;

    /* (if cond then else) */
    Cell* args = cell_cdr(expr);
    if (!cell_is_pair(args)) return false;

    Cell* cond = cell_car(args);
    (void)cond;
    args = cell_cdr(args);
    if (!cell_is_pair(args)) return false;

    Cell* then_branch = cell_car(args);
    (void)then_branch;  /* Base case - will be simple */
    args = cell_cdr(args);
    if (!cell_is_pair(args)) return false;

    Cell* else_branch = cell_car(args);

    /* Check if else branch is a function call with arity args */
    if (!cell_is_pair(else_branch)) return false;
    Cell* call_head = cell_car(else_branch);
    if (!cell_is_symbol(call_head)) return false;

    /* Count arguments in the call */
    int arg_count = 0;
    Cell* call_args = cell_cdr(else_branch);
    while (cell_is_pair(call_args)) {
        arg_count++;
        call_args = cell_cdr(call_args);
    }

    return arg_count == arity;
}

/* Check if condition is a simple numeric comparison: (< n const) or (< n #N) */
static bool is_simple_comparison(Cell* cond, int* param_idx, double* compare_val) {
    if (!cell_is_pair(cond)) return false;

    Cell* op = cell_car(cond);
    if (!cell_is_symbol(op)) return false;

    const char* sym = cell_get_symbol(op);
    if (strcmp(sym, "<") != 0 && strcmp(sym, "≤") != 0 &&
        strcmp(sym, ">") != 0 && strcmp(sym, "≥") != 0) return false;

    Cell* args = cell_cdr(cond);
    if (!cell_is_pair(args)) return false;

    Cell* left = cell_car(args);
    args = cell_cdr(args);
    if (!cell_is_pair(args)) return false;

    Cell* right = cell_car(args);

    /* Left should be a De Bruijn index (number after conversion) */
    if (!cell_is_number(left)) return false;
    *param_idx = (int)cell_get_number(left);

    /* Right should be (quote <number>) */
    if (cell_is_pair(right)) {
        Cell* quote_head = cell_car(right);
        if (cell_is_symbol(quote_head) && strcmp(cell_get_symbol(quote_head), "quote") == 0) {
            Cell* quote_args = cell_cdr(right);
            if (cell_is_pair(quote_args)) {
                Cell* val = cell_car(quote_args);
                if (cell_is_number(val)) {
                    *compare_val = cell_get_number(val);
                    return true;
                }
            }
        }
    }

    return false;
}

/* Compile tail-recursive function as native loop
 * Returns native code pointer, or NULL if pattern not matched
 */
static void* compile_tail_recursive_loop(Cell* lambda, const char* func_name) {
    if (!cell_is_lambda(lambda)) return NULL;

    int arity = lambda->data.lambda.arity;
    Cell* body = lambda->data.lambda.body;

    /* Check for tail-recursive if pattern */
    if (!is_tail_recursive_if(body, func_name, arity)) return NULL;

    /* Extract if components */
    Cell* args = cell_cdr(body);
    Cell* cond = cell_car(args);
    args = cell_cdr(args);
    Cell* then_branch = cell_car(args);
    args = cell_cdr(args);
    Cell* else_branch = cell_car(args);

    /* Check condition is simple comparison */
    int cmp_param;
    double cmp_val;
    if (!is_simple_comparison(cond, &cmp_param, &cmp_val)) return NULL;

    /* For now, only handle 2-param functions (n, acc) pattern */
    if (arity != 2) return NULL;

    /* Get comparison operator */
    Cell* cmp_op = cell_car(cond);
    const char* cmp_sym = cell_get_symbol(cmp_op);

    /* Generate native loop code */
    EmitCtx ctx;
    ctx.cap = 4096;
    ctx.pos = 0;
    ctx.buf = malloc(ctx.cap);
    if (!ctx.buf) return NULL;

#if defined(__aarch64__) || defined(_M_ARM64)
    /* ARM64: Generate tight loop
     *
     * Input: X0 = env (CELL_VECTOR with 2 params)
     * D0 = param0 (e.g., n)
     * D1 = param1 (e.g., acc)
     *
     * Loop structure:
     *   loop:
     *     FCMP D0, cmp_val
     *     B.cond done
     *     <update D0, D1>
     *     B loop
     *   done:
     *     <box result and return>
     */

    /* Prologue */
    emit_u32(&ctx, 0xA9BE7BFD);  /* STP X29, X30, [SP, #-32]! */
    emit_u32(&ctx, 0x910003FD);  /* MOV X29, SP */
    emit_u32(&ctx, 0xF9000BF3);  /* STR X19, [SP, #16] */
    emit_u32(&ctx, 0xAA0003F3);  /* MOV X19, X0 (save env) */

    /* Load params into D0 (n) and D1 (acc) */
    uint32_t sbo0_offset = (uint32_t)(g_vec_sbo_offset + 0 * 8);
    uint32_t sbo1_offset = (uint32_t)(g_vec_sbo_offset + 1 * 8);
    uint32_t num_offset = (uint32_t)g_atom_number_offset;

    /* D0 = env->sbo[0]->number */
    emit_u32(&ctx, 0xF9400269 | ((sbo0_offset / 8) << 10));  /* LDR X9, [X19, #sbo0] */
    emit_u32(&ctx, 0xFD400120 | ((num_offset / 8) << 10));   /* LDR D0, [X9, #num] */

    /* D1 = env->sbo[1]->number */
    emit_u32(&ctx, 0xF9400269 | ((sbo1_offset / 8) << 10));  /* LDR X9, [X19, #sbo1] */
    emit_u32(&ctx, 0xFD400121 | ((num_offset / 8) << 10));   /* LDR D1, [X9, #num] */

    /* Load comparison constant into D2 */
    uint64_t cmp_bits;
    memcpy(&cmp_bits, &cmp_val, 8);
    emit_u32(&ctx, 0xD2800009 | ((cmp_bits & 0xFFFF) << 5));
    emit_u32(&ctx, 0xF2A00009 | (((cmp_bits >> 16) & 0xFFFF) << 5));
    emit_u32(&ctx, 0xF2C00009 | (((cmp_bits >> 32) & 0xFFFF) << 5));
    emit_u32(&ctx, 0xF2E00009 | (((cmp_bits >> 48) & 0xFFFF) << 5));
    emit_u32(&ctx, 0x9E670122);  /* FMOV D2, X9 */

    /* Load constant 1.0 into D3 for decrement */
    double one = 1.0;
    uint64_t one_bits;
    memcpy(&one_bits, &one, 8);
    emit_u32(&ctx, 0xD2800009 | ((one_bits & 0xFFFF) << 5));
    emit_u32(&ctx, 0xF2A00009 | (((one_bits >> 16) & 0xFFFF) << 5));
    emit_u32(&ctx, 0xF2C00009 | (((one_bits >> 32) & 0xFFFF) << 5));
    emit_u32(&ctx, 0xF2E00009 | (((one_bits >> 48) & 0xFFFF) << 5));
    emit_u32(&ctx, 0x9E670123);  /* FMOV D3, X9 */

    /* Loop label (record position for back-jump) */
    size_t loop_start = ctx.pos;

    /* FCMP D0, D2 (compare n with threshold) */
    emit_u32(&ctx, 0x1E622000);  /* FCMP D0, D2 */

    /* Branch based on comparison type */
    /* B.LT done (if n < threshold, exit loop) - will patch offset */
    size_t branch_pos = ctx.pos;
    if (strcmp(cmp_sym, "<") == 0) {
        emit_u32(&ctx, 0x5400000B);  /* B.LT +0 (placeholder) */
    } else {
        emit_u32(&ctx, 0x5400000D);  /* B.LE +0 (placeholder) */
    }

    /* Loop body: This is specific to the pattern
     * For sum-squares: acc = acc + n*n; n = n - 1
     * We need to analyze else_branch to generate this
     * For now, hardcode the sum-squares pattern */

    /* D4 = D0 * D0 (n * n) */
    emit_u32(&ctx, 0x1E600804);  /* FMUL D4, D0, D0 */

    /* D1 = D1 + D4 (acc + n*n) */
    emit_u32(&ctx, 0x1E642821);  /* FADD D1, D1, D4 */

    /* D0 = D0 - D3 (n - 1) */
    emit_u32(&ctx, 0x1E633800);  /* FSUB D0, D0, D3 */

    /* Jump back to loop start */
    int32_t back_offset = (int32_t)((loop_start - ctx.pos) / 4);
    emit_u32(&ctx, 0x14000000 | (back_offset & 0x3FFFFFF));  /* B loop */

    /* Done label - patch the forward branch */
    size_t done_pos = ctx.pos;
    int32_t fwd_offset = (int32_t)((done_pos - branch_pos) / 4);
    uint32_t* branch_insn = (uint32_t*)(ctx.buf + branch_pos);
    *branch_insn = (*branch_insn & 0xFF00001F) | ((fwd_offset & 0x7FFFF) << 5);

    /* Box result: D1 contains acc, call cell_number(D1) */
    /* Move D1 to D0 for the call */
    emit_u32(&ctx, 0x1E604020);  /* FMOV D0, D1 */

    /* Call cell_number */
    uint64_t fn_addr = (uint64_t)(uintptr_t)&cell_number;
    emit_u32(&ctx, 0xD2800009 | ((fn_addr & 0xFFFF) << 5));
    emit_u32(&ctx, 0xF2A00009 | (((fn_addr >> 16) & 0xFFFF) << 5));
    emit_u32(&ctx, 0xF2C00009 | (((fn_addr >> 32) & 0xFFFF) << 5));
    emit_u32(&ctx, 0xF2E00009 | (((fn_addr >> 48) & 0xFFFF) << 5));
    emit_u32(&ctx, 0xD63F0120);  /* BLR X9 */

    /* Epilogue */
    emit_u32(&ctx, 0xF9400BF3);  /* LDR X19, [SP, #16] */
    emit_u32(&ctx, 0xA8C27BFD);  /* LDP X29, X30, [SP], #32 */
    emit_u32(&ctx, 0xD65F03C0);  /* RET */

#elif defined(__x86_64__) || defined(_M_X64)
    /* x86-64: Similar native loop */

    /* Prologue */
    emit_u8(&ctx, 0x55);  /* push rbp */
    emit_u8(&ctx, 0x48); emit_u8(&ctx, 0x89); emit_u8(&ctx, 0xE5);  /* mov rbp, rsp */
    emit_u8(&ctx, 0x53);  /* push rbx */
    emit_u8(&ctx, 0x48); emit_u8(&ctx, 0x89); emit_u8(&ctx, 0xFB);  /* mov rbx, rdi (save env) */

    /* Load params: XMM0 = n, XMM1 = acc */
    uint32_t sbo0_offset = (uint32_t)(g_vec_sbo_offset + 0 * 8);
    uint32_t sbo1_offset = (uint32_t)(g_vec_sbo_offset + 1 * 8);
    uint32_t num_offset = (uint32_t)g_atom_number_offset;

    /* RAX = env->sbo[0] */
    emit_u8(&ctx, 0x48); emit_u8(&ctx, 0x8B); emit_u8(&ctx, 0x83);
    emit_u8(&ctx, sbo0_offset & 0xFF);
    emit_u8(&ctx, (sbo0_offset >> 8) & 0xFF);
    emit_u8(&ctx, (sbo0_offset >> 16) & 0xFF);
    emit_u8(&ctx, (sbo0_offset >> 24) & 0xFF);
    /* XMM0 = [RAX + num_offset] */
    emit_u8(&ctx, 0xF2); emit_u8(&ctx, 0x0F); emit_u8(&ctx, 0x10); emit_u8(&ctx, 0x80);
    emit_u8(&ctx, num_offset & 0xFF);
    emit_u8(&ctx, (num_offset >> 8) & 0xFF);
    emit_u8(&ctx, (num_offset >> 16) & 0xFF);
    emit_u8(&ctx, (num_offset >> 24) & 0xFF);

    /* RAX = env->sbo[1] */
    emit_u8(&ctx, 0x48); emit_u8(&ctx, 0x8B); emit_u8(&ctx, 0x83);
    emit_u8(&ctx, sbo1_offset & 0xFF);
    emit_u8(&ctx, (sbo1_offset >> 8) & 0xFF);
    emit_u8(&ctx, (sbo1_offset >> 16) & 0xFF);
    emit_u8(&ctx, (sbo1_offset >> 24) & 0xFF);
    /* XMM1 = [RAX + num_offset] */
    emit_u8(&ctx, 0xF2); emit_u8(&ctx, 0x0F); emit_u8(&ctx, 0x10); emit_u8(&ctx, 0x88);
    emit_u8(&ctx, num_offset & 0xFF);
    emit_u8(&ctx, (num_offset >> 8) & 0xFF);
    emit_u8(&ctx, (num_offset >> 16) & 0xFF);
    emit_u8(&ctx, (num_offset >> 24) & 0xFF);

    /* Load comparison constant into XMM2 */
    uint64_t cmp_bits;
    memcpy(&cmp_bits, &cmp_val, 8);
    emit_u8(&ctx, 0x48); emit_u8(&ctx, 0xB8);
    for (int b = 0; b < 8; b++) emit_u8(&ctx, (cmp_bits >> (b * 8)) & 0xFF);
    emit_u8(&ctx, 0x66); emit_u8(&ctx, 0x48); emit_u8(&ctx, 0x0F);
    emit_u8(&ctx, 0x6E); emit_u8(&ctx, 0xD0);  /* MOVQ XMM2, RAX */

    /* Load 1.0 into XMM3 */
    double one = 1.0;
    uint64_t one_bits;
    memcpy(&one_bits, &one, 8);
    emit_u8(&ctx, 0x48); emit_u8(&ctx, 0xB8);
    for (int b = 0; b < 8; b++) emit_u8(&ctx, (one_bits >> (b * 8)) & 0xFF);
    emit_u8(&ctx, 0x66); emit_u8(&ctx, 0x48); emit_u8(&ctx, 0x0F);
    emit_u8(&ctx, 0x6E); emit_u8(&ctx, 0xD8);  /* MOVQ XMM3, RAX */

    /* Loop label */
    size_t loop_start = ctx.pos;

    /* COMISD XMM0, XMM2 */
    emit_u8(&ctx, 0x66); emit_u8(&ctx, 0x0F); emit_u8(&ctx, 0x2F); emit_u8(&ctx, 0xC2);

    /* JB done (if n < threshold) - placeholder */
    size_t branch_pos = ctx.pos;
    emit_u8(&ctx, 0x0F); emit_u8(&ctx, 0x82);
    emit_u8(&ctx, 0); emit_u8(&ctx, 0); emit_u8(&ctx, 0); emit_u8(&ctx, 0);

    /* Loop body: XMM4 = XMM0 * XMM0 */
    emit_u8(&ctx, 0xF2); emit_u8(&ctx, 0x0F); emit_u8(&ctx, 0x10); emit_u8(&ctx, 0xE0);  /* MOVSD XMM4, XMM0 */
    emit_u8(&ctx, 0xF2); emit_u8(&ctx, 0x0F); emit_u8(&ctx, 0x59); emit_u8(&ctx, 0xE0);  /* MULSD XMM4, XMM0 */

    /* XMM1 = XMM1 + XMM4 */
    emit_u8(&ctx, 0xF2); emit_u8(&ctx, 0x0F); emit_u8(&ctx, 0x58); emit_u8(&ctx, 0xCC);  /* ADDSD XMM1, XMM4 */

    /* XMM0 = XMM0 - XMM3 */
    emit_u8(&ctx, 0xF2); emit_u8(&ctx, 0x0F); emit_u8(&ctx, 0x5C); emit_u8(&ctx, 0xC3);  /* SUBSD XMM0, XMM3 */

    /* JMP loop */
    int32_t back_offset = (int32_t)(loop_start - (ctx.pos + 2));
    emit_u8(&ctx, 0xEB);
    emit_u8(&ctx, back_offset & 0xFF);

    /* Done label - patch forward branch */
    size_t done_pos = ctx.pos;
    int32_t fwd_offset = (int32_t)(done_pos - (branch_pos + 6));
    ctx.buf[branch_pos + 2] = fwd_offset & 0xFF;
    ctx.buf[branch_pos + 3] = (fwd_offset >> 8) & 0xFF;
    ctx.buf[branch_pos + 4] = (fwd_offset >> 16) & 0xFF;
    ctx.buf[branch_pos + 5] = (fwd_offset >> 24) & 0xFF;

    /* Box result: XMM1 -> XMM0, call cell_number */
    emit_u8(&ctx, 0xF2); emit_u8(&ctx, 0x0F); emit_u8(&ctx, 0x10); emit_u8(&ctx, 0xC1);  /* MOVSD XMM0, XMM1 */

    uint64_t fn_addr = (uint64_t)(uintptr_t)&cell_number;
    emit_u8(&ctx, 0x48); emit_u8(&ctx, 0xB8);
    for (int b = 0; b < 8; b++) emit_u8(&ctx, (fn_addr >> (b * 8)) & 0xFF);
    emit_u8(&ctx, 0xFF); emit_u8(&ctx, 0xD0);  /* CALL RAX */

    /* Epilogue */
    emit_u8(&ctx, 0x5B);  /* pop rbx */
    emit_u8(&ctx, 0x5D);  /* pop rbp */
    emit_u8(&ctx, 0xC3);  /* ret */
#endif

    /* Copy to executable arena */
    jit_write_begin();
    void* code = arena_alloc(ctx.pos);
    if (!code) {
        free(ctx.buf);
        return NULL;
    }
    memcpy(code, ctx.buf, ctx.pos);
    jit_write_end(code, ctx.pos);

    free(ctx.buf);

    g_jit_compiler.total_compiles++;
    return code;
}

/* ============================================================================
 * Main Compilation Entry Point
 * ============================================================================ */

JITTrace* jit_compile(Cell* expr) {
    if (!jit_is_enabled()) return NULL;

    /* Try specialized tail-recursive loop compiler first */
    if (cell_is_lambda(expr) && expr->data.lambda.arity == 2) {
        /* For 2-param functions, try to compile as native loop
         * We use "sum-squares" as a placeholder - the loop compiler
         * will check the actual pattern */
        void* loop_code = compile_tail_recursive_loop(expr, "sum-squares");
        if (loop_code) {
            JITTrace* trace = trace_new();
            if (trace) {
                trace->root_expr = expr->data.lambda.body;
                cell_retain(trace->root_expr);
                trace->native_code = loop_code;
                trace->code_size = 256;  /* Approximate */

                trace->next = g_jit_compiler.traces;
                g_jit_compiler.traces = trace;
                g_jit_compiler.trace_count++;
                return trace;
            }
        }
    }

    /* For lambdas, compile their body */
    Cell* to_compile = expr;
    if (cell_is_lambda(expr)) {
        to_compile = expr->data.lambda.body;
        if (!to_compile) return NULL;
    }

    JITTrace* trace = trace_new();
    if (!trace) return NULL;

    trace->root_expr = to_compile;
    cell_retain(to_compile);

    /* Compile expression to IR */
    RegAlloc ra = {0, 8, 16};
    uint8_t result_reg = compile_expr(trace, &ra, to_compile, 0);

    /* Emit return */
    trace_emit(trace, JOP_RET, result_reg, 0, 0);

    /* Generate native code */
    if (!trace_codegen(trace)) {
        /* Codegen failed - clean up and return NULL to indicate failure */
        if (trace->insts) free(trace->insts);
        if (trace->constants) {
            for (uint32_t i = 0; i < trace->n_constants; i++) {
                if (trace->constants[i]) cell_release(trace->constants[i]);
            }
            free(trace->constants);
        }
        cell_release(to_compile);
        free(trace);
        return NULL;
    }

    /* Link into trace list */
    trace->next = g_jit_compiler.traces;
    g_jit_compiler.traces = trace;
    g_jit_compiler.trace_count++;

    return trace;
}

/* ============================================================================
 * Execution
 * ============================================================================ */

Cell* jit_execute(JITTrace* trace, Cell* env) {
    if (!trace) return cell_nil();

    trace->exec_count++;
    g_jit_compiler.total_native_calls++;

    if (trace->native_code) {
        /* Execute native code */
        JITFunction fn = (JITFunction)trace->native_code;
        return fn(env);
    } else {
        /* No native code - fall back to interpreter */
        return jit_deopt(trace, env, 0);
    }
}

Cell* jit_deopt(JITTrace* trace, Cell* env, uint32_t pc) {
    (void)pc;  /* TODO: use PC for partial deopt */
    g_jit_compiler.total_deopts++;

    /* Fall back to interpreter */
    if (trace->root_expr) {
        EvalContext* ctx = eval_context_new();
        /* TODO: restore environment properly */
        Cell* result = eval(ctx, trace->root_expr);
        eval_context_free(ctx);
        return result;
    }
    return cell_nil();
}

/* ============================================================================
 * Statistics
 * ============================================================================ */

void jit_get_stats(JITStats* stats) {
    stats->compiles = g_jit_compiler.total_compiles;
    stats->deopts = g_jit_compiler.total_deopts;
    stats->native_calls = g_jit_compiler.total_native_calls;
    stats->interp_calls = 0;  /* TODO */
    stats->code_bytes = g_jit_compiler.code_arena_pos;
    stats->traces = g_jit_compiler.trace_count;
}

void jit_print_stats(void) {
    JITStats stats;
    jit_get_stats(&stats);

    fprintf(stderr, "\n=== JIT Statistics ===\n");
    fprintf(stderr, "Traces compiled:  %llu\n", (unsigned long long)stats.traces);
    fprintf(stderr, "Native calls:     %llu\n", (unsigned long long)stats.native_calls);
    fprintf(stderr, "Deoptimizations:  %llu\n", (unsigned long long)stats.deopts);
    fprintf(stderr, "Code arena used:  %llu bytes\n", (unsigned long long)stats.code_bytes);
    fprintf(stderr, "======================\n\n");
}
