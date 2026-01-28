#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cell.h"
#include "primitives.h"
#include "eval.h"
#include "module.h"
#include "trampoline.h"

/* Compile-time flag to enable trampoline evaluator
 * NOTE: Trampoline evaluator is functional for basic cases but not production-ready
 * - Basic arithmetic: ✅ Working
 * - Lambdas: ✅ Working
 * - Simple recursion: ✅ Working
 * - Complex stdlib code: ❌ Causes segfaults (needs debugging)
 * Default: 0 (use stable recursive evaluator) */
#ifndef USE_TRAMPOLINE
#define USE_TRAMPOLINE 0  /* 1 = trampoline, 0 = recursive */
#endif

#define MAX_INPUT 4096

/* Simple S-expression parser */

typedef struct {
    const char* input;
    int pos;
    int line;      /* Current line number (1-based) */
    int column;    /* Current column number (1-based) */
} Parser;

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
    /* Skip '(' */
    p->pos++;
    p->column++;
    skip_whitespace(p);

    /* Empty list */
    if (p->input[p->pos] == ')') {
        p->pos++;
        p->column++;
        return cell_nil();
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

    return result;
}

static Cell* parse_number(Parser* p) {
    double value = 0;
    int is_negative = 0;

    if (p->input[p->pos] == '-') {
        is_negative = 1;
        p->pos++;
        p->column++;
    }

    while (p->input[p->pos] >= '0' && p->input[p->pos] <= '9') {
        value = value * 10 + (p->input[p->pos] - '0');
        p->pos++;
        p->column++;
    }

    /* Handle decimal point */
    if (p->input[p->pos] == '.') {
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

    if (is_negative) value = -value;

    return cell_number(value);
}

static Cell* parse_symbol(Parser* p) {
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

    return cell_symbol(buffer);
}

static Cell* parse_string(Parser* p) {
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

    return cell_string(buffer);
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
        p->pos++;
        p->column++;
        /* Boolean */
        if (p->input[p->pos] == 't') {
            p->pos++;
            p->column++;
            return cell_bool(true);
        } else if (p->input[p->pos] == 'f') {
            p->pos++;
            p->column++;
            return cell_bool(false);
        }
        /* Number with # prefix */
        else if ((p->input[p->pos] >= '0' && p->input[p->pos] <= '9') ||
                 (p->input[p->pos] == '-' && p->input[p->pos + 1] >= '0' && p->input[p->pos + 1] <= '9')) {
            return parse_number(p);
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
    Parser p = {input, 0, 1, 1};  /* pos=0, line=1, column=1 */
    return parse_expr(&p);
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

    accumulated[0] = '\0';
    int balance = 0;
    int is_interactive = isatty(fileno(stdin));

    while (1) {
        if (is_interactive && balance == 0) {
            printf("guage> ");
        } else if (is_interactive && balance > 0) {
            printf("...    ");
        }
        fflush(stdout);

        if (fgets(input, MAX_INPUT, stdin) == NULL) {
            if (is_interactive) printf("\n");
            break;
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
#if USE_TRAMPOLINE
            Cell* result = trampoline_eval(ctx, expr);
#else
            Cell* result = eval(ctx, expr);
#endif

            /* Print result */
            cell_print(result);
            printf("\n");

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

    eval_context_free(ctx);
}

int main(int argc, char** argv) {
    repl();
    return 0;
}
