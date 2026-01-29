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
Cell* prim_prim_apply(Cell* args); /* ⊡ - prim-apply (apply primitive to args) */

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
Cell* prim_mod(Cell* args);       /* % - modulo */
Cell* prim_lt(Cell* args);        /* < - less than */
Cell* prim_gt(Cell* args);        /* > - greater than */
Cell* prim_le(Cell* args);        /* ≤ - less than or equal */
Cell* prim_ge(Cell* args);        /* ≥ - greater than or equal */

/* Property-Based Testing Generators */
Cell* prim_gen_int(Cell* args);        /* gen-int - random integer in range */
Cell* prim_gen_bool(Cell* args);       /* gen-bool - random boolean */
Cell* prim_gen_symbol(Cell* args);     /* gen-symbol - random from list */
Cell* prim_gen_list(Cell* args);       /* gen-list - random list */
Cell* prim_test_property(Cell* args);  /* ⊨-prop - property-based test */

/* Type predicates */
Cell* prim_is_number(Cell* args);
Cell* prim_is_bool(Cell* args);
Cell* prim_is_symbol(Cell* args);
Cell* prim_is_nil(Cell* args);
Cell* prim_is_pair(Cell* args);
Cell* prim_is_atom(Cell* args);

/* Effect primitives (⟪, ⟪⟫, ↯, ⟪?, ⟪→ are special forms in eval.c) */
Cell* prim_effect_pure(Cell* args);    /* ⤴ - pure lift */
Cell* prim_effect_bind(Cell* args);    /* ≫ - effect bind */

/* Actor primitives */
Cell* prim_spawn(Cell* args);          /* ⟳ - spawn actor */
Cell* prim_send(Cell* args);           /* →! - send message */
Cell* prim_receive(Cell* args);        /* ←? - receive message */
Cell* prim_actor_run(Cell* args);      /* ⟳! - run scheduler */
Cell* prim_actor_alive(Cell* args);    /* ⟳? - check alive */
Cell* prim_actor_result(Cell* args);   /* ⟳→ - get result */
Cell* prim_actor_reset(Cell* args);    /* ⟳∅ - reset all actors */

/* Channel primitives */
Cell* prim_chan_create(Cell* args);     /* ⟿⊚ - create channel */
Cell* prim_chan_send(Cell* args);       /* ⟿→ - send to channel */
Cell* prim_chan_recv(Cell* args);       /* ⟿← - receive from channel */
Cell* prim_chan_close(Cell* args);      /* ⟿× - close channel */
Cell* prim_chan_reset(Cell* args);      /* ⟿∅ - reset all channels */

/* Documentation primitives */
Cell* prim_doc_get(Cell* args);        /* ⌂ - get documentation */
Cell* prim_doc_type(Cell* args);       /* ⌂∈ - get type signature */
Cell* prim_doc_deps(Cell* args);       /* ⌂≔ - get dependencies */
Cell* prim_doc_source(Cell* args);     /* ⌂⊛ - get source code */

/* Structure primitives - Leaf */
Cell* prim_struct_define_leaf(Cell* args);  /* ⊙≔ - define leaf type */
Cell* prim_struct_create(Cell* args);       /* ⊙ - create struct instance */
Cell* prim_struct_get_field(Cell* args);    /* ⊙→ - get field value */
Cell* prim_struct_update_field(Cell* args); /* ⊙← - update field (immutable) */
Cell* prim_struct_type_check(Cell* args);   /* ⊙? - check structure type */

/* Structure primitives - Node/ADT */
Cell* prim_struct_define_node(Cell* args);  /* ⊚≔ - define node/ADT type */
Cell* prim_struct_create_node(Cell* args);  /* ⊚ - create node instance */
Cell* prim_struct_get_node(Cell* args);     /* ⊚→ - get node field value */
Cell* prim_struct_is_node(Cell* args);      /* ⊚? - check node type and variant */

/* Structure primitives - Graph */
Cell* prim_graph_define(Cell* args);        /* ⊝≔ - define graph type */
Cell* prim_graph_create(Cell* args);        /* ⊝ - create graph instance */
Cell* prim_graph_add_node(Cell* args);      /* ⊝⊕ - add node to graph */
Cell* prim_graph_add_edge(Cell* args);      /* ⊝⊗ - add edge to graph */
Cell* prim_graph_query(Cell* args);         /* ⊝→ - query graph property */
Cell* prim_graph_is(Cell* args);            /* ⊝? - check graph type */

/* I/O primitives - Console */
Cell* prim_print(Cell* args);               /* ≋ - print with newline */
Cell* prim_print_str(Cell* args);           /* ≋≈ - print string */
Cell* prim_read_line(Cell* args);           /* ≋← - read line from stdin */

/* I/O primitives - Files */
Cell* prim_read_file(Cell* args);           /* ≋⊳ - read file */
Cell* prim_write_file(Cell* args);          /* ≋⊲ - write file */
Cell* prim_append_file(Cell* args);         /* ≋⊕ - append to file */
Cell* prim_file_exists(Cell* args);         /* ≋? - file exists */
Cell* prim_file_empty(Cell* args);          /* ≋∅? - file empty */

/* Module System */
Cell* prim_load(Cell* args);                /* ⋘ - load and evaluate file */
Cell* prim_module_import(Cell* args);       /* ⋖ - selective import */
Cell* prim_module_info(Cell* args);         /* ⌂⊚ - module information */
Cell* prim_module_dependencies(Cell* args); /* ⌂⊚→ - module dependencies */

/* Primitive documentation structure */
typedef struct {
    const char* description;     /* What this primitive does */
    const char* type_signature;  /* Type signature */
} PrimitiveDoc;

/* Environment for primitives */
typedef struct {
    const char* name;
    Cell* (*fn)(Cell*);
    int arity;                   /* Number of arguments (-1 for variadic) */
    PrimitiveDoc doc;            /* MANDATORY documentation */
} Primitive;

/* Initialize primitive environment */
Cell* primitives_init(void);

/* Lookup primitive by symbol */
Cell* primitives_lookup(Cell* env, const char* sym);

/* Lookup primitive by name (returns NULL if not found) */
const Primitive* primitive_lookup_by_name(const char* name);

#endif /* GUAGE_PRIMITIVES_H */
