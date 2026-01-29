#ifndef GUAGE_EVAL_H
#define GUAGE_EVAL_H

#include <stddef.h>
#include "cell.h"
#include "fiber.h"

/* Evaluator
 *
 * Evaluates Guage expressions in an environment.
 * Supports:
 * - Lambda calculus (De Bruijn indices)
 * - Primitive operations
 * - Quote/eval metaprogramming
 */

/* Documentation for user-defined functions */
typedef struct FunctionDoc {
    char* name;                    /* Function name */
    char* description;             /* Auto-generated description */
    char* type_signature;          /* Inferred type signature */
    char** dependencies;           /* Array of dependency names */
    size_t dependency_count;       /* Number of dependencies */
    struct FunctionDoc* next;      /* Linked list */
} FunctionDoc;

/* Forward declarations */
typedef struct EvalContext EvalContext;

/* Effect handler frame for dynamic handler stack */
typedef struct EffectFrame {
    const char* effect_name;       /* Which effect this handles */
    Cell* handlers;                /* Alist: (op-name . handler-fn) */
    struct EffectFrame* parent;    /* Previous frame on stack */
    bool resumable;                /* true for resumable handlers */
    Fiber* owner_fiber;            /* Fiber that owns this handler frame */
} EffectFrame;

/* Evaluation context */
struct EvalContext {
    Cell* env;          /* Current environment */
    Cell* primitives;   /* Primitive bindings */
    FunctionDoc* user_docs;  /* User function documentation */
    Cell* type_registry;     /* Type definitions (alist: type_tag -> schema) */
    Cell* effect_registry;   /* Effect definitions (alist: name -> ops-list) */
};

/* Create new evaluation context */
EvalContext* eval_context_new(void);

/* Free evaluation context */
void eval_context_free(EvalContext* ctx);

/* Evaluate expression */
Cell* eval(EvalContext* ctx, Cell* expr);

/* Evaluate expression in specific environment (for pattern matching with closures) */
Cell* eval_internal(EvalContext* ctx, Cell* env, Cell* expr);

/* Define global binding */
void eval_define(EvalContext* ctx, const char* name, Cell* value);

/* Lookup variable in context */
Cell* eval_lookup(EvalContext* ctx, const char* name);

/* Lookup variable in local environment (for trampoline) */
Cell* eval_lookup_env(Cell* env, const char* name);

/* Find user function documentation by name (for primitives to use) */
FunctionDoc* eval_find_user_doc(const char* name);

/* Set current eval context (for primitives to access user docs) */
void eval_set_current_context(EvalContext* ctx);

/* Get current eval context (for primitives to access type registry) */
EvalContext* eval_get_current_context(void);

/* Type registry operations */
void eval_register_type(EvalContext* ctx, Cell* type_tag, Cell* schema);
Cell* eval_lookup_type(EvalContext* ctx, Cell* type_tag);
bool eval_has_type(EvalContext* ctx, Cell* type_tag);

/* Effect registry operations */
void eval_register_effect(EvalContext* ctx, const char* name, Cell* operations);
Cell* eval_lookup_effect(EvalContext* ctx, const char* name);
bool eval_has_effect(EvalContext* ctx, const char* name);

/* Effect handler stack operations */
void effect_push_handler(EffectFrame* frame);
void effect_pop_handler(void);
EffectFrame* effect_find_handler(const char* effect_name);

/* Helper functions for trampoline evaluator */
Cell* extend_env(Cell* env, Cell* args);
int list_length(Cell* list);
Cell* env_lookup_index(Cell* env, int index);
bool env_is_indexed(Cell* env);

#endif /* GUAGE_EVAL_H */
