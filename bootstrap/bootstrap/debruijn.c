#include "debruijn.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Count list length */
static int list_length(Cell* list) {
    int count = 0;
    while (cell_is_pair(list)) {
        count++;
        list = cell_cdr(list);
    }
    return count;
}

/* Create new context with parameter names */
NameContext* context_new(const char** names, int count, NameContext* parent) {
    NameContext* ctx = (NameContext*)malloc(sizeof(NameContext));
    ctx->names = names;
    ctx->count = count;
    ctx->parent = parent;
    return ctx;
}

/* Free context */
void context_free(NameContext* ctx) {
    if (ctx == NULL) return;
    free(ctx);
}

/* Lookup name in context, return De Bruijn index or -1 if not found */
int debruijn_lookup(const char* name, NameContext* ctx) {
    int depth = 0;
    NameContext* current = ctx;

    while (current != NULL) {
        /* Search in current level */
        for (int i = 0; i < current->count; i++) {
            if (strcmp(name, current->names[i]) == 0) {
                return depth + i;
            }
        }
        /* Move to outer scope */
        depth += current->count;
        current = current->parent;
    }

    /* Not found - free variable */
    return -1;
}

/* Convert list of expressions */
Cell* debruijn_convert_list(Cell* list, NameContext* ctx) {
    if (cell_is_nil(list)) {
        return cell_nil();
    }

    if (!cell_is_pair(list)) {
        return debruijn_convert(list, ctx);
    }

    Cell* first = cell_car(list);
    Cell* rest = cell_cdr(list);

    Cell* converted_first = debruijn_convert(first, ctx);
    Cell* converted_rest = debruijn_convert_list(rest, ctx);

    Cell* result = cell_cons(converted_first, converted_rest);

    cell_release(converted_first);
    cell_release(converted_rest);

    return result;
}

/* Convert expression from named to De Bruijn indices */
Cell* debruijn_convert(Cell* expr, NameContext* ctx) {
    /* Self-evaluating literals */
    if (cell_is_number(expr) || cell_is_bool(expr) || cell_is_nil(expr)) {
        cell_retain(expr);
        return expr;
    }

    /* Symbol - convert to index if bound, keep as symbol if free */
    if (cell_is_symbol(expr)) {
        const char* name = cell_get_symbol(expr);
        int index = debruijn_lookup(name, ctx);

        if (index >= 0) {
            /* Bound variable - convert to De Bruijn index */
            return cell_number((double)index);
        } else {
            /* Free variable (primitive or global) - keep as symbol */
            cell_retain(expr);
            return expr;
        }
    }

    /* Pair (function application or special form) */
    if (cell_is_pair(expr)) {
        Cell* first = cell_car(expr);
        Cell* rest = cell_cdr(expr);

        /* Check for lambda - convert with extended context */
        if (cell_is_symbol(first)) {
            const char* sym = cell_get_symbol(first);

            if (strcmp(sym, "λ") == 0) {
                /* Nested lambda: (λ (params...) body) */
                Cell* params = cell_car(rest);
                Cell* body_expr = cell_car(cell_cdr(rest));

                /* Count parameters */
                int param_count = list_length(params);

                /* Extract parameter names */
                const char** param_names = (const char**)malloc(param_count * sizeof(char*));
                Cell* param_iter = params;
                for (int i = 0; i < param_count; i++) {
                    Cell* param = cell_car(param_iter);
                    param_names[i] = cell_get_symbol(param);
                    param_iter = cell_cdr(param_iter);
                }

                /* Create new context (extends current ctx) */
                NameContext* new_ctx = context_new(param_names, param_count, ctx);

                /* Convert body with extended context */
                Cell* converted_body = debruijn_convert(body_expr, new_ctx);

                /* Keep original lambda structure: (λ (params) converted_body) */
                /* This way it can be re-evaluated properly */
                Cell* lambda_sym = cell_symbol("λ");
                Cell* result = cell_cons(lambda_sym,
                                        cell_cons(params,
                                                 cell_cons(converted_body, cell_nil())));

                /* Cleanup */
                context_free(new_ctx);
                free(param_names);
                cell_release(lambda_sym);
                cell_release(converted_body);

                return result;
            }
        }

        /* Regular application - convert each subexpression */
        Cell* converted_first = debruijn_convert(first, ctx);
        Cell* converted_rest = debruijn_convert_list(rest, ctx);
        Cell* result = cell_cons(converted_first, converted_rest);

        cell_release(converted_first);
        cell_release(converted_rest);

        return result;
    }

    /* Unknown type - return as-is */
    fprintf(stderr, "Warning: Unknown expression type in debruijn_convert\n");
    cell_retain(expr);
    return expr;
}
