#include "pattern.h"
#include "pattern_check.h"
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

/* Helper: Check if clause has guard syntax: (pattern | guard) */
static bool has_guard(Cell* pattern_expr) {
    /* Guard syntax: (pattern | guard)
     * Must be a list with 3 elements: (pattern symbol-| guard) */
    if (!pattern_expr || pattern_expr->type != CELL_PAIR) {
        return false;
    }

    /* Need at least 2 more elements */
    Cell* rest = cell_cdr(pattern_expr);
    if (!rest || rest->type != CELL_PAIR) {
        return false;
    }

    /* Second element should be the pipe symbol "|" */
    Cell* second = cell_car(rest);
    if (!second || second->type != CELL_ATOM_SYMBOL) {
        return false;
    }
    if (strcmp(second->data.atom.symbol, "|") != 0) {
        return false;
    }

    /* Need the guard expression */
    Cell* rest2 = cell_cdr(rest);
    if (!rest2 || rest2->type != CELL_PAIR) {
        return false;
    }

    return true;
}

/* Helper: Extract pattern and guard from guard syntax */
static void extract_pattern_and_guard(Cell* pattern_expr, Cell** pattern_out, Cell** guard_out) {
    /* Guard syntax: (pattern | guard) */
    *pattern_out = cell_car(pattern_expr);

    /* Skip the pipe symbol */
    Cell* rest = cell_cdr(pattern_expr);
    Cell* rest2 = cell_cdr(rest);
    *guard_out = cell_car(rest2);
}

/* Helper: Check if pattern is a pair pattern (⟨⟩ pat1 pat2) */
static bool is_pair_pattern(Cell* pattern) {
    /* Pattern must be a list: (⟨⟩ pat1 pat2) */
    if (!pattern || pattern->type != CELL_PAIR) {
        return false;
    }

    /* First element should be the cons symbol "⟨⟩" */
    Cell* first = cell_car(pattern);
    if (!first || first->type != CELL_ATOM_SYMBOL) {
        return false;
    }
    if (strcmp(first->data.atom.symbol, "⟨⟩") != 0) {
        return false;
    }

    /* Should have at least 2 more elements */
    Cell* rest = cell_cdr(pattern);
    if (!rest || rest->type != CELL_PAIR) {
        return false;
    }

    Cell* second_rest = cell_cdr(rest);
    if (!second_rest || second_rest->type != CELL_PAIR) {
        return false;
    }

    /* Valid pair pattern structure */
    return true;
}

/* Helper: Extract sub-patterns from pair pattern */
static void extract_pair_subpatterns(Cell* pattern, Cell** pat1, Cell** pat2) {
    /* pattern is (⟨⟩ pat1 pat2) */
    /* Skip first element (the ⟨⟩ symbol) */
    Cell* rest = cell_cdr(pattern);
    *pat1 = cell_car(rest);

    Cell* second_rest = cell_cdr(rest);
    *pat2 = cell_car(second_rest);
}

/* Helper: Check if cell is a single binding (var . value) vs list of bindings */
static bool is_single_binding(Cell* cell) {
    if (!cell || cell->type != CELL_PAIR) {
        return false;
    }
    Cell* first = cell_car(cell);
    /* If car is a symbol, it's a single binding */
    return first && first->type == CELL_ATOM_SYMBOL;
}

/* Helper: Append bindings2 to the end of bindings1 list */
static Cell* append_bindings(Cell* bindings1, Cell* bindings2) {
    if (!bindings1) return bindings2;
    if (!bindings2) return bindings1;

    /* Find the last cons cell in bindings1 */
    Cell* current = bindings1;
    cell_retain(bindings1);

    while (current) {
        Cell* next = cell_cdr(current);
        if (!next || next->type == CELL_ATOM_NIL) {
            /* Found the end - append bindings2 directly (no extra wrapping!) */
            cell_retain(bindings2);  /* Retain because we're adding to structure */
            cell_release(current->data.pair.cdr);
            current->data.pair.cdr = bindings2;
            break;
        }
        if (next->type != CELL_PAIR) {
            break;
        }
        current = next;
    }

    return bindings1;
}

/* Helper: Merge two binding lists
 * Handles:
 * - Single + Single: ((a . 1) . ((b . 2) . nil))
 * - List + Single: ((a . 1) . ((b . 2) . ((c . 3) . nil)))
 * - Single + List: ((a . 1) . ((b . 2) . ((c . 3) . nil)))
 * - List + List: ((a . 1) . ((b . 2) . ((c . 3) . ((d . 4) . nil))))
 */
static Cell* merge_bindings(Cell* bindings1, Cell* bindings2) {
    if (!bindings1) return bindings2;
    if (!bindings2) return bindings1;

    /* Check if bindings are single or lists */
    bool b1_single = is_single_binding(bindings1);
    bool b2_single = is_single_binding(bindings2);

    if (b1_single && b2_single) {
        /* Both single: (b1 . (b2 . nil)) */
        Cell* tail = cell_cons(bindings2, cell_nil());
        Cell* result = cell_cons(bindings1, tail);
        cell_release(tail);
        return result;
    } else if (b1_single && !b2_single) {
        /* b1 single, b2 list: (b1 . b2_list) */
        Cell* result = cell_cons(bindings1, bindings2);
        return result;
    } else if (!b1_single && b2_single) {
        /* b1 list, b2 single: wrap b2 in list then append */
        Cell* b2_wrapped = cell_cons(bindings2, cell_nil());
        Cell* result = append_bindings(bindings1, b2_wrapped);
        cell_release(b2_wrapped);
        return result;
    } else {
        /* Both lists: append b2 list to end of b1 list */
        return append_bindings(bindings1, bindings2);
    }
}

/* Helper: Extend environment with bindings list
 * Bindings can be:
 * - Single: (var . value)
 * - List: ((var1 . val1) . ((var2 . val2) . nil))
 * Returns new environment with bindings prepended
 */
static Cell* extend_env_with_bindings(Cell* bindings, Cell* env) {
    if (!bindings) {
        return env;
    }

    /* Check if bindings is a single binding or a list */
    Cell* first = cell_car(bindings);

    /* If car is a symbol, it's a single binding: (symbol . value) */
    if (first && first->type == CELL_ATOM_SYMBOL) {
        /* Single binding - prepend to env */
        cell_retain(bindings);
        cell_retain(env);
        return cell_cons(bindings, env);
    }

    /* List of bindings - walk through and prepend each */
    Cell* new_env = env;
    cell_retain(env);

    /* Build list of bindings to prepend */
    Cell* current = bindings;
    while (current && current->type == CELL_PAIR) {
        Cell* binding = cell_car(current);
        if (binding && binding->type == CELL_PAIR) {
            cell_retain(binding);
            Cell* old_new_env = new_env;
            new_env = cell_cons(binding, old_new_env);
            cell_release(old_new_env);
        }
        current = cell_cdr(current);
    }

    return new_env;
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

    /* Pair pattern - (⟨⟩ pat1 pat2) (Day 17) */
    if (is_pair_pattern(pattern)) {
        /* Value must be a pair */
        if (!value || value->type != CELL_PAIR) {
            return failure;
        }

        /* Extract sub-patterns */
        Cell* pat1;
        Cell* pat2;
        extract_pair_subpatterns(pattern, &pat1, &pat2);

        /* Match car of value against pat1 */
        Cell* value_car = cell_car(value);
        MatchResult match1 = pattern_try_match(value_car, pat1);
        if (!match1.success) {
            return failure;
        }

        /* Match cdr of value against pat2 */
        Cell* value_cdr = cell_cdr(value);
        MatchResult match2 = pattern_try_match(value_cdr, pat2);
        if (!match2.success) {
            /* First match succeeded but second failed - clean up */
            if (match1.bindings) {
                cell_release(match1.bindings);
            }
            return failure;
        }

        /* Merge bindings from both matches */
        Cell* merged = merge_bindings(match1.bindings, match2.bindings);
        MatchResult result = {.success = true, .bindings = merged};
        return result;
    }

    /* Leaf structure pattern - (⊙ :type field-patterns...) (Day 18) */
    if (pattern && pattern->type == CELL_PAIR) {
        Cell* first = cell_car(pattern);
        if (first && first->type == CELL_ATOM_SYMBOL &&
            strcmp(first->data.atom.symbol, "⊙") == 0) {
            /* Value must be a structure */
            if (!value || value->type != CELL_STRUCT) {
                return failure;
            }
            /* Must be a leaf structure */
            if (cell_struct_kind(value) != STRUCT_LEAF) {
                return failure;
            }

            /* Extract pattern type tag */
            Cell* rest = cell_cdr(pattern);
            if (!rest || rest->type != CELL_PAIR) {
                return failure;
            }
            Cell* pattern_type = cell_car(rest);
            if (!pattern_type || pattern_type->type != CELL_ATOM_SYMBOL) {
                return failure;
            }

            /* Match type tag */
            Cell* value_type = cell_struct_type_tag(value);
            if (!symbols_equal(value_type, pattern_type)) {
                return failure;
            }

            /* Get field patterns */
            Cell* field_patterns = cell_cdr(rest);

            /* Get value fields */
            Cell* value_fields = cell_struct_fields(value);

            /* Match each field pattern against corresponding field value */
            Cell* bindings = NULL;
            Cell* current_pattern = field_patterns;
            Cell* current_field = value_fields;

            while (current_pattern && current_pattern->type == CELL_PAIR) {
                /* Check we have a corresponding field */
                if (!current_field || current_field->type != CELL_PAIR) {
                    /* Pattern has more fields than value - mismatch */
                    if (bindings) cell_release(bindings);
                    return failure;
                }

                Cell* field_pat = cell_car(current_pattern);
                Cell* field_binding = cell_car(current_field);  /* (name . value) */
                Cell* field_value = cell_cdr(field_binding);

                /* Recursively match field value against field pattern */
                MatchResult field_match = pattern_try_match(field_value, field_pat);
                if (!field_match.success) {
                    if (bindings) cell_release(bindings);
                    return failure;
                }

                /* Merge field bindings */
                if (field_match.bindings) {
                    bindings = merge_bindings(bindings, field_match.bindings);
                }

                current_pattern = cell_cdr(current_pattern);
                current_field = cell_cdr(current_field);
            }

            /* Check if value has more fields than pattern */
            if (current_field && current_field->type == CELL_PAIR) {
                /* Value has more fields than pattern - mismatch */
                if (bindings) cell_release(bindings);
                return failure;
            }

            MatchResult result = {.success = true, .bindings = bindings};
            return result;
        }
    }

    /* Node/ADT pattern - (⊚ :type :variant field-patterns...) (Day 18) */
    if (pattern && pattern->type == CELL_PAIR) {
        Cell* first = cell_car(pattern);
        if (first && first->type == CELL_ATOM_SYMBOL &&
            strcmp(first->data.atom.symbol, "⊚") == 0) {
            /* Value must be a structure */
            if (!value || value->type != CELL_STRUCT) {
                return failure;
            }
            /* Must be a node structure */
            if (cell_struct_kind(value) != STRUCT_NODE) {
                return failure;
            }

            /* Extract pattern type tag */
            Cell* rest = cell_cdr(pattern);
            if (!rest || rest->type != CELL_PAIR) {
                return failure;
            }
            Cell* pattern_type = cell_car(rest);
            if (!pattern_type || pattern_type->type != CELL_ATOM_SYMBOL) {
                return failure;
            }

            /* Match type tag */
            Cell* value_type = cell_struct_type_tag(value);
            if (!symbols_equal(value_type, pattern_type)) {
                return failure;
            }

            /* Extract pattern variant */
            Cell* rest2 = cell_cdr(rest);
            if (!rest2 || rest2->type != CELL_PAIR) {
                return failure;
            }
            Cell* pattern_variant = cell_car(rest2);
            if (!pattern_variant || pattern_variant->type != CELL_ATOM_SYMBOL) {
                return failure;
            }

            /* Match variant */
            Cell* value_variant = cell_struct_variant(value);
            if (!symbols_equal(value_variant, pattern_variant)) {
                return failure;
            }

            /* Get field patterns */
            Cell* field_patterns = cell_cdr(rest2);

            /* Get value fields */
            Cell* value_fields = cell_struct_fields(value);

            /* Match each field pattern against corresponding field value */
            Cell* bindings = NULL;
            Cell* current_pattern = field_patterns;
            Cell* current_field = value_fields;

            while (current_pattern && current_pattern->type == CELL_PAIR) {
                /* Check we have a corresponding field */
                if (!current_field || current_field->type != CELL_PAIR) {
                    /* Pattern has more fields than value - mismatch */
                    if (bindings) cell_release(bindings);
                    return failure;
                }

                Cell* field_pat = cell_car(current_pattern);
                Cell* field_binding = cell_car(current_field);  /* (name . value) */
                Cell* field_value = cell_cdr(field_binding);

                /* Recursively match field value against field pattern */
                MatchResult field_match = pattern_try_match(field_value, field_pat);
                if (!field_match.success) {
                    if (bindings) cell_release(bindings);
                    return failure;
                }

                /* Merge field bindings */
                if (field_match.bindings) {
                    bindings = merge_bindings(bindings, field_match.bindings);
                }

                current_pattern = cell_cdr(current_pattern);
                current_field = cell_cdr(current_field);
            }

            /* Check if value has more fields than pattern */
            if (current_field && current_field->type == CELL_PAIR) {
                /* Value has more fields than pattern - mismatch */
                if (bindings) cell_release(bindings);
                return failure;
            }

            MatchResult result = {.success = true, .bindings = bindings};
            return result;
        }
    }

    /* Pattern type not supported yet */
    return failure;
}

/* Evaluate a match expression */
Cell* pattern_eval_match(Cell* expr, Cell* clauses, Cell* env, EvalContext* ctx) {
    if (!ctx) {
        return cell_error("no-context", expr);
    }

    /* Evaluate the expression to match using the provided environment
     * This is critical for De Bruijn indices in closures - we need the
     * current indexed environment, not the global environment */
    Cell* value = eval_internal(ctx, env, expr);
    if (!value) {
        return cell_error("eval-failed", expr);
    }
    if (value->type == CELL_ERROR) {
        return value;
    }

    /* Check exhaustiveness and emit warnings */
    ExhaustivenessResult check = pattern_check_exhaustiveness(clauses);

    if (check.status == COVERAGE_PARTIAL) {
        /* Incomplete pattern match - warn about potential :no-match error */
        warn_incomplete_match(value);
    } else if (check.status == COVERAGE_REDUNDANT) {
        /* Unreachable patterns detected */
        warn_unreachable_pattern(check.first_unreachable);
    }
    /* COVERAGE_COMPLETE - all good, no warnings */

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

        /* Extract pattern (possibly with guard) and result */
        Cell* pattern_expr = clause->data.pair.car;
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

        /* Check if pattern has guard syntax: (pattern | guard) */
        Cell* pattern;
        Cell* guard_expr = NULL;
        if (has_guard(pattern_expr)) {
            extract_pattern_and_guard(pattern_expr, &pattern, &guard_expr);
        } else {
            pattern = pattern_expr;
        }

        /* Try to match the pattern against the value */
        MatchResult match = pattern_try_match(value, pattern);

        if (match.success) {
            /* If guard exists, evaluate it in extended environment */
            if (guard_expr) {
                Cell* extended_env = extend_env_with_bindings(match.bindings, env);

                /* Temporarily set ctx->env for symbol lookup */
                Cell* old_ctx_env = ctx->env;
                cell_retain(extended_env);
                ctx->env = extended_env;

                /* Evaluate guard */
                Cell* guard_result = eval(ctx, guard_expr);

                /* Restore ctx->env */
                ctx->env = old_ctx_env;
                cell_release(extended_env);

                /* Check if guard succeeded */
                bool guard_passed = false;
                if (guard_result && guard_result->type == CELL_ATOM_BOOL) {
                    guard_passed = guard_result->data.atom.boolean;
                }
                cell_release(guard_result);

                if (!guard_passed) {
                    /* Guard failed - try next clause */
                    if (match.bindings) {
                        cell_release(match.bindings);
                    }
                    current = current->data.pair.cdr;
                    continue;
                }
            }

            /* Pattern matched and guard passed (or no guard) - evaluate result */
            /* Match succeeded! Evaluate the result expression */
            Cell* result;

            if (match.bindings) {
                /* Extend the LOCAL environment with pattern bindings
                 * This creates: ((v . value) . [closure_params...]) */
                Cell* extended_env = extend_env_with_bindings(match.bindings, env);

                /* Temporarily set ctx->env to the extended environment
                 * This allows symbol lookup to find both pattern bindings (v) and closure params (converted to De Bruijn) */
                Cell* old_ctx_env = ctx->env;
                cell_retain(extended_env);
                ctx->env = extended_env;

                /* Evaluate result - symbol lookup uses ctx->env, De Bruijn uses closure environment */
                result = eval(ctx, result_expr);

                /* Restore original ctx->env */
                ctx->env = old_ctx_env;
                cell_release(extended_env);
                cell_release(match.bindings);
            } else {
                /* No bindings, but still need to temporarily set ctx->env for closure parameter access */
                Cell* old_ctx_env = ctx->env;
                cell_retain(env);
                ctx->env = env;

                result = eval(ctx, result_expr);

                ctx->env = old_ctx_env;
                cell_release(env);
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
