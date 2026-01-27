#include "pattern.h"
#include "cell.h"
#include "eval.h"
#include <string.h>
#include <stdlib.h>

/* Helper: Check if pattern is wildcard (_) */
static bool is_wildcard(Cell* pattern) {
    if (!pattern || pattern->type != CELL_ATOM_SYMBOL) {
        return false;
    }
    return strcmp(pattern->data.atom.symbol, "_") == 0;
}

/* Helper: Check if two numbers are equal */
static bool numbers_equal(Cell* a, Cell* b) {
    if (a->type != CELL_ATOM_NUMBER || b->type != CELL_ATOM_NUMBER) {
        return false;
    }
    return a->data.atom.number == b->data.atom.number;
}

/* Helper: Check if two booleans are equal */
static bool bools_equal(Cell* a, Cell* b) {
    if (a->type != CELL_ATOM_BOOL || b->type != CELL_ATOM_BOOL) {
        return false;
    }
    return a->data.atom.boolean == b->data.atom.boolean;
}

/* Helper: Check if two symbols are equal */
static bool symbols_equal(Cell* a, Cell* b) {
    if (a->type != CELL_ATOM_SYMBOL || b->type != CELL_ATOM_SYMBOL) {
        return false;
    }
    return strcmp(a->data.atom.symbol, b->data.atom.symbol) == 0;
}

/* Helper: Check if both are nil */
static bool both_nil(Cell* a, Cell* b) {
    return (a == NULL || a->type == CELL_ATOM_NIL) &&
           (b == NULL || b->type == CELL_ATOM_NIL);
}

/* Try to match a value against a pattern */
MatchResult pattern_try_match(Cell* value, Cell* pattern) {
    MatchResult failure = {.success = false, .bindings = NULL};
    MatchResult success = {.success = true, .bindings = NULL};

    /* Wildcard always matches, no bindings */
    if (is_wildcard(pattern)) {
        return success;
    }

    /* Nil pattern */
    if (both_nil(value, pattern)) {
        return success;
    }

    /* Number literal pattern */
    if (pattern && pattern->type == CELL_ATOM_NUMBER) {
        if (value && numbers_equal(value, pattern)) {
            return success;
        }
        return failure;
    }

    /* Boolean literal pattern */
    if (pattern && pattern->type == CELL_ATOM_BOOL) {
        if (value && bools_equal(value, pattern)) {
            return success;
        }
        return failure;
    }

    /* Symbol literal pattern */
    if (pattern && pattern->type == CELL_ATOM_SYMBOL) {
        if (value && symbols_equal(value, pattern)) {
            return success;
        }
        return failure;
    }

    /* Pattern type not supported yet */
    return failure;
}

/* Evaluate a match expression */
Cell* pattern_eval_match(Cell* expr, Cell* clauses, EvalContext* ctx) {
    if (!ctx) {
        return cell_error("no-context", expr);
    }

    /* Evaluate the expression to match */
    Cell* value = eval(ctx, expr);
    if (!value) {
        return cell_error("eval-failed", expr);
    }
    if (value->type == CELL_ERROR) {
        return value;
    }

    /* Try each clause in order */
    Cell* current = clauses;
    while (current && current->type == CELL_PAIR) {
        Cell* clause = current->data.pair.car;

        /* Each clause should be a pair: [pattern result] */
        if (!clause || clause->type != CELL_PAIR) {
            return cell_error("invalid-clause", clause ? clause : cell_nil());
        }

        Cell* pattern = clause->data.pair.car;
        Cell* rest = clause->data.pair.cdr;

        if (!rest || rest->type != CELL_PAIR) {
            return cell_error("invalid-clause", clause);
        }

        Cell* result_expr = rest->data.pair.car;

        /* Try to match the pattern */
        MatchResult match = pattern_try_match(value, pattern);

        if (match.success) {
            /* Match succeeded! Evaluate the result expression */
            /* For now (Day 15), we don't have variable bindings */
            /* Just evaluate the result in the current context */
            Cell* result = eval(ctx, result_expr);

            /* Clean up bindings if any */
            if (match.bindings) {
                pattern_free_bindings(match.bindings);
            }

            return result;
        }

        /* Try next clause */
        current = current->data.pair.cdr;
    }

    /* No clause matched */
    return cell_error("no-match", value);
}

/* Free bindings list */
void pattern_free_bindings(Cell* bindings) {
    /* For now, bindings are NULL */
    /* When we add variable patterns (Day 16), we'll need to */
    /* properly free the binding alist */
    (void)bindings;  /* Unused for now */
}
