#ifndef GUAGE_CFG_H
#define GUAGE_CFG_H

#include "cell.h"

/* Control Flow Graph Generation
 *
 * Auto-generates CFG (Control Flow Graph) for lambda functions.
 * CFG shows all possible execution paths through the function.
 *
 * Structure:
 * - Nodes: Basic blocks (sequences without branches)
 * - Edges: Control flow (true/false/unconditional)
 * - Entry: First block
 * - Exit: Return points
 */

/* Generate CFG for a lambda body
 *
 * Args:
 *   lambda_body - The body expression of the lambda
 *
 * Returns:
 *   CELL_GRAPH with graph_type = GRAPH_CFG
 *   - nodes: List of basic block expressions
 *   - edges: List of ⟨from_idx to_idx label⟩
 *   - entry: Entry block index (number)
 *   - exit: Exit block index (number)
 *   - metadata: Additional CFG properties
 */
Cell* generate_cfg(Cell* lambda_body);

/* Helper: Check if expression is a branch point (conditional) */
bool is_branch_point(Cell* expr);

/* Helper: Check if expression is a terminal (return point) */
bool is_terminal(Cell* expr);

#endif /* GUAGE_CFG_H */
