#include "span.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* =========================================================================
 * SourceMap Implementation
 *
 * Binary search over file bases for O(log F) file lookup.
 * Binary search over line_starts for O(log L) line resolution.
 * Lazy line table construction (built when file is registered).
 * ========================================================================= */

/* Global singleton */
SourceMap* g_source_map = NULL;

/* === Line Table Construction === */

/* Build line_starts array from source text.
 * line_starts[0] = 0 (first line always starts at byte 0).
 * Each subsequent entry is the byte offset after a '\n'. */
static void build_line_table(SourceFile* file) {
    /* Count lines first */
    uint32_t count = 1;  /* At least one line */
    for (uint32_t i = 0; i < file->size; i++) {
        if (file->source[i] == '\n') count++;
    }

    file->line_starts = (uint32_t*)malloc(count * sizeof(uint32_t));
    file->line_count = count;
    file->line_starts[0] = 0;

    uint32_t idx = 1;
    for (uint32_t i = 0; i < file->size; i++) {
        if (file->source[i] == '\n' && idx < count) {
            file->line_starts[idx++] = i + 1;
        }
    }
}

/* === SourceMap Lifecycle === */

SourceMap* srcmap_new(void) {
    SourceMap* map = (SourceMap*)calloc(1, sizeof(SourceMap));
    map->file_capacity = 8;
    map->files = (SourceFile*)calloc(map->file_capacity, sizeof(SourceFile));
    map->next_base = 1;  /* Start at 1 so byte 0 is "no position" */

    map->interned_capacity = 16;
    map->interned_spans = (SpanData*)calloc(map->interned_capacity, sizeof(SpanData));
    return map;
}

void srcmap_free(SourceMap* map) {
    if (!map) return;
    for (uint32_t i = 0; i < map->file_count; i++) {
        free((void*)map->files[i].filename);
        free((void*)map->files[i].source);
        free(map->files[i].line_starts);
    }
    free(map->files);
    free(map->interned_spans);
    free(map);
}

/* === File Registration === */

uint32_t srcmap_add_file(SourceMap* map, const char* filename, const char* source, uint32_t source_len) {
    /* Grow if needed */
    if (map->file_count >= map->file_capacity) {
        map->file_capacity *= 2;
        map->files = (SourceFile*)realloc(map->files, map->file_capacity * sizeof(SourceFile));
    }

    uint16_t file_id = (uint16_t)map->file_count;
    SourceFile* f = &map->files[map->file_count++];

    f->filename = strdup(filename);
    f->base = map->next_base;
    f->size = source_len;
    f->source = (char*)malloc(source_len + 1);
    memcpy((void*)f->source, source, source_len);
    ((char*)f->source)[source_len] = '\0';
    f->file_id = file_id;

    build_line_table(f);

    /* Advance next_base past this file (with 1-byte gap for safety) */
    map->next_base += source_len + 1;

    return f->base;
}

/* === File Lookup === */

SourceFile* srcmap_file_for_pos(SourceMap* map, BytePos pos) {
    if (!map || map->file_count == 0 || pos == 0) return NULL;

    /* Binary search: find the file whose base <= pos < base + size */
    uint32_t lo = 0, hi = map->file_count;
    while (lo < hi) {
        uint32_t mid = lo + (hi - lo) / 2;
        if (map->files[mid].base + map->files[mid].size < pos) {
            lo = mid + 1;
        } else if (map->files[mid].base > pos) {
            hi = mid;
        } else {
            return &map->files[mid];
        }
    }
    /* Fallback: check last file */
    if (lo > 0) {
        SourceFile* f = &map->files[lo - 1];
        if (pos >= f->base && pos < f->base + f->size) {
            return f;
        }
    }
    return NULL;
}

/* === Line/Column Resolution === */

/* Binary search line_starts to find the line containing a local offset */
static uint32_t find_line(SourceFile* file, uint32_t local_offset) {
    uint32_t lo = 0, hi = file->line_count;
    while (lo + 1 < hi) {
        uint32_t mid = lo + (hi - lo) / 2;
        if (file->line_starts[mid] <= local_offset) {
            lo = mid;
        } else {
            hi = mid;
        }
    }
    return lo;  /* 0-based line index */
}

ResolvedPos srcmap_resolve(SourceMap* map, BytePos pos) {
    ResolvedPos r = {0};
    if (pos == 0) {
        r.filename = "<unknown>";
        r.line = 0;
        r.column = 0;
        return r;
    }

    SourceFile* file = srcmap_file_for_pos(map, pos);
    if (!file) {
        r.filename = "<unknown>";
        r.line = 0;
        r.column = 0;
        r.byte_offset = pos;
        return r;
    }

    uint32_t local = pos - file->base;
    uint32_t line_idx = find_line(file, local);

    r.filename = file->filename;
    r.line = line_idx + 1;  /* 1-based */
    r.column = local - file->line_starts[line_idx] + 1;  /* 1-based */
    r.byte_offset = pos;
    r.file_id = file->file_id;
    return r;
}

ResolvedSpan srcmap_resolve_span(SourceMap* map, Span span) {
    ResolvedSpan rs = {0};
    if (span_is_none(span)) return rs;

    BytePos lo = span_lo(span);
    BytePos hi = span_hi(span);

    rs.start = srcmap_resolve(map, lo);
    rs.end = srcmap_resolve(map, hi);

    /* Get the source line text for the start position */
    SourceFile* file = srcmap_file_for_pos(map, lo);
    if (file) {
        uint32_t local = lo - file->base;
        uint32_t line_idx = find_line(file, local);
        uint32_t line_start = file->line_starts[line_idx];
        uint32_t line_end;
        if (line_idx + 1 < file->line_count) {
            line_end = file->line_starts[line_idx + 1];
            /* Strip trailing newline */
            if (line_end > line_start && file->source[line_end - 1] == '\n') {
                line_end--;
            }
        } else {
            line_end = file->size;
        }
        rs.line_text = file->source + line_start;
        rs.line_text_len = line_end - line_start;
    }
    return rs;
}

/* === Source Line Text === */

const char* srcmap_line_text(SourceFile* file, uint32_t line_idx, uint32_t* out_len) {
    if (!file || line_idx >= file->line_count) return NULL;

    uint32_t start = file->line_starts[line_idx];
    uint32_t end;
    if (line_idx + 1 < file->line_count) {
        end = file->line_starts[line_idx + 1];
        if (end > start && file->source[end - 1] == '\n') end--;
    } else {
        end = file->size;
    }

    if (out_len) *out_len = end - start;
    return file->source + start;
}

/* === Span Interning === */

Span srcmap_intern_span(SourceMap* map, SpanData data) {
    if (map->interned_count >= map->interned_capacity) {
        map->interned_capacity *= 2;
        map->interned_spans = (SpanData*)realloc(
            map->interned_spans, map->interned_capacity * sizeof(SpanData));
    }

    uint32_t idx = map->interned_count++;
    map->interned_spans[idx] = data;

    Span s;
    s.interned.index = idx;
    s.interned.marker1 = SPAN_INTERNED_MARKER;
    s.interned.marker2 = SPAN_INTERNED_MARKER;
    return s;
}

SpanData srcmap_lookup_span(SourceMap* map, Span s) {
    if (!span_is_interned(s)) {
        SpanData d;
        d.lo = s.inline_span.lo;
        d.hi = s.inline_span.lo + s.inline_span.len;
        d.ctxt = s.inline_span.ctxt;
        d.file_id = 0;  /* Must resolve via binary search */
        return d;
    }
    assert(s.interned.index < map->interned_count);
    return map->interned_spans[s.interned.index];
}
