#include "pattern.h"
#include "cell.h"
#include "eval.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Helper: Check if pattern is wildcard (_) */
static bool is_wildcard(Cell* pattern) {
    if (!pattern || pattern->type != CELL_ATOM_SYMBOL) {
        return false;
    }
    return strcmp(pattern->data.atom.symbol, "_") == 0;
}

/* Helper: Check if symbol is a keyword (starts with :) */
static bool is_keyword(const char* sym) {
    return sym && sym[0] == ':';
}

/* Helper: Check if pattern is a variable (non-keyword symbol, not wildcard) */
static bool is_variable_pattern(Cell* pattern) {
    if (!pattern || pattern->type != CELL_ATOM_SYMBOL) {
        return false;
    }
    const char* sym = pattern->data.atom.symbol;
    return !is_keyword(sym) && strcmp(sym, "_") != 0;
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

    /* Variable pattern - binds value to name (Day 16) */
    if (is_variable_pattern(pattern)) {
        /* Create binding: (symbol . value) */
        Cell* var_symbol = cell_symbol(pattern->data.atom.symbol);
        cell_retain(value);  /* Retain value for binding */
        Cell* binding = cell_cons(var_symbol, value);

        /* Return success with single binding */
        MatchResult result = {.success = true, .bindings = binding};
        return result;
    }

    /* Keyword symbol literal pattern (e.g., :foo) */
    if (pattern && pattern->type == CELL_ATOM_SYMBOL) {
        /* At this point, it's a keyword (starts with :) */
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

    /* Clauses should be a list of pairs: ((pattern1 result1) (pattern2 result2) ...) */
    /* Each clause is (pattern result) where pattern is data and result is an unevaluated expression */

    /* Walk through the clause list */
    Cell* current = clauses;
    while (current && cell_is_pair(current)) {
        Cell* clause = cell_car(current);

        /* Each clause should be a pair (pattern result) or (pattern . (result)) */
        if (!clause || clause->type != CELL_PAIR) {
            cell_release(value);
            return cell_error("invalid-clause", clause ? clause : cell_nil());
        }

        /* Extract pattern and result */
        Cell* pattern = clause->data.pair.car;
        Cell* rest = clause->data.pair.cdr;

        /* Result can be in two formats:
         * 1. (pattern result) -> rest is (result . nil), extract car
         * 2. (pattern . result) -> rest is the result directly */
        Cell* result_expr;
        if (rest && rest->type == CELL_PAIR) {
            result_expr = rest->data.pair.car;
        } else {
            result_expr = rest;
        }

        /* Try to match the pattern against the value */
        MatchResult match = pattern_try_match(value, pattern);

        if (match.success) {
            /* Match succeeded! Evaluate the result expression */
            Cell* result;

            if (match.bindings) {
                /* Extend environment with pattern bindings */
                Cell* old_env = ctx->env;
                cell_retain(old_env);

                /* Prepend bindings to environment */
                ctx->env = cell_cons(match.bindings, old_env);

                /* Evaluate result in extended environment */
                result = eval(ctx, result_expr);

                /* Restore old environment */
                cell_release(ctx->env);
                ctx->env = old_env;

                /* Release bindings (we got ownership from pattern_try_match) */
                cell_release(match.bindings);
            } else {
                /* No bindings, evaluate in current context */
                result = eval(ctx, result_expr);
            }

            cell_release(value);
            return result;
        }

        /* Try next clause */
        current = current->data.pair.cdr;
    }

    /* No clause matched */
    Cell* error = cell_error("no-match", value);
    cell_release(value);
    return error;
}

/* Free bindings list */
void pattern_free_bindings(Cell* bindings) {
    /* For now, bindings are NULL */
    /* When we add variable patterns (Day 16), we'll need to */
    /* properly free the binding alist */
    (void)bindings;  /* Unused for now */
}
