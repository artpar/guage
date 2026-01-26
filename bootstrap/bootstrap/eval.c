#include "eval.h"
#include "primitives.h"
#include "debruijn.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Create new evaluation context */
EvalContext* eval_context_new(void) {
    EvalContext* ctx = (EvalContext*)malloc(sizeof(EvalContext));
    ctx->env = cell_nil();
    ctx->primitives = primitives_init();
    return ctx;
}

/* Free evaluation context */
void eval_context_free(EvalContext* ctx) {
    cell_release(ctx->env);
    cell_release(ctx->primitives);
    free(ctx);
}

/* Lookup variable in environment */
Cell* eval_lookup_env(Cell* env, const char* name) {
    while (cell_is_pair(env)) {
        Cell* binding = cell_car(env);
        if (cell_is_pair(binding)) {
            Cell* var = cell_car(binding);
            if (cell_is_symbol(var)) {
                const char* var_name = cell_get_symbol(var);
                if (strcmp(var_name, name) == 0) {
                    Cell* value = cell_cdr(binding);
                    cell_retain(value);
                    return value;
                }
            }
        }
        env = cell_cdr(env);
    }
    return NULL;
}

/* Lookup variable */
Cell* eval_lookup(EvalContext* ctx, const char* name) {
    /* Try user environment first */
    Cell* value = eval_lookup_env(ctx->env, name);
    if (value) return value;

    /* Try primitives */
    return primitives_lookup(ctx->primitives, name);
}

/* Define global binding */
void eval_define(EvalContext* ctx, const char* name, Cell* value) {
    Cell* sym = cell_symbol(name);
    Cell* binding = cell_cons(sym, value);
    ctx->env = cell_cons(binding, ctx->env);
}

/* Lookup De Bruijn index in environment (for indexed environments) */
Cell* env_lookup_index(Cell* env, int index) {
    Cell* current = env;
    for (int i = 0; i < index; i++) {
        if (!cell_is_pair(current)) {
            return NULL;
        }
        current = cell_cdr(current);
    }
    if (cell_is_pair(current)) {
        Cell* value = cell_car(current);
        cell_retain(value);
        return value;
    }
    return NULL;
}

/* Extend environment with argument values (prepend args to env) */
Cell* extend_env(Cell* env, Cell* args) {
    if (cell_is_nil(args)) {
        cell_retain(env);
        return env;
    }

    Cell* first = cell_car(args);
    Cell* rest = cell_cdr(args);
    Cell* extended_rest = extend_env(env, rest);
    Cell* result = cell_cons(first, extended_rest);

    cell_release(extended_rest);
    return result;
}

/* Count list length */
int list_length(Cell* list) {
    int count = 0;
    while (cell_is_pair(list)) {
        count++;
        list = cell_cdr(list);
    }
    return count;
}

/* Extract parameter names from list */
const char** extract_param_names(Cell* params) {
    int count = list_length(params);
    const char** names = (const char**)malloc(count * sizeof(char*));

    Cell* current = params;
    for (int i = 0; i < count; i++) {
        Cell* param = cell_car(current);
        names[i] = cell_get_symbol(param);
        current = cell_cdr(current);
    }

    return names;
}

/* Forward declaration */
static Cell* eval_internal(EvalContext* ctx, Cell* env, Cell* expr);

/* Evaluate list (for function application) */
static Cell* eval_list(EvalContext* ctx, Cell* env, Cell* expr) {
    if (cell_is_nil(expr)) {
        return cell_nil();
    }

    if (!cell_is_pair(expr)) {
        return eval_internal(ctx, env, expr);
    }

    Cell* first = cell_car(expr);
    Cell* rest = cell_cdr(expr);

    Cell* first_eval = eval_internal(ctx, env, first);
    Cell* rest_eval = eval_list(ctx, env, rest);

    Cell* result = cell_cons(first_eval, rest_eval);

    cell_release(first_eval);
    cell_release(rest_eval);

    return result;
}

/* Apply function to arguments */
static Cell* apply(EvalContext* ctx, Cell* fn, Cell* args) {
    if (fn->type == CELL_BUILTIN) {
        /* Call builtin primitive */
        Cell* (*builtin_fn)(Cell*) = (Cell* (*)(Cell*))fn->data.atom.builtin;
        return builtin_fn(args);
    }

    if (fn->type == CELL_LAMBDA) {
        /* Extract lambda components */
        Cell* closure_env = fn->data.lambda.env;
        Cell* body = fn->data.lambda.body;
        int arity = fn->data.lambda.arity;

        /* Check argument count */
        int arg_count = list_length(args);
        if (arg_count != arity) {
            fprintf(stderr, "Error: Arity mismatch: expected %d, got %d\n",
                    arity, arg_count);
            return cell_nil();
        }

        /* Create new environment: prepend args to closure env */
        Cell* new_env = extend_env(closure_env, args);

        /* Evaluate body in new environment */
        Cell* result = eval_internal(ctx, new_env, body);

        /* Cleanup */
        cell_release(new_env);

        return result;
    }

    fprintf(stderr, "Error: Cannot apply non-function\n");
    return cell_nil();
}

/* Check if environment is indexed (not named/assoc) */
static bool env_is_indexed(Cell* env) {
    /* Indexed environment: (val1 val2 val3 ...) */
    /* Named environment: ((sym1 . val1) (sym2 . val2) ...) */
    if (cell_is_nil(env)) return true;  /* Empty env can be either */
    if (!cell_is_pair(env)) return false;

    Cell* first = cell_car(env);
    /* If first element is a pair, it's likely a named binding */
    return !cell_is_pair(first);
}

/* Evaluate expression */
static Cell* eval_internal(EvalContext* ctx, Cell* env, Cell* expr) {
    /* Numbers can be De Bruijn indices OR literals */
    if (cell_is_number(expr)) {
        /* Only try to interpret as De Bruijn index if env is indexed */
        if (env_is_indexed(env)) {
            double num = cell_get_number(expr);
            if (num >= 0 && num == (int)num) {
                /* Try as De Bruijn index */
                Cell* value = env_lookup_index(env, (int)num);
                if (value != NULL) {
                    /* Found indexed binding */
                    return value;
                }
            }
        }
        /* Not an index or named env - treat as number literal */
        cell_retain(expr);
        return expr;
    }

    /* Self-evaluating literals */
    if (cell_is_bool(expr) || cell_is_nil(expr)) {
        cell_retain(expr);
        return expr;
    }

    /* Variable lookup */
    if (cell_is_symbol(expr)) {
        const char* name = cell_get_symbol(expr);
        Cell* value = eval_lookup(ctx, name);
        if (value == NULL) {
            fprintf(stderr, "Error: Undefined variable '%s'\n", name);
            return cell_nil();
        }
        return value;
    }

    /* Special forms and function application */
    if (cell_is_pair(expr)) {
        Cell* first = cell_car(expr);
        Cell* rest = cell_cdr(expr);

        /* Special forms - PURE SYMBOLS ONLY */
        if (cell_is_symbol(first)) {
            const char* sym = cell_get_symbol(first);

            /* ⌜ - quote */
            if (strcmp(sym, "⌜") == 0) {
                Cell* arg = cell_car(rest);
                cell_retain(arg);
                return arg;
            }

            /* ≔ - define */
            if (strcmp(sym, "≔") == 0) {
                Cell* name = cell_car(rest);
                Cell* value_expr = cell_car(cell_cdr(rest));

                assert(cell_is_symbol(name));
                const char* name_str = cell_get_symbol(name);

                /* Check if value_expr is a lambda - enable named recursion */
                bool is_lambda = false;
                if (cell_is_pair(value_expr)) {
                    Cell* first_val = cell_car(value_expr);
                    if (cell_is_symbol(first_val)) {
                        const char* sym_val = cell_get_symbol(first_val);
                        is_lambda = (strcmp(sym_val, "λ") == 0);
                    }
                }

                /* For lambdas, pre-bind name to nil to enable self-reference */
                if (is_lambda) {
                    Cell* placeholder = cell_nil();
                    eval_define(ctx, name_str, placeholder);
                    cell_release(placeholder);
                }

                /* Evaluate the value (lambda will see name in environment) */
                Cell* value = eval_internal(ctx, env, value_expr);

                /* Define/redefine the name with the actual value */
                eval_define(ctx, name_str, value);

                return value;
            }

            /* :λ-converted - already converted nested lambda */
            if (strcmp(sym, ":λ-converted") == 0) {
                /* Parse: (:λ-converted (param1 param2 ...) converted_body) */
                Cell* params = cell_car(rest);
                Cell* converted_body = cell_car(cell_cdr(rest));

                /* Body is already converted, don't convert again */
                int arity = list_length(params);

                /* Closure env: use indexed env if we're in a lambda, empty if top-level */
                Cell* closure_env = env_is_indexed(env) ? env : cell_nil();

                /* Create lambda cell with closure environment */
                Cell* lambda = cell_lambda(closure_env, converted_body, arity);

                return lambda;
            }

            /* λ - lambda */
            if (strcmp(sym, "λ") == 0) {
                /* Parse: (λ (param1 param2 ...) body) */
                Cell* params = cell_car(rest);
                Cell* body_expr = cell_car(cell_cdr(rest));

                /* Count parameters and extract names */
                int arity = list_length(params);
                const char** param_names = extract_param_names(params);

                /* Create conversion context */
                NameContext* ctx_convert = context_new(param_names, arity, NULL);

                /* Convert body to De Bruijn indices */
                Cell* converted_body = debruijn_convert(body_expr, ctx_convert);

                /* Closure env: use indexed env if we're in a lambda, empty if top-level */
                Cell* closure_env = env_is_indexed(env) ? env : cell_nil();

                /* Create lambda cell with closure environment */
                Cell* lambda = cell_lambda(closure_env, converted_body, arity);

                /* Cleanup */
                context_free(ctx_convert);
                free(param_names);
                cell_release(converted_body);

                return lambda;
            }

            /* ? - conditional (no "if") */
            if (strcmp(sym, "?") == 0) {
                Cell* cond_expr = cell_car(rest);
                Cell* then_expr = cell_car(cell_cdr(rest));
                Cell* else_expr = cell_car(cell_cdr(cell_cdr(rest)));

                Cell* cond_val = eval_internal(ctx, env, cond_expr);
                Cell* result;

                if (cell_is_bool(cond_val) && cell_get_bool(cond_val)) {
                    result = eval_internal(ctx, env, then_expr);
                } else {
                    result = eval_internal(ctx, env, else_expr);
                }

                cell_release(cond_val);
                return result;
            }
        }

        /* Function application */
        Cell* fn = eval_internal(ctx, env, first);
        Cell* args = eval_list(ctx, env, rest);

        Cell* result = apply(ctx, fn, args);

        cell_release(fn);
        cell_release(args);

        return result;
    }

    /* Unknown expression */
    fprintf(stderr, "Error: Cannot evaluate expression\n");
    cell_retain(expr);
    return expr;
}

/* Public eval interface */
Cell* eval(EvalContext* ctx, Cell* expr) {
    return eval_internal(ctx, ctx->env, expr);
}
