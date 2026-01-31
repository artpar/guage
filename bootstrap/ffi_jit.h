#ifndef GUAGE_FFI_JIT_H
#define GUAGE_FFI_JIT_H

#include "cell.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* FFI C type enum — maps Guage type symbols to C ABI types */
typedef enum {
    FFI_VOID,
    FFI_DOUBLE,
    FFI_INT32,
    FFI_INT64,
    FFI_UINT32,
    FFI_UINT64,
    FFI_PTR,        /* void* — maps to FFI_PTR cell */
    FFI_CSTRING,    /* const char* — zero-copy from Guage string */
    FFI_BOOL,
    FFI_SIZE_T,
    FFI_FLOAT,
    FFI_BUFFER      /* uint8_t* — zero-copy from Guage buffer */
} FFICType;

#define FFI_MAX_ARGS 8

/* Function signature for JIT stub generation */
typedef struct {
    FFICType ret_type;
    FFICType arg_types[FFI_MAX_ARGS];
    int      n_args;
    void*    fn_ptr;        /* dlsym result — the actual C function */
} FFISig;

/* A JIT-compiled stub — stored as linked list node for cleanup */
typedef struct JITStub {
    void*    code;          /* mmap'd executable memory */
    size_t   code_size;     /* mmap region size (page-aligned) */
    FFISig   sig;           /* Signature this stub was built for */
    struct JITStub* next;   /* Linked list for cleanup */
} JITStub;

/* Global JIT state — init once, destroy on exit */
typedef struct {
    JITStub* stubs;         /* Linked list of all stubs */
    int      stub_count;
} JITState;

/* Emit context — buffer for writing machine code */
typedef struct {
    uint8_t* buf;
    size_t   pos;
    size_t   cap;
} EmitCtx;

/* === JIT State lifecycle === */
void     jit_state_init(void);
void     jit_state_destroy(void);

/* === Stub generation === */
/* Returns a Cell* (*)(Cell*) function pointer, or NULL on failure.
 * The stub is registered in global JIT state for cleanup. */
typedef Cell* (*FFIStubFn)(Cell*);
FFIStubFn jit_emit_stub(FFISig* sig);

/* === Emit helpers === */
void emit_u8(EmitCtx* ctx, uint8_t b);
void emit_u16(EmitCtx* ctx, uint16_t v);
void emit_u32(EmitCtx* ctx, uint32_t v);
void emit_u64(EmitCtx* ctx, uint64_t v);
void emit_bytes(EmitCtx* ctx, const uint8_t* data, size_t len);

/* === Error helpers (called from JIT stubs) === */
Cell* ffi_type_error(int arg_index, int expected_type, int got_type);
Cell* ffi_arity_error(int expected, int got);

/* === Type symbol parsing === */
FFICType ffi_parse_type_symbol(const char* sym);

/* === Platform emitters (implemented per-arch) === */
#if defined(__x86_64__) || defined(_M_X64)
bool emit_x64_stub(EmitCtx* ctx, FFISig* sig);
#endif
#if defined(__aarch64__) || defined(_M_ARM64)
bool emit_a64_stub(EmitCtx* ctx, FFISig* sig);
#endif

#endif /* GUAGE_FFI_JIT_H */
