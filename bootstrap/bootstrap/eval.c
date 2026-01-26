#include "eval.h"
#include "primitives.h"
#include "debruijn.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ============ Helper Data Structures for Dependency Extraction ============ */

/* Set of symbols (for tracking parameters) */
typedef struct {
    char** symbols;
    size_t count;
    size_t capacity;
} SymbolSet;

/* Create new symbol set */
static SymbolSet* symbol_set_create(void) {
    SymbolSet* set = (SymbolSet*)malloc(sizeof(SymbolSet));
    set->capacity = 8;
    set->count = 0;
    set->symbols = (char**)malloc(sizeof(char*) * set->capacity);
    return set;
}

/* Add symbol to set (if not already present) */
static void symbol_set_add(SymbolSet* set, const char* symbol) {
    /* Check if already present */
    for (size_t i = 0; i < set->count; i++) {
        if (strcmp(set->symbols[i], symbol) == 0) {
            return;  /* Already in set */
        }
    }

    /* Grow if needed */
    if (set->count >= set->capacity) {
        set->capacity *= 2;
        set->symbols = (char**)realloc(set->symbols, sizeof(char*) * set->capacity);
    }

    /* Add new symbol */
    set->symbols[set->count++] = strdup(symbol);
}

/* Check if symbol is in set */
static int symbol_set_contains(SymbolSet* set, const char* symbol) {
    for (size_t i = 0; i < set->count; i++) {
        if (strcmp(set->symbols[i], symbol) == 0) {
            return 1;
        }
    }
    return 0;
}

/* Clone a symbol set */
static SymbolSet* symbol_set_clone(SymbolSet* set) {
    SymbolSet* clone = symbol_set_create();
    for (size_t i = 0; i < set->count; i++) {
        symbol_set_add(clone, set->symbols[i]);
    }
    return clone;
}

/* Free symbol set */
static void symbol_set_free(SymbolSet* set) {
    for (size_t i = 0; i < set->count; i++) {
        free(set->symbols[i]);
    }
    free(set->symbols);
    free(set);
}

/* List of dependency names */
typedef struct {
    char** names;
    size_t count;
    size_t capacity;
} DepList;

/* Create new dependency list */
static DepList* dep_list_create(void) {
    DepList* list = (DepList*)malloc(sizeof(DepList));
    list->capacity = 8;
    list->count = 0;
    list->names = (char**)malloc(sizeof(char*) * list->capacity);
    return list;
}

/* Add dependency to list (if not already present) */
static void dep_list_add(DepList* list, const char* name) {
    /* Check if already present */
    for (size_t i = 0; i < list->count; i++) {
        if (strcmp(list->names[i], name) == 0) {
            return;  /* Already in list */
        }
    }

    /* Grow if needed */
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->names = (char**)realloc(list->names, sizeof(char*) * list->capacity);
    }

    /* Add new name */
    list->names[list->count++] = strdup(name);
}

/* Check if name is in list */
static int dep_list_contains(DepList* list, const char* name) {
    for (size_t i = 0; i < list->count; i++) {
        if (strcmp(list->names[i], name) == 0) {
            return 1;
        }
    }
    return 0;
}

/* Free dependency list (but not the strings - they're owned by FunctionDoc) */
static void dep_list_free_shallow(DepList* list) {
    free(list->names);
    free(list);
}

/* ============ End Helper Data Structures ============ */

/* ============ Dependency Extraction ============ */

/* Extract parameters from lambda params list */
static void extract_lambda_params(Cell* params, SymbolSet* param_set) {
    if (!params || cell_is_nil(params)) return;

    if (cell_is_pair(params)) {
        Cell* first = cell_car(params);
        if (cell_is_symbol(first)) {
            symbol_set_add(param_set, cell_get_symbol(first));
        }
        extract_lambda_params(cell_cdr(params), param_set);
    } else if (cell_is_symbol(params)) {
        /* Variadic parameter */
        symbol_set_add(param_set, cell_get_symbol(params));
    }
}

/* Extract dependencies from expression
 * params: set of parameter names (variables that are bound, not dependencies)
 * deps: output list of dependency names
 */
static void extract_dependencies(Cell* expr, SymbolSet* params, DepList* deps) {
    if (!expr || cell_is_nil(expr)) return;

    /* Literals - no dependencies */
    if (cell_is_number(expr) || cell_is_bool(expr)) {
        return;
    }

    /* Symbols - check if dependency */
    if (cell_is_symbol(expr)) {
        const char* sym = cell_get_symbol(expr);
        if (!symbol_set_contains(params, sym)) {
            /* Not a parameter - it's a dependency */
            dep_list_add(deps, sym);
        }
        return;
    }

    /* Pairs - recursively process */
    if (cell_is_pair(expr)) {
        extract_dependencies(cell_car(expr), params, deps);
        extract_dependencies(cell_cdr(expr), params, deps);
        return;
    }

    /* Lambdas - add params, then process body */
    if (cell_is_lambda(expr)) {
        SymbolSet* extended_params = symbol_set_clone(params);

        /* Note: In our implementation, lambda env contains the body
         * We need to extract the parameters from somewhere...
         * For now, lambdas converted to De Bruijn don't have param names
         * So we can only extract deps from the body itself
         */
        Cell* body = expr->data.lambda.body;
        extract_dependencies(body, extended_params, deps);

        symbol_set_free(extended_params);
        return;
    }

    /* Errors - might contain dependencies */
    if (cell_is_error(expr)) {
        Cell* error_data = cell_error_data(expr);
        if (error_data) {
            extract_dependencies(error_data, params, deps);
        }
        return;
    }
}

/* ============ End Dependency Extraction ============ */

/* ============ Documentation API ============ */

/* Forward declaration */
static FunctionDoc* doc_find(EvalContext* ctx, const char* name);

/* Global current context (for primitives to access user docs) */
static EvalContext* g_current_context = NULL;

/* Set current eval context */
void eval_set_current_context(EvalContext* ctx) {
    g_current_context = ctx;
}

/* Find user function documentation (for primitives) */
FunctionDoc* eval_find_user_doc(const char* name) {
    if (!g_current_context) return NULL;
    return doc_find(g_current_context, name);
}

/* Create a new documentation entry */
static FunctionDoc* doc_create(const char* name) {
    FunctionDoc* doc = (FunctionDoc*)malloc(sizeof(FunctionDoc));
    doc->name = strdup(name);
    doc->description = NULL;
    doc->type_signature = NULL;
    doc->dependencies = NULL;
    doc->dependency_count = 0;
    doc->next = NULL;
    return doc;
}

/* Find documentation by name */
static FunctionDoc* doc_find(EvalContext* ctx, const char* name) {
    FunctionDoc* current = ctx->user_docs;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

/* Get description of a dependency (primitive or user function) */
static const char* get_dependency_description(EvalContext* ctx, const char* name) {
    /* First check if it's a user function */
    FunctionDoc* doc = doc_find(ctx, name);
    if (doc && doc->description) {
        return doc->description;
    }

    /* Check if it's a primitive */
    const Primitive* prim = primitive_lookup_by_name(name);
    if (prim && prim->doc.description) {
        return prim->doc.description;
    }

    return NULL;
}

/* Count lambda parameters (nesting depth) */
static int count_lambda_params(Cell* lambda) {
    if (!lambda || !cell_is_lambda(lambda)) {
        return 0;
    }
    return lambda->data.lambda.arity;
}

/* Infer type signature for a lambda */
static void doc_infer_type(FunctionDoc* doc, Cell* lambda) {
    if (!lambda || !cell_is_lambda(lambda)) {
        doc->type_signature = strdup("unknown");
        return;
    }

    int param_count = count_lambda_params(lambda);

    /* Build simple type signature: α → β or α → α → β, etc. */
    char buf[256] = "";
    for (int i = 0; i < param_count; i++) {
        if (i > 0) strcat(buf, " → ");
        strcat(buf, "α");
    }
    if (param_count > 0) strcat(buf, " → ");
    strcat(buf, "β");

    doc->type_signature = strdup(buf);
}

/* Generate description from dependencies */
static void doc_generate_description(EvalContext* ctx, FunctionDoc* doc, Cell* body) {
    /* Extract dependencies from the body */
    SymbolSet* params = symbol_set_create();
    DepList* deps = dep_list_create();
    extract_dependencies(body, params, deps);

    /* Store dependencies - transfer ownership of strings */
    doc->dependency_count = deps->count;
    doc->dependencies = deps->names;  /* Transfer ownership of array AND strings */

    /* Compose description based on dependencies */
    if (deps->count == 0) {
        doc->description = strdup("User-defined function");
    } else if (deps->count == 1) {
        /* Single dependency - look up its description */
        const char* dep_desc = get_dependency_description(ctx, deps->names[0]);
        if (dep_desc) {
            char buf[512];
            snprintf(buf, sizeof(buf), "Function using: %s", dep_desc);
            doc->description = strdup(buf);
        } else {
            char buf[256];
            snprintf(buf, sizeof(buf), "Function using: %s", deps->names[0]);
            doc->description = strdup(buf);
        }
    } else {
        /* Multiple dependencies - list them */
        char buf[1024] = "Function using: ";
        size_t max_deps = (deps->count < 5) ? deps->count : 5;
        for (size_t i = 0; i < max_deps; i++) {
            if (i > 0) strcat(buf, ", ");
            strcat(buf, deps->names[i]);
        }
        if (deps->count > 5) {
            strcat(buf, ", ...");
        }
        doc->description = strdup(buf);
    }

    symbol_set_free(params);
    /* Free the DepList struct but NOT the array (doc owns it now) */
    free(deps);
}

/* ============ End Documentation API ============ */

/* Create new evaluation context */
EvalContext* eval_context_new(void) {
    EvalContext* ctx = (EvalContext*)malloc(sizeof(EvalContext));
    ctx->env = cell_nil();
    ctx->primitives = primitives_init();
    ctx->user_docs = NULL;  /* Initialize doc list */
    return ctx;
}

/* Free documentation list */
static void doc_free_all(FunctionDoc* head) {
    while (head) {
        FunctionDoc* next = head->next;
        free(head->name);
        free(head->description);
        free(head->type_signature);
        for (size_t i = 0; i < head->dependency_count; i++) {
            free(head->dependencies[i]);
        }
        free(head->dependencies);
        free(head);
        head = next;
    }
}

/* Free evaluation context */
void eval_context_free(EvalContext* ctx) {
    cell_release(ctx->env);
    cell_release(ctx->primitives);
    doc_free_all(ctx->user_docs);
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

    /* Generate documentation if value is a lambda */
    if (value && cell_is_lambda(value)) {
        FunctionDoc* doc = doc_create(name);

        /* Generate docs - panic if it fails */
        doc_generate_description(ctx, doc, value->data.lambda.body);
        if (!doc->description) {
            fprintf(stderr, "PANIC: Failed to generate description for function '%s'\n", name);
            abort();
        }

        doc_infer_type(doc, value);
        if (!doc->type_signature) {
            fprintf(stderr, "PANIC: Failed to infer type for function '%s'\n", name);
            abort();
        }

        /* Add to context's doc list */
        doc->next = ctx->user_docs;
        ctx->user_docs = doc;

        /* Auto-print documentation (unless --no-doc flag is set) */
        printf("📝 %s :: %s\n", name, doc->type_signature);
        printf("   %s\n", doc->description);
        if (doc->dependency_count > 0) {
            printf("   Dependencies: ");
            for (size_t i = 0; i < doc->dependency_count; i++) {
                if (i > 0) printf(", ");
                printf("%s", doc->dependencies[i]);
            }
            printf("\n");
        }
    }
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
    eval_set_current_context(ctx);  /* Set global context for primitives */
    return eval_internal(ctx, ctx->env, expr);
}
