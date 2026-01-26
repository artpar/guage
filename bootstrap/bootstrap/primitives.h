#ifndef GUAGE_PRIMITIVES_H
#define GUAGE_PRIMITIVES_H

#include "cell.h"

/* The 42 Primitives of Guage
 *
 * Core Lambda Calculus (6)
 * Metaprogramming (3)
 * Type Constructors (9) - compile-time only
 * Linear Logic (4) - compile-time only
 * Session Types (5) - compile-time only
 * Effects (4)
 * Refinement Types (4) - compile-time only
 * Actors (3)
 * Comparison & Logic (4)
 */

/* Runtime primitives (evaluated at runtime) */

/* Core Lambda Calculus */
Cell* prim_cons(Cell* args);      /* ⟨ ⟩ - construct cell */
Cell* prim_car(Cell* args);       /* ◁ - head */
Cell* prim_cdr(Cell* args);       /* ▷ - tail */
Cell* prim_lambda(Cell* args);    /* λ - abstraction */
Cell* prim_apply(Cell* args);     /* · - application */

/* Metaprogramming */
Cell* prim_quote(Cell* args);     /* ⌜⌝ - quote */
Cell* prim_eval(Cell* args);      /* ⌞⌟ - eval */

/* Comparison & Logic */
Cell* prim_equal(Cell* args);     /* ≡ - equality */
Cell* prim_not_equal(Cell* args); /* ≢ - inequality */
Cell* prim_and(Cell* args);       /* ∧ - logical AND */
Cell* prim_or(Cell* args);        /* ∨ - logical OR */
Cell* prim_not(Cell* args);       /* ¬ - logical NOT */

/* Arithmetic (derived, not primitive, but needed) */
Cell* prim_add(Cell* args);       /* ⊕ - addition */
Cell* prim_sub(Cell* args);       /* ⊖ - subtraction */
Cell* prim_mul(Cell* args);       /* ⊗ - multiplication */
Cell* prim_div(Cell* args);       /* ⊘ - division */
Cell* prim_lt(Cell* args);        /* < - less than */
Cell* prim_gt(Cell* args);        /* > - greater than */
Cell* prim_le(Cell* args);        /* ≤ - less than or equal */
Cell* prim_ge(Cell* args);        /* ≥ - greater than or equal */

/* Type predicates */
Cell* prim_is_number(Cell* args);
Cell* prim_is_bool(Cell* args);
Cell* prim_is_symbol(Cell* args);
Cell* prim_is_nil(Cell* args);
Cell* prim_is_pair(Cell* args);
Cell* prim_is_atom(Cell* args);

/* Effect primitives (placeholder) */
Cell* prim_effect_block(Cell* args);   /* ⟪⟫ - effect computation */
Cell* prim_effect_handle(Cell* args);  /* ↯ - effect handler */
Cell* prim_effect_pure(Cell* args);    /* ⤴ - pure lift */
Cell* prim_effect_bind(Cell* args);    /* ≫ - effect sequencing */

/* Actor primitives (placeholder) */
Cell* prim_spawn(Cell* args);          /* ⟳ - spawn actor */
Cell* prim_send(Cell* args);           /* →! - send message */
Cell* prim_receive(Cell* args);        /* ←? - receive message */

/* Environment for primitives */
typedef struct {
    const char* name;
    Cell* (*fn)(Cell*);
} Primitive;

/* Initialize primitive environment */
Cell* primitives_init(void);

/* Lookup primitive by symbol */
Cell* primitives_lookup(Cell* env, const char* sym);

#endif /* GUAGE_PRIMITIVES_H */
