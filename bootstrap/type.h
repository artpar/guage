#ifndef TYPE_H
#define TYPE_H

#include <stdbool.h>

/* Type expression kinds */
typedef enum {
    TYPE_VAR,       /* α, β, γ - type variables */
    TYPE_NUM,       /* ℕ - natural numbers */
    TYPE_BOOL,      /* 𝔹 - booleans */
    TYPE_NIL,       /* ∅ - nil */
    TYPE_SYMBOL,    /* :symbol - symbols */
    TYPE_LAMBDA,    /* λ - lambda functions */
    TYPE_ERROR,     /* ⚠ - errors */
    TYPE_STRUCT,    /* ⊙ - leaf structures */
    TYPE_NODE,      /* ⊚ - node structures */
    TYPE_GRAPH,     /* ⊝ - graphs */
    TYPE_EFFECT,    /* effect - effect computations */
    TYPE_UNIT,      /* () - unit type */
    TYPE_PAIR,      /* ⟨α β⟩ - pair type */
    TYPE_LIST,      /* [α] - list type */
    TYPE_FUNC,      /* α → β - function type */
    TYPE_UNION,     /* α | β - union type */
    TYPE_PATTERN,   /* [[pattern result]] - pattern list */
} TypeKind;

/* Type expression structure */
typedef struct TypeExpr {
    TypeKind kind;
    union {
        /* TYPE_VAR */
        char var_name;

        /* TYPE_FUNC: α → β */
        struct {
            struct TypeExpr* from;
            struct TypeExpr* to;
        } func;

        /* TYPE_PAIR: ⟨α β⟩ */
        struct {
            struct TypeExpr* car;
            struct TypeExpr* cdr;
        } pair;

        /* TYPE_LIST: [α] */
        struct TypeExpr* elem;

        /* TYPE_UNION: α | β */
        struct {
            struct TypeExpr* left;
            struct TypeExpr* right;
        } either;

        /* TYPE_PATTERN: [[α β]] */
        struct {
            struct TypeExpr* pattern;
            struct TypeExpr* result;
        } pattern;
    } data;
} TypeExpr;

/* Parse type signature string into TypeExpr tree */
TypeExpr* type_parse(const char* sig);

/* Free type expression tree */
void type_free(TypeExpr* type);

/* Debug: Print type expression */
void type_print(TypeExpr* type);

/* Check if type is a function type */
bool type_is_function(TypeExpr* type);

/* Get function arity from type signature */
int type_function_arity(TypeExpr* type);

/* Get argument type at position (0-indexed) */
TypeExpr* type_get_argument(TypeExpr* type, int pos);

/* Get return type */
TypeExpr* type_get_return(TypeExpr* type);

#endif /* TYPE_H */
