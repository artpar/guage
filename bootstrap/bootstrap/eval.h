#ifndef GUAGE_EVAL_H
#define GUAGE_EVAL_H

#include "cell.h"

/* Evaluator
 *
 * Evaluates Guage expressions in an environment.
 * Supports:
 * - Lambda calculus (De Bruijn indices)
 * - Primitive operations
 * - Quote/eval metaprogramming
 */

/* Evaluation context */
typedef struct {
    Cell* env;          /* Current environment */
    Cell* primitives;   /* Primitive bindings */
} EvalContext;

/* Create new evaluation context */
EvalContext* eval_context_new(void);

/* Free evaluation context */
void eval_context_free(EvalContext* ctx);

/* Evaluate expression */
Cell* eval(EvalContext* ctx, Cell* expr);

/* Define global binding */
void eval_define(EvalContext* ctx, const char* name, Cell* value);

/* Lookup variable */
Cell* eval_lookup(EvalContext* ctx, const char* name);

#endif /* GUAGE_EVAL_H */
