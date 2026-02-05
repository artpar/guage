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
        // Free pattern clauses if converting from pattern-based
        if (existing->is_pattern_based && existing->clauses) {
            MacroClause* c = existing->clauses;
            while (c) {
                MacroClause* next = c->next;
                if (c->pattern) cell_release(c->pattern);
                if (c->templ) cell_release(c->templ);
                free(c);
                c = next;
            }
            existing->clauses = NULL;
        }
        if (existing->params) cell_release(existing->params);
        if (existing->body) cell_release(existing->body);
        cell_retain(params);
        cell_retain(body);
        existing->params = params;
        existing->body = body;
        existing->is_pattern_based = false;
        existing->clauses = NULL;
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
    entry->is_pattern_based = false;
    entry->clauses = NULL;
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
    if (!macro || !ctx) {
        return cell_error("invalid-macro-application", cell_nil());
    }

    /* Handle pattern-based macros */
    if (macro->is_pattern_based) {
        return macro_apply_pattern(macro, args);
    }

    /* Simple macro: require args */
    if (!args) {
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

        /* Free pattern clauses if pattern-based macro */
        if (entry->is_pattern_based && entry->clauses) {
            MacroClause* c = entry->clauses;
            while (c) {
                MacroClause* nc = c->next;
                if (c->pattern) cell_release(c->pattern);
                if (c->templ) cell_release(c->templ);
                free(c);
                c = nc;
            }
        }
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

/* ═══════════════════════════════════════════════════════════════
 * Pattern-Based Macros (Day 75)
 * ═══════════════════════════════════════════════════════════════ */

bool macro_is_pattern_var(Cell* sym) {
    if (!sym || !cell_is_symbol(sym)) {
        return false;
    }
    const char* name = cell_get_symbol(sym);
    return name && name[0] == '$';
}

/* Check if symbol is the ellipsis marker (...) */
static bool is_ellipsis(Cell* expr) {
    if (!expr || !cell_is_symbol(expr)) {
        return false;
    }
    const char* name = cell_get_symbol(expr);
    return strcmp(name, "...") == 0;
}

/* Note: Legacy rest marker (. or ...) support removed in favor of ellipsis syntax */

/* Check if pattern has ellipsis rest: ($var ...) at end */
static bool has_ellipsis_rest(Cell* pattern, Cell** rest_var) {
    /* Look for ($var ...) pattern - pattern var followed by ... */
    if (!pattern || !cell_is_pair(pattern)) {
        return false;
    }

    Cell* first = cell_car(pattern);
    Cell* second_cons = cell_cdr(pattern);

    /* Check if first is pattern var and second (last) is ... */
    if (macro_is_pattern_var(first) &&
        cell_is_pair(second_cons) &&
        is_ellipsis(cell_car(second_cons)) &&
        cell_is_nil(cell_cdr(second_cons))) {
        if (rest_var) *rest_var = first;
        return true;
    }

    return false;
}

/* Lookup binding in association list */
static Cell* lookup_binding(Cell* bindings, const char* name) {
    Cell* b = bindings;
    while (b && cell_is_pair(b)) {
        Cell* pair = cell_car(b);
        if (cell_is_pair(pair)) {
            Cell* key = cell_car(pair);
            if (cell_is_symbol(key) && strcmp(cell_get_symbol(key), name) == 0) {
                return cell_cdr(pair);
            }
        }
        b = cell_cdr(b);
    }
    return NULL;
}

/* Add a binding to association list */
static Cell* add_binding(Cell* bindings, Cell* var, Cell* val) {
    Cell* pair = cell_cons(var, val);
    Cell* result = cell_cons(pair, bindings);
    cell_release(pair);
    return result;
}

Cell* macro_pattern_match(Cell* pattern, Cell* args) {
    /* Base case: nil pattern matches nil args */
    if (cell_is_nil(pattern)) {
        if (cell_is_nil(args)) {
            return cell_nil();  /* Empty bindings = match */
        }
        return NULL;  /* No match: pattern exhausted but args remain */
    }

    /* Pattern variable: matches any single element */
    if (macro_is_pattern_var(pattern)) {
        Cell* bindings = cell_nil();
        bindings = add_binding(bindings, pattern, args);
        return bindings;
    }

    /* Literal symbol: must match exactly */
    if (cell_is_symbol(pattern)) {
        if (cell_is_symbol(args) &&
            strcmp(cell_get_symbol(pattern), cell_get_symbol(args)) == 0) {
            return cell_nil();  /* Match, no bindings */
        }
        return NULL;  /* No match */
    }

    /* Literal value (number, bool, etc): must match exactly */
    if (!cell_is_pair(pattern)) {
        if (cell_equal(pattern, args)) {
            return cell_nil();
        }
        return NULL;
    }

    /* Pattern is a list */
    if (!cell_is_pair(args) && !cell_is_nil(args)) {
        return NULL;  /* Args must also be a list */
    }

    /* Check for rest pattern: (... . $rest) or (pat1 . $rest) */
    Cell* pattern_cdr = cell_cdr(pattern);
    if (cell_is_pair(pattern) && !cell_is_nil(pattern_cdr) && !cell_is_pair(pattern_cdr)) {
        /* Dotted pair: (pat . $rest) */
        if (macro_is_pattern_var(pattern_cdr)) {
            /* Match head, bind rest */
            Cell* pat_head = cell_car(pattern);

            if (cell_is_nil(args)) {
                /* No args but pattern expects at least one */
                return NULL;
            }

            if (!cell_is_pair(args)) {
                return NULL;
            }

            Cell* arg_head = cell_car(args);
            Cell* arg_rest = cell_cdr(args);

            Cell* head_bindings = macro_pattern_match(pat_head, arg_head);
            if (!head_bindings && head_bindings != cell_nil()) {
                return NULL;
            }

            /* Bind rest variable to remaining args */
            Cell* result = head_bindings ? head_bindings : cell_nil();
            result = add_binding(result, pattern_cdr, arg_rest);
            return result;
        }
    }

    /* Check for ellipsis rest pattern: ($var ...) captures all remaining args */
    Cell* rest_var = NULL;
    if (has_ellipsis_rest(pattern, &rest_var)) {
        /* Bind rest variable to all remaining args (including empty) */
        Cell* bindings = cell_nil();
        bindings = add_binding(bindings, rest_var, args);
        return bindings;
    }

    /* Regular list pattern */
    if (cell_is_nil(args)) {
        return NULL;  /* Args exhausted but pattern remains */
    }

    if (!cell_is_pair(args)) {
        return NULL;
    }

    Cell* pat_head = cell_car(pattern);
    Cell* arg_head = cell_car(args);

    /* Check for quoted literal in pattern */
    if (cell_is_pair(pat_head) && cell_is_symbol(cell_car(pat_head))) {
        const char* sym = cell_get_symbol(cell_car(pat_head));
        if (strcmp(sym, "quote") == 0 || strcmp(sym, "quote") == 0) {
            /* (⌜ literal) - match literal exactly */
            Cell* literal = cell_car(cell_cdr(pat_head));
            if (!cell_equal(literal, arg_head)) {
                return NULL;
            }
            /* Match rest */
            Cell* rest_bindings = macro_pattern_match(cell_cdr(pattern), cell_cdr(args));
            return rest_bindings;
        }
    }

    /* Match head */
    Cell* head_bindings = macro_pattern_match(pat_head, arg_head);
    if (!head_bindings && head_bindings != cell_nil()) {
        if (head_bindings == NULL) return NULL;
    }

    /* Match rest */
    Cell* rest_bindings = macro_pattern_match(cell_cdr(pattern), cell_cdr(args));
    if (!rest_bindings && rest_bindings != cell_nil()) {
        if (head_bindings) cell_release(head_bindings);
        if (rest_bindings == NULL) return NULL;
    }

    /* Merge bindings */
    Cell* result = head_bindings ? head_bindings : cell_nil();
    Cell* r = rest_bindings;
    while (r && cell_is_pair(r)) {
        Cell* pair = cell_car(r);
        if (cell_is_pair(pair)) {
            result = add_binding(result, cell_car(pair), cell_cdr(pair));
        }
        r = cell_cdr(r);
    }
    if (rest_bindings && rest_bindings != cell_nil()) {
        cell_release(rest_bindings);
    }

    return result;
}

Cell* macro_expand_template(Cell* templ, Cell* bindings) {
    if (!templ) {
        return cell_nil();
    }

    /* Pattern variable: lookup and substitute */
    if (macro_is_pattern_var(templ)) {
        const char* name = cell_get_symbol(templ);
        Cell* val = lookup_binding(bindings, name);
        if (val) {
            cell_retain(val);
            return val;
        }
        /* Unbound pattern var - return as-is (error case) */
        cell_retain(templ);
        return templ;
    }

    /* Atom: return as-is */
    if (!cell_is_pair(templ)) {
        cell_retain(templ);
        return templ;
    }

    /* Check for ($var ...) ellipsis splice pattern */
    /* This splices the list bound to $var directly */
    Cell* rest_var = NULL;
    if (has_ellipsis_rest(templ, &rest_var)) {
        const char* name = cell_get_symbol(rest_var);
        Cell* val = lookup_binding(bindings, name);
        if (val) {
            cell_retain(val);
            return val;
        }
        /* Unbound - return nil */
        return cell_nil();
    }

    /* Check for (. $rest) splice pattern (legacy) */
    Cell* templ_car = cell_car(templ);
    if (cell_is_symbol(templ_car)) {
        const char* sym = cell_get_symbol(templ_car);
        if (strcmp(sym, ".") == 0) {
            /* Splice: return the bound list directly */
            Cell* splice_var = cell_car(cell_cdr(templ));
            if (macro_is_pattern_var(splice_var)) {
                const char* name = cell_get_symbol(splice_var);
                Cell* val = lookup_binding(bindings, name);
                if (val) {
                    cell_retain(val);
                    return val;
                }
            }
        }
    }

    /* List: recursively expand, handling splice at tail position */
    Cell* expanded_car = macro_expand_template(templ_car, bindings);

    /* Check if cdr is ($var ...) - need to splice */
    Cell* templ_cdr = cell_cdr(templ);
    Cell* splice_rest = NULL;
    if (has_ellipsis_rest(templ_cdr, &splice_rest)) {
        /* Splice: (head $rest ...) -> (head . expanded_rest) */
        const char* name = cell_get_symbol(splice_rest);
        Cell* val = lookup_binding(bindings, name);
        if (val) {
            Cell* result = cell_cons(expanded_car, val);
            cell_release(expanded_car);
            return result;
        }
    }

    /* Normal recursive expansion */
    Cell* expanded_cdr = macro_expand_template(templ_cdr, bindings);

    Cell* result = cell_cons(expanded_car, expanded_cdr);
    cell_release(expanded_car);
    cell_release(expanded_cdr);

    return result;
}

void macro_define_pattern(const char* name, Cell* clauses) {
    if (!name || !clauses) {
        return;
    }

    /* Check if macro already exists */
    MacroEntry* existing = macro_lookup(name);
    if (existing) {
        /* Free old clauses if pattern-based */
        if (existing->is_pattern_based) {
            MacroClause* c = existing->clauses;
            while (c) {
                MacroClause* next = c->next;
                if (c->pattern) cell_release(c->pattern);
                if (c->templ) cell_release(c->templ);
                free(c);
                c = next;
            }
        } else {
            /* Converting simple macro to pattern-based */
            if (existing->params) cell_release(existing->params);
            if (existing->body) cell_release(existing->body);
            existing->params = NULL;
            existing->body = NULL;
        }
        existing->is_pattern_based = true;
        existing->clauses = NULL;
    } else {
        /* Create new entry */
        existing = malloc(sizeof(MacroEntry));
        if (!existing) {
            fprintf(stderr, "Error: Failed to allocate macro entry\n");
            return;
        }
        existing->name = strdup(name);
        existing->params = NULL;
        existing->body = NULL;
        existing->is_pattern_based = true;
        existing->clauses = NULL;
        existing->next = registry.head;
        registry.head = existing;
    }

    /* Parse clauses: ((pattern template) ...) */
    MacroClause* head = NULL;
    MacroClause* tail = NULL;

    Cell* c = clauses;
    while (c && cell_is_pair(c)) {
        Cell* clause = cell_car(c);
        if (cell_is_pair(clause)) {
            Cell* pattern = cell_car(clause);
            Cell* templ = cell_car(cell_cdr(clause));

            MacroClause* mc = malloc(sizeof(MacroClause));
            if (!mc) continue;

            cell_retain(pattern);
            cell_retain(templ);
            mc->pattern = pattern;
            mc->templ = templ;
            mc->next = NULL;

            if (!head) {
                head = mc;
                tail = mc;
            } else {
                tail->next = mc;
                tail = mc;
            }
        }
        c = cell_cdr(c);
    }

    existing->clauses = head;
}

Cell* macro_apply_pattern(MacroEntry* macro, Cell* args) {
    if (!macro || !macro->is_pattern_based) {
        return cell_error("not-pattern-macro", cell_nil());
    }

    /* Try each clause */
    for (MacroClause* clause = macro->clauses; clause; clause = clause->next) {
        Cell* bindings = macro_pattern_match(clause->pattern, args);
        if (bindings || cell_is_nil(bindings)) {
            /* Match found! Expand template */
            Cell* result = macro_expand_template(clause->templ, bindings);
            if (bindings && !cell_is_nil(bindings)) {
                cell_release(bindings);
            }
            return result;
        }
    }

    /* No pattern matched */
    return cell_error("no-matching-pattern", args);
}
