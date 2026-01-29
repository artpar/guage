#ifndef MACRO_H
#define MACRO_H

#include "cell.h"
#include "eval.h"

/**
 * Macro System for Guage
 *
 * Provides compile-time code transformation via macros.
 * Macros are functions from code to code that expand before evaluation.
 *
 * Syntax: (⧉ name (param1 param2 ...) template-body)
 *
 * Example:
 *   (⧉ when (condition body)
 *     (⌞̃ (? (~ condition) (~ body) ∅)))
 *
 *   (when (> x #0) (⊕ x #1))
 *   ; Expands to: (? (> x #0) (⊕ x #1) ∅)
 */

/* Macro entry in registry */
typedef struct MacroEntry {
    char* name;              /* Macro name (symbol) */
    Cell* params;            /* List of parameter symbols */
    Cell* body;              /* Template body (uses quasiquote/unquote) */
    struct MacroEntry* next; /* Next in linked list */
} MacroEntry;

/* Global macro registry */
typedef struct {
    MacroEntry* head;
} MacroRegistry;

/**
 * Initialize macro system.
 * Call once at startup.
 */
void macro_init(void);

/**
 * Define a new macro.
 *
 * @param name Macro name (symbol)
 * @param params List of parameter symbols
 * @param body Template body (quasiquoted expression)
 */
void macro_define(const char* name, Cell* params, Cell* body);

/**
 * Lookup a macro by name.
 *
 * @param name Macro name to find
 * @return MacroEntry* if found, NULL otherwise
 */
MacroEntry* macro_lookup(const char* name);

/**
 * Check if an expression is a macro call.
 *
 * @param expr Expression to check
 * @return true if expr is (macro-name ...), false otherwise
 */
bool macro_is_macro_call(Cell* expr);

/**
 * Expand all macros in an expression recursively.
 *
 * This is the main entry point for macro expansion.
 * Called before evaluation to transform code.
 *
 * @param expr Expression to expand
 * @param ctx Evaluation context (for variable resolution)
 * @return Expanded expression (may be same as input if no macros)
 */
Cell* macro_expand(Cell* expr, EvalContext* ctx);

/**
 * Apply a macro to arguments (internal helper).
 *
 * @param macro Macro to apply
 * @param args Argument list
 * @param ctx Evaluation context
 * @return Expanded code
 */
Cell* macro_apply(MacroEntry* macro, Cell* args, EvalContext* ctx);

/**
 * Build bindings from macro parameters and arguments.
 *
 * @param params Parameter list
 * @param args Argument list
 * @return Association list of bindings
 */
Cell* macro_build_bindings(Cell* params, Cell* args);

/**
 * Cleanup macro registry (called at shutdown).
 */
void macro_cleanup(void);

/**
 * Generate unique symbol (gensym).
 * Used for macro hygiene to avoid variable capture.
 *
 * @param prefix Optional prefix for symbol (can be NULL)
 * @return Unique symbol (e.g., "g_42" or "prefix_42")
 */
Cell* macro_gensym(const char* prefix);

/**
 * List all defined macros.
 *
 * @return List of macro names as symbols
 */
Cell* macro_list(void);

/**
 * Expand macro once (for debugging).
 * Unlike macro_expand, doesn't recursively expand.
 *
 * @param expr Expression to expand
 * @param ctx Evaluation context
 * @return Expanded expression (one level only)
 */
Cell* macro_expand_once(Cell* expr, EvalContext* ctx);

#endif // MACRO_H
