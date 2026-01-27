#include "dfg.h"
#include "primitives.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============ DFG Generation Implementation ============ */

/* Node builder - collects operation nodes and data edges */
typedef struct {
    Cell** nodes;        /* Array of operation expressions */
    size_t node_count;
    size_t node_capacity;

    Cell** edges;        /* Array of edge cells ⟨from to label⟩ */
    size_t edge_count;
    size_t edge_capacity;

    int* entry_indices;  /* Parameter node indices */
    size_t entry_count;
    size_t entry_capacity;

    int* exit_indices;   /* Return value node indices */
    size_t exit_count;
    size_t exit_capacity;

    int param_count;     /* Number of lambda parameters */
} DFGBuilder;

/* Create new DFG builder */
static DFGBuilder* dfg_builder_create(int param_count) {
    DFGBuilder* builder = (DFGBuilder*)malloc(sizeof(DFGBuilder));
    builder->node_capacity = 16;
    builder->node_count = 0;
    builder->nodes = (Cell**)malloc(sizeof(Cell*) * builder->node_capacity);

    builder->edge_capacity = 32;
    builder->edge_count = 0;
    builder->edges = (Cell**)malloc(sizeof(Cell*) * builder->edge_capacity);

    builder->entry_capacity = 8;
    builder->entry_count = 0;
    builder->entry_indices = (int*)malloc(sizeof(int) * builder->entry_capacity);

    builder->exit_capacity = 8;
    builder->exit_count = 0;
    builder->exit_indices = (int*)malloc(sizeof(int) * builder->exit_capacity);

    builder->param_count = param_count;

    return builder;
}

/* Add node to builder, return node index */
static int dfg_builder_add_node(DFGBuilder* builder, Cell* node_expr) {
    /* Grow if needed */
    if (builder->node_count >= builder->node_capacity) {
        builder->node_capacity *= 2;
        builder->nodes = (Cell**)realloc(builder->nodes,
                                         sizeof(Cell*) * builder->node_capacity);
    }

    /* Add node and retain */
    int idx = (int)builder->node_count;
    cell_retain(node_expr);
    builder->nodes[builder->node_count++] = node_expr;

    return idx;
}

/* Add edge to builder */
static void dfg_builder_add_edge(DFGBuilder* builder, int from, int to, const char* label) {
    /* Grow if needed */
    if (builder->edge_count >= builder->edge_capacity) {
        builder->edge_capacity *= 2;
        builder->edges = (Cell**)realloc(builder->edges,
                                        sizeof(Cell*) * builder->edge_capacity);
    }

    /* Create edge: ⟨from ⟨to ⟨label ∅⟩⟩⟩ */
    Cell* from_cell = cell_number(from);
    Cell* to_cell = cell_number(to);
    Cell* label_cell = cell_symbol(label);

    Cell* edge = cell_cons(from_cell,
                   cell_cons(to_cell,
                     cell_cons(label_cell, cell_nil())));

    builder->edges[builder->edge_count++] = edge;
}

/* Add entry index (parameter) */
static void dfg_builder_add_entry(DFGBuilder* builder, int idx) {
    if (builder->entry_count >= builder->entry_capacity) {
        builder->entry_capacity *= 2;
        builder->entry_indices = (int*)realloc(builder->entry_indices,
                                              sizeof(int) * builder->entry_capacity);
    }
    builder->entry_indices[builder->entry_count++] = idx;
}

/* Add exit index (return value) */
static void dfg_builder_add_exit(DFGBuilder* builder, int idx) {
    if (builder->exit_count >= builder->exit_capacity) {
        builder->exit_capacity *= 2;
        builder->exit_indices = (int*)realloc(builder->exit_indices,
                                             sizeof(int) * builder->exit_capacity);
    }
    builder->exit_indices[builder->exit_count++] = idx;
}

/* Free DFG builder */
static void dfg_builder_free(DFGBuilder* builder) {
    /* Release all nodes */
    for (size_t i = 0; i < builder->node_count; i++) {
        cell_release(builder->nodes[i]);
    }
    free(builder->nodes);

    /* Release all edges */
    for (size_t i = 0; i < builder->edge_count; i++) {
        cell_release(builder->edges[i]);
    }
    free(builder->edges);

    free(builder->entry_indices);
    free(builder->exit_indices);

    free(builder);
}

/* Check if expression is an operation (produces a value) */
bool is_operation(Cell* expr) {
    if (!cell_is_pair(expr)) {
        return false; /* Atoms are values, not operations */
    }

    Cell* head = cell_car(expr);
    if (!cell_is_symbol(head)) {
        return false;
    }

    const char* sym = cell_get_symbol(head);

    /* Arithmetic operations */
    if (strcmp(sym, "⊕") == 0 || strcmp(sym, "⊖") == 0 ||
        strcmp(sym, "⊗") == 0 || strcmp(sym, "⊘") == 0) {
        return true;
    }

    /* Comparison operations */
    if (strcmp(sym, "≡") == 0 || strcmp(sym, "≢") == 0 ||
        strcmp(sym, "<") == 0 || strcmp(sym, ">") == 0 ||
        strcmp(sym, "≤") == 0 || strcmp(sym, "≥") == 0) {
        return true;
    }

    /* Logic operations */
    if (strcmp(sym, "∧") == 0 || strcmp(sym, "∨") == 0 || strcmp(sym, "¬") == 0) {
        return true;
    }

    /* Conditional (produces a value) */
    if (strcmp(sym, "?") == 0) {
        return true;
    }

    /* List operations */
    if (strcmp(sym, "⟨⟩") == 0 || strcmp(sym, "◁") == 0 || strcmp(sym, "▷") == 0) {
        return true;
    }

    /* Lambda application (function call) */
    /* Any other pair is potentially a function application */
    return true;
}

/* Check if expression uses a variable (De Bruijn index) */
bool uses_variable(Cell* expr, int var_index) {
    if (cell_is_number(expr)) {
        /* Check if it's a De Bruijn index matching var_index */
        return (int)cell_get_number(expr) == var_index;
    }

    if (cell_is_pair(expr)) {
        /* Recursively check car and cdr */
        return uses_variable(cell_car(expr), var_index) ||
               uses_variable(cell_cdr(expr), var_index);
    }

    return false;
}

/* Walk AST and build DFG
 * Returns the node index that produces the result of this expression
 */
static int dfg_walk(DFGBuilder* builder, Cell* expr) {
    if (expr == NULL) {
        return -1;
    }

    /* Constants produce values directly */
    if (cell_is_number(expr) && cell_get_number(expr) >= 0 &&
        cell_get_number(expr) < builder->param_count) {
        /* This is a De Bruijn index (parameter reference) */
        int param_idx = (int)cell_get_number(expr);

        /* Create node for parameter */
        Cell* param_node = cell_cons(cell_symbol(":param"),
                                     cell_cons(cell_number(param_idx), cell_nil()));
        int node_idx = dfg_builder_add_node(builder, param_node);
        cell_release(param_node);

        /* Mark as entry point */
        dfg_builder_add_entry(builder, node_idx);

        return node_idx;
    }

    /* Other atoms (numbers, bools, symbols) are constant values */
    if (!cell_is_pair(expr)) {
        int node_idx = dfg_builder_add_node(builder, expr);
        return node_idx;
    }

    /* Operations */
    Cell* head = cell_car(expr);

    if (!cell_is_symbol(head)) {
        /* Function application - treat as operation */
        int node_idx = dfg_builder_add_node(builder, expr);

        /* Walk function */
        int func_idx = dfg_walk(builder, head);
        if (func_idx >= 0) {
            dfg_builder_add_edge(builder, func_idx, node_idx, ":data");
        }

        /* Walk arguments */
        Cell* args = cell_cdr(expr);
        while (cell_is_pair(args)) {
            int arg_idx = dfg_walk(builder, cell_car(args));
            if (arg_idx >= 0) {
                dfg_builder_add_edge(builder, arg_idx, node_idx, ":data");
            }
            args = cell_cdr(args);
        }

        return node_idx;
    }

    const char* op = cell_get_symbol(head);

    /* Conditional: test, then, else */
    if (strcmp(op, "?") == 0) {
        int node_idx = dfg_builder_add_node(builder, expr);

        Cell* rest = cell_cdr(expr);
        if (cell_is_pair(rest)) {
            /* Test expression */
            int test_idx = dfg_walk(builder, cell_car(rest));
            if (test_idx >= 0) {
                dfg_builder_add_edge(builder, test_idx, node_idx, ":control");
            }

            rest = cell_cdr(rest);
            if (cell_is_pair(rest)) {
                /* Then branch */
                int then_idx = dfg_walk(builder, cell_car(rest));
                if (then_idx >= 0) {
                    dfg_builder_add_edge(builder, then_idx, node_idx, ":data");
                }

                rest = cell_cdr(rest);
                if (cell_is_pair(rest)) {
                    /* Else branch */
                    int else_idx = dfg_walk(builder, cell_car(rest));
                    if (else_idx >= 0) {
                        dfg_builder_add_edge(builder, else_idx, node_idx, ":data");
                    }
                }
            }
        }

        return node_idx;
    }

    /* Binary operations: ⊕, ⊖, ⊗, ⊘, ≡, <, etc. */
    if (is_operation(expr)) {
        int node_idx = dfg_builder_add_node(builder, expr);

        /* Walk operands */
        Cell* args = cell_cdr(expr);
        while (cell_is_pair(args)) {
            int arg_idx = dfg_walk(builder, cell_car(args));
            if (arg_idx >= 0) {
                dfg_builder_add_edge(builder, arg_idx, node_idx, ":data");
            }
            args = cell_cdr(args);
        }

        return node_idx;
    }

    /* Default: treat as operation */
    int node_idx = dfg_builder_add_node(builder, expr);
    return node_idx;
}

/* Generate DFG for a lambda body */
Cell* generate_dfg(Cell* lambda_body, int param_count) {
    if (lambda_body == NULL) {
        return cell_nil();
    }

    /* Create builder */
    DFGBuilder* builder = dfg_builder_create(param_count);

    /* Walk the lambda body */
    int result_idx = dfg_walk(builder, lambda_body);

    /* Mark result as exit point */
    if (result_idx >= 0) {
        dfg_builder_add_exit(builder, result_idx);
    }

    /* Build nodes list */
    Cell* nodes = cell_nil();
    for (int i = (int)builder->node_count - 1; i >= 0; i--) {
        cell_retain(builder->nodes[i]);
        nodes = cell_cons(builder->nodes[i], nodes);
    }

    /* Build edges list */
    Cell* edges = cell_nil();
    for (int i = (int)builder->edge_count - 1; i >= 0; i--) {
        cell_retain(builder->edges[i]);
        edges = cell_cons(builder->edges[i], edges);
    }

    /* Build entry list */
    Cell* entry = cell_nil();
    for (int i = (int)builder->entry_count - 1; i >= 0; i--) {
        entry = cell_cons(cell_number(builder->entry_indices[i]), entry);
    }

    /* Build exit list */
    Cell* exit = cell_nil();
    for (int i = (int)builder->exit_count - 1; i >= 0; i--) {
        exit = cell_cons(cell_number(builder->exit_indices[i]), exit);
    }

    /* Build metadata with entry/exit */
    Cell* metadata = cell_cons(
        cell_cons(cell_symbol(":entry"), entry),
        cell_cons(
            cell_cons(cell_symbol(":exit"), exit),
            cell_nil()
        )
    );

    /* Create DFG graph */
    Cell* dfg = cell_graph(GRAPH_DFG, nodes, edges, metadata);

    /* Set entry and exit explicitly in graph structure */
    Cell* result = dfg;
    if (result->type == CELL_GRAPH) {
        cell_retain(entry);
        result->data.graph.entry = entry;
        cell_retain(exit);
        result->data.graph.exit = exit;
    }

    /* Cleanup builder only - cell_graph retains nodes/edges/metadata */
    dfg_builder_free(builder);

    /* Don't release nodes/edges/metadata - they're owned by the graph now */

    return result;
}
