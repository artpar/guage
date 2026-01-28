#include "pattern.h"
#include "pattern_check.h"
#include "cell.h"
#include "eval.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Forward declarations for mutually recursive helpers */
static bool is_pair_pattern(Cell* pattern);

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

/* Helper: Check if pattern is an as-pattern: name@subpattern
 * As-pattern syntax: (name @ subpattern) - a list with 3 elements
 * Where:
 * - name is a variable (non-keyword symbol, not wildcard)
 * - @ is the separator symbol
 * - subpattern is any valid pattern
 */
static bool is_as_pattern(Cell* pattern) {
    /* Must be a list with at least 3 elements */
    if (!pattern || pattern->type != CELL_PAIR) {
        return false;
    }

    /* First element should be a variable (non-keyword symbol, not wildcard) */
    Cell* first = cell_car(pattern);
    if (!is_variable_pattern(first)) {
        return false;
    }

    /* Need at least 2 more elements */
    Cell* rest = cell_cdr(pattern);
    if (!rest || rest->type != CELL_PAIR) {
        return false;
    }

    /* Second element should be the @ symbol */
    Cell* second = cell_car(rest);
    if (!second || second->type != CELL_ATOM_SYMBOL) {
        return false;
    }
    if (strcmp(second->data.atom.symbol, "@") != 0) {
        return false;
    }

    /* Need the subpattern */
    Cell* rest2 = cell_cdr(rest);
    if (!rest2 || rest2->type != CELL_PAIR) {
        return false;
    }

    return true;
}

/* Helper: Extract name and subpattern from as-pattern syntax
 * Input: (name @ subpattern)
 * Output: name_out = name symbol, subpattern_out = subpattern
 */
static void extract_as_pattern(Cell* pattern, Cell** name_out, Cell** subpattern_out) {
    /* As-pattern syntax: (name @ subpattern) */
    *name_out = cell_car(pattern);

    /* Skip the @ symbol */
    Cell* rest = cell_cdr(pattern);
    Cell* rest2 = cell_cdr(rest);
    *subpattern_out = cell_car(rest2);
}

/* Helper: Check if pattern is an or-pattern: (∨ pattern₁ pattern₂ ...)
 * Or-pattern syntax: (∨ pat1 pat2 ...) - list with ∨ as first element
 * Must have at least 2 alternatives (3+ total elements)
 */
static bool is_or_pattern(Cell* pattern) {
    /* Must be a list */
    if (!pattern || pattern->type != CELL_PAIR) {
        return false;
    }

    /* First element should be the logical-or symbol "∨" */
    Cell* first = cell_car(pattern);
    if (!first || first->type != CELL_ATOM_SYMBOL) {
        return false;
    }
    if (strcmp(first->data.atom.symbol, "∨") != 0) {
        return false;
    }

    /* Need at least 2 alternatives */
    Cell* rest = cell_cdr(pattern);
    if (!rest || rest->type != CELL_PAIR) {
        return false;
    }

    /* Second alternative */
    Cell* rest2 = cell_cdr(rest);
    if (!rest2 || rest2->type != CELL_PAIR) {
        return false;
    }

    return true;
}

/* Helper: Extract alternatives from or-pattern
 * Input: (∨ pat1 pat2 pat3 ...)
 * Output: list of alternative patterns (pat1 pat2 pat3 ...)
 */
static Cell* extract_or_alternatives(Cell* pattern) {
    /* Or-pattern syntax: (∨ pat1 pat2 ...) */
    /* Return the rest after ∨ symbol */
    return cell_cdr(pattern);
}

/* Helper: Extract all variable bindings from a pattern (for consistency checking)
 * Returns a list of variable names (symbols) that this pattern would bind
 * Used to ensure or-pattern alternatives bind the same variables
 */
static Cell* extract_pattern_variables(Cell* pattern) {
    if (!pattern) {
        return cell_nil();
    }

    /* Wildcard _ binds nothing */
    if (is_wildcard(pattern)) {
        return cell_nil();
    }

    /* Variable pattern binds itself */
    if (is_variable_pattern(pattern)) {
        cell_retain(pattern);
        return cell_cons(pattern, cell_nil());
    }

    /* Literal patterns bind nothing */
    if (pattern->type == CELL_ATOM_NUMBER ||
        pattern->type == CELL_ATOM_BOOL ||
        pattern->type == CELL_ATOM_NIL ||
        (pattern->type == CELL_ATOM_SYMBOL && is_keyword(pattern->data.atom.symbol))) {
        return cell_nil();
    }

    /* As-pattern (name @ subpattern) binds name + subpattern variables */
    if (is_as_pattern(pattern)) {
        Cell* name;
        Cell* subpattern;
        extract_as_pattern(pattern, &name, &subpattern);

        Cell* subvars = extract_pattern_variables(subpattern);
        cell_retain(name);
        Cell* result = cell_cons(name, subvars);
        cell_release(subvars);
        return result;
    }

    /* Or-pattern (∨ pat1 pat2 ...) - check first alternative
     * (All alternatives must bind same variables, verified separately) */
    if (is_or_pattern(pattern)) {
        Cell* alternatives = extract_or_alternatives(pattern);
        if (alternatives && alternatives->type == CELL_PAIR) {
            return extract_pattern_variables(cell_car(alternatives));
        }
        return cell_nil();
    }

    /* Pair pattern (⟨⟩ pat1 pat2) recursively extracts from both */
    if (is_pair_pattern(pattern)) {
        Cell* rest = cell_cdr(pattern);  /* Skip ⟨⟩ symbol */
        Cell* pat1 = cell_car(rest);
        Cell* pat2 = cell_car(cell_cdr(rest));

        Cell* vars1 = extract_pattern_variables(pat1);
        Cell* vars2 = extract_pattern_variables(pat2);

        /* Merge the variable lists */
        Cell* merged = cell_nil();
        Cell* current = vars1;
        while (current && current->type == CELL_PAIR) {
            Cell* var = cell_car(current);
            cell_retain(var);
            Cell* new_cons = cell_cons(var, merged);
            cell_release(merged);
            merged = new_cons;
            current = cell_cdr(current);
        }

        current = vars2;
        while (current && current->type == CELL_PAIR) {
            Cell* var = cell_car(current);
            cell_retain(var);
            Cell* new_cons = cell_cons(var, merged);
            cell_release(merged);
            merged = new_cons;
            current = cell_cdr(current);
        }

        cell_release(vars1);
        cell_release(vars2);
        return merged;
    }

    /* Structure patterns (⊙ Type ...) and ADT patterns (⊚ Type Variant ...)
     * Extract variables from field patterns */
    if (pattern->type == CELL_PAIR) {
        Cell* first = cell_car(pattern);
        if (first && first->type == CELL_ATOM_SYMBOL) {
            const char* sym = first->data.atom.symbol;
            if (strcmp(sym, "⊙") == 0 || strcmp(sym, "⊚") == 0) {
                /* Skip structure symbol and type/variant, extract from fields */
                Cell* rest = cell_cdr(pattern);
                if (rest && rest->type == CELL_PAIR) {
                    rest = cell_cdr(rest);  /* Skip type */
                    if (strcmp(sym, "⊚") == 0 && rest && rest->type == CELL_PAIR) {
                        rest = cell_cdr(rest);  /* Skip variant for ADT */
                    }

                    /* Extract variables from all fields */
                    Cell* vars = cell_nil();
                    while (rest && rest->type == CELL_PAIR) {
                        Cell* field_pat = cell_car(rest);
                        Cell* field_vars = extract_pattern_variables(field_pat);

                        /* Merge field_vars into vars */
                        Cell* current = field_vars;
                        while (current && current->type == CELL_PAIR) {
                            Cell* var = cell_car(current);
                            cell_retain(var);
                            Cell* new_cons = cell_cons(var, vars);
                            cell_release(vars);
                            vars = new_cons;
                            current = cell_cdr(current);
                        }
                        cell_release(field_vars);

                        rest = cell_cdr(rest);
                    }
                    return vars;
                }
            }
        }
    }

    /* Unknown pattern type - assume no variables */
    return cell_nil();
}

/* Helper: Check if two variable lists are equivalent (same variables, order doesn't matter) */
static bool variable_lists_equal(Cell* vars1, Cell* vars2) {
    /* Count variables in each list */
    int count1 = 0, count2 = 0;
    Cell* c1 = vars1;
    while (c1 && c1->type == CELL_PAIR) {
        count1++;
        c1 = cell_cdr(c1);
    }
    c1 = vars2;
    while (c1 && c1->type == CELL_PAIR) {
        count2++;
        c1 = cell_cdr(c1);
    }

    if (count1 != count2) {
        return false;
    }

    /* Check that every variable in vars1 exists in vars2 */
    Cell* current1 = vars1;
    while (current1 && current1->type == CELL_PAIR) {
        Cell* var1 = cell_car(current1);
        bool found = false;

        Cell* current2 = vars2;
        while (current2 && current2->type == CELL_PAIR) {
            Cell* var2 = cell_car(current2);
            if (symbols_equal(var1, var2)) {
                found = true;
                break;
            }
            current2 = cell_cdr(current2);
        }

        if (!found) {
            return false;
        }
        current1 = cell_cdr(current1);
    }

    return true;
}

/* Helper: Check variable consistency across or-pattern alternatives
 * All alternatives must bind the same set of variables
 * Returns true if consistent, false otherwise
 */
static bool check_or_pattern_consistency(Cell* alternatives) {
    if (!alternatives || alternatives->type != CELL_PAIR) {
        return true;  /* Empty or invalid - no inconsistency */
    }

    /* Extract variables from first alternative (reference) */
    Cell* first_alt = cell_car(alternatives);
    Cell* ref_vars = extract_pattern_variables(first_alt);

    /* Check all remaining alternatives */
    Cell* current = cell_cdr(alternatives);
    bool consistent = true;
    while (current && current->type == CELL_PAIR && consistent) {
        Cell* alt = cell_car(current);
        Cell* alt_vars = extract_pattern_variables(alt);

        if (!variable_lists_equal(ref_vars, alt_vars)) {
            consistent = false;
        }

        cell_release(alt_vars);
        current = cell_cdr(current);
    }

    cell_release(ref_vars);
    return consistent;
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

    /* As-pattern: name@subpattern (Day 59)
     * Binds both the whole value (to name) AND the pattern parts (via subpattern)
     * Example: pair@(⟨⟩ a b) binds pair → whole value, a → car, b → cdr
     */
    if (is_as_pattern(pattern)) {
        /* Extract name and subpattern */
        Cell* name_symbol;
        Cell* subpattern;
        extract_as_pattern(pattern, &name_symbol, &subpattern);

        /* Try to match subpattern against value */
        MatchResult submatch = pattern_try_match(value, subpattern);
        if (!submatch.success) {
            /* Subpattern didn't match */
            return failure;
        }

        /* Create binding for the whole value: name → value */
        cell_retain(value);  /* Retain value for binding */
        Cell* whole_binding = cell_cons(name_symbol, value);

        /* Merge whole_binding with subpattern bindings */
        Cell* merged = merge_bindings(whole_binding, submatch.bindings);
        MatchResult result = {.success = true, .bindings = merged};
        return result;
    }

    /* Or-pattern: (∨ pattern₁ pattern₂ ...) (Day 60)
     * Try each alternative in order, first match wins
     * Example: (∨ #0 #1 #2) matches 0, 1, or 2
     */
    if (is_or_pattern(pattern)) {
        /* Extract alternatives */
        Cell* alternatives = extract_or_alternatives(pattern);

        /* Check variable consistency across alternatives */
        if (!check_or_pattern_consistency(alternatives)) {
            /* Inconsistent variable bindings - this is a pattern error
             * Return failure (ideally should be a compile-time error) */
            return failure;
        }

        /* Try each alternative in order */
        Cell* current = alternatives;
        while (current && current->type == CELL_PAIR) {
            Cell* alt_pattern = cell_car(current);

            /* Try to match this alternative */
            MatchResult alt_match = pattern_try_match(value, alt_pattern);

            if (alt_match.success) {
                /* This alternative matched - return its bindings */
                return alt_match;
            }

            /* Try next alternative */
            current = cell_cdr(current);
        }

        /* No alternative matched */
        return failure;
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
