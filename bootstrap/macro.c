#include "macro.h"
#include "cell.h"
#include "eval.h"
#include "primitives.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Global macro registry */
static MacroRegistry registry = { .head = NULL };

/* Gensym counter for unique symbol generation */
static size_t gensym_counter = 0;

void macro_init(void) {
    registry.head = NULL;
}

void macro_define(const char* name, Cell* params, Cell* body) {
    if (!name || !params || !body) {
        return;
    }

    // Check if macro already exists
    MacroEntry* existing = macro_lookup(name);
    if (existing) {
        // Redefine: update existing entry
        if (existing->params) cell_release(existing->params);
        if (existing->body) cell_release(existing->body);
        cell_retain(params);
        cell_retain(body);
        existing->params = params;
        existing->body = body;
        return;
    }

    // Create new entry
    MacroEntry* entry = malloc(sizeof(MacroEntry));
    if (!entry) {
        fprintf(stderr, "Error: Failed to allocate macro entry\n");
        return;
    }

    entry->name = strdup(name);
    cell_retain(params);
    cell_retain(body);
    entry->params = params;
    entry->body = body;
    entry->next = registry.head;
    registry.head = entry;
}

MacroEntry* macro_lookup(const char* name) {
    if (!name) return NULL;

    for (MacroEntry* entry = registry.head; entry != NULL; entry = entry->next) {
        if (strcmp(entry->name, name) == 0) {
            return entry;
        }
    }
    return NULL;
}

bool macro_is_macro_call(Cell* expr) {
    if (!expr || !cell_is_pair(expr)) {
        return false;
    }

    Cell* first = cell_car(expr);
    if (!cell_is_symbol(first)) {
        return false;
    }

    const char* name = cell_get_symbol(first);
    return macro_lookup(name) != NULL;
}

Cell* macro_build_bindings(Cell* params, Cell* args) {
    // Build association list: ((param1 . arg1) (param2 . arg2) ...)
    Cell* bindings = cell_nil();
    Cell* p = params;
    Cell* a = args;

    while (p != NULL && !cell_is_nil(p)) {
        if (!cell_is_pair(p)) {
            // Params must be a proper list
            cell_release(bindings);
            return cell_error("invalid-macro-params", cell_nil());
        }

        if (cell_is_nil(a)) {
            // Too few arguments
            cell_release(bindings);
            return cell_error("macro-arg-count-mismatch", cell_nil());
        }

        if (!cell_is_pair(a)) {
            // Args must be a proper list
            cell_release(bindings);
            return cell_error("invalid-macro-args", cell_nil());
        }

        Cell* param = cell_car(p);
        Cell* arg = cell_car(a);

        // Create binding: (param . arg)
        Cell* binding = cell_cons(param, arg);
        bindings = cell_cons(binding, bindings);

        p = cell_cdr(p);
        a = cell_cdr(a);
    }

    // Check if too many arguments
    if (a != NULL && !cell_is_nil(a)) {
        cell_release(bindings);
        return cell_error("macro-arg-count-mismatch", cell_nil());
    }

    return bindings;
}

Cell* macro_apply(MacroEntry* macro, Cell* args, EvalContext* ctx) {
    if (!macro || !args || !ctx) {
        return cell_error("invalid-macro-application", cell_nil());
    }

    // Build bindings from params and args
    Cell* bindings = macro_build_bindings(macro->params, args);
    if (cell_is_error(bindings)) {
        return bindings;
    }

    // Evaluate macro body in the macro environment
    // The body is a quasiquoted template, so evaluating it produces expanded code
    // Save and restore environment for macro parameter binding
    Cell* old_env = ctx->env;
    ctx->env = bindings;

    Cell* expanded = eval(ctx, macro->body);

    ctx->env = old_env;

    // Cleanup
    cell_release(bindings);

    return expanded;
}

Cell* macro_expand(Cell* expr, EvalContext* ctx) {
    if (!expr || !ctx) {
        return expr;
    }

    // Atoms don't expand
    if (!cell_is_pair(expr)) {
        return expr;
    }

    Cell* first = cell_car(expr);

    // Check if first element is a macro
    if (cell_is_symbol(first)) {
        const char* name = cell_get_symbol(first);
        MacroEntry* macro = macro_lookup(name);

        if (macro) {
            // This is a macro call! Expand it.
            Cell* args = cell_cdr(expr);
            Cell* expanded = macro_apply(macro, args, ctx);

            if (cell_is_error(expanded)) {
                return expanded;
            }

            // Recursively expand result (macros can call macros)
            Cell* result = macro_expand(expanded, ctx);

            // Only release if result is different
            if (result != expanded) {
                cell_release(expanded);
            }
            return result;
        }
    }

    // Not a macro - recursively expand subexpressions
    Cell* expanded_car = macro_expand(cell_car(expr), ctx);
    Cell* expanded_cdr = macro_expand(cell_cdr(expr), ctx);

    // Check if anything changed
    if (expanded_car == cell_car(expr) && expanded_cdr == cell_cdr(expr)) {
        return expr;  // No changes
    }

    // Build new expression with expanded parts
    Cell* result = cell_cons(expanded_car, expanded_cdr);

    // Release temporaries if they're different from originals
    if (expanded_car != cell_car(expr)) {
        cell_release(expanded_car);
    }
    if (expanded_cdr != cell_cdr(expr)) {
        cell_release(expanded_cdr);
    }

    return result;
}

void macro_cleanup(void) {
    MacroEntry* entry = registry.head;
    while (entry != NULL) {
        MacroEntry* next = entry->next;

        free(entry->name);
        if (entry->params) cell_release(entry->params);
        if (entry->body) cell_release(entry->body);
        free(entry);

        entry = next;
    }
    registry.head = NULL;
    gensym_counter = 0;  /* Reset gensym counter */
}

Cell* macro_gensym(const char* prefix) {
    char buf[128];
    if (prefix && *prefix) {
        snprintf(buf, sizeof(buf), "%s_%zu", prefix, gensym_counter++);
    } else {
        snprintf(buf, sizeof(buf), "g_%zu", gensym_counter++);
    }
    return cell_symbol(buf);
}

Cell* macro_list(void) {
    Cell* result = cell_nil();

    for (MacroEntry* entry = registry.head; entry != NULL; entry = entry->next) {
        Cell* name = cell_symbol(entry->name);
        Cell* new_result = cell_cons(name, result);
        cell_release(name);
        cell_release(result);
        result = new_result;
    }

    return result;
}

Cell* macro_expand_once(Cell* expr, EvalContext* ctx) {
    if (!expr || !ctx) {
        return expr;
    }

    /* Atoms don't expand */
    if (!cell_is_pair(expr)) {
        cell_retain(expr);
        return expr;
    }

    Cell* first = cell_car(expr);

    /* Check if first element is a macro */
    if (cell_is_symbol(first)) {
        const char* name = cell_get_symbol(first);
        MacroEntry* macro = macro_lookup(name);

        if (macro) {
            /* This is a macro call! Expand it ONCE (no recursion) */
            Cell* args = cell_cdr(expr);
            return macro_apply(macro, args, ctx);
        }
    }

    /* Not a macro - return as-is */
    cell_retain(expr);
    return expr;
}
