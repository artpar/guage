#include "eval.h"
#include "intern.h"
#include "primitives.h"
#include "debruijn.h"
#include "pattern.h"
#include "module.h"
#include "macro.h"
#include "scheduler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ============ Coverage Bitmap (Part 3 — span tracking) ============ */

uint8_t* g_coverage_bitmap = NULL;
static uint32_t g_coverage_total_bytes = 0;

void coverage_init(uint32_t total_bytes) {
    if (g_coverage_bitmap) free(g_coverage_bitmap);
    g_coverage_total_bytes = total_bytes;
    uint32_t bitmap_bytes = (total_bytes + 7) / 8;
    g_coverage_bitmap = (uint8_t*)calloc(bitmap_bytes, 1);
}

void coverage_mark(Span span) {
    uint32_t lo = span.inline_span.lo;
    uint32_t len = span.inline_span.len;
    if (lo == 0 && len == 0) return;
    uint32_t hi = lo + len;
    if (hi > g_coverage_total_bytes) hi = g_coverage_total_bytes;
    for (uint32_t i = lo; i < hi; i++) {
        g_coverage_bitmap[i / 8] |= (1u << (i % 8));
    }
}

void coverage_emit_json(FILE* out, const char* filename) {
    if (!g_coverage_bitmap || g_coverage_total_bytes == 0) return;
    uint32_t covered = 0;
    for (uint32_t i = 0; i < g_coverage_total_bytes; i++) {
        if (g_coverage_bitmap[i / 8] & (1u << (i % 8))) covered++;
    }
    double pct = (g_coverage_total_bytes > 0) ?
        (100.0 * covered / g_coverage_total_bytes) : 0.0;
    fprintf(out, "{\"t\":\"coverage\",\"file\":\"%s\",\"covered_bytes\":%u,"
                 "\"total_bytes\":%u,\"pct\":%.1f}\n",
            filename, covered, g_coverage_total_bytes, pct);
}

/* ── Profiling counters (eval subsystem) ── */
uint64_t g_prof_eval_steps = 0;
uint64_t g_prof_lambda_calls = 0;
uint64_t g_prof_builtin_calls = 0;
uint64_t g_prof_tail_calls = 0;
uint64_t g_prof_env_lookups = 0;
uint64_t g_prof_env_steps = 0;

/* ── O(1) global binding table (indexed by intern sym_id) ── */
static Cell* g_global_table[4096];

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
        Cell* head = cell_car(expr);
        /* Handle :λ-converted — treat params as bound, only process body */
        if (cell_is_symbol(head) && strcmp(cell_get_symbol(head), ":λ-converted") == 0) {
            Cell* rest = cell_cdr(expr);
            if (rest && cell_is_pair(rest)) {
                Cell* inner_params = cell_car(rest);
                Cell* body_rest = cell_cdr(rest);
                /* Add inner params to bound set */
                SymbolSet* extended = symbol_set_clone(params);
                extract_lambda_params(inner_params, extended);
                if (body_rest && cell_is_pair(body_rest)) {
                    extract_dependencies(cell_car(body_rest), extended, deps);
                }
                symbol_set_free(extended);
                return;
            }
        }
        extract_dependencies(head, params, deps);
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
    if (UNLIKELY(cell_is_error(expr))) {
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

/* Thread-local current context (for primitives to access user docs) */
static _Thread_local EvalContext* g_current_context = NULL;

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
    doc->summary = NULL;
    doc->description = NULL;
    doc->flow_info = NULL;
    doc->type_signature = NULL;
    doc->param_names = NULL;
    doc->arity = 0;
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

/* Check if body returns only symbols (like :hot, :cold) in all branches */
static bool returns_only_symbols(Cell* expr) {
    if (!expr) return false;
    if (cell_is_symbol(expr)) {
        const char* s = cell_get_symbol(expr);
        return (s[0] == ':');  /* keyword symbols */
    }
    /* In a conditional, check both branches */
    if (cell_is_pair(expr)) {
        Cell* func = cell_car(expr);
        if (cell_is_symbol(func) && strcmp(cell_get_symbol(func), "?") == 0) {
            Cell* args = cell_cdr(expr);
            if (args && cell_is_pair(args)) {
                Cell* rest = cell_cdr(args);
                if (rest && cell_is_pair(rest)) {
                    Cell* then_branch = cell_car(rest);
                    Cell* else_rest = cell_cdr(rest);
                    if (else_rest && cell_is_pair(else_rest)) {
                        Cell* else_branch = cell_car(else_rest);
                        return returns_only_symbols(then_branch) &&
                               returns_only_symbols(else_branch);
                    }
                }
            }
        }
        /* Quoted symbol */
        if (cell_is_symbol(func) && strcmp(cell_get_symbol(func), "⌜") == 0) {
            Cell* qrest = cell_cdr(expr);
            if (qrest && cell_is_pair(qrest)) {
                Cell* qv = cell_car(qrest);
                if (cell_is_symbol(qv)) {
                    const char* s = cell_get_symbol(qv);
                    return (s[0] == ':');
                }
            }
        }
    }
    return false;
}

/* Check if body contains ∅? test + ∅ base case (list recursion pattern) */
static bool has_nil_base_case(Cell* expr) {
    if (!expr || !cell_is_pair(expr)) return false;
    Cell* func = cell_car(expr);
    if (cell_is_symbol(func) && strcmp(cell_get_symbol(func), "?") == 0) {
        Cell* args = cell_cdr(expr);
        if (args && cell_is_pair(args)) {
            Cell* cond = cell_car(args);
            /* Check if condition is (∅? ...) */
            if (cell_is_pair(cond)) {
                Cell* cf = cell_car(cond);
                if (cell_is_symbol(cf) && strcmp(cell_get_symbol(cf), "∅?") == 0) {
                    return true;
                }
            }
        }
    }
    return false;
}

/* Check if body uses ⟨⟩ (cons) for list construction */
static bool uses_cons(Cell* expr) {
    if (!expr) return false;
    if (cell_is_symbol(expr) && strcmp(cell_get_symbol(expr), "⟨⟩") == 0) return true;
    if (cell_is_pair(expr)) {
        return uses_cons(cell_car(expr)) || uses_cons(cell_cdr(expr));
    }
    return false;
}

/* Forward declaration */
static bool param_used_as_function(Cell* expr, int param_index);

/* Infer type signature for a lambda */
static void doc_infer_type(FunctionDoc* doc, Cell* lambda) {
    if (!lambda || !cell_is_lambda(lambda)) {
        doc->type_signature = strdup("unknown");
        return;
    }

    int param_count = count_lambda_params(lambda);
    Cell* body = lambda->data.lambda.body;
    const char* name = doc->name;

    char buf[256] = "";

    /* Identity: body is De Bruijn index 0, single param → α → α */
    if (cell_is_number(body) && cell_get_number(body) == 0.0 && param_count == 1) {
        doc->type_signature = strdup("α → α");
        return;
    }

    /* Name ends with ? → predicate: ... → 𝔹 */
    if (name) {
        size_t nlen = strlen(name);
        if (nlen > 0 && name[nlen - 1] == '?') {
            for (int i = 0; i < param_count; i++) {
                if (i > 0) strcat(buf, " → ");
                strcat(buf, "α");
            }
            if (param_count > 0) strcat(buf, " → ");
            strcat(buf, "𝔹");
            doc->type_signature = strdup(buf);
            return;
        }
    }

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

    /* Returns only keyword symbols → ... → Symbol */
    if (returns_only_symbols(body)) {
        for (int i = 0; i < param_count; i++) {
            if (i > 0) strcat(buf, " → ");
            strcat(buf, "α");
        }
        if (param_count > 0) strcat(buf, " → ");
        strcat(buf, "Symbol");
        doc->type_signature = strdup(buf);
        return;
    }

    /* Check if body is a conditional */
    if (cell_is_pair(body)) {
        Cell* func = cell_car(body);
        if (cell_is_symbol(func) && strcmp(cell_get_symbol(func), "?") == 0) {
            Cell* args = cell_cdr(body);
            if (args && cell_is_pair(args)) {
                Cell* rest = cell_cdr(args);
                if (rest && cell_is_pair(rest)) {
                    Cell* then_branch = cell_car(rest);
                    /* If then branch is a literal number (not a De Bruijn index), returns number.
                     * De Bruijn indices are small integers < param_count, skip those. */
                    if (then_branch && cell_is_number(then_branch)) {
                        double tv = cell_get_number(then_branch);
                        /* Skip De Bruijn indices (small integers that are param refs) */
                        if (tv < 0 || tv >= param_count || tv != (int)tv) {
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
    }

    /* List recursive pattern: ∅? base case + cons → List → List */
    if (has_nil_base_case(body) && uses_cons(body)) {
        /* Check param count to determine: map/filter pattern or single-list fn */
        if (param_count == 2) {
            /* Higher-order list function or two-arg list function */
            /* Check if first param is used as function */
            bool first_is_fn = false;
            if (cell_is_pair(body)) {
                first_is_fn = param_used_as_function(body, 0);
            }
            if (first_is_fn) {
                doc->type_signature = strdup("(α → β) → List → List");
                return;
            }
            doc->type_signature = strdup("List → List → List");
            return;
        }
        if (param_count == 1) {
            doc->type_signature = strdup("List → List");
            return;
        }
        if (param_count == 3) {
            /* fold pattern */
            doc->type_signature = strdup("(α → β → α) → α → List → α");
            return;
        }
    }

    /* List recursive with nil base, no cons */
    if (has_nil_base_case(body) && !uses_cons(body)) {
        if (param_count == 1) {
            doc->type_signature = strdup("List → ℕ");
            return;
        }
        if (param_count == 3) {
            /* fold pattern: (f acc lst) → acc when nil */
            doc->type_signature = strdup("(α → β → α) → α → List → α");
            return;
        }
        if (param_count == 2) {
            /* Two-arg list function returning accumulated value */
            doc->type_signature = strdup("α → List → α");
            return;
        }
    }

    /* Higher-order: first param used as function → (α → β) → ... */
    if (param_count >= 1) {
        bool first_is_fn = param_used_as_function(body, 0);
        if (first_is_fn) {
            strcpy(buf, "(α → β)");
            for (int i = 1; i < param_count; i++) {
                strcat(buf, " → α");
            }
            strcat(buf, " → β");
            doc->type_signature = strdup(buf);
            return;
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

/* Context passed through description composition */
typedef struct {
    EvalContext* eval_ctx;
    const char** param_names;  /* Real param names from lambda */
    int arity;                 /* Number of params */
    const char* self_name;     /* Name of the function being documented (for recursion detection) */
} DocComposeCtx;

/* Forward declarations */
static char* compose_desc(DocComposeCtx* dctx, Cell* expr, int depth);

/* Get infix operator symbol for binary ops, or NULL if not a binary op */
static const char* get_infix_op(const char* sym) {
    if (strcmp(sym, "⊗") == 0) return "*";
    if (strcmp(sym, "⊕") == 0) return "+";
    if (strcmp(sym, "⊖") == 0) return "-";
    if (strcmp(sym, "⊘") == 0) return "/";
    if (strcmp(sym, "≡") == 0) return "==";
    if (strcmp(sym, "≢") == 0) return "!=";
    if (strcmp(sym, "<") == 0) return "<";
    if (strcmp(sym, ">") == 0) return ">";
    if (strcmp(sym, "≤") == 0) return "<=";
    if (strcmp(sym, "≥") == 0) return ">=";
    if (strcmp(sym, "∧") == 0) return "and";
    if (strcmp(sym, "∨") == 0) return "or";
    if (strcmp(sym, "%") == 0) return "%";
    return NULL;
}

/* Check if a symbol is the :λ-converted marker */
static bool is_lambda_converted(Cell* expr) {
    return cell_is_symbol(expr) &&
           strcmp(cell_get_symbol(expr), ":λ-converted") == 0;
}

/* Compose a conditional expression as Python-like pseudocode */
static char* compose_cond_desc(DocComposeCtx* dctx, Cell* args, int depth) {
    if (!args || !cell_is_pair(args)) return strdup("???");

    Cell* cond = cell_car(args);
    Cell* rest = cell_cdr(args);
    if (!rest || !cell_is_pair(rest)) return strdup("???");

    Cell* then_branch = cell_car(rest);
    Cell* else_rest = cell_cdr(rest);
    if (!else_rest || !cell_is_pair(else_rest)) return strdup("???");

    Cell* else_branch = cell_car(else_rest);

    char* cond_desc = compose_desc(dctx, cond, depth + 1);
    char* then_desc = compose_desc(dctx, then_branch, depth + 1);
    char* else_desc = compose_desc(dctx, else_branch, depth + 1);

    char result[MAX_DESC_LENGTH];

    /* Check if else branch is another conditional (chained) */
    bool else_is_cond = false;
    if (cell_is_pair(else_branch)) {
        Cell* ef = cell_car(else_branch);
        if (cell_is_symbol(ef) && strcmp(cell_get_symbol(ef), "?") == 0) {
            else_is_cond = true;
        }
    }

    if (else_is_cond) {
        /* Chained: "then_desc if cond_desc, elif ..." */
        snprintf(result, sizeof(result), "%s if %s, %s",
                 then_desc, cond_desc, else_desc);
    } else {
        snprintf(result, sizeof(result), "%s if %s else %s",
                 then_desc, cond_desc, else_desc);
    }

    free(cond_desc);
    free(then_desc);
    free(else_desc);
    return strdup(result);
}

/* Recursively compose Python-like pseudocode description from expression AST */
static char* compose_desc(DocComposeCtx* dctx, Cell* expr, int depth) {
    if (depth > MAX_RECURSION_DEPTH) {
        return strdup("...");
    }

    if (!expr) return strdup("nil");

    /* Integer */
    if (cell_is_integer(expr)) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%lld", (long long)cell_get_integer(expr));
        return strdup(buf);
    }

    /* Number - might be De Bruijn index or literal */
    if (cell_is_number(expr)) {
        double num = cell_get_number(expr);
        /* De Bruijn indices are small non-negative integers in lambda bodies */
        if (num >= 0 && num < dctx->arity && num == (int)num) {
            int index = (int)num;
            if (dctx->param_names && index < dctx->arity) {
                return strdup(dctx->param_names[index]);
            }
            /* Fallback if no param names */
            char buf[32];
            snprintf(buf, sizeof(buf), "arg%d", index);
            return strdup(buf);
        } else if (num >= 0 && num < 10 && num == (int)num && !dctx->param_names) {
            /* Legacy fallback for functions without stored param names */
            int index = (int)num;
            char buf[32];
            snprintf(buf, sizeof(buf), "arg%d", index);
            return strdup(buf);
        } else {
            char buf[64];
            if (num == (int)num) {
                snprintf(buf, sizeof(buf), "%.0f", num);
            } else {
                snprintf(buf, sizeof(buf), "%g", num);
            }
            return strdup(buf);
        }
    }

    /* Boolean */
    if (cell_is_bool(expr)) {
        return strdup(cell_get_bool(expr) ? "True" : "False");
    }

    /* Nil */
    if (cell_is_nil(expr)) {
        return strdup("[]");
    }

    /* Symbol */
    if (cell_is_symbol(expr)) {
        const char* sym = cell_get_symbol(expr);
        /* Filter out ∅ as empty list */
        if (strcmp(sym, "∅") == 0) return strdup("[]");
        return strdup(sym);
    }

    /* String */
    if (cell_is_string(expr)) {
        const char* s = cell_get_string(expr);
        char buf[256];
        snprintf(buf, sizeof(buf), "\"%s\"", s);
        return strdup(buf);
    }

    /* Pair (function application) */
    if (cell_is_pair(expr)) {
        Cell* func = cell_car(expr);
        Cell* args = cell_cdr(expr);

        if (cell_is_symbol(func)) {
            const char* fn = cell_get_symbol(func);

            /* :λ-converted — suppress marker, describe inner body with merged scope */
            if (strcmp(fn, ":λ-converted") == 0) {
                /* (:λ-converted (params) body) — describe body with extended param scope */
                if (args && cell_is_pair(args)) {
                    Cell* inner_params = cell_car(args);
                    Cell* inner_rest = cell_cdr(args);
                    if (inner_rest && cell_is_pair(inner_rest)) {
                        /* Build merged param names: [inner_params..., outer_params...] */
                        int inner_count = list_length(inner_params);
                        int total = inner_count + dctx->arity;
                        const char** merged = (const char**)malloc(total * sizeof(char*));
                        /* Inner params first (De Bruijn 0..inner_count-1) */
                        Cell* p = inner_params;
                        for (int i = 0; i < inner_count && cell_is_pair(p); i++) {
                            Cell* ps = cell_car(p);
                            merged[i] = cell_is_symbol(ps) ? cell_get_symbol(ps) : "?";
                            p = cell_cdr(p);
                        }
                        /* Outer params next (De Bruijn inner_count..total-1) */
                        for (int i = 0; i < dctx->arity; i++) {
                            merged[inner_count + i] = dctx->param_names ? dctx->param_names[i] : "?";
                        }
                        DocComposeCtx inner_dctx;
                        inner_dctx.eval_ctx = dctx->eval_ctx;
                        inner_dctx.param_names = merged;
                        inner_dctx.arity = total;
                        inner_dctx.self_name = dctx->self_name;
                        char* result = compose_desc(&inner_dctx, cell_car(inner_rest), depth);
                        free(merged);
                        return result;
                    }
                }
                return strdup("lambda(...)");
            }

            /* Quote: (⌜ val) → literal value */
            if (strcmp(fn, "⌜") == 0) {
                if (args && cell_is_pair(args)) {
                    Cell* qv = cell_car(args);
                    if (cell_is_number(qv)) {
                        char buf[64];
                        double num = cell_get_number(qv);
                        if (num == (int)num)
                            snprintf(buf, sizeof(buf), "%.0f", num);
                        else
                            snprintf(buf, sizeof(buf), "%g", num);
                        return strdup(buf);
                    }
                    if (cell_is_symbol(qv)) {
                        return strdup(cell_get_symbol(qv));
                    }
                    if (cell_is_string(qv)) {
                        char buf[256];
                        snprintf(buf, sizeof(buf), "\"%s\"", cell_get_string(qv));
                        return strdup(buf);
                    }
                    if (cell_is_nil(qv)) return strdup("[]");
                    if (cell_is_bool(qv)) return strdup(cell_get_bool(qv) ? "True" : "False");
                    return compose_desc(dctx, qv, depth + 1);
                }
                return strdup("(quoted)");
            }

            /* Sequence: (⪢ e1 e2 ...) → "e1; e2; ..." */
            if (strcmp(fn, "⪢") == 0) {
                char result[MAX_DESC_LENGTH] = "";
                Cell* arg_list = args;
                bool first = true;
                while (arg_list && cell_is_pair(arg_list)) {
                    char* d = compose_desc(dctx, cell_car(arg_list), depth + 1);
                    if (!first) strncat(result, "; ", sizeof(result) - strlen(result) - 1);
                    strncat(result, d, sizeof(result) - strlen(result) - 1);
                    free(d);
                    first = false;
                    arg_list = cell_cdr(arg_list);
                }
                return strdup(result);
            }

            /* Conditional: (? cond then else) */
            if (strcmp(fn, "?") == 0) {
                return compose_cond_desc(dctx, args, depth);
            }

            /* Negation: (¬ x) → "not x" */
            if (strcmp(fn, "¬") == 0) {
                if (args && cell_is_pair(args)) {
                    char* operand = compose_desc(dctx, cell_car(args), depth + 1);
                    char result[MAX_DESC_LENGTH];
                    snprintf(result, sizeof(result), "not %s", operand);
                    free(operand);
                    return strdup(result);
                }
            }

            /* Binary infix operators */
            const char* infix = get_infix_op(fn);
            if (infix && args && cell_is_pair(args)) {
                Cell* left = cell_car(args);
                Cell* rest = cell_cdr(args);
                if (rest && cell_is_pair(rest)) {
                    Cell* right = cell_car(rest);
                    char* l = compose_desc(dctx, left, depth + 1);
                    char* r = compose_desc(dctx, right, depth + 1);
                    char result[MAX_DESC_LENGTH];
                    snprintf(result, sizeof(result), "%s %s %s", l, infix, r);
                    free(l);
                    free(r);
                    return strdup(result);
                }
            }

            /* Pair construction: (⟨⟩ a b) → "cons(a, b)" */
            if (strcmp(fn, "⟨⟩") == 0 && args && cell_is_pair(args)) {
                Cell* head = cell_car(args);
                Cell* tail_rest = cell_cdr(args);
                if (tail_rest && cell_is_pair(tail_rest)) {
                    char* h = compose_desc(dctx, head, depth + 1);
                    char* t = compose_desc(dctx, cell_car(tail_rest), depth + 1);
                    char result[MAX_DESC_LENGTH];
                    snprintf(result, sizeof(result), "cons(%s, %s)", h, t);
                    free(h);
                    free(t);
                    return strdup(result);
                }
            }

            /* Effect operations: special descriptions */
            if (strcmp(fn, "←?") == 0) return strdup("receive()");
            if (strcmp(fn, "→!") == 0) {
                if (args && cell_is_pair(args)) {
                    char* target = compose_desc(dctx, cell_car(args), depth + 1);
                    Cell* msg_rest = cell_cdr(args);
                    if (msg_rest && cell_is_pair(msg_rest)) {
                        char* msg = compose_desc(dctx, cell_car(msg_rest), depth + 1);
                        char result[MAX_DESC_LENGTH];
                        snprintf(result, sizeof(result), "send(%s, %s)", target, msg);
                        free(target);
                        free(msg);
                        return strdup(result);
                    }
                    free(target);
                }
                return strdup("send(...)");
            }
            if (strcmp(fn, "↯") == 0) return strdup("raise(...)");
            if (strcmp(fn, "⟪") == 0) return strdup("perform(...)");

            /* General function call: f(arg1, arg2, ...) */
            /* Check if this is a self-recursive call */
            bool is_self = (dctx->self_name && strcmp(fn, dctx->self_name) == 0);

            char args_buf[1024] = "";
            Cell* arg_list = args;
            bool first = true;
            while (arg_list && cell_is_pair(arg_list)) {
                Cell* arg_expr = cell_car(arg_list);
                /* Skip :λ-converted appearing as an argument (suppress) */
                if (is_lambda_converted(arg_expr)) {
                    arg_list = cell_cdr(arg_list);
                    continue;
                }
                char* d = compose_desc(dctx, arg_expr, depth + 1);
                if (!first) strncat(args_buf, ", ", sizeof(args_buf) - strlen(args_buf) - 1);
                strncat(args_buf, d, sizeof(args_buf) - strlen(args_buf) - 1);
                free(d);
                first = false;
                arg_list = cell_cdr(arg_list);
            }

            char result[MAX_DESC_LENGTH];
            snprintf(result, sizeof(result), "%s(%s)", fn, args_buf);
            return strdup(result);
        }

        /* Non-symbol function (e.g., a parameter used as a function) */
        char* func_desc = compose_desc(dctx, func, depth + 1);

        char args_buf[1024] = "";
        Cell* arg_list = args;
        bool first = true;
        while (arg_list && cell_is_pair(arg_list)) {
            char* d = compose_desc(dctx, cell_car(arg_list), depth + 1);
            if (!first) strncat(args_buf, ", ", sizeof(args_buf) - strlen(args_buf) - 1);
            strncat(args_buf, d, sizeof(args_buf) - strlen(args_buf) - 1);
            free(d);
            first = false;
            arg_list = cell_cdr(arg_list);
        }

        char result[MAX_DESC_LENGTH];
        snprintf(result, sizeof(result), "%s(%s)", func_desc, args_buf);
        free(func_desc);
        return strdup(result);
    }

    /* Lambda */
    if (cell_is_lambda(expr)) {
        return strdup("lambda(...)");
    }

    return strdup("???");
}

/* Noisy dependency symbols to filter out (control flow, not meaningful deps) */
static bool is_noise_dep(const char* name) {
    if (!name) return true;
    /* Control flow */
    if (strcmp(name, "?") == 0 || strcmp(name, "⌜") == 0 ||
        strcmp(name, "⪢") == 0 || strcmp(name, "≔") == 0 ||
        strcmp(name, ":λ-converted") == 0 || strcmp(name, "⌜⌝") == 0)
        return true;
    /* Nil literal */
    if (strcmp(name, "∅") == 0) return true;
    /* Keyword symbols (like :hot, :cold) — values, not deps */
    if (name[0] == ':') return true;
    return false;
}

/* Count branch nodes (? conditionals) in expression */
static int count_branches(Cell* expr) {
    if (!expr || !cell_is_pair(expr)) return 0;
    Cell* func = cell_car(expr);
    int count = 0;
    if (cell_is_symbol(func) && strcmp(cell_get_symbol(func), "?") == 0) {
        count = 1;
        /* Count in else branch for chained conditionals */
        Cell* rest = cell_cdr(expr);
        if (rest && cell_is_pair(rest)) {
            Cell* rest2 = cell_cdr(rest);
            if (rest2 && cell_is_pair(rest2)) {
                Cell* rest3 = cell_cdr(rest2);
                if (rest3 && cell_is_pair(rest3)) {
                    count += count_branches(cell_car(rest3));
                }
            }
        }
    }
    /* Also check sub-expressions */
    Cell* iter = expr;
    while (cell_is_pair(iter)) {
        if (cell_is_pair(cell_car(iter))) {
            count += count_branches(cell_car(iter));
        }
        iter = cell_cdr(iter);
    }
    return count;
}

/* Check if body contains a self-recursive call */
static bool has_self_call(Cell* expr, const char* self_name) {
    if (!expr || !self_name) return false;
    if (cell_is_symbol(expr) && strcmp(cell_get_symbol(expr), self_name) == 0) return true;
    if (cell_is_pair(expr)) {
        return has_self_call(cell_car(expr), self_name) ||
               has_self_call(cell_cdr(expr), self_name);
    }
    return false;
}

/* Check if body contains effect operations */
static bool has_effects(Cell* expr) {
    if (!expr) return false;
    if (cell_is_symbol(expr)) {
        const char* s = cell_get_symbol(expr);
        return (strcmp(s, "↯") == 0 || strcmp(s, "⟪") == 0 ||
                strcmp(s, "←?") == 0 || strcmp(s, "→!") == 0 ||
                strcmp(s, "⟪⟫") == 0 || strcmp(s, "⟪↺⟫") == 0);
    }
    if (cell_is_pair(expr)) {
        return has_effects(cell_car(expr)) || has_effects(cell_cdr(expr));
    }
    return false;
}

/* Check if a De Bruijn indexed arg is used in function position (higher-order) */
static bool param_used_as_function(Cell* expr, int param_index) {
    if (!expr || !cell_is_pair(expr)) return false;
    Cell* func = cell_car(expr);
    if (cell_is_number(func)) {
        double num = cell_get_number(func);
        if (num == param_index) return true;
    }
    /* Recurse into sub-expressions */
    Cell* iter = expr;
    while (cell_is_pair(iter)) {
        if (param_used_as_function(cell_car(iter), param_index)) return true;
        iter = cell_cdr(iter);
    }
    return false;
}

/* Generate flow info string from body analysis */
static char* generate_flow_info(Cell* body, const char* self_name, int arity) {
    char buf[512] = "";
    int branches = count_branches(body);
    bool recursive = has_self_call(body, self_name);
    bool effects = has_effects(body);
    bool higher_order = false;

    for (int i = 0; i < arity; i++) {
        if (param_used_as_function(body, i)) {
            higher_order = true;
            break;
        }
    }

    bool need_sep = false;

    if (recursive) {
        strcat(buf, "self-recursive");
        need_sep = true;
    }

    if (branches > 0) {
        if (need_sep) strcat(buf, ", ");
        char tmp[64];
        if (branches == 1)
            snprintf(tmp, sizeof(tmp), "1 branch");
        else
            snprintf(tmp, sizeof(tmp), "%d branches", branches);
        strcat(buf, tmp);
        need_sep = true;
    }

    if (higher_order) {
        if (need_sep) strcat(buf, ", ");
        strcat(buf, "higher-order");
        need_sep = true;
    }

    if (effects) {
        if (need_sep) strcat(buf, ", ");
        strcat(buf, "effectful");
        need_sep = true;
    }

    if (buf[0] == '\0') return NULL;
    return strdup(buf);
}

/* Generate description from body using recursive composition */
static void doc_generate_description(EvalContext* ctx, FunctionDoc* doc, Cell* body,
                                     const char** param_names, int arity, const char* self_name) {
    /* Extract dependencies from the body */
    SymbolSet* params = symbol_set_create();
    DepList* deps = dep_list_create();
    extract_dependencies(body, params, deps);

    /* Filter noisy dependencies */
    DepList* filtered = dep_list_create();
    for (size_t i = 0; i < deps->count; i++) {
        if (!is_noise_dep(deps->names[i])) {
            dep_list_add(filtered, deps->names[i]);
        }
    }
    /* Free original deps */
    for (size_t i = 0; i < deps->count; i++) free(deps->names[i]);
    free(deps->names);
    free(deps);

    /* Store filtered dependencies */
    doc->dependency_count = filtered->count;
    doc->dependencies = filtered->names;
    free(filtered);

    /* Store param info on doc */
    doc->param_names = param_names;
    doc->arity = arity;

    /* Set up composition context */
    DocComposeCtx dctx;
    dctx.eval_ctx = ctx;
    dctx.param_names = param_names;
    dctx.arity = arity;
    dctx.self_name = self_name;

    /* Special pattern: identity (body == De Bruijn index 0) */
    if (cell_is_number(body) && cell_get_number(body) == 0.0 && arity == 1) {
        const char* pn = (param_names && arity > 0) ? param_names[0] : "x";
        char buf[128];
        snprintf(buf, sizeof(buf), "return %s unchanged", pn);
        doc->description = strdup(buf);
    }
    /* Special pattern: constant (body is a literal with no param refs) */
    else if ((cell_is_bool(body) || cell_is_nil(body) || cell_is_string(body)) && arity > 0) {
        char* val = compose_desc(&dctx, body, 0);
        char buf[256];
        snprintf(buf, sizeof(buf), "always returns %s", val);
        doc->description = strdup(buf);
        free(val);
    }
    else {
        doc->description = compose_desc(&dctx, body, 0);
    }

    /* Generate flow info */
    doc->flow_info = generate_flow_info(body, self_name, arity);

    symbol_set_free(params);
}

/* ============ End Documentation API ============ */

/* Create new evaluation context */
EvalContext* eval_context_new(void) {
    EvalContext* ctx = (EvalContext*)malloc(sizeof(EvalContext));
    ctx->env = cell_nil();
    ctx->global_env = ctx->env;
    ctx->primitives = primitives_init();
    ctx->user_docs = NULL;  /* Initialize doc list */
    ctx->type_registry = cell_nil();  /* Initialize type registry */
    ctx->effect_registry = cell_nil();  /* Initialize effect registry */
    ctx->reductions_left = 0;    /* 0 = disabled (REPL/non-actor eval) */
    ctx->continuation = NULL;
    ctx->continuation_env = NULL;
    macro_init();  /* Initialize macro system */
    return ctx;
}

/* Free documentation list */
static void doc_free_all(FunctionDoc* head) {
    while (head) {
        FunctionDoc* next = head->next;
        free(head->name);
        free(head->summary);
        free(head->description);
        free(head->flow_info);
        free(head->type_signature);
        /* param_names are borrowed from lambda, not freed here */
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
    cell_release(ctx->effect_registry);
    doc_free_all(ctx->user_docs);
    free(ctx);
}

/* Lookup variable in environment */
Cell* eval_lookup_env(Cell* env, const char* name) {
    if (UNLIKELY(g_profile_enabled)) g_prof_env_lookups++;
    while (cell_is_pair(env)) {
        if (UNLIKELY(g_profile_enabled)) g_prof_env_steps++;
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
    if (UNLIKELY(g_profile_enabled)) g_prof_env_lookups++;

    /* Fast path: no local bindings (pattern match etc.) — skip alist walk */
    if (LIKELY(ctx->env == ctx->global_env)) {
        /* O(1) global table lookup by intern sym_id */
        InternResult r = intern(name);
        Cell* val = g_global_table[r.id];
        if (val) {
            if (UNLIKELY(g_profile_enabled)) g_prof_env_steps++;
            cell_retain(val);
            return val;
        }
        /* O(1) primitive table */
        return primitives_lookup(ctx->primitives, name);
    }

    /* Slow path: local bindings exist — must walk alist first for shadowing */
    Cell* val = eval_lookup_env(ctx->env, name);
    if (val) return val;

    /* Then global table */
    InternResult r = intern(name);
    val = g_global_table[r.id];
    if (val) {
        if (UNLIKELY(g_profile_enabled)) g_prof_env_steps++;
        cell_retain(val);
        return val;
    }

    /* Then primitives */
    return primitives_lookup(ctx->primitives, name);
}

/* Define global binding */
void eval_define(EvalContext* ctx, const char* name, Cell* value) {
    /* Track this symbol in module registry */
    const char* current_module = module_get_current_loading();
    if (current_module != NULL) {
        /* Loading a module file - register in that module */
        module_registry_add_symbol(current_module, name);
        /* Store in module's local environment */
        module_registry_define(current_module, name, value);
        /* Only promote to global if exported (or no exports declared) */
        if (module_registry_is_exported(current_module, name)) {
            Cell* sym = cell_symbol(name);
            Cell* binding = cell_cons(sym, value);
            ctx->env = cell_cons(binding, ctx->env);
        }
    } else {
        /* REPL definition - always global */
        Cell* sym = cell_symbol(name);
        Cell* binding = cell_cons(sym, value);
        ctx->env = cell_cons(binding, ctx->env);
        module_registry_add_symbol("<repl>", name);
    }

    /* Keep global_env in sync so eval_lookup fast-path works */
    ctx->global_env = ctx->env;

    /* O(1) global table: store by intern sym_id */
    {
        InternResult r = intern(name);
        Cell* old = g_global_table[r.id];
        if (value) cell_retain(value);
        g_global_table[r.id] = value;
        if (old) cell_release(old);
    }

    /* Generate documentation if value is a lambda */
    if (value && cell_is_lambda(value)) {
        FunctionDoc* doc = doc_create(name);

        /* Generate docs - graceful degradation on failure */
        doc_generate_description(ctx, doc, value->data.lambda.body,
                                 value->data.lambda.param_names,
                                 value->data.lambda.arity, name);
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
        if (doc->flow_info) {
            printf("   Flow: %s\n", doc->flow_info);
        }
        if (doc->dependency_count > 0) {
            printf("   Deps: ");
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
    /* Fast path: flat vector env — O(1) direct index */
    if (LIKELY(env->type == CELL_VECTOR)) {
        Cell* val = cell_vector_get(env, (uint32_t)index);
        if (val) cell_retain(val);
        return val;
    }

    /* Legacy path: linked-list env with :__indexed__ marker */
    Cell* current = env;
    for (int i = 0; i <= index; i++) {
        if (!cell_is_pair(current)) {
            return NULL;
        }

        Cell* elem = cell_car(current);

        /* Skip marker */
        if (cell_is_symbol(elem) && elem->sym_id == SYM_ID_INDEXED) {
                current = cell_cdr(current);
                continue;  /* Don't count this as an index */
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

/* Extend environment with argument values — flat vector env.
 * Creates a single CELL_VECTOR: [arg₀, arg₁, ..., parent_val₀, parent_val₁, ...]
 * De Bruijn index i → vector[i], O(1) lookup. */
Cell* extend_env(Cell* env, Cell* args) {
    /* Count args */
    int arity = 0;
    Cell* a = args;
    while (cell_is_pair(a)) { arity++; a = cell_cdr(a); }

    /* Parent size */
    uint32_t parent_size = 0;
    if (env->type == CELL_VECTOR) {
        parent_size = cell_vector_size(env);
    } else if (!cell_is_nil(env)) {
        /* Legacy linked-list env — count elements (excluding marker) */
        Cell* cur = env;
        while (cell_is_pair(cur)) {
            Cell* elem = cell_car(cur);
            if (!(cell_is_symbol(elem) && elem->sym_id == SYM_ID_INDEXED)) {
                parent_size++;
            }
            cur = cell_cdr(cur);
        }
    }

    uint32_t total = (uint32_t)arity + parent_size;
    Cell* vec = cell_vector_new(total);

    /* Push args (index 0 = first arg = De Bruijn 0) */
    a = args;
    while (cell_is_pair(a)) {
        cell_vector_push(vec, cell_car(a));
        a = cell_cdr(a);
    }

    /* Copy parent env values */
    if (env->type == CELL_VECTOR) {
        Cell** buf = (env->data.vector.capacity <= 4)
            ? env->data.vector.sbo : env->data.vector.heap;
        for (uint32_t i = 0; i < parent_size; i++) {
            cell_vector_push(vec, buf[i]);
        }
    } else if (!cell_is_nil(env)) {
        /* Legacy linked-list parent */
        Cell* cur = env;
        while (cell_is_pair(cur)) {
            Cell* elem = cell_car(cur);
            if (!(cell_is_symbol(elem) && elem->sym_id == SYM_ID_INDEXED)) {
                cell_vector_push(vec, elem);
            }
            cur = cell_cdr(cur);
        }
    }

    return vec;
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

/* Forward declaration */
const char** extract_param_names_with_constraints(Cell* params, int* out_count, Cell** out_constraints);

/* Extract parameter names from list, handling ⊳ markers and : annotations.
 * (⊳ T) — skip ⊳, use T as param name (generic type param marker)
 * (x : T) — use x, skip : and T (type annotation)
 * Returns extracted names array; sets *out_count to actual param count. */
const char** extract_param_names_counted(Cell* params, int* out_count) {
    return extract_param_names_with_constraints(params, out_count, NULL);
}

/* Extract param names and optionally collect trait constraints.
 * If out_constraints is non-NULL, builds a list of (param_idx . :TraitName) pairs.
 * Syntax: (⊳ T :Showable) — :Showable constrains T (colon-prefixed symbol after T) */
const char** extract_param_names_with_constraints(Cell* params, int* out_count, Cell** out_constraints) {
    /* First pass: count actual params */
    int count = 0;
    Cell* current = params;
    while (cell_is_pair(current)) {
        Cell* param = cell_car(current);
        if (cell_is_symbol(param) && param->sym_id == SYM_ID_GENERIC_PARAM) {
            /* ⊳ marker — skip it, next element is the actual param */
            current = cell_cdr(current);
            if (cell_is_pair(current)) {
                count++;
                current = cell_cdr(current);
                /* Skip optional constraint (colon-prefixed symbol) */
                if (cell_is_pair(current)) {
                    Cell* maybe_constraint = cell_car(current);
                    if (cell_is_symbol(maybe_constraint)) {
                        const char* cs = cell_get_symbol(maybe_constraint);
                        if (cs[0] == ':' && cs[1] >= 'A' && cs[1] <= 'Z') {
                            current = cell_cdr(current);
                        }
                    }
                }
            }
        } else if (cell_is_symbol(param)) {
            count++;
            current = cell_cdr(current);
            /* Check for : annotation */
            if (cell_is_pair(current)) {
                Cell* maybe_colon = cell_car(current);
                if (cell_is_symbol(maybe_colon) &&
                    strcmp(cell_get_symbol(maybe_colon), ":") == 0) {
                    current = cell_cdr(current); /* skip : */
                    if (cell_is_pair(current)) {
                        current = cell_cdr(current); /* skip type */
                    }
                }
            }
        } else {
            count++;
            current = cell_cdr(current);
        }
    }

    const char** names = (const char**)malloc(count * sizeof(char*));
    *out_count = count;

    /* Second pass: extract names and constraints */
    Cell* constraints = cell_nil();
    current = params;
    int i = 0;
    while (cell_is_pair(current) && i < count) {
        Cell* param = cell_car(current);
        if (cell_is_symbol(param) && param->sym_id == SYM_ID_GENERIC_PARAM) {
            current = cell_cdr(current);
            if (cell_is_pair(current)) {
                names[i] = cell_get_symbol(cell_car(current));
                current = cell_cdr(current);
                /* Check for constraint */
                if (out_constraints && cell_is_pair(current)) {
                    Cell* maybe_constraint = cell_car(current);
                    if (cell_is_symbol(maybe_constraint)) {
                        const char* cs = cell_get_symbol(maybe_constraint);
                        if (cs[0] == ':' && cs[1] >= 'A' && cs[1] <= 'Z') {
                            Cell* pair = cell_cons(cell_number((double)i), maybe_constraint);
                            constraints = cell_cons(pair, constraints);
                            cell_release(pair);
                            current = cell_cdr(current);
                        }
                    }
                }
                i++;
            }
        } else if (cell_is_symbol(param)) {
            names[i++] = cell_get_symbol(param);
            current = cell_cdr(current);
            if (cell_is_pair(current)) {
                Cell* maybe_colon = cell_car(current);
                if (cell_is_symbol(maybe_colon) &&
                    strcmp(cell_get_symbol(maybe_colon), ":") == 0) {
                    current = cell_cdr(current);
                    if (cell_is_pair(current)) {
                        current = cell_cdr(current);
                    }
                }
            }
        } else {
            names[i++] = cell_get_symbol(param);
            current = cell_cdr(current);
        }
    }

    if (out_constraints) {
        if (cell_is_nil(constraints)) {
            *out_constraints = NULL;
        } else {
            *out_constraints = constraints;
            cell_retain(constraints);
        }
    }
    cell_release(constraints);

    return names;
}

/* Legacy wrapper for simple params */
const char** extract_param_names(Cell* params) {
    int count;
    return extract_param_names_counted(params, &count);
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
            /* Check for unquote: ~ or unquote */
            if (first->sym_id == SYM_ID_UNQUOTE || first->sym_id == SYM_ID_UNQUOTE_ALT) {
                /* Evaluate the unquoted expression */
                Cell* rest = cell_cdr(expr);
                if (cell_is_nil(rest)) {
                    return cell_error_at("quasiquote-error",
                        cell_symbol("unquote-requires-argument"), expr->span);
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
    /* Fast path: flat vector env */
    if (env->type == CELL_VECTOR) return true;
    if (cell_is_nil(env)) return true;  /* Empty env can be either */
    if (!cell_is_pair(env)) return false;

    /* Legacy: walk the environment looking for the indexed marker */
    Cell* current = env;
    while (cell_is_pair(current)) {
        Cell* elem = cell_car(current);
        if (cell_is_symbol(elem) && elem->sym_id == SYM_ID_INDEXED) {
                return true;  /* Found marker - this is indexed */
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
    if (UNLIKELY(g_profile_enabled)) g_prof_eval_steps++;
    /* BEAM-style reduction counting: yield when budget exhausted */
    if (ctx->reductions_left > 0) {
        if (--ctx->reductions_left <= 0) {
            /* Save continuation for scheduler to resume */
            ctx->continuation = expr;
            ctx->continuation_env = env;
            if (owned_expr) cell_release(owned_expr);
            if (owned_env) cell_release(owned_env);
            return CELL_YIELD_SENTINEL;
        }
    }
    /* Note: owned resources released right before setting new ones */
    /* Coverage tracking: mark span of every evaluated expression */
    if (expr && g_coverage_bitmap) {
        coverage_mark(expr->span);
    }
    /* Macro expansion pass - expand macros before evaluation */
    if (cell_is_pair(expr)) {
        Cell* first = cell_car(expr);
        if (cell_is_symbol(first) && macro_is_macro_call(expr)) {
            /* This is a macro call - expand it */
            Cell* expanded = macro_expand(expr, ctx);
            if (UNLIKELY(cell_is_error(expanded))) {
                error_stamp_return(expanded, expr->span.inline_span.lo);
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

    /* Native integers are always self-evaluating (never De Bruijn indices) */
    if (cell_is_integer(expr)) {
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

        /* Check for dot-qualified access (e.g., m.sin) */
        const char* dot = strchr(name, '.');
        if (dot && dot != name && dot[1] != '\0') {
            /* Split at first dot: prefix.suffix */
            size_t prefix_len = (size_t)(dot - name);
            char prefix[256];
            if (prefix_len >= sizeof(prefix)) prefix_len = sizeof(prefix) - 1;
            memcpy(prefix, name, prefix_len);
            prefix[prefix_len] = '\0';
            const char* suffix = dot + 1;

            /* Resolve alias to module path */
            const char* module_path = module_registry_resolve_alias(prefix);
            if (module_path) {
                /* Check export boundary */
                if (!module_registry_is_exported(module_path, suffix)) {
                    Cell* var_name = cell_symbol(name);
                    Cell* result = cell_error_at("not-exported", var_name, expr->span);
                    cell_release(var_name);
                    return result;
                }
                Cell* value = module_registry_lookup(module_path, suffix);
                if (value) return value;
                /* Fall through to normal lookup if not found in module */
            }
            /* If no alias matched, fall through to normal lookup */
        }

        /* Regular symbols: variable lookup */
        Cell* value = eval_lookup(ctx, name);
        if (value == NULL) {
            Cell* var_name = cell_symbol(name);
            Cell* result = cell_error_at("undefined-variable", var_name, expr->span);
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
            uint16_t id = first->sym_id;

            /* ⌜ - quote */
            if (id == SYM_ID_QUOTE) {
                Cell* arg = cell_car(rest);
                cell_retain(arg);
                return arg;
            }

            /* ⌞̃ - quasiquote (quote with unquote support) */
            if (id == SYM_ID_QUASIQUOTE || id == SYM_ID_QUASIQUOTE_ALT) {
                Cell* arg = cell_car(rest);
                return eval_quasiquote(ctx, env, arg);
            }

            /* ⧉⊜ - pattern-based macro (macro-rules) */
            if (id == SYM_ID_MACRO_RULES) {
                /* Syntax: (⧉⊜ name ((pattern) template) ...) */
                Cell* name = cell_car(rest);
                Cell* clauses = cell_cdr(rest);

                if (!cell_is_symbol(name)) {
                    return cell_error_at("macro-name-not-symbol", name, expr->span);
                }
                const char* name_str = cell_get_symbol(name);

                /* Register pattern-based macro */
                macro_define_pattern(name_str, clauses);

                /* Return name as symbol */
                cell_retain(name);
                return name;
            }

            /* ⧉ - macro define (3 args) OR arity (1 arg) */
            if (id == SYM_ID_MACRO) {
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
                        return cell_error_at("macro-name-not-symbol", name, expr->span);
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
            if (id == SYM_ID_DEFINE) {
                Cell* name = cell_car(rest);
                Cell* value_expr = cell_car(cell_cdr(rest));

                assert(cell_is_symbol(name));
                const char* name_str = cell_get_symbol(name);

                /* Check if value_expr is a lambda - enable named recursion */
                bool is_lambda = false;
                if (cell_is_pair(value_expr)) {
                    Cell* first_val = cell_car(value_expr);
                    if (cell_is_symbol(first_val)) {
                        is_lambda = (first_val->sym_id == SYM_ID_LAMBDA);
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

            /* ∈ - type declaration (special form: first arg is NOT evaluated) */
            if (id == SYM_ID_TYPE_DECL) {
                /* Parse: (∈ name type-expr) */
                Cell* name = cell_car(rest);
                Cell* type_expr = cell_car(cell_cdr(rest));

                if (!cell_is_symbol(name)) {
                    return cell_error_at("∈ requires symbol as first argument", name, expr->span);
                }

                /* Evaluate the type expression */
                Cell* type = eval_internal(ctx, env, type_expr);
                if (UNLIKELY(cell_is_error(type))) {
                    error_stamp_return(type, expr->span.inline_span.lo);
                    return type;
                }

                /* Store in type annotation registry (primitives.c has the registry) */
                /* Call the primitive with the name (as symbol) and type */
                extern Cell* prim_type_declare(Cell*);
                Cell* args = cell_cons(name, cell_cons(type, cell_nil()));
                cell_retain(name);
                Cell* result = prim_type_declare(args);
                cell_release(args);
                return result;
            }

            /* ∈? - type query (special form: arg is NOT evaluated) */
            if (id == SYM_ID_TYPE_CHECK) {
                /* Parse: (∈? name) */
                Cell* name = cell_car(rest);

                if (!cell_is_symbol(name)) {
                    return cell_error_at("∈? requires symbol", name, expr->span);
                }

                /* Query from type annotation registry */
                extern Cell* prim_type_query(Cell*);
                Cell* args = cell_cons(name, cell_nil());
                cell_retain(name);
                Cell* result = prim_type_query(args);
                cell_release(args);
                return result;
            }

            /* ∈✓ - type validation (special form: arg is NOT evaluated) */
            if (id == SYM_ID_TYPE_VALIDATE) {
                /* Parse: (∈✓ name) */
                Cell* name = cell_car(rest);

                if (!cell_is_symbol(name)) {
                    return cell_error_at("∈✓ requires symbol", name, expr->span);
                }

                /* Validate binding against declared type */
                extern Cell* prim_type_validate(Cell*);
                Cell* args = cell_cons(name, cell_nil());
                cell_retain(name);
                Cell* result = prim_type_validate(args);
                cell_release(args);
                return result;
            }

            /* ∈⍜ - Deep type inference (special form for named bindings)
             * For symbols: checks annotation registry first, then infers from value.
             * For expressions: evaluates first, then infers from result.
             */
            if (id == SYM_ID_TYPE_INFER) {
                Cell* expr = cell_car(rest);
                extern Cell* prim_type_infer(Cell*);
                extern Cell* prim_type_query(Cell*);

                if (cell_is_symbol(expr)) {
                    /* Check annotation registry first */
                    Cell* query_args = cell_cons(expr, cell_nil());
                    cell_retain(expr);
                    Cell* declared = prim_type_query(query_args);
                    cell_release(query_args);

                    if (!cell_is_nil(declared)) {
                        return declared;
                    }
                    cell_release(declared);

                    /* Fall back to inferring from current value */
                    const char* sym_name = cell_get_symbol(expr);
                    Cell* value = eval_lookup(ctx, sym_name);
                    if (value) {
                        Cell* infer_args = cell_cons(value, cell_nil());
                        Cell* inferred = prim_type_infer(infer_args);
                        cell_release(infer_args);
                        cell_release(value);
                        return inferred;
                    }

                    /* Unknown symbol → ⊤ */
                    extern Cell* make_type_struct(const char* kind, Cell* fields);
                    return make_type_struct(":any", cell_nil());
                }

                /* Non-symbol: evaluate then infer.
                 * NOTE: Do NOT propagate errors - we want to infer their type too. */
                Cell* evaled = eval_internal(ctx, env, expr);

                Cell* infer_args = cell_cons(evaled, cell_nil());
                Cell* result = prim_type_infer(infer_args);
                cell_release(infer_args);
                cell_release(evaled);
                return result;
            }

            /* ∈⊢ - type-check function application (special form: first arg NOT evaluated) */
            if (id == SYM_ID_TYPE_ASSERT) {
                /* Parse: (∈⊢ fn-name arg1 arg2 ...) */
                Cell* fn_name = cell_car(rest);
                Cell* arg_exprs = cell_cdr(rest);

                if (!cell_is_symbol(fn_name)) {
                    return cell_error_at("∈⊢ requires symbol as first argument", fn_name, expr->span);
                }

                /* Evaluate the arguments (build list in reverse, then reverse) */
                Cell* eval_args_rev = cell_nil();
                Cell* current = arg_exprs;
                while (cell_is_pair(current)) {
                    Cell* arg = eval_internal(ctx, env, cell_car(current));
                    if (UNLIKELY(cell_is_error(arg))) {
                        cell_release(eval_args_rev);
                        error_stamp_return(arg, expr->span.inline_span.lo);
                        return arg;
                    }
                    eval_args_rev = cell_cons(arg, eval_args_rev);
                    current = cell_cdr(current);
                }

                /* Reverse the list to get correct order */
                Cell* eval_args = cell_nil();
                current = eval_args_rev;
                while (cell_is_pair(current)) {
                    Cell* elem = cell_car(current);
                    cell_retain(elem);
                    eval_args = cell_cons(elem, eval_args);
                    current = cell_cdr(current);
                }
                cell_release(eval_args_rev);

                /* Call the primitive with fn_name and evaluated args */
                extern Cell* prim_type_check_apply(Cell*);
                Cell* call_args = cell_cons(fn_name, eval_args);
                cell_retain(fn_name);
                Cell* result = prim_type_check_apply(call_args);
                cell_release(call_args);
                return result;
            }

            /* ∈⍜* - Expression type inference (special form: expr NOT evaluated)
             * Infers the type of an expression without evaluating it.
             * Uses primitive signatures, annotation registry, and recursive analysis.
             */
            if (id == SYM_ID_TYPE_INFER_ALL) {
                Cell* expr = cell_car(rest);
                extern Cell* prim_type_infer(Cell*);
                extern Cell* prim_type_prim_sig(Cell*);
                extern Cell* prim_type_query(Cell*);
                extern Cell* make_type_struct(const char* kind, Cell* fields);
                extern bool is_type_struct(Cell* c);
                extern const char* get_type_kind(Cell* type);
                extern bool types_equal(Cell* t1, Cell* t2);

                /* Literal number */
                if (cell_is_number(expr)) {
                    return make_type_struct(":int", cell_nil());
                }

                /* Literal boolean */
                if (cell_is_bool(expr)) {
                    return make_type_struct(":bool", cell_nil());
                }

                /* String literal */
                if (cell_is_string(expr)) {
                    return make_type_struct(":string", cell_nil());
                }

                /* Nil */
                if (cell_is_nil(expr)) {
                    return make_type_struct(":nil", cell_nil());
                }

                /* Symbol → check annotation registry, then infer from current value */
                if (cell_is_symbol(expr)) {
                    /* Check annotation registry first */
                    Cell* query_args = cell_cons(expr, cell_nil());
                    cell_retain(expr);
                    Cell* declared = prim_type_query(query_args);
                    cell_release(query_args);

                    if (!cell_is_nil(declared)) {
                        return declared;
                    }
                    cell_release(declared);

                    /* Fall back to inferring from current value */
                    const char* sym_name = cell_get_symbol(expr);
                    Cell* value = eval_lookup(ctx, sym_name);
                    if (value) {
                        Cell* infer_args = cell_cons(value, cell_nil());
                        Cell* inferred = prim_type_infer(infer_args);
                        cell_release(infer_args);
                        cell_release(value);
                        return inferred;
                    }

                    /* Unknown symbol → ⊤ */
                    return make_type_struct(":any", cell_nil());
                }

                /* Application: (operator arg1 arg2 ...) */
                if (cell_is_pair(expr)) {
                    Cell* op = cell_car(expr);
                    Cell* op_args = cell_cdr(expr);

                    /* Conditional: (? cond then else) → union of then/else types */
                    if (cell_is_symbol(op) && op->sym_id == SYM_ID_IF) {
                        /* Skip condition, infer then and else branches */
                        Cell* then_expr = cell_car(cell_cdr(op_args));
                        Cell* else_expr = cell_car(cell_cdr(cell_cdr(op_args)));

                        /* Recursively infer branch types */
                        Cell* then_wrapped = cell_cons(cell_symbol("∈⍜*"),
                                             cell_cons(then_expr, cell_nil()));
                        cell_retain(then_expr);
                        Cell* then_type = eval_internal(ctx, env, then_wrapped);
                        cell_release(then_wrapped);

                        Cell* else_wrapped = cell_cons(cell_symbol("∈⍜*"),
                                             cell_cons(else_expr, cell_nil()));
                        cell_retain(else_expr);
                        Cell* else_type = eval_internal(ctx, env, else_wrapped);
                        cell_release(else_wrapped);

                        if (UNLIKELY(cell_is_error(then_type))) {
                            cell_release(else_type);
                            error_stamp_return(then_type, expr->span.inline_span.lo);
                            return then_type;
                        }
                        if (UNLIKELY(cell_is_error(else_type))) {
                            cell_release(then_type);
                            error_stamp_return(else_type, expr->span.inline_span.lo);
                            return else_type;
                        }

                        /* Same type → that type. Different → union */
                        if (types_equal(then_type, else_type)) {
                            cell_release(else_type);
                            return then_type;
                        }

                        /* Build union type */
                        Cell* left_field = cell_cons(cell_symbol(":left"), then_type);
                        Cell* right_field = cell_cons(cell_symbol(":right"), else_type);
                        Cell* fields = cell_cons(right_field, cell_nil());
                        fields = cell_cons(left_field, fields);
                        return make_type_struct(":union", fields);
                    }

                    /* Lambda: (λ (params...) body) → (→ ⊤ ... body-type) */
                    if (cell_is_symbol(op) && op->sym_id == SYM_ID_LAMBDA) {
                        Cell* params = cell_car(op_args);
                        Cell* body = cell_car(cell_cdr(op_args));

                        /* Count parameters */
                        int arity = 0;
                        Cell* p = params;
                        while (cell_is_pair(p)) {
                            arity++;
                            p = cell_cdr(p);
                        }

                        /* Infer body type recursively */
                        Cell* body_wrapped = cell_cons(cell_symbol("∈⍜*"),
                                             cell_cons(body, cell_nil()));
                        cell_retain(body);
                        Cell* body_type = eval_internal(ctx, env, body_wrapped);
                        cell_release(body_wrapped);

                        if (UNLIKELY(cell_is_error(body_type))) {
                            error_stamp_return(body_type, expr->span.inline_span.lo);
                            return body_type;
                        }

                        /* Build (→ ⊤ ... ⊤ body-type) */
                        Cell* result_type = body_type;
                        for (int i = arity - 1; i >= 0; i--) {
                            Cell* domain = make_type_struct(":any", cell_nil());
                            Cell* domain_field = cell_cons(cell_symbol(":domain"), domain);
                            Cell* codomain_field = cell_cons(cell_symbol(":codomain"), result_type);
                            Cell* flds = cell_cons(codomain_field, cell_nil());
                            flds = cell_cons(domain_field, flds);
                            result_type = make_type_struct(":func", flds);
                        }

                        return result_type;
                    }

                    /* Primitive/function application: look up operator's return type */
                    if (cell_is_symbol(op)) {
                        const char* op_name = cell_get_symbol(op);

                        /* Check primitive signature first */
                        Cell* sym_with_colon;
                        if (op_name[0] != ':') {
                            char buf[256];
                            snprintf(buf, sizeof(buf), ":%s", op_name);
                            sym_with_colon = cell_symbol(buf);
                        } else {
                            sym_with_colon = cell_symbol(op_name);
                        }

                        Cell* sig_args = cell_cons(sym_with_colon, cell_nil());
                        Cell* sig = prim_type_prim_sig(sig_args);
                        cell_release(sig_args);

                        if (!cell_is_nil(sig) && is_type_struct(sig)) {
                            /* Walk through the function type to get the return type */
                            /* (→ A (→ B C)) with 2 args → C */
                            Cell* current_type = sig;
                            Cell* arg_iter = op_args;
                            while (cell_is_pair(arg_iter) && is_type_struct(current_type)) {
                                const char* k = get_type_kind(current_type);
                                if (!k || strcmp(k, ":func") != 0) break;
                                current_type = cell_struct_get_field(current_type, cell_symbol(":codomain"));
                                arg_iter = cell_cdr(arg_iter);
                            }

                            /* current_type is the return type */
                            if (current_type) {
                                cell_retain(current_type);
                                cell_release(sig);
                                return current_type;
                            }
                            cell_release(sig);
                        } else {
                            cell_release(sig);
                        }

                        /* Check if it's a user function with annotation */
                        Cell* fn_query_args = cell_cons(op, cell_nil());
                        cell_retain(op);
                        Cell* fn_type = prim_type_query(fn_query_args);
                        cell_release(fn_query_args);

                        if (!cell_is_nil(fn_type) && is_type_struct(fn_type)) {
                            /* Walk through function type to get return type */
                            Cell* current_type = fn_type;
                            Cell* arg_iter = op_args;
                            while (cell_is_pair(arg_iter) && is_type_struct(current_type)) {
                                const char* k = get_type_kind(current_type);
                                if (!k || strcmp(k, ":func") != 0) break;
                                current_type = cell_struct_get_field(current_type, cell_symbol(":codomain"));
                                arg_iter = cell_cdr(arg_iter);
                            }

                            if (current_type) {
                                cell_retain(current_type);
                                cell_release(fn_type);
                                return current_type;
                            }
                            cell_release(fn_type);
                        } else {
                            cell_release(fn_type);
                        }

                        /* Unknown function → ⊤ */
                        return make_type_struct(":any", cell_nil());
                    }
                }

                /* Fallback: evaluate and infer */
                Cell* evaled = eval_internal(ctx, env, expr);
                if (UNLIKELY(cell_is_error(evaled))) {
                    error_stamp_return(evaled, expr->span.inline_span.lo);
                    return evaled;
                }
                Cell* infer_args = cell_cons(evaled, cell_nil());
                Cell* inferred = prim_type_infer(infer_args);
                cell_release(infer_args);
                cell_release(evaled);
                return inferred;
            }

            /* :λ-converted - already converted nested lambda */
            if (id == SYM_ID_LAMBDA_CONV) {
                /* Parse: (:λ-converted (param1 param2 ...) converted_body) */
                Cell* params = cell_car(rest);
                Cell* converted_body = cell_car(cell_cdr(rest));

                /* Body is already converted, don't convert again */
                int arity = list_length(params);

                /* Closure env: use indexed env if we're in a lambda, empty if top-level */
                Cell* closure_env = env_is_indexed(env) ? env : cell_nil();

                /* Resolve source line from span */
                int source_line = 0;
                if (g_source_map && !span_is_none(expr->span)) {
                    ResolvedPos rp = srcmap_resolve(g_source_map, span_lo(expr->span));
                    source_line = (int)rp.line;
                }

                Cell* lambda = cell_lambda(closure_env, converted_body, arity,
                                          module_get_current_loading(), source_line);

                /* Store param names for documentation */
                {
                    int pn_count = 0;
                    const char** pn = extract_param_names_counted(params, &pn_count);
                    if (pn && pn_count == arity) {
                        lambda->data.lambda.param_names = (const char**)malloc(arity * sizeof(char*));
                        for (int pi = 0; pi < arity; pi++) {
                            lambda->data.lambda.param_names[pi] = strdup(pn[pi]);
                        }
                    }
                    free(pn);
                }

                return lambda;
            }

            /* λ - lambda */
            if (id == SYM_ID_LAMBDA) {
                /* Parse: (λ (param1 param2 ...) body) */
                Cell* params = cell_car(rest);
                Cell* body_expr = cell_car(cell_cdr(rest));

                /* Expand macros in body BEFORE De Bruijn conversion */
                /* This ensures macro templates with lambdas work correctly */
                Cell* expanded_body = macro_expand(body_expr, ctx);
                if (UNLIKELY(cell_is_error(expanded_body))) {
                    error_stamp_return(expanded_body, expr->span.inline_span.lo);
                    return expanded_body;
                }
                /* macro_expand may return the same pointer without retaining;
                 * retain so we can safely release later */
                cell_retain(expanded_body);

                /* Count parameters and extract names (handles ⊳, : annotations, and constraints) */
                int arity = 0;
                Cell* constraints = NULL;
                const char** param_names = extract_param_names_with_constraints(params, &arity, &constraints);

                /* Create conversion context */
                NameContext* ctx_convert = context_new(param_names, arity, NULL);

                /* Convert body to De Bruijn indices */
                Cell* converted_body = debruijn_convert(expanded_body, ctx_convert);
                cell_release(expanded_body);

                /* Closure env: use indexed env if we're in a lambda, empty if top-level */
                Cell* closure_env = env_is_indexed(env) ? env : cell_nil();

                /* Resolve source line from span */
                int source_line = 0;
                if (g_source_map && !span_is_none(expr->span)) {
                    ResolvedPos rp = srcmap_resolve(g_source_map, span_lo(expr->span));
                    source_line = (int)rp.line;
                }

                Cell* lambda = cell_lambda(closure_env, converted_body, arity,
                                          module_get_current_loading(), source_line);

                /* Attach constraints if any */
                if (constraints) {
                    lambda->data.lambda.constraints = constraints;
                }

                /* Store param names on lambda for documentation */
                lambda->data.lambda.param_names = (const char**)malloc(arity * sizeof(char*));
                for (int pi = 0; pi < arity; pi++) {
                    lambda->data.lambda.param_names[pi] = strdup(param_names[pi]);
                }

                /* Cleanup */
                context_free(ctx_convert);
                free(param_names);
                cell_release(converted_body);

                return lambda;
            }

            /* ? - conditional (no "if") */
            if (id == SYM_ID_IF) {
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

            /* ⪢ - sequencing: evaluate all, return last (Day 107) */
            if (id == SYM_ID_SEQUENCE) {
                if (!cell_is_pair(rest)) {
                    return cell_error_at("⪢ requires at least one expression", expr, expr->span);
                }
                Cell* current = rest;
                while (cell_is_pair(current)) {
                    Cell* next = cell_cdr(current);
                    if (!cell_is_pair(next)) {
                        /* Last expression: tail call */
                        expr = cell_car(current);
                        goto tail_call;
                    }
                    /* Intermediate expression: eval and discard */
                    Cell* intermediate = eval_internal(ctx, env, cell_car(current));
                    if (UNLIKELY(cell_is_error(intermediate))) {
                        error_stamp_return(intermediate, expr->span.inline_span.lo);
                        return intermediate;
                    }
                    cell_release(intermediate);
                    current = next;
                }
                return cell_error_at("⪢ unexpected end", expr, expr->span);
            }

            /* ∧ - short-circuit AND (special form) */
            if (id == SYM_ID_AND) {
                Cell* a_expr = cell_car(rest);
                Cell* b_expr = cell_car(cell_cdr(rest));
                Cell* a_val = eval_internal(ctx, env, a_expr);
                if (UNLIKELY(cell_is_error(a_val))) {
                    error_stamp_return(a_val, expr->span.inline_span.lo);
                    return a_val;
                }
                if (cell_is_bool(a_val) && !cell_get_bool(a_val)) {
                    return a_val;  /* #f — short circuit, don't eval b */
                }
                cell_release(a_val);
                /* a was truthy, result is b (tail position) */
                expr = b_expr;
                goto tail_call;
            }

            /* ∨ - short-circuit OR (special form) */
            if (id == SYM_ID_OR) {
                Cell* a_expr = cell_car(rest);
                Cell* b_expr = cell_car(cell_cdr(rest));
                Cell* a_val = eval_internal(ctx, env, a_expr);
                if (UNLIKELY(cell_is_error(a_val))) {
                    error_stamp_return(a_val, expr->span.inline_span.lo);
                    return a_val;
                }
                if (cell_is_bool(a_val) && cell_get_bool(a_val)) {
                    return a_val;  /* #t — short circuit, don't eval b */
                }
                cell_release(a_val);
                /* a was falsy, result is b (tail position) */
                expr = b_expr;
                goto tail_call;
            }

            /* ⚡? - error auto-propagation (Rust ? operator) */
            if (id == SYM_ID_TRY_PROP) {
                Cell* inner_expr = cell_car(rest);
                Cell* val = eval_internal(ctx, env, inner_expr);
                if (UNLIKELY(cell_is_error(val))) {
                    error_stamp_return(val, expr->span.inline_span.lo);
                    return val;  /* Propagate error to caller */
                }
                return val;  /* Unwrap: return value as-is */
            }

            /* ∈⊡ - refinement type definition (special form: name arg unevaluated) */
            if (id == SYM_ID_REFINE_DEF) {
                /* Parse: (∈⊡ name base-type-expr predicate-expr) */
                Cell* name = cell_car(rest);
                Cell* base_expr = cell_car(cell_cdr(rest));
                Cell* pred_expr = cell_car(cell_cdr(cell_cdr(rest)));

                if (!cell_is_symbol(name)) {
                    return cell_error_at("∈⊡ requires symbol as first argument", name, expr->span);
                }

                /* Evaluate base type and predicate */
                Cell* base_type = eval_internal(ctx, env, base_expr);
                if (UNLIKELY(cell_is_error(base_type))) {
                    error_stamp_return(base_type, expr->span.inline_span.lo);
                    return base_type;
                }

                Cell* predicate = eval_internal(ctx, env, pred_expr);
                if (UNLIKELY(cell_is_error(predicate))) {
                    error_stamp_return(predicate, expr->span.inline_span.lo);
                    return predicate;
                }

                /* Call the primitive with (name base_type predicate) */
                extern Cell* prim_refine_def(Cell*);
                Cell* args = cell_cons(name, cell_cons(base_type, cell_cons(predicate, cell_nil())));
                cell_retain(name);
                Cell* result = prim_refine_def(args);
                cell_release(args);
                return result;
            }

            /* ∇ - pattern match (special form) */
            if (id == SYM_ID_RECUR) {
                Cell* expr_unevaled = cell_car(rest);
                Cell* clauses_sexpr = cell_car(cell_cdr(rest));

                /* User should quote the whole clause list: (⌜ ((p1 r1) (p2 r2) ...)) */
                /* Eval once to unquote, giving us the data structure */
                Cell* clauses_data = eval_internal(ctx, env, clauses_sexpr);

                /* Pass to pattern matching with current environment
                 * This ensures De Bruijn indices in closures are dereferenced correctly */
                Cell* result = pattern_eval_match(expr_unevaled, clauses_data, env, ctx);

                /* Clean up */
                cell_release(clauses_data);

                return result;
            }

            /* ============ Effect System Special Forms ============ */

            /* ⟪ - declare effect type
             * Syntax: (⟪ :name :op1 :op2 ...)
             * Registers effect with its operations in the effect registry.
             */
            if (id == SYM_ID_EFFECT_DEF) {
                Cell* name = cell_car(rest);
                if (!cell_is_symbol(name)) {
                    return cell_error_at("effect-declare-requires-symbol", name, expr->span);
                }
                const char* effect_name = cell_get_symbol(name);

                /* Collect operation names into a list */
                Cell* ops = cell_nil();
                Cell* op_rest = cell_cdr(rest);
                while (cell_is_pair(op_rest)) {
                    Cell* op = cell_car(op_rest);
                    if (!cell_is_symbol(op)) {
                        cell_release(ops);
                        return cell_error_at("effect-op-requires-symbol", op, expr->span);
                    }
                    cell_retain(op);
                    ops = cell_cons(op, ops);
                    op_rest = cell_cdr(op_rest);
                }

                eval_register_effect(ctx, effect_name, ops);
                cell_release(ops);
                return cell_bool(true);
            }

            /* ⟪? - query if effect is declared
             * Syntax: (⟪? :name)
             * Returns #t if effect exists, #f otherwise.
             */
            if (id == SYM_ID_EFFECT_Q) {
                Cell* name = cell_car(rest);
                if (!cell_is_symbol(name)) {
                    return cell_error_at("effect-query-requires-symbol", name, expr->span);
                }
                return cell_bool(eval_has_effect(ctx, cell_get_symbol(name)));
            }

            /* ⟪→ - get effect operations
             * Syntax: (⟪→ :name)
             * Returns list of operation symbols, or error if undeclared.
             */
            if (id == SYM_ID_EFFECT_GET) {
                Cell* name = cell_car(rest);
                if (!cell_is_symbol(name)) {
                    return cell_error_at("effect-ops-requires-symbol", name, expr->span);
                }
                Cell* ops = eval_lookup_effect(ctx, cell_get_symbol(name));
                if (!ops) {
                    return cell_error_at("undeclared-effect", name, expr->span);
                }
                return ops;
            }

            /* ⟪⟫ - handle effects
             * Syntax: (⟪⟫ body (:Effect (:op1 handler1) (:op2 handler2)) ...)
             * Installs handlers, evaluates body, removes handlers.
             * Body can perform effects which are dispatched to handlers.
             * Multiple effect handler specs supported.
             */
            if (id == SYM_ID_HANDLE) {
                Cell* body = cell_car(rest);
                Cell* handler_specs = cell_cdr(rest);

                /* Parse handler specs and build frames.
                 * Each spec: (:EffectName (:op1 handler1) (:op2 handler2) ...)
                 * We push one frame per effect.
                 */
                int frame_count = 0;
                EffectFrame frames[16]; /* Max 16 effects per handle */

                Cell* spec_iter = handler_specs;
                while (cell_is_pair(spec_iter)) {
                    Cell* spec = cell_car(spec_iter);
                    if (!cell_is_pair(spec)) {
                        return cell_error_at("effect-handler-requires-list", spec, expr->span);
                    }

                    Cell* effect_name_cell = cell_car(spec);
                    if (!cell_is_symbol(effect_name_cell)) {
                        return cell_error_at("effect-handler-requires-effect-name", effect_name_cell, expr->span);
                    }

                    const char* eff_name = cell_get_symbol(effect_name_cell);

                    /* Build handlers alist: (:op . handler-fn) */
                    Cell* handlers_alist = cell_nil();
                    Cell* op_iter = cell_cdr(spec);
                    while (cell_is_pair(op_iter)) {
                        Cell* op_spec = cell_car(op_iter);
                        if (!cell_is_pair(op_spec)) {
                            cell_release(handlers_alist);
                            return cell_error_at("effect-op-handler-requires-list", op_spec, expr->span);
                        }

                        Cell* op_name = cell_car(op_spec);
                        Cell* handler_expr = cell_car(cell_cdr(op_spec));

                        if (!cell_is_symbol(op_name)) {
                            cell_release(handlers_alist);
                            return cell_error_at("effect-op-requires-symbol", op_name, expr->span);
                        }

                        /* Evaluate the handler expression to get a function */
                        Cell* handler_fn = eval_internal(ctx, env, handler_expr);
                        if (UNLIKELY(cell_is_error(handler_fn))) {
                            cell_release(handlers_alist);
                            error_stamp_return(handler_fn, expr->span.inline_span.lo);
                            return handler_fn;
                        }

                        /* Add to alist: (op-name . handler-fn) */
                        cell_retain(op_name);
                        Cell* binding = cell_cons(op_name, handler_fn);
                        handlers_alist = cell_cons(binding, handlers_alist);

                        op_iter = cell_cdr(op_iter);
                    }

                    if (frame_count >= 16) {
                        cell_release(handlers_alist);
                        return cell_error_at("too-many-effect-handlers", cell_number(16), expr->span);
                    }

                    frames[frame_count].effect_name = eff_name;
                    frames[frame_count].handlers = handlers_alist;
                    frames[frame_count].parent = NULL;
                    frames[frame_count].resumable = false;
                    frames[frame_count].owner_fiber = NULL;
                    frame_count++;

                    spec_iter = cell_cdr(spec_iter);
                }

                /* Push all handler frames (last pushed = first checked) */
                for (int i = 0; i < frame_count; i++) {
                    effect_push_handler(&frames[i]);
                }

                /* Evaluate body with handlers installed */
                Cell* result = eval_internal(ctx, env, body);

                /* Pop all handler frames (reverse order) */
                for (int i = 0; i < frame_count; i++) {
                    effect_pop_handler();
                }

                /* Release handler alists */
                for (int i = 0; i < frame_count; i++) {
                    cell_release(frames[i].handlers);
                }

                return result;
            }

            /* ⟪↺⟫ - resumable handle effects (fiber-based)
             * Syntax: (⟪↺⟫ body (:Effect (:op1 handler1) ...) ...)
             * Like ⟪⟫ but handlers receive continuation k as first arg.
             * Calling (k value) resumes body at the perform point.
             * Not calling k aborts — handler result replaces ⟪↺⟫.
             * Implementation: fiber/coroutine-based delimited continuations.
             */
            if (id == SYM_ID_RESUME) {
                Cell* body = cell_car(rest);
                Cell* handler_specs = cell_cdr(rest);

                /* Parse handler specs — same as ⟪⟫ */
                int frame_count = 0;
                EffectFrame frames[16];

                Cell* spec_iter = handler_specs;
                while (cell_is_pair(spec_iter)) {
                    Cell* spec = cell_car(spec_iter);
                    if (!cell_is_pair(spec)) {
                        return cell_error_at("effect-handler-requires-list", spec, expr->span);
                    }

                    Cell* effect_name_cell = cell_car(spec);
                    if (!cell_is_symbol(effect_name_cell)) {
                        return cell_error_at("effect-handler-requires-effect-name", effect_name_cell, expr->span);
                    }

                    const char* eff_name = cell_get_symbol(effect_name_cell);

                    /* Build handlers alist */
                    Cell* handlers_alist = cell_nil();
                    Cell* op_iter = cell_cdr(spec);
                    while (cell_is_pair(op_iter)) {
                        Cell* op_spec = cell_car(op_iter);
                        if (!cell_is_pair(op_spec)) {
                            cell_release(handlers_alist);
                            return cell_error_at("effect-op-handler-requires-list", op_spec, expr->span);
                        }

                        Cell* op_name = cell_car(op_spec);
                        Cell* handler_expr = cell_car(cell_cdr(op_spec));

                        if (!cell_is_symbol(op_name)) {
                            cell_release(handlers_alist);
                            return cell_error_at("effect-op-requires-symbol", op_name, expr->span);
                        }

                        Cell* handler_fn = eval_internal(ctx, env, handler_expr);
                        if (UNLIKELY(cell_is_error(handler_fn))) {
                            cell_release(handlers_alist);
                            error_stamp_return(handler_fn, expr->span.inline_span.lo);
                            return handler_fn;
                        }

                        cell_retain(op_name);
                        Cell* binding = cell_cons(op_name, handler_fn);
                        handlers_alist = cell_cons(binding, handlers_alist);

                        op_iter = cell_cdr(op_iter);
                    }

                    if (frame_count >= 16) {
                        cell_release(handlers_alist);
                        return cell_error_at("too-many-effect-handlers", cell_number(16), expr->span);
                    }

                    frames[frame_count].effect_name = eff_name;
                    frames[frame_count].handlers = handlers_alist;
                    frames[frame_count].parent = NULL;
                    frames[frame_count].resumable = true;
                    frames[frame_count].owner_fiber = NULL; /* Set below */
                    frame_count++;

                    spec_iter = cell_cdr(spec_iter);
                }

                /* Create fiber for body evaluation */
                Fiber* fiber = fiber_create(ctx, body, env, FIBER_DEFAULT_STACK_SIZE);

                /* Point all frames to this fiber */
                for (int i = 0; i < frame_count; i++) {
                    frames[i].owner_fiber = fiber;
                }

                /* Push handler frames onto global handler stack */
                for (int i = 0; i < frame_count; i++) {
                    effect_push_handler(&frames[i]);
                }

                /* Save/set current fiber */
                Fiber* prev_fiber = fiber_current();
                fiber_set_current(fiber);

                /* Start body evaluation in fiber */
                fiber_start(fiber);

                /* Handler dispatch loop */
                Cell* result = NULL;
                while (fiber->state == FIBER_SUSPENDED) {
                    if (fiber->is_shift_yield) {
                        /* Shift yield — not expected in ⟪↺⟫ context */
                        result = cell_error_at("shift-in-effect-handler", cell_nil(), expr->span);
                        break;
                    }

                    /* Perform yield — find handler */
                    const char* perf_eff = fiber->perform_eff;
                    const char* perf_op = fiber->perform_op;
                    Cell* perf_args = fiber->perform_args;

                    Cell* handler_fn = NULL;
                    for (int i = 0; i < frame_count; i++) {
                        if (strcmp(frames[i].effect_name, perf_eff) == 0) {
                            Cell* h_iter = frames[i].handlers;
                            while (cell_is_pair(h_iter)) {
                                Cell* binding = cell_car(h_iter);
                                if (cell_is_pair(binding)) {
                                    Cell* key = cell_car(binding);
                                    if (cell_is_symbol(key) &&
                                        strcmp(cell_get_symbol(key), perf_op) == 0) {
                                        handler_fn = cell_cdr(binding);
                                        break;
                                    }
                                }
                                h_iter = cell_cdr(h_iter);
                            }
                            if (handler_fn) break;
                        }
                    }

                    if (!handler_fn) {
                        /* Not handled by our frames — propagate to outer handler.
                         * If we're running inside an outer fiber, yield our fiber's
                         * perform info to the outer fiber's ⟪↺⟫ dispatch loop. */
                        if (prev_fiber != NULL) {
                            /* Copy perform info to our fiber and let it look like
                             * we performed the effect. The outer dispatch loop
                             * will see the perform and handle it. */
                            /* We need to yield our inner fiber first, then yield
                             * the outer fiber. Since we ARE on the outer fiber's
                             * stack, we can set up perform info on the outer fiber. */

                            /* The inner fiber already yielded to us. We need to
                             * forward this to the outer handler. Pop our frames,
                             * yield the outer (prev) fiber with the same perform info. */
                            for (int pi = 0; pi < frame_count; pi++) {
                                effect_pop_handler();
                            }

                            /* Set perform info on the outer fiber context */
                            Fiber* outer = prev_fiber;
                            outer->perform_eff = perf_eff;
                            outer->perform_op = perf_op;
                            if (outer->perform_args) cell_release(outer->perform_args);
                            outer->perform_args = perf_args;
                            if (perf_args) cell_retain(perf_args);
                            outer->is_shift_yield = false;

                            /* Yield outer fiber to outer ⟪↺⟫ dispatch loop */
                            fiber_yield(outer);

                            /* Outer handler resumed us with a value — forward to inner fiber */
                            Cell* outer_value = outer->resume_value;

                            /* Re-push our frames */
                            for (int pi = 0; pi < frame_count; pi++) {
                                effect_push_handler(&frames[pi]);
                            }

                            /* Resume inner fiber with the value from outer handler */
                            fiber_resume(fiber, outer_value);
                            continue;
                        }

                        result = cell_error_at("unhandled-resumable-op", cell_symbol(perf_op), expr->span);
                        break;
                    }

                    /* Build continuation k */
                    extern Fiber* g_handling_fiber;
                    extern EffectFrame* g_handling_frames;
                    extern int g_handling_frame_count;

                    Fiber* prev_handling = g_handling_fiber;
                    EffectFrame* prev_hframes = g_handling_frames;
                    int prev_hcount = g_handling_frame_count;

                    g_handling_fiber = fiber;
                    g_handling_frames = frames;
                    g_handling_frame_count = frame_count;

                    extern Cell* prim_fiber_resume_k(Cell* args);
                    Cell* k = cell_builtin((void*)prim_fiber_resume_k);

                    /* Build call args: (k arg1 arg2 ...) */
                    if (perf_args) {
                        cell_retain(perf_args);
                    }
                    Cell* call_args = cell_cons(k, perf_args ? perf_args : cell_nil());

                    /* Call handler function */
                    cell_retain(handler_fn);

                    Cell* handler_result;
                    if (handler_fn->type == CELL_BUILTIN) {
                        Cell* (*builtin_fn)(Cell*) = (Cell* (*)(Cell*))handler_fn->data.atom.builtin;
                        handler_result = builtin_fn(call_args);
                    } else if (handler_fn->type == CELL_LAMBDA) {
                        Cell* lambda_env = handler_fn->data.lambda.env;
                        Cell* lambda_body = handler_fn->data.lambda.body;
                        Cell* new_env = extend_env(lambda_env, call_args);
                        handler_result = eval_internal(ctx, new_env, lambda_body);
                        cell_release(new_env);
                    } else {
                        handler_result = cell_error_at("handler-not-callable", handler_fn, expr->span);
                    }

                    cell_release(handler_fn);
                    cell_release(call_args);

                    /* Restore previous handling context */
                    g_handling_fiber = prev_handling;
                    g_handling_frames = prev_hframes;
                    g_handling_frame_count = prev_hcount;

                    /* If fiber is still suspended and handler didn't call k,
                     * the handler aborted — return handler result */
                    if (fiber->state == FIBER_SUSPENDED && !fiber->k_used) {
                        result = handler_result;
                        break;
                    }

                    /* Handler called k. Two sub-cases:
                     * (a) Fiber finished — handler_result is the final ⟪↺⟫ result
                     *     (handler may have post-processed k's return value)
                     * (b) Fiber re-suspended — another perform, loop continues */
                    if (fiber->state == FIBER_FINISHED) {
                        result = handler_result;
                        break;
                    }

                    if (fiber->state == FIBER_SUSPENDED) {
                        /* Another perform — reset k_used for next handler call,
                         * discard handler_result (it was nil from prim_fiber_resume_k) */
                        fiber->k_used = false;
                        cell_release(handler_result);
                        continue;
                    }

                    /* Shouldn't reach here */
                    result = handler_result;
                    break;
                }

                /* If fiber finished normally (no performs) */
                if (result == NULL && fiber->state == FIBER_FINISHED) {
                    result = fiber->result;
                    cell_retain(result);
                }

                /* Pop handler frames */
                for (int i = 0; i < frame_count; i++) {
                    effect_pop_handler();
                }

                /* Restore previous fiber */
                fiber_set_current(prev_fiber);

                /* Cleanup */
                fiber_destroy(fiber);
                for (int i = 0; i < frame_count; i++) {
                    cell_release(frames[i].handlers);
                }

                return result ? result : cell_error_at("fiber-error", cell_nil(), expr->span);
            }

            /* ↯ - perform effect operation
             * Syntax: (↯ :Effect :op arg1 arg2 ...)
             * Looks up handler on dynamic stack, calls it with args.
             * Non-resumable: returns handler function's result directly.
             * Resumable: checks replay buffer, or signals perform-request.
             */
            if (id == SYM_ID_PERFORM) {
                Cell* effect_name_cell = cell_car(rest);
                Cell* op_name_cell = cell_car(cell_cdr(rest));
                Cell* arg_exprs = cell_cdr(cell_cdr(rest));

                if (!cell_is_symbol(effect_name_cell)) {
                    return cell_error_at("perform-requires-effect-symbol", effect_name_cell, expr->span);
                }
                if (!cell_is_symbol(op_name_cell)) {
                    return cell_error_at("perform-requires-op-symbol", op_name_cell, expr->span);
                }

                const char* eff_name = cell_get_symbol(effect_name_cell);
                const char* op_name = cell_get_symbol(op_name_cell);

                /* Find handler on the dynamic stack */
                EffectFrame* frame = effect_find_handler(eff_name);
                if (!frame) {
                    return cell_error_at("unhandled-effect", effect_name_cell, expr->span);
                }

                /* ===== RESUMABLE PATH (fiber-based) ===== */
                if (frame->resumable && fiber_current() != NULL) {
                    Fiber* fiber = fiber_current();

                    /* Evaluate argument expressions */
                    Cell* args = cell_nil();
                    Cell* args_tail = cell_nil();
                    Cell* arg_iter = arg_exprs;
                    while (cell_is_pair(arg_iter)) {
                        Cell* arg_val = eval_internal(ctx, env, cell_car(arg_iter));
                        if (UNLIKELY(cell_is_error(arg_val))) {
                            cell_release(args);
                            error_stamp_return(arg_val, expr->span.inline_span.lo);
                            return arg_val;
                        }
                        Cell* new_pair = cell_cons(arg_val, cell_nil());
                        if (cell_is_nil(args)) {
                            args = new_pair;
                            args_tail = new_pair;
                        } else {
                            args_tail->data.pair.cdr = new_pair;
                            cell_retain(new_pair);
                            Cell* old_nil = cell_nil();
                            cell_release(old_nil);
                            args_tail = new_pair;
                        }
                        arg_iter = cell_cdr(arg_iter);
                    }

                    /* Store perform info on fiber */
                    fiber->perform_eff = eff_name;
                    fiber->perform_op = op_name;
                    if (fiber->perform_args) {
                        cell_release(fiber->perform_args);
                    }
                    fiber->perform_args = args;
                    cell_retain(args);
                    fiber->is_shift_yield = false;

                    /* Yield to handler — context switches to ⟪↺⟫ dispatch loop */
                    fiber_yield(fiber);

                    /* When we return here, fiber was resumed with a value */
                    Cell* resumed = fiber->resume_value;
                    cell_retain(resumed);
                    cell_release(args);
                    return resumed;
                }

                /* ===== NON-RESUMABLE PATH (original behavior) ===== */

                /* Look up operation handler in the frame's alist */
                Cell* handler_fn = NULL;
                Cell* h_iter = frame->handlers;
                while (cell_is_pair(h_iter)) {
                    Cell* binding = cell_car(h_iter);
                    if (cell_is_pair(binding)) {
                        Cell* key = cell_car(binding);
                        if (cell_is_symbol(key) && strcmp(cell_get_symbol(key), op_name) == 0) {
                            handler_fn = cell_cdr(binding);
                            break;
                        }
                    }
                    h_iter = cell_cdr(h_iter);
                }

                if (!handler_fn) {
                    return cell_error_at("unhandled-operation", op_name_cell, expr->span);
                }

                /* Evaluate argument expressions */
                Cell* args = cell_nil();
                Cell* args_tail = cell_nil();
                Cell* arg_iter = arg_exprs;
                while (cell_is_pair(arg_iter)) {
                    Cell* arg_val = eval_internal(ctx, env, cell_car(arg_iter));
                    if (UNLIKELY(cell_is_error(arg_val))) {
                        cell_release(args);
                        error_stamp_return(arg_val, expr->span.inline_span.lo);
                        return arg_val;
                    }
                    Cell* new_pair = cell_cons(arg_val, cell_nil());
                    if (cell_is_nil(args)) {
                        args = new_pair;
                        args_tail = new_pair;
                    } else {
                        /* Append to end of list */
                        args_tail->data.pair.cdr = new_pair;
                        cell_retain(new_pair);
                        Cell* old_nil = cell_nil();
                        cell_release(old_nil);
                        args_tail = new_pair;
                    }
                    arg_iter = cell_cdr(arg_iter);
                }

                /* Call handler function with args */
                cell_retain(handler_fn);

                if (handler_fn->type == CELL_BUILTIN) {
                    Cell* (*builtin_fn)(Cell*) = (Cell* (*)(Cell*))handler_fn->data.atom.builtin;
                    Cell* result = builtin_fn(args);
                    cell_release(handler_fn);
                    cell_release(args);
                    return result;
                }

                if (handler_fn->type == CELL_LAMBDA) {
                    Cell* closure_env = handler_fn->data.lambda.env;
                    Cell* fn_body = handler_fn->data.lambda.body;
                    int arity = handler_fn->data.lambda.arity;

                    /* Check arity */
                    int arg_count = list_length(args);
                    if (arg_count != arity) {
                        cell_release(handler_fn);
                        cell_release(args);
                        Cell* expected = cell_number(arity);
                        Cell* actual = cell_number(arg_count);
                        Cell* data = cell_cons(expected, cell_cons(actual, cell_nil()));
                        cell_release(expected);
                        cell_release(actual);
                        return cell_error_at("handler-arity-mismatch", data, expr->span);
                    }

                    cell_retain(fn_body);
                    Cell* new_env = extend_env(closure_env, args);
                    cell_release(handler_fn);
                    cell_release(args);

                    /* TCO: handler body in tail position */
                    if (owned_env) {
                        cell_release(owned_env);
                    }
                    if (owned_expr) {
                        cell_release(owned_expr);
                    }
                    env = new_env;
                    owned_env = new_env;
                    expr = fn_body;
                    owned_expr = fn_body;
                    goto tail_call;
                }

                cell_release(handler_fn);
                cell_release(args);
                return cell_error_at("handler-not-a-function", handler_fn, expr->span);
            }

            /* ⟪⊸⟫ - reset/prompt (delimited continuation delimiter)
             * Syntax: (⟪⊸⟫ body)
             * Evaluates body in a new fiber. If body calls (⊸ handler-fn),
             * handler-fn receives one-shot continuation k.
             */
            if (id == SYM_ID_COMPOSE) {
                Cell* body = cell_car(rest);

                /* Create fiber for body evaluation */
                Fiber* fiber = fiber_create(ctx, body, env, FIBER_DEFAULT_STACK_SIZE);

                /* Save/set current fiber */
                Fiber* prev_fiber = fiber_current();
                fiber_set_current(fiber);

                /* Start body evaluation in fiber */
                fiber_start(fiber);

                Cell* result = NULL;

                while (fiber->state == FIBER_SUSPENDED && fiber->is_shift_yield) {
                    /* Shift yield — call shift handler with k */
                    Cell* shift_handler = fiber->shift_handler;
                    if (!shift_handler) {
                        result = cell_error_at("shift-no-handler", cell_nil(), expr->span);
                        break;
                    }

                    /* Build one-shot k */
                    extern Fiber* g_handling_fiber;
                    Fiber* prev_handling = g_handling_fiber;
                    g_handling_fiber = fiber;

                    extern Cell* prim_fiber_resume_k(Cell* args);
                    Cell* k = cell_builtin((void*)prim_fiber_resume_k);

                    /* Call shift_handler(k) */
                    Cell* call_args = cell_cons(k, cell_nil());
                    cell_retain(shift_handler);

                    Cell* handler_result;
                    if (shift_handler->type == CELL_BUILTIN) {
                        Cell* (*builtin_fn)(Cell*) = (Cell* (*)(Cell*))shift_handler->data.atom.builtin;
                        handler_result = builtin_fn(call_args);
                    } else if (shift_handler->type == CELL_LAMBDA) {
                        Cell* lambda_env = shift_handler->data.lambda.env;
                        Cell* lambda_body = shift_handler->data.lambda.body;
                        Cell* new_env = extend_env(lambda_env, call_args);
                        handler_result = eval_internal(ctx, new_env, lambda_body);
                        cell_release(new_env);
                    } else {
                        handler_result = cell_error_at("shift-handler-not-callable", shift_handler, expr->span);
                    }

                    cell_release(shift_handler);
                    cell_release(call_args);
                    g_handling_fiber = prev_handling;

                    /* If handler didn't call k (abort), return handler result */
                    if (fiber->state == FIBER_SUSPENDED && !fiber->k_used) {
                        result = handler_result;
                        break;
                    }

                    /* If fiber finished via k, return handler result (not fiber->result)
                     * because handler may post-process k's return value */
                    if (fiber->state == FIBER_FINISHED) {
                        result = handler_result;
                        break;
                    }

                    /* If fiber suspended again (another shift), reset k_used and loop */
                    if (fiber->state == FIBER_SUSPENDED && fiber->is_shift_yield) {
                        cell_release(handler_result);
                        fiber->k_used = false;
                        continue;
                    }

                    result = handler_result;
                    break;
                }

                /* If fiber finished normally (no shift) */
                if (result == NULL && fiber->state == FIBER_FINISHED) {
                    result = fiber->result;
                    cell_retain(result);
                }

                /* Restore previous fiber */
                fiber_set_current(prev_fiber);
                fiber_destroy(fiber);

                return result ? result : cell_error_at("reset-error", cell_nil(), expr->span);
            }

            /* ⊸ - shift/control (capture delimited continuation)
             * Syntax: (⊸ handler-fn)
             * Must be called inside a ⟪⊸⟫ reset block.
             * handler-fn receives one-shot continuation k.
             */
            if (id == SYM_ID_PIPE) {
                Fiber* fiber = fiber_current();
                if (!fiber) {
                    return cell_error_at("shift-outside-reset", cell_nil(), expr->span);
                }

                /* Evaluate handler function */
                Cell* handler_fn = eval_internal(ctx, env, cell_car(rest));
                if (UNLIKELY(cell_is_error(handler_fn))) {
                    error_stamp_return(handler_fn, expr->span.inline_span.lo);
                    return handler_fn;
                }

                /* Store handler on fiber */
                if (fiber->shift_handler) {
                    cell_release(fiber->shift_handler);
                }
                fiber->shift_handler = handler_fn;
                cell_retain(handler_fn);
                fiber->is_shift_yield = true;

                /* Yield to reset block */
                fiber_yield(fiber);

                /* Resumed with value from k */
                Cell* resumed = fiber->resume_value;
                cell_retain(resumed);
                cell_release(handler_fn);
                return resumed;
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
                    Cell* result = cell_error_at("undefined-variable", var_name, expr->span);
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
            if (UNLIKELY(g_profile_enabled)) g_prof_builtin_calls++;
            /* Call builtin primitive - not a tail call */
            Cell* (*builtin_fn)(Cell*) = (Cell* (*)(Cell*))fn->data.atom.builtin;
            Cell* result = builtin_fn(args);
            cell_release(fn);
            cell_release(args);
            return result;
        }

        if (fn->type == CELL_LAMBDA) {
            if (UNLIKELY(g_profile_enabled)) g_prof_lambda_calls++;
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
                return cell_error_at("arity-mismatch", data, expr->span);
            }

            /* Check trait constraints if any */
            if (fn->data.lambda.constraints) {
                Cell* clist = fn->data.lambda.constraints;
                while (clist && !cell_is_nil(clist)) {
                    Cell* cpair = cell_car(clist);
                    int idx = (int)cell_get_number(cell_car(cpair));
                    const char* trait = cell_get_symbol(cell_cdr(cpair));

                    /* Get the arg at this index (it should be a type symbol) */
                    Cell* arg_at = args;
                    for (int ci = 0; ci < idx && arg_at && !cell_is_nil(arg_at); ci++) {
                        arg_at = cell_cdr(arg_at);
                    }
                    if (arg_at && !cell_is_nil(arg_at)) {
                        Cell* type_arg = cell_car(arg_at);
                        if (cell_is_symbol(type_arg)) {
                            const char* type_name = cell_get_symbol(type_arg);
                            if (!trait_type_satisfies(type_name, trait)) {
                                Cell* data = cell_cons(type_arg, cell_symbol(trait));
                                cell_release(fn);
                                cell_release(args);
                                return cell_error_at("trait-constraint-unsatisfied", data, expr->span);
                            }
                        }
                    }
                    clist = cell_cdr(clist);
                }
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
            if (UNLIKELY(g_profile_enabled)) g_prof_tail_calls++;
            goto tail_call;
        }

        /* Not a function */
        cell_release(args);
        cell_retain(fn);
        Cell* result = cell_error_at("not-a-function", fn, expr->span);
        cell_release(fn);
        return result;
    }

    /* Unknown expression */
    cell_retain(expr);
    return cell_error_at("eval-error", expr, expr->span);
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

/* ============ Effect Registry ============ */

/* Thread-local effect handler stack (dynamic scoping) */
static _Thread_local EffectFrame* effect_handler_stack = NULL;

/* Register an effect type with its operations */
void eval_register_effect(EvalContext* ctx, const char* name, Cell* operations) {
    Cell* name_sym = cell_symbol(name);

    /* Remove old entry if exists */
    Cell* new_registry = cell_nil();
    Cell* current = ctx->effect_registry;
    while (cell_is_pair(current)) {
        Cell* binding = cell_car(current);
        if (cell_is_pair(binding)) {
            Cell* tag = cell_car(binding);
            if (!cell_equal(tag, name_sym)) {
                cell_retain(binding);
                new_registry = cell_cons(binding, new_registry);
            }
        }
        current = cell_cdr(current);
    }
    cell_release(ctx->effect_registry);
    ctx->effect_registry = new_registry;

    /* Add new binding: (name . operations) */
    cell_retain(operations);
    Cell* binding = cell_cons(name_sym, operations);
    ctx->effect_registry = cell_cons(binding, ctx->effect_registry);
}

/* Lookup an effect's operations list (returns NULL if not found) */
Cell* eval_lookup_effect(EvalContext* ctx, const char* name) {
    Cell* name_sym = cell_symbol(name);
    Cell* current = ctx->effect_registry;
    while (cell_is_pair(current)) {
        Cell* binding = cell_car(current);
        if (cell_is_pair(binding)) {
            Cell* tag = cell_car(binding);
            if (cell_equal(tag, name_sym)) {
                Cell* ops = cell_cdr(binding);
                cell_retain(ops);
                cell_release(name_sym);
                return ops;
            }
        }
        current = cell_cdr(current);
    }
    cell_release(name_sym);
    return NULL;
}

/* Check if an effect is registered */
bool eval_has_effect(EvalContext* ctx, const char* name) {
    Cell* ops = eval_lookup_effect(ctx, name);
    if (ops) {
        cell_release(ops);
        return true;
    }
    return false;
}

/* Push a handler frame onto the global handler stack */
void effect_push_handler(EffectFrame* frame) {
    frame->parent = effect_handler_stack;
    effect_handler_stack = frame;
}

/* Pop the top handler frame from the stack */
void effect_pop_handler(void) {
    if (effect_handler_stack) {
        effect_handler_stack = effect_handler_stack->parent;
    }
}

/* Find a handler for the given effect name (walks stack, inner first) */
EffectFrame* effect_find_handler(const char* effect_name) {
    EffectFrame* frame = effect_handler_stack;
    while (frame) {
        if (strcmp(frame->effect_name, effect_name) == 0) {
            return frame;
        }
        frame = frame->parent;
    }
    return NULL;
}

/* ============ Fiber-based Resumable Effect Infrastructure ============ */

/* Globals for handler dispatch context (saved/restored around handler calls) */
Fiber* g_handling_fiber = NULL;
EffectFrame* g_handling_frames = NULL;
int g_handling_frame_count = 0;

/* prim_fiber_resume_k - One-shot continuation for fiber-based resumable effects.
 *
 * When handler calls (k value), this resumes the suspended fiber with the value.
 * The fiber continues from where it yielded (inside ↯).
 * If the fiber performs again, this function dispatches the new perform
 * recursively, so the calling handler sees the final body result.
 * One-shot: calling k a second time returns an error.
 *
 * Args: (value)
 */
Cell* prim_fiber_resume_k(Cell* args) {
    Fiber* fiber = g_handling_fiber;
    EffectFrame* frames = g_handling_frames;
    int frame_count = g_handling_frame_count;

    if (!fiber) {
        return cell_error_at("resume-k-no-fiber", cell_nil(), SPAN_NONE);
    }

    /* One-shot enforcement */
    if (fiber->k_used) {
        return cell_error_at("one-shot-continuation-already-used", cell_nil(), SPAN_NONE);
    }
    fiber->k_used = true;

    if (fiber->state != FIBER_SUSPENDED) {
        return cell_error_at("resume-k-fiber-not-suspended", cell_nil(), SPAN_NONE);
    }

    /* Get the resume value */
    Cell* value = cell_car(args);

    /* Resume the fiber with the value */
    fiber_resume(fiber, value);

    /* Handle any subsequent performs until fiber finishes or aborts */
    while (fiber->state == FIBER_SUSPENDED && !fiber->is_shift_yield) {
        const char* perf_eff = fiber->perform_eff;
        const char* perf_op = fiber->perform_op;
        Cell* perf_args = fiber->perform_args;

        /* Find handler */
        Cell* handler_fn = NULL;
        for (int i = 0; i < frame_count; i++) {
            if (strcmp(frames[i].effect_name, perf_eff) == 0) {
                Cell* h_iter = frames[i].handlers;
                while (cell_is_pair(h_iter)) {
                    Cell* binding = cell_car(h_iter);
                    if (cell_is_pair(binding)) {
                        Cell* key = cell_car(binding);
                        if (cell_is_symbol(key) &&
                            strcmp(cell_get_symbol(key), perf_op) == 0) {
                            handler_fn = cell_cdr(binding);
                            break;
                        }
                    }
                    h_iter = cell_cdr(h_iter);
                }
                if (handler_fn) break;
            }
        }

        if (!handler_fn) {
            return cell_error_at("unhandled-resumable-op", cell_symbol(perf_op), SPAN_NONE);
        }

        /* Reset k_used for new perform, build new k */
        fiber->k_used = false;

        /* Set up handling context for recursive k calls */
        Fiber* prev_handling = g_handling_fiber;
        EffectFrame* prev_hframes = g_handling_frames;
        int prev_hcount = g_handling_frame_count;
        g_handling_fiber = fiber;
        g_handling_frames = frames;
        g_handling_frame_count = frame_count;

        Cell* new_k = cell_builtin((void*)prim_fiber_resume_k);

        if (perf_args) cell_retain(perf_args);
        Cell* call_args = cell_cons(new_k, perf_args ? perf_args : cell_nil());

        cell_retain(handler_fn);

        Cell* handler_result;
        EvalContext* eval_ctx = fiber->eval_ctx;
        if (handler_fn->type == CELL_BUILTIN) {
            Cell* (*builtin_fn)(Cell*) = (Cell* (*)(Cell*))handler_fn->data.atom.builtin;
            handler_result = builtin_fn(call_args);
        } else if (handler_fn->type == CELL_LAMBDA) {
            Cell* lambda_env = handler_fn->data.lambda.env;
            Cell* lambda_body = handler_fn->data.lambda.body;
            Cell* new_env = extend_env(lambda_env, call_args);
            handler_result = eval_internal(eval_ctx, new_env, lambda_body);
            cell_release(new_env);
        } else {
            handler_result = cell_error_at("handler-not-callable", handler_fn, SPAN_NONE);
        }

        cell_release(handler_fn);
        cell_release(call_args);

        g_handling_fiber = prev_handling;
        g_handling_frames = prev_hframes;
        g_handling_frame_count = prev_hcount;

        /* If handler didn't call k (abort), return handler's result */
        if (fiber->state == FIBER_SUSPENDED && !fiber->k_used) {
            return handler_result;
        }

        /* If fiber finished (handler called k, which recursively handled everything),
         * return handler_result (which wraps the final result via post-processing) */
        if (fiber->state == FIBER_FINISHED) {
            return handler_result;
        }

        /* If fiber re-suspended again: this shouldn't happen because
         * the recursive prim_fiber_resume_k call handles inner performs.
         * But just in case, loop. */
        cell_release(handler_result);
    }

    /* Fiber finished without further performs */
    if (fiber->state == FIBER_FINISHED) {
        Cell* result = fiber->result;
        cell_retain(result);
        return result;
    }

    /* Fiber suspended for a shift yield — return control to ⟪⊸⟫ dispatch loop.
     * The dispatch loop will see the fiber is suspended with is_shift_yield
     * and handle the new shift. Return nil as the k result since the
     * handler's return value will be discarded by the dispatch loop anyway. */
    if (fiber->state == FIBER_SUSPENDED && fiber->is_shift_yield) {
        return cell_nil();
    }

    return cell_error_at("resume-k-unexpected-state", cell_nil(), SPAN_NONE);
}
