#include "pattern_check.h"
#include "cell.h"
#include <stdio.h>
#include <string.h>

/* Helper: Check if pattern is wildcard (_) */
static bool is_wildcard_check(Cell* pattern) {
    if (!pattern || pattern->type != CELL_ATOM_SYMBOL) {
        return false;
    }
    return strcmp(pattern->data.atom.symbol, "_") == 0;
}

/* Helper: Check if symbol is a keyword (starts with :) */
static bool is_keyword_check(const char* sym) {
    return sym && sym[0] == ':';
}

/* Helper: Check if pattern is a variable (non-keyword symbol, not wildcard) */
static bool is_variable_check(Cell* pattern) {
    if (!pattern || pattern->type != CELL_ATOM_SYMBOL) {
        return false;
    }
    const char* sym = pattern->data.atom.symbol;
    return !is_keyword_check(sym) && strcmp(sym, "_") != 0;
}

/* Helper: Check if pattern is a catch-all (wildcard or variable) */
static bool is_catch_all(Cell* pattern) {
    return is_wildcard_check(pattern) || is_variable_check(pattern);
}

/* Check if patterns are exhaustive */
ExhaustivenessResult pattern_check_exhaustiveness(Cell* clauses) {
    ExhaustivenessResult result = {
        .status = COVERAGE_PARTIAL,
        .first_unreachable = -1,
        .has_catch_all = false
    };

    if (!clauses) {
        return result;
    }

    /* Walk through clauses looking for catch-all and checking reachability */
    Cell* current = clauses;
    int index = 0;
    bool seen_catch_all = false;

    while (current && cell_is_pair(current)) {
        Cell* clause = cell_car(current);

        /* Extract pattern from clause */
        if (!clause || clause->type != CELL_PAIR) {
            /* Invalid clause, skip */
            current = cell_cdr(current);
            index++;
            continue;
        }

        Cell* pattern = cell_car(clause);

        /* Check if this pattern is catch-all */
        if (is_catch_all(pattern)) {
            if (seen_catch_all) {
                /* Multiple catch-alls - first one already handled everything */
                if (result.first_unreachable == -1) {
                    result.first_unreachable = index;
                }
            } else {
                seen_catch_all = true;
                result.has_catch_all = true;
            }
        } else if (seen_catch_all) {
            /* Pattern after catch-all is unreachable */
            if (result.first_unreachable == -1) {
                result.first_unreachable = index;
            }
        }

        current = cell_cdr(current);
        index++;
    }

    /* Determine overall status */
    if (result.first_unreachable >= 0) {
        result.status = COVERAGE_REDUNDANT;
    } else if (result.has_catch_all) {
        result.status = COVERAGE_COMPLETE;
    } else {
        result.status = COVERAGE_PARTIAL;
    }

    return result;
}

/* Emit warning about incomplete pattern match */
void warn_incomplete_match(Cell* value) {
    fprintf(stderr, "\nerror️  Warning: Pattern match may be incomplete\n");
    fprintf(stderr, "   -> Matching value of type: ");

    if (!value) {
        fprintf(stderr, "nil\n");
    } else {
        switch (value->type) {
            case CELL_ATOM_NUMBER:
                fprintf(stderr, "number (infinite domain)\n");
                break;
            case CELL_ATOM_BOOL:
                fprintf(stderr, "boolean (check both #t and #f)\n");
                break;
            case CELL_ATOM_SYMBOL:
                fprintf(stderr, "symbol (infinite domain)\n");
                break;
            case CELL_ATOM_NIL:
                fprintf(stderr, "nil\n");
                break;
            case CELL_PAIR:
                fprintf(stderr, "pair\n");
                break;
            case CELL_STRUCT:
                if (cell_struct_kind(value) == STRUCT_LEAF) {
                    Cell* type_tag = cell_struct_type_tag(value);
                    fprintf(stderr, "structure %s\n",
                            type_tag ? type_tag->data.atom.symbol : "unknown");
                } else if (cell_struct_kind(value) == STRUCT_NODE) {
                    Cell* type_tag = cell_struct_type_tag(value);
                    fprintf(stderr, "ADT %s (check all variants)\n",
                            type_tag ? type_tag->data.atom.symbol : "unknown");
                }
                break;
            default:
                fprintf(stderr, "unknown\n");
                break;
        }
    }
    fprintf(stderr, "   -> Consider adding a catch-all pattern: _ or variable\n");
    fprintf(stderr, "   -> Missing cases will cause runtime :no-match error\n\n");
}

/* Emit warning about unreachable pattern */
void warn_unreachable_pattern(int clause_index) {
    fprintf(stderr, "\nerror️  Warning: Unreachable pattern detected\n");
    fprintf(stderr, "   -> Pattern at position %d (1-based: %d)\n",
            clause_index, clause_index + 1);
    fprintf(stderr, "   -> Previous pattern(s) already handle all cases\n");
    fprintf(stderr, "   -> This pattern will never match\n");
    fprintf(stderr, "   -> Consider removing it or reordering patterns\n\n");
}
