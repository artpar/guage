#include "ffi_jit.h"
#include "eval.h"
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

/* === FFI Struct Layout (Step 4) === */

CallbackStub g_callbacks[FFI_MAX_CALLBACKS] = {{0}};

uint32_t ffi_type_size(FFICType t) {
    switch (t) {
        case FFI_BOOL:    return 1;
        case FFI_INT32:   return 4;
        case FFI_UINT32:  return 4;
        case FFI_FLOAT:   return 4;
        case FFI_INT64:   return 8;
        case FFI_UINT64:  return 8;
        case FFI_DOUBLE:  return 8;
        case FFI_PTR:     return 8;
        case FFI_CSTRING: return 8;
        case FFI_SIZE_T:  return sizeof(size_t);
        case FFI_BUFFER:  return 8;
        default:          return 0;
    }
}

uint32_t ffi_type_align(FFICType t) {
    /* Natural alignment = size for all standard C types */
    return ffi_type_size(t);
}

FFIStructLayout* ffi_compute_layout(Cell* field_list) {
    FFIStructLayout* layout = (FFIStructLayout*)calloc(1, sizeof(FFIStructLayout));
    if (!layout) return NULL;

    uint32_t offset = 0;
    uint32_t max_align = 1;
    Cell* cur = field_list;

    while (cur && cell_is_pair(cur) && layout->n_fields < 32) {
        Cell* spec = cell_car(cur);
        if (!cell_is_pair(spec)) { cur = cell_cdr(cur); continue; }

        Cell* name_cell = cell_car(spec);
        Cell* type_cell = cell_cdr(spec);
        /* Handle both (name . type) and (name type) */
        if (cell_is_pair(type_cell)) type_cell = cell_car(type_cell);

        if (!cell_is_symbol(name_cell) || !cell_is_symbol(type_cell)) {
            cur = cell_cdr(cur);
            continue;
        }

        FFICType ct = ffi_parse_type_symbol(cell_get_symbol(type_cell));
        uint32_t sz = ffi_type_size(ct);
        uint32_t al = ffi_type_align(ct);
        if (al == 0) al = 1;
        if (al > max_align) max_align = al;

        /* Align offset */
        offset = (offset + al - 1) & ~(al - 1);

        FFIField* f = &layout->fields[layout->n_fields];
        f->name = cell_get_symbol(name_cell);
        f->type = ct;
        f->offset = offset;
        f->size = sz;

        offset += sz;
        layout->n_fields++;
        cur = cell_cdr(cur);
    }

    /* Tail padding for struct alignment */
    layout->alignment = max_align;
    layout->total_size = (offset + max_align - 1) & ~(max_align - 1);
    return layout;
}

/* === FFI Callbacks (Step 5) === */

int ffi_callback_alloc(Cell* closure, FFISig* sig) {
    for (int i = 0; i < FFI_MAX_CALLBACKS; i++) {
        if (!g_callbacks[i].active) {
            g_callbacks[i].closure = closure;
            cell_retain(closure);
            g_callbacks[i].sig = *sig;
            g_callbacks[i].active = true;
            g_callbacks[i].trampoline = NULL; /* Set by platform emitter */
            return i;
        }
    }
    return -1; /* No free slots */
}

void ffi_callback_free(int slot) {
    if (slot < 0 || slot >= FFI_MAX_CALLBACKS) return;
    CallbackStub* cb = &g_callbacks[slot];
    if (!cb->active) return;

    if (cb->closure) {
        cell_release(cb->closure);
        cb->closure = NULL;
    }
    if (cb->trampoline) {
        long page_size = sysconf(_SC_PAGESIZE);
        size_t alloc_size = (cb->trampoline_size + (size_t)page_size - 1) & ~((size_t)page_size - 1);
        munmap(cb->trampoline, alloc_size);
        cb->trampoline = NULL;
    }
    cb->active = false;
}

Cell* ffi_callback_dispatch(int slot, uint64_t* raw_args) {
    if (slot < 0 || slot >= FFI_MAX_CALLBACKS || !g_callbacks[slot].active)
        return cell_error("callback-invalid-slot", cell_number(slot));

    CallbackStub* cb = &g_callbacks[slot];

    /* Marshal C args to Guage list */
    Cell* args = cell_nil();
    for (int i = cb->sig.n_args - 1; i >= 0; i--) {
        Cell* arg;
        switch (cb->sig.arg_types[i]) {
            case FFI_INT32:
                arg = cell_integer((int64_t)(int32_t)raw_args[i]);
                break;
            case FFI_INT64:
                arg = cell_integer((int64_t)raw_args[i]);
                break;
            case FFI_UINT32:
                arg = cell_integer((int64_t)(uint32_t)raw_args[i]);
                break;
            case FFI_UINT64:
                arg = cell_integer((int64_t)raw_args[i]);
                break;
            case FFI_DOUBLE: {
                double d;
                memcpy(&d, &raw_args[i], sizeof(double));
                arg = cell_number(d);
                break;
            }
            case FFI_FLOAT: {
                float f;
                memcpy(&f, &raw_args[i], sizeof(float));
                arg = cell_number((double)f);
                break;
            }
            case FFI_PTR:
            case FFI_BUFFER:
                arg = cell_ffi_ptr((void*)raw_args[i], NULL, "callback-arg");
                break;
            case FFI_CSTRING:
                arg = raw_args[i] ? cell_string((const char*)raw_args[i]) : cell_nil();
                break;
            case FFI_BOOL:
                arg = cell_bool(raw_args[i] != 0);
                break;
            default:
                arg = cell_nil();
                break;
        }
        args = cell_cons(arg, args);
    }

    /* Reverse the args list (it was built in reverse via cons) */
    Cell* reversed = cell_nil();
    Cell* cur = args;
    while (cell_is_pair(cur)) {
        reversed = cell_cons(cell_car(cur), reversed);
        cur = cell_cdr(cur);
    }

    /* Build application expression: (closure arg1 arg2 ...) */
    Cell* app = cell_cons(cb->closure, reversed);

    /* Evaluate through the standard eval path */
    EvalContext* ctx = eval_context_new();
    Cell* result = eval(ctx, app);
    eval_context_free(ctx);
    return result;
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
