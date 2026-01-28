#ifndef GUAGE_DEBRUIJN_H
#define GUAGE_DEBRUIJN_H

#include "cell.h"

/* De Bruijn Index Conversion
 *
 * Converts lambda calculus expressions from named variables
 * to De Bruijn indices for efficient runtime evaluation.
 *
 * Example:
 *   Input:  (λ (x) (+ x 1))
 *   Output: (λ (+ 0 1))  ; 0 refers to x
 *
 *   Input:  (λ (x) (λ (y) (+ x y)))
 *   Output: (λ (λ (+ 1 0)))  ; 0→y, 1→x
 */

/* Name binding context for conversion */
typedef struct NameContext {
    const char** names;     /* Array of parameter names */
    int count;              /* Number of names at this level */
    struct NameContext* parent;  /* Outer scope */
} NameContext;

/* Create new context with parameter names */
NameContext* context_new(const char** names, int count, NameContext* parent);

/* Free context */
void context_free(NameContext* ctx);

/* Lookup name in context, return De Bruijn index or -1 if not found */
int debruijn_lookup(const char* name, NameContext* ctx);

/* Convert expression from named to De Bruijn indices */
Cell* debruijn_convert(Cell* expr, NameContext* ctx);

/* Convert list of expressions */
Cell* debruijn_convert_list(Cell* list, NameContext* ctx);

#endif /* GUAGE_DEBRUIJN_H */
