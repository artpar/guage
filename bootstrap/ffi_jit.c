#include "ffi_jit.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#ifdef __APPLE__
#include <libkern/OSCacheControl.h>
#include <pthread.h>
#endif

/* Global JIT state */
static JITState g_jit = {0};

void jit_state_init(void) {
    g_jit.stubs = NULL;
    g_jit.stub_count = 0;
}

void jit_state_destroy(void) {
    JITStub* s = g_jit.stubs;
    while (s) {
        JITStub* next = s->next;
        if (s->code) {
            munmap(s->code, s->code_size);
        }
        free(s);
        s = next;
    }
    g_jit.stubs = NULL;
    g_jit.stub_count = 0;
}

/* === Emit helpers === */

void emit_u8(EmitCtx* ctx, uint8_t b) {
    if (ctx->pos < ctx->cap) {
        ctx->buf[ctx->pos++] = b;
    }
}

void emit_u16(EmitCtx* ctx, uint16_t v) {
    emit_u8(ctx, (uint8_t)(v & 0xFF));
    emit_u8(ctx, (uint8_t)((v >> 8) & 0xFF));
}

void emit_u32(EmitCtx* ctx, uint32_t v) {
    emit_u8(ctx, (uint8_t)(v & 0xFF));
    emit_u8(ctx, (uint8_t)((v >> 8) & 0xFF));
    emit_u8(ctx, (uint8_t)((v >> 16) & 0xFF));
    emit_u8(ctx, (uint8_t)((v >> 24) & 0xFF));
}

void emit_u64(EmitCtx* ctx, uint64_t v) {
    emit_u32(ctx, (uint32_t)(v & 0xFFFFFFFF));
    emit_u32(ctx, (uint32_t)((v >> 32) & 0xFFFFFFFF));
}

void emit_bytes(EmitCtx* ctx, const uint8_t* data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        emit_u8(ctx, data[i]);
    }
}

/* === Error helpers (called from JIT stubs via absolute address) === */

Cell* ffi_type_error(int arg_index, int expected_type, int got_type) {
    char buf[128];
    snprintf(buf, sizeof(buf), "ffi-type-error: arg %d expected type %d got %d",
             arg_index, expected_type, got_type);
    return cell_error(buf, cell_nil());
}

Cell* ffi_arity_error(int expected, int got) {
    char buf[128];
    snprintf(buf, sizeof(buf), "ffi-arity-error: expected %d args got %d", expected, got);
    return cell_error(buf, cell_nil());
}

/* === Type symbol parsing === */

FFICType ffi_parse_type_symbol(const char* sym) {
    if (!sym) return FFI_VOID;
    /* Skip leading colon from Guage symbols (e.g. ":double" → "double") */
    if (sym[0] == ':') sym++;
    if (strcmp(sym, "void") == 0)   return FFI_VOID;
    if (strcmp(sym, "double") == 0) return FFI_DOUBLE;
    if (strcmp(sym, "int") == 0)    return FFI_INT32;
    if (strcmp(sym, "long") == 0)   return FFI_INT64;
    if (strcmp(sym, "uint") == 0)   return FFI_UINT32;
    if (strcmp(sym, "ulong") == 0)  return FFI_UINT64;
    if (strcmp(sym, "float") == 0)  return FFI_FLOAT;
    if (strcmp(sym, "char*") == 0)  return FFI_CSTRING;
    if (strcmp(sym, "void*") == 0)  return FFI_PTR;
    if (strcmp(sym, "bool") == 0)   return FFI_BOOL;
    if (strcmp(sym, "size_t") == 0) return FFI_SIZE_T;
    if (strcmp(sym, "buffer") == 0) return FFI_BUFFER;
    return FFI_VOID; /* fallback */
}

/* === Platform mmap helpers === */

static void* jit_alloc_page(size_t size) {
    /* Round up to page size */
    long page_size = sysconf(_SC_PAGESIZE);
    size_t alloc_size = (size + (size_t)page_size - 1) & ~((size_t)page_size - 1);

#if defined(__APPLE__) && defined(__aarch64__)
    /* macOS ARM64: MAP_JIT for W^X with JIT entitlement */
    void* mem = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE | PROT_EXEC,
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_JIT, -1, 0);
#else
    /* All others: start with RW, switch to RX after writing */
    void* mem = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif

    if (mem == MAP_FAILED) return NULL;
    return mem;
}

static bool jit_protect_exec(void* mem, size_t size) {
    long page_size = sysconf(_SC_PAGESIZE);
    size_t alloc_size = (size + (size_t)page_size - 1) & ~((size_t)page_size - 1);

#if defined(__APPLE__) && defined(__aarch64__)
    /* macOS ARM64: switch from write to exec mode */
    pthread_jit_write_protect_np(1); /* enable exec, disable write */
    sys_icache_invalidate(mem, alloc_size);
    return true;
#elif defined(__aarch64__)
    /* Linux ARM64: mprotect + cache flush */
    if (mprotect(mem, alloc_size, PROT_READ | PROT_EXEC) != 0) return false;
    __builtin___clear_cache((char*)mem, (char*)mem + alloc_size);
    return true;
#else
    /* x86-64: just mprotect to RX (no icache flush needed) */
    return mprotect(mem, alloc_size, PROT_READ | PROT_EXEC) == 0;
#endif
}

#if defined(__APPLE__) && defined(__aarch64__)
static void jit_write_begin(void) {
    pthread_jit_write_protect_np(0); /* enable write, disable exec */
}
#else
#define jit_write_begin() ((void)0)
#endif

/* === Stub generation dispatch === */

FFIStubFn jit_emit_stub(FFISig* sig) {
    /* Allocate emit buffer (generous — stubs are small) */
    EmitCtx ctx;
    ctx.cap = 4096;
    ctx.pos = 0;
    ctx.buf = (uint8_t*)malloc(ctx.cap);
    if (!ctx.buf) return NULL;

    bool ok = false;

#if defined(__x86_64__) || defined(_M_X64)
    ok = emit_x64_stub(&ctx, sig);
#elif defined(__aarch64__) || defined(_M_ARM64)
    ok = emit_a64_stub(&ctx, sig);
#else
    fprintf(stderr, "FFI JIT: unsupported architecture\n");
#endif

    if (!ok) {
        free(ctx.buf);
        return NULL;
    }

    /* Allocate executable memory */
    size_t code_size = ctx.pos;
    void* code = jit_alloc_page(code_size);
    if (!code) {
        free(ctx.buf);
        return NULL;
    }

    /* Copy code to executable page */
    jit_write_begin();
    memcpy(code, ctx.buf, code_size);
    free(ctx.buf);

    /* Make executable */
    if (!jit_protect_exec(code, code_size)) {
        long page_size = sysconf(_SC_PAGESIZE);
        size_t alloc_size = (code_size + (size_t)page_size - 1) & ~((size_t)page_size - 1);
        munmap(code, alloc_size);
        return NULL;
    }

    /* Register stub for cleanup */
    JITStub* stub = (JITStub*)malloc(sizeof(JITStub));
    stub->code = code;
    long page_size = sysconf(_SC_PAGESIZE);
    stub->code_size = (code_size + (size_t)page_size - 1) & ~((size_t)page_size - 1);
    stub->sig = *sig;
    stub->next = g_jit.stubs;
    g_jit.stubs = stub;
    g_jit.stub_count++;

    return (FFIStubFn)code;
}
