#include "type.h"
#include <stdio.h>
#include <string.h>

#define TEST(name, sig) do { \
    printf("\nTest: %s\n", name); \
    printf("Input: %s\n", sig); \
    TypeExpr* type = type_parse(sig); \
    if (type) { \
        printf("Parsed: "); \
        type_print(type); \
        printf("\n"); \
        if (type_is_function(type)) { \
            printf("Function arity: %d\n", type_function_arity(type)); \
        } \
        type_free(type); \
    } else { \
        printf("PARSE FAILED\n"); \
    } \
} while(0)

int main() {
    printf("===== Type Parser Tests =====\n");

    /* Simple types */
    TEST("Number type", "ℕ");
    TEST("Boolean type", "Bool");
    TEST("Type variable", "α");
    TEST("Symbol type", ":symbol");
    TEST("Error type", "error");

    /* Function types */
    TEST("Unary function", "α -> β");
    TEST("Binary function", "α -> β -> γ");
    TEST("Binary arithmetic", "ℕ -> ℕ -> ℕ");

    /* Compound types */
    TEST("Pair type", "⟨α β⟩");
    TEST("List type", "[α]");
    TEST("Pair to car", "⟨α β⟩ -> α");
    TEST("Construct pair", "α -> β -> ⟨α β⟩");

    /* Complex types */
    TEST("Pattern matching", "α -> [[pattern result]] -> β");
    TEST("Error creation", ":symbol -> α -> error");
    TEST("Union type", "Bool -> :symbol -> Bool | error");
    TEST("Predicate", "α -> Bool");
    TEST("Comparison", "ℕ -> ℕ -> Bool");

    /* Real primitive signatures */
    TEST("cons (cons)", "α -> β -> ⟨α β⟩");
    TEST("car (car)", "⟨α β⟩ -> α");
    TEST("quote (quote)", "α -> α");
    TEST("eval (eval)", "α -> β");
    TEST("match (match)", "α -> [[pattern result]] -> β");
    TEST("equal (equal?)", "α -> α -> Bool");
    TEST("and (and)", "Bool -> Bool -> Bool");
    TEST("not (not)", "Bool -> Bool");
    TEST("add (+)", "ℕ -> ℕ -> ℕ");
    TEST("is-number (number?)", "α -> Bool");
    TEST("assert (assert)", "Bool -> :symbol -> Bool | error");
    TEST("test-case (test-case)", ":symbol -> α -> α -> Bool | error");

    printf("\n===== All Tests Complete =====\n");
    return 0;
}
