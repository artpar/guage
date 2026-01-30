#ifndef GUAGE_DIAGNOSTIC_H
#define GUAGE_DIAGNOSTIC_H

#include "span.h"
#include "cell.h"
#include <stdio.h>

/* =========================================================================
 * Diagnostic Engine (Rust/Elm/Zig Combined)
 *
 * Structured diagnostics with multi-span support, FixIt suggestions,
 * conversational error messages, and Levenshtein fuzzy matching.
 * Renders to terminal (with color) and JSON (LSP-compatible).
 * ========================================================================= */

/* === Severity Levels === */

typedef enum {
    DIAG_ERROR,           /* Fatal: prevents execution */
    DIAG_WARNING,         /* Suspicious but valid */
    DIAG_NOTE,            /* Additional context ("defined here") */
    DIAG_HELP,            /* Actionable suggestion ("try X instead") */
    DIAG_HINT             /* Subtle nudge (LSP DiagnosticSeverity.Hint) */
} DiagLevel;

/* === Labeled Span === */

typedef struct {
    Span span;            /* Where in source */
    const char* label;    /* Text displayed at this location (NULL = no label) */
    bool is_primary;      /* Primary (^^^) vs secondary (---) underline */
} DiagSpan;

/* === FixIt Suggestion === */

typedef enum {
    FIX_MACHINE_SAFE,     /* Auto-applicable (no human review needed) */
    FIX_HAS_PLACEHOLDERS, /* Contains <placeholder> requiring human input */
    FIX_MAYBE_INCORRECT,  /* Might fix it, might not */
    FIX_UNSPECIFIED
} FixApplicability;

typedef struct {
    Span span;            /* Range to replace */
    const char* new_text; /* Replacement text */
} FixEdit;

typedef struct {
    const char* message;          /* "try using ⚡⊳ for a default value" */
    FixApplicability applicability;
    FixEdit* edits;               /* Array of simultaneous edits */
    uint32_t edit_count;
} FixIt;

/* === Diagnostic === */

typedef struct Diagnostic {
    DiagLevel level;
    const char* message;          /* Primary message */
    const char* error_code;       /* "E0042" style (NULL = none) */

    DiagSpan* spans;              /* Array of labeled spans */
    uint32_t span_count;

    FixIt* fixits;                /* Array of suggestions */
    uint32_t fixit_count;

    struct Diagnostic** children; /* Sub-diagnostics (notes, helps) */
    uint32_t child_count;
} Diagnostic;

/* === Diagnostic Construction === */

Diagnostic* diag_new(DiagLevel level, const char* message);
void diag_set_code(Diagnostic* diag, const char* code);
void diag_add_span(Diagnostic* diag, Span span, const char* label, bool is_primary);
void diag_add_fixit(Diagnostic* diag, const char* message, FixApplicability app,
                    Span span, const char* new_text);
Diagnostic* diag_add_child(Diagnostic* diag, DiagLevel level, const char* message);
void diag_free(Diagnostic* diag);

/* === Rendering === */

/* Terminal rendering (color when isatty, plain otherwise) */
void diag_render(SourceMap* map, Diagnostic* diag, FILE* out);

/* Render an error Cell with full diagnostic output (convenience) */
void diag_render_error(SourceMap* map, Cell* error, FILE* out);

/* JSON output (for future LSP / IDE integration) */
void diag_render_json(SourceMap* map, Diagnostic* diag, FILE* out);

/* === "Did You Mean?" === */

/* Levenshtein distance with early termination */
int levenshtein_distance(const char* s1, const char* s2, int max_dist);

/* Find the closest name in scope to `name`, within `max_distance`.
 * Returns the closest match or NULL. */
const char* diag_suggest_name(const char* name, Cell* env, int max_distance);

#endif /* GUAGE_DIAGNOSTIC_H */
