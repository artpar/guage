#include "primitives.h"
#include "eval.h"
#include "cfg.h"
#include "dfg.h"
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

/* % - modulo (remainder) */
Cell* prim_mod(Cell* args) {
    Cell* a = arg1(args);
    Cell* b = arg2(args);
    assert(cell_is_number(a) && cell_is_number(b));
    double divisor = cell_get_number(b);
    assert(divisor != 0.0);  /* Should be checked by refinement types */
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
