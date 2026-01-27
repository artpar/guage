#ifndef GUAGE_PATTERN_CHECK_H
#define GUAGE_PATTERN_CHECK_H

#include "cell.h"
#include <stdbool.h>

/* Pattern Exhaustiveness Checking
 *
 * Analyzes pattern match clauses to determine:
 * - Coverage: Are all cases handled?
 * - Redundancy: Are some patterns unreachable?
 * - Completeness: Is there a catch-all (wildcard/variable)?
 *
 * Emits warnings (not errors) to help improve code safety.
 */

/* Coverage analysis result */
typedef enum {
    COVERAGE_COMPLETE,     /* All cases covered (has wildcard/variable) */
    COVERAGE_PARTIAL,      /* Some cases may be missing */
    COVERAGE_REDUNDANT     /* Has unreachable patterns */
} CoverageStatus;

typedef struct {
    CoverageStatus status;
    int first_unreachable;  /* Index of first unreachable pattern (-1 if none) */
    bool has_catch_all;     /* Has wildcard or variable pattern */
} ExhaustivenessResult;

/* Check if patterns are exhaustive
 *
 * Analyzes a list of pattern clauses to determine coverage.
 * Returns information about completeness and redundancy.
 *
 * clauses: List of (pattern result) pairs
 *
 * Returns:
 *   - COVERAGE_COMPLETE: Has catch-all, all reachable
 *   - COVERAGE_PARTIAL: No catch-all, may miss cases
 *   - COVERAGE_REDUNDANT: Has unreachable patterns
 */
ExhaustivenessResult pattern_check_exhaustiveness(Cell* clauses);

/* Emit warning about incomplete pattern match
 *
 * Called when pattern match lacks catch-all for infinite domain.
 * Warns about potential runtime :no-match errors.
 */
void warn_incomplete_match(Cell* value);

/* Emit warning about unreachable pattern
 *
 * Called when pattern appears after catch-all.
 * Helps identify dead code.
 *
 * clause_index: 0-based index of unreachable pattern (for display: +1)
 */
void warn_unreachable_pattern(int clause_index);

#endif /* GUAGE_PATTERN_CHECK_H */
