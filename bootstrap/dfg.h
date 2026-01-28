#ifndef GUAGE_DFG_H
#define GUAGE_DFG_H

#include "cell.h"

/* Data Flow Graph Generation
 *
 * Auto-generates DFG (Data Flow Graph) for lambda functions.
 * DFG shows how data (values) flow through computations.
 *
 * Structure:
 * - Nodes: Operations (⊕, ⊗, ?, λ, variables, constants)
 * - Edges: Data dependencies (producer → consumer)
 * - Entry: Function parameters (De Bruijn indices)
 * - Exit: Return values
 */

/* Generate DFG for a lambda body
 *
 * Args:
 *   lambda_body - The body expression of the lambda
 *   param_count - Number of parameters (for De Bruijn tracking)
 *
 * Returns:
 *   CELL_GRAPH with graph_type = GRAPH_DFG
 *   - nodes: List of operation expressions
 *   - edges: List of ⟨from_idx to_idx label⟩
 *     * label can be :data (value dependency) or :control (? dependencies)
 *   - entry: Parameter indices (list of numbers)
 *   - exit: Return value indices (list of numbers)
 *   - metadata: Additional DFG properties
 */
Cell* generate_dfg(Cell* lambda_body, int param_count);

/* Helper: Check if expression is an operation (produces a value) */
bool is_operation(Cell* expr);

/* Helper: Check if expression uses a variable (De Bruijn index) */
bool uses_variable(Cell* expr, int var_index);

#endif /* GUAGE_DFG_H */
