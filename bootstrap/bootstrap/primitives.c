#include "primitives.h"
#include <stdio.h>
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
    /* Eval is handled by the evaluator */
    return args;
}

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
    assert(divisor != 0.0);  /* Should be checked by refinement types */
    return cell_number(cell_get_number(a) / divisor);
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

/* Effect primitives (placeholder implementations) */

Cell* prim_effect_block(Cell* args) {
    /* ⟪⟫ - effect computation block */
    /* For now, just evaluate the body */
    return arg1(args);
}

Cell* prim_effect_handle(Cell* args) {
    /* ↯ - effect handler */
    /* Placeholder: will need proper effect handler implementation */
    return arg1(args);
}

Cell* prim_effect_pure(Cell* args) {
    /* ⤴ - pure lift */
    Cell* result = arg1(args);
    cell_retain(result);
    return result;
}

Cell* prim_effect_bind(Cell* args) {
    /* ≫ - effect sequencing (monadic bind) */
    /* Placeholder: will need proper implementation */
    return arg2(args);
}

/* Actor primitives (placeholder implementations) */

Cell* prim_spawn(Cell* args) {
    (void)args;  /* Unused for now */
    /* ⟳ - spawn actor */
    /* Placeholder: will need actor runtime */
    printf("spawn: not yet implemented\n");
    return cell_nil();
}

Cell* prim_send(Cell* args) {
    (void)args;  /* Unused for now */
    /* →! - send message */
    /* Placeholder: will need actor runtime */
    printf("send: not yet implemented\n");
    return cell_nil();
}

Cell* prim_receive(Cell* args) {
    (void)args;  /* Unused for now */
    /* ←? - receive message */
    /* Placeholder: will need actor runtime */
    printf("receive: not yet implemented\n");
    return cell_nil();
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

    /* TODO: Look up in user function registry */
    return cell_symbol("Undocumented user function");
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

    /* TODO: Look up in user function registry */
    return cell_symbol("Unknown type");
}

/* ⌂≔ - Get dependencies for symbol */
Cell* prim_doc_deps(Cell* args) {
    Cell* name = arg1(args);
    (void)name;

    /* Primitives have no dependencies */
    /* TODO: Extract dependencies for user functions */
    return cell_nil();
}

/* ⌂⊛ - Get source code for symbol */
Cell* prim_doc_source(Cell* args) {
    Cell* name = arg1(args);
    const char* sym = cell_get_symbol(name);

    /* Primitives are built-in */
    for (int i = 0; primitives[i].name != NULL; i++) {
        if (strcmp(primitives[i].name, sym) == 0) {
            return cell_symbol("<primitive>");
        }
    }

    /* TODO: Return actual source for user functions */
    return cell_nil();
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
    {"<", prim_lt, 2, {"Test if first number less than second", "ℕ → ℕ → 𝔹"}},
    {">", prim_gt, 2, {"Test if first number greater than second", "ℕ → ℕ → 𝔹"}},
    {"≤", prim_le, 2, {"Test if first number less than or equal to second", "ℕ → ℕ → 𝔹"}},
    {"≥", prim_ge, 2, {"Test if first number greater than or equal to second", "ℕ → ℕ → 𝔹"}},

    /* Type predicates */
    {"ℕ?", prim_is_number, 1, {"Test if value is a number", "α → 𝔹"}},
    {"𝔹?", prim_is_bool, 1, {"Test if value is a boolean", "α → 𝔹"}},
    {":?", prim_is_symbol, 1, {"Test if value is a symbol", "α → 𝔹"}},
    {"∅?", prim_is_nil, 1, {"Test if value is nil", "α → 𝔹"}},
    {"⟨⟩?", prim_is_pair, 1, {"Test if value is a pair", "α → 𝔹"}},
    {"#?", prim_is_atom, 1, {"Test if value is an atom", "α → 𝔹"}},

    /* Debug & Error Handling */
    {"⚠", prim_error_create, 2, {"Create error value", ":symbol → α → ⚠"}},
    {"⚠?", prim_is_error, 1, {"Test if value is an error", "α → 𝔹"}},
    {"⊢", prim_assert, 2, {"Assert condition is true, error otherwise", "𝔹 → :symbol → 𝔹 | ⚠"}},
    {"⟲", prim_trace, 1, {"Print value for debugging and return it", "α → α"}},

    /* Self-Introspection */
    {"⊙", prim_type_of, 1, {"Get type of value as symbol", "α → :symbol"}},
    {"⧉", prim_arity, 1, {"Get arity of lambda", "λ → ℕ"}},
    {"⊛", prim_source, 1, {"Get source code of lambda", "λ → expression"}},

    /* Testing */
    {"≟", prim_deep_equal, 2, {"Deep equality test (recursive)", "α → α → 𝔹"}},
    {"⊨", prim_test_case, 3, {"Run test case: name, expected, actual", ":symbol → α → α → 𝔹 | ⚠"}},

    /* Effects (placeholder) */
    {"⟪⟫", prim_effect_block, 1, {"Effect computation block", "effect → α"}},
    {"↯", prim_effect_handle, 2, {"Handle effect with handler", "effect → handler → α"}},
    {"⤴", prim_effect_pure, 1, {"Lift pure value into effect", "α → effect"}},
    {"≫", prim_effect_bind, 2, {"Sequence effects", "effect → (α → effect) → effect"}},

    /* Actors (placeholder) */
    {"⟳", prim_spawn, 1, {"Spawn new actor", "behavior → actor"}},
    {"→!", prim_send, 2, {"Send message to actor", "actor → message → ()"}},
    {"←?", prim_receive, 0, {"Receive message (blocks)", "() → message"}},

    /* Documentation primitives */
    {"⌂", prim_doc_get, 1, {"Get documentation for symbol", ":symbol → string"}},
    {"⌂∈", prim_doc_type, 1, {"Get type signature for symbol", ":symbol → string"}},
    {"⌂≔", prim_doc_deps, 1, {"Get dependencies for symbol", ":symbol → [symbols]"}},
    {"⌂⊛", prim_doc_source, 1, {"Get source code for symbol", ":symbol → expression"}},

    {NULL, NULL, 0, {NULL, NULL}}
};

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
