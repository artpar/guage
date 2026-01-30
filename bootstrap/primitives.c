#include "primitives.h"
#include "eval.h"
#include "cfg.h"
#include "dfg.h"
#include "pattern.h"
#include "type.h"
#include "testgen.h"
#include "module.h"
#include "macro.h"
#include "actor.h"
#include "channel.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>

/* Helper: get first argument */
static Cell* arg1(Cell* args) {
    assert(cell_is_pair(args));
    return cell_car(args);
}

/* Helper: get second argument */
static Cell* arg2(Cell* args) {
    assert(cell_is_pair(args));
    Cell* rest = cell_cdr(args);
    assert(cell_is_pair(rest));
    return cell_car(rest);
}

/* Helper: get third argument */
static Cell* arg3(Cell* args) {
    assert(cell_is_pair(args));
    Cell* rest = cell_cdr(args);
    assert(cell_is_pair(rest));
    rest = cell_cdr(rest);
    assert(cell_is_pair(rest));
    return cell_car(rest);
}

/* Helper: get fourth argument */
static Cell* arg4(Cell* args) {
    assert(cell_is_pair(args));
    Cell* rest = cell_cdr(args);
    assert(cell_is_pair(rest));
    rest = cell_cdr(rest);
    assert(cell_is_pair(rest));
    rest = cell_cdr(rest);
    assert(cell_is_pair(rest));
    return cell_car(rest);
}

/* Core Lambda Calculus */

/* ⟨ ⟩ - construct cell */
Cell* prim_cons(Cell* args) {
    Cell* a = arg1(args);
    Cell* b = arg2(args);
    return cell_cons(a, b);
}

/* ◁ - head (car) */
Cell* prim_car(Cell* args) {
    Cell* pair = arg1(args);
    assert(cell_is_pair(pair));
    Cell* result = cell_car(pair);
    cell_retain(result);
    return result;
}

/* ▷ - tail (cdr) */
Cell* prim_cdr(Cell* args) {
    Cell* pair = arg1(args);
    assert(cell_is_pair(pair));
    Cell* result = cell_cdr(pair);
    cell_retain(result);
    return result;
}

/* λ - abstraction (handled by evaluator) */
Cell* prim_lambda(Cell* args) {
    /* Lambda construction is handled specially by the evaluator */
    return args;
}

/* · - application (handled by evaluator) */
Cell* prim_apply(Cell* args) {
    /* Application is handled by the evaluator */
    return args;
}

/* Metaprogramming */

/* ⌜⌝ - quote (prevent evaluation) */
Cell* prim_quote(Cell* args) {
    Cell* result = arg1(args);
    cell_retain(result);
    return result;
}

/* ⌞⌟ - eval (evaluate as code) */
Cell* prim_eval(Cell* args) {
    /* Get the expression to evaluate */
    Cell* expr = arg1(args);

    /* Get current eval context */
    EvalContext* ctx = eval_get_current_context();
    if (!ctx) {
        return cell_error("no-context", expr);
    }

    /* Evaluate the expression in current environment */
    return eval(ctx, expr);
}

/* ⊡ - prim-apply (apply primitive to argument list) */
Cell* prim_prim_apply(Cell* args) {
    Cell* fn = arg1(args);
    Cell* arg_list = arg2(args);

    /* Check that fn is a builtin primitive */
    if (fn->type != CELL_BUILTIN) {
        return cell_error("not-a-primitive", fn);
    }

    /* Check that arg_list is a proper list */
    if (!cell_is_nil(arg_list) && !cell_is_pair(arg_list)) {
        return cell_error("not-a-list", arg_list);
    }

    /* Get the primitive function pointer */
    Cell* (*builtin_fn)(Cell*) = (Cell* (*)(Cell*))fn->data.atom.builtin;

    /* Apply the primitive to the argument list */
    return builtin_fn(arg_list);
}

/* ∇ - pattern match is now a SPECIAL FORM in eval.c (not a primitive) */
/* This ensures clauses are not evaluated before pattern matching */

/* Comparison & Logic */

/* ≡ - equality */
Cell* prim_equal(Cell* args) {
    Cell* a = arg1(args);
    Cell* b = arg2(args);
    return cell_bool(cell_equal(a, b));
}

/* ≢ - inequality */
Cell* prim_not_equal(Cell* args) {
    Cell* result = prim_equal(args);
    bool val = !cell_get_bool(result);
    cell_release(result);
    return cell_bool(val);
}

/* ∧ - logical AND */
Cell* prim_and(Cell* args) {
    Cell* a = arg1(args);
    Cell* b = arg2(args);
    assert(cell_is_bool(a) && cell_is_bool(b));
    return cell_bool(cell_get_bool(a) && cell_get_bool(b));
}

/* ∨ - logical OR */
Cell* prim_or(Cell* args) {
    Cell* a = arg1(args);
    Cell* b = arg2(args);
    assert(cell_is_bool(a) && cell_is_bool(b));
    return cell_bool(cell_get_bool(a) || cell_get_bool(b));
}

/* ¬ - logical NOT */
Cell* prim_not(Cell* args) {
    Cell* a = arg1(args);
    assert(cell_is_bool(a));
    return cell_bool(!cell_get_bool(a));
}

/* Arithmetic */

/* ⊕ - addition */
Cell* prim_add(Cell* args) {
    Cell* a = arg1(args);
    Cell* b = arg2(args);
    assert(cell_is_number(a) && cell_is_number(b));
    return cell_number(cell_get_number(a) + cell_get_number(b));
}

/* ⊖ - subtraction */
Cell* prim_sub(Cell* args) {
    Cell* a = arg1(args);
    Cell* b = arg2(args);
    assert(cell_is_number(a) && cell_is_number(b));
    return cell_number(cell_get_number(a) - cell_get_number(b));
}

/* ⊗ - multiplication */
Cell* prim_mul(Cell* args) {
    Cell* a = arg1(args);
    Cell* b = arg2(args);
    assert(cell_is_number(a) && cell_is_number(b));
    return cell_number(cell_get_number(a) * cell_get_number(b));
}

/* ⊘ - division */
Cell* prim_div(Cell* args) {
    Cell* a = arg1(args);
    Cell* b = arg2(args);
    assert(cell_is_number(a) && cell_is_number(b));
    double divisor = cell_get_number(b);
    if (divisor == 0.0) {
        return cell_error("div-by-zero", b);
    }
    return cell_number(cell_get_number(a) / divisor);
}

/* ÷ - integer division (quotient/floor division) */
Cell* prim_quot(Cell* args) {
    Cell* a = arg1(args);
    Cell* b = arg2(args);
    assert(cell_is_number(a) && cell_is_number(b));
    double divisor = cell_get_number(b);
    if (divisor == 0.0) {
        return cell_error("quot-by-zero", b);
    }
    /* Use floor to get integer quotient */
    return cell_number(floor(cell_get_number(a) / divisor));
}

/* % - modulo (remainder) */
Cell* prim_mod(Cell* args) {
    Cell* a = arg1(args);
    Cell* b = arg2(args);
    assert(cell_is_number(a) && cell_is_number(b));
    double divisor = cell_get_number(b);
    if (divisor == 0.0) {
        return cell_error("mod-by-zero", b);
    }
    /* Use fmod for floating point modulo */
    return cell_number(fmod(cell_get_number(a), divisor));
}

/* < - less than */
Cell* prim_lt(Cell* args) {
    Cell* a = arg1(args);
    Cell* b = arg2(args);
    assert(cell_is_number(a) && cell_is_number(b));
    return cell_bool(cell_get_number(a) < cell_get_number(b));
}

/* > - greater than */
Cell* prim_gt(Cell* args) {
    Cell* a = arg1(args);
    Cell* b = arg2(args);
    assert(cell_is_number(a) && cell_is_number(b));
    return cell_bool(cell_get_number(a) > cell_get_number(b));
}

/* ≤ - less than or equal */
Cell* prim_le(Cell* args) {
    Cell* a = arg1(args);
    Cell* b = arg2(args);
    assert(cell_is_number(a) && cell_is_number(b));
    return cell_bool(cell_get_number(a) <= cell_get_number(b));
}

/* ≥ - greater than or equal */
Cell* prim_ge(Cell* args) {
    Cell* a = arg1(args);
    Cell* b = arg2(args);
    assert(cell_is_number(a) && cell_is_number(b));
    return cell_bool(cell_get_number(a) >= cell_get_number(b));
}

/* Math operations */

/* √ - square root */
Cell* prim_sqrt(Cell* args) {
    Cell* a = arg1(args);
    assert(cell_is_number(a));
    double val = cell_get_number(a);
    if (val < 0.0) {
        return cell_error("sqrt-negative", a);
    }
    return cell_number(sqrt(val));
}

/* ^ - power (exponentiation) */
Cell* prim_pow(Cell* args) {
    Cell* base = arg1(args);
    Cell* exp = arg2(args);
    assert(cell_is_number(base) && cell_is_number(exp));
    return cell_number(pow(cell_get_number(base), cell_get_number(exp)));
}

/* |n| - absolute value */
Cell* prim_abs(Cell* args) {
    Cell* a = arg1(args);
    assert(cell_is_number(a));
    return cell_number(fabs(cell_get_number(a)));
}

/* ⌊n⌋ - floor (round down) */
Cell* prim_floor(Cell* args) {
    Cell* a = arg1(args);
    assert(cell_is_number(a));
    return cell_number(floor(cell_get_number(a)));
}

/* ⌈n⌉ - ceiling (round up) */
Cell* prim_ceil(Cell* args) {
    Cell* a = arg1(args);
    assert(cell_is_number(a));
    return cell_number(ceil(cell_get_number(a)));
}

/* ⌊n⌉ - round (nearest integer) */
Cell* prim_round(Cell* args) {
    Cell* a = arg1(args);
    assert(cell_is_number(a));
    return cell_number(round(cell_get_number(a)));
}

/* min - minimum of two numbers */
Cell* prim_min(Cell* args) {
    Cell* a = arg1(args);
    Cell* b = arg2(args);
    assert(cell_is_number(a) && cell_is_number(b));
    double av = cell_get_number(a);
    double bv = cell_get_number(b);
    return cell_number(av < bv ? av : bv);
}

/* max - maximum of two numbers */
Cell* prim_max(Cell* args) {
    Cell* a = arg1(args);
    Cell* b = arg2(args);
    assert(cell_is_number(a) && cell_is_number(b));
    double av = cell_get_number(a);
    double bv = cell_get_number(b);
    return cell_number(av > bv ? av : bv);
}

/* sin - sine (radians) */
Cell* prim_sin(Cell* args) {
    Cell* a = arg1(args);
    assert(cell_is_number(a));
    return cell_number(sin(cell_get_number(a)));
}

/* cos - cosine (radians) */
Cell* prim_cos(Cell* args) {
    Cell* a = arg1(args);
    assert(cell_is_number(a));
    return cell_number(cos(cell_get_number(a)));
}

/* tan - tangent (radians) */
Cell* prim_tan(Cell* args) {
    Cell* a = arg1(args);
    assert(cell_is_number(a));
    return cell_number(tan(cell_get_number(a)));
}

/* asin - arcsine (returns radians) */
Cell* prim_asin(Cell* args) {
    Cell* a = arg1(args);
    assert(cell_is_number(a));
    double val = cell_get_number(a);
    if (val < -1.0 || val > 1.0) {
        return cell_error("asin-domain", a);
    }
    return cell_number(asin(val));
}

/* acos - arccosine (returns radians) */
Cell* prim_acos(Cell* args) {
    Cell* a = arg1(args);
    assert(cell_is_number(a));
    double val = cell_get_number(a);
    if (val < -1.0 || val > 1.0) {
        return cell_error("acos-domain", a);
    }
    return cell_number(acos(val));
}

/* atan - arctangent (returns radians) */
Cell* prim_atan(Cell* args) {
    Cell* a = arg1(args);
    assert(cell_is_number(a));
    return cell_number(atan(cell_get_number(a)));
}

/* atan2 - two-argument arctangent (y, x) */
Cell* prim_atan2(Cell* args) {
    Cell* y = arg1(args);
    Cell* x = arg2(args);
    assert(cell_is_number(y) && cell_is_number(x));
    return cell_number(atan2(cell_get_number(y), cell_get_number(x)));
}

/* log - natural logarithm */
Cell* prim_log(Cell* args) {
    Cell* a = arg1(args);
    assert(cell_is_number(a));
    double val = cell_get_number(a);
    if (val <= 0.0) {
        return cell_error("log-domain", a);
    }
    return cell_number(log(val));
}

/* log10 - base-10 logarithm */
Cell* prim_log10(Cell* args) {
    Cell* a = arg1(args);
    assert(cell_is_number(a));
    double val = cell_get_number(a);
    if (val <= 0.0) {
        return cell_error("log10-domain", a);
    }
    return cell_number(log10(val));
}

/* exp - e^x */
Cell* prim_exp(Cell* args) {
    Cell* a = arg1(args);
    assert(cell_is_number(a));
    return cell_number(exp(cell_get_number(a)));
}

/* π - pi constant */
Cell* prim_pi(Cell* args) {
    (void)args;  /* unused */
    return cell_number(M_PI);
}

/* e - Euler's number constant */
Cell* prim_e(Cell* args) {
    (void)args;  /* unused */
    return cell_number(M_E);
}

/* rand - random number between 0 and 1 */
Cell* prim_rand(Cell* args) {
    (void)args;  /* unused */
    return cell_number((double)rand() / (double)RAND_MAX);
}

/* rand-int - random integer from 0 to n-1 */
Cell* prim_rand_int(Cell* args) {
    Cell* n = arg1(args);
    assert(cell_is_number(n));
    int max = (int)cell_get_number(n);
    if (max <= 0) {
        return cell_error("rand-int-invalid", n);
    }
    return cell_number(rand() % max);
}

/* Property-Based Testing Generators */

/* gen-int - generate random integer in range [low, high] inclusive */
Cell* prim_gen_int(Cell* args) {
    Cell* low = arg1(args);
    Cell* high = arg2(args);

    if (!cell_is_number(low) || !cell_is_number(high)) {
        return cell_error("gen-int-invalid-args", cell_nil());
    }

    int low_val = (int)cell_get_number(low);
    int high_val = (int)cell_get_number(high);

    if (low_val > high_val) {
        return cell_error("gen-int-invalid-range", cell_nil());
    }

    int range = high_val - low_val + 1;
    int result = low_val + (rand() % range);
    return cell_number(result);
}

/* gen-bool - generate random boolean */
Cell* prim_gen_bool(Cell* args) {
    (void)args;  /* unused */
    return cell_bool(rand() % 2 == 0);
}

/* gen-symbol - generate random symbol from list */
Cell* prim_gen_symbol(Cell* args) {
    Cell* symbols = arg1(args);

    if (cell_is_nil(symbols)) {
        return cell_error("gen-symbol-empty-list", cell_nil());
    }

    if (!cell_is_pair(symbols)) {
        return cell_error("gen-symbol-not-list", symbols);
    }

    /* Count list length */
    int len = 0;
    Cell* curr = symbols;
    while (cell_is_pair(curr)) {
        len++;
        curr = cell_cdr(curr);
    }

    /* Pick random index */
    int idx = rand() % len;

    /* Get element at index */
    curr = symbols;
    for (int i = 0; i < idx; i++) {
        curr = cell_cdr(curr);
    }

    Cell* result = cell_car(curr);
    cell_retain(result);
    return result;
}

/* gen-list - generate random list using generator function */
Cell* prim_gen_list(Cell* args) {
    Cell* gen_fn = arg1(args);
    Cell* size_cell = arg2(args);

    if (!cell_is_lambda(gen_fn)) {
        return cell_error("gen-list-not-function", gen_fn);
    }

    if (!cell_is_number(size_cell)) {
        return cell_error("gen-list-invalid-size", size_cell);
    }

    int size = (int)cell_get_number(size_cell);
    if (size < 0) {
        return cell_error("gen-list-negative-size", size_cell);
    }

    /* Get eval context */
    EvalContext* ctx = eval_get_current_context();
    if (!ctx) {
        return cell_error("no-context", cell_nil());
    }

    /* Build list from tail to head */
    Cell* result = cell_nil();

    for (int i = 0; i < size; i++) {
        /* Evaluate lambda body in its closure environment */
        /* For zero-argument lambdas, just evaluate the body directly */
        Cell* generated = eval_internal(ctx, gen_fn->data.lambda.env, gen_fn->data.lambda.body);

        if (cell_is_error(generated)) {
            cell_release(result);
            return generated;
        }

        result = cell_cons(generated, result);
        cell_release(generated);
    }

    return result;
}

/* Type predicates */

Cell* prim_is_number(Cell* args) {
    return cell_bool(cell_is_number(arg1(args)));
}

Cell* prim_is_bool(Cell* args) {
    return cell_bool(cell_is_bool(arg1(args)));
}

Cell* prim_is_symbol(Cell* args) {
    return cell_bool(cell_is_symbol(arg1(args)));
}

Cell* prim_is_nil(Cell* args) {
    return cell_bool(cell_is_nil(arg1(args)));
}

Cell* prim_is_pair(Cell* args) {
    return cell_bool(cell_is_pair(arg1(args)));
}

Cell* prim_is_atom(Cell* args) {
    return cell_bool(cell_is_atom(arg1(args)));
}

/* ============== Type Annotation Primitives ============== */

/* Global type annotation registry (maps symbol names to types)
 * Stored as alist: ((name . type) (name . type) ...)
 */
static Cell* type_annotation_registry = NULL;

/* Helper: Create a type structure with given tag and fields */
Cell* make_type_struct(const char* kind, Cell* fields) {
    Cell* type_tag = cell_symbol(":type");
    Cell* kind_field = cell_cons(cell_symbol(":kind"), cell_symbol(kind));
    Cell* fields_with_kind = cell_cons(kind_field, fields);
    return cell_struct(STRUCT_LEAF, type_tag, NULL, fields_with_kind);
}

/* Helper: Check if cell is a type structure */
bool is_type_struct(Cell* c) {
    if (!cell_is_struct(c)) return false;
    Cell* tag = cell_struct_type_tag(c);
    return cell_is_symbol(tag) && strcmp(cell_get_symbol(tag), ":type") == 0;
}

/* Helper: Get type kind from type structure */
const char* get_type_kind(Cell* type) {
    if (!is_type_struct(type)) return NULL;
    Cell* kind = cell_struct_get_field(type, cell_symbol(":kind"));
    if (!kind || !cell_is_symbol(kind)) return NULL;
    return cell_get_symbol(kind);
}

/* Helper: Check type equality recursively */
bool types_equal(Cell* t1, Cell* t2) {
    if (!is_type_struct(t1) || !is_type_struct(t2)) {
        return cell_equal(t1, t2);
    }

    const char* k1 = get_type_kind(t1);
    const char* k2 = get_type_kind(t2);

    if (!k1 || !k2 || strcmp(k1, k2) != 0) return false;

    /* For basic types, kind equality is sufficient */
    if (strcmp(k1, ":int") == 0 || strcmp(k1, ":bool") == 0 ||
        strcmp(k1, ":string") == 0 || strcmp(k1, ":any") == 0 ||
        strcmp(k1, ":nil") == 0 || strcmp(k1, ":symbol") == 0 ||
        strcmp(k1, ":pair") == 0 || strcmp(k1, ":function") == 0 ||
        strcmp(k1, ":error") == 0) {
        return true;
    }

    /* For function types, compare domain and codomain */
    if (strcmp(k1, ":func") == 0) {
        Cell* d1 = cell_struct_get_field(t1, cell_symbol(":domain"));
        Cell* d2 = cell_struct_get_field(t2, cell_symbol(":domain"));
        Cell* c1 = cell_struct_get_field(t1, cell_symbol(":codomain"));
        Cell* c2 = cell_struct_get_field(t2, cell_symbol(":codomain"));
        return types_equal(d1, d2) && types_equal(c1, c2);
    }

    /* For list types, compare element types */
    if (strcmp(k1, ":list") == 0) {
        Cell* e1 = cell_struct_get_field(t1, cell_symbol(":element"));
        Cell* e2 = cell_struct_get_field(t2, cell_symbol(":element"));
        return types_equal(e1, e2);
    }

    /* For pair types, compare car and cdr types */
    if (strcmp(k1, ":pair-type") == 0) {
        Cell* a1 = cell_struct_get_field(t1, cell_symbol(":car"));
        Cell* a2 = cell_struct_get_field(t2, cell_symbol(":car"));
        Cell* b1 = cell_struct_get_field(t1, cell_symbol(":cdr"));
        Cell* b2 = cell_struct_get_field(t2, cell_symbol(":cdr"));
        return types_equal(a1, a2) && types_equal(b1, b2);
    }

    /* For union types, compare left and right */
    if (strcmp(k1, ":union") == 0) {
        Cell* l1 = cell_struct_get_field(t1, cell_symbol(":left"));
        Cell* l2 = cell_struct_get_field(t2, cell_symbol(":left"));
        Cell* r1 = cell_struct_get_field(t1, cell_symbol(":right"));
        Cell* r2 = cell_struct_get_field(t2, cell_symbol(":right"));
        return types_equal(l1, l2) && types_equal(r1, r2);
    }

    /* For struct types, compare tags */
    if (strcmp(k1, ":struct") == 0) {
        Cell* tag1 = cell_struct_get_field(t1, cell_symbol(":tag"));
        Cell* tag2 = cell_struct_get_field(t2, cell_symbol(":tag"));
        if (tag1 && tag2) return cell_equal(tag1, tag2);
        return tag1 == tag2; /* Both NULL = equal */
    }

    /* For graph types, kind equality is sufficient */
    if (strcmp(k1, ":graph") == 0) {
        return true;
    }

    return false;
}

/* Helper: Check if t1 is subtype of t2 */
static bool is_subtype(Cell* t1, Cell* t2) {
    /* If t2 is a type structure, check its kind */
    if (is_type_struct(t2)) {
        const char* k2 = get_type_kind(t2);

        /* Everything is subtype of ⊤ (any) */
        if (k2 && strcmp(k2, ":any") == 0) return true;

        /* Check union subtypes */
        if (k2 && strcmp(k2, ":union") == 0) {
            Cell* left = cell_struct_get_field(t2, cell_symbol(":left"));
            Cell* right = cell_struct_get_field(t2, cell_symbol(":right"));
            return is_subtype(t1, left) || is_subtype(t1, right);
        }
    }

    /* If both are type structures, compare them */
    if (is_type_struct(t1) && is_type_struct(t2)) {
        return types_equal(t1, t2);
    }

    /* For non-struct types (symbols like :symbol, :pair, :function), just compare equality */
    return cell_equal(t1, t2);
}

/* ℤ - Integer type constant */
Cell* prim_type_int(Cell* args) {
    (void)args;
    return make_type_struct(":int", cell_nil());
}

/* 𝔹 - Boolean type constant */
Cell* prim_type_bool(Cell* args) {
    (void)args;
    return make_type_struct(":bool", cell_nil());
}

/* 𝕊 - String type constant */
Cell* prim_type_string(Cell* args) {
    (void)args;
    return make_type_struct(":string", cell_nil());
}

/* ⊤ - Any type constant (top) */
Cell* prim_type_any(Cell* args) {
    (void)args;
    return make_type_struct(":any", cell_nil());
}

/* ∅ₜ - Nil type constant */
Cell* prim_type_nil(Cell* args) {
    (void)args;
    return make_type_struct(":nil", cell_nil());
}

/* → - Function type constructor
 * Args: domain, codomain, [additional codomains...]
 * (→ ℤ ℤ) = function from int to int
 * (→ ℤ ℤ ℤ) = function from int to int to int (curried)
 */
Cell* prim_type_func(Cell* args) {
    if (!cell_is_pair(args)) {
        return cell_error("→ requires at least two type arguments", cell_nil());
    }

    Cell* domain = arg1(args);
    Cell* rest = cell_cdr(args);

    if (!cell_is_pair(rest)) {
        return cell_error("→ requires at least two type arguments", cell_nil());
    }

    Cell* codomain = cell_car(rest);
    Cell* more = cell_cdr(rest);

    /* If there are more args, recursively build curried function type */
    if (cell_is_pair(more)) {
        /* Build codomain recursively: (→ ℤ ℤ ℤ) = (→ ℤ (→ ℤ ℤ)) */
        Cell* inner_codomain = prim_type_func(rest);
        if (cell_is_error(inner_codomain)) return inner_codomain;
        codomain = inner_codomain;
    }

    /* Build fields: (:domain . domain_type) (:codomain . codomain_type) */
    cell_retain(domain);
    cell_retain(codomain);
    Cell* domain_field = cell_cons(cell_symbol(":domain"), domain);
    Cell* codomain_field = cell_cons(cell_symbol(":codomain"), codomain);
    Cell* fields = cell_cons(codomain_field, cell_nil());
    fields = cell_cons(domain_field, fields);

    return make_type_struct(":func", fields);
}

/* [] - List type constructor (special form - handled by parser)
 * But we need a primitive for programmatic use
 * [ℤ] = list of integers
 */
Cell* prim_type_list(Cell* args) {
    if (!cell_is_pair(args)) {
        return cell_error("[] requires element type", cell_nil());
    }

    Cell* elem_type = arg1(args);
    cell_retain(elem_type);
    Cell* elem_field = cell_cons(cell_symbol(":element"), elem_type);
    Cell* fields = cell_cons(elem_field, cell_nil());

    return make_type_struct(":list", fields);
}

/* ⟨⟩ₜ - Pair type constructor
 * (⟨⟩ₜ ℤ 𝕊) = pair of int and string
 */
Cell* prim_type_pair(Cell* args) {
    if (!cell_is_pair(args)) {
        return cell_error("⟨⟩ₜ requires two type arguments", cell_nil());
    }

    Cell* car_type = arg1(args);
    Cell* rest = cell_cdr(args);

    if (!cell_is_pair(rest)) {
        return cell_error("⟨⟩ₜ requires two type arguments", cell_nil());
    }

    Cell* cdr_type = cell_car(rest);

    cell_retain(car_type);
    cell_retain(cdr_type);
    Cell* car_field = cell_cons(cell_symbol(":car"), car_type);
    Cell* cdr_field = cell_cons(cell_symbol(":cdr"), cdr_type);
    Cell* fields = cell_cons(cdr_field, cell_nil());
    fields = cell_cons(car_field, fields);

    return make_type_struct(":pair-type", fields);
}

/* ∪ₜ - Union type constructor
 * (∪ₜ ℤ ∅ₜ) = int or nil
 */
Cell* prim_type_union(Cell* args) {
    if (!cell_is_pair(args)) {
        return cell_error("∪ₜ requires two type arguments", cell_nil());
    }

    Cell* left = arg1(args);
    Cell* rest = cell_cdr(args);

    if (!cell_is_pair(rest)) {
        return cell_error("∪ₜ requires two type arguments", cell_nil());
    }

    Cell* right = cell_car(rest);

    cell_retain(left);
    cell_retain(right);
    Cell* left_field = cell_cons(cell_symbol(":left"), left);
    Cell* right_field = cell_cons(cell_symbol(":right"), right);
    Cell* fields = cell_cons(right_field, cell_nil());
    fields = cell_cons(left_field, fields);

    return make_type_struct(":union", fields);
}

/* ∈⊙ - Runtime type detection (typeof)
 * Returns the type of a runtime value
 */
Cell* prim_typeof(Cell* args) {
    Cell* val = arg1(args);

    if (cell_is_number(val)) {
        return make_type_struct(":int", cell_nil());
    }
    if (cell_is_bool(val)) {
        return make_type_struct(":bool", cell_nil());
    }
    if (cell_is_string(val)) {
        return make_type_struct(":string", cell_nil());
    }
    if (cell_is_nil(val)) {
        return make_type_struct(":nil", cell_nil());
    }
    if (cell_is_symbol(val)) {
        return cell_symbol(":symbol");
    }
    if (cell_is_pair(val)) {
        return cell_symbol(":pair");
    }
    if (cell_is_lambda(val)) {
        return cell_symbol(":function");
    }
    if (cell_is_error(val)) {
        return cell_symbol(":error");
    }
    if (cell_is_struct(val)) {
        return cell_symbol(":struct");
    }
    if (cell_is_graph(val)) {
        return cell_symbol(":graph");
    }
    if (cell_is_actor(val)) {
        return cell_symbol(":actor");
    }

    return cell_symbol(":unknown");
}

/* ∈≡ - Type equality check */
Cell* prim_type_equal(Cell* args) {
    Cell* t1 = arg1(args);
    Cell* t2 = arg2(args);
    return cell_bool(types_equal(t1, t2));
}

/* ∈⊆ - Subtype check */
Cell* prim_type_subtype(Cell* args) {
    Cell* t1 = arg1(args);
    Cell* t2 = arg2(args);
    return cell_bool(is_subtype(t1, t2));
}

/* ∈! - Type assertion
 * Assert that value has given type, return value or error
 */
Cell* prim_type_assert(Cell* args) {
    Cell* val = arg1(args);
    Cell* expected_type = arg2(args);

    /* Get runtime type of value */
    Cell* actual_type = prim_typeof(args);

    /* Check if actual type is subtype of expected */
    Cell* subtype_args = cell_cons(actual_type, cell_cons(expected_type, cell_nil()));
    Cell* is_sub = prim_type_subtype(subtype_args);
    cell_release(subtype_args);

    if (cell_get_bool(is_sub)) {
        cell_release(is_sub);
        cell_release(actual_type);
        cell_retain(val);
        return val;
    }

    cell_release(is_sub);
    cell_release(actual_type);
    return cell_error(":type-error", val);
}

/* ∈ - Declare type annotation for a symbol
 * (∈ name type) - stores the type annotation
 */
Cell* prim_type_declare(Cell* args) {
    Cell* name = arg1(args);
    Cell* type = arg2(args);

    if (!cell_is_symbol(name)) {
        return cell_error("∈ requires symbol as first argument", name);
    }

    /* Initialize registry if needed */
    if (type_annotation_registry == NULL) {
        type_annotation_registry = cell_nil();
    }

    /* Check if already declared */
    Cell* current = type_annotation_registry;
    while (cell_is_pair(current)) {
        Cell* binding = cell_car(current);
        Cell* bound_name = cell_car(binding);
        if (cell_equal(bound_name, name)) {
            /* Update existing binding */
            Cell* new_binding = cell_cons(name, type);
            cell_retain(name);
            cell_retain(type);
            /* Replace in list (simplified - just prepend) */
            type_annotation_registry = cell_cons(new_binding, type_annotation_registry);
            return type;
        }
        current = cell_cdr(current);
    }

    /* Add new binding */
    Cell* binding = cell_cons(name, type);
    cell_retain(name);
    cell_retain(type);
    type_annotation_registry = cell_cons(binding, type_annotation_registry);

    cell_retain(type);
    return type;
}

/* ∈? - Query type annotation for a symbol
 * (∈? name) - returns declared type or nil
 */
Cell* prim_type_query(Cell* args) {
    Cell* name = arg1(args);

    if (!cell_is_symbol(name)) {
        return cell_error("∈? requires symbol", name);
    }

    if (type_annotation_registry == NULL) {
        return cell_nil();
    }

    Cell* current = type_annotation_registry;
    while (cell_is_pair(current)) {
        Cell* binding = cell_car(current);
        Cell* bound_name = cell_car(binding);
        if (cell_equal(bound_name, name)) {
            Cell* type = cell_cdr(binding);
            cell_retain(type);
            return type;
        }
        current = cell_cdr(current);
    }

    return cell_nil();
}

/* ∈◁ - Get domain (input type) of function type */
Cell* prim_type_domain(Cell* args) {
    Cell* func_type = arg1(args);

    if (!is_type_struct(func_type)) {
        return cell_error("∈◁ requires function type", func_type);
    }

    const char* kind = get_type_kind(func_type);
    if (!kind || strcmp(kind, ":func") != 0) {
        return cell_error("∈◁ requires function type", func_type);
    }

    Cell* domain = cell_struct_get_field(func_type, cell_symbol(":domain"));
    if (domain) {
        cell_retain(domain);
        return domain;
    }
    return cell_nil();
}

/* ∈▷ - Get codomain (output type) of function type */
Cell* prim_type_codomain(Cell* args) {
    Cell* func_type = arg1(args);

    if (!is_type_struct(func_type)) {
        return cell_error("∈▷ requires function type", func_type);
    }

    const char* kind = get_type_kind(func_type);
    if (!kind || strcmp(kind, ":func") != 0) {
        return cell_error("∈▷ requires function type", func_type);
    }

    Cell* codomain = cell_struct_get_field(func_type, cell_symbol(":codomain"));
    if (codomain) {
        cell_retain(codomain);
        return codomain;
    }
    return cell_nil();
}

/* ∈⊙ₜ - Get element type of list type */
Cell* prim_type_element(Cell* args) {
    Cell* list_type = arg1(args);

    if (!is_type_struct(list_type)) {
        return cell_error("∈⊙ₜ requires list type", list_type);
    }

    const char* kind = get_type_kind(list_type);
    if (!kind || strcmp(kind, ":list") != 0) {
        return cell_error("∈⊙ₜ requires list type", list_type);
    }

    Cell* element = cell_struct_get_field(list_type, cell_symbol(":element"));
    if (element) {
        cell_retain(element);
        return element;
    }
    return cell_nil();
}

/* Helper: Check if value matches declared type */
static bool value_matches_type(Cell* val, Cell* type) {
    if (!is_type_struct(type)) {
        return true;  /* Non-type values vacuously match */
    }

    const char* kind = get_type_kind(type);
    if (!kind) return true;

    /* Any type (⊤) matches everything */
    if (strcmp(kind, ":any") == 0) {
        return true;
    }

    /* Int type */
    if (strcmp(kind, ":int") == 0) {
        return cell_is_number(val);
    }

    /* Bool type */
    if (strcmp(kind, ":bool") == 0) {
        return cell_is_bool(val);
    }

    /* String type */
    if (strcmp(kind, ":string") == 0) {
        return cell_is_string(val);
    }

    /* Nil type */
    if (strcmp(kind, ":nil") == 0) {
        return cell_is_nil(val);
    }

    /* Function type - check if value is a lambda */
    if (strcmp(kind, ":func") == 0) {
        return cell_is_lambda(val);
    }

    /* List type - check if value is nil or pair */
    if (strcmp(kind, ":list") == 0) {
        if (cell_is_nil(val)) return true;  /* Empty list is valid */
        if (!cell_is_pair(val)) return false;
        /* For non-empty lists, we'd need to check all elements
         * For now, just check it's a list structure */
        return true;
    }

    /* Pair type */
    if (strcmp(kind, ":pair") == 0) {
        return cell_is_pair(val);
    }

    /* Union type - check if value matches either side */
    if (strcmp(kind, ":union") == 0) {
        Cell* left = cell_struct_get_field(type, cell_symbol(":left"));
        Cell* right = cell_struct_get_field(type, cell_symbol(":right"));
        bool matches = (left && value_matches_type(val, left)) ||
                      (right && value_matches_type(val, right));
        return matches;
    }

    /* Error type */
    if (strcmp(kind, ":error") == 0) {
        return cell_is_error(val);
    }

    return true;  /* Unknown types pass */
}

/* ∈✓ - Validate binding against declared type */
Cell* prim_type_validate(Cell* args) {
    Cell* name = arg1(args);

    if (!cell_is_symbol(name)) {
        return cell_error("∈✓ requires symbol", name);
    }

    /* Query declared type */
    Cell* query_args = cell_cons(name, cell_nil());
    cell_retain(name);
    Cell* declared_type = prim_type_query(query_args);
    cell_release(query_args);

    /* No declared type - vacuously valid */
    if (cell_is_nil(declared_type)) {
        cell_release(declared_type);
        return cell_bool(true);
    }

    /* Look up the actual value */
    EvalContext* ctx = eval_get_current_context();
    if (!ctx) {
        cell_release(declared_type);
        return cell_error("no-eval-context", name);
    }

    const char* sym_name = cell_get_symbol(name);
    Cell* value = eval_lookup(ctx, sym_name);

    if (!value) {
        cell_release(declared_type);
        return cell_error("unbound-symbol", name);
    }

    /* Check if value matches type */
    bool matches = value_matches_type(value, declared_type);
    cell_release(declared_type);

    if (matches) {
        return cell_bool(true);
    } else {
        return cell_error(":type-error", name);
    }
}

/* ∈✓* - Validate ALL declared types */
Cell* prim_type_validate_all(Cell* args) {
    (void)args;

    if (type_annotation_registry == NULL || cell_is_nil(type_annotation_registry)) {
        return cell_bool(true);  /* No declarations - all valid */
    }

    EvalContext* ctx = eval_get_current_context();
    if (!ctx) {
        return cell_error("no-eval-context", cell_nil());
    }

    Cell* errors = cell_nil();
    Cell* current = type_annotation_registry;

    while (cell_is_pair(current)) {
        Cell* binding = cell_car(current);
        Cell* name = cell_car(binding);
        Cell* declared_type = cell_cdr(binding);

        const char* sym_name = cell_get_symbol(name);
        Cell* value = eval_lookup(ctx, sym_name);

        if (value && !value_matches_type(value, declared_type)) {
            /* Add to error list */
            Cell* err_entry = cell_cons(name, declared_type);
            cell_retain(name);
            cell_retain(declared_type);
            errors = cell_cons(err_entry, errors);
        }

        current = cell_cdr(current);
    }

    if (cell_is_nil(errors)) {
        return cell_bool(true);
    } else {
        return cell_error(":type-errors", errors);
    }
}

/* ∈⊢ - Type-check function application */
Cell* prim_type_check_apply(Cell* args) {
    Cell* fn = arg1(args);

    if (!cell_is_symbol(fn)) {
        return cell_error("∈⊢ requires symbol as first argument", fn);
    }

    /* Query declared type for the function */
    Cell* query_args = cell_cons(fn, cell_nil());
    cell_retain(fn);
    Cell* fn_type = prim_type_query(query_args);
    cell_release(query_args);

    /* No declared type - vacuously passes */
    if (cell_is_nil(fn_type)) {
        cell_release(fn_type);
        return cell_bool(true);
    }

    /* Check if it's a function type */
    if (!is_type_struct(fn_type)) {
        cell_release(fn_type);
        return cell_bool(true);
    }

    const char* kind = get_type_kind(fn_type);
    if (!kind || strcmp(kind, ":func") != 0) {
        cell_release(fn_type);
        return cell_bool(true);  /* Not a function type - pass */
    }

    /* Get domain type */
    Cell* domain = cell_struct_get_field(fn_type, cell_symbol(":domain"));

    /* Check each argument */
    Cell* arg_list = cell_cdr(args);  /* Skip function symbol */
    Cell* current_type = fn_type;

    while (cell_is_pair(arg_list) && current_type && is_type_struct(current_type)) {
        kind = get_type_kind(current_type);
        if (!kind || strcmp(kind, ":func") != 0) break;

        Cell* arg_val = cell_car(arg_list);
        domain = cell_struct_get_field(current_type, cell_symbol(":domain"));

        if (domain && !value_matches_type(arg_val, domain)) {
            cell_release(fn_type);
            return cell_error(":type-error", arg_val);
        }

        /* Move to codomain (next argument type) */
        current_type = cell_struct_get_field(current_type, cell_symbol(":codomain"));
        arg_list = cell_cdr(arg_list);
    }

    cell_release(fn_type);
    return cell_bool(true);
}

/* ============================================================================
 * Type Inference Primitives (Day 85)
 * ============================================================================ */

/* Helper: Check if value is a proper list (nil-terminated pair chain) */
static bool is_proper_list(Cell* val) {
    Cell* current = val;
    while (cell_is_pair(current)) {
        current = cell_cdr(current);
    }
    return cell_is_nil(current);
}

/* Forward declaration for recursive inference */
static Cell* infer_value_type(Cell* val);

/* Helper: Collect all element types from a proper list, returning
 * a single type (possibly union) that covers all elements.
 * Assumes val is a proper list (already checked). */
static Cell* infer_list_element_type(Cell* val) {
    if (cell_is_nil(val)) {
        /* Empty list has no element type - use ⊤ */
        return make_type_struct(":any", cell_nil());
    }

    /* Infer type of first element */
    Cell* first = cell_car(val);
    Cell* result_type = infer_value_type(first);

    /* Walk rest of list, building union if types differ */
    Cell* current = cell_cdr(val);
    while (cell_is_pair(current)) {
        Cell* elem = cell_car(current);
        Cell* elem_type = infer_value_type(elem);

        if (!types_equal(result_type, elem_type)) {
            /* Build union of existing result_type and new elem_type */
            Cell* left_field = cell_cons(cell_symbol(":left"), result_type);
            Cell* right_field = cell_cons(cell_symbol(":right"), elem_type);
            Cell* fields = cell_cons(right_field, cell_nil());
            fields = cell_cons(left_field, fields);
            result_type = make_type_struct(":union", fields);
        } else {
            cell_release(elem_type);
        }

        current = cell_cdr(current);
    }

    return result_type;
}

/* Core: Infer the deep type of a runtime value.
 * Unlike prim_typeof which returns shallow types (:pair, :function),
 * this recursively infers structure:
 *   pairs → (⟨⟩ₜ car-type cdr-type)
 *   proper lists → ([]ₜ element-type)
 *   functions → (→ domain codomain) from annotation, or (→ ⊤ ... ⊤)
 *   structs → struct type with tag
 */
static Cell* infer_value_type(Cell* val) {
    /* Basic types → type structs */
    if (cell_is_number(val)) {
        return make_type_struct(":int", cell_nil());
    }
    if (cell_is_bool(val)) {
        return make_type_struct(":bool", cell_nil());
    }
    if (cell_is_string(val)) {
        return make_type_struct(":string", cell_nil());
    }
    if (cell_is_nil(val)) {
        return make_type_struct(":nil", cell_nil());
    }

    /* Pair/List types → deep inference */
    if (cell_is_pair(val)) {
        if (is_proper_list(val)) {
            /* Proper list: infer element type */
            Cell* elem_type = infer_list_element_type(val);
            Cell* elem_field = cell_cons(cell_symbol(":element"), elem_type);
            Cell* fields = cell_cons(elem_field, cell_nil());
            return make_type_struct(":list", fields);
        } else {
            /* Improper pair: infer car and cdr types */
            Cell* car_type = infer_value_type(cell_car(val));
            Cell* cdr_type = infer_value_type(cell_cdr(val));
            Cell* car_field = cell_cons(cell_symbol(":car"), car_type);
            Cell* cdr_field = cell_cons(cell_symbol(":cdr"), cdr_type);
            Cell* fields = cell_cons(cdr_field, cell_nil());
            fields = cell_cons(car_field, fields);
            return make_type_struct(":pair-type", fields);
        }
    }

    /* Function types → check annotation registry, else generic */
    if (cell_is_lambda(val)) {
        /* Check if this function has a declared name in the annotation registry */
        /* We can't easily map lambda → name, so just build from arity */
        int arity = val->data.lambda.arity;

        /* Build (→ ⊤ ... ⊤) with arity+1 types (arity domains + 1 codomain) */
        Cell* any_type = make_type_struct(":any", cell_nil());
        Cell* result = any_type; /* codomain = ⊤ */

        for (int i = arity - 1; i >= 0; i--) {
            Cell* domain = make_type_struct(":any", cell_nil());
            Cell* domain_field = cell_cons(cell_symbol(":domain"), domain);
            Cell* codomain_field = cell_cons(cell_symbol(":codomain"), result);
            Cell* fields = cell_cons(codomain_field, cell_nil());
            fields = cell_cons(domain_field, fields);
            result = make_type_struct(":func", fields);
        }

        return result;
    }

    /* Symbol type */
    if (cell_is_symbol(val)) {
        return make_type_struct(":symbol", cell_nil());
    }

    /* Error type */
    if (cell_is_error(val)) {
        return make_type_struct(":error", cell_nil());
    }

    /* Struct type → return type struct with the struct's tag */
    if (cell_is_struct(val)) {
        Cell* tag = cell_struct_type_tag(val);
        if (tag) {
            cell_retain(tag);
            Cell* tag_field = cell_cons(cell_symbol(":tag"), tag);
            Cell* fields = cell_cons(tag_field, cell_nil());
            return make_type_struct(":struct", fields);
        }
        return make_type_struct(":struct", cell_nil());
    }

    /* Graph type */
    if (cell_is_graph(val)) {
        return make_type_struct(":graph", cell_nil());
    }

    return make_type_struct(":any", cell_nil());
}

/* ∈⍜ - Deep type inference on a value
 * Unlike ∈⊙ which returns shallow types, this recursively infers:
 *   (∈⍜ (⟨⟩ #1 #2)) → (⟨⟩ₜ (ℤ) (ℤ))  (not just :pair)
 *   (∈⍜ (⟨⟩ #1 ∅))  → ([]ₜ (ℤ))        (detects proper lists)
 *   (∈⍜ (λ (x) x))  → (→ (⊤) (⊤))      (from arity)
 *
 * For named bindings: checks annotation registry first.
 */
Cell* prim_type_infer(Cell* args) {
    Cell* val = arg1(args);
    return infer_value_type(val);
}

/* ∈⍜⊕ - Get type signature of a primitive operation
 * (∈⍜⊕ :⊕) → (→ (ℤ) (ℤ) (ℤ))
 * Returns ∅ for unknown primitives
 */
Cell* prim_type_prim_sig(Cell* args) {
    Cell* sym = arg1(args);

    if (!cell_is_symbol(sym)) {
        return cell_error("∈⍜⊕ requires symbol", sym);
    }

    const char* name = cell_get_symbol(sym);
    /* Strip leading : from keyword symbol */
    if (name[0] == ':') name++;

    /* Build type signatures for known primitives */

    /* Helper macros for building common signatures */
    #define SIG_INT make_type_struct(":int", cell_nil())
    #define SIG_BOOL make_type_struct(":bool", cell_nil())
    #define SIG_STR make_type_struct(":string", cell_nil())
    #define SIG_ANY make_type_struct(":any", cell_nil())
    #define SIG_NIL make_type_struct(":nil", cell_nil())

    /* Build (→ a b) */
    #define FUNC1(dom, cod) ({ \
        Cell* _d = (dom); Cell* _c = (cod); \
        Cell* _df = cell_cons(cell_symbol(":domain"), _d); \
        Cell* _cf = cell_cons(cell_symbol(":codomain"), _c); \
        Cell* _fs = cell_cons(_cf, cell_nil()); \
        _fs = cell_cons(_df, _fs); \
        make_type_struct(":func", _fs); \
    })

    /* Build (→ a (→ b c)) */
    #define FUNC2(a, b, c) FUNC1((a), FUNC1((b), (c)))

    /* Arithmetic: ℤ → ℤ → ℤ */
    if (strcmp(name, "⊕") == 0 || strcmp(name, "⊖") == 0 ||
        strcmp(name, "⊗") == 0 || strcmp(name, "⊘") == 0 ||
        strcmp(name, "%") == 0) {
        return FUNC2(SIG_INT, SIG_INT, SIG_INT);
    }

    /* Comparison: ℤ → ℤ → 𝔹 */
    if (strcmp(name, "<") == 0 || strcmp(name, ">") == 0 ||
        strcmp(name, "≤") == 0 || strcmp(name, "≥") == 0) {
        return FUNC2(SIG_INT, SIG_INT, SIG_BOOL);
    }

    /* Equality: ⊤ → ⊤ → 𝔹 */
    if (strcmp(name, "≡") == 0 || strcmp(name, "≢") == 0 ||
        strcmp(name, "≟") == 0) {
        return FUNC2(SIG_ANY, SIG_ANY, SIG_BOOL);
    }

    /* Logic: 𝔹 → 𝔹 → 𝔹 */
    if (strcmp(name, "∧") == 0 || strcmp(name, "∨") == 0) {
        return FUNC2(SIG_BOOL, SIG_BOOL, SIG_BOOL);
    }

    /* Logic: 𝔹 → 𝔹 */
    if (strcmp(name, "¬") == 0) {
        return FUNC1(SIG_BOOL, SIG_BOOL);
    }

    /* List primitives: ⊤ → ⊤ → ⊤ */
    if (strcmp(name, "⟨⟩") == 0) {
        return FUNC2(SIG_ANY, SIG_ANY, SIG_ANY);
    }

    /* List access: ⊤ → ⊤ */
    if (strcmp(name, "◁") == 0 || strcmp(name, "▷") == 0) {
        return FUNC1(SIG_ANY, SIG_ANY);
    }

    /* Type predicates: ⊤ → 𝔹 */
    if (strcmp(name, "⚠?") == 0 || strcmp(name, "∅?") == 0) {
        return FUNC1(SIG_ANY, SIG_BOOL);
    }

    /* Type introspection: ⊤ → ⊤ */
    if (strcmp(name, "∈⊙") == 0 || strcmp(name, "⊙") == 0) {
        return FUNC1(SIG_ANY, SIG_ANY);
    }

    /* String operations: 𝕊 → ℤ */
    if (strcmp(name, "≈#") == 0) {
        return FUNC1(SIG_STR, SIG_INT);
    }

    /* String concat: 𝕊 → 𝕊 → 𝕊 */
    if (strcmp(name, "≈⊕") == 0) {
        return FUNC2(SIG_STR, SIG_STR, SIG_STR);
    }

    /* String compare: 𝕊 → 𝕊 → 𝔹 */
    if (strcmp(name, "≈≡") == 0) {
        return FUNC2(SIG_STR, SIG_STR, SIG_BOOL);
    }

    /* String substring: 𝕊 → ℤ → ℤ → 𝕊 */
    if (strcmp(name, "≈⊂") == 0) {
        Cell* inner = FUNC2(SIG_INT, SIG_INT, SIG_STR);
        return FUNC1(SIG_STR, inner);
    }

    /* String index: 𝕊 → ℤ → 𝕊 */
    if (strcmp(name, "≈@") == 0) {
        return FUNC2(SIG_STR, SIG_INT, SIG_STR);
    }

    /* Trace: ⊤ → ⊤ */
    if (strcmp(name, "⟲") == 0) {
        return FUNC1(SIG_ANY, SIG_ANY);
    }

    /* Error create: ⊤ → ⊤ → ⊤ */
    if (strcmp(name, "⚠") == 0) {
        return FUNC2(SIG_ANY, SIG_ANY, SIG_ANY);
    }

    /* Math functions: ℤ → ℤ */
    if (strcmp(name, "√") == 0 || strcmp(name, "⌊") == 0 ||
        strcmp(name, "⌈") == 0 || strcmp(name, "abs") == 0 ||
        strcmp(name, "sin") == 0 || strcmp(name, "cos") == 0 ||
        strcmp(name, "tan") == 0 || strcmp(name, "log") == 0 ||
        strcmp(name, "exp") == 0) {
        return FUNC1(SIG_INT, SIG_INT);
    }

    /* Math: ℤ → ℤ → ℤ */
    if (strcmp(name, "^") == 0 || strcmp(name, "min") == 0 ||
        strcmp(name, "max") == 0) {
        return FUNC2(SIG_INT, SIG_INT, SIG_INT);
    }

    #undef SIG_INT
    #undef SIG_BOOL
    #undef SIG_STR
    #undef SIG_ANY
    #undef SIG_NIL
    #undef FUNC1
    #undef FUNC2

    /* Unknown primitive */
    return cell_nil();
}

/* Debug & Error Handling Primitives */

/* ⚠ - create error */
Cell* prim_error_create(Cell* args) {
    Cell* message_cell = arg1(args);
    const char* message = cell_is_symbol(message_cell) ?
        cell_get_symbol(message_cell) : "error";

    Cell* data = cell_is_pair(cell_cdr(args)) ?
        arg2(args) : cell_nil();

    return cell_error(message, data);
}

/* ⚠? - is error */
Cell* prim_is_error(Cell* args) {
    return cell_bool(cell_is_error(arg1(args)));
}

/* ⚠⊙ - get error type (returns symbol with : prefix) */
Cell* prim_error_type(Cell* args) {
    Cell* val = arg1(args);
    if (!cell_is_error(val)) {
        return cell_error("not-an-error", val);
    }
    const char* msg = cell_error_message(val);
    /* Only prepend : if message doesn't already start with : */
    if (msg[0] == ':') {
        return cell_symbol(msg);
    } else {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), ":%s", msg);
        return cell_symbol(buffer);
    }
}

/* ⚠→ - get error data */
Cell* prim_error_data(Cell* args) {
    Cell* val = arg1(args);
    if (!cell_is_error(val)) {
        return cell_error("not-an-error", val);
    }
    return cell_error_data(val);
}

/* ⊢ - assert */
Cell* prim_assert(Cell* args) {
    Cell* condition = arg1(args);
    Cell* message_cell = arg2(args);

    /* Check condition */
    bool is_true = cell_is_bool(condition) && cell_get_bool(condition);

    if (!is_true) {
        const char* message = cell_is_symbol(message_cell) ?
            cell_get_symbol(message_cell) : "assertion-failed";
        return cell_error(message, condition);
    }

    /* Return true on success */
    return cell_bool(true);
}

/* ⟳ - trace (print and return value) */
Cell* prim_trace(Cell* args) {
    Cell* value = arg1(args);
    printf("⟳ ");
    cell_print(value);
    printf("\n");
    cell_retain(value);
    return value;
}

/* Self-Introspection Primitives */

/* ⊙ - type-of */
Cell* prim_type_of(Cell* args) {
    Cell* value = arg1(args);
    const char* type_name = "unknown";

    if (cell_is_number(value)) type_name = "number";
    else if (cell_is_bool(value)) type_name = "bool";
    else if (cell_is_symbol(value)) type_name = "symbol";
    else if (cell_is_nil(value)) type_name = "nil";
    else if (cell_is_pair(value)) type_name = "pair";
    else if (cell_is_lambda(value)) type_name = "lambda";
    else if (cell_is_error(value)) type_name = "error";
    else if (cell_is_actor(value)) type_name = "actor";
    else if (cell_is_channel(value)) type_name = "channel";

    return cell_symbol(type_name);
}

/* ⧉ - arity (for lambdas) */
Cell* prim_arity(Cell* args) {
    Cell* fn = arg1(args);
    if (cell_is_lambda(fn)) {
        return cell_number((double)fn->data.lambda.arity);
    }
    return cell_error("not-a-lambda", fn);
}

/* ⊛ - source (get lambda body) */
Cell* prim_source(Cell* args) {
    Cell* fn = arg1(args);
    if (cell_is_lambda(fn)) {
        Cell* body = fn->data.lambda.body;
        cell_retain(body);
        return body;
    }
    return cell_error("not-a-lambda", fn);
}

/* Macro System Primitives (Day 70) */

/* ⊛⊙ - gensym (generate unique symbol for hygiene) */
Cell* prim_gensym(Cell* args) {
    /* (⊛⊙) - Generate unique symbol with default prefix "g" */
    /* (⊛⊙ "prefix") - Generate unique symbol with custom prefix */

    if (cell_is_nil(args)) {
        return macro_gensym(NULL);
    }

    Cell* prefix = arg1(args);
    if (cell_is_string(prefix)) {
        return macro_gensym(cell_get_string(prefix));
    }
    if (cell_is_symbol(prefix)) {
        return macro_gensym(cell_get_symbol(prefix));
    }

    return cell_error("gensym-prefix-must-be-string", prefix);
}

/* ⧉→ - macro-expand (show expansion for debugging) */
Cell* prim_macro_expand(Cell* args) {
    /* (⧉→ expr) - Expand macros in expression once */
    /* (⧉→ expr #t) - Expand macros fully */

    if (cell_is_nil(args)) {
        return cell_error("macro-expand-missing-expr", cell_nil());
    }

    Cell* expr = arg1(args);
    Cell* rest = cell_cdr(args);

    /* Create temporary context for expansion */
    EvalContext* ctx = eval_context_new();

    Cell* result;
    if (!cell_is_nil(rest) && cell_is_bool(arg1(rest)) && cell_get_bool(arg1(rest))) {
        /* Full expansion */
        result = macro_expand(expr, ctx);
    } else {
        /* Single-step expansion */
        result = macro_expand_once(expr, ctx);
    }

    eval_context_free(ctx);
    return result;
}

/* ⧉? - macro-list (list all defined macros) */
Cell* prim_macro_list(Cell* args) {
    /* (⧉?) - List all defined macro names */
    (void)args;  /* Unused */
    return macro_list();
}

/* Testing Primitives */

/* ≟ - deep-equal (structural equality) */
Cell* prim_deep_equal(Cell* args) {
    Cell* a = arg1(args);
    Cell* b = arg2(args);
    return cell_bool(cell_equal(a, b));
}

/* ⊨ - test-case */
Cell* prim_test_case(Cell* args) {
    Cell* name = arg1(args);
    Cell* expected = arg2(args);
    Cell* actual = cell_is_pair(cell_cdr(cell_cdr(args))) ?
        cell_car(cell_cdr(cell_cdr(args))) : cell_nil();

    printf("⊨ Test: ");
    cell_print(name);

    if (cell_equal(expected, actual)) {
        printf(" ✓ PASS\n");
        return cell_bool(true);
    } else {
        printf(" ✗ FAIL\n");
        printf("  Expected: ");
        cell_print(expected);
        printf("\n  Actual:   ");
        cell_print(actual);
        printf("\n");
        return cell_error("test-failed", name);
    }
}

/* Property-Based Testing */

/* Helper: shrink integer value toward zero */
static Cell* shrink_int(Cell* value) {
    if (!cell_is_number(value)) return value;

    double num = cell_get_number(value);
    if (num == 0) return value;

    /* Shrink toward zero by halving */
    double shrunk = num / 2.0;
    if (fabs(shrunk) < 1.0 && num != 0) {
        shrunk = (num > 0) ? 0 : 0;
    }

    return cell_number(shrunk);
}

/* Helper: shrink list by removing elements */
static Cell* shrink_list(Cell* list) {
    if (!cell_is_pair(list)) return list;

    /* Try removing first element */
    Cell* tail = cell_cdr(list);
    cell_retain(tail);
    return tail;
}

/* ⊨-prop - property-based test with shrinking */
Cell* prim_test_property(Cell* args) {
    Cell* name = arg1(args);
    Cell* predicate = arg2(args);
    Cell* generator = arg3(args);

    /* Optional: number of test cases (default 100) */
    Cell* num_tests_cell = (cell_is_pair(cell_cdr(cell_cdr(cell_cdr(args)))) &&
                            cell_is_pair(cell_cdr(cell_cdr(cell_cdr(args))))) ?
                           cell_car(cell_cdr(cell_cdr(cell_cdr(args)))) : NULL;

    int num_tests = num_tests_cell && cell_is_number(num_tests_cell) ?
                    (int)cell_get_number(num_tests_cell) : 100;

    if (!cell_is_lambda(predicate)) {
        return cell_error("property-not-function", predicate);
    }

    if (!cell_is_lambda(generator)) {
        return cell_error("generator-not-function", generator);
    }

    /* Get eval context */
    EvalContext* ctx = eval_get_current_context();
    if (!ctx) {
        return cell_error("no-context", cell_nil());
    }

    printf("⊨-prop Property Test: ");
    cell_print(name);
    printf(" (%d cases)\n", num_tests);

    int passed = 0;
    int failed = 0;
    Cell* failing_input = NULL;

    /* Run test cases */
    for (int i = 0; i < num_tests; i++) {
        /* Generate test input: evaluate generator lambda body directly */
        Cell* input = eval_internal(ctx, generator->data.lambda.env, generator->data.lambda.body);

        if (cell_is_error(input)) {
            printf("  Generator error: ");
            cell_print(input);
            printf("\n");
            cell_release(input);
            return cell_error("generator-failed", name);
        }

        /* Apply predicate: manually apply lambda to input */
        /* Create argument list */
        Cell* pred_args = cell_cons(input, cell_nil());

        /* Extend predicate's closure environment with argument */
        Cell* pred_env = extend_env(predicate->data.lambda.env, pred_args);

        /* Evaluate predicate body in extended environment */
        Cell* result = eval_internal(ctx, pred_env, predicate->data.lambda.body);

        cell_release(pred_args);
        cell_release(pred_env);

        if (cell_is_error(result)) {
            printf("  Predicate error on input: ");
            cell_print(input);
            printf("\n  Error: ");
            cell_print(result);
            printf("\n");
            cell_release(input);
            cell_release(result);
            return cell_error("predicate-failed", name);
        }

        /* Check result */
        if (cell_is_bool(result) && cell_get_bool(result)) {
            passed++;
        } else {
            failed++;
            if (failing_input == NULL) {
                failing_input = input;
                cell_retain(failing_input);
            }
        }

        cell_release(input);
        cell_release(result);

        /* Stop after first failure for shrinking */
        if (failed > 0) break;
    }

    /* Report results */
    if (failed == 0) {
        printf("  ✓ PASS: %d/%d tests passed\n", passed, num_tests);
        return cell_bool(true);
    } else {
        printf("  ✗ FAIL: Found failing case after %d tests\n", passed);

        /* Try to shrink failing input */
        printf("  Shrinking...\n");
        Cell* shrunk = failing_input;
        cell_retain(shrunk);

        int shrink_steps = 0;
        const int max_shrink = 100;

        while (shrink_steps < max_shrink) {
            Cell* candidate = NULL;

            /* Try different shrinking strategies based on type */
            if (cell_is_number(shrunk)) {
                candidate = shrink_int(shrunk);
            } else if (cell_is_pair(shrunk)) {
                candidate = shrink_list(shrunk);
            } else {
                break;  /* Can't shrink this type */
            }

            if (candidate == shrunk || cell_equal(candidate, shrunk)) {
                cell_release(candidate);
                break;  /* No more shrinking possible */
            }

            /* Test shrunk value: manually apply predicate */
            Cell* shrink_args = cell_cons(candidate, cell_nil());
            Cell* shrink_env = extend_env(predicate->data.lambda.env, shrink_args);
            Cell* result = eval_internal(ctx, shrink_env, predicate->data.lambda.body);
            cell_release(shrink_args);
            cell_release(shrink_env);

            if (cell_is_bool(result) && cell_get_bool(result)) {
                /* Shrunk value passes, keep original */
                cell_release(candidate);
                cell_release(result);
                break;
            } else {
                /* Shrunk value still fails, use it */
                cell_release(shrunk);
                shrunk = candidate;
                shrink_steps++;
            }

            cell_release(result);
        }

        printf("  Minimal failing input: ");
        cell_print(shrunk);
        printf("\n");

        Cell* error = cell_error("property-failed", name);
        cell_release(shrunk);
        cell_release(failing_input);
        return error;
    }
}

/* Effect primitives
 * ⟪, ⟪⟫, ↯, ⟪?, ⟪→ are special forms in eval.c
 * ⤴ and ≫ are primitives (work with evaluated args)
 */

Cell* prim_effect_pure(Cell* args) {
    /* ⤴ - pure lift: returns value unchanged */
    Cell* result = arg1(args);
    cell_retain(result);
    return result;
}

Cell* prim_effect_bind(Cell* args) {
    /* ≫ - effect bind/sequence: (≫ val fn) applies fn to val */
    Cell* val = arg1(args);
    Cell* fn = arg2(args);

    if (fn->type != CELL_LAMBDA && fn->type != CELL_BUILTIN) {
        return cell_error("bind-requires-function", fn);
    }

    /* Apply fn to val */
    cell_retain(val);
    Cell* apply_args = cell_cons(val, cell_nil());

    if (fn->type == CELL_BUILTIN) {
        Cell* (*builtin_fn)(Cell*) = (Cell* (*)(Cell*))fn->data.atom.builtin;
        Cell* result = builtin_fn(apply_args);
        cell_release(apply_args);
        return result;
    }

    /* Lambda application */
    Cell* closure_env = fn->data.lambda.env;
    Cell* body = fn->data.lambda.body;

    /* Extend closure env with the value */
    extern Cell* extend_env(Cell* env, Cell* args);
    Cell* new_env = extend_env(closure_env, apply_args);
    cell_release(apply_args);

    /* Evaluate body in extended env */
    extern Cell* eval_internal(EvalContext* ctx, Cell* env, Cell* expr);
    EvalContext* ectx = eval_get_current_context();
    cell_retain(body);
    Cell* result = eval_internal(ectx, new_env, body);
    cell_release(new_env);
    cell_release(body);
    return result;
}

/* ============ Actor Primitives ============ */

/* ⟳ - spawn actor
 * (⟳ behavior) where behavior is (λ (self) ...)
 * Creates actor, passes self-reference as actor cell.
 * The behavior body should use ←? to receive messages. */
Cell* prim_spawn(Cell* args) {
    Cell* behavior = arg1(args);
    if (!cell_is_lambda(behavior)) {
        return cell_error("spawn-not-lambda", behavior);
    }

    EvalContext* ctx = eval_get_current_context();

    /* We need a fiber body that applies behavior to self-actor-cell.
     * Since the evaluator resolves named symbols via ctx->env,
     * we define unique bindings there per actor. */

    /* Create actor first to get the ID */
    Cell* placeholder = cell_nil();
    Actor* actor = actor_create(ctx, placeholder, ctx->env);
    cell_release(placeholder);

    if (!actor) {
        return cell_error("max-actors-exceeded", cell_nil());
    }

    /* Build unique symbol names using actor ID */
    char fn_name[64], self_name[64];
    snprintf(fn_name, sizeof(fn_name), "__actor_fn_%d", actor->id);
    snprintf(self_name, sizeof(self_name), "__actor_self_%d", actor->id);

    /* Define behavior and self in ctx->env */
    Cell* self_cell = cell_actor(actor->id);
    eval_define(ctx, fn_name, behavior);
    eval_define(ctx, self_name, self_cell);
    cell_release(self_cell);

    /* Build body expression: (__actor_fn_N __actor_self_N) */
    Cell* fn_sym = cell_symbol(fn_name);
    Cell* self_sym = cell_symbol(self_name);
    Cell* body = cell_cons(fn_sym, cell_cons(self_sym, cell_nil()));
    cell_release(fn_sym);
    cell_release(self_sym);

    /* Update fiber body */
    cell_release(actor->fiber->body);
    actor->fiber->body = body;
    cell_retain(body);
    cell_release(body);

    return cell_actor(actor->id);
}

/* →! - send message to actor
 * (→! actor message) */
Cell* prim_send(Cell* args) {
    Cell* target = arg1(args);
    Cell* message = arg2(args);

    if (!cell_is_actor(target)) {
        return cell_error("send-not-actor", target);
    }

    int id = cell_get_actor_id(target);
    Actor* actor = actor_lookup(id);
    if (!actor || !actor->alive) {
        return cell_error("dead-actor", target);
    }

    actor_send(actor, message);
    return cell_nil();
}

/* ←? - receive message
 * (←?) — dequeues from current actor's mailbox.
 * If empty, yields the fiber (suspends the actor). */
Cell* prim_receive(Cell* args) {
    (void)args;

    Actor* actor = actor_current();
    if (!actor) {
        return cell_error("receive-no-actor", cell_nil());
    }

    /* Check mailbox */
    Cell* msg = actor_receive(actor);
    if (msg) {
        return msg;
    }

    /* Empty mailbox — yield fiber to scheduler */
    Fiber* fiber = actor->fiber;
    if (fiber) {
        fiber->suspend_reason = SUSPEND_MAILBOX;
        fiber_yield(fiber);
        /* Resumed by scheduler with message as resume_value */
        Cell* resumed = fiber->resume_value;
        if (resumed) {
            cell_retain(resumed);
            return resumed;
        }
    }

    return cell_nil();
}

/* ⟳! - run scheduler
 * (⟳! max-ticks) — run cooperative round-robin scheduler */
Cell* prim_actor_run(Cell* args) {
    Cell* max_cell = arg1(args);
    if (!cell_is_number(max_cell)) {
        return cell_error("run-not-number", max_cell);
    }
    int max_ticks = (int)cell_get_number(max_cell);
    int ran = actor_run_all(max_ticks);
    return cell_number((double)ran);
}

/* ⟳? - check if actor is alive
 * (⟳? actor) → #t or #f */
Cell* prim_actor_alive(Cell* args) {
    Cell* target = arg1(args);
    if (!cell_is_actor(target)) {
        return cell_error("alive-not-actor", target);
    }
    int id = cell_get_actor_id(target);
    Actor* actor = actor_lookup(id);
    if (!actor) return cell_bool(false);
    return cell_bool(actor->alive);
}

/* ⟳→ - get finished actor's result
 * (⟳→ actor) → result or error */
Cell* prim_actor_result(Cell* args) {
    Cell* target = arg1(args);
    if (!cell_is_actor(target)) {
        return cell_error("result-not-actor", target);
    }
    int id = cell_get_actor_id(target);
    Actor* actor = actor_lookup(id);
    if (!actor) {
        return cell_error("actor-not-found", target);
    }
    if (actor->alive) {
        return cell_error("actor-still-running", target);
    }
    if (actor->result) {
        cell_retain(actor->result);
        return actor->result;
    }
    return cell_nil();
}

/* ⟳∅ - reset all actors (for testing) */
Cell* prim_actor_reset(Cell* args) {
    (void)args;
    actor_reset_all();
    return cell_nil();
}

/* ============ Channel Primitives ============ */

/* ⟿⊚ - create channel
 * (⟿⊚) or (⟿⊚ capacity) — create bounded channel */
Cell* prim_chan_create(Cell* args) {
    int capacity = DEFAULT_CHANNEL_CAPACITY;
    if (args && !cell_is_nil(args)) {
        Cell* cap_cell = cell_car(args);
        if (cell_is_number(cap_cell)) {
            capacity = (int)cell_get_number(cap_cell);
            if (capacity <= 0) capacity = DEFAULT_CHANNEL_CAPACITY;
        }
    }

    Channel* chan = channel_create(capacity);
    if (!chan) {
        return cell_error("channel-limit", cell_nil());
    }
    return cell_channel(chan->id);
}

/* ⟿→ - send to channel
 * (⟿→ chan value) — send value, yields if buffer full */
Cell* prim_chan_send(Cell* args) {
    if (!args || cell_is_nil(args)) {
        return cell_error("chan-send-args", cell_nil());
    }
    Cell* chan_cell = cell_car(args);
    if (!cell_is_channel(chan_cell)) {
        return cell_error("chan-send-not-channel", chan_cell);
    }

    Cell* rest = cell_cdr(args);
    if (!rest || cell_is_nil(rest)) {
        return cell_error("chan-send-no-value", cell_nil());
    }
    Cell* value = cell_car(rest);

    int chan_id = cell_get_channel_id(chan_cell);
    Channel* chan = channel_lookup(chan_id);
    if (!chan) {
        return cell_error("chan-send-invalid", cell_nil());
    }
    if (chan->closed) {
        return cell_error("chan-send-closed", cell_nil());
    }

    /* Try non-blocking send */
    if (channel_try_send(chan, value)) {
        return cell_nil();
    }

    /* Buffer full — yield fiber to scheduler */
    Actor* actor = actor_current();
    if (actor && actor->fiber) {
        Fiber* fiber = actor->fiber;
        fiber->suspend_reason = SUSPEND_CHAN_SEND;
        fiber->suspend_channel_id = chan_id;
        fiber->suspend_send_value = value;
        cell_retain(value);
        fiber_yield(fiber);
        /* Resumed by scheduler after successful send */
        return cell_nil();
    }

    return cell_error("chan-send-full", cell_nil());
}

/* ⟿← - receive from channel
 * (⟿← chan) — receive value, yields if buffer empty */
Cell* prim_chan_recv(Cell* args) {
    if (!args || cell_is_nil(args)) {
        return cell_error("chan-recv-args", cell_nil());
    }
    Cell* chan_cell = cell_car(args);
    if (!cell_is_channel(chan_cell)) {
        return cell_error("chan-recv-not-channel", chan_cell);
    }

    int chan_id = cell_get_channel_id(chan_cell);
    Channel* chan = channel_lookup(chan_id);
    if (!chan) {
        return cell_error("chan-recv-invalid", cell_nil());
    }

    /* Try non-blocking recv */
    Cell* value = channel_try_recv(chan);
    if (value) {
        return value;
    }

    /* Empty and closed → error */
    if (chan->closed) {
        return cell_error("chan-recv-closed", cell_nil());
    }

    /* Empty — yield fiber to scheduler */
    Actor* actor = actor_current();
    if (actor && actor->fiber) {
        Fiber* fiber = actor->fiber;
        fiber->suspend_reason = SUSPEND_CHAN_RECV;
        fiber->suspend_channel_id = chan_id;
        fiber_yield(fiber);
        /* Resumed by scheduler with received value */
        Cell* resumed = fiber->resume_value;
        if (resumed) {
            cell_retain(resumed);
            return resumed;
        }
    }

    return cell_error("chan-recv-empty", cell_nil());
}

/* ⟿× - close channel
 * (⟿× chan) — close channel, no more sends */
Cell* prim_chan_close(Cell* args) {
    if (!args || cell_is_nil(args)) {
        return cell_error("chan-close-args", cell_nil());
    }
    Cell* chan_cell = cell_car(args);
    if (!cell_is_channel(chan_cell)) {
        return cell_error("chan-close-not-channel", chan_cell);
    }

    int chan_id = cell_get_channel_id(chan_cell);
    Channel* chan = channel_lookup(chan_id);
    if (!chan) {
        return cell_error("chan-close-invalid", cell_nil());
    }

    channel_close(chan);
    return cell_nil();
}

/* ⟿∅ - reset all channels (for testing) */
Cell* prim_chan_reset(Cell* args) {
    (void)args;
    channel_reset_all();
    return cell_nil();
}

/* ⟿⊞ - select from multiple channels (blocking)
 * (⟿⊞ ch1 ch2 ...) — wait for first channel with data
 * Returns ⟨channel value⟩ pair */
Cell* prim_chan_select(Cell* args) {
    if (!args || cell_is_nil(args)) {
        return cell_error("select-no-args", cell_nil());
    }

    /* Collect channel IDs */
    int ids[MAX_SELECT_CHANNELS];
    int count = 0;
    Cell* cur = args;
    while (cur && !cell_is_nil(cur) && count < MAX_SELECT_CHANNELS) {
        Cell* ch = cell_car(cur);
        if (!cell_is_channel(ch)) {
            return cell_error("select-not-channel", ch);
        }
        ids[count++] = cell_get_channel_id(ch);
        cur = cell_cdr(cur);
    }

    /* First pass: try_recv with round-robin fairness */
    static int rr_counter = 0;
    int start = rr_counter % count;
    rr_counter++;
    int closed_empty = 0;
    for (int j = 0; j < count; j++) {
        int idx = (start + j) % count;
        Channel* chan = channel_lookup(ids[idx]);
        if (!chan || (chan->closed && chan->count == 0)) {
            closed_empty++;
            continue;
        }
        Cell* val = channel_try_recv(chan);
        if (val) {
            Cell* result = cell_cons(cell_channel(ids[idx]), val);
            cell_release(val);
            return result;
        }
    }

    /* All closed+empty → error */
    if (closed_empty == count) {
        return cell_error("select-all-closed", cell_nil());
    }

    /* No data — yield fiber to scheduler */
    Actor* actor = actor_current();
    if (actor && actor->fiber) {
        Fiber* fiber = actor->fiber;
        fiber->suspend_reason = SUSPEND_SELECT;
        fiber->suspend_select_count = count;
        for (int i = 0; i < count; i++) {
            fiber->suspend_select_ids[i] = ids[i];
        }
        fiber_yield(fiber);
        /* Resumed by scheduler with ⟨channel value⟩ pair */
        Cell* resumed = fiber->resume_value;
        if (resumed) {
            cell_retain(resumed);
            return resumed;
        }
    }

    return cell_error("select-no-actor", cell_nil());
}

/* ⟿⊞? - try select from multiple channels (non-blocking)
 * (⟿⊞? ch1 ch2 ...) — return first ready or ∅
 * Returns ⟨channel value⟩ pair or ∅ */
Cell* prim_chan_select_try(Cell* args) {
    if (!args || cell_is_nil(args)) {
        return cell_error("select-no-args", cell_nil());
    }

    /* Collect channel IDs */
    int ids[MAX_SELECT_CHANNELS];
    int count = 0;
    Cell* cur = args;
    while (cur && !cell_is_nil(cur) && count < MAX_SELECT_CHANNELS) {
        Cell* ch = cell_car(cur);
        if (!cell_is_channel(ch)) {
            return cell_error("select-not-channel", ch);
        }
        ids[count++] = cell_get_channel_id(ch);
        cur = cell_cdr(cur);
    }

    /* Try recv with round-robin fairness */
    static int rr_counter = 0;
    int start = rr_counter % count;
    rr_counter++;
    int closed_empty = 0;
    for (int j = 0; j < count; j++) {
        int idx = (start + j) % count;
        Channel* chan = channel_lookup(ids[idx]);
        if (!chan || (chan->closed && chan->count == 0)) {
            closed_empty++;
            continue;
        }
        Cell* val = channel_try_recv(chan);
        if (val) {
            Cell* result = cell_cons(cell_channel(ids[idx]), val);
            cell_release(val);
            return result;
        }
    }

    /* All closed → error; otherwise just nothing ready */
    if (closed_empty == count) {
        return cell_error("select-all-closed", cell_nil());
    }

    return cell_nil();
}

/* ============ Structure Primitives ============ */

/* Need access to eval context for type registry */
extern EvalContext* eval_get_current_context(void);

/* ⊙≔ - Define leaf structure type
 * Args: type_tag (symbol) followed by field names (symbols)
 * Example: (⊙≔ :Point :x :y)
 * Returns: type_tag
 */
Cell* prim_struct_define_leaf(Cell* args) {
    if (!cell_is_pair(args)) {
        return cell_error("⊙≔ requires at least a type tag", cell_nil());
    }

    Cell* type_tag = arg1(args);
    if (!cell_is_symbol(type_tag)) {
        return cell_error("⊙≔ type tag must be a symbol", type_tag);
    }

    /* Collect field names from remaining args */
    Cell* fields = cell_nil();
    Cell* rest = cell_cdr(args);
    while (cell_is_pair(rest)) {
        Cell* field = cell_car(rest);
        if (!cell_is_symbol(field)) {
            cell_release(fields);
            return cell_error("⊙≔ field names must be symbols", field);
        }
        cell_retain(field);
        fields = cell_cons(field, fields);
        rest = cell_cdr(rest);
    }

    /* Reverse fields to preserve order */
    Cell* reversed = cell_nil();
    while (cell_is_pair(fields)) {
        Cell* field = cell_car(fields);
        cell_retain(field);
        reversed = cell_cons(field, reversed);
        Cell* next = cell_cdr(fields);
        cell_release(fields);
        fields = next;
    }

    /* Create schema: ⟨:leaf fields⟩ */
    Cell* kind_tag = cell_symbol(":leaf");
    Cell* schema = cell_cons(kind_tag, reversed);

    /* Register in type registry */
    EvalContext* ctx = eval_get_current_context();
    eval_register_type(ctx, type_tag, schema);

    /* Return the type tag */
    cell_retain(type_tag);
    return type_tag;
}

/* ⊙ - Create leaf structure instance
 * Args: type_tag followed by field values
 * Example: (⊙ :Point #3 #4)
 * Returns: struct cell
 */
Cell* prim_struct_create(Cell* args) {
    if (!cell_is_pair(args)) {
        return cell_error("⊙ requires type tag", cell_nil());
    }

    Cell* type_tag = arg1(args);
    if (!cell_is_symbol(type_tag)) {
        return cell_error("⊙ type tag must be a symbol", type_tag);
    }

    /* Lookup type schema */
    EvalContext* ctx = eval_get_current_context();
    Cell* schema = eval_lookup_type(ctx, type_tag);
    if (!schema) {
        return cell_error("⊙ undefined type", type_tag);
    }

    /* Extract field names from schema: ⟨:leaf fields⟩ */
    if (!cell_is_pair(schema)) {
        cell_release(schema);
        return cell_error("⊙ invalid schema", type_tag);
    }

    Cell* field_names = cell_cdr(schema);  /* Skip :leaf tag */

    /* Collect field values from args */
    Cell* rest = cell_cdr(args);  /* Skip type tag */
    Cell* field_pairs = cell_nil();

    Cell* names = field_names;
    while (cell_is_pair(names) && cell_is_pair(rest)) {
        Cell* field_name = cell_car(names);
        Cell* field_value = cell_car(rest);

        /* Create (name . value) pair */
        cell_retain(field_name);
        cell_retain(field_value);
        Cell* pair = cell_cons(field_name, field_value);
        field_pairs = cell_cons(pair, field_pairs);

        names = cell_cdr(names);
        rest = cell_cdr(rest);
    }

    /* Check we got all fields */
    if (cell_is_pair(names)) {
        cell_release(schema);
        cell_release(field_pairs);
        return cell_error("⊙ not enough field values", type_tag);
    }
    if (cell_is_pair(rest)) {
        cell_release(schema);
        cell_release(field_pairs);
        return cell_error("⊙ too many field values", type_tag);
    }

    /* Reverse field_pairs to preserve order */
    Cell* reversed = cell_nil();
    while (cell_is_pair(field_pairs)) {
        Cell* pair = cell_car(field_pairs);
        cell_retain(pair);
        reversed = cell_cons(pair, reversed);
        Cell* next = cell_cdr(field_pairs);
        cell_release(field_pairs);
        field_pairs = next;
    }

    /* Create structure cell */
    cell_retain(type_tag);
    Cell* result = cell_struct(STRUCT_LEAF, type_tag, NULL, reversed);

    cell_release(schema);
    return result;
}

/* ⊙→ - Get field value from leaf structure
 * Args: struct, field_name
 * Example: (⊙→ point :x)
 * Returns: field value
 */
Cell* prim_struct_get_field(Cell* args) {
    if (!cell_is_pair(args)) {
        return cell_error("⊙→ requires struct and field name", cell_nil());
    }

    Cell* structure = arg1(args);
    if (!cell_is_struct(structure)) {
        return cell_error("⊙→ first arg must be struct", structure);
    }

    Cell* rest = cell_cdr(args);
    if (!cell_is_pair(rest)) {
        return cell_error("⊙→ requires field name", cell_nil());
    }

    Cell* field_name = cell_car(rest);
    if (!cell_is_symbol(field_name)) {
        return cell_error("⊙→ field name must be symbol", field_name);
    }

    /* Use existing accessor */
    Cell* value = cell_struct_get_field(structure, field_name);
    if (!value) {
        return cell_error("⊙→ field not found", field_name);
    }

    return value;
}

/* ⊙← - Update field in leaf structure (immutable, returns new struct)
 * Args: struct, field_name, new_value
 * Example: (⊙← point :x #5)
 * Returns: new struct with updated field
 */
Cell* prim_struct_update_field(Cell* args) {
    if (!cell_is_pair(args)) {
        return cell_error("⊙← requires struct, field name, and value", cell_nil());
    }

    Cell* structure = arg1(args);
    if (!cell_is_struct(structure)) {
        return cell_error("⊙← first arg must be struct", structure);
    }

    Cell* rest = cell_cdr(args);
    if (!cell_is_pair(rest)) {
        return cell_error("⊙← requires field name and value", cell_nil());
    }

    Cell* field_name = cell_car(rest);
    if (!cell_is_symbol(field_name)) {
        return cell_error("⊙← field name must be symbol", field_name);
    }

    Cell* rest2 = cell_cdr(rest);
    if (!cell_is_pair(rest2)) {
        return cell_error("⊙← requires new value", cell_nil());
    }

    Cell* new_value = cell_car(rest2);

    /* Build new field list with updated value */
    Cell* old_fields = cell_struct_fields(structure);
    Cell* new_fields = cell_nil();
    bool found = false;

    /* Copy fields, replacing the one being updated */
    Cell* curr = old_fields;
    while (cell_is_pair(curr)) {
        Cell* pair = cell_car(curr);  /* (field_name . value) */
        Cell* name = cell_car(pair);

        if (cell_equal(name, field_name)) {
            /* This is the field to update */
            cell_retain(name);
            cell_retain(new_value);
            Cell* new_pair = cell_cons(name, new_value);
            new_fields = cell_cons(new_pair, new_fields);
            found = true;
        } else {
            /* Keep existing field */
            cell_retain(pair);
            new_fields = cell_cons(pair, new_fields);
        }

        curr = cell_cdr(curr);
    }

    if (!found) {
        cell_release(new_fields);
        return cell_error("⊙← field not found", field_name);
    }

    /* Reverse to maintain order */
    Cell* reversed = cell_nil();
    while (cell_is_pair(new_fields)) {
        Cell* pair = cell_car(new_fields);
        cell_retain(pair);
        reversed = cell_cons(pair, reversed);
        Cell* next = cell_cdr(new_fields);
        cell_release(new_fields);
        new_fields = next;
    }

    /* Create new structure with updated fields */
    Cell* type_tag = cell_struct_type_tag(structure);
    cell_retain(type_tag);
    Cell* result = cell_struct(STRUCT_LEAF, type_tag, NULL, reversed);

    return result;
}

/* ⊙? - Check if value is structure of given type
 * Args: value, type_tag
 * Example: (⊙? point :Point)
 * Returns: #t or #f
 */
Cell* prim_struct_type_check(Cell* args) {
    if (!cell_is_pair(args)) {
        return cell_bool(false);
    }

    Cell* value = arg1(args);
    if (!cell_is_struct(value)) {
        return cell_bool(false);
    }

    Cell* rest = cell_cdr(args);
    if (!cell_is_pair(rest)) {
        return cell_bool(false);
    }

    Cell* expected_type = cell_car(rest);
    if (!cell_is_symbol(expected_type)) {
        return cell_error("⊙? type tag must be symbol", expected_type);
    }

    Cell* actual_type = cell_struct_type_tag(value);
    return cell_bool(cell_equal(actual_type, expected_type));
}

/* ============ Node/ADT Structure Primitives ============ */

/* ⊚≔ - Define node/ADT structure type with variants
 * Args: type_tag (symbol) followed by variant definitions
 * Each variant: [variant_tag field1 field2 ...]
 * Example: (⊚≔ :List [:Nil] [:Cons :head :tail])
 * Returns: type_tag
 */
Cell* prim_struct_define_node(Cell* args) {
    if (!cell_is_pair(args)) {
        return cell_error("⊚≔ requires at least a type tag", cell_nil());
    }

    Cell* type_tag = arg1(args);
    if (!cell_is_symbol(type_tag)) {
        return cell_error("⊚≔ type tag must be a symbol", type_tag);
    }

    /* Collect variant definitions from remaining args */
    Cell* variants = cell_nil();
    Cell* rest = cell_cdr(args);

    while (cell_is_pair(rest)) {
        Cell* variant_def = cell_car(rest);

        /* Each variant should be a list [variant_tag field1 field2 ...] */
        if (!cell_is_pair(variant_def)) {
            cell_release(variants);
            return cell_error("⊚≔ each variant must be a list", variant_def);
        }

        Cell* variant_tag = cell_car(variant_def);
        if (!cell_is_symbol(variant_tag)) {
            cell_release(variants);
            return cell_error("⊚≔ variant tag must be a symbol", variant_tag);
        }

        /* Collect field names for this variant */
        Cell* fields = cell_nil();
        Cell* field_rest = cell_cdr(variant_def);
        while (cell_is_pair(field_rest)) {
            Cell* field = cell_car(field_rest);
            if (!cell_is_symbol(field)) {
                cell_release(fields);
                cell_release(variants);
                return cell_error("⊚≔ field names must be symbols", field);
            }
            cell_retain(field);
            fields = cell_cons(field, fields);
            field_rest = cell_cdr(field_rest);
        }

        /* Reverse fields to preserve order */
        Cell* reversed_fields = cell_nil();
        while (cell_is_pair(fields)) {
            Cell* field = cell_car(fields);
            cell_retain(field);
            reversed_fields = cell_cons(field, reversed_fields);
            Cell* next = cell_cdr(fields);
            cell_release(fields);
            fields = next;
        }

        /* Create variant schema: ⟨variant_tag fields⟩ */
        cell_retain(variant_tag);
        Cell* variant_schema = cell_cons(variant_tag, reversed_fields);

        /* Add to variants list */
        variants = cell_cons(variant_schema, variants);
        rest = cell_cdr(rest);
    }

    /* Check that at least one variant was provided */
    if (cell_is_nil(variants)) {
        return cell_error("⊚≔ requires at least one variant", type_tag);
    }

    /* Reverse variants to preserve order */
    Cell* reversed_variants = cell_nil();
    while (cell_is_pair(variants)) {
        Cell* variant = cell_car(variants);
        cell_retain(variant);
        reversed_variants = cell_cons(variant, reversed_variants);
        Cell* next = cell_cdr(variants);
        cell_release(variants);
        variants = next;
    }

    /* Create schema: ⟨:node variants⟩ */
    Cell* kind_tag = cell_symbol(":node");
    Cell* schema = cell_cons(kind_tag, reversed_variants);

    /* Register in type registry */
    EvalContext* ctx = eval_get_current_context();
    eval_register_type(ctx, type_tag, schema);

    /* Return the type tag */
    cell_retain(type_tag);
    return type_tag;
}

/* ⊚ - Create node/ADT structure instance
 * Args: type_tag variant_tag field_values...
 * Example: (⊚ :List :Cons #1 nil-list)
 * Returns: struct cell with variant
 */
Cell* prim_struct_create_node(Cell* args) {
    if (!cell_is_pair(args)) {
        return cell_error("⊚ requires type tag", cell_nil());
    }

    Cell* type_tag = arg1(args);
    if (!cell_is_symbol(type_tag)) {
        return cell_error("⊚ type tag must be a symbol", type_tag);
    }

    if (!cell_is_pair(cell_cdr(args))) {
        return cell_error("⊚ requires variant tag", type_tag);
    }

    Cell* variant_tag = arg2(args);
    if (!cell_is_symbol(variant_tag)) {
        return cell_error("⊚ variant tag must be a symbol", variant_tag);
    }

    /* Lookup type schema */
    EvalContext* ctx = eval_get_current_context();
    Cell* schema = eval_lookup_type(ctx, type_tag);
    if (!schema) {
        return cell_error("⊚ undefined type", type_tag);
    }

    /* Extract variants from schema: ⟨:node variants⟩ */
    if (!cell_is_pair(schema)) {
        cell_release(schema);
        return cell_error("⊚ invalid schema", type_tag);
    }

    Cell* variants = cell_cdr(schema);  /* Skip :node tag */

    /* Find the variant schema */
    Cell* variant_schema = NULL;
    Cell* vlist = variants;
    while (cell_is_pair(vlist)) {
        Cell* variant_def = cell_car(vlist);
        if (cell_is_pair(variant_def)) {
            Cell* vtag = cell_car(variant_def);
            if (cell_equal(vtag, variant_tag)) {
                variant_schema = variant_def;
                break;
            }
        }
        vlist = cell_cdr(vlist);
    }

    if (!variant_schema) {
        cell_release(schema);
        return cell_error("⊚ unknown variant", variant_tag);
    }

    Cell* field_names = cell_cdr(variant_schema);  /* Skip variant tag */

    /* Collect field values from args */
    Cell* rest = cell_cdr(cell_cdr(args));  /* Skip type tag and variant tag */
    Cell* field_pairs = cell_nil();

    Cell* names = field_names;
    while (cell_is_pair(names) && cell_is_pair(rest)) {
        Cell* field_name = cell_car(names);
        Cell* field_value = cell_car(rest);

        /* Create (name . value) pair */
        cell_retain(field_name);
        cell_retain(field_value);
        Cell* pair = cell_cons(field_name, field_value);
        field_pairs = cell_cons(pair, field_pairs);

        names = cell_cdr(names);
        rest = cell_cdr(rest);
    }

    /* Check field count matches */
    if (cell_is_pair(names)) {
        cell_release(field_pairs);
        cell_release(schema);
        return cell_error("⊚ not enough field values", type_tag);
    }
    if (cell_is_pair(rest)) {
        cell_release(field_pairs);
        cell_release(schema);
        return cell_error("⊚ too many field values", type_tag);
    }

    /* Reverse field pairs to preserve order */
    Cell* reversed_pairs = cell_nil();
    while (cell_is_pair(field_pairs)) {
        Cell* pair = cell_car(field_pairs);
        cell_retain(pair);
        reversed_pairs = cell_cons(pair, reversed_pairs);
        Cell* next = cell_cdr(field_pairs);
        cell_release(field_pairs);
        field_pairs = next;
    }

    /* Create struct with variant */
    cell_retain(type_tag);
    cell_retain(variant_tag);
    Cell* result = cell_struct(STRUCT_NODE, type_tag, variant_tag, reversed_pairs);

    cell_release(schema);
    return result;
}

/* ⊚→ - Get field value from node structure
 * Args: struct field_name
 * Example: (⊚→ cons-cell :head)
 * Returns: field value
 */
Cell* prim_struct_get_node(Cell* args) {
    Cell* st = arg1(args);
    Cell* field_name = arg2(args);

    if (!cell_is_struct(st)) {
        return cell_error("⊚→ first arg must be struct", st);
    }

    if (cell_struct_kind(st) != STRUCT_NODE) {
        return cell_error("⊚→ requires node structure", st);
    }

    if (!cell_is_symbol(field_name)) {
        return cell_error("⊚→ field name must be symbol", field_name);
    }

    Cell* value = cell_struct_get_field(st, field_name);
    if (!value) {
        return cell_error("⊚→ field not found", field_name);
    }

    return value;
}

/* ⊚? - Check if value is node structure of given type and variant
 * Args: value type_tag variant_tag
 * Example: (⊚? my-list :List :Cons)
 * Returns: #t or #f
 */
Cell* prim_struct_is_node(Cell* args) {
    Cell* value = arg1(args);
    Cell* expected_type = arg2(args);
    Cell* expected_variant = arg3(args);

    /* Check if it's a struct at all */
    if (!cell_is_struct(value)) {
        return cell_bool(false);
    }

    /* Check if it's a node */
    if (cell_struct_kind(value) != STRUCT_NODE) {
        return cell_bool(false);
    }

    /* Check type tag */
    Cell* actual_type = cell_struct_type_tag(value);
    if (!cell_equal(actual_type, expected_type)) {
        return cell_bool(false);
    }

    /* Check variant tag */
    Cell* actual_variant = cell_struct_variant(value);
    if (!actual_variant || !cell_equal(actual_variant, expected_variant)) {
        return cell_bool(false);
    }

    return cell_bool(true);
}

/* ============ Graph Primitives ============ */

/* ⊝≔ - Define graph structure type
 * Args: type_tag (symbol) graph_type (symbol) followed by field names (symbols)
 * Example: (⊝≔ :Graph :generic :nodes :edges)
 * Returns: type_tag
 */
Cell* prim_graph_define(Cell* args) {
    if (!cell_is_pair(args)) {
        return cell_error("⊝≔ requires at least type tag and graph type", cell_nil());
    }

    Cell* type_tag = arg1(args);
    if (!cell_is_symbol(type_tag)) {
        return cell_error("⊝≔ type tag must be a symbol", type_tag);
    }

    Cell* rest = cell_cdr(args);
    if (!cell_is_pair(rest)) {
        return cell_error("⊝≔ requires graph type", type_tag);
    }

    Cell* graph_type_sym = cell_car(rest);
    if (!cell_is_symbol(graph_type_sym)) {
        return cell_error("⊝≔ graph type must be a symbol", graph_type_sym);
    }

    /* Validate graph type - must be :generic, :cfg, :dfg, :call, or :dep */
    const char* gtype = cell_get_symbol(graph_type_sym);
    if (strcmp(gtype, ":generic") != 0 &&
        strcmp(gtype, ":cfg") != 0 &&
        strcmp(gtype, ":dfg") != 0 &&
        strcmp(gtype, ":call") != 0 &&
        strcmp(gtype, ":dep") != 0) {
        return cell_error("⊝≔ graph type must be :generic, :cfg, :dfg, :call, or :dep", graph_type_sym);
    }

    /* Collect field names from remaining args */
    Cell* fields = cell_nil();
    rest = cell_cdr(rest);
    while (cell_is_pair(rest)) {
        Cell* field = cell_car(rest);
        if (!cell_is_symbol(field)) {
            cell_release(fields);
            return cell_error("⊝≔ field names must be symbols", field);
        }
        cell_retain(field);
        fields = cell_cons(field, fields);
        rest = cell_cdr(rest);
    }

    /* Reverse fields to preserve order */
    Cell* reversed = cell_nil();
    while (cell_is_pair(fields)) {
        Cell* field = cell_car(fields);
        cell_retain(field);
        reversed = cell_cons(field, reversed);
        Cell* next = cell_cdr(fields);
        cell_release(fields);
        fields = next;
    }

    /* Create schema: ⟨:graph ⟨graph_type ⟨fields⟩⟩⟩ */
    cell_retain(graph_type_sym);
    Cell* type_and_fields = cell_cons(graph_type_sym, reversed);
    Cell* kind_tag = cell_symbol(":graph");
    Cell* schema = cell_cons(kind_tag, type_and_fields);

    /* Register in type registry */
    EvalContext* ctx = eval_get_current_context();
    eval_register_type(ctx, type_tag, schema);

    /* Return the type tag */
    cell_retain(type_tag);
    return type_tag;
}

/* ⊝ - Create empty graph instance
 * Args: type_tag
 * Example: (⊝ :Graph)
 * Returns: graph cell
 */
Cell* prim_graph_create(Cell* args) {
    Cell* type_tag = arg1(args);
    if (!cell_is_symbol(type_tag)) {
        return cell_error("⊝ type tag must be a symbol", type_tag);
    }

    /* Lookup type schema */
    EvalContext* ctx = eval_get_current_context();
    Cell* schema = eval_lookup_type(ctx, type_tag);
    if (!schema) {
        return cell_error("⊝ undefined type", type_tag);
    }

    /* Extract graph type from schema: ⟨:graph ⟨graph_type fields⟩⟩ */
    if (!cell_is_pair(schema)) {
        cell_release(schema);
        return cell_error("⊝ invalid schema", type_tag);
    }

    Cell* kind = cell_car(schema);
    const char* kind_str = cell_get_symbol(kind);
    if (strcmp(kind_str, ":graph") != 0) {
        cell_release(schema);
        return cell_error("⊝ type is not a graph", type_tag);
    }

    Cell* type_and_fields = cell_cdr(schema);
    if (!cell_is_pair(type_and_fields)) {
        cell_release(schema);
        return cell_error("⊝ invalid schema format", type_tag);
    }

    Cell* graph_type_sym = cell_car(type_and_fields);
    const char* gtype = cell_get_symbol(graph_type_sym);

    /* Map symbol to GraphType enum */
    GraphType graph_type;
    if (strcmp(gtype, ":generic") == 0) {
        graph_type = GRAPH_GENERIC;
    } else if (strcmp(gtype, ":cfg") == 0) {
        graph_type = GRAPH_CFG;
    } else if (strcmp(gtype, ":dfg") == 0) {
        graph_type = GRAPH_DFG;
    } else if (strcmp(gtype, ":call") == 0) {
        graph_type = GRAPH_CALL;
    } else if (strcmp(gtype, ":dep") == 0) {
        graph_type = GRAPH_DEP;
    } else {
        cell_release(schema);
        return cell_error("⊝ unknown graph type", graph_type_sym);
    }

    /* Create empty graph */
    Cell* result = cell_graph(graph_type, cell_nil(), cell_nil(), cell_nil());

    cell_release(schema);
    return result;
}

/* ⊝⊕ - Add node to graph
 * Args: graph node_data
 * Example: (⊝⊕ g #0)
 * Returns: new graph with node added
 */
Cell* prim_graph_add_node(Cell* args) {
    Cell* graph = arg1(args);
    Cell* node_data = arg2(args);

    if (!cell_is_graph(graph)) {
        return cell_error("⊝⊕ first arg must be graph", graph);
    }

    /* Use cell.c function to add node (returns new graph) */
    Cell* result = cell_graph_add_node(graph, node_data);
    return result;
}

/* ⊝⊗ - Add edge to graph
 * Args: graph from_node to_node label
 * Example: (⊝⊗ g #0 #1 :edge)
 * Returns: new graph with edge added
 */
Cell* prim_graph_add_edge(Cell* args) {
    Cell* graph = arg1(args);
    Cell* from = arg2(args);
    Cell* to = arg3(args);
    Cell* label = arg4(args);

    if (!cell_is_graph(graph)) {
        return cell_error("⊝⊗ first arg must be graph", graph);
    }

    /* Use cell.c function to add edge (returns new graph) */
    Cell* result = cell_graph_add_edge(graph, from, to, label);
    return result;
}

/* ⊝→ - Query graph property
 * Args: graph property_name
 * Example: (⊝→ g :nodes)
 * Returns: requested property
 */
Cell* prim_graph_query(Cell* args) {
    Cell* graph = arg1(args);
    Cell* property = arg2(args);

    if (!cell_is_graph(graph)) {
        return cell_error("⊝→ first arg must be graph", graph);
    }

    if (!cell_is_symbol(property)) {
        return cell_error("⊝→ property must be symbol", property);
    }

    const char* prop = cell_get_symbol(property);

    if (strcmp(prop, ":nodes") == 0) {
        return cell_graph_nodes(graph);
    } else if (strcmp(prop, ":edges") == 0) {
        return cell_graph_edges(graph);
    } else if (strcmp(prop, ":entry") == 0) {
        Cell* entry = cell_graph_entry(graph);
        return entry ? entry : cell_nil();
    } else if (strcmp(prop, ":exit") == 0) {
        Cell* exit = cell_graph_exit(graph);
        return exit ? exit : cell_nil();
    } else if (strcmp(prop, ":metadata") == 0) {
        return cell_graph_metadata(graph);
    } else {
        return cell_error("⊝→ unknown property", property);
    }
}

/* ⊝? - Check if value is graph of given type
 * Args: value type_tag
 * Example: (⊝? g :Graph)
 * Returns: #t or #f
 */
Cell* prim_graph_is(Cell* args) {
    Cell* value = arg1(args);
    Cell* expected_type = arg2(args);

    /* Check if it's a graph at all */
    if (!cell_is_graph(value)) {
        return cell_bool(false);
    }

    /* Check for built-in graph types (CFG, DFG, CALL, DEP) */
    if (cell_is_symbol(expected_type)) {
        const char* type_str = cell_get_symbol(expected_type);
        GraphType gt = value->data.graph.graph_type;

        /* Map built-in graph types to symbols */
        if (strcmp(type_str, ":CFG") == 0) {
            return cell_bool(gt == GRAPH_CFG);
        } else if (strcmp(type_str, ":DFG") == 0) {
            return cell_bool(gt == GRAPH_DFG);
        } else if (strcmp(type_str, ":CALL") == 0 || strcmp(type_str, ":CallGraph") == 0) {
            return cell_bool(gt == GRAPH_CALL);
        } else if (strcmp(type_str, ":DEP") == 0 || strcmp(type_str, ":DepGraph") == 0) {
            return cell_bool(gt == GRAPH_DEP);
        }
    }

    /* Look up expected type schema for user-defined types */
    EvalContext* ctx = eval_get_current_context();
    Cell* schema = eval_lookup_type(ctx, expected_type);
    if (!schema) {
        return cell_bool(false);
    }

    /* Verify it's a graph schema */
    if (!cell_is_pair(schema)) {
        cell_release(schema);
        return cell_bool(false);
    }

    Cell* kind = cell_car(schema);
    const char* kind_str = cell_get_symbol(kind);
    if (strcmp(kind_str, ":graph") != 0) {
        cell_release(schema);
        return cell_bool(false);
    }

    /* For user-defined graph types, check if GENERIC */
    cell_release(schema);
    return cell_bool(value->data.graph.graph_type == GRAPH_GENERIC);
}

/* ⊝↦ - Graph traverse (BFS or DFS)
 * Args: graph :bfs/:dfs start_node visitor_fn
 * Example: (⊝↦ cfg :bfs :entry (λ (n) n))
 * Returns: list of visited nodes in traversal order
 */
Cell* prim_graph_traverse(Cell* args) {
    Cell* graph = arg1(args);
    Cell* mode = arg2(args);
    Cell* start = arg3(args);
    Cell* visitor = arg4(args);

    if (!cell_is_graph(graph)) {
        return cell_error("⊝↦ first arg must be graph", graph);
    }

    if (!cell_is_symbol(mode)) {
        return cell_error("⊝↦ mode must be :bfs or :dfs", mode);
    }

    if (!cell_is_lambda(visitor)) {
        return cell_error("⊝↦ visitor must be function", visitor);
    }

    const char* mode_str = cell_get_symbol(mode);
    bool is_bfs = strcmp(mode_str, ":bfs") == 0;
    bool is_dfs = strcmp(mode_str, ":dfs") == 0;

    if (!is_bfs && !is_dfs) {
        return cell_error("⊝↦ mode must be :bfs or :dfs", mode);
    }

    /* Check if start node exists in graph */
    Cell* nodes = cell_graph_nodes(graph);
    bool start_exists = false;
    Cell* node_iter = nodes;
    while (cell_is_pair(node_iter)) {
        if (cell_equal(cell_car(node_iter), start)) {
            start_exists = true;
            break;
        }
        node_iter = cell_cdr(node_iter);
    }
    if (!start_exists) {
        return cell_nil();  /* Node not in graph, return empty list */
    }

    /* Get graph edges */
    Cell* edges = cell_graph_edges(graph);

    /* Build adjacency list: node -> list of successors */
    /* Use a simple association list */
    Cell* adj_list = cell_nil();

    Cell* edge_iter = edges;
    while (cell_is_pair(edge_iter)) {
        Cell* edge = cell_car(edge_iter);
        /* Edge format: ⟨from ⟨to ⟨label ∅⟩⟩⟩ */
        if (cell_is_pair(edge)) {
            Cell* from = cell_car(edge);
            Cell* rest = cell_cdr(edge);
            if (cell_is_pair(rest)) {
                Cell* to = cell_car(rest);

                /* Find or create entry for 'from' in adj_list */
                Cell* entry = cell_nil();
                Cell* adj_iter = adj_list;
                bool found = false;

                while (cell_is_pair(adj_iter)) {
                    Cell* pair = cell_car(adj_iter);
                    Cell* key = cell_car(pair);

                    /* Compare nodes using deep equality */
                    if (cell_equal(key, from)) {
                        entry = pair;
                        found = true;
                        break;
                    }
                    adj_iter = cell_cdr(adj_iter);
                }

                if (found) {
                    /* Add 'to' to existing successors list */
                    Cell* successors = cell_cdr(entry);
                    cell_retain(to);
                    Cell* new_successors = cell_cons(to, successors);
                    /* Update the entry (immutable, rebuild adj_list) */
                    cell_retain(from);
                    Cell* new_entry = cell_cons(from, new_successors);

                    /* Replace old entry in adj_list */
                    Cell* new_adj = cell_nil();
                    adj_iter = adj_list;
                    while (cell_is_pair(adj_iter)) {
                        Cell* pair = cell_car(adj_iter);
                        Cell* key = cell_car(pair);
                        if (cell_equal(key, from)) {
                            cell_retain(new_entry);
                            new_adj = cell_cons(new_entry, new_adj);
                        } else {
                            cell_retain(pair);
                            new_adj = cell_cons(pair, new_adj);
                        }
                        adj_iter = cell_cdr(adj_iter);
                    }
                    cell_release(adj_list);
                    adj_list = new_adj;
                } else {
                    /* Create new entry */
                    cell_retain(to);
                    Cell* successors = cell_cons(to, cell_nil());
                    cell_retain(from);
                    Cell* new_entry = cell_cons(from, successors);
                    adj_list = cell_cons(new_entry, adj_list);
                }
            }
        }
        edge_iter = cell_cdr(edge_iter);
    }

    /* Perform traversal */
    Cell* visited = cell_nil();  /* List of visited nodes */
    Cell* queue = cell_cons(start, cell_nil());  /* Queue/stack for traversal */
    Cell* visited_set = cell_nil();  /* Set of visited nodes (for cycle detection) */

    while (cell_is_pair(queue)) {
        /* Pop from queue/stack (retain current, release old queue head) */
        Cell* current = cell_car(queue);
        cell_retain(current);
        Cell* old_queue = queue;
        queue = cell_cdr(queue);
        cell_retain(queue);
        cell_release(old_queue);

        /* Check if already visited */
        bool already_visited = false;
        Cell* vs_iter = visited_set;
        while (cell_is_pair(vs_iter)) {
            if (cell_equal(cell_car(vs_iter), current)) {
                already_visited = true;
                break;
            }
            vs_iter = cell_cdr(vs_iter);
        }

        if (already_visited) {
            cell_release(current);
            continue;
        }

        /* Mark as visited (cons will retain current) */
        Cell* old_visited_set = visited_set;
        visited_set = cell_cons(current, visited_set);
        cell_release(old_visited_set);

        /* Apply visitor function */
        EvalContext* ctx = eval_get_current_context();
        Cell* visitor_arg = cell_cons(current, cell_nil());
        Cell* visitor_env = extend_env(visitor->data.lambda.env, visitor_arg);
        Cell* result = eval_internal(ctx, visitor_env, visitor->data.lambda.body);
        cell_release(visitor_arg);
        cell_release(visitor_env);

        /* Add result to visited list (cons will retain result) */
        Cell* old_visited = visited;
        visited = cell_cons(result, visited);
        cell_release(old_visited);

        /* Add successors to queue */
        Cell* adj_iter = adj_list;
        while (cell_is_pair(adj_iter)) {
            Cell* pair = cell_car(adj_iter);
            Cell* key = cell_car(pair);

            if (cell_equal(key, current)) {
                Cell* successors = cell_cdr(pair);
                Cell* succ_iter = successors;

                while (cell_is_pair(succ_iter)) {
                    Cell* succ = cell_car(succ_iter);

                    /* Check if successor already in visited_set */
                    bool succ_visited = false;
                    Cell* vs_iter2 = visited_set;
                    while (cell_is_pair(vs_iter2)) {
                        if (cell_equal(cell_car(vs_iter2), succ)) {
                            succ_visited = true;
                            break;
                        }
                        vs_iter2 = cell_cdr(vs_iter2);
                    }

                    if (!succ_visited) {
                        /* Add to queue (cons will retain succ) */
                        Cell* old_q = queue;
                        queue = cell_cons(succ, queue);
                        cell_release(old_q);
                    }

                    succ_iter = cell_cdr(succ_iter);
                }
                break;
            }
            adj_iter = cell_cdr(adj_iter);
        }

        cell_release(current);
    }

    /* Cleanup */
    cell_release(adj_list);
    cell_release(queue);
    cell_release(visited_set);
    /* Note: edges is a borrowed reference from the graph, don't release */

    /* Reverse visited list to get correct order */
    Cell* result = cell_nil();
    while (cell_is_pair(visited)) {
        Cell* node = cell_car(visited);
        cell_retain(node);
        result = cell_cons(node, result);
        Cell* next = cell_cdr(visited);
        cell_release(visited);
        visited = next;
    }

    return result;
}

/* ⊝⊃ - Check if to_node is reachable from from_node
 * Args: graph from_node to_node
 * Example: (⊝⊃ cfg :entry :exit)
 * Returns: #t or #f
 */
Cell* prim_graph_reachable(Cell* args) {
    Cell* graph = arg1(args);
    Cell* from = arg2(args);
    Cell* to = arg3(args);

    if (!cell_is_graph(graph)) {
        return cell_error("⊝⊃ first arg must be graph", graph);
    }

    /* Use BFS to check reachability */
    Cell* queue = cell_cons(from, cell_nil());
    Cell* visited = cell_nil();
    Cell* edges = cell_graph_edges(graph);
    bool found = false;

    while (cell_is_pair(queue) && !found) {
        /* Pop from queue (retain current, release old queue head) */
        Cell* current = cell_car(queue);
        cell_retain(current);
        Cell* old_queue = queue;
        queue = cell_cdr(queue);
        cell_retain(queue);
        cell_release(old_queue);

        /* Check if we've reached the target */
        if (cell_equal(current, to)) {
            found = true;
            cell_release(current);
            break;
        }

        /* Check if already visited */
        bool already_visited = false;
        Cell* v_iter = visited;
        while (cell_is_pair(v_iter)) {
            if (cell_equal(cell_car(v_iter), current)) {
                already_visited = true;
                break;
            }
            v_iter = cell_cdr(v_iter);
        }

        if (already_visited) {
            cell_release(current);
            continue;
        }

        /* Mark as visited (cons will retain current) */
        Cell* old_visited = visited;
        visited = cell_cons(current, visited);
        cell_release(old_visited);

        /* Add successors to queue */
        Cell* edge_iter = edges;
        while (cell_is_pair(edge_iter)) {
            Cell* edge = cell_car(edge_iter);
            if (cell_is_pair(edge)) {
                Cell* edge_from = cell_car(edge);
                Cell* rest = cell_cdr(edge);
                if (cell_is_pair(rest)) {
                    Cell* edge_to = cell_car(rest);

                    if (cell_equal(edge_from, current)) {
                        /* Add to queue (cons will retain edge_to) */
                        Cell* old_q = queue;
                        queue = cell_cons(edge_to, queue);
                        cell_release(old_q);
                    }
                }
            }
            edge_iter = cell_cdr(edge_iter);
        }

        cell_release(current);
    }

    /* Cleanup */
    cell_release(queue);
    cell_release(visited);
    /* Note: edges is a borrowed reference from the graph, don't release */

    return cell_bool(found);
}

/* ⊝⊚ - Get successors of a node
 * Args: graph node
 * Example: (⊝⊚ cfg :entry)
 * Returns: list of successor nodes
 */
Cell* prim_graph_successors(Cell* args) {
    Cell* graph = arg1(args);
    Cell* node = arg2(args);

    if (!cell_is_graph(graph)) {
        return cell_error("⊝⊚ first arg must be graph", graph);
    }

    Cell* edges = cell_graph_edges(graph);
    Cell* result = cell_nil();

    /* Find all edges from 'node' */
    Cell* edge_iter = edges;
    while (cell_is_pair(edge_iter)) {
        Cell* edge = cell_car(edge_iter);
        if (cell_is_pair(edge)) {
            Cell* from = cell_car(edge);
            Cell* rest = cell_cdr(edge);
            if (cell_is_pair(rest)) {
                Cell* to = cell_car(rest);

                if (cell_equal(from, node)) {
                    /* Add to result (cons will retain to) */
                    Cell* old_result = result;
                    result = cell_cons(to, old_result);
                    cell_release(old_result);
                }
            }
        }
        edge_iter = cell_cdr(edge_iter);
    }

    /* Note: edges is a borrowed reference from the graph, don't release */
    return result;
}

/* ⊝⊙ - Get predecessors of a node
 * Args: graph node
 * Example: (⊝⊙ cfg :exit)
 * Returns: list of predecessor nodes
 */
Cell* prim_graph_predecessors(Cell* args) {
    Cell* graph = arg1(args);
    Cell* node = arg2(args);

    if (!cell_is_graph(graph)) {
        return cell_error("⊝⊙ first arg must be graph", graph);
    }

    Cell* edges = cell_graph_edges(graph);
    Cell* result = cell_nil();

    /* Find all edges to 'node' */
    Cell* edge_iter = edges;
    while (cell_is_pair(edge_iter)) {
        Cell* edge = cell_car(edge_iter);
        if (cell_is_pair(edge)) {
            Cell* from = cell_car(edge);
            Cell* rest = cell_cdr(edge);
            if (cell_is_pair(rest)) {
                Cell* to = cell_car(rest);

                if (cell_equal(to, node)) {
                    /* Add to result (cons will retain from) */
                    Cell* old_result = result;
                    result = cell_cons(from, old_result);
                    cell_release(old_result);
                }
            }
        }
        edge_iter = cell_cdr(edge_iter);
    }

    /* Note: edges is a borrowed reference from the graph, don't release */
    return result;
}

/* ⊝⇝ - Find path from from_node to to_node
 * Args: graph from_node to_node
 * Example: (⊝⇝ cfg :entry :exit)
 * Returns: list of nodes in path, or ∅ if no path exists
 */
Cell* prim_graph_path(Cell* args) {
    Cell* graph = arg1(args);
    Cell* from = arg2(args);
    Cell* to = arg3(args);

    if (!cell_is_graph(graph)) {
        return cell_error("⊝⇝ first arg must be graph", graph);
    }

    /* BFS with parent tracking */
    Cell* queue = cell_cons(from, cell_nil());
    Cell* visited = cell_nil();
    Cell* parents = cell_nil();  /* Association list: node -> parent */
    Cell* edges = cell_graph_edges(graph);
    bool found = false;

    while (cell_is_pair(queue) && !found) {
        /* Pop from queue (retain current, release old queue head) */
        Cell* current = cell_car(queue);
        cell_retain(current);
        Cell* old_queue = queue;
        queue = cell_cdr(queue);
        cell_retain(queue);
        cell_release(old_queue);

        /* Check if we've reached the target */
        if (cell_equal(current, to)) {
            found = true;
            /* Don't release current yet - need it for path reconstruction */
            break;
        }

        /* Check if already visited */
        bool already_visited = false;
        Cell* v_iter = visited;
        while (cell_is_pair(v_iter)) {
            if (cell_equal(cell_car(v_iter), current)) {
                already_visited = true;
                break;
            }
            v_iter = cell_cdr(v_iter);
        }

        if (already_visited) {
            cell_release(current);
            continue;
        }

        /* Mark as visited (cons will retain current) */
        Cell* old_visited = visited;
        visited = cell_cons(current, visited);
        cell_release(old_visited);

        /* Add successors to queue */
        Cell* edge_iter = edges;
        while (cell_is_pair(edge_iter)) {
            Cell* edge = cell_car(edge_iter);
            if (cell_is_pair(edge)) {
                Cell* edge_from = cell_car(edge);
                Cell* rest = cell_cdr(edge);
                if (cell_is_pair(rest)) {
                    Cell* edge_to = cell_car(rest);

                    if (cell_equal(edge_from, current)) {
                        /* Check if successor already visited */
                        bool succ_visited = false;
                        Cell* v_iter2 = visited;
                        while (cell_is_pair(v_iter2)) {
                            if (cell_equal(cell_car(v_iter2), edge_to)) {
                                succ_visited = true;
                                break;
                            }
                            v_iter2 = cell_cdr(v_iter2);
                        }

                        if (!succ_visited) {
                            /* Record parent relationship (cons will retain both) */
                            Cell* parent_entry = cell_cons(edge_to, current);
                            Cell* old_parents = parents;
                            parents = cell_cons(parent_entry, parents);
                            cell_release(old_parents);

                            /* Add to queue (cons will retain edge_to) */
                            Cell* old_q = queue;
                            queue = cell_cons(edge_to, queue);
                            cell_release(old_q);
                        }
                    }
                }
            }
            edge_iter = cell_cdr(edge_iter);
        }

        cell_release(current);
    }

    /* Reconstruct path if found */
    Cell* result;
    if (found) {
        result = cell_nil();
        Cell* current_node = to;
        cell_retain(current_node);

        /* Build path from end to start */
        result = cell_cons(current_node, result);

        while (!cell_equal(current_node, from)) {
            /* Find parent of current_node */
            Cell* parent = NULL;
            Cell* p_iter = parents;
            while (cell_is_pair(p_iter)) {
                Cell* entry = cell_car(p_iter);
                Cell* child = cell_car(entry);
                if (cell_equal(child, current_node)) {
                    parent = cell_cdr(entry);
                    break;
                }
                p_iter = cell_cdr(p_iter);
            }

            if (parent) {
                cell_retain(parent);
                result = cell_cons(parent, result);
                cell_release(current_node);
                current_node = parent;
            } else {
                /* Path broken - should not happen if found==true */
                cell_release(current_node);
                cell_release(result);
                result = cell_nil();
                break;
            }
        }

        cell_release(current_node);
    } else {
        result = cell_nil();
    }

    /* Cleanup */
    cell_release(queue);
    cell_release(visited);
    cell_release(parents);
    /* Note: edges is a borrowed reference from the graph, don't release */

    return result;
}

/* ⊝∘ - Detect cycles in graph
 * Args: graph
 * Example: (⊝∘ cfg)
 * Returns: list of cycles (each cycle is a list of nodes), or ∅ if no cycles
 */
Cell* prim_graph_cycles(Cell* args) {
    Cell* graph = arg1(args);

    if (!cell_is_graph(graph)) {
        return cell_error("⊝∘ first arg must be graph", graph);
    }

    Cell* nodes = cell_graph_nodes(graph);
    Cell* edges = cell_graph_edges(graph);
    Cell* cycles = cell_nil();

    /* Use DFS with colors: white (unvisited), gray (visiting), black (visited) */
    Cell* white = nodes;  /* All nodes start as white */
    Cell* gray = cell_nil();
    Cell* black = cell_nil();
    Cell* stack = cell_nil();  /* DFS stack */

    /* Start DFS from each unvisited node */
    Cell* node_iter = white;
    while (cell_is_pair(node_iter)) {
        Cell* start = cell_car(node_iter);

        /* Check if start is already black */
        bool is_black = false;
        Cell* b_iter = black;
        while (cell_is_pair(b_iter)) {
            if (cell_equal(cell_car(b_iter), start)) {
                is_black = true;
                break;
            }
            b_iter = cell_cdr(b_iter);
        }

        if (!is_black) {
            /* DFS from start */
            Cell* old_stack = stack;
            stack = cell_cons(start, old_stack);
            cell_release(old_stack);

            Cell* old_gray = gray;
            gray = cell_cons(start, old_gray);
            cell_release(old_gray);

            while (cell_is_pair(stack)) {
                Cell* current = cell_car(stack);
                bool has_unvisited_successor = false;

                /* Check successors */
                Cell* edge_iter = edges;
                while (cell_is_pair(edge_iter)) {
                    Cell* edge = cell_car(edge_iter);
                    if (cell_is_pair(edge)) {
                        Cell* from = cell_car(edge);
                        Cell* rest = cell_cdr(edge);
                        if (cell_is_pair(rest)) {
                            Cell* to = cell_car(rest);

                            if (cell_equal(from, current)) {
                                /* Check if 'to' is gray (back edge - cycle detected) */
                                bool is_gray = false;
                                Cell* g_iter = gray;
                                while (cell_is_pair(g_iter)) {
                                    if (cell_equal(cell_car(g_iter), to)) {
                                        is_gray = true;
                                        break;
                                    }
                                    g_iter = cell_cdr(g_iter);
                                }

                                if (is_gray) {
                                    /* Cycle detected! Record it */
                                    Cell* cycle = cell_nil();
                                    cycle = cell_cons(to, cycle);
                                    Cell* old_cycle = cycle;
                                    cycle = cell_cons(current, old_cycle);
                                    cell_release(old_cycle);

                                    Cell* old_cycles = cycles;
                                    cycles = cell_cons(cycle, old_cycles);
                                    cell_release(old_cycles);
                                }

                                /* Check if 'to' is white (unvisited) */
                                bool is_white = true;
                                g_iter = gray;
                                while (cell_is_pair(g_iter)) {
                                    if (cell_equal(cell_car(g_iter), to)) {
                                        is_white = false;
                                        break;
                                    }
                                    g_iter = cell_cdr(g_iter);
                                }
                                Cell* b_iter2 = black;
                                while (cell_is_pair(b_iter2)) {
                                    if (cell_equal(cell_car(b_iter2), to)) {
                                        is_white = false;
                                        break;
                                    }
                                    b_iter2 = cell_cdr(b_iter2);
                                }

                                if (is_white) {
                                    /* Visit this successor */
                                    Cell* old_stk = stack;
                                    stack = cell_cons(to, old_stk);
                                    cell_release(old_stk);

                                    Cell* old_gry = gray;
                                    gray = cell_cons(to, old_gry);
                                    cell_release(old_gry);

                                    has_unvisited_successor = true;
                                    break;
                                }
                            }
                        }
                    }
                    edge_iter = cell_cdr(edge_iter);
                }

                if (!has_unvisited_successor) {
                    /* Finished with current node - mark as black */
                    Cell* popped = cell_car(stack);
                    cell_retain(popped);

                    Cell* old_stack = stack;
                    stack = cell_cdr(stack);
                    cell_retain(stack);
                    cell_release(old_stack);

                    /* Move from gray to black */
                    Cell* new_gray = cell_nil();
                    Cell* g_iter = gray;
                    while (cell_is_pair(g_iter)) {
                        Cell* node = cell_car(g_iter);
                        if (!cell_equal(node, popped)) {
                            Cell* old_new_gray = new_gray;
                            new_gray = cell_cons(node, old_new_gray);
                            cell_release(old_new_gray);
                        }
                        g_iter = cell_cdr(g_iter);
                    }
                    cell_release(gray);
                    gray = new_gray;

                    Cell* old_black = black;
                    black = cell_cons(popped, old_black);
                    cell_release(old_black);
                    cell_release(popped);
                }
            }
        }

        node_iter = cell_cdr(node_iter);
    }

    /* Cleanup */
    /* Note: nodes and edges are borrowed references from the graph, don't release */
    cell_release(stack);
    cell_release(gray);
    cell_release(black);

    return cycles;
}

/* String Operations */

/* ≈ - Convert value to string */
Cell* prim_str(Cell* args) {
    Cell* value = arg1(args);
    char buffer[256];

    if (cell_is_number(value)) {
        double num = cell_get_number(value);
        snprintf(buffer, sizeof(buffer), "%.10g", num);
        return cell_string(buffer);
    } else if (cell_is_bool(value)) {
        return cell_string(cell_get_bool(value) ? "#t" : "#f");
    } else if (cell_is_symbol(value)) {
        const char* sym = cell_get_symbol(value);
        return cell_string(sym);
    } else if (cell_is_string(value)) {
        cell_retain(value);  /* Already a string */
        return value;
    } else if (cell_is_nil(value)) {
        return cell_string("∅");
    } else {
        return cell_error("≈ cannot convert type to string", value);
    }
}

/* ≈⊕ - Concatenate two strings */
Cell* prim_str_concat(Cell* args) {
    Cell* str1 = arg1(args);
    Cell* str2 = arg2(args);

    if (!cell_is_string(str1) || !cell_is_string(str2)) {
        return cell_error("≈⊕ requires two strings", str1);
    }

    const char* s1 = cell_get_string(str1);
    const char* s2 = cell_get_string(str2);

    size_t len = strlen(s1) + strlen(s2) + 1;
    char* result = (char*)malloc(len);
    strcpy(result, s1);
    strcat(result, s2);

    Cell* ret = cell_string(result);
    free(result);  /* cell_string strdup's */
    return ret;
}

/* ≈# - String length */
Cell* prim_str_length(Cell* args) {
    Cell* str = arg1(args);
    assert(cell_is_string(str));
    return cell_number((double)strlen(cell_get_string(str)));
}

/* ≈→ - Character at index (returns symbol) */
Cell* prim_str_ref(Cell* args) {
    Cell* str = arg1(args);
    Cell* idx = arg2(args);

    assert(cell_is_string(str) && cell_is_number(idx));

    const char* s = cell_get_string(str);
    int i = (int)cell_get_number(idx);
    int len = strlen(s);

    if (i < 0 || i >= len) {
        return cell_error("≈→ index out of bounds", idx);
    }

    char ch[2] = {s[i], '\0'};
    return cell_symbol(ch);  /* Return single-char symbol */
}

/* ≈⊂ - Substring (start, end) */
Cell* prim_str_slice(Cell* args) {
    Cell* str = arg1(args);
    Cell* start_cell = arg2(args);
    Cell* end_cell = arg3(args);

    assert(cell_is_string(str) && cell_is_number(start_cell) && cell_is_number(end_cell));

    const char* s = cell_get_string(str);
    int len = strlen(s);
    int start = (int)cell_get_number(start_cell);
    int end = (int)cell_get_number(end_cell);

    /* Bounds checking */
    if (start < 0) start = 0;
    if (end > len) end = len;
    if (start > end) start = end;

    int slice_len = end - start;
    char* result = (char*)malloc(slice_len + 1);
    strncpy(result, s + start, slice_len);
    result[slice_len] = '\0';

    Cell* ret = cell_string(result);
    free(result);
    return ret;
}

/* ≈? - Is string? */
Cell* prim_is_string(Cell* args) {
    return cell_bool(cell_is_string(arg1(args)));
}

/* ≈∅? - Is empty string? */
Cell* prim_str_empty(Cell* args) {
    Cell* str = arg1(args);
    if (!cell_is_string(str)) {
        return cell_bool(false);
    }
    return cell_bool(strlen(cell_get_string(str)) == 0);
}

/* ≈≡ - String equality */
Cell* prim_str_equal(Cell* args) {
    Cell* str1 = arg1(args);
    Cell* str2 = arg2(args);

    if (!cell_is_string(str1) || !cell_is_string(str2)) {
        return cell_bool(false);
    }

    return cell_bool(strcmp(cell_get_string(str1), cell_get_string(str2)) == 0);
}

/* ≈< - String ordering */
Cell* prim_str_less(Cell* args) {
    Cell* str1 = arg1(args);
    Cell* str2 = arg2(args);

    assert(cell_is_string(str1) && cell_is_string(str2));

    return cell_bool(strcmp(cell_get_string(str1), cell_get_string(str2)) < 0);
}

/* ============ I/O Primitives ============ */

/* ≋ - Print value to stdout with newline */
Cell* prim_print(Cell* args) {
    Cell* value = arg1(args);

    if (cell_is_string(value)) {
        printf("%s\n", cell_get_string(value));
    } else if (cell_is_number(value)) {
        printf("%.10g\n", cell_get_number(value));
    } else if (cell_is_bool(value)) {
        printf("%s\n", cell_get_bool(value) ? "#t" : "#f");
    } else if (cell_is_symbol(value)) {
        printf("%s\n", cell_get_symbol(value));
    } else if (cell_is_nil(value)) {
        printf("∅\n");
    } else {
        printf("<value>\n");
    }

    fflush(stdout);
    cell_retain(value);  /* Return the value */
    return value;
}

/* ≋≈ - Print string to stdout without newline */
Cell* prim_print_str(Cell* args) {
    Cell* str = arg1(args);

    if (!cell_is_string(str)) {
        return cell_error("≋≈ requires a string", str);
    }

    printf("%s", cell_get_string(str));
    fflush(stdout);

    cell_retain(str);
    return str;
}

/* ≋← - Read line from stdin */
Cell* prim_read_line(Cell* args) {
    char buffer[4096];

    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        /* EOF or error */
        return cell_error(":read-error", cell_nil());
    }

    /* Remove trailing newline */
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }

    return cell_string(buffer);
}

/* ≋⊳ - Read entire file as string */
Cell* prim_read_file(Cell* args) {
    Cell* path = arg1(args);

    if (!cell_is_string(path)) {
        return cell_error("≋⊳ requires a string path", path);
    }

    const char* filename = cell_get_string(path);
    FILE* file = fopen(filename, "r");

    if (!file) {
        return cell_error(":file-not-found", path);
    }

    /* Get file size */
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    /* Allocate buffer */
    char* buffer = (char*)malloc(file_size + 1);
    if (!buffer) {
        fclose(file);
        return cell_error(":out-of-memory", path);
    }

    /* Read entire file */
    size_t bytes_read = fread(buffer, 1, file_size, file);
    buffer[bytes_read] = '\0';

    fclose(file);

    Cell* result = cell_string(buffer);
    free(buffer);
    return result;
}

/* ≋⊲ - Write string to file (overwrites) */
Cell* prim_write_file(Cell* args) {
    Cell* path = arg1(args);
    Cell* content = arg2(args);

    if (!cell_is_string(path)) {
        return cell_error("≋⊲ requires string path", path);
    }

    if (!cell_is_string(content)) {
        return cell_error("≋⊲ requires string content", content);
    }

    const char* filename = cell_get_string(path);
    const char* data = cell_get_string(content);

    FILE* file = fopen(filename, "w");
    if (!file) {
        return cell_error(":file-write-error", path);
    }

    fputs(data, file);
    fclose(file);

    cell_retain(path);  /* Return path */
    return path;
}

/* ≋⊕ - Append string to file */
Cell* prim_append_file(Cell* args) {
    Cell* path = arg1(args);
    Cell* content = arg2(args);

    if (!cell_is_string(path)) {
        return cell_error("≋⊕ requires string path", path);
    }

    if (!cell_is_string(content)) {
        return cell_error("≋⊕ requires string content", content);
    }

    const char* filename = cell_get_string(path);
    const char* data = cell_get_string(content);

    FILE* file = fopen(filename, "a");
    if (!file) {
        return cell_error(":file-append-error", path);
    }

    fputs(data, file);
    fclose(file);

    cell_retain(path);  /* Return path */
    return path;
}

/* ≋? - Check if file exists */
Cell* prim_file_exists(Cell* args) {
    Cell* path = arg1(args);

    if (!cell_is_string(path)) {
        return cell_bool(false);
    }

    const char* filename = cell_get_string(path);
    FILE* file = fopen(filename, "r");

    if (file) {
        fclose(file);
        return cell_bool(true);
    }

    return cell_bool(false);
}

/* ≋∅? - Check if file is empty */
Cell* prim_file_empty(Cell* args) {
    Cell* path = arg1(args);

    if (!cell_is_string(path)) {
        return cell_bool(false);
    }

    const char* filename = cell_get_string(path);
    FILE* file = fopen(filename, "r");

    if (!file) {
        return cell_bool(false);  /* File doesn't exist = not empty */
    }

    /* Check file size */
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fclose(file);

    return cell_bool(size == 0);
}

/* Module System */

/* Parser struct from main.c */
typedef struct {
    const char* input;
    int pos;
} LoadParser;

/* Forward declarations from main.c */
extern Cell* parse(const char* input);

/* Helper: skip whitespace and comments */
static void load_skip_whitespace(LoadParser* p) {
    while (1) {
        /* Skip whitespace */
        while (p->input[p->pos] == ' ' ||
               p->input[p->pos] == '\t' ||
               p->input[p->pos] == '\n' ||
               p->input[p->pos] == '\r') {
            p->pos++;
        }

        /* Skip comments */
        if (p->input[p->pos] == ';') {
            while (p->input[p->pos] != '\0' &&
                   p->input[p->pos] != '\n') {
                p->pos++;
            }
            continue;
        }

        break;
    }
}

/* ⋘ - Load and evaluate a file */
Cell* prim_load(Cell* args) {
    Cell* path = arg1(args);

    if (!cell_is_string(path)) {
        return cell_error("⋘ requires a string path", path);
    }

    const char* filename = cell_get_string(path);

    /* Read file */
    FILE* file = fopen(filename, "r");
    if (!file) {
        return cell_error(":file-not-found", path);
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = (char*)malloc(file_size + 1);
    if (!buffer) {
        fclose(file);
        return cell_error(":out-of-memory", path);
    }

    size_t bytes_read = fread(buffer, 1, file_size, file);
    buffer[bytes_read] = '\0';
    fclose(file);

    /* Register module and set as currently loading */
    module_registry_add(filename);

    /* Track dependency if loaded from within another module (Day 29) */
    const char* parent_module = module_get_current_loading();
    if (parent_module && strcmp(parent_module, filename) != 0) {
        /* parent_module depends on filename */
        module_registry_add_dependency(parent_module, filename);
    }

    module_set_current_loading(filename);

    /* Get current context */
    EvalContext* ctx = eval_get_current_context();

    /* Parse and evaluate all expressions in the file */
    LoadParser parser = {buffer, 0};
    Cell* result = cell_nil();
    int expression_count = 0;

    while (parser.input[parser.pos] != '\0') {
        load_skip_whitespace(&parser);

        if (parser.input[parser.pos] == '\0') {
            break;
        }

        /* Parse one expression */
        Cell* expr = parse(parser.input + parser.pos);

        if (!expr) {
            module_set_current_loading(NULL);
            free(buffer);
            return cell_error(":parse-error", path);
        }

        /* Evaluate expression */
        if (expression_count > 0) {
            cell_release(result);
        }
        result = eval(ctx, expr);
        cell_release(expr);

        if (cell_is_error(result)) {
            module_set_current_loading(NULL);
            free(buffer);
            return result;
        }

        expression_count++;

        /* Advance parser position - skip past the expression we just parsed
         * by counting balanced parentheses */
        int start_pos = parser.pos;
        int depth = 0;
        int in_string = 0;
        int started_with_paren = (parser.input[parser.pos] == '(');

        if (started_with_paren) {
            /* For S-expressions, count balanced parens */
            while (parser.input[parser.pos] != '\0') {
                char c = parser.input[parser.pos];

                if (c == '"' && (parser.pos == 0 || parser.input[parser.pos - 1] != '\\')) {
                    in_string = !in_string;
                    parser.pos++;
                    continue;
                }

                if (!in_string) {
                    if (c == '(') {
                        depth++;
                    } else if (c == ')') {
                        depth--;
                        if (depth == 0) {
                            parser.pos++;
                            break;
                        }
                    }
                }

                parser.pos++;
            }
        } else {
            /* For atoms, skip until whitespace or paren */
            while (parser.input[parser.pos] != '\0' &&
                   parser.input[parser.pos] != ' ' &&
                   parser.input[parser.pos] != '\n' &&
                   parser.input[parser.pos] != '\t' &&
                   parser.input[parser.pos] != '\r' &&
                   parser.input[parser.pos] != '(' &&
                   parser.input[parser.pos] != ')') {
                parser.pos++;
            }
        }

        /* Safety check - ensure we made progress */
        if (parser.pos == start_pos) {
            /* Didn't advance - skip at least one character to avoid infinite loop */
            parser.pos++;
        }
    }

    /* Clear current loading module */
    module_set_current_loading(NULL);

    free(buffer);
    return result;
}

/* Forward declaration */
static Primitive primitives[];
/* Documentation primitives */

/* ⌂ - Get documentation for symbol */
Cell* prim_doc_get(Cell* args) {
    Cell* name = arg1(args);
    const char* sym = cell_get_symbol(name);

    /* Look up in primitives table */
    for (int i = 0; primitives[i].name != NULL; i++) {
        if (strcmp(primitives[i].name, sym) == 0) {
            return cell_symbol(primitives[i].doc.description);
        }
    }

    /* Look up in user function registry */
    FunctionDoc* doc = eval_find_user_doc(sym);
    if (doc && doc->description) {
        return cell_symbol(doc->description);
    }

    return cell_symbol(":no-documentation");
}

/* ⌂∈ - Get type signature for symbol */
Cell* prim_doc_type(Cell* args) {
    Cell* name = arg1(args);
    const char* sym = cell_get_symbol(name);

    /* Look up in primitives table */
    for (int i = 0; primitives[i].name != NULL; i++) {
        if (strcmp(primitives[i].name, sym) == 0) {
            return cell_symbol(primitives[i].doc.type_signature);
        }
    }

    /* Look up in user function registry */
    FunctionDoc* doc = eval_find_user_doc(sym);
    if (doc && doc->type_signature) {
        return cell_symbol(doc->type_signature);
    }

    return cell_symbol(":unknown-type");
}

/* ⌂≔ - Get dependencies for symbol */
Cell* prim_doc_deps(Cell* args) {
    Cell* name = arg1(args);
    const char* sym = cell_get_symbol(name);

    /* Primitives have no dependencies */
    const Primitive* prim = primitive_lookup_by_name(sym);
    if (prim) {
        return cell_nil();
    }

    /* Look up user function dependencies */
    FunctionDoc* doc = eval_find_user_doc(sym);
    if (doc) {
        /* Build list of dependencies */
        Cell* result = cell_nil();
        for (int i = (int)doc->dependency_count - 1; i >= 0; i--) {
            Cell* dep_sym = cell_symbol(doc->dependencies[i]);
            result = cell_cons(dep_sym, result);
        }
        return result;
    }

    return cell_nil();
}

/* ⌂⊛ - Get source code for symbol (Day 27: Enhanced provenance) */
Cell* prim_doc_source(Cell* args) {
    Cell* name = arg1(args);
    const char* sym = cell_get_symbol(name);

    /* Strip leading colon if present (keywords like :⊕ should match primitives) */
    const char* search_name = sym;
    if (sym[0] == ':') {
        search_name = sym + 1;
    }

    /* Primitives are built-in */
    for (int i = 0; primitives[i].name != NULL; i++) {
        if (strcmp(primitives[i].name, search_name) == 0) {
            /* Return simple structure for primitives */
            Cell* type_tag = cell_symbol(":Provenance");
            Cell* fields = cell_nil();

            Cell* module_key = cell_symbol(":module");
            Cell* module_val = cell_string("<primitive>");
            fields = cell_cons(cell_cons(module_key, module_val), fields);
            cell_release(module_key);
            cell_release(module_val);

            Cell* result = cell_struct(STRUCT_LEAF, type_tag, NULL, fields);
            cell_release(type_tag);
            cell_release(fields);
            return result;
        }
    }

    /* Look up which module defines this symbol */
    const char* module_name = module_registry_find_symbol(search_name);
    if (!module_name) {
        return cell_error("symbol-not-found", name);
    }

    /* Get module entry for metadata */
    ModuleEntry* entry = module_registry_get_entry(module_name);
    if (!entry) {
        return cell_error("module-not-found", cell_string(module_name));
    }

    /* Try to get the actual value to extract line number if it's a lambda */
    EvalContext* ctx = eval_get_current_context();
    Cell* value = eval_lookup(ctx, search_name);
    int line_number = 0;
    const char* source_module = module_name;

    if (value && cell_is_lambda(value)) {
        line_number = value->data.lambda.source_line;
        if (value->data.lambda.source_module) {
            source_module = value->data.lambda.source_module;
        }
    }

    /* Build provenance structure */
    Cell* type_tag = cell_symbol(":Provenance");
    Cell* fields = cell_nil();

    /* Add fields in reverse order (they're consed onto front) */
    /* :defined-at - timestamp */
    Cell* defined_key = cell_symbol(":defined-at");
    Cell* defined_val = cell_number((double)entry->loaded_at);
    fields = cell_cons(cell_cons(defined_key, defined_val), fields);
    cell_release(defined_key);
    cell_release(defined_val);

    /* :load-order - sequence number */
    Cell* order_key = cell_symbol(":load-order");
    Cell* order_val = cell_number((double)entry->load_order);
    fields = cell_cons(cell_cons(order_key, order_val), fields);
    cell_release(order_key);
    cell_release(order_val);

    /* :line - line number */
    Cell* line_key = cell_symbol(":line");
    Cell* line_val = cell_number((double)line_number);
    fields = cell_cons(cell_cons(line_key, line_val), fields);
    cell_release(line_key);
    cell_release(line_val);

    /* :module - module path */
    Cell* module_key = cell_symbol(":module");
    Cell* module_val = cell_string(source_module);
    fields = cell_cons(cell_cons(module_key, module_val), fields);
    cell_release(module_key);
    cell_release(module_val);

    Cell* result = cell_struct(STRUCT_LEAF, type_tag, NULL, fields);
    cell_release(type_tag);
    cell_release(fields);

    return result;
}

/* ⌂⊚ - Module information (list modules, find symbol, list symbols) */
Cell* prim_module_info(Cell* args) {
    /* (⌂⊚) - List all loaded modules */
    if (cell_is_nil(args)) {
        return module_registry_list_modules();
    }

    Cell* arg1_val = arg1(args);

    /* (⌂⊚ (⌜ symbol)) or (⌂⊚ :symbol) - Find which module defines symbol */
    if (cell_is_symbol(arg1_val)) {
        const char* sym = cell_get_symbol(arg1_val);
        const char* module_name = module_registry_find_symbol(sym);

        if (module_name == NULL) {
            return cell_error("symbol-not-in-any-module", arg1_val);
        }

        return cell_string(module_name);
    }

    /* (⌂⊚ "module-name") - List all symbols from module */
    if (cell_is_string(arg1_val)) {
        const char* module_name = cell_get_string(arg1_val);
        return module_registry_list_symbols(module_name);
    }

    /* Invalid argument type */
    return cell_error("⌂⊚ requires nil, symbol, or string", arg1_val);
}

/* ⋖ - Selective import (Day 28) */
Cell* prim_module_import(Cell* args) {
    /* (⋖ "module-path" ⟨:sym1 :sym2 ...⟩) */
    /* Validates symbols exist in module, ensures module is loaded */

    if (cell_is_nil(args)) {
        return cell_error("module-import-missing-args", cell_nil());
    }

    Cell* module_path_cell = arg1(args);
    Cell* rest = cell_cdr(args);

    if (cell_is_nil(rest)) {
        return cell_error("module-import-missing-symbols", module_path_cell);
    }

    Cell* symbols = arg1(rest);

    /* First argument must be string (module path) */
    if (!cell_is_string(module_path_cell)) {
        return cell_error("module-path-must-be-string", module_path_cell);
    }

    const char* module_path = cell_get_string(module_path_cell);

    /* Second argument must be list of symbols */
    if (!cell_is_pair(symbols) && !cell_is_nil(symbols)) {
        return cell_error("symbol-list-must-be-list", symbols);
    }

    /* Check if module is loaded */
    if (!module_registry_has_module(module_path)) {
        return cell_error("module-not-loaded", module_path_cell);
    }

    /* Get module's symbol list */
    Cell* module_symbols = module_registry_list_symbols(module_path);

    /* Validate each requested symbol exists in module */
    Cell* curr = symbols;
    while (!cell_is_nil(curr)) {
        Cell* requested_sym = cell_car(curr);

        if (!cell_is_symbol(requested_sym)) {
            cell_release(module_symbols);
            return cell_error("symbol-list-must-contain-symbols", requested_sym);
        }

        /* Check if this symbol is in module */
        const char* requested_name = cell_get_symbol(requested_sym);
        /* Normalize: strip leading : if present */
        if (requested_name[0] == ':') {
            requested_name++;
        }

        int found = 0;
        Cell* mod_curr = module_symbols;
        while (!cell_is_nil(mod_curr)) {
            Cell* mod_sym = cell_car(mod_curr);
            if (cell_is_symbol(mod_sym)) {
                const char* mod_name = cell_get_symbol(mod_sym);
                if (mod_name[0] == ':') {
                    mod_name++;
                }
                if (strcmp(mod_name, requested_name) == 0) {
                    found = 1;
                    break;
                }
            }
            mod_curr = cell_cdr(mod_curr);
        }

        if (!found) {
            cell_release(module_symbols);
            return cell_error("symbol-not-in-module", requested_sym);
        }

        curr = cell_cdr(curr);
    }

    cell_release(module_symbols);

    /* All symbols validated - return success */
    return cell_symbol(":ok");
}

/* ⌂⊚→ - Get module dependencies (Day 29) */
Cell* prim_module_dependencies(Cell* args) {
    /* (⌂⊚→ "module-path") */
    /* Returns list of module names (as strings) that this module depends on */

    if (cell_is_nil(args)) {
        return cell_error("module-deps-missing-args", cell_nil());
    }

    Cell* module_path_cell = arg1(args);

    /* Argument must be string (module path) */
    if (!cell_is_string(module_path_cell)) {
        return cell_error("module-path-must-be-string", module_path_cell);
    }

    const char* module_path = cell_get_string(module_path_cell);

    /* Check if module is loaded */
    if (!module_registry_has_module(module_path)) {
        return cell_error("module-not-loaded", module_path_cell);
    }

    /* Get module dependencies */
    return module_registry_get_dependencies(module_path);
}

/* ⌂⊚# - Get/set module version (Day 70) */
Cell* prim_module_version(Cell* args) {
    /* (⌂⊚# "module-path") - Get version */
    /* (⌂⊚# "module-path" "version") - Set version */

    if (cell_is_nil(args)) {
        return cell_error("module-version-missing-args", cell_nil());
    }

    Cell* module_path_cell = arg1(args);

    if (!cell_is_string(module_path_cell)) {
        return cell_error("module-path-must-be-string", module_path_cell);
    }

    const char* module_path = cell_get_string(module_path_cell);

    if (!module_registry_has_module(module_path)) {
        return cell_error("module-not-loaded", module_path_cell);
    }

    Cell* rest = cell_cdr(args);
    if (cell_is_nil(rest)) {
        /* Get version */
        const char* version = module_registry_get_version(module_path);
        if (version) {
            return cell_string(version);
        }
        return cell_nil();
    }

    /* Set version */
    Cell* version_cell = arg1(rest);
    if (!cell_is_string(version_cell)) {
        return cell_error("version-must-be-string", version_cell);
    }

    module_registry_set_version(module_path, cell_get_string(version_cell));
    return cell_symbol(":ok");
}

/* ⌂⊚↑ - Get/set module exports (Day 70) */
Cell* prim_module_exports(Cell* args) {
    /* (⌂⊚↑ "module-path") - Get exports */
    /* (⌂⊚↑ "module-path" (:sym1 :sym2 ...)) - Set exports */

    if (cell_is_nil(args)) {
        return cell_error("module-exports-missing-args", cell_nil());
    }

    Cell* module_path_cell = arg1(args);

    if (!cell_is_string(module_path_cell)) {
        return cell_error("module-path-must-be-string", module_path_cell);
    }

    const char* module_path = cell_get_string(module_path_cell);

    if (!module_registry_has_module(module_path)) {
        return cell_error("module-not-loaded", module_path_cell);
    }

    Cell* rest = cell_cdr(args);
    if (cell_is_nil(rest)) {
        /* Get exports */
        return module_registry_get_exports(module_path);
    }

    /* Set exports */
    Cell* exports = arg1(rest);
    if (!cell_is_pair(exports) && !cell_is_nil(exports)) {
        return cell_error("exports-must-be-list", exports);
    }

    module_registry_set_exports(module_path, exports);
    return cell_symbol(":ok");
}

/* ⌂⊚⊛ - Detect module cycles (Day 70) */
Cell* prim_module_cycles(Cell* args) {
    /* (⌂⊚⊛ "module-path") - Detect cycles starting from module */
    /* (⌂⊚⊛) - Detect all cycles in system */

    if (cell_is_nil(args)) {
        /* Check all modules */
        Cell* all_cycles = cell_nil();
        Cell* modules = module_registry_list_modules();
        Cell* curr = modules;

        while (curr && !cell_is_nil(curr)) {
            Cell* mod = cell_car(curr);
            if (cell_is_string(mod)) {
                Cell* cycles = module_registry_detect_cycles(cell_get_string(mod));
                if (!cell_is_nil(cycles)) {
                    Cell* new_all = cell_cons(cycles, all_cycles);
                    cell_release(all_cycles);
                    all_cycles = new_all;
                }
                cell_release(cycles);
            }
            curr = cell_cdr(curr);
        }

        cell_release(modules);
        return all_cycles;
    }

    Cell* module_path_cell = arg1(args);

    if (!cell_is_string(module_path_cell)) {
        return cell_error("module-path-must-be-string", module_path_cell);
    }

    const char* module_path = cell_get_string(module_path_cell);

    if (!module_registry_has_module(module_path)) {
        return cell_error("module-not-loaded", module_path_cell);
    }

    return module_registry_detect_cycles(module_path);
}

/* Forward declaration */
Cell* prim_doc_generate(Cell* args);

/* 📖→ - Export documentation to file (Day 63 Phase 2) */
Cell* prim_doc_export(Cell* args) {
    /* (📖→ "module-path" "output-path") */
    /* Generates markdown and writes to file */

    if (cell_is_nil(args)) {
        return cell_error("doc-export-missing-args", cell_nil());
    }

    Cell* module_path_cell = arg1(args);
    Cell* rest = cell_cdr(args);

    if (cell_is_nil(rest)) {
        return cell_error("doc-export-missing-output-path", module_path_cell);
    }

    Cell* output_path_cell = arg1(rest);

    /* Arguments must be strings */
    if (!cell_is_string(module_path_cell)) {
        return cell_error("module-path-must-be-string", module_path_cell);
    }

    if (!cell_is_string(output_path_cell)) {
        return cell_error("output-path-must-be-string", output_path_cell);
    }

    /* Generate documentation */
    Cell* doc_args = cell_cons(module_path_cell, cell_nil());
    Cell* doc = prim_doc_generate(doc_args);
    cell_release(doc_args);

    /* Check if generation succeeded */
    if (cell_is_error(doc)) {
        return doc;
    }

    if (!cell_is_string(doc)) {
        cell_release(doc);
        return cell_error("doc-generate-failed", module_path_cell);
    }

    /* Write to file */
    const char* output_path = cell_get_string(output_path_cell);
    const char* doc_content = cell_get_string(doc);

    FILE* f = fopen(output_path, "w");
    if (!f) {
        cell_release(doc);
        return cell_error("file-write-error", output_path_cell);
    }

    fputs(doc_content, f);
    fclose(f);

    cell_release(doc);

    /* Return output path on success */
    cell_retain(output_path_cell);
    return output_path_cell;
}

/* 📖 - Generate markdown documentation for module (Day 63) */
Cell* prim_doc_generate(Cell* args) {
    /* (📖 "module-path") */
    /* Generates markdown documentation for all functions in a module */

    if (cell_is_nil(args)) {
        return cell_error("doc-generate-missing-args", cell_nil());
    }

    Cell* module_path_cell = arg1(args);

    /* Argument must be string (module path) */
    if (!cell_is_string(module_path_cell)) {
        return cell_error("module-path-must-be-string", module_path_cell);
    }

    const char* module_path = cell_get_string(module_path_cell);

    /* Check if module is loaded */
    if (!module_registry_has_module(module_path)) {
        return cell_error("module-not-loaded", module_path_cell);
    }

    /* Get all symbols from module */
    Cell* symbols = module_registry_list_symbols(module_path);
    if (cell_is_nil(symbols)) {
        /* Module exists but has no exported symbols - return empty doc */
        return cell_string("# Empty Module\n\nNo exported functions.\n");
    }

    /* Build markdown documentation */
    char* doc_buffer = malloc(65536);  /* 64KB buffer for documentation */
    if (!doc_buffer) {
        cell_release(symbols);
        return cell_error("out-of-memory", cell_nil());
    }

    /* Module header */
    int offset = snprintf(doc_buffer, 65536, "# Module: %s\n\n", module_path);

    /* Get module dependencies */
    Cell* deps = module_registry_get_dependencies(module_path);
    if (!cell_is_nil(deps)) {
        offset += snprintf(doc_buffer + offset, 65536 - offset, "## Dependencies\n\n");
        Cell* curr = deps;
        while (cell_is_pair(curr)) {
            Cell* dep = cell_car(curr);
            if (cell_is_string(dep)) {
                offset += snprintf(doc_buffer + offset, 65536 - offset,
                                   "- `%s`\n", cell_get_string(dep));
            }
            curr = cell_cdr(curr);
        }
        offset += snprintf(doc_buffer + offset, 65536 - offset, "\n");
    }
    cell_release(deps);

    /* Functions section */
    offset += snprintf(doc_buffer + offset, 65536 - offset, "## Functions\n\n");

    /* Iterate over symbols */
    Cell* curr_sym = symbols;
    while (cell_is_pair(curr_sym)) {
        Cell* symbol = cell_car(curr_sym);
        const char* sym_name = cell_get_symbol(symbol);

        /* Skip special symbols (starting with underscore or double-colon) */
        if (sym_name[0] == '_' || (sym_name[0] == ':' && sym_name[1] == ':')) {
            curr_sym = cell_cdr(curr_sym);
            continue;
        }

        /* Function header */
        offset += snprintf(doc_buffer + offset, 65536 - offset,
                           "### %s\n\n", sym_name);

        /* Get type signature */
        Cell* type_args = cell_cons(symbol, cell_nil());
        Cell* type_sig = prim_doc_type(type_args);
        cell_release(type_args);

        if (cell_is_symbol(type_sig)) {
            const char* type_str = cell_get_symbol(type_sig);
            if (strcmp(type_str, ":unknown-type") != 0) {
                offset += snprintf(doc_buffer + offset, 65536 - offset,
                                   "**Type:** `%s`\n\n", type_str);
            }
        }
        cell_release(type_sig);

        /* Get description */
        Cell* desc_args = cell_cons(symbol, cell_nil());
        Cell* desc = prim_doc_get(desc_args);
        cell_release(desc_args);

        if (cell_is_symbol(desc)) {
            const char* desc_str = cell_get_symbol(desc);
            if (strcmp(desc_str, ":no-documentation") != 0) {
                offset += snprintf(doc_buffer + offset, 65536 - offset,
                                   "**Description:** %s\n\n", desc_str);
            }
        }
        cell_release(desc);

        /* Get dependencies */
        Cell* deps_args = cell_cons(symbol, cell_nil());
        Cell* func_deps = prim_doc_deps(deps_args);
        cell_release(deps_args);

        if (cell_is_pair(func_deps)) {
            offset += snprintf(doc_buffer + offset, 65536 - offset,
                               "**Uses:** ");

            Cell* dep_curr = func_deps;
            int dep_count = 0;
            while (cell_is_pair(dep_curr)) {
                Cell* dep = cell_car(dep_curr);
                if (cell_is_symbol(dep)) {
                    if (dep_count > 0) {
                        offset += snprintf(doc_buffer + offset, 65536 - offset, ", ");
                    }
                    offset += snprintf(doc_buffer + offset, 65536 - offset,
                                       "`%s`", cell_get_symbol(dep));
                    dep_count++;
                }
                dep_curr = cell_cdr(dep_curr);
            }

            if (dep_count > 0) {
                offset += snprintf(doc_buffer + offset, 65536 - offset, "\n\n");
            }
        }
        cell_release(func_deps);

        /* Separator */
        offset += snprintf(doc_buffer + offset, 65536 - offset, "---\n\n");

        curr_sym = cell_cdr(curr_sym);
    }

    cell_release(symbols);

    /* Create result string */
    Cell* result = cell_string(doc_buffer);
    free(doc_buffer);

    return result;
}

/* 📖⊛ - Generate module index with cross-references (Day 63 Phase 3) */
Cell* prim_doc_index(Cell* args) {
    /* (📖⊛) or (📖⊛ "output-path") */
    /* Generates markdown index of all loaded modules with cross-references */

    /* Get list of all loaded modules */
    Cell* modules = module_registry_list_modules();
    if (cell_is_nil(modules)) {
        return cell_string("# Module Index\n\nNo modules loaded.\n");
    }

    /* Build markdown index */
    char* index_buffer = malloc(131072);  /* 128KB buffer for index */
    if (!index_buffer) {
        cell_release(modules);
        return cell_error("out-of-memory", cell_nil());
    }

    /* Index header */
    int offset = snprintf(index_buffer, 131072,
                         "# Module Index\n\n"
                         "Documentation index for all loaded modules.\n\n"
                         "## Modules\n\n");

    /* Count modules */
    int module_count = 0;
    Cell* count_curr = modules;
    while (cell_is_pair(count_curr)) {
        module_count++;
        count_curr = cell_cdr(count_curr);
    }

    offset += snprintf(index_buffer + offset, 131072 - offset,
                      "**Total modules loaded:** %d\n\n", module_count);

    /* Build dependency graph */
    offset += snprintf(index_buffer + offset, 131072 - offset,
                      "## Module List\n\n");

    /* Iterate over modules */
    Cell* curr_mod = modules;
    while (cell_is_pair(curr_mod)) {
        Cell* module_path = cell_car(curr_mod);
        if (cell_is_string(module_path)) {
            const char* mod_path = cell_get_string(module_path);

            /* Module header with anchor */
            offset += snprintf(index_buffer + offset, 131072 - offset,
                             "### `%s`\n\n", mod_path);

            /* Get module dependencies */
            Cell* deps = module_registry_get_dependencies(mod_path);
            if (!cell_is_nil(deps)) {
                offset += snprintf(index_buffer + offset, 131072 - offset,
                                 "**Dependencies:**\n");

                Cell* dep_curr = deps;
                while (cell_is_pair(dep_curr)) {
                    Cell* dep = cell_car(dep_curr);
                    if (cell_is_string(dep)) {
                        offset += snprintf(index_buffer + offset, 131072 - offset,
                                         "- `%s`\n", cell_get_string(dep));
                    }
                    dep_curr = cell_cdr(dep_curr);
                }
                offset += snprintf(index_buffer + offset, 131072 - offset, "\n");
            }
            cell_release(deps);

            /* List exported functions */
            Cell* symbols = module_registry_list_symbols(mod_path);
            if (!cell_is_nil(symbols)) {
                int func_count = 0;
                Cell* sym_curr = symbols;
                while (cell_is_pair(sym_curr)) {
                    Cell* sym = cell_car(sym_curr);
                    const char* sym_name = cell_get_symbol(sym);

                    /* Skip internal symbols */
                    if (sym_name[0] != '_' && !(sym_name[0] == ':' && sym_name[1] == ':')) {
                        func_count++;
                    }
                    sym_curr = cell_cdr(sym_curr);
                }

                offset += snprintf(index_buffer + offset, 131072 - offset,
                                 "**Exported functions:** %d\n\n", func_count);

                /* List function names */
                if (func_count > 0) {
                    offset += snprintf(index_buffer + offset, 131072 - offset,
                                     "**Functions:** ");

                    sym_curr = symbols;
                    int listed = 0;
                    while (cell_is_pair(sym_curr)) {
                        Cell* sym = cell_car(sym_curr);
                        const char* sym_name = cell_get_symbol(sym);

                        /* Skip internal symbols */
                        if (sym_name[0] != '_' && !(sym_name[0] == ':' && sym_name[1] == ':')) {
                            if (listed > 0) {
                                offset += snprintf(index_buffer + offset, 131072 - offset, ", ");
                            }
                            offset += snprintf(index_buffer + offset, 131072 - offset,
                                             "`%s`", sym_name);
                            listed++;
                        }
                        sym_curr = cell_cdr(sym_curr);
                    }
                    offset += snprintf(index_buffer + offset, 131072 - offset, "\n\n");
                }
            }
            cell_release(symbols);

            offset += snprintf(index_buffer + offset, 131072 - offset, "---\n\n");
        }
        curr_mod = cell_cdr(curr_mod);
    }

    cell_release(modules);

    /* Check if we should export to file */
    if (!cell_is_nil(args)) {
        Cell* output_path_cell = arg1(args);
        if (cell_is_string(output_path_cell)) {
            const char* output_path = cell_get_string(output_path_cell);

            FILE* f = fopen(output_path, "w");
            if (!f) {
                free(index_buffer);
                return cell_error("file-write-error", output_path_cell);
            }

            fputs(index_buffer, f);
            fclose(f);

            free(index_buffer);

            /* Return output path on success */
            cell_retain(output_path_cell);
            return output_path_cell;
        }
    }

    /* Create result string */
    Cell* result = cell_string(index_buffer);
    free(index_buffer);

    return result;
}

/* ============ Structure Analysis Helpers for Test Generation ============ */

/* Check if expression contains a conditional (?) */
static bool has_conditional(Cell* expr) {
    if (!expr) return false;

    /* Check if this is a conditional expression */
    if (cell_is_pair(expr)) {
        Cell* first = cell_car(expr);
        if (cell_is_symbol(first)) {
            const char* sym = cell_get_symbol(first);
            if (strcmp(sym, "?") == 0) {
                return true;
            }
        }

        /* Recursively check subexpressions */
        if (has_conditional(cell_car(expr))) return true;
        if (has_conditional(cell_cdr(expr))) return true;
    }

    return false;
}

/* Check if expression contains recursion (self-reference) */
static bool has_recursion(Cell* expr, const char* func_name) {
    if (!expr || !func_name) return false;

    /* Check if this symbol is the function name */
    if (cell_is_symbol(expr)) {
        const char* sym = cell_get_symbol(expr);
        if (strcmp(sym, func_name) == 0) {
            return true;
        }
    }

    /* Recursively check pairs */
    if (cell_is_pair(expr)) {
        if (has_recursion(cell_car(expr), func_name)) return true;
        if (has_recursion(cell_cdr(expr), func_name)) return true;
    }

    return false;
}

/* Check if expression contains comparison with zero */
static bool has_zero_comparison(Cell* expr) {
    if (!expr) return false;

    /* Check if this is a comparison with #0 */
    if (cell_is_pair(expr)) {
        Cell* first = cell_car(expr);
        if (cell_is_symbol(first)) {
            const char* sym = cell_get_symbol(first);
            if (strcmp(sym, "≡") == 0 || strcmp(sym, "<") == 0 ||
                strcmp(sym, ">") == 0 || strcmp(sym, "≤") == 0 ||
                strcmp(sym, "≥") == 0) {
                /* Check if any argument is #0 */
                Cell* rest = cell_cdr(expr);
                while (cell_is_pair(rest)) {
                    Cell* arg = cell_car(rest);
                    if (cell_is_number(arg) && cell_get_number(arg) == 0.0) {
                        return true;
                    }
                    rest = cell_cdr(rest);
                }
            }
        }

        /* Recursively check */
        if (has_zero_comparison(cell_car(expr))) return true;
        if (has_zero_comparison(cell_cdr(expr))) return true;
    }

    return false;
}

/* Generate test for conditional branch coverage */
static Cell* generate_branch_test(const char* func_name, Cell* test_list) {
    /* For now, generate a simple branch test */
    char test_name[128];
    snprintf(test_name, sizeof(test_name), ":test-%s-branch", func_name);

    Cell* test_name_sym = cell_symbol(test_name);
    Cell* test_arg = cell_number(1);
    Cell* test_call = cell_cons(cell_symbol(func_name),
                               cell_cons(test_arg, cell_nil()));
    Cell* test_check = cell_cons(cell_symbol("ℕ?"),
                                 cell_cons(test_call, cell_nil()));
    Cell* test = cell_cons(cell_symbol("⊨"),
                          cell_cons(test_name_sym,
                          cell_cons(cell_bool(true),
                          cell_cons(test_check, cell_nil()))));

    return cell_cons(test, test_list);
}

/* Generate test for base case (when recursion detected) */
static Cell* generate_base_case_test(const char* func_name, Cell* test_list) {
    char test_name[128];
    snprintf(test_name, sizeof(test_name), ":test-%s-base-case", func_name);

    Cell* test_name_sym = cell_symbol(test_name);
    Cell* test_arg = cell_number(0);  /* Test with zero for base case */
    Cell* test_call = cell_cons(cell_symbol(func_name),
                               cell_cons(test_arg, cell_nil()));
    Cell* test_check = cell_cons(cell_symbol("ℕ?"),
                                 cell_cons(test_call, cell_nil()));
    Cell* test = cell_cons(cell_symbol("⊨"),
                          cell_cons(test_name_sym,
                          cell_cons(cell_bool(true),
                          cell_cons(test_check, cell_nil()))));

    return cell_cons(test, test_list);
}

/* Generate test for recursive case */
static Cell* generate_recursive_test(const char* func_name, Cell* test_list) {
    char test_name[128];
    snprintf(test_name, sizeof(test_name), ":test-%s-recursive", func_name);

    Cell* test_name_sym = cell_symbol(test_name);
    Cell* test_arg = cell_number(3);  /* Test with small number for recursion */
    Cell* test_call = cell_cons(cell_symbol(func_name),
                               cell_cons(test_arg, cell_nil()));
    Cell* test_check = cell_cons(cell_symbol("ℕ?"),
                                 cell_cons(test_call, cell_nil()));
    Cell* test = cell_cons(cell_symbol("⊨"),
                          cell_cons(test_name_sym,
                          cell_cons(cell_bool(true),
                          cell_cons(test_check, cell_nil()))));

    return cell_cons(test, test_list);
}

/* Generate edge case test for zero handling */
static Cell* generate_zero_edge_test(const char* func_name, Cell* test_list) {
    char test_name[128];
    snprintf(test_name, sizeof(test_name), ":test-%s-zero-edge", func_name);

    Cell* test_name_sym = cell_symbol(test_name);
    Cell* test_arg = cell_number(0);
    Cell* test_call = cell_cons(cell_symbol(func_name),
                               cell_cons(test_arg, cell_nil()));
    Cell* test_check = cell_cons(cell_symbol("ℕ?"),
                                 cell_cons(test_call, cell_nil()));
    Cell* test = cell_cons(cell_symbol("⊨"),
                          cell_cons(test_name_sym,
                          cell_cons(cell_bool(true),
                          cell_cons(test_check, cell_nil()))));

    return cell_cons(test, test_list);
}

/* ⌂⊨ - Auto-generate tests for symbol (TYPE-DIRECTED TEST GENERATION)
 * Args: symbol (function or primitive name)
 * Returns: list of test cases
 *
 * This is the CORE of Guage's first-class testing philosophy.
 * Tests are automatically generated from type signatures using type-directed
 * generation. No hardcoded patterns - fully extensible and comprehensive.
 */
Cell* prim_doc_tests(Cell* args) {
    Cell* name = arg1(args);
    if (!cell_is_symbol(name)) {
        return cell_error("⌂⊨ requires a symbol argument", name);
    }

    const char* sym = cell_get_symbol(name);

    /* Strip leading colon if present (handle :symbol → symbol) */
    if (sym && sym[0] == ':') {
        sym = sym + 1;
    }

    /* Check if it's a primitive */
    const Primitive* prim = primitive_lookup_by_name(sym);
    if (prim) {
        /* Parse type signature */
        const char* type_sig = prim->doc.type_signature;
        TypeExpr* type = type_parse(type_sig);

        if (!type) {
            return cell_error("⌂⊨ invalid type signature", name);
        }

        /* Generate tests using type-directed generation */
        Cell* tests = testgen_for_primitive(sym, type);

        /* Free type expression */
        type_free(type);

        return tests;
    }

    /* Check if it's a user function */
    FunctionDoc* doc = eval_find_user_doc(sym);
    if (doc) {
        Cell* tests = cell_nil();

        /* First: Try type-directed generation if we have a type signature */
        const char* type_sig = doc->type_signature;
        if (type_sig) {
            TypeExpr* type = type_parse(type_sig);
            if (type) {
                tests = testgen_for_primitive(sym, type);
                type_free(type);
            }
        }

        /* Second: Add structure-based tests by analyzing function body */
        EvalContext* ctx = eval_get_current_context();
        Cell* func_value = eval_lookup(ctx, sym);

        if (func_value && cell_is_lambda(func_value)) {
            /* Get lambda body for analysis */
            Cell* body = func_value->data.lambda.body;

            /* Analyze structure and generate tests */
            bool has_cond = has_conditional(body);
            bool has_rec = has_recursion(body, sym);
            bool has_zero = has_zero_comparison(body);

            /* Generate branch coverage test if conditional found */
            if (has_cond) {
                tests = generate_branch_test(sym, tests);
            }

            /* Generate recursion tests if self-reference found */
            if (has_rec) {
                tests = generate_base_case_test(sym, tests);
                tests = generate_recursive_test(sym, tests);
            }

            /* Generate zero edge case test if zero comparison found */
            if (has_zero) {
                tests = generate_zero_edge_test(sym, tests);
            }
        }

        return tests;
    }

    return cell_error("⌂⊨ symbol not found", name);
}

/* ⌂⊨! - Execute auto-generated tests for function
 * Args: quoted symbol (function name)
 * Returns: (passed failed total) list
 *
 * Generates tests using ⌂⊨ and executes them, returning results.
 */
Cell* prim_doc_tests_run(Cell* args) {
    /* Generate tests first */
    Cell* tests = prim_doc_tests(args);

    if (cell_is_error(tests)) {
        return tests;  /* Propagate error */
    }

    if (cell_is_nil(tests)) {
        /* No tests generated - return (0 0 0) */
        return cell_cons(cell_number(0),
               cell_cons(cell_number(0),
               cell_cons(cell_number(0), cell_nil())));
    }

    /* Execute each test and count results */
    int total = 0;
    int passed = 0;
    int failed = 0;

    EvalContext* ctx = eval_get_current_context();

    Cell* current = tests;
    while (cell_is_pair(current)) {
        Cell* test_expr = cell_car(current);
        total++;

        /* Evaluate the test expression */
        Cell* result = eval_internal(ctx, ctx->env, test_expr);

        /* Check if evaluation succeeded (not an error) */
        if (!cell_is_error(result)) {
            passed++;
        } else {
            failed++;
        }

        cell_release(result);
        current = cell_cdr(current);
    }

    cell_release(tests);

    /* Return (passed failed total) */
    return cell_cons(cell_number(passed),
           cell_cons(cell_number(failed),
           cell_cons(cell_number(total), cell_nil())));
}

/* ============ Mutation Testing ============ */

/* Helper: Clone a cell deeply (for mutation) */
static Cell* clone_cell_deep(Cell* cell) {
    if (!cell) return NULL;

    switch (cell->type) {
        case CELL_ATOM_NUMBER:
            return cell_number(cell->data.atom.number);
        case CELL_ATOM_BOOL:
            return cell_bool(cell->data.atom.boolean);
        case CELL_ATOM_NIL:
            return cell_nil();
        case CELL_ATOM_SYMBOL:
            return cell_symbol(cell->data.atom.symbol);
        case CELL_ATOM_STRING:
            return cell_string(cell->data.atom.string);
        case CELL_PAIR: {
            Cell* car = clone_cell_deep(cell_car(cell));
            Cell* cdr = clone_cell_deep(cell_cdr(cell));
            Cell* pair = cell_cons(car, cdr);
            cell_release(car);
            cell_release(cdr);
            return pair;
        }
        case CELL_LAMBDA: {
            /* Clone lambda structure */
            Cell* body = clone_cell_deep(cell->data.lambda.body);
            Cell* env = cell->data.lambda.env;  /* Share environment */
            /* Use cell_lambda: (env, body, arity, source_module, source_line) */
            Cell* lambda = cell_lambda(env, body, cell->data.lambda.arity, "<mutation>", 0);
            cell_release(body);
            return lambda;
        }
        default:
            /* For other types, just retain */
            cell_retain(cell);
            return cell;
    }
}

/* Mutate arithmetic operators */
static Cell* mutate_operator(Cell* expr, int mutation_index, int* current_index) {
    if (!cell_is_pair(expr)) return NULL;

    Cell* first = cell_car(expr);
    if (!cell_is_symbol(first)) return NULL;

    const char* op = cell_get_symbol(first);
    const char* mutations[4] = {NULL, NULL, NULL, NULL};
    int num_mutations = 0;

    /* Define mutation alternatives for each operator */
    if (strcmp(op, "⊕") == 0) {
        mutations[0] = "⊖"; mutations[1] = "⊗"; mutations[2] = "⊘";
        num_mutations = 3;
    } else if (strcmp(op, "⊖") == 0) {
        mutations[0] = "⊕"; mutations[1] = "⊗"; mutations[2] = "⊘";
        num_mutations = 3;
    } else if (strcmp(op, "⊗") == 0) {
        mutations[0] = "⊕"; mutations[1] = "⊖"; mutations[2] = "⊘";
        num_mutations = 3;
    } else if (strcmp(op, "⊘") == 0) {
        mutations[0] = "⊕"; mutations[1] = "⊖"; mutations[2] = "⊗";
        num_mutations = 3;
    } else if (strcmp(op, "≡") == 0) {
        mutations[0] = "≢"; mutations[1] = "<"; mutations[2] = ">";
        num_mutations = 3;
    } else if (strcmp(op, "≢") == 0) {
        mutations[0] = "≡";
        num_mutations = 1;
    } else if (strcmp(op, "<") == 0) {
        mutations[0] = ">"; mutations[1] = "≤"; mutations[2] = "≥";
        num_mutations = 3;
    } else if (strcmp(op, ">") == 0) {
        mutations[0] = "<"; mutations[1] = "≤"; mutations[2] = "≥";
        num_mutations = 3;
    } else if (strcmp(op, "∧") == 0) {
        mutations[0] = "∨";
        num_mutations = 1;
    } else if (strcmp(op, "∨") == 0) {
        mutations[0] = "∧";
        num_mutations = 1;
    }

    if (num_mutations == 0) return NULL;

    /* Check if this is the mutation point we want */
    if (*current_index == mutation_index) {
        /* Apply the first mutation alternative */
        Cell* mutated = clone_cell_deep(expr);
        cell_release(cell_car(mutated));
        mutated->data.pair.car = cell_symbol(mutations[0]);
        cell_retain(mutated->data.pair.car);
        return mutated;
    }

    (*current_index)++;
    return NULL;
}

/* Mutate numeric/boolean constants */
static Cell* mutate_constant(Cell* expr, int mutation_index, int* current_index) {
    if (cell_is_number(expr)) {
        double val = cell_get_number(expr);
        /* Skip 0 and 1 - most likely De Bruijn indices for first two args
         * This is a heuristic; real constants like #2, #3 will still mutate */
        if ((val == 0.0 || val == 1.0)) {
            return NULL;  /* Don't mutate De Bruijn indices */
        }
        if (*current_index == mutation_index) {
            /* Mutate number: n→n+1 for positive, n→n-1 for negative */
            if (val > 0) return cell_number(val + 1.0);
            return cell_number(val - 1.0);
        }
        (*current_index)++;
    } else if (cell_is_bool(expr)) {
        if (*current_index == mutation_index) {
            /* Flip boolean */
            return cell_bool(!cell_get_bool(expr));
        }
        (*current_index)++;
    }
    return NULL;
}

/* Mutate conditional (swap branches) */
static Cell* mutate_conditional(Cell* expr, int mutation_index, int* current_index) {
    if (!cell_is_pair(expr)) return NULL;

    Cell* first = cell_car(expr);
    if (!cell_is_symbol(first)) return NULL;
    if (strcmp(cell_get_symbol(first), "?") != 0) return NULL;

    /* This is a conditional: (? cond then else) */
    Cell* rest = cell_cdr(expr);
    if (!cell_is_pair(rest)) return NULL;

    Cell* cond = cell_car(rest);
    Cell* rest2 = cell_cdr(rest);
    if (!cell_is_pair(rest2)) return NULL;

    Cell* then_branch = cell_car(rest2);
    Cell* rest3 = cell_cdr(rest2);
    if (!cell_is_pair(rest3)) return NULL;

    Cell* else_branch = cell_car(rest3);

    if (*current_index == mutation_index) {
        /* Swap then/else branches */
        Cell* mutated_cond = clone_cell_deep(cond);
        Cell* mutated_then = clone_cell_deep(else_branch);  /* Swapped! */
        Cell* mutated_else = clone_cell_deep(then_branch);  /* Swapped! */

        Cell* result = cell_cons(cell_symbol("?"),
                       cell_cons(mutated_cond,
                       cell_cons(mutated_then,
                       cell_cons(mutated_else, cell_nil()))));

        cell_release(mutated_cond);
        cell_release(mutated_then);
        cell_release(mutated_else);

        return result;
    }

    (*current_index)++;
    return NULL;
}

/* Recursively generate a single mutant from expression */
static Cell* generate_single_mutant(Cell* expr, int mutation_index, int* current_index) {
    if (!expr) return NULL;

    /* Try operator mutation */
    Cell* mutated = mutate_operator(expr, mutation_index, current_index);
    if (mutated) return mutated;

    /* Try constant mutation */
    mutated = mutate_constant(expr, mutation_index, current_index);
    if (mutated) return mutated;

    /* Try conditional mutation */
    mutated = mutate_conditional(expr, mutation_index, current_index);
    if (mutated) return mutated;

    /* Recursively try to mutate subexpressions */
    if (cell_is_pair(expr)) {
        Cell* car = cell_car(expr);
        Cell* cdr = cell_cdr(expr);

        /* Try mutating car */
        mutated = generate_single_mutant(car, mutation_index, current_index);
        if (mutated) {
            Cell* cloned_cdr = clone_cell_deep(cdr);
            Cell* result = cell_cons(mutated, cloned_cdr);
            cell_release(mutated);
            cell_release(cloned_cdr);
            return result;
        }

        /* Try mutating cdr (don't reset index - continue counting) */
        mutated = generate_single_mutant(cdr, mutation_index, current_index);
        if (mutated) {
            Cell* cloned_car = clone_cell_deep(car);
            Cell* result = cell_cons(cloned_car, mutated);
            cell_release(cloned_car);
            cell_release(mutated);
            return result;
        }
    }

    return NULL;
}

/* Count total mutation points in expression */
static int count_mutation_points(Cell* expr) {
    if (!expr) return 0;

    int count = 0;

    /* Check if this node is a mutation point */
    if (cell_is_pair(expr)) {
        Cell* first = cell_car(expr);
        if (cell_is_symbol(first)) {
            const char* op = cell_get_symbol(first);
            /* Arithmetic operators */
            if (strcmp(op, "⊕") == 0 || strcmp(op, "⊖") == 0 ||
                strcmp(op, "⊗") == 0 || strcmp(op, "⊘") == 0) {
                count++;
            }
            /* Comparison operators */
            else if (strcmp(op, "≡") == 0 || strcmp(op, "≢") == 0 ||
                     strcmp(op, "<") == 0 || strcmp(op, ">") == 0) {
                count++;
            }
            /* Logical operators */
            else if (strcmp(op, "∧") == 0 || strcmp(op, "∨") == 0) {
                count++;
            }
            /* Conditionals */
            else if (strcmp(op, "?") == 0) {
                count++;
            }
        }
    }

    /* Constants (but skip 0 and 1 - likely De Bruijn indices) */
    if (cell_is_number(expr)) {
        double val = cell_get_number(expr);
        /* Skip 0 and 1 - most likely De Bruijn indices
         * Real constants like #2, #3, #42 will still count */
        if (!(val == 0.0 || val == 1.0)) {
            count++;
        }
    } else if (cell_is_bool(expr)) {
        count++;
    }

    /* Recursively count in subexpressions */
    if (cell_is_pair(expr)) {
        count += count_mutation_points(cell_car(expr));
        count += count_mutation_points(cell_cdr(expr));
    }

    return count;
}

/* ⌂⊨⊗ - Mutation testing
 * Args: quoted symbol (function name)
 * Returns: (killed survived total)
 */
Cell* prim_mutation_test(Cell* args) {
    if (!cell_is_pair(args)) {
        return cell_error("⌂⊨⊗ requires a quoted function name", cell_nil());
    }

    Cell* quoted = arg1(args);
    if (!cell_is_symbol(quoted)) {
        return cell_error("⌂⊨⊗ requires a symbol argument", quoted);
    }

    const char* func_name = cell_get_symbol(quoted);

    /* Strip leading colon if present (handle :symbol → symbol) */
    if (func_name && func_name[0] == ':') {
        func_name = func_name + 1;
    }

    /* Look up function in environment */
    EvalContext* ctx = eval_get_current_context();
    Cell* original_func = eval_lookup(ctx, func_name);

    if (!original_func) {
        return cell_error("⌂⊨⊗ function not found", quoted);
    }

    /* Verify it's a lambda */
    if (original_func->type != CELL_LAMBDA) {
        cell_release(original_func);
        return cell_error("⌂⊨⊗ argument must be a function", quoted);
    }

    /* Get lambda body */
    Cell* body = original_func->data.lambda.body;

    /* Count total mutation points */
    int total_mutations = count_mutation_points(body);

    if (total_mutations == 0) {
        cell_release(original_func);
        /* No mutations possible - return (0 0 0) */
        return cell_cons(cell_number(0),
               cell_cons(cell_number(0),
               cell_cons(cell_number(0), cell_nil())));
    }

    /* Generate tests for this function */
    Cell* test_gen_args = cell_cons(quoted, cell_nil());
    Cell* tests = prim_doc_tests(test_gen_args);
    cell_release(test_gen_args);

    if (cell_is_error(tests)) {
        cell_release(original_func);
        return tests;  /* Propagate error */
    }

    if (cell_is_nil(tests)) {
        cell_release(original_func);
        cell_release(tests);
        /* No tests to run - return (0 0 total) */
        return cell_cons(cell_number(0),
               cell_cons(cell_number(0),
               cell_cons(cell_number(total_mutations), cell_nil())));
    }

    /* Run mutation testing */
    int killed = 0;
    int survived = 0;

    for (int i = 0; i < total_mutations; i++) {
        /* Generate mutant */
        int index = 0;
        Cell* mutant_body = generate_single_mutant(body, i, &index);

        if (!mutant_body) continue;

        /* Create mutated lambda */
        Cell* mutant_env = original_func->data.lambda.env;
        /* cell_lambda: (env, body, arity, source_module, source_line) */
        Cell* mutant_func = cell_lambda(mutant_env, mutant_body, original_func->data.lambda.arity, "<mutant>", 0);
        cell_release(mutant_body);

        /* Temporarily replace function in environment */
        eval_define(ctx, func_name, mutant_func);

        /* Run tests on mutant */
        bool mutant_killed = false;
        Cell* current_test = tests;
        while (cell_is_pair(current_test)) {
            Cell* test_expr = cell_car(current_test);
            Cell* result = eval_internal(ctx, ctx->env, test_expr);

            /* If test fails (error), mutant is killed */
            if (cell_is_error(result)) {
                mutant_killed = true;
                cell_release(result);
                break;
            }

            cell_release(result);
            current_test = cell_cdr(current_test);
        }

        /* Restore original function */
        eval_define(ctx, func_name, original_func);

        cell_release(mutant_func);

        if (mutant_killed) {
            killed++;
        } else {
            survived++;
        }
    }

    cell_release(original_func);
    cell_release(tests);

    /* Return (killed survived total) */
    return cell_cons(cell_number(killed),
           cell_cons(cell_number(survived),
           cell_cons(cell_number(total_mutations), cell_nil())));
}

/* ============ CFG/DFG Query Primitives ============ */

/* ⌂⟿ - Get Control Flow Graph for function
 * Args: quoted symbol (function name)
 * Returns: CFG graph structure
 */
Cell* prim_query_cfg(Cell* args) {
    if (!cell_is_pair(args)) {
        return cell_error("⌂⟿ requires a quoted function name", cell_nil());
    }

    Cell* quoted = arg1(args);
    if (!cell_is_symbol(quoted)) {
        return cell_error("⌂⟿ requires a symbol argument", quoted);
    }

    const char* func_name = cell_get_symbol(quoted);

    /* Look up function in environment */
    EvalContext* ctx = eval_get_current_context();
    Cell* func_value = eval_lookup(ctx, func_name);

    if (!func_value) {
        return cell_error("⌂⟿ function not found", quoted);
    }

    /* Verify it's a lambda */
    if (func_value->type != CELL_LAMBDA) {
        cell_release(func_value);
        return cell_error("⌂⟿ argument must be a function", quoted);
    }

    /* Get lambda body */
    Cell* body = func_value->data.lambda.body;

    /* Generate CFG */
    Cell* cfg = generate_cfg(body);

    cell_release(func_value);
    return cfg;
}

/* Query data flow graph for function - ⌂⇝ */
Cell* prim_query_dfg(Cell* args) {
    if (!cell_is_pair(args)) {
        return cell_error("⌂⇝ requires a quoted function name", cell_nil());
    }

    Cell* quoted = arg1(args);
    if (!cell_is_symbol(quoted)) {
        return cell_error("⌂⇝ requires a symbol argument", quoted);
    }

    const char* func_name = cell_get_symbol(quoted);

    /* Look up function in environment */
    EvalContext* ctx = eval_get_current_context();
    Cell* func_value = eval_lookup(ctx, func_name);

    if (!func_value) {
        return cell_error("⌂⇝ function not found", quoted);
    }

    /* Verify it's a lambda */
    if (func_value->type != CELL_LAMBDA) {
        cell_release(func_value);
        return cell_error("⌂⇝ argument must be a function", quoted);
    }

    /* Get lambda body and parameter count */
    Cell* body = func_value->data.lambda.body;
    int param_count = func_value->data.lambda.arity;

    /* Generate DFG */
    Cell* dfg = generate_dfg(body, param_count);

    cell_release(func_value);
    return dfg;
}

/* Primitive table - PURE SYMBOLS ONLY */
/* Primitive table - PURE SYMBOLS ONLY
 * EVERY primitive MUST have documentation */
static Primitive primitives[] = {
    /* Core Lambda Calculus */
    {"⟨⟩", prim_cons, 2, {"Construct pair from two values", "α → β → ⟨α β⟩"}},
    {"◁", prim_car, 1, {"Get first element of pair (head)", "⟨α β⟩ → α"}},
    {"▷", prim_cdr, 1, {"Get second element of pair (tail)", "⟨α β⟩ → β"}},

    /* Metaprogramming */
    {"⌜", prim_quote, 1, {"Quote expression (prevent evaluation)", "α → α"}},
    {"⌞", prim_eval, 1, {"Evaluate expression as code", "α → β"}},
    {"⊡", prim_prim_apply, 2, {"Apply primitive to argument list", "(α → β) → [α] → β"}},

    /* Pattern Matching - ∇ is now a SPECIAL FORM in eval.c, not a primitive */

    /* Comparison & Logic */
    {"≡", prim_equal, 2, {"Test if two values are equal", "α → α → 𝔹"}},
    {"≢", prim_not_equal, 2, {"Test if two values are not equal", "α → α → 𝔹"}},
    {"∧", prim_and, 2, {"Logical AND of two booleans", "𝔹 → 𝔹 → 𝔹"}},
    {"∨", prim_or, 2, {"Logical OR of two booleans", "𝔹 → 𝔹 → 𝔹"}},
    {"¬", prim_not, 1, {"Logical NOT of boolean", "𝔹 → 𝔹"}},

    /* Arithmetic */
    {"⊕", prim_add, 2, {"Add two numbers", "ℕ → ℕ → ℕ"}},
    {"⊖", prim_sub, 2, {"Subtract second number from first", "ℕ → ℕ → ℕ"}},
    {"⊗", prim_mul, 2, {"Multiply two numbers", "ℕ → ℕ → ℕ"}},
    {"⊘", prim_div, 2, {"Divide first number by second", "ℕ → ℕ → ℕ"}},
    {"÷", prim_quot, 2, {"Integer division (quotient/floor)", "ℕ → ℕ → ℕ"}},
    {"%", prim_mod, 2, {"Modulo (remainder after division)", "ℕ → ℕ → ℕ"}},
    {"<", prim_lt, 2, {"Test if first number less than second", "ℕ → ℕ → 𝔹"}},
    {">", prim_gt, 2, {"Test if first number greater than second", "ℕ → ℕ → 𝔹"}},
    {"≤", prim_le, 2, {"Test if first number less than or equal to second", "ℕ → ℕ → 𝔹"}},
    {"≥", prim_ge, 2, {"Test if first number greater than or equal to second", "ℕ → ℕ → 𝔹"}},

    /* Math operations */
    {"√", prim_sqrt, 1, {"Square root", "ℕ → ℕ"}},
    {"^", prim_pow, 2, {"Power (exponentiation)", "ℕ → ℕ → ℕ"}},
    {"|", prim_abs, 1, {"Absolute value", "ℕ → ℕ"}},
    {"⌊⌋", prim_floor, 1, {"Floor (round down to integer)", "ℕ → ℕ"}},
    {"⌈⌉", prim_ceil, 1, {"Ceiling (round up to integer)", "ℕ → ℕ"}},
    {"⌊⌉", prim_round, 1, {"Round to nearest integer", "ℕ → ℕ"}},
    {"min", prim_min, 2, {"Minimum of two numbers", "ℕ → ℕ → ℕ"}},
    {"max", prim_max, 2, {"Maximum of two numbers", "ℕ → ℕ → ℕ"}},
    {"sin", prim_sin, 1, {"Sine (radians)", "ℕ → ℕ"}},
    {"cos", prim_cos, 1, {"Cosine (radians)", "ℕ → ℕ"}},
    {"tan", prim_tan, 1, {"Tangent (radians)", "ℕ → ℕ"}},
    {"asin", prim_asin, 1, {"Arcsine (returns radians)", "ℕ → ℕ"}},
    {"acos", prim_acos, 1, {"Arccosine (returns radians)", "ℕ → ℕ"}},
    {"atan", prim_atan, 1, {"Arctangent (returns radians)", "ℕ → ℕ"}},
    {"atan2", prim_atan2, 2, {"Two-argument arctangent (y, x)", "ℕ → ℕ → ℕ"}},
    {"log", prim_log, 1, {"Natural logarithm", "ℕ → ℕ"}},
    {"log10", prim_log10, 1, {"Base-10 logarithm", "ℕ → ℕ"}},
    {"exp", prim_exp, 1, {"Exponential (e^x)", "ℕ → ℕ"}},
    {"π", prim_pi, 0, {"Pi constant (3.14159...)", "ℕ"}},
    {"e", prim_e, 0, {"Euler's number constant (2.71828...)", "ℕ"}},
    {"rand", prim_rand, 0, {"Random number between 0 and 1", "() → ℕ"}},
    {"rand-int", prim_rand_int, 1, {"Random integer from 0 to n-1", "ℕ → ℕ"}},

    /* Type predicates */
    {"ℕ?", prim_is_number, 1, {"Test if value is a number", "α → 𝔹"}},
    {"𝔹?", prim_is_bool, 1, {"Test if value is a boolean", "α → 𝔹"}},
    {":?", prim_is_symbol, 1, {"Test if value is a symbol", "α → 𝔹"}},
    {"∅?", prim_is_nil, 1, {"Test if value is nil", "α → 𝔹"}},
    {"⟨⟩?", prim_is_pair, 1, {"Test if value is a pair", "α → 𝔹"}},
    {"#?", prim_is_atom, 1, {"Test if value is an atom", "α → 𝔹"}},

    /* Type Annotation System */
    {"ℤ", prim_type_int, 0, {"Integer type constant", "() → Type"}},
    {"𝔹", prim_type_bool, 0, {"Boolean type constant", "() → Type"}},
    {"𝕊", prim_type_string, 0, {"String type constant", "() → Type"}},
    {"⊤", prim_type_any, 0, {"Any type constant (top)", "() → Type"}},
    {"∅ₜ", prim_type_nil, 0, {"Nil type constant", "() → Type"}},
    {"→", prim_type_func, -1, {"Function type constructor", "Type → Type → Type"}},
    {"[]ₜ", prim_type_list, 1, {"List type constructor", "Type → Type"}},
    {"⟨⟩ₜ", prim_type_pair, 2, {"Pair type constructor", "Type → Type → Type"}},
    {"∪ₜ", prim_type_union, 2, {"Union type constructor", "Type → Type → Type"}},
    {"∈⊙", prim_typeof, 1, {"Get runtime type of value", "α → Type"}},
    {"∈≡", prim_type_equal, 2, {"Test type equality", "Type → Type → 𝔹"}},
    {"∈⊆", prim_type_subtype, 2, {"Test if first type is subtype of second", "Type → Type → 𝔹"}},
    {"∈!", prim_type_assert, 2, {"Assert value has type, return value or error", "α → Type → α | ⚠"}},
    {"∈", prim_type_declare, 2, {"Declare type annotation for symbol", ":symbol → Type → Type"}},
    {"∈?", prim_type_query, 1, {"Query type annotation for symbol", ":symbol → Type | ∅"}},
    {"∈◁", prim_type_domain, 1, {"Get domain (input type) of function type", "Type → Type"}},
    {"∈▷", prim_type_codomain, 1, {"Get codomain (output type) of function type", "Type → Type"}},
    {"∈⊙ₜ", prim_type_element, 1, {"Get element type of list type", "Type → Type"}},
    {"∈✓", prim_type_validate, 1, {"Validate binding against declared type", ":symbol → 𝔹 | ⚠"}},
    {"∈✓*", prim_type_validate_all, 0, {"Validate ALL declared types", "() → 𝔹 | ⚠"}},
    {"∈⊢", prim_type_check_apply, -1, {"Type-check function application", ":symbol → α... → 𝔹 | ⚠"}},

    /* Type Inference (Day 85) */
    {"∈⍜", prim_type_infer, 1, {"Deep type inference on value", "α → Type"}},
    {"∈⍜⊕", prim_type_prim_sig, 1, {"Get type signature of primitive", ":symbol → Type | ∅"}},

    /* Debug & Error Handling */
    {"⚠", prim_error_create, 2, {"Create error value", ":symbol → α → ⚠"}},
    {"⚠?", prim_is_error, 1, {"Test if value is an error", "α → 𝔹"}},
    {"⚠⊙", prim_error_type, 1, {"Get error type as symbol", "⚠ → :symbol"}},
    {"⚠→", prim_error_data, 1, {"Get error data", "⚠ → α"}},
    {"⊢", prim_assert, 2, {"Assert condition is true, error otherwise", "𝔹 → :symbol → 𝔹 | ⚠"}},
    {"⟲", prim_trace, 1, {"Print value for debugging and return it", "α → α"}},

    /* Self-Introspection */
    /* Note: ⊙ symbol now used for structures (see below) */
    {"⧉", prim_arity, 1, {"Get arity of lambda", "λ → ℕ"}},
    {"⊛", prim_source, 1, {"Get source code of lambda", "λ → expression"}},

    /* Macro System (Day 70) */
    {"⊛⊙", prim_gensym, -1, {"Generate unique symbol for macro hygiene", "() → :symbol | ≈ → :symbol"}},
    {"⧉→", prim_macro_expand, -1, {"Expand macros in expression (debug)", "α → α | α → 𝔹 → α"}},
    {"⧉?", prim_macro_list, 0, {"List all defined macros", "() → [:symbol]"}},

    /* Testing */
    {"≟", prim_deep_equal, 2, {"Deep equality test (recursive)", "α → α → 𝔹"}},
    {"⊨", prim_test_case, 3, {"Run test case: name, expected, actual", ":symbol → α → α → 𝔹 | ⚠"}},

    /* Property-Based Testing */
    {"gen-int", prim_gen_int, 2, {"Generate random integer in range [low, high]", "ℕ → ℕ → ℕ"}},
    {"gen-bool", prim_gen_bool, 0, {"Generate random boolean", "() → 𝔹"}},
    {"gen-symbol", prim_gen_symbol, 1, {"Generate random symbol from list", "[α] → α"}},
    {"gen-list", prim_gen_list, 2, {"Generate random list using generator function", "(()→α) → ℕ → [α]"}},
    {"⊨-prop", prim_test_property, 3, {"Property-based test with shrinking", ":symbol → (α→𝔹) → (()→α) → 𝔹 | ⚠"}},

    /* Effects (placeholder) */
    /* Effects: ⟪, ⟪⟫, ↯, ⟪?, ⟪→ are special forms in eval.c */
    {"⤴", prim_effect_pure, 1, {"Lift pure value (identity)", "α → α"}},
    {"≫", prim_effect_bind, 2, {"Effect bind: apply function to value", "α → (α → β) → β"}},

    /* Actor primitives */
    {"⟳", prim_spawn, 1, {"Spawn new actor with behavior function", "(λ (self) ...) → ⟳[id]"}},
    {"→!", prim_send, 2, {"Send message to actor (fire-and-forget)", "⟳ → α → ∅"}},
    {"←?", prim_receive, 0, {"Receive message (yields if mailbox empty)", "() → α"}},
    {"⟳!", prim_actor_run, 1, {"Run actor scheduler for N ticks", "ℕ → ℕ"}},
    {"⟳?", prim_actor_alive, 1, {"Check if actor is alive", "⟳ → 𝔹"}},
    {"⟳→", prim_actor_result, 1, {"Get finished actor result", "⟳ → α | ⚠"}},
    {"⟳∅", prim_actor_reset, 0, {"Reset all actors (testing)", "() → ∅"}},

    /* Channel primitives */
    {"⟿⊚", prim_chan_create, -1, {"Create channel (optional capacity)", "() → ⟿ | ℕ → ⟿"}},
    {"⟿→", prim_chan_send, 2, {"Send value to channel (yields if full)", "⟿ → α → ∅"}},
    {"⟿←", prim_chan_recv, 1, {"Receive from channel (yields if empty)", "⟿ → α"}},
    {"⟿×", prim_chan_close, 1, {"Close channel", "⟿ → ∅"}},
    {"⟿∅", prim_chan_reset, 0, {"Reset all channels (testing)", "() → ∅"}},
    {"⟿⊞",  prim_chan_select,     -1, {"Select from multiple channels (blocking)", "[⟿] → ⟨⟿ α⟩"}},
    {"⟿⊞?", prim_chan_select_try, -1, {"Try select (non-blocking)", "[⟿] → ⟨⟿ α⟩ | ∅"}},

    /* Documentation primitives */
    {"⌂", prim_doc_get, 1, {"Get documentation for symbol", ":symbol → string"}},
    {"⌂∈", prim_doc_type, 1, {"Get type signature for symbol", ":symbol → string"}},
    {"⌂≔", prim_doc_deps, 1, {"Get dependencies for symbol", ":symbol → [symbols]"}},
    {"⌂⊛", prim_doc_source, 1, {"Get source code for symbol", ":symbol → expression"}},
    {"⌂⊨", prim_doc_tests, 1, {"Auto-generate tests for symbol", ":symbol → [tests]"}},
    {"⌂⊨!", prim_doc_tests_run, 1, {"Execute auto-generated tests for symbol", ":symbol → (passed failed total)"}},
    {"⌂⊨⊗", prim_mutation_test, 1, {"Mutation testing - test test suite quality", ":symbol → (killed survived total)"}},
    {"📖", prim_doc_generate, 1, {"Generate markdown documentation for module", "≈ → ≈"}},
    {"📖→", prim_doc_export, 2, {"Export documentation to file", "≈ → ≈ → ≈"}},
    {"📖⊛", prim_doc_index, 0, {"Generate module index with cross-references", "() → ≈ | ≈ → ≈"}},
    {"⌂⊚", prim_module_info, 1, {"Query module information", "() → [modules] | :symbol → string | string → [symbols]"}},

    /* CFG/DFG Query primitives */
    {"⌂⟿", prim_query_cfg, 1, {"Get control flow graph for function", ":symbol → CFG"}},
    {"⌂⇝", prim_query_dfg, 1, {"Get data flow graph for function", ":symbol → DFG"}},

    /* Structure primitives - Leaf (⊙) */
    {"⊙≔", prim_struct_define_leaf, -1, {"Define leaf structure type with field names", ":symbol → [:symbol] → :symbol"}},
    {"⊙", prim_struct_create, -1, {"Create structure instance with field values", ":symbol → [α] → ⊙"}},
    {"⊙→", prim_struct_get_field, 2, {"Get field value from structure", "⊙ → :symbol → α"}},
    {"⊙←", prim_struct_update_field, 3, {"Update field in structure (immutable)", "⊙ → :symbol → α → ⊙"}},
    {"⊙?", prim_struct_type_check, 2, {"Check if value is structure of given type", "α → :symbol → 𝔹"}},

    /* Structure primitives - Node/ADT (⊚) */
    {"⊚≔", prim_struct_define_node, -1, {"Define node/ADT type with variants", ":symbol → [[variant]] → :symbol"}},
    {"⊚", prim_struct_create_node, -1, {"Create node instance with variant", ":symbol → :symbol → [α] → ⊚"}},
    {"⊚→", prim_struct_get_node, 2, {"Get field value from node", "⊚ → :symbol → α"}},
    {"⊚?", prim_struct_is_node, 3, {"Check if value is node of given type and variant", "α → :symbol → :symbol → 𝔹"}},

    /* Graph primitives */
    {"⊝≔", prim_graph_define, -1, {"Define graph type with graph_type and fields", ":symbol → :symbol → [:symbol] → :symbol"}},
    {"⊝", prim_graph_create, 1, {"Create empty graph instance", ":symbol → ⊝"}},
    {"⊝⊕", prim_graph_add_node, 2, {"Add node to graph (immutable)", "⊝ → α → ⊝"}},
    {"⊝⊗", prim_graph_add_edge, 4, {"Add edge to graph (immutable)", "⊝ → α → α → α → ⊝"}},
    {"⊝→", prim_graph_query, 2, {"Query graph property (:nodes, :edges, :entry, :exit, :metadata)", "⊝ → :symbol → α"}},
    {"⊝?", prim_graph_is, 2, {"Check if value is graph of given type", "α → :symbol → 𝔹"}},

    /* Graph algorithm primitives */
    {"⊝↦", prim_graph_traverse, 4, {"Graph traverse with visitor (BFS or DFS)", "⊝ → :symbol → α → (α → β) → [β]"}},
    {"⊝⊃", prim_graph_reachable, 3, {"Check if to-node reachable from from-node", "⊝ → α → α → 𝔹"}},
    {"⊝⊚", prim_graph_successors, 2, {"Get direct successor nodes", "⊝ → α → [α]"}},
    {"⊝⊙", prim_graph_predecessors, 2, {"Get direct predecessor nodes", "⊝ → α → [α]"}},
    {"⊝⇝", prim_graph_path, 3, {"Find shortest path between nodes", "⊝ → α → α → [α] | ∅"}},
    {"⊝∘", prim_graph_cycles, 1, {"Detect cycles in graph", "⊝ → [[α]] | ∅"}},

    /* String operations */
    {"≈", prim_str, 1, {"Convert value to string", "α → ≈"}},
    {"≈⊕", prim_str_concat, 2, {"Concatenate two strings", "≈ → ≈ → ≈"}},
    {"≈#", prim_str_length, 1, {"Get string length", "≈ → ℕ"}},
    {"≈→", prim_str_ref, 2, {"Get character at index", "≈ → ℕ → :symbol"}},
    {"≈⊂", prim_str_slice, 3, {"Get substring from start to end", "≈ → ℕ → ℕ → ≈"}},
    {"≈?", prim_is_string, 1, {"Test if value is a string", "α → 𝔹"}},
    {"≈∅?", prim_str_empty, 1, {"Test if string is empty", "≈ → 𝔹"}},
    {"≈≡", prim_str_equal, 2, {"Test string equality", "≈ → ≈ → 𝔹"}},
    {"≈<", prim_str_less, 2, {"Test string ordering", "≈ → ≈ → 𝔹"}},

    /* I/O operations - Console */
    {"≋", prim_print, 1, {"Print value to stdout with newline", "α → α"}},
    {"≋≈", prim_print_str, 1, {"Print string to stdout without newline", "≈ → ≈"}},
    {"≋←", prim_read_line, 0, {"Read line from stdin", "() → ≈"}},

    /* I/O operations - Files */
    {"≋⊳", prim_read_file, 1, {"Read entire file as string", "≈ → ≈"}},
    {"≋⊲", prim_write_file, 2, {"Write string to file (overwrites)", "≈ → ≈ → ≈"}},
    {"≋⊕", prim_append_file, 2, {"Append string to file", "≈ → ≈ → ≈"}},
    {"≋?", prim_file_exists, 1, {"Check if file exists", "≈ → 𝔹"}},
    {"≋∅?", prim_file_empty, 1, {"Check if file is empty", "≈ → 𝔹"}},

    /* Module System */
    {"⋘", prim_load, 1, {"Load and evaluate file", "≈ → α"}},
    {"⋖", prim_module_import, 2, {"Validate symbols exist in module", "≈ → [::symbol] → ::ok | ⚠"}},
    {"⌂⊚→", prim_module_dependencies, 1, {"Get module dependencies", "≈ → [≈]"}},

    /* Module System Enhancements (Day 70) */
    {"⌂⊚#", prim_module_version, -1, {"Get/set module version", "≈ → ≈ | ≈ → ≈ → :ok"}},
    {"⌂⊚↑", prim_module_exports, -1, {"Get/set module exports", "≈ → [:symbol] | ≈ → [:symbol] → :ok"}},
    {"⌂⊚⊛", prim_module_cycles, -1, {"Detect circular dependencies", "() → [[≈]] | ≈ → [[≈]]"}},

    {NULL, NULL, 0, {NULL, NULL}}
};

/* Look up primitive by name (returns NULL if not found) */
const Primitive* primitive_lookup_by_name(const char* name) {
    for (int i = 0; primitives[i].name != NULL; i++) {
        if (strcmp(primitives[i].name, name) == 0) {
            return &primitives[i];
        }
    }
    return NULL;
}

/* Initialize primitive environment */
Cell* primitives_init(void) {
    Cell* env = cell_nil();

    /* Build environment as association list */
    for (int i = 0; primitives[i].name != NULL; i++) {
        Cell* name = cell_symbol(primitives[i].name);
        Cell* fn = cell_builtin((void*)primitives[i].fn);
        Cell* binding = cell_cons(name, fn);
        env = cell_cons(binding, env);
    }

    return env;
}

/* Lookup primitive by symbol */
Cell* primitives_lookup(Cell* env, const char* sym) {
    while (cell_is_pair(env)) {
        Cell* binding = cell_car(env);
        if (cell_is_pair(binding)) {
            Cell* name = cell_car(binding);
            if (cell_is_symbol(name)) {
                const char* name_str = cell_get_symbol(name);
                if (strcmp(name_str, sym) == 0) {
                    Cell* value = cell_cdr(binding);
                    cell_retain(value);
                    return value;
                }
            }
        }
        env = cell_cdr(env);
    }

    return NULL;  /* Not found */
}
