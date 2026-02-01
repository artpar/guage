#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <stdatomic.h>
#else
#define isatty(fd) 0
#define fileno _fileno
#endif
#include "cell.h"
#include "span.h"
#include "intern.h"
#include "siphash.h"
#include "primitives.h"
#include "eval.h"
#include "module.h"
#include "actor.h"
#include "linenoise.h"
#include "diagnostic.h"
#include "scheduler.h"
#include <time.h>
#include <math.h>

/* Evaluator uses goto-based tail call optimization (TCO) */

#define MAX_INPUT 4096

/* Simple S-expression parser */

typedef struct {
    const char* input;
    int pos;
    int line;      /* Current line number (1-based) */
    int column;    /* Current column number (1-based) */
    uint32_t file_base; /* Base offset in SourceMap global byte space */
} Parser;

/* Get current global byte position for span construction */
static inline BytePos parser_pos(Parser* p) {
    return p->file_base + (uint32_t)p->pos;
}

static void skip_whitespace(Parser* p) {
    while (1) {
        /* Skip whitespace */
        while (p->input[p->pos] == ' ' ||
               p->input[p->pos] == '\t' ||
               p->input[p->pos] == '\n' ||
               p->input[p->pos] == '\r') {
            /* Track line/column */
            if (p->input[p->pos] == '\n') {
                p->line++;
                p->column = 1;
            } else if (p->input[p->pos] == '\t') {
                p->column += 4;  /* Assume tab = 4 spaces */
            } else {
                p->column++;
            }
            p->pos++;
        }

        /* Skip comments (;  to end of line) */
        if (p->input[p->pos] == ';') {
            while (p->input[p->pos] != '\0' &&
                   p->input[p->pos] != '\n') {
                p->pos++;
                p->column++;
            }
            continue;  /* Go back and skip more whitespace */
        }

        break;  /* No more whitespace or comments */
    }
}

static Cell* parse_expr(Parser* p);

static Cell* parse_list(Parser* p) {
    BytePos lo = parser_pos(p);

    /* Skip '(' */
    p->pos++;
    p->column++;
    skip_whitespace(p);

    /* Empty list */
    if (p->input[p->pos] == ')') {
        p->pos++;
        p->column++;
        Cell* nil = cell_nil();
        nil->span = span_new(lo, parser_pos(p));
        return nil;
    }

    /* Parse elements */
    Cell* result = cell_nil();
    Cell* tail = NULL;

    while (p->input[p->pos] != ')' && p->input[p->pos] != '\0') {
        Cell* elem = parse_expr(p);
        Cell* new_pair = cell_cons(elem, cell_nil());

        if (tail == NULL) {
            result = new_pair;
            tail = new_pair;
        } else {
            tail->data.pair.cdr = new_pair;
            tail = new_pair;
        }

        skip_whitespace(p);
    }

    if (p->input[p->pos] == ')') {
        p->pos++;
        p->column++;
    }

    /* Stamp span on the list head */
    result->span = span_new(lo, parser_pos(p));
    return result;
}

static Cell* parse_number(Parser* p) {
    BytePos lo = parser_pos(p);
    double value = 0;
    int64_t ival = 0;
    int is_negative = 0;
    int has_decimal = 0;

    if (p->input[p->pos] == '-') {
        is_negative = 1;
        p->pos++;
        p->column++;
    }

    while (p->input[p->pos] >= '0' && p->input[p->pos] <= '9') {
        value = value * 10 + (p->input[p->pos] - '0');
        ival = ival * 10 + (p->input[p->pos] - '0');
        p->pos++;
        p->column++;
    }

    /* Handle decimal point */
    if (p->input[p->pos] == '.') {
        has_decimal = 1;
        p->pos++;
        p->column++;
        double fraction = 0;
        double divisor = 10;
        while (p->input[p->pos] >= '0' && p->input[p->pos] <= '9') {
            fraction += (p->input[p->pos] - '0') / divisor;
            divisor *= 10;
            p->pos++;
            p->column++;
        }
        value += fraction;
    }

    if (is_negative) { value = -value; ival = -ival; }

    /* Integer suffix 'i' — no decimal allowed */
    if (!has_decimal && p->input[p->pos] == 'i') {
        p->pos++;
        p->column++;
        Cell* c = cell_integer(ival);
        c->span = span_new(lo, parser_pos(p));
        return c;
    }

    Cell* c = cell_number(value);
    c->span = span_new(lo, parser_pos(p));
    return c;
}

/* Parse hex integer literal: digits already started after '#x' prefix */
static Cell* parse_hex_integer(Parser* p) {
    BytePos lo = parser_pos(p);
    int64_t value = 0;
    int is_negative = 0;

    if (p->input[p->pos] == '-') {
        is_negative = 1;
        p->pos++;
        p->column++;
    }

    while (1) {
        char ch = p->input[p->pos];
        if (ch >= '0' && ch <= '9')      { value = (value << 4) | (ch - '0'); }
        else if (ch >= 'a' && ch <= 'f') { value = (value << 4) | (ch - 'a' + 10); }
        else if (ch >= 'A' && ch <= 'F') { value = (value << 4) | (ch - 'A' + 10); }
        else break;
        p->pos++;
        p->column++;
    }

    if (is_negative) value = -value;

    /* Require 'i' suffix for hex integers */
    if (p->input[p->pos] == 'i') {
        p->pos++;
        p->column++;
    }

    Cell* c = cell_integer(value);
    c->span = span_new(lo, parser_pos(p));
    return c;
}

static Cell* parse_symbol(Parser* p) {
    BytePos lo = parser_pos(p);
    char buffer[256];
    int i = 0;

    /* Accept any non-whitespace, non-delimiter character */
    /* This includes UTF-8 symbols like ⊕, λ, etc. */
    while (p->input[p->pos] != '\0' &&
           p->input[p->pos] != ' ' &&
           p->input[p->pos] != '\t' &&
           p->input[p->pos] != '\n' &&
           p->input[p->pos] != '\r' &&
           p->input[p->pos] != '(' &&
           p->input[p->pos] != ')') {
        buffer[i++] = p->input[p->pos++];
        p->column++;  /* Count each byte (UTF-8 safe for display) */
        if (i >= 255) break;
    }
    buffer[i] = '\0';

    Cell* c = cell_symbol(buffer);
    c->span = span_new(lo, parser_pos(p));
    return c;
}

static Cell* parse_string(Parser* p) {
    BytePos lo = parser_pos(p);
    /* Skip opening quote */
    p->pos++;
    p->column++;

    char buffer[1024];
    int i = 0;

    /* Read until closing quote */
    while (p->input[p->pos] != '"' && p->input[p->pos] != '\0') {
        /* Handle escape sequences */
        if (p->input[p->pos] == '\\' && p->input[p->pos + 1] != '\0') {
            p->pos++;  /* Skip backslash */
            p->column++;
            switch (p->input[p->pos]) {
                case 'n':  buffer[i++] = '\n'; break;
                case 't':  buffer[i++] = '\t'; break;
                case 'r':  buffer[i++] = '\r'; break;
                case '\\': buffer[i++] = '\\'; break;
                case '"':  buffer[i++] = '"'; break;
                default:   buffer[i++] = p->input[p->pos]; break;
            }
            p->pos++;
            p->column++;
        } else {
            buffer[i++] = p->input[p->pos++];
            p->column++;
        }

        if (i >= 1023) break;  /* Buffer overflow protection */
    }

    buffer[i] = '\0';

    /* Skip closing quote */
    if (p->input[p->pos] == '"') {
        p->pos++;
        p->column++;
    }

    Cell* c = cell_string(buffer);
    c->span = span_new(lo, parser_pos(p));
    return c;
}

static Cell* parse_expr(Parser* p) {
    skip_whitespace(p);

    /* End of input */
    if (p->input[p->pos] == '\0') {
        return NULL;
    }

    /* List */
    if (p->input[p->pos] == '(') {
        return parse_list(p);
    }

    /* String literal */
    if (p->input[p->pos] == '"') {
        return parse_string(p);
    }

    /* Number or Boolean with # prefix */
    if (p->input[p->pos] == '#') {
        BytePos lo = parser_pos(p);
        p->pos++;
        p->column++;
        /* Boolean */
        if (p->input[p->pos] == 't') {
            p->pos++;
            p->column++;
            Cell* c = cell_bool(true);
            c->span = span_new(lo, parser_pos(p));
            return c;
        } else if (p->input[p->pos] == 'f') {
            p->pos++;
            p->column++;
            Cell* c = cell_bool(false);
            c->span = span_new(lo, parser_pos(p));
            return c;
        }
        /* Hex integer literal: #xFFi or #xFF */
        else if (p->input[p->pos] == 'x' || p->input[p->pos] == 'X') {
            p->pos++;
            p->column++;
            Cell* c = parse_hex_integer(p);
            c->span = span_new(lo, parser_pos(p));
            return c;
        }
        /* Number with # prefix — lo already captured before '#' */
        else if ((p->input[p->pos] >= '0' && p->input[p->pos] <= '9') ||
                 (p->input[p->pos] == '-' && p->input[p->pos + 1] >= '0' && p->input[p->pos + 1] <= '9')) {
            Cell* c = parse_number(p);
            /* Override span to include the '#' prefix */
            c->span = span_new(lo, parser_pos(p));
            return c;
        }
        /* Invalid # syntax - backtrack */
        p->pos--;
        p->column--;
    }

    /* Number without prefix */
    if ((p->input[p->pos] >= '0' && p->input[p->pos] <= '9') ||
        (p->input[p->pos] == '-' && p->input[p->pos + 1] >= '0' && p->input[p->pos + 1] <= '9')) {
        return parse_number(p);
    }

    /* Symbol */
    return parse_symbol(p);
}

Cell* parse(const char* input) {
    /* Register input with SourceMap if available */
    uint32_t file_base = 0;
    if (g_source_map) {
        file_base = srcmap_add_file(g_source_map, "<repl>", input, (uint32_t)strlen(input));
    }
    Parser p = {input, 0, 1, 1, file_base};
    return parse_expr(&p);
}

/* Parse one expression from input at *pos with given file_base.
 * Advances *pos past the parsed expression (including trailing whitespace).
 * Returns NULL at end of input. */
Cell* parse_next(const char* input, int* pos, uint32_t file_base) {
    Parser p = {input, *pos, 1, 1, file_base};

    /* Compute correct line/column from start of file to current pos */
    for (int i = 0; i < *pos; i++) {
        if (input[i] == '\n') {
            p.line++;
            p.column = 1;
        } else {
            p.column++;
        }
    }

    Cell* result = parse_expr(&p);
    *pos = p.pos;
    return result;
}

/* Count parentheses balance in string */
static int paren_balance(const char* str) {
    int balance = 0;
    int i = 0;
    int in_comment = 0;
    int in_string = 0;

    while (str[i] != '\0') {
        /* Handle string boundaries */
        if (str[i] == '"' && (i == 0 || str[i-1] != '\\')) {
            in_string = !in_string;
        }

        if (str[i] == ';' && !in_string) {
            in_comment = 1;
        } else if (str[i] == '\n') {
            in_comment = 0;
        } else if (!in_comment && !in_string) {
            if (str[i] == '(') balance++;
            else if (str[i] == ')') balance--;
        }
        i++;
    }
    return balance;
}

/* REPL command handler */
static void handle_help_command(EvalContext* ctx, const char* cmd) {
    (void)ctx;  /* Unused, but kept for future use */

    /* Trim whitespace from command */
    while (*cmd == ' ' || *cmd == '\t') cmd++;

    /* :help with no args - show all commands */
    if (*cmd == '\0' || *cmd == '\n') {
        printf("\n╔═══════════════════════════════════════════════════════════╗\n");
        printf("║  Guage REPL Help System                                   ║\n");
        printf("╠═══════════════════════════════════════════════════════════╣\n");
        printf("║  REPL Commands:                                           ║\n");
        printf("║    :help              List all REPL commands              ║\n");
        printf("║    :help <symbol>     Show primitive documentation        ║\n");
        printf("║    :primitives        List all primitive symbols          ║\n");
        printf("║    :modules           List loaded modules                 ║\n");
        printf("║                                                           ║\n");
        printf("║  Special Forms:                                           ║\n");
        printf("║    λ, ≔, ?, ∇         Core language constructs           ║\n");
        printf("║                                                           ║\n");
        printf("║  Example Usage:                                           ║\n");
        printf("║    :help ⊕            Show documentation for addition    ║\n");
        printf("║    :primitives        List all available primitives      ║\n");
        printf("╚═══════════════════════════════════════════════════════════╝\n\n");
        return;
    }

    /* :help <symbol> - show primitive documentation */
    char symbol[256];
    int i = 0;
    while (*cmd && *cmd != ' ' && *cmd != '\t' && *cmd != '\n' && i < 255) {
        symbol[i++] = *cmd++;
    }
    symbol[i] = '\0';

    const Primitive* prim = primitive_lookup_by_name(symbol);
    if (prim == NULL) {
        printf("Unknown primitive: %s\n", symbol);
        printf("Type :primitives to see all available primitives\n");
        return;
    }

    printf("\n┌─────────────────────────────────────────────────────────┐\n");
    printf("│ Primitive: %s\n", prim->name);
    printf("├─────────────────────────────────────────────────────────┤\n");
    printf("│ Description: %s\n", prim->doc.description);
    printf("│ Type: %s\n", prim->doc.type_signature);
    printf("│ Arity: %d\n", prim->arity);
    printf("└─────────────────────────────────────────────────────────┘\n\n");
}

static void handle_primitives_command(void) {
    extern Primitive primitives[];  /* From primitives.c */

    printf("\n╔═══════════════════════════════════════════════════════════╗\n");
    printf("║  Guage Primitives (78 total)                             ║\n");
    printf("╠═══════════════════════════════════════════════════════════╣\n");

    /* Core Lambda Calculus */
    printf("║  Core Lambda Calculus:                                    ║\n");
    printf("║    ⟨⟩  ◁  ▷                                              ║\n");
    printf("║                                                           ║\n");

    /* Metaprogramming */
    printf("║  Metaprogramming:                                         ║\n");
    printf("║    ⌜  ⌞                                                   ║\n");
    printf("║                                                           ║\n");

    /* Comparison & Logic */
    printf("║  Comparison & Logic:                                      ║\n");
    printf("║    ≡  ≢  ∧  ∨  ¬                                         ║\n");
    printf("║                                                           ║\n");

    /* Arithmetic */
    printf("║  Arithmetic:                                              ║\n");
    printf("║    ⊕  ⊖  ⊗  ⊘  %%  <  >  ≤  ≥                            ║\n");
    printf("║                                                           ║\n");

    /* Type Predicates */
    printf("║  Type Predicates:                                         ║\n");
    printf("║    ℕ?  𝔹?  :?  ∅?  ⟨⟩?  #?  ≈?  ⚠?                       ║\n");
    printf("║                                                           ║\n");

    /* Debug & Testing */
    printf("║  Debug & Testing:                                         ║\n");
    printf("║    ⚠  ⊢  ⟲  ⧉  ⊛  ≟  ⊨                                   ║\n");
    printf("║                                                           ║\n");

    /* I/O */
    printf("║  I/O:                                                     ║\n");
    printf("║    ≋  ≋≈  ≋←  ≋⊳  ≋⊲  ≋⊕  ≋?  ≋∅?                        ║\n");
    printf("║                                                           ║\n");

    /* Modules */
    printf("║  Modules:                                                 ║\n");
    printf("║    ⋘  ⋖  ⌂⊚  ⌂⊚→                                         ║\n");
    printf("║                                                           ║\n");

    /* Strings */
    printf("║  Strings:                                                 ║\n");
    printf("║    ≈  ≈#  ≈⊙  ≈⊕  ≈⊂  ≈≡  ≈<  ≈>                         ║\n");
    printf("║                                                           ║\n");

    /* Structures */
    printf("║  Structures (Leaf):                                       ║\n");
    printf("║    ⊙≔  ⊙  ⊙→  ⊙←  ⊙?                                     ║\n");
    printf("║                                                           ║\n");

    printf("║  Structures (Node/ADT):                                   ║\n");
    printf("║    ⊚≔  ⊚  ⊚→  ⊚?                                         ║\n");
    printf("║                                                           ║\n");

    printf("║  Structures (Graph):                                      ║\n");
    printf("║    ⊝≔  ⊝  ⊝⊕  ⊝⊗  ⊝→  ⊝?                                 ║\n");
    printf("║                                                           ║\n");

    /* CFG/DFG */
    printf("║  CFG/DFG Analysis:                                        ║\n");
    printf("║    ⌂⇝  ⌂⇝⊳                                               ║\n");
    printf("║                                                           ║\n");

    /* Effects & Actors (placeholders) */
    printf("║  Effects (placeholder):                                   ║\n");
    printf("║    ⟪⟫  ↯  ⤴  ≫                                           ║\n");
    printf("║                                                           ║\n");

    printf("║  Actors (placeholder):                                    ║\n");
    printf("║    ⟳  →!  ←?                                             ║\n");
    printf("╠═══════════════════════════════════════════════════════════╣\n");
    printf("║  Use :help <symbol> to see detailed documentation        ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n\n");
}

static void handle_modules_command(EvalContext* ctx) {
    (void)ctx;  /* Unused, but kept for future use */

    printf("\n┌─────────────────────────────────────────────────────────┐\n");
    printf("│ Loaded Modules:\n");
    printf("├─────────────────────────────────────────────────────────┤\n");

    /* Get module registry using ⌂⊚ primitive */
    Cell* registry = prim_module_info(cell_nil());

    if (cell_is_nil(registry)) {
        printf("│ (no modules loaded)\n");
    } else {
        /* registry is a flat list of strings */
        Cell* current = registry;
        int count = 0;
        while (!cell_is_nil(current)) {
            if (cell_is_pair(current)) {
                Cell* module_name = cell_car(current);
                if (cell_is_string(module_name)) {
                    printf("│ %d. %s\n", ++count, cell_get_string(module_name));
                }
                current = cell_cdr(current);
            } else {
                break;
            }
        }
        if (count == 0) {
            printf("│ (no modules loaded)\n");
        }
    }

    cell_release(registry);
    printf("└─────────────────────────────────────────────────────────┘\n\n");
}

/* Tab completion callback for linenoise */
static void completion_callback(const char *buf, linenoiseCompletions *lc) {
    /* Complete REPL commands */
    if (buf[0] == ':') {
        const char *commands[] = {":help", ":primitives", ":modules", NULL};
        for (int i = 0; commands[i]; i++) {
            if (strncmp(buf, commands[i], strlen(buf)) == 0) {
                linenoiseAddCompletion(lc, commands[i]);
            }
        }
        return;
    }

    /* Complete common primitive names and special forms */
    const char *symbols[] = {
        /* Special forms */
        "λ", "≔", "?", "∇", "⌜", "⌞",
        /* Constants */
        "#t", "#f", "∅",
        /* Core */
        "⟨⟩", "◁", "▷",
        /* Arithmetic */
        "⊕", "⊖", "⊗", "⊘", "÷", "%", "<", ">", "≤", "≥",
        /* Logic */
        "≡", "≢", "∧", "∨", "¬",
        /* Type predicates */
        "ℕ?", "𝔹?", ":?", "∅?", "⟨⟩?", "#?", "≈?", "⚠?",
        /* Debug */
        "⚠", "⊢", "⟲", "⧉", "⊛", "≟", "⊨",
        /* I/O */
        "≋", "≋≈", "≋←", "≋⊳", "≋⊲", "≋⊕", "≋?", "≋∅?",
        /* Modules */
        "⋘", "⋖", "⌂⊚", "⌂⊚→",
        /* Strings */
        "≈", "≈#", "≈⊙", "≈⊕", "≈⊂", "≈≡", "≈<", "≈>", "≈→",
        /* Structures */
        "⊙≔", "⊙", "⊙→", "⊙←", "⊙?",
        "⊚≔", "⊚", "⊚→", "⊚?",
        "⊝≔", "⊝", "⊝⊕", "⊝⊗", "⊝→", "⊝?",
        /* Math */
        "√", "^", "|", "⌊⌋", "⌈⌉", "⌊⌉", "min", "max",
        "sin", "cos", "tan", "asin", "acos", "atan", "atan2",
        "log", "log10", "exp", "π", "e", "rand", "rand-int",
        /* CFG/DFG */
        "⌂⇝", "⌂⇝⊳",
        /* Auto-doc */
        "⌂", "⌂∈", "⌂≔", "⌂⊛", "⌂⊨",
        NULL
    };

    for (int i = 0; symbols[i]; i++) {
        if (strncmp(buf, symbols[i], strlen(buf)) == 0) {
            linenoiseAddCompletion(lc, symbols[i]);
        }
    }
}

/* Get home directory path for history file */
static const char* get_history_path(void) {
    static char path[512];
    const char* home = getenv("HOME");
    if (home) {
        snprintf(path, sizeof(path), "%s/.guage_history", home);
        return path;
    }
    return NULL;
}

/* ── Test run result (shared between --test and --load-test) ── */
typedef struct {
    int pass_count;
    int fail_count;
    double elapsed_us;
    uint64_t alloc_delta;
    int had_error;
} TestRunResult;

/* ── Core test logic: parse, eval, drain actors, return results ── */
static TestRunResult run_test_core(const char* filename, char* buffer, size_t len) {
    TestRunResult r = {0};

    /* Register with SourceMap */
    uint32_t file_base = 0;
    if (g_source_map) {
        file_base = srcmap_add_file(g_source_map, filename, buffer, (uint32_t)len);
    }

    /* Initialize coverage bitmap for this file */
    coverage_init((uint32_t)len);

    /* Setup eval context */
    EvalContext* ctx = eval_context_new();
    module_registry_init();
    module_registry_add("<test>");
    eval_define(ctx, "#t", cell_bool(true));
    eval_define(ctx, "#f", cell_bool(false));
    eval_define(ctx, "∅", cell_nil());

    /* Track alloc stats for leak detection */
    uint64_t alloc_start = cell_get_alloc_count();

    /* Parse and eval loop */
    int pos = 0;

    struct timespec t_start, t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_start);

    while (buffer[pos] != '\0') {
        Cell* expr = parse_next(buffer, &pos, file_base);
        if (!expr) break;

        Cell* result = eval(ctx, expr);
        cell_release(expr);

        if (cell_is_error(result)) {
            r.had_error = 1;
        }
        cell_release(result);
    }

    /* Drain pending actor work */
    sched_run_all(100000);

    clock_gettime(CLOCK_MONOTONIC, &t_end);
    r.elapsed_us = (t_end.tv_sec - t_start.tv_sec) * 1e6 +
                   (t_end.tv_nsec - t_start.tv_nsec) / 1e3;

    /* Alloc stats */
    uint64_t alloc_end = cell_get_alloc_count();
    r.alloc_delta = alloc_end - alloc_start;

    /* Snapshot test counters */
    test_get_counts(&r.pass_count, &r.fail_count);

    eval_context_free(ctx);
    return r;
}

/* ── Test file runner (--test mode) ── */
static void run_test_file(const char* filename) {
    /* Read file */
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "{\"t\":\"error\",\"file\":\"%s\",\"msg\":\"cannot open file\"}\n", filename);
        exit(2);
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = (char*)malloc(file_size + 1);
    if (!buffer) {
        fclose(file);
        fprintf(stderr, "{\"t\":\"error\",\"file\":\"%s\",\"msg\":\"out of memory\"}\n", filename);
        exit(2);
    }

    size_t bytes_read = fread(buffer, 1, file_size, file);
    buffer[bytes_read] = '\0';
    fclose(file);

    TestRunResult r = run_test_core(filename, buffer, bytes_read);

    /* Emit structured results and exit */
    test_emit_and_exit(filename, r.had_error, r.elapsed_us, r.alloc_delta);

    /* unreachable — test_emit_and_exit calls exit() */
    free(buffer);
}

#ifndef _WIN32
/* ── Load test: shared memory layout ── */
typedef struct {
    double elapsed_us;
    int pass_count;
    int fail_count;
    uint64_t alloc_delta;
    long peak_rss_kb;
} LoadIterResult;

typedef struct {
    _Atomic int next_iter;
    _Atomic int completed;
    LoadIterResult results[];
} SharedLoadState;

/* qsort comparator for doubles */
static int cmp_double(const void* a, const void* b) {
    double da = *(const double*)a, db = *(const double*)b;
    return (da > db) - (da < db);
}

static void run_load_test(const char* filename, int iterations, int workers,
                          int warmup, int schedulers_per_worker) {
    /* Read file */
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "{\"t\":\"error\",\"file\":\"%s\",\"msg\":\"cannot open file\"}\n", filename);
        exit(2);
    }
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* buffer = (char*)malloc(file_size + 1);
    if (!buffer) { fclose(file); exit(2); }
    size_t bytes_read = fread(buffer, 1, file_size, file);
    buffer[bytes_read] = '\0';
    fclose(file);

    /* Allocate shared memory for results */
    size_t shm_size = sizeof(SharedLoadState) + iterations * sizeof(LoadIterResult);
    SharedLoadState* shared = mmap(NULL, shm_size, PROT_READ | PROT_WRITE,
                                   MAP_SHARED | MAP_ANON, -1, 0);
    if (shared == MAP_FAILED) {
        fprintf(stderr, "{\"t\":\"error\",\"msg\":\"mmap failed\"}\n");
        exit(2);
    }
    atomic_store(&shared->next_iter, 0);
    atomic_store(&shared->completed, 0);
    memset(shared->results, 0, iterations * sizeof(LoadIterResult));

    struct timespec wall_start, wall_end;
    clock_gettime(CLOCK_MONOTONIC, &wall_start);

    /* Fork workers */
    pid_t* pids = malloc(workers * sizeof(pid_t));
    for (int w = 0; w < workers; w++) {
        pid_t pid = fork();
        if (pid < 0) {
            fprintf(stderr, "{\"t\":\"error\",\"msg\":\"fork failed\"}\n");
            exit(2);
        }
        if (pid == 0) {
            /* Child worker */
            while (1) {
                int iter = atomic_fetch_add(&shared->next_iter, 1);
                if (iter >= iterations) _exit(0);

                /* Reset state for this iteration */
                actor_reset_all();
                test_reset_counts();
                sched_shutdown();
                sched_init(schedulers_per_worker);

                /* Re-init source map and interning for this process */
                if (!g_source_map) g_source_map = srcmap_new();

                TestRunResult r = run_test_core(filename, buffer, bytes_read);

                /* Record results */
                struct rusage ru;
                getrusage(RUSAGE_SELF, &ru);

                shared->results[iter].elapsed_us = r.elapsed_us;
                shared->results[iter].pass_count = r.pass_count;
                shared->results[iter].fail_count = r.fail_count;
                shared->results[iter].alloc_delta = r.alloc_delta;
                #ifdef __APPLE__
                shared->results[iter].peak_rss_kb = ru.ru_maxrss / 1024;
                #else
                shared->results[iter].peak_rss_kb = ru.ru_maxrss;
                #endif

                int done = atomic_fetch_add(&shared->completed, 1) + 1;
                fprintf(stderr, "{\"t\":\"load-iter\",\"iter\":%d,\"worker\":%d,"
                        "\"elapsed_us\":%.1f,\"pass\":%d,\"fail\":%d}\n",
                        iter, w, r.elapsed_us, r.pass_count, r.fail_count);

                /* Progress on stderr */
                if (done % 10 == 0 || done == iterations) {
                    fprintf(stderr, "{\"t\":\"load-progress\",\"completed\":%d,\"total\":%d}\n",
                            done, iterations);
                }
            }
        }
        pids[w] = pid;
    }

    /* Wait for all workers */
    for (int w = 0; w < workers; w++) {
        waitpid(pids[w], NULL, 0);
    }

    clock_gettime(CLOCK_MONOTONIC, &wall_end);
    double wall_secs = (wall_end.tv_sec - wall_start.tv_sec) +
                       (wall_end.tv_nsec - wall_start.tv_nsec) / 1e9;

    /* Compute stats (skip warmup iterations) */
    int effective = iterations - warmup;
    if (effective <= 0) effective = iterations; /* fallback if warmup >= iterations */
    int start_idx = iterations - effective;     /* skip first 'warmup' iterations */

    double* latencies = malloc(effective * sizeof(double));
    int total_pass = 0, total_fail = 0;
    long peak_rss = 0;
    double sum = 0;

    for (int i = 0; i < effective; i++) {
        LoadIterResult* lr = &shared->results[start_idx + i];
        latencies[i] = lr->elapsed_us;
        sum += lr->elapsed_us;
        total_pass += lr->pass_count;
        total_fail += lr->fail_count;
        if (lr->peak_rss_kb > peak_rss) peak_rss = lr->peak_rss_kb;
    }

    qsort(latencies, effective, sizeof(double), cmp_double);

    double mean = sum / effective;
    double variance = 0;
    for (int i = 0; i < effective; i++) {
        double d = latencies[i] - mean;
        variance += d * d;
    }
    double stddev = (effective > 1) ? sqrt(variance / (effective - 1)) : 0;

    double p50 = latencies[effective / 2];
    double p95 = latencies[(int)(effective * 0.95)];
    double p99 = latencies[(int)(effective * 0.99)];
    double min_us = latencies[0];
    double max_us = latencies[effective - 1];
    double throughput = (wall_secs > 0) ? effective / wall_secs : 0;

    /* Emit summary */
    fprintf(stderr,
        "{\"t\":\"load-summary\",\"file\":\"%s\",\"iterations\":%d,\"workers\":%d,"
        "\"warmup\":%d,\"stats\":{\"p50_us\":%.1f,\"p95_us\":%.1f,\"p99_us\":%.1f,"
        "\"mean_us\":%.1f,\"stddev_us\":%.1f,\"min_us\":%.1f,\"max_us\":%.1f,"
        "\"throughput_ops_sec\":%.1f,\"peak_rss_kb\":%ld,"
        "\"total_pass\":%d,\"total_fail\":%d}}\n",
        filename, iterations, workers, warmup,
        p50, p95, p99, mean, stddev, min_us, max_us,
        throughput, peak_rss, total_pass, total_fail);

    free(latencies);
    free(pids);
    munmap(shared, shm_size);
    free(buffer);

    exit(total_fail > 0 ? 1 : 0);
}
#endif /* !_WIN32 */

/* REPL */
void repl(void) {
    char input[MAX_INPUT];
    char accumulated[MAX_INPUT * 4];  /* Buffer for multi-line input */
    EvalContext* ctx = eval_context_new();

    /* Initialize module registry */
    module_registry_init();

    /* Add virtual <repl> module for REPL-defined symbols */
    module_registry_add("<repl>");

    printf("Guage: The Ultralanguage\n");
    printf("Type expressions to evaluate. Ctrl+D to exit.\n");
    printf("Type :help for REPL commands.\n\n");

    /* Load some initial definitions */
    printf("Loading primitives...\n");

    /* Define symbolic constants only */
    eval_define(ctx, "#t", cell_bool(true));
    eval_define(ctx, "#f", cell_bool(false));
    eval_define(ctx, "∅", cell_nil());

    printf("Ready.\n\n");

    /* Setup linenoise */
    int is_interactive = isatty(fileno(stdin));

    if (is_interactive) {
        /* Enable multi-line mode for better editing */
        linenoiseSetMultiLine(1);

        /* Set up tab completion */
        linenoiseSetCompletionCallback(completion_callback);

        /* Load command history */
        const char* history_file = get_history_path();
        if (history_file) {
            linenoiseHistoryLoad(history_file);
            linenoiseHistorySetMaxLen(1000);  /* Keep last 1000 commands */
        }
    }

    accumulated[0] = '\0';
    int balance = 0;

    while (1) {
        char* line;

        if (is_interactive) {
            /* Use linenoise for interactive mode */
            const char* prompt = (balance == 0) ? "guage> " : "...    ";
            line = linenoise(prompt);

            if (line == NULL) {
                /* Ctrl+D pressed */
                printf("\n");
                break;
            }

            /* Copy to input buffer */
            strncpy(input, line, MAX_INPUT - 1);
            input[MAX_INPUT - 1] = '\0';
            strncat(input, "\n", MAX_INPUT - strlen(input) - 1);  /* Add newline */

            linenoiseFree(line);
        } else {
            /* Use fgets for non-interactive mode (pipes, files) */
            if (fgets(input, MAX_INPUT, stdin) == NULL) {
                break;
            }
        }

        /* Skip empty lines when not accumulating */
        if (balance == 0 && input[0] == '\n') continue;

        /* Check for REPL commands (only when balanced) */
        if (balance == 0 && input[0] == ':') {
            /* Remove trailing newline */
            size_t len = strlen(input);
            if (len > 0 && input[len-1] == '\n') {
                input[len-1] = '\0';
            }

            /* Parse command */
            if (strncmp(input, ":help", 5) == 0) {
                handle_help_command(ctx, input + 5);
            } else if (strcmp(input, ":primitives") == 0) {
                handle_primitives_command();
            } else if (strcmp(input, ":modules") == 0) {
                handle_modules_command(ctx);
            } else {
                printf("Unknown command: %s\n", input);
                printf("Type :help for available commands\n");
            }
            continue;
        }

        /* Accumulate input */
        strncat(accumulated, input, MAX_INPUT * 4 - strlen(accumulated) - 1);
        balance += paren_balance(input);

        /* If balanced, parse and evaluate */
        if (balance == 0 && strlen(accumulated) > 0) {
            /* Skip if only whitespace */
            int only_whitespace = 1;
            for (size_t i = 0; i < strlen(accumulated); i++) {
                if (accumulated[i] != ' ' && accumulated[i] != '\t' &&
                    accumulated[i] != '\n' && accumulated[i] != '\r') {
                    /* Check if it's part of a comment */
                    if (accumulated[i] == ';') {
                        /* Skip to end of line */
                        while (i < strlen(accumulated) && accumulated[i] != '\n') i++;
                    } else {
                        only_whitespace = 0;
                        break;
                    }
                }
            }

            if (only_whitespace) {
                accumulated[0] = '\0';
                continue;
            }

            /* Parse */
            Cell* expr = parse(accumulated);
            if (expr == NULL) {
                /* Count lines in accumulated input */
                int line_count = 1;
                for (size_t i = 0; i < strlen(accumulated); i++) {
                    if (accumulated[i] == '\n') line_count++;
                }
                printf("Parse error at line %d\n", line_count);
                printf("Hint: Check for unbalanced parentheses or incomplete expressions\n");
                accumulated[0] = '\0';
                continue;
            }

            /* Evaluate */
            Cell* result = eval(ctx, expr);

            /* Print result — use diagnostic renderer for errors */
            if (cell_is_error(result) && g_source_map) {
                diag_render_error(g_source_map, result, stderr);
            } else {
                cell_print(result);
                printf("\n");
            }

            /* Add to history (only for interactive mode) */
            if (is_interactive) {
                /* Trim trailing newlines from accumulated input */
                char* trimmed = accumulated;
                size_t len = strlen(trimmed);
                while (len > 0 && (trimmed[len-1] == '\n' || trimmed[len-1] == '\r')) {
                    trimmed[len-1] = '\0';
                    len--;
                }
                if (len > 0) {
                    linenoiseHistoryAdd(trimmed);
                    /* Save history periodically */
                    const char* history_file = get_history_path();
                    if (history_file) {
                        linenoiseHistorySave(history_file);
                    }
                }
            }

            /* Cleanup */
            cell_release(expr);
            cell_release(result);

            /* Reset accumulator */
            accumulated[0] = '\0';
        } else if (balance < 0) {
            /* Count lines */
            int line_count = 1;
            for (size_t i = 0; i < strlen(accumulated); i++) {
                if (accumulated[i] == '\n') line_count++;
            }
            printf("Error: Unbalanced parentheses (too many closing parens) near line %d\n", line_count);
            printf("Hint: Check for extra ')' or missing '(' in your expression\n");
            accumulated[0] = '\0';
            balance = 0;
        }
    }

    /* Drain pending actor work after eval loop exits.
     * Uniform behavior: interactive (Ctrl-D) and file/pipe (EOF) both drain.
     * sched_run_all self-terminates on deadlock or all actors finished. */
    sched_run_all(100000);

    eval_context_free(ctx);
}

/* Global argc/argv storage for ⊙⌂ primitive */
static int g_argc = 0;
static char** g_argv = NULL;

int guage_get_argc(void) { return g_argc; }
char** guage_get_argv(void) { return g_argv; }

int main(int argc, char** argv) {
    g_argc = argc;
    g_argv = argv;

    /* Parse arguments */
    const char* test_file = NULL;
    const char* load_test_file = NULL;
    int sched_override = 0;       /* 0 = auto-detect */
    int load_iterations = 100;
    int load_workers = 0;         /* 0 = auto-detect CPU count */
    int load_warmup = 2;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--test") == 0 && i + 1 < argc) {
            test_file = argv[++i];
        } else if (strcmp(argv[i], "--load-test") == 0 && i + 1 < argc) {
            load_test_file = argv[++i];
        } else if (strcmp(argv[i], "--schedulers") == 0 && i + 1 < argc) {
            sched_override = atoi(argv[++i]);
            if (sched_override < 1) sched_override = 1;
        } else if (strcmp(argv[i], "--iterations") == 0 && i + 1 < argc) {
            load_iterations = atoi(argv[++i]);
            if (load_iterations < 1) load_iterations = 1;
        } else if (strcmp(argv[i], "--workers") == 0 && i + 1 < argc) {
            load_workers = atoi(argv[++i]);
            if (load_workers < 1) load_workers = 1;
        } else if (strcmp(argv[i], "--warmup") == 0 && i + 1 < argc) {
            load_warmup = atoi(argv[++i]);
            if (load_warmup < 0) load_warmup = 0;
        } else if (argv[i][0] != '-') {
            /* Positional arg: treat as test file */
            test_file = argv[i];
        }
    }

    /* Auto-detect CPU count */
    int num_cpus = 1;
#ifndef _WIN32
    {
        long n = sysconf(_SC_NPROCESSORS_ONLN);
        if (n > 0) num_cpus = (int)n;
    }
#endif

    /* Default workers to CPU count if not specified */
    if (load_workers == 0) load_workers = num_cpus;

    guage_siphash_init();
    intern_init();
    intern_preload();

    /* Initialize global SourceMap for span tracking */
    g_source_map = srcmap_new();

    /* Initialize sentinel (immortal) error cells */
    error_sentinels_init();

    /* Initialize scheduler subsystem:
     * --test: single scheduler (deterministic) unless --schedulers overrides
     * --load-test: single scheduler per parent (workers handle their own)
     * REPL: auto-detect CPU count for real parallelism
     * --schedulers N: always overrides */
    int sched_count_init;
    if (sched_override > 0) {
        sched_count_init = sched_override;
    } else if (test_file || load_test_file) {
        sched_count_init = 1;
    } else {
        sched_count_init = num_cpus;
    }
    sched_init(sched_count_init);

    if (load_test_file) {
#ifndef _WIN32
        int sched_per_worker = sched_override > 0 ? sched_override : 1;
        run_load_test(load_test_file, load_iterations, load_workers,
                      load_warmup, sched_per_worker);
#else
        fprintf(stderr, "load-test not supported on Windows\n");
        exit(2);
#endif
    } else if (test_file) {
        /* Deterministic scheduling for reproducible tests */
        sched_set_deterministic(1, sched_hash_seed(test_file));
        run_test_file(test_file);
    } else {
        repl();
    }

    sched_shutdown();
    srcmap_free(g_source_map);
    g_source_map = NULL;
    return 0;
}
