#ifndef GUAGE_PATTERN_H
#define GUAGE_PATTERN_H

#include "cell.h"
#include "eval.h"
#include <stdbool.h>

/* Pattern Matching
 *
 * Implements pattern matching for Guage:
 * - Literal patterns (#42, :foo, ∅, #t)
 * - Wildcard pattern (_)
 * - Variable patterns (x) - Day 16
 * - Pair patterns ((⟨⟩ a b)) - Day 17
 * - ADT patterns ([:Cons h t]) - Day 18
 */

/* Pattern matching result */
typedef struct {
    bool success;           /* Did pattern match? */
    Cell* bindings;         /* Alist: ((var . value) ...) or NULL */
} MatchResult;

/* Try to match a value against a pattern
 *
 * Returns:
 *   {success: true, bindings: ...} if match succeeded
 *   {success: false, bindings: NULL} if match failed
 *
 * Pattern types supported:
 * - Wildcard: _ (always matches, no bindings) - Day 15
 * - Literals: #42, #t, #f, :foo, ∅ - Day 15
 * - Variables: x, n, etc (binds value) - Day 16 ✅
 */
MatchResult pattern_try_match(Cell* value, Cell* pattern);

/* Evaluate a match expression
 *
 * Syntax: (∇ expr [pattern₁ result₁] [pattern₂ result₂] ...)
 *
 * Evaluates expr, then tries each pattern clause in order.
 * Returns the result of the first matching clause.
 * Returns error if no clause matches.
 */
Cell* pattern_eval_match(Cell* expr, Cell* clauses, EvalContext* ctx);

/* Free bindings list */
void pattern_free_bindings(Cell* bindings);

#endif /* GUAGE_PATTERN_H */
