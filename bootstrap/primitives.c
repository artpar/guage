#include "primitives.h"
#include "eval.h"
#include "cfg.h"
#include "dfg.h"
#include "pattern.h"
#include "type.h"
#include "testgen.h"
#include "module.h"
#include "trampoline.h"
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
#if USE_TRAMPOLINE
        result = trampoline_eval(ctx, expr);
#else
        result = eval(ctx, expr);
#endif
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
        const char* type_sig = doc->type_signature;

        if (type_sig) {
            /* Parse type signature */
            TypeExpr* type = type_parse(type_sig);

            if (type) {
                /* Generate tests using type-directed generation */
                Cell* tests = testgen_for_primitive(sym, type);

                /* Free type expression */
                type_free(type);

                return tests;
            }
        }

        /* No type signature - return empty tests for now */
        /* TODO: Add structure-based test generation for untyped functions */
        return cell_nil();
    }

    return cell_error("⌂⊨ symbol not found", name);
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
    /* Note: ⊙ symbol now used for structures (see below) */
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
    {"⌂⊨", prim_doc_tests, 1, {"Auto-generate tests for symbol", ":symbol → [tests]"}},
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
