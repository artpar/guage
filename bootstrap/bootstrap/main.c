#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cell.h"
#include "primitives.h"
#include "eval.h"

#define MAX_INPUT 4096

/* Simple S-expression parser */

typedef struct {
    const char* input;
    int pos;
} Parser;

static void skip_whitespace(Parser* p) {
    while (1) {
        /* Skip whitespace */
        while (p->input[p->pos] == ' ' ||
               p->input[p->pos] == '\t' ||
               p->input[p->pos] == '\n' ||
               p->input[p->pos] == '\r') {
            p->pos++;
        }

        /* Skip comments (;  to end of line) */
        if (p->input[p->pos] == ';') {
            while (p->input[p->pos] != '\0' &&
                   p->input[p->pos] != '\n') {
                p->pos++;
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
    skip_whitespace(p);

    /* Empty list */
    if (p->input[p->pos] == ')') {
        p->pos++;
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
    }

    return result;
}

static Cell* parse_number(Parser* p) {
    double value = 0;
    int is_negative = 0;

    if (p->input[p->pos] == '-') {
        is_negative = 1;
        p->pos++;
    }

    while (p->input[p->pos] >= '0' && p->input[p->pos] <= '9') {
        value = value * 10 + (p->input[p->pos] - '0');
        p->pos++;
    }

    /* Handle decimal point */
    if (p->input[p->pos] == '.') {
        p->pos++;
        double fraction = 0;
        double divisor = 10;
        while (p->input[p->pos] >= '0' && p->input[p->pos] <= '9') {
            fraction += (p->input[p->pos] - '0') / divisor;
            divisor *= 10;
            p->pos++;
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
        if (i >= 255) break;
    }
    buffer[i] = '\0';

    return cell_symbol(buffer);
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

    /* Number or Boolean with # prefix */
    if (p->input[p->pos] == '#') {
        p->pos++;
        /* Boolean */
        if (p->input[p->pos] == 't') {
            p->pos++;
            return cell_bool(true);
        } else if (p->input[p->pos] == 'f') {
            p->pos++;
            return cell_bool(false);
        }
        /* Number with # prefix */
        else if ((p->input[p->pos] >= '0' && p->input[p->pos] <= '9') ||
                 (p->input[p->pos] == '-' && p->input[p->pos + 1] >= '0' && p->input[p->pos + 1] <= '9')) {
            return parse_number(p);
        }
        /* Invalid # syntax - backtrack */
        p->pos--;
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
    Parser p = {input, 0};
    return parse_expr(&p);
}

/* Count parentheses balance in string */
static int paren_balance(const char* str) {
    int balance = 0;
    int i = 0;
    int in_comment = 0;

    while (str[i] != '\0') {
        if (str[i] == ';') {
            in_comment = 1;
        } else if (str[i] == '\n') {
            in_comment = 0;
        } else if (!in_comment) {
            if (str[i] == '(') balance++;
            else if (str[i] == ')') balance--;
        }
        i++;
    }
    return balance;
}

/* REPL */
void repl(void) {
    char input[MAX_INPUT];
    char accumulated[MAX_INPUT * 4];  /* Buffer for multi-line input */
    EvalContext* ctx = eval_context_new();

    printf("Guage: The Ultralanguage\n");
    printf("Type expressions to evaluate. Ctrl+D to exit.\n\n");

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
                printf("Parse error\n");
                accumulated[0] = '\0';
                continue;
            }

            /* Evaluate */
            Cell* result = eval(ctx, expr);

            /* Print result */
            cell_print(result);
            printf("\n");

            /* Cleanup */
            cell_release(expr);
            cell_release(result);

            /* Reset accumulator */
            accumulated[0] = '\0';
        } else if (balance < 0) {
            printf("Error: Unbalanced parentheses (too many closing parens)\n");
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
