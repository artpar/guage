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
    TEST("Boolean type", "𝔹");
    TEST("Type variable", "α");
    TEST("Symbol type", ":symbol");
    TEST("Error type", "⚠");

    /* Function types */
    TEST("Unary function", "α → β");
    TEST("Binary function", "α → β → γ");
    TEST("Binary arithmetic", "ℕ → ℕ → ℕ");

    /* Compound types */
    TEST("Pair type", "⟨α β⟩");
    TEST("List type", "[α]");
    TEST("Pair to car", "⟨α β⟩ → α");
    TEST("Construct pair", "α → β → ⟨α β⟩");

    /* Complex types */
    TEST("Pattern matching", "α → [[pattern result]] → β");
    TEST("Error creation", ":symbol → α → ⚠");
    TEST("Union type", "𝔹 → :symbol → 𝔹 | ⚠");
    TEST("Predicate", "α → 𝔹");
    TEST("Comparison", "ℕ → ℕ → 𝔹");

    /* Real primitive signatures */
    TEST("cons (⟨⟩)", "α → β → ⟨α β⟩");
    TEST("car (◁)", "⟨α β⟩ → α");
    TEST("quote (⌜)", "α → α");
    TEST("eval (⌞)", "α → β");
    TEST("match (∇)", "α → [[pattern result]] → β");
    TEST("equal (≡)", "α → α → 𝔹");
    TEST("and (∧)", "𝔹 → 𝔹 → 𝔹");
    TEST("not (¬)", "𝔹 → 𝔹");
    TEST("add (⊕)", "ℕ → ℕ → ℕ");
    TEST("is-number (ℕ?)", "α → 𝔹");
    TEST("assert (⊢)", "𝔹 → :symbol → 𝔹 | ⚠");
    TEST("test-case (⊨)", ":symbol → α → α → 𝔹 | ⚠");

    printf("\n===== All Tests Complete =====\n");
    return 0;
}
