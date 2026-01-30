#include "diagnostic.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#ifndef _WIN32
#include <unistd.h>
#else
#define isatty(fd) 0
#define fileno _fileno
#endif

/* =========================================================================
 * Diagnostic Engine Implementation
 *
 * Terminal renderer: Rust/Elm hybrid with conversational messages,
 * multi-line source context, primary/secondary underlines, FixIt
 * suggestions, and cause chain display.
 *
 * Levenshtein: Wagner-Fischer O(min(m,n)) space with early termination.
 * ========================================================================= */

/* === ANSI Color Codes === */

static bool use_color(FILE* out) {
    /* Respect NO_COLOR environment variable */
    if (getenv("NO_COLOR")) return false;
    return isatty(fileno(out));
}

#define ANSI_RED     "\x1b[31;1m"
#define ANSI_YELLOW  "\x1b[33;1m"
#define ANSI_BLUE    "\x1b[34;1m"
#define ANSI_GREEN   "\x1b[32;1m"
#define ANSI_CYAN    "\x1b[36;1m"
#define ANSI_MAGENTA "\x1b[35;1m"
#define ANSI_BOLD    "\x1b[1m"
#define ANSI_DIM     "\x1b[2m"
#define ANSI_RESET   "\x1b[0m"

static const char* level_color(DiagLevel level, bool color) {
    if (!color) return "";
    switch (level) {
        case DIAG_ERROR:   return ANSI_RED;
        case DIAG_WARNING: return ANSI_YELLOW;
        case DIAG_NOTE:    return ANSI_BLUE;
        case DIAG_HELP:    return ANSI_GREEN;
        case DIAG_HINT:    return ANSI_CYAN;
    }
    return "";
}

static const char* level_name(DiagLevel level) {
    switch (level) {
        case DIAG_ERROR:   return "error";
        case DIAG_WARNING: return "warning";
        case DIAG_NOTE:    return "note";
        case DIAG_HELP:    return "help";
        case DIAG_HINT:    return "hint";
    }
    return "unknown";
}

static const char* reset_code(bool color) {
    return color ? ANSI_RESET : "";
}

/* === Diagnostic Construction === */

Diagnostic* diag_new(DiagLevel level, const char* message) {
    Diagnostic* d = (Diagnostic*)calloc(1, sizeof(Diagnostic));
    d->level = level;
    d->message = message;  /* Not owned — caller manages lifetime */
    return d;
}

void diag_set_code(Diagnostic* diag, const char* code) {
    diag->error_code = code;
}

void diag_add_span(Diagnostic* diag, Span span, const char* label, bool is_primary) {
    uint32_t idx = diag->span_count++;
    diag->spans = (DiagSpan*)realloc(diag->spans, diag->span_count * sizeof(DiagSpan));
    diag->spans[idx].span = span;
    diag->spans[idx].label = label;
    diag->spans[idx].is_primary = is_primary;
}

void diag_add_fixit(Diagnostic* diag, const char* message, FixApplicability app,
                    Span span, const char* new_text) {
    uint32_t idx = diag->fixit_count++;
    diag->fixits = (FixIt*)realloc(diag->fixits, diag->fixit_count * sizeof(FixIt));
    diag->fixits[idx].message = message;
    diag->fixits[idx].applicability = app;
    diag->fixits[idx].edits = (FixEdit*)malloc(sizeof(FixEdit));
    diag->fixits[idx].edits[0].span = span;
    diag->fixits[idx].edits[0].new_text = new_text;
    diag->fixits[idx].edit_count = 1;
}

Diagnostic* diag_add_child(Diagnostic* diag, DiagLevel level, const char* message) {
    Diagnostic* child = diag_new(level, message);
    uint32_t idx = diag->child_count++;
    diag->children = (Diagnostic**)realloc(diag->children,
                                           diag->child_count * sizeof(Diagnostic*));
    diag->children[idx] = child;
    return child;
}

void diag_free(Diagnostic* diag) {
    if (!diag) return;
    free(diag->spans);
    for (uint32_t i = 0; i < diag->fixit_count; i++) {
        free(diag->fixits[i].edits);
    }
    free(diag->fixits);
    for (uint32_t i = 0; i < diag->child_count; i++) {
        diag_free(diag->children[i]);
    }
    free(diag->children);
    free(diag);
}

/* === Terminal Rendering === */

void diag_render(SourceMap* map, Diagnostic* diag, FILE* out) {
    bool color = use_color(out);

    /* Header: ⚠ E0017: undefined-variable ── app.scm */
    fprintf(out, "\n%s%s", level_color(diag->level, color),
            (diag->level == DIAG_ERROR) ? "⚠ " : "");

    if (diag->error_code) {
        fprintf(out, "%s: ", diag->error_code);
    }
    fprintf(out, "%s%s", diag->message, reset_code(color));

    /* Show filename from primary span */
    for (uint32_t i = 0; i < diag->span_count; i++) {
        if (diag->spans[i].is_primary && !span_is_none(diag->spans[i].span)) {
            ResolvedPos pos = srcmap_resolve(map, span_lo(diag->spans[i].span));
            if (pos.line > 0) {
                fprintf(out, " %s── %s:%u:%u%s",
                        color ? ANSI_DIM : "",
                        pos.filename, pos.line, pos.column,
                        reset_code(color));
            }
            break;
        }
    }
    fprintf(out, "\n");

    /* Source snippets for each labeled span */
    for (uint32_t i = 0; i < diag->span_count; i++) {
        DiagSpan* ds = &diag->spans[i];
        if (span_is_none(ds->span)) continue;

        ResolvedSpan rs = srcmap_resolve_span(map, ds->span);
        if (rs.start.line == 0) continue;

        /* Line number gutter width */
        int gutter = 3;
        if (rs.start.line >= 100) gutter = 4;
        if (rs.start.line >= 1000) gutter = 5;

        /* Show source line */
        fprintf(out, "%s%*u│%s ", color ? ANSI_DIM : "",
                gutter, rs.start.line, reset_code(color));
        if (rs.line_text && rs.line_text_len > 0) {
            fwrite(rs.line_text, 1, rs.line_text_len, out);
        }
        fprintf(out, "\n");

        /* Underline */
        fprintf(out, "%*s│ ", gutter, "");
        uint32_t col_start = rs.start.column > 0 ? rs.start.column - 1 : 0;
        uint32_t col_end = rs.end.line == rs.start.line ? (rs.end.column > 0 ? rs.end.column - 1 : col_start + 1) : col_start + 1;
        if (col_end <= col_start) col_end = col_start + 1;

        for (uint32_t c = 0; c < col_start; c++) fprintf(out, " ");
        fprintf(out, "%s", level_color(diag->level, color));
        char underline_char = ds->is_primary ? '^' : '~';
        for (uint32_t c = col_start; c < col_end; c++) fprintf(out, "%c", underline_char);
        fprintf(out, "%s", reset_code(color));

        if (ds->label) {
            fprintf(out, " %s%s%s", level_color(diag->level, color),
                    ds->label, reset_code(color));
        }
        fprintf(out, "\n");
    }

    /* FixIt suggestions */
    for (uint32_t i = 0; i < diag->fixit_count; i++) {
        FixIt* fix = &diag->fixits[i];
        fprintf(out, "%s  help%s: %s\n",
                color ? ANSI_GREEN : "", reset_code(color), fix->message);

        /* Show the replacement if we have source context */
        for (uint32_t e = 0; e < fix->edit_count; e++) {
            if (!span_is_none(fix->edits[e].span) && fix->edits[e].new_text) {
                ResolvedSpan rs = srcmap_resolve_span(map, fix->edits[e].span);
                if (rs.start.line > 0 && rs.line_text) {
                    int gutter = 3;
                    fprintf(out, "%s%*u│%s ", color ? ANSI_DIM : "",
                            gutter, rs.start.line, reset_code(color));

                    /* Print line with replacement highlighted */
                    uint32_t col_start = rs.start.column > 0 ? rs.start.column - 1 : 0;
                    uint32_t col_end = rs.end.line == rs.start.line ? (rs.end.column > 0 ? rs.end.column - 1 : col_start + 1) : col_start + 1;

                    /* Before replacement */
                    if (col_start <= rs.line_text_len) {
                        fwrite(rs.line_text, 1, col_start, out);
                    }
                    /* Replacement text highlighted */
                    fprintf(out, "%s%s%s", color ? ANSI_GREEN : "",
                            fix->edits[e].new_text, reset_code(color));
                    /* After replacement */
                    if (col_end <= rs.line_text_len) {
                        fwrite(rs.line_text + col_end, 1, rs.line_text_len - col_end, out);
                    }
                    fprintf(out, "\n");
                }
            }
        }
    }

    /* Child diagnostics */
    for (uint32_t i = 0; i < diag->child_count; i++) {
        Diagnostic* child = diag->children[i];
        fprintf(out, "  %s%s%s: %s\n",
                level_color(child->level, color),
                level_name(child->level),
                reset_code(color),
                child->message);
    }

    fprintf(out, "\n");
}

/* === Error Cell Rendering === */

void diag_render_error(SourceMap* map, Cell* error, FILE* out) {
    if (!error || !cell_is_error(error)) return;

    bool color = use_color(out);
    const char* msg = cell_error_message(error);
    Span espan = cell_error_span(error);

    /* Header */
    fprintf(out, "\n%s⚠ %s%s", color ? ANSI_RED : "", msg, color ? ANSI_RESET : "");

    /* Error data */
    Cell* data = cell_error_data(error);
    if (data && !cell_is_nil(data)) {
        fprintf(out, ":");
        cell_print(data);
    }

    /* Source location */
    if (!span_is_none(espan) && map) {
        ResolvedPos pos = srcmap_resolve(map, span_lo(espan));
        if (pos.line > 0) {
            fprintf(out, " %s── %s:%u:%u%s",
                    color ? ANSI_DIM : "",
                    pos.filename, pos.line, pos.column,
                    color ? ANSI_RESET : "");
        }
    }
    fprintf(out, "\n");

    /* Source snippet */
    if (!span_is_none(espan) && map) {
        ResolvedSpan rs = srcmap_resolve_span(map, espan);
        if (rs.start.line > 0 && rs.line_text) {
            int gutter = 3;
            fprintf(out, "%s%*u│%s ", color ? ANSI_DIM : "",
                    gutter, rs.start.line, color ? ANSI_RESET : "");
            fwrite(rs.line_text, 1, rs.line_text_len, out);
            fprintf(out, "\n");

            /* Underline */
            fprintf(out, "%*s│ ", gutter, "");
            uint32_t col_start = rs.start.column > 0 ? rs.start.column - 1 : 0;
            uint32_t col_end = rs.end.line == rs.start.line ? (rs.end.column > 0 ? rs.end.column - 1 : col_start + 1) : col_start + 1;
            if (col_end <= col_start) col_end = col_start + 1;
            for (uint32_t c = 0; c < col_start; c++) fprintf(out, " ");
            fprintf(out, "%s", color ? ANSI_RED : "");
            for (uint32_t c = col_start; c < col_end; c++) fprintf(out, "^");
            fprintf(out, "%s\n", color ? ANSI_RESET : "");
        }
    }

    /* Error return trace */
    uint16_t trace_len = cell_error_trace_len(error);
    uint32_t* trace = cell_error_return_trace(error);
    if (trace_len > 0 && trace && map) {
        fprintf(out, "\n  %sError return trace (most recent first):%s\n",
                color ? ANSI_DIM : "", color ? ANSI_RESET : "");
        uint32_t start = (trace_len > ERROR_TRACE_CAP) ? trace_len - ERROR_TRACE_CAP : 0;
        for (uint32_t i = trace_len; i > start; i--) {
            uint32_t idx = (i - 1) % ERROR_TRACE_CAP;
            ResolvedPos rp = srcmap_resolve(map, trace[idx]);
            if (rp.line > 0) {
                fprintf(out, "  → %s:%u:%u\n", rp.filename, rp.line, rp.column);
            }
        }
    }

    /* Cause chain */
    Cell* cause = cell_error_cause(error);
    if (cause && cell_is_error(cause)) {
        fprintf(out, "  %scaused by:%s\n", color ? ANSI_DIM : "", color ? ANSI_RESET : "");
        diag_render_error(map, cause, out);
    }
}

/* === JSON Rendering === */

void diag_render_json(SourceMap* map, Diagnostic* diag, FILE* out) {
    fprintf(out, "{\"level\":\"%s\"", level_name(diag->level));
    if (diag->error_code) {
        fprintf(out, ",\"code\":\"%s\"", diag->error_code);
    }
    /* Escape message for JSON */
    fprintf(out, ",\"message\":\"");
    for (const char* c = diag->message; *c; c++) {
        if (*c == '"') fprintf(out, "\\\"");
        else if (*c == '\\') fprintf(out, "\\\\");
        else if (*c == '\n') fprintf(out, "\\n");
        else fputc(*c, out);
    }
    fprintf(out, "\"");

    /* Spans */
    fprintf(out, ",\"spans\":[");
    for (uint32_t i = 0; i < diag->span_count; i++) {
        if (i > 0) fprintf(out, ",");
        DiagSpan* ds = &diag->spans[i];
        ResolvedSpan rs = srcmap_resolve_span(map, ds->span);
        fprintf(out, "{\"file\":\"%s\"", rs.start.filename ? rs.start.filename : "");
        fprintf(out, ",\"byte_start\":%u,\"byte_end\":%u",
                span_lo(ds->span), span_hi(ds->span));
        fprintf(out, ",\"line_start\":%u,\"line_end\":%u",
                rs.start.line, rs.end.line);
        fprintf(out, ",\"col_start\":%u,\"col_end\":%u",
                rs.start.column, rs.end.column);
        fprintf(out, ",\"is_primary\":%s", ds->is_primary ? "true" : "false");
        if (ds->label) {
            fprintf(out, ",\"label\":\"");
            for (const char* c = ds->label; *c; c++) {
                if (*c == '"') fprintf(out, "\\\"");
                else fputc(*c, out);
            }
            fprintf(out, "\"");
        }
        fprintf(out, "}");
    }
    fprintf(out, "]");

    /* Children */
    fprintf(out, ",\"children\":[");
    for (uint32_t i = 0; i < diag->child_count; i++) {
        if (i > 0) fprintf(out, ",");
        diag_render_json(map, diag->children[i], out);
    }
    fprintf(out, "]");

    fprintf(out, "}\n");
}

/* === Levenshtein Distance === */

int levenshtein_distance(const char* s1, const char* s2, int max_dist) {
    int len1 = (int)strlen(s1);
    int len2 = (int)strlen(s2);

    /* Quick rejection */
    int diff = len1 - len2;
    if (diff < 0) diff = -diff;
    if (diff > max_dist) return max_dist + 1;

    /* Wagner-Fischer with O(min(m,n)) space */
    if (len1 > len2) {
        const char* tmp = s1; s1 = s2; s2 = tmp;
        int t = len1; len1 = len2; len2 = t;
    }

    int* prev = (int*)malloc((len1 + 1) * sizeof(int));
    int* curr = (int*)malloc((len1 + 1) * sizeof(int));

    for (int i = 0; i <= len1; i++) prev[i] = i;

    for (int j = 1; j <= len2; j++) {
        curr[0] = j;
        int min_in_row = j;
        for (int i = 1; i <= len1; i++) {
            int cost = (s1[i-1] == s2[j-1]) ? 0 : 1;
            curr[i] = prev[i] + 1;              /* deletion */
            if (curr[i-1] + 1 < curr[i])
                curr[i] = curr[i-1] + 1;        /* insertion */
            if (prev[i-1] + cost < curr[i])
                curr[i] = prev[i-1] + cost;     /* substitution */
            if (curr[i] < min_in_row) min_in_row = curr[i];
        }
        /* Early termination */
        if (min_in_row > max_dist) {
            free(prev); free(curr);
            return max_dist + 1;
        }
        int* t = prev; prev = curr; curr = t;
    }

    int result = prev[len1];
    free(prev); free(curr);
    return result;
}

/* === "Did You Mean?" === */

const char* diag_suggest_name(const char* name, Cell* env, int max_distance) {
    const char* best = NULL;
    int best_dist = max_distance + 1;

    /* Walk environment alist */
    Cell* cur = env;
    while (cur && cell_is_pair(cur)) {
        Cell* binding = cell_car(cur);
        if (binding && cell_is_pair(binding)) {
            Cell* key = cell_car(binding);
            if (key && cell_is_symbol(key)) {
                const char* candidate = cell_get_symbol(key);
                int d = levenshtein_distance(name, candidate, max_distance);
                if (d < best_dist) {
                    best_dist = d;
                    best = candidate;
                }
            }
        }
        cur = cell_cdr(cur);
    }

    return best;
}
