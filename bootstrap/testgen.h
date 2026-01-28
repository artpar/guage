#ifndef TESTGEN_H
#define TESTGEN_H

#include "type.h"
#include "cell.h"

/* Generate comprehensive tests for a primitive based on its type signature */
Cell* testgen_for_primitive(const char* prim_name, TypeExpr* type);

/* Test generation strategies based on type patterns */
Cell* testgen_binary_arithmetic(const char* name);
Cell* testgen_comparison(const char* name);
Cell* testgen_logical(const char* name);
Cell* testgen_predicate(const char* name);
Cell* testgen_pair_construct(const char* name);
Cell* testgen_pair_access(const char* name, bool is_car);
Cell* testgen_pattern_match(const char* name);
Cell* testgen_quote(const char* name);
Cell* testgen_eval(const char* name);
Cell* testgen_error_create(const char* name);
Cell* testgen_polymorphic(const char* name, TypeExpr* type);

/* Utility: Build test case structure */
Cell* testgen_build_test(const char* test_name, Cell* expected, Cell* actual);

/* Utility: Build simple test checking type of result */
Cell* testgen_build_type_test(const char* prim_name, const char* test_suffix,
                               const char* type_predicate, Cell* args);

#endif /* TESTGEN_H */
