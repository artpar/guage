#include "testgen.h"
#include "type.h"
#include "cell.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Build test case: (⊨ :test-name expected actual) */
Cell* testgen_build_test(const char* test_name, Cell* expected, Cell* actual) {
    Cell* name_sym = cell_symbol(test_name);
    Cell* test = cell_cons(cell_symbol("⊨"),
                 cell_cons(name_sym,
                 cell_cons(expected,
                 cell_cons(actual, cell_nil()))));
    return test;
}

/* Build type check test: (⊨ :test-name #t (TYPE? (prim args...))) */
Cell* testgen_build_type_test(const char* prim_name, const char* test_suffix,
                               const char* type_predicate, Cell* args) {
    char test_name[256];
    snprintf(test_name, sizeof(test_name), ":test-%s-%s", prim_name, test_suffix);

    /* Build (prim args...) */
    Cell* prim_call = cell_cons(cell_symbol(prim_name), args);

    /* Build (TYPE? (prim args...)) */
    Cell* type_check = cell_cons(cell_symbol(type_predicate),
                       cell_cons(prim_call, cell_nil()));

    /* Build test */
    return testgen_build_test(test_name, cell_bool(true), type_check);
}

/* Binary arithmetic: ℕ → ℕ → ℕ */
Cell* testgen_binary_arithmetic(const char* name) {
    Cell* tests = cell_nil();

    /* Test 1: Normal case - returns number */
    Cell* args1 = cell_cons(cell_number(5), cell_cons(cell_number(3), cell_nil()));
    Cell* test1 = testgen_build_type_test(name, "normal", "ℕ?", args1);
    tests = cell_cons(test1, tests);

    /* Test 2: Zero operand */
    Cell* args2 = cell_cons(cell_number(0), cell_cons(cell_number(5), cell_nil()));
    Cell* test2 = testgen_build_type_test(name, "zero", "ℕ?", args2);
    tests = cell_cons(test2, tests);

    /* Test 3: Identity/Special case */
    if (strcmp(name, "⊕") == 0 || strcmp(name, "⊖") == 0) {
        /* 0 + x = x, x - 0 = x */
        Cell* args3 = cell_cons(cell_number(0), cell_cons(cell_number(0), cell_nil()));
        Cell* test3 = testgen_build_type_test(name, "identity", "ℕ?", args3);
        tests = cell_cons(test3, tests);
    }

    return tests;
}

/* Comparison: ℕ → ℕ → 𝔹 */
Cell* testgen_comparison(const char* name) {
    Cell* tests = cell_nil();

    /* Test 1: Returns boolean */
    Cell* args1 = cell_cons(cell_number(5), cell_cons(cell_number(3), cell_nil()));
    Cell* test1 = testgen_build_type_test(name, "returns-bool", "𝔹?", args1);
    tests = cell_cons(test1, tests);

    /* Test 2: Equal values */
    Cell* args2 = cell_cons(cell_number(5), cell_cons(cell_number(5), cell_nil()));
    Cell* test2 = testgen_build_type_test(name, "equal-values", "𝔹?", args2);
    tests = cell_cons(test2, tests);

    /* Test 3: Zero comparison */
    Cell* args3 = cell_cons(cell_number(0), cell_cons(cell_number(5), cell_nil()));
    Cell* test3 = testgen_build_type_test(name, "with-zero", "𝔹?", args3);
    tests = cell_cons(test3, tests);

    return tests;
}

/* Logical: 𝔹 → 𝔹 → 𝔹 or 𝔹 → 𝔹 */
Cell* testgen_logical(const char* name) {
    Cell* tests = cell_nil();

    if (strcmp(name, "¬") == 0) {
        /* Unary: not */
        Cell* args1 = cell_cons(cell_bool(true), cell_nil());
        Cell* test1 = testgen_build_type_test(name, "true", "𝔹?", args1);
        tests = cell_cons(test1, tests);

        Cell* args2 = cell_cons(cell_bool(false), cell_nil());
        Cell* test2 = testgen_build_type_test(name, "false", "𝔹?", args2);
        tests = cell_cons(test2, tests);
    } else {
        /* Binary: and, or */
        Cell* args1 = cell_cons(cell_bool(true), cell_cons(cell_bool(true), cell_nil()));
        Cell* test1 = testgen_build_type_test(name, "both-true", "𝔹?", args1);
        tests = cell_cons(test1, tests);

        Cell* args2 = cell_cons(cell_bool(false), cell_cons(cell_bool(false), cell_nil()));
        Cell* test2 = testgen_build_type_test(name, "both-false", "𝔹?", args2);
        tests = cell_cons(test2, tests);

        Cell* args3 = cell_cons(cell_bool(true), cell_cons(cell_bool(false), cell_nil()));
        Cell* test3 = testgen_build_type_test(name, "mixed", "𝔹?", args3);
        tests = cell_cons(test3, tests);
    }

    return tests;
}

/* Predicate: α → 𝔹 */
Cell* testgen_predicate(const char* name) {
    Cell* tests = cell_nil();

    /* Test with different types */
    Cell* test_values[] = {
        cell_number(42),
        cell_bool(true),
        cell_symbol(":foo"),
        cell_nil(),
        cell_cons(cell_number(1), cell_number(2)),
        NULL
    };

    for (int i = 0; test_values[i] != NULL; i++) {
        char suffix[64];
        snprintf(suffix, sizeof(suffix), "case-%d", i);
        Cell* args = cell_cons(test_values[i], cell_nil());
        Cell* test = testgen_build_type_test(name, suffix, "𝔹?", args);
        tests = cell_cons(test, tests);
    }

    return tests;
}

/* Pair construction: α → β → ⟨α β⟩ */
Cell* testgen_pair_construct(const char* name) {
    Cell* tests = cell_nil();

    /* Test 1: Creates pair */
    Cell* args1 = cell_cons(cell_number(1), cell_cons(cell_number(2), cell_nil()));
    Cell* test1 = testgen_build_type_test(name, "creates-pair", "⟨⟩?", args1);
    tests = cell_cons(test1, tests);

    /* Test 2: With different types */
    Cell* args2 = cell_cons(cell_symbol(":x"), cell_cons(cell_number(42), cell_nil()));
    Cell* test2 = testgen_build_type_test(name, "mixed-types", "⟨⟩?", args2);
    tests = cell_cons(test2, tests);

    /* Test 3: Nested pairs */
    Cell* inner = cell_cons(cell_number(3), cell_number(4));
    Cell* args3 = cell_cons(cell_number(1), cell_cons(inner, cell_nil()));
    Cell* test3 = testgen_build_type_test(name, "nested", "⟨⟩?", args3);
    tests = cell_cons(test3, tests);

    return tests;
}

/* Pair access: ⟨α β⟩ → α or ⟨α β⟩ → β */
Cell* testgen_pair_access(const char* name, bool is_car) {
    Cell* tests = cell_nil();

    /* Create test pair */
    Cell* test_pair = cell_cons(cell_number(10), cell_number(20));

    /* Test: Access returns correct type */
    const char* suffix = is_car ? "gets-first" : "gets-second";
    Cell* args = cell_cons(test_pair, cell_nil());

    /* Check it returns a number (from our test pair) */
    Cell* test = testgen_build_type_test(name, suffix, "ℕ?", args);
    tests = cell_cons(test, tests);

    return tests;
}

/* Pattern matching: α → [[pattern result]] → β */
Cell* testgen_pattern_match(const char* name) {
    Cell* tests = cell_nil();

    /* Test 1: Wildcard pattern
     * (∇ #42 (⟨⟩ (⟨⟩ (⌜ _) (⟨⟩ :ok ∅)) ∅))
     */
    char test1_name[256];
    snprintf(test1_name, sizeof(test1_name), ":test-%s-wildcard", name);

    /* Build: (⌜ _) */
    Cell* wildcard = cell_cons(cell_symbol("⌜"),
                     cell_cons(cell_symbol("_"), cell_nil()));

    /* Build: (⟨⟩ :ok ∅) */
    Cell* result1 = cell_cons(cell_symbol("⟨⟩"),
                    cell_cons(cell_symbol(":ok"),
                    cell_cons(cell_nil(), cell_nil())));

    /* Build: (⟨⟩ (⌜ _) (⟨⟩ :ok ∅)) */
    Cell* clause1 = cell_cons(cell_symbol("⟨⟩"),
                    cell_cons(wildcard,
                    cell_cons(result1, cell_nil())));

    /* Build: (⟨⟩ clause1 ∅) */
    Cell* clauses1 = cell_cons(cell_symbol("⟨⟩"),
                     cell_cons(clause1,
                     cell_cons(cell_nil(), cell_nil())));

    /* Build: (∇ #42 clauses) */
    Cell* match_call1 = cell_cons(cell_symbol(name),
                        cell_cons(cell_number(42),
                        cell_cons(clauses1, cell_nil())));

    /* Build: (≡ (∇ ...) :ok) */
    Cell* check1 = cell_cons(cell_symbol("≡"),
                   cell_cons(match_call1,
                   cell_cons(cell_symbol(":ok"), cell_nil())));

    Cell* test1 = testgen_build_test(test1_name, cell_bool(true), check1);
    tests = cell_cons(test1, tests);

    /* Test 2: Literal pattern
     * (∇ #42 (⟨⟩ (⟨⟩ (⌜ #42) (⟨⟩ :matched ∅)) ∅))
     */
    char test2_name[256];
    snprintf(test2_name, sizeof(test2_name), ":test-%s-literal", name);

    /* Build: (⌜ #42) */
    Cell* lit_pattern = cell_cons(cell_symbol("⌜"),
                        cell_cons(cell_number(42), cell_nil()));

    /* Build: (⟨⟩ :matched ∅) */
    Cell* result2 = cell_cons(cell_symbol("⟨⟩"),
                    cell_cons(cell_symbol(":matched"),
                    cell_cons(cell_nil(), cell_nil())));

    /* Build clause and clauses */
    Cell* clause2 = cell_cons(cell_symbol("⟨⟩"),
                    cell_cons(lit_pattern,
                    cell_cons(result2, cell_nil())));

    Cell* clauses2 = cell_cons(cell_symbol("⟨⟩"),
                     cell_cons(clause2,
                     cell_cons(cell_nil(), cell_nil())));

    /* Build match call */
    Cell* match_call2 = cell_cons(cell_symbol(name),
                        cell_cons(cell_number(42),
                        cell_cons(clauses2, cell_nil())));

    /* Build check */
    Cell* check2 = cell_cons(cell_symbol("≡"),
                   cell_cons(match_call2,
                   cell_cons(cell_symbol(":matched"), cell_nil())));

    Cell* test2 = testgen_build_test(test2_name, cell_bool(true), check2);
    tests = cell_cons(test2, tests);

    /* Test 3: No match error */
    char test3_name[256];
    snprintf(test3_name, sizeof(test3_name), ":test-%s-no-match", name);

    /* Build: (⌜ #99) pattern that won't match #42 */
    Cell* no_match_pattern = cell_cons(cell_symbol("⌜"),
                             cell_cons(cell_number(99), cell_nil()));

    Cell* result3 = cell_cons(cell_symbol("⟨⟩"),
                    cell_cons(cell_symbol(":no"),
                    cell_cons(cell_nil(), cell_nil())));

    Cell* clause3 = cell_cons(cell_symbol("⟨⟩"),
                    cell_cons(no_match_pattern,
                    cell_cons(result3, cell_nil())));

    Cell* clauses3 = cell_cons(cell_symbol("⟨⟩"),
                     cell_cons(clause3,
                     cell_cons(cell_nil(), cell_nil())));

    Cell* match_call3 = cell_cons(cell_symbol(name),
                        cell_cons(cell_number(42),
                        cell_cons(clauses3, cell_nil())));

    /* Check that it produces an error */
    Cell* check3 = cell_cons(cell_symbol("⚠?"),
                   cell_cons(match_call3, cell_nil()));

    Cell* test3 = testgen_build_test(test3_name, cell_bool(true), check3);
    tests = cell_cons(test3, tests);

    return tests;
}

/* Quote: α → α */
Cell* testgen_quote(const char* name) {
    Cell* tests = cell_nil();

    /* Test: Quoting prevents evaluation */
    char test_name[256];
    snprintf(test_name, sizeof(test_name), ":test-%s-prevents-eval", name);

    /* Build: (⌜ (⊕ 1 2)) */
    Cell* expr = cell_cons(cell_symbol("⊕"),
                 cell_cons(cell_number(1),
                 cell_cons(cell_number(2), cell_nil())));
    Cell* args = cell_cons(expr, cell_nil());

    /* Result should be a pair (the quoted expression) */
    Cell* test = testgen_build_type_test(name, "prevents-eval", "⟨⟩?", args);
    tests = cell_cons(test, tests);

    return tests;
}

/* Eval: α → β */
Cell* testgen_eval(const char* name) {
    Cell* tests = cell_nil();

    /* Test: Evaluates quoted expression */
    char test_name[256];
    snprintf(test_name, sizeof(test_name), ":test-%s-evaluates", name);

    /* Build: (⌞ (⌜ (⊕ 1 2))) should return #3 */
    Cell* inner = cell_cons(cell_symbol("⊕"),
                  cell_cons(cell_number(1),
                  cell_cons(cell_number(2), cell_nil())));
    Cell* quoted = cell_cons(cell_symbol("⌜"),
                   cell_cons(inner, cell_nil()));
    Cell* eval_call = cell_cons(cell_symbol(name),
                      cell_cons(quoted, cell_nil()));

    /* Check result is a number */
    Cell* check = cell_cons(cell_symbol("ℕ?"),
                  cell_cons(eval_call, cell_nil()));

    Cell* test = testgen_build_test(test_name, cell_bool(true), check);
    tests = cell_cons(test, tests);

    return tests;
}

/* Error creation: :symbol → α → ⚠ */
Cell* testgen_error_create(const char* name) {
    Cell* tests = cell_nil();

    /* Test: Creates error */
    Cell* args = cell_cons(cell_symbol(":test-error"),
                 cell_cons(cell_number(42), cell_nil()));
    Cell* test = testgen_build_type_test(name, "creates-error", "⚠?", args);
    tests = cell_cons(test, tests);

    return tests;
}

/* Polymorphic/generic functions */
Cell* testgen_polymorphic(const char* name, TypeExpr* type) {
    Cell* tests = cell_nil();

    /* Generate basic type conformance test */
    /* For now, just test that it doesn't crash */
    char test_name[256];
    snprintf(test_name, sizeof(test_name), ":test-%s-polymorphic", name);

    /* Use number as generic test value */
    Cell* args = cell_cons(cell_number(42), cell_nil());

    /* Just check it returns something (not crashes) */
    /* We can't check specific type for polymorphic functions */
    Cell* prim_call = cell_cons(cell_symbol(name), args);

    /* Use (⟨⟩? result) as generic "returns something" test */
    /* Actually, better to just check not error */
    Cell* check = cell_cons(cell_symbol("¬"),
                  cell_cons(cell_cons(cell_symbol("⚠?"),
                  cell_cons(prim_call, cell_nil())), cell_nil()));

    Cell* test = testgen_build_test(test_name, cell_bool(true), check);
    tests = cell_cons(test, tests);

    return tests;
}

/* Main test generation based on type structure */
Cell* testgen_for_primitive(const char* prim_name, TypeExpr* type) {
    if (!type || !prim_name) {
        return cell_nil();
    }

    /* Check for specific patterns */

    /* Pattern matching: α → [[pattern]] → β */
    if (type->kind == TYPE_FUNC &&
        type->data.func.to && type->data.func.to->kind == TYPE_FUNC) {
        TypeExpr* second_arg = type->data.func.to->data.func.from;
        if (second_arg && second_arg->kind == TYPE_PATTERN) {
            return testgen_pattern_match(prim_name);
        }
    }

    /* Binary arithmetic: ℕ → ℕ → ℕ */
    if (type->kind == TYPE_FUNC &&
        type->data.func.from && type->data.func.from->kind == TYPE_NUM &&
        type->data.func.to && type->data.func.to->kind == TYPE_FUNC) {
        TypeExpr* second = type->data.func.to->data.func.from;
        TypeExpr* ret = type->data.func.to->data.func.to;
        if (second && second->kind == TYPE_NUM &&
            ret && ret->kind == TYPE_NUM) {
            return testgen_binary_arithmetic(prim_name);
        }
    }

    /* Comparison: ℕ → ℕ → 𝔹 */
    if (type->kind == TYPE_FUNC &&
        type->data.func.from && type->data.func.from->kind == TYPE_NUM &&
        type->data.func.to && type->data.func.to->kind == TYPE_FUNC) {
        TypeExpr* second = type->data.func.to->data.func.from;
        TypeExpr* ret = type->data.func.to->data.func.to;
        if (second && second->kind == TYPE_NUM &&
            ret && ret->kind == TYPE_BOOL) {
            return testgen_comparison(prim_name);
        }
    }

    /* Logical: 𝔹 → 𝔹 → 𝔹 or 𝔹 → 𝔹 */
    if (type->kind == TYPE_FUNC &&
        type->data.func.from && type->data.func.from->kind == TYPE_BOOL) {
        TypeExpr* ret = type_get_return(type);
        if (ret && ret->kind == TYPE_BOOL) {
            return testgen_logical(prim_name);
        }
    }

    /* Predicate: α → 𝔹 */
    if (type->kind == TYPE_FUNC &&
        type->data.func.from && type->data.func.from->kind == TYPE_VAR) {
        TypeExpr* ret = type->data.func.to;
        if (ret && ret->kind == TYPE_BOOL) {
            return testgen_predicate(prim_name);
        }
    }

    /* Pair construction: α → β → ⟨α β⟩ */
    if (type->kind == TYPE_FUNC &&
        type->data.func.to && type->data.func.to->kind == TYPE_FUNC) {
        TypeExpr* ret = type->data.func.to->data.func.to;
        if (ret && ret->kind == TYPE_PAIR) {
            if (strcmp(prim_name, "⟨⟩") == 0) {
                return testgen_pair_construct(prim_name);
            }
        }
    }

    /* Pair access: ⟨α β⟩ → α or ⟨α β⟩ → β */
    if (type->kind == TYPE_FUNC &&
        type->data.func.from && type->data.func.from->kind == TYPE_PAIR) {
        bool is_car = (strcmp(prim_name, "◁") == 0);
        return testgen_pair_access(prim_name, is_car);
    }

    /* Quote: α → α */
    if (strcmp(prim_name, "⌜") == 0) {
        return testgen_quote(prim_name);
    }

    /* Eval: α → β */
    if (strcmp(prim_name, "⌞") == 0) {
        return testgen_eval(prim_name);
    }

    /* Error creation: :symbol → α → ⚠ */
    if (type->kind == TYPE_FUNC &&
        type->data.func.from && type->data.func.from->kind == TYPE_SYMBOL) {
        TypeExpr* ret = type_get_return(type);
        if (ret && ret->kind == TYPE_ERROR) {
            return testgen_error_create(prim_name);
        }
    }

    /* Default: polymorphic/generic tests */
    return testgen_polymorphic(prim_name, type);
}
