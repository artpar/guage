#ifndef GUAGE_SPAN_H
#define GUAGE_SPAN_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/* =========================================================================
 * 8-Byte Span System (Rust Model)
 *
 * Every Cell carries a Span that records exactly where in source it came from.
 * Spans are compact (8 bytes) with an inline form covering >99% of cases
 * and an interned fallback for rare large/complex spans.
 *
 * Line/column are NEVER stored — computed on demand via binary search over
 * the line table (O(log L) where L = number of lines). This is what Rust,
 * Clang, Go, and Zig all do.
 * ========================================================================= */

/* === Core Types === */

typedef uint32_t BytePos;         /* Global byte offset into virtual source space */
typedef uint32_t SpanIndex;       /* Index into span interner */

/* Full logical span (16 bytes) — stored in interner for rare cases */
typedef struct {
    BytePos lo;                   /* Start byte offset */
    BytePos hi;                   /* End byte offset (exclusive) */
    uint16_t ctxt;                /* Macro expansion context (0 = root) */
    uint16_t file_id;             /* Source file index (for O(1) file lookup) */
} SpanData;

/* Compact span (8 bytes) — stored on every Cell.
 * Rust-style inline-or-intern encoding. */
typedef union {
    struct {                      /* Inline form (>99% of spans) */
        uint32_t lo;              /* Start byte offset */
        uint16_t len;             /* Length (0..65534). 0xFFFF = sentinel for interned */
        uint16_t ctxt;            /* Macro context (0..65534). 0xFFFF = interned marker */
    } inline_span;
    struct {                      /* Interned form (<1% of spans) */
        uint32_t index;           /* Index into span interner table */
        uint16_t marker1;         /* 0xFFFF */
        uint16_t marker2;         /* 0xFFFF */
    } interned;
    uint64_t raw;                 /* For zero-init and comparison */
} Span;

#define SPAN_NONE ((Span){.raw = 0})
#define SPAN_INTERNED_MARKER 0xFFFF

/* === Source File === */

typedef struct {
    const char* filename;         /* Owned string (strdup'd) */
    uint32_t base;                /* Base offset in global byte space */
    uint32_t size;                /* File size in bytes */
    const char* source;           /* Full source text (owned, for snippets) */
    uint32_t* line_starts;        /* Sorted array: byte offset of each line start */
    uint32_t line_count;          /* Number of lines */
    uint16_t file_id;             /* Index in SourceMap.files[] */
} SourceFile;

/* === Source Map === */

typedef struct {
    SourceFile* files;
    uint32_t file_count;
    uint32_t file_capacity;
    uint32_t next_base;           /* Next available global offset (file bases grow upward) */

    /* Span interner (for rare large/complex spans) */
    SpanData* interned_spans;
    uint32_t interned_count;
    uint32_t interned_capacity;
} SourceMap;

/* === Macro Expansion Info === */

typedef struct {
    uint16_t parent_ctxt;         /* Enclosing context (0 = root) */
    Span call_site;               /* Where the macro was invoked */
    Span def_site;                /* Where the macro was defined */
    const char* macro_name;       /* Name of the macro (interned) */
} ExpansionInfo;

/* === Resolved Position (computed on demand) === */

typedef struct {
    const char* filename;
    uint32_t line;                /* 1-based */
    uint32_t column;              /* 1-based (byte offset within line) */
    uint32_t byte_offset;         /* Global byte position */
    uint16_t file_id;
} ResolvedPos;

typedef struct {
    ResolvedPos start;
    ResolvedPos end;
    const char* line_text;        /* Pointer into source (not owned) */
    uint32_t line_text_len;       /* Length of the source line */
} ResolvedSpan;

/* === API === */

/* SourceMap lifecycle */
SourceMap* srcmap_new(void);
void srcmap_free(SourceMap* map);

/* Register a source file. Returns the file_base for the parser. */
uint32_t srcmap_add_file(SourceMap* map, const char* filename, const char* source, uint32_t source_len);

/* Resolve a byte position to file:line:col */
ResolvedPos srcmap_resolve(SourceMap* map, BytePos pos);

/* Resolve a full span to start/end positions + source line text */
ResolvedSpan srcmap_resolve_span(SourceMap* map, Span span);

/* Span construction */
static inline Span span_new(BytePos lo, BytePos hi) {
    uint32_t len = hi - lo;
    if (len <= 0xFFFE) {
        Span s;
        s.inline_span.lo = lo;
        s.inline_span.len = (uint16_t)len;
        s.inline_span.ctxt = 0;
        return s;
    }
    /* Fallback: would need interning for very large spans.
     * For now, clamp to max inline length. */
    Span s;
    s.inline_span.lo = lo;
    s.inline_span.len = 0xFFFE;
    s.inline_span.ctxt = 0;
    return s;
}

static inline Span span_with_ctxt(BytePos lo, BytePos hi, uint16_t ctxt) {
    uint32_t len = hi - lo;
    if (len <= 0xFFFE && ctxt < SPAN_INTERNED_MARKER) {
        Span s;
        s.inline_span.lo = lo;
        s.inline_span.len = (uint16_t)len;
        s.inline_span.ctxt = ctxt;
        return s;
    }
    /* Would need interning */
    Span s;
    s.inline_span.lo = lo;
    s.inline_span.len = (len <= 0xFFFE) ? (uint16_t)len : 0xFFFE;
    s.inline_span.ctxt = (ctxt < SPAN_INTERNED_MARKER) ? ctxt : 0;
    return s;
}

/* Span predicates */
static inline bool span_is_none(Span s) {
    return s.raw == 0;
}

static inline bool span_is_interned(Span s) {
    return s.inline_span.len == SPAN_INTERNED_MARKER &&
           s.inline_span.ctxt == SPAN_INTERNED_MARKER;
}

/* Span accessors (decode from inline or interned form) */
static inline BytePos span_lo(Span s) {
    if (span_is_interned(s)) {
        /* Would look up in interner — for now, return 0 */
        return 0;
    }
    return s.inline_span.lo;
}

static inline BytePos span_hi(Span s) {
    if (span_is_interned(s)) {
        return 0;
    }
    return s.inline_span.lo + s.inline_span.len;
}

static inline uint16_t span_ctxt(Span s) {
    if (span_is_interned(s)) {
        return 0;
    }
    return s.inline_span.ctxt;
}

/* Intern a span into the SourceMap (for rare large/complex spans) */
Span srcmap_intern_span(SourceMap* map, SpanData data);

/* Look up interned span */
SpanData srcmap_lookup_span(SourceMap* map, Span s);

/* Get the SourceFile for a byte position (binary search over file bases) */
SourceFile* srcmap_file_for_pos(SourceMap* map, BytePos pos);

/* Get a specific source line text (0-based line index within file) */
const char* srcmap_line_text(SourceFile* file, uint32_t line_idx, uint32_t* out_len);

/* Global SourceMap singleton (set during init) */
extern SourceMap* g_source_map;

#endif /* GUAGE_SPAN_H */
