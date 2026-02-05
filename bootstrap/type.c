#include "type.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

/* Forward declarations */
static TypeExpr* parse_type_expr(const char** sig);
static TypeExpr* parse_primary(const char** sig);
static void skip_whitespace(const char** sig);

/* Allocate new type expression */
static TypeExpr* type_alloc(TypeKind kind) {
    TypeExpr* type = malloc(sizeof(TypeExpr));
    if (!type) return NULL;
    type->kind = kind;
    return type;
}

/* Skip whitespace */
static void skip_whitespace(const char** sig) {
    while (**sig && isspace((unsigned char)**sig)) {
        (*sig)++;
    }
}

/* Check if string starts with prefix */
static bool starts_with(const char* str, const char* prefix) {
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

/* Parse primary type (atom, variable, or parenthesized) */
static TypeExpr* parse_primary(const char** sig) {
    skip_whitespace(sig);

    /* Greek letters - type variables */
    if (starts_with(*sig, "α")) {
        *sig += strlen("α");
        TypeExpr* type = type_alloc(TYPE_VAR);
        type->data.var_name = 'a';
        return type;
    }
    if (starts_with(*sig, "β")) {
        *sig += strlen("β");
        TypeExpr* type = type_alloc(TYPE_VAR);
        type->data.var_name = 'b';
        return type;
    }
    if (starts_with(*sig, "γ")) {
        *sig += strlen("γ");
        TypeExpr* type = type_alloc(TYPE_VAR);
        type->data.var_name = 'c';
        return type;
    }

    /* Number type */
    if (starts_with(*sig, "ℕ")) {
        *sig += strlen("ℕ");
        return type_alloc(TYPE_NUM);
    }

    /* Boolean type */
    if (starts_with(*sig, "Bool")) {
        *sig += strlen("Bool");
        return type_alloc(TYPE_BOOL);
    }

    /* Nil type */
    if (starts_with(*sig, "nil")) {
        *sig += strlen("nil");
        return type_alloc(TYPE_NIL);
    }

    /* Symbol type */
    if (starts_with(*sig, ":symbol")) {
        *sig += strlen(":symbol");
        return type_alloc(TYPE_SYMBOL);
    }

    /* Lambda type */
    if (starts_with(*sig, "lambda")) {
        *sig += strlen("lambda");
        return type_alloc(TYPE_LAMBDA);
    }

    /* Error type */
    if (starts_with(*sig, "error")) {
        *sig += strlen("error");
        return type_alloc(TYPE_ERROR);
    }

    /* Struct type */
    if (starts_with(*sig, "struct-create")) {
        *sig += strlen("struct-create");
        return type_alloc(TYPE_STRUCT);
    }

    /* Node type */
    if (starts_with(*sig, "adt-create")) {
        *sig += strlen("adt-create");
        return type_alloc(TYPE_NODE);
    }

    /* Graph type */
    if (starts_with(*sig, "graph-create")) {
        *sig += strlen("graph-create");
        return type_alloc(TYPE_GRAPH);
    }

    /* Effect type */
    if (starts_with(*sig, "effect")) {
        *sig += strlen("effect");
        return type_alloc(TYPE_EFFECT);
    }

    /* Unit type () */
    if (starts_with(*sig, "()")) {
        *sig += strlen("()");
        return type_alloc(TYPE_UNIT);
    }

    /* Pair type ⟨α β⟩ */
    if (starts_with(*sig, "⟨")) {
        *sig += strlen("⟨");
        skip_whitespace(sig);

        TypeExpr* car_type = parse_type_expr(sig);
        if (!car_type) return NULL;

        skip_whitespace(sig);

        TypeExpr* cdr_type = parse_type_expr(sig);
        if (!cdr_type) {
            type_free(car_type);
            return NULL;
        }

        skip_whitespace(sig);

        if (!starts_with(*sig, "⟩")) {
            type_free(car_type);
            type_free(cdr_type);
            return NULL;
        }
        *sig += strlen("⟩");

        TypeExpr* type = type_alloc(TYPE_PAIR);
        type->data.pair.car = car_type;
        type->data.pair.cdr = cdr_type;
        return type;
    }

    /* List type [α] or Pattern type [[...]] */
    if (starts_with(*sig, "[")) {
        *sig += 1;
        skip_whitespace(sig);

        /* Check for pattern type [[...]] */
        if (starts_with(*sig, "[")) {
            /* Pattern type */
            *sig += 1;
            skip_whitespace(sig);

            /* Parse "pattern result" or similar */
            /* For now, just skip to ]] */
            int depth = 2;
            while (**sig && depth > 0) {
                if (**sig == '[') depth++;
                else if (**sig == ']') depth--;
                (*sig)++;
            }

            TypeExpr* type = type_alloc(TYPE_PATTERN);
            type->data.pattern.pattern = type_alloc(TYPE_VAR);
            type->data.pattern.pattern->data.var_name = 'p';
            type->data.pattern.result = type_alloc(TYPE_VAR);
            type->data.pattern.result->data.var_name = 'r';
            return type;
        }

        /* List type [α] */
        TypeExpr* elem_type = parse_type_expr(sig);
        if (!elem_type) return NULL;

        skip_whitespace(sig);

        if (!starts_with(*sig, "]")) {
            type_free(elem_type);
            return NULL;
        }
        *sig += 1;

        TypeExpr* type = type_alloc(TYPE_LIST);
        type->data.elem = elem_type;
        return type;
    }

    /* Parenthesized type */
    if (starts_with(*sig, "(")) {
        *sig += 1;
        TypeExpr* type = parse_type_expr(sig);
        skip_whitespace(sig);
        if (starts_with(*sig, ")")) {
            *sig += 1;
        }
        return type;
    }

    return NULL;
}

/* Parse type expression (handles →, |) */
static TypeExpr* parse_type_expr(const char** sig) {
    TypeExpr* left = parse_primary(sig);
    if (!left) return NULL;

    skip_whitespace(sig);

    /* Function type α → β (right-associative) */
    if (starts_with(*sig, "->")) {
        *sig += strlen("->");
        skip_whitespace(sig);

        TypeExpr* right = parse_type_expr(sig);  /* Recursive for right-associativity */
        if (!right) {
            type_free(left);
            return NULL;
        }

        TypeExpr* func_type = type_alloc(TYPE_FUNC);
        func_type->data.func.from = left;
        func_type->data.func.to = right;
        return func_type;
    }

    /* Union type α | β */
    if (starts_with(*sig, "abs")) {
        *sig += strlen("abs");
        skip_whitespace(sig);

        TypeExpr* right = parse_type_expr(sig);
        if (!right) {
            type_free(left);
            return NULL;
        }

        TypeExpr* union_type = type_alloc(TYPE_UNION);
        union_type->data.either.left = left;
        union_type->data.either.right = right;
        return union_type;
    }

    return left;
}

/* Parse type signature string */
TypeExpr* type_parse(const char* sig) {
    if (!sig) return NULL;
    const char* ptr = sig;
    return parse_type_expr(&ptr);
}

/* Free type expression tree */
void type_free(TypeExpr* type) {
    if (!type) return;

    switch (type->kind) {
        case TYPE_FUNC:
            type_free(type->data.func.from);
            type_free(type->data.func.to);
            break;

        case TYPE_PAIR:
            type_free(type->data.pair.car);
            type_free(type->data.pair.cdr);
            break;

        case TYPE_LIST:
            type_free(type->data.elem);
            break;

        case TYPE_UNION:
            type_free(type->data.either.left);
            type_free(type->data.either.right);
            break;

        case TYPE_PATTERN:
            type_free(type->data.pattern.pattern);
            type_free(type->data.pattern.result);
            break;

        default:
            /* Simple types - no nested structure */
            break;
    }

    free(type);
}

/* Debug: Print type expression */
void type_print(TypeExpr* type) {
    if (!type) {
        printf("NULL");
        return;
    }

    switch (type->kind) {
        case TYPE_VAR:
            printf("%c", type->data.var_name);
            break;
        case TYPE_NUM:
            printf("ℕ");
            break;
        case TYPE_BOOL:
            printf("Bool");
            break;
        case TYPE_NIL:
            printf("nil");
            break;
        case TYPE_SYMBOL:
            printf(":symbol");
            break;
        case TYPE_LAMBDA:
            printf("lambda");
            break;
        case TYPE_ERROR:
            printf("error");
            break;
        case TYPE_STRUCT:
            printf("struct-create");
            break;
        case TYPE_NODE:
            printf("adt-create");
            break;
        case TYPE_GRAPH:
            printf("graph-create");
            break;
        case TYPE_EFFECT:
            printf("effect");
            break;
        case TYPE_UNIT:
            printf("()");
            break;
        case TYPE_PAIR:
            printf("⟨");
            type_print(type->data.pair.car);
            printf(" ");
            type_print(type->data.pair.cdr);
            printf("⟩");
            break;
        case TYPE_LIST:
            printf("[");
            type_print(type->data.elem);
            printf("]");
            break;
        case TYPE_FUNC:
            type_print(type->data.func.from);
            printf(" -> ");
            type_print(type->data.func.to);
            break;
        case TYPE_UNION:
            type_print(type->data.either.left);
            printf(" | ");
            type_print(type->data.either.right);
            break;
        case TYPE_PATTERN:
            printf("[[pattern]]");
            break;
    }
}

/* Check if type is a function type */
bool type_is_function(TypeExpr* type) {
    return type && type->kind == TYPE_FUNC;
}

/* Get function arity from type signature */
int type_function_arity(TypeExpr* type) {
    int arity = 0;
    while (type && type->kind == TYPE_FUNC) {
        arity++;
        type = type->data.func.to;
    }
    return arity;
}

/* Get argument type at position (0-indexed) */
TypeExpr* type_get_argument(TypeExpr* type, int pos) {
    for (int i = 0; i < pos && type && type->kind == TYPE_FUNC; i++) {
        type = type->data.func.to;
    }
    if (type && type->kind == TYPE_FUNC) {
        return type->data.func.from;
    }
    return NULL;
}

/* Get return type */
TypeExpr* type_get_return(TypeExpr* type) {
    while (type && type->kind == TYPE_FUNC) {
        type = type->data.func.to;
    }
    return type;
}
