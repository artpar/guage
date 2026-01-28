#include "eval.h"
#include "primitives.h"
#include "debruijn.h"
#include "pattern.h"
#include "module.h"
#include "macro.h"
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
    if (cell_is_number(expr) || cell_is_bool(expr) || cell_is_string(expr)) {
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

/* Get current eval context (for primitives) */
EvalContext* eval_get_current_context(void) {
    return g_current_context;
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

/* Check if expression uses only arithmetic operations */
static bool uses_only_arithmetic(Cell* expr) {
    if (!expr) return false;

    if (cell_is_symbol(expr)) {
        const char* s = cell_get_symbol(expr);
        return (strcmp(s, "⊕") == 0 || strcmp(s, "⊖") == 0 ||
                strcmp(s, "⊗") == 0 || strcmp(s, "⊘") == 0);
    }

    if (cell_is_pair(expr)) {
        Cell* func = cell_car(expr);
        if (cell_is_symbol(func)) {
            const char* s = cell_get_symbol(func);
            if (strcmp(s, "⊕") == 0 || strcmp(s, "⊖") == 0 ||
                strcmp(s, "⊗") == 0 || strcmp(s, "⊘") == 0) {
                return true;
            }
        }
        return uses_only_arithmetic(cell_car(expr)) ||
               uses_only_arithmetic(cell_cdr(expr));
    }

    return false;
}

/* Check if expression returns boolean */
static bool returns_bool(Cell* expr) {
    if (!expr) return false;

    if (cell_is_pair(expr)) {
        Cell* func = cell_car(expr);
        if (cell_is_symbol(func)) {
            const char* s = cell_get_symbol(func);
            /* Comparison operators return bool */
            if (strcmp(s, "<") == 0 || strcmp(s, ">") == 0 ||
                strcmp(s, "≡") == 0 || strcmp(s, "≢") == 0 ||
                strcmp(s, "≤") == 0 || strcmp(s, "≥") == 0) {
                return true;
            }
            /* Logical operators return bool */
            if (strcmp(s, "∧") == 0 || strcmp(s, "∨") == 0 || strcmp(s, "¬") == 0) {
                return true;
            }
            /* Type predicates return bool */
            if (strcmp(s, "ℕ?") == 0 || strcmp(s, "𝔹?") == 0 ||
                strcmp(s, ":?") == 0 || strcmp(s, "∅?") == 0 ||
                strcmp(s, "⟨⟩?") == 0 || strcmp(s, "#?") == 0 ||
                strcmp(s, "⚠?") == 0) {
                return true;
            }
        }
    }

    return false;
}

/* Infer type signature for a lambda - MOST STRONGLY TYPED */
static void doc_infer_type(FunctionDoc* doc, Cell* lambda) {
    if (!lambda || !cell_is_lambda(lambda)) {
        doc->type_signature = strdup("unknown");
        return;
    }

    int param_count = count_lambda_params(lambda);
    Cell* body = lambda->data.lambda.body;

    char buf[256] = "";

    /* Infer the MOST SPECIFIC type based on body structure */

    /* Check if only uses arithmetic - strongest type: ℕ → ℕ */
    if (uses_only_arithmetic(body)) {
        for (int i = 0; i < param_count; i++) {
            if (i > 0) strcat(buf, " → ");
            strcat(buf, "ℕ");
        }
        if (param_count > 0) strcat(buf, " → ");
        strcat(buf, "ℕ");
        doc->type_signature = strdup(buf);
        return;
    }

    /* Check if returns boolean - strong type: α → 𝔹 */
    if (returns_bool(body)) {
        for (int i = 0; i < param_count; i++) {
            if (i > 0) strcat(buf, " → ");
            strcat(buf, "α");
        }
        if (param_count > 0) strcat(buf, " → ");
        strcat(buf, "𝔹");
        doc->type_signature = strdup(buf);
        return;
    }

    /* Check if body is a conditional - might return bool or same type */
    if (cell_is_pair(body)) {
        Cell* func = cell_car(body);
        if (cell_is_symbol(func) && strcmp(cell_get_symbol(func), "?") == 0) {
            /* Conditional - check branches */
            Cell* args = cell_cdr(body);
            if (args && cell_is_pair(args)) {
                Cell* rest = cell_cdr(args);
                if (rest && cell_is_pair(rest)) {
                    Cell* then_branch = cell_car(rest);
                    /* If then branch is number, likely returns number */
                    if (then_branch && cell_is_number(then_branch)) {
                        for (int i = 0; i < param_count; i++) {
                            if (i > 0) strcat(buf, " → ");
                            strcat(buf, "ℕ");
                        }
                        if (param_count > 0) strcat(buf, " → ");
                        strcat(buf, "ℕ");
                        doc->type_signature = strdup(buf);
                        return;
                    }
                }
            }
        }
    }

    /* Default: Generic polymorphic type α → β */
    for (int i = 0; i < param_count; i++) {
        if (i > 0) strcat(buf, " → ");
        strcat(buf, "α");
    }
    if (param_count > 0) strcat(buf, " → ");
    strcat(buf, "β");

    doc->type_signature = strdup(buf);
}

/* ============ Recursive Description Composition ============ */

#define MAX_DESC_LENGTH 2048
#define MAX_RECURSION_DEPTH 15

/* Forward declarations */
static char* compose_expr_description(EvalContext* ctx, Cell* expr, SymbolSet* params, int depth);
static char* compose_conditional_description(EvalContext* ctx, Cell* args, SymbolSet* params, int depth);
static char* compose_binary_op_description(EvalContext* ctx, const char* op_name,
                                           Cell* args, SymbolSet* params, int depth);

/* Recursively compose human-readable description from expression AST */
static char* compose_expr_description(EvalContext* ctx, Cell* expr, SymbolSet* params, int depth) {
    if (depth > MAX_RECURSION_DEPTH) {
        return strdup("(deeply nested expression)");
    }

    if (!expr) return strdup("nil");

    /* Number - might be De Bruijn index or literal */
    if (cell_is_number(expr)) {
        double num = cell_get_number(expr);
        /* De Bruijn indices are small non-negative integers in lambda bodies */
        /* Heuristic: if it's 0-9 and integer, treat as parameter reference */
        if (num >= 0 && num < 10 && num == (int)num) {
            /* Likely a De Bruijn index - convert to param name */
            int index = (int)num;
            char buf[64];
            /* Use more natural names */
            switch (index) {
                case 0: strcpy(buf, "the argument"); break;
                case 1: strcpy(buf, "second argument"); break;
                case 2: strcpy(buf, "third argument"); break;
                default: snprintf(buf, sizeof(buf), "argument %d", index + 1); break;
            }
            return strdup(buf);
        } else {
            /* Literal number */
            char buf[64];
            snprintf(buf, sizeof(buf), "%.0f", num);
            return strdup(buf);
        }
    }

    /* Boolean */
    if (cell_is_bool(expr)) {
        return strdup(cell_get_bool(expr) ? "true" : "false");
    }

    /* Nil */
    if (cell_is_nil(expr)) {
        return strdup("nil");
    }

    /* Symbol */
    if (cell_is_symbol(expr)) {
        const char* sym = cell_get_symbol(expr);

        /* Check if it's a parameter */
        if (symbol_set_contains(params, sym)) {
            return strdup(sym);
        }

        /* Check if it's a primitive */
        const Primitive* prim = primitive_lookup_by_name(sym);
        if (prim && prim->doc.description) {
            /* Return simplified description */
            return strdup(sym);  /* Just return the symbol name for brevity */
        }

        /* Check if it's a user function */
        FunctionDoc* user_doc = doc_find(ctx, sym);
        if (user_doc) {
            return strdup(sym);  /* Return function name */
        }

        return strdup(sym);
    }

    /* Pair (function application) */
    if (cell_is_pair(expr)) {
        Cell* func = cell_car(expr);
        Cell* args = cell_cdr(expr);

        /* Special pattern matching for known constructs */
        if (cell_is_symbol(func)) {
            const char* func_name = cell_get_symbol(func);

            /* Quote-wrapped number literal: (⌜ n) -> just return the number */
            if (strcmp(func_name, "⌜") == 0) {
                if (args && cell_is_pair(args)) {
                    Cell* quoted_val = cell_car(args);
                    if (cell_is_number(quoted_val)) {
                        char buf[64];
                        snprintf(buf, sizeof(buf), "%.0f", cell_get_number(quoted_val));
                        return strdup(buf);
                    }
                }
                /* Fallthrough for other quoted values */
            }

            /* Conditional: (? cond then else) */
            if (strcmp(func_name, "?") == 0) {
                return compose_conditional_description(ctx, args, params, depth);
            }

            /* Binary operators */
            if (strcmp(func_name, "⊗") == 0) {
                return compose_binary_op_description(ctx, "multiply", args, params, depth);
            }
            if (strcmp(func_name, "⊕") == 0) {
                return compose_binary_op_description(ctx, "add", args, params, depth);
            }
            if (strcmp(func_name, "⊖") == 0) {
                return compose_binary_op_description(ctx, "subtract", args, params, depth);
            }
            if (strcmp(func_name, "⊘") == 0) {
                return compose_binary_op_description(ctx, "divide", args, params, depth);
            }
            if (strcmp(func_name, "≡") == 0) {
                return compose_binary_op_description(ctx, "equals", args, params, depth);
            }
            if (strcmp(func_name, "≢") == 0) {
                return compose_binary_op_description(ctx, "not equals", args, params, depth);
            }
            if (strcmp(func_name, "<") == 0) {
                return compose_binary_op_description(ctx, "less than", args, params, depth);
            }
            if (strcmp(func_name, ">") == 0) {
                return compose_binary_op_description(ctx, "greater than", args, params, depth);
            }
            if (strcmp(func_name, "≤") == 0) {
                return compose_binary_op_description(ctx, "less than or equal", args, params, depth);
            }
            if (strcmp(func_name, "≥") == 0) {
                return compose_binary_op_description(ctx, "greater than or equal", args, params, depth);
            }
            if (strcmp(func_name, "∧") == 0) {
                return compose_binary_op_description(ctx, "and", args, params, depth);
            }
            if (strcmp(func_name, "∨") == 0) {
                return compose_binary_op_description(ctx, "or", args, params, depth);
            }
        }

        /* Generic application: apply func to args */
        char* func_desc = compose_expr_description(ctx, func, params, depth + 1);

        /* Compose args */
        char args_buf[512] = "";
        Cell* arg_list = args;
        bool first = true;
        while (arg_list && cell_is_pair(arg_list)) {
            char* arg_desc = compose_expr_description(ctx, cell_car(arg_list), params, depth + 1);
            if (!first) strncat(args_buf, " and ", sizeof(args_buf) - strlen(args_buf) - 1);
            strncat(args_buf, arg_desc, sizeof(args_buf) - strlen(args_buf) - 1);
            free(arg_desc);
            first = false;
            arg_list = cell_cdr(arg_list);
        }

        char result[MAX_DESC_LENGTH];
        snprintf(result, sizeof(result), "apply %s to %s", func_desc, args_buf);
        free(func_desc);
        return strdup(result);
    }

    /* Lambda */
    if (cell_is_lambda(expr)) {
        /* Don't recurse into nested lambdas for now */
        return strdup("(nested lambda)");
    }

    return strdup("(unknown)");
}

/* Compose conditional description: (? cond then else) */
static char* compose_conditional_description(EvalContext* ctx, Cell* args,
                                             SymbolSet* params, int depth) {
    if (!args || !cell_is_pair(args)) {
        return strdup("(malformed conditional)");
    }

    Cell* cond = cell_car(args);
    Cell* rest = cell_cdr(args);
    if (!rest || !cell_is_pair(rest)) {
        return strdup("(malformed conditional)");
    }

    Cell* then_branch = cell_car(rest);
    Cell* else_rest = cell_cdr(rest);
    if (!else_rest || !cell_is_pair(else_rest)) {
        return strdup("(malformed conditional)");
    }

    Cell* else_branch = cell_car(else_rest);

    char* cond_desc = compose_expr_description(ctx, cond, params, depth + 1);
    char* then_desc = compose_expr_description(ctx, then_branch, params, depth + 1);
    char* else_desc = compose_expr_description(ctx, else_branch, params, depth + 1);

    char result[MAX_DESC_LENGTH];
    snprintf(result, sizeof(result), "if %s then %s else %s",
             cond_desc, then_desc, else_desc);

    free(cond_desc);
    free(then_desc);
    free(else_desc);
    return strdup(result);
}

/* Compose binary operation description */
static char* compose_binary_op_description(EvalContext* ctx, const char* op_name,
                                          Cell* args, SymbolSet* params, int depth) {
    if (!args || !cell_is_pair(args)) {
        return strdup("(malformed binary op)");
    }

    Cell* left = cell_car(args);
    Cell* rest = cell_cdr(args);
    if (!rest || !cell_is_pair(rest)) {
        return strdup("(malformed binary op)");
    }

    Cell* right = cell_car(rest);

    char* left_desc = compose_expr_description(ctx, left, params, depth + 1);
    char* right_desc = compose_expr_description(ctx, right, params, depth + 1);

    char result[MAX_DESC_LENGTH];
    snprintf(result, sizeof(result), "%s %s and %s", op_name, left_desc, right_desc);

    free(left_desc);
    free(right_desc);
    return strdup(result);
}

/* Generate description from body using recursive composition */
static void doc_generate_description(EvalContext* ctx, FunctionDoc* doc, Cell* body) {
    /* Extract dependencies from the body */
    SymbolSet* params = symbol_set_create();
    DepList* deps = dep_list_create();
    extract_dependencies(body, params, deps);

    /* Store dependencies - transfer ownership of strings */
    doc->dependency_count = deps->count;
    doc->dependencies = deps->names;  /* Transfer ownership of array AND strings */

    /* Recursively compose description from body structure */
    doc->description = compose_expr_description(ctx, body, params, 0);

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
    ctx->type_registry = cell_nil();  /* Initialize type registry */
    macro_init();  /* Initialize macro system */
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
    cell_release(ctx->type_registry);
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

    /* Track this symbol in module registry */
    const char* current_module = module_get_current_loading();
    if (current_module != NULL) {
        /* Loading a module file - register in that module */
        module_registry_add_symbol(current_module, name);
    } else {
        /* REPL definition - register in virtual <repl> module */
        module_registry_add_symbol("<repl>", name);
    }

    /* Generate documentation if value is a lambda */
    if (value && cell_is_lambda(value)) {
        FunctionDoc* doc = doc_create(name);

        /* Generate docs - graceful degradation on failure */
        doc_generate_description(ctx, doc, value->data.lambda.body);
        if (!doc->description) {
            fprintf(stderr, "Warning: Could not generate description for '%s'\n", name);
            doc->description = strdup("undocumented");
        }

        doc_infer_type(doc, value);
        if (!doc->type_signature) {
            fprintf(stderr, "Warning: Could not infer type for '%s'\n", name);
            doc->type_signature = strdup("α → β");  /* Generic fallback */
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

    /* Walk through the environment, skipping the marker if encountered */
    for (int i = 0; i <= index; i++) {
        if (!cell_is_pair(current)) {
            return NULL;
        }

        Cell* elem = cell_car(current);

        /* Skip marker */
        if (cell_is_symbol(elem)) {
            const char* sym = cell_get_symbol(elem);
            if (strcmp(sym, ":__indexed__") == 0) {
                current = cell_cdr(current);
                continue;  /* Don't count this as an index */
            }
        }

        /* Found the element at the desired index */
        if (i == index) {
            cell_retain(elem);
            return elem;
        }

        /* Continue to next element */
        current = cell_cdr(current);
    }

    return NULL;
}

/* Extend environment with argument values (prepend args to env) */
Cell* extend_env(Cell* env, Cell* args) {
    /* When creating an indexed environment from scratch, add a marker at the END
     * to distinguish it from named environments. This allows both the C evaluator
     * and the Guage self-hosting evaluator to work correctly. */

    /* Base case: extend with the existing env (which might be nil or indexed) */
    if (cell_is_nil(args)) {
        /* If env is empty and we're creating a new indexed env, add marker */
        if (cell_is_nil(env)) {
            Cell* marker = cell_symbol(":__indexed__");
            Cell* result = cell_cons(marker, cell_nil());
            cell_release(marker);
            return result;
        }
        cell_retain(env);
        return env;
    }

    /* Recursive case: cons this arg onto the extended rest */
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
Cell* eval_internal(EvalContext* ctx, Cell* env, Cell* expr);

/* Evaluate quasiquote expression (supports unquote)
 * Walks tree recursively, evaluating unquoted parts */
static Cell* eval_quasiquote(EvalContext* ctx, Cell* env, Cell* expr) {
    /* Check if this is an unquote form: (~ expr) */
    if (cell_is_pair(expr)) {
        Cell* first = cell_car(expr);
        if (cell_is_symbol(first)) {
            const char* sym = cell_get_symbol(first);
            /* Check for unquote: ~ or unquote */
            if (strcmp(sym, "~") == 0 || strcmp(sym, "unquote") == 0) {
                /* Evaluate the unquoted expression */
                Cell* rest = cell_cdr(expr);
                if (cell_is_nil(rest)) {
                    return cell_error("quasiquote-error",
                        cell_symbol("unquote-requires-argument"));
                }
                Cell* unquoted_expr = cell_car(rest);
                return eval_internal(ctx, env, unquoted_expr);
            }
        }

        /* Not an unquote - recursively process car and cdr */
        Cell* new_car = eval_quasiquote(ctx, env, first);
        Cell* rest = cell_cdr(expr);
        Cell* new_cdr = eval_quasiquote(ctx, env, rest);

        Cell* result = cell_cons(new_car, new_cdr);
        cell_release(new_car);
        cell_release(new_cdr);
        return result;
    }

    /* Atoms (numbers, bools, symbols, nil) - return as-is */
    cell_retain(expr);
    return expr;
}

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
/* apply() function removed - inlined into eval_internal for TCO */

/* Check if environment is indexed (not named/assoc) */
bool env_is_indexed(Cell* env) {
    /* Indexed environment: (val1 val2 ... :__indexed__) with marker at end */
    /* Named environment: ((sym1 . val1) (sym2 . val2) ...) */
    if (cell_is_nil(env)) return true;  /* Empty env can be either */
    if (!cell_is_pair(env)) return false;

    /* Walk the environment looking for the indexed marker */
    Cell* current = env;
    while (cell_is_pair(current)) {
        Cell* elem = cell_car(current);
        if (cell_is_symbol(elem)) {
            const char* sym = cell_get_symbol(elem);
            if (strcmp(sym, ":__indexed__") == 0) {
                return true;  /* Found marker - this is indexed */
            }
        }
        current = cell_cdr(current);
    }

    /* No marker found - check if it looks like a named environment */
    Cell* first = cell_car(env);
    if (cell_is_pair(first)) {
        Cell* car_of_first = cell_car(first);
        if (cell_is_symbol(car_of_first)) {
            const char* sym = cell_get_symbol(car_of_first);
            if (sym[0] != ':') {
                /* Regular symbol - this is a named binding */
                return false;
            }
        }
    }
    /* Not a named binding and no marker - assume indexed for backwards compat */
    return true;
}

/* Evaluate expression with proper tail call optimization */
Cell* eval_internal(EvalContext* ctx, Cell* env, Cell* expr) {
    Cell* owned_env = NULL;   /* Track owned environments for cleanup */
    Cell* owned_expr = NULL;  /* Track owned expressions for cleanup */

tail_call:  /* TCO: loop back here instead of recursive call */
    /* Note: owned resources released right before setting new ones */
    /* Macro expansion pass - expand macros before evaluation */
    if (cell_is_pair(expr)) {
        Cell* first = cell_car(expr);
        if (cell_is_symbol(first) && macro_is_macro_call(expr)) {
            /* This is a macro call - expand it */
            Cell* expanded = macro_expand(expr, ctx);
            if (cell_is_error(expanded)) {
                return expanded;
            }
            /* Evaluate the expanded code - TAIL CALL */
            /* Release previous owned_expr before replacing */
            if (owned_expr) {
                cell_release(owned_expr);
            }

            expr = expanded;
            owned_expr = expanded;  /* Track for cleanup */
            goto tail_call;
        }
    }

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
    if (cell_is_bool(expr) || cell_is_nil(expr) || cell_is_string(expr)) {
        cell_retain(expr);
        return expr;
    }

    /* Keyword symbols (start with ':') are self-evaluating */
    if (cell_is_symbol(expr)) {
        const char* name = cell_get_symbol(expr);
        if (name[0] == ':') {
            /* Keywords are self-evaluating (like :Point, :x, :Cons) */
            cell_retain(expr);
            return expr;
        }

        /* Regular symbols: variable lookup */
        Cell* value = eval_lookup(ctx, name);
        if (value == NULL) {
            Cell* var_name = cell_symbol(name);
            Cell* result = cell_error("undefined-variable", var_name);
            cell_release(var_name);
            return result;
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

            /* ⌞̃ - quasiquote (quote with unquote support) */
            if (strcmp(sym, "⌞̃") == 0 || strcmp(sym, "quasiquote") == 0) {
                Cell* arg = cell_car(rest);
                return eval_quasiquote(ctx, env, arg);
            }

            /* ⧉ - macro define (3 args) OR arity (1 arg) */
            if (strcmp(sym, "⧉") == 0) {
                /* Count arguments */
                int arg_count = 0;
                Cell* temp = rest;
                while (temp && cell_is_pair(temp)) {
                    arg_count++;
                    temp = cell_cdr(temp);
                }

                /* If 3 arguments, it's a macro definition */
                if (arg_count == 3) {
                    /* Syntax: (⧉ name (param1 param2 ...) body) */
                    Cell* name = cell_car(rest);
                    Cell* params = cell_car(cell_cdr(rest));
                    Cell* body = cell_car(cell_cdr(cell_cdr(rest)));

                    if (!cell_is_symbol(name)) {
                        return cell_error("macro-name-not-symbol", name);
                    }
                    const char* name_str = cell_get_symbol(name);

                    /* Register macro - DON'T evaluate body (it's a template) */
                    macro_define(name_str, params, body);

                    /* Return name as symbol */
                    cell_retain(name);
                    return name;
                }
                /* Otherwise fall through to primitive (arity) */
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
                /* Day 27: Pass source location (module tracked, line number not yet available) */
                Cell* lambda = cell_lambda(closure_env, converted_body, arity,
                                          module_get_current_loading(), 0);

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
                /* Day 27: Pass source location (module tracked, line number not yet available) */
                Cell* lambda = cell_lambda(closure_env, converted_body, arity,
                                          module_get_current_loading(), 0);

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

                /* TCO: Branch evaluation is in tail position */
                if (cell_is_bool(cond_val) && cell_get_bool(cond_val)) {
                    cell_release(cond_val);
                    expr = then_expr;
                    goto tail_call;
                } else {
                    cell_release(cond_val);
                    expr = else_expr;
                    goto tail_call;
                }
            }

            /* ∇ - pattern match (special form) */
            if (strcmp(sym, "∇") == 0) {
                Cell* expr_unevaled = cell_car(rest);
                Cell* clauses_sexpr = cell_car(cell_cdr(rest));

                /* User should quote the whole clause list: (⌜ ((p1 r1) (p2 r2) ...)) */
                /* Eval once to unquote, giving us the data structure */
                Cell* clauses_data = eval_internal(ctx, env, clauses_sexpr);

                /* Pass to pattern matching */
                Cell* result = pattern_eval_match(expr_unevaled, clauses_data, ctx);

                /* Clean up */
                cell_release(clauses_data);

                return result;
            }
        }

        /* Function application */
        /* Special case: :? primitive lookup (keyword exception) */
        Cell* fn;
        if (cell_is_symbol(first)) {
            const char* fn_name = cell_get_symbol(first);
            if (strcmp(fn_name, ":?") == 0) {
                /* Look up :? as primitive, not keyword */
                fn = eval_lookup(ctx, fn_name);
                if (fn == NULL) {
                    Cell* var_name = cell_symbol(fn_name);
                    Cell* result = cell_error("undefined-variable", var_name);
                    cell_release(var_name);
                    Cell* args = eval_list(ctx, env, rest);
                    cell_release(args);
                    return result;
                }
            } else {
                fn = eval_internal(ctx, env, first);
            }
        } else {
            fn = eval_internal(ctx, env, first);
        }

        Cell* args = eval_list(ctx, env, rest);

        /* Inline apply() for TCO */
        if (fn->type == CELL_BUILTIN) {
            /* Call builtin primitive - not a tail call */
            Cell* (*builtin_fn)(Cell*) = (Cell* (*)(Cell*))fn->data.atom.builtin;
            Cell* result = builtin_fn(args);
            cell_release(fn);
            cell_release(args);
            return result;
        }

        if (fn->type == CELL_LAMBDA) {
            /* Extract lambda components */
            Cell* closure_env = fn->data.lambda.env;
            Cell* body = fn->data.lambda.body;
            int arity = fn->data.lambda.arity;

            /* Check argument count */
            int arg_count = list_length(args);
            if (arg_count != arity) {
                Cell* expected = cell_number(arity);
                Cell* actual = cell_number(arg_count);
                Cell* data = cell_cons(expected, cell_cons(actual, cell_nil()));
                cell_release(expected);
                cell_release(actual);
                cell_release(fn);
                cell_release(args);
                return cell_error("arity-mismatch", data);
            }

            /* Retain body before releasing fn (body points into fn) */
            cell_retain(body);

            /* Create new environment: prepend args to closure env */
            Cell* new_env = extend_env(closure_env, args);

            /* TCO: Lambda body evaluation is in tail position */
            cell_release(fn);
            cell_release(args);

            /* Release previous owned_env/expr before replacing */
            if (owned_env) {
                cell_release(owned_env);
            }
            if (owned_expr) {
                cell_release(owned_expr);
            }

            env = new_env;
            owned_env = new_env;  /* Track for cleanup */
            expr = body;
            owned_expr = body;     /* Track for cleanup */
            goto tail_call;
        }

        /* Not a function */
        cell_release(args);
        cell_retain(fn);
        Cell* result = cell_error("not-a-function", fn);
        cell_release(fn);
        return result;
    }

    /* Unknown expression */
    cell_retain(expr);
    return cell_error("eval-error", expr);
}

/* Public eval interface */
Cell* eval(EvalContext* ctx, Cell* expr) {
    eval_set_current_context(ctx);  /* Set global context for primitives */
    return eval_internal(ctx, ctx->env, expr);
}

/* ============ Type Registry Operations ============ */

/* Register a type definition in the type registry
 * type_tag: symbol like :Point, :List, :Tree
 * schema: structure definition (fields, variants, etc)
 */
void eval_register_type(EvalContext* ctx, Cell* type_tag, Cell* schema) {
    assert(cell_is_symbol(type_tag));

    /* Check if type already exists - overwrite if so */
    Cell* existing = eval_lookup_type(ctx, type_tag);
    if (existing) {
        cell_release(existing);
        /* Remove old entry by rebuilding registry without it */
        Cell* new_registry = cell_nil();
        Cell* current = ctx->type_registry;
        while (cell_is_pair(current)) {
            Cell* binding = cell_car(current);
            if (cell_is_pair(binding)) {
                Cell* tag = cell_car(binding);
                if (!cell_equal(tag, type_tag)) {
                    cell_retain(binding);
                    new_registry = cell_cons(binding, new_registry);
                }
            }
            current = cell_cdr(current);
        }
        cell_release(ctx->type_registry);
        ctx->type_registry = new_registry;
    }

    /* Add new binding */
    cell_retain(type_tag);
    cell_retain(schema);
    Cell* binding = cell_cons(type_tag, schema);
    ctx->type_registry = cell_cons(binding, ctx->type_registry);
}

/* Lookup a type definition in the type registry
 * Returns schema or NULL if not found
 */
Cell* eval_lookup_type(EvalContext* ctx, Cell* type_tag) {
    assert(cell_is_symbol(type_tag));

    Cell* current = ctx->type_registry;
    while (cell_is_pair(current)) {
        Cell* binding = cell_car(current);
        if (cell_is_pair(binding)) {
            Cell* tag = cell_car(binding);
            if (cell_equal(tag, type_tag)) {
                Cell* schema = cell_cdr(binding);
                cell_retain(schema);
                return schema;
            }
        }
        current = cell_cdr(current);
    }

    return NULL;
}

/* Check if a type is registered
 * Returns true if type_tag exists in registry
 */
bool eval_has_type(EvalContext* ctx, Cell* type_tag) {
    assert(cell_is_symbol(type_tag));

    Cell* schema = eval_lookup_type(ctx, type_tag);
    if (schema) {
        cell_release(schema);
        return true;
    }
    return false;
}
