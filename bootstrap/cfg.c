#include "cfg.h"
#include "primitives.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============ CFG Generation Implementation ============ */

/* Block builder - collects basic blocks and edges */
typedef struct {
    Cell** blocks;       /* Array of block expressions */
    size_t block_count;
    size_t block_capacity;

    Cell** edges;        /* Array of edge cells ⟨from to label⟩ */
    size_t edge_count;
    size_t edge_capacity;

    int entry_idx;       /* Entry block index */
    int exit_idx;        /* Exit block index */
} CFGBuilder;

/* Create new CFG builder */
static CFGBuilder* cfg_builder_create(void) {
    CFGBuilder* builder = (CFGBuilder*)malloc(sizeof(CFGBuilder));
    builder->block_capacity = 8;
    builder->block_count = 0;
    builder->blocks = (Cell**)malloc(sizeof(Cell*) * builder->block_capacity);

    builder->edge_capacity = 16;
    builder->edge_count = 0;
    builder->edges = (Cell**)malloc(sizeof(Cell*) * builder->edge_capacity);

    builder->entry_idx = -1;
    builder->exit_idx = -1;

    return builder;
}

/* Add block to builder, return block index */
static int cfg_builder_add_block(CFGBuilder* builder, Cell* block_expr) {
    /* Grow if needed */
    if (builder->block_count >= builder->block_capacity) {
        builder->block_capacity *= 2;
        builder->blocks = (Cell**)realloc(builder->blocks,
                                         sizeof(Cell*) * builder->block_capacity);
    }

    /* Add block and retain */
    int idx = (int)builder->block_count;
    cell_retain(block_expr);
    builder->blocks[builder->block_count++] = block_expr;

    return idx;
}

/* Add edge to builder */
static void cfg_builder_add_edge(CFGBuilder* builder, int from, int to, const char* label) {
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

/* Free CFG builder */
static void cfg_builder_free(CFGBuilder* builder) {
    /* Release all blocks */
    for (size_t i = 0; i < builder->block_count; i++) {
        cell_release(builder->blocks[i]);
    }
    free(builder->blocks);

    /* Release all edges */
    for (size_t i = 0; i < builder->edge_count; i++) {
        cell_release(builder->edges[i]);
    }
    free(builder->edges);

    free(builder);
}

/* Check if expression is a conditional (branch point) */
bool is_branch_point(Cell* expr) {
    if (!cell_is_pair(expr)) {
        return false;
    }

    Cell* op = cell_car(expr);
    if (!cell_is_symbol(op)) {
        return false;
    }

    const char* sym = cell_get_symbol(op);
    return strcmp(sym, "if") == 0;  /* Conditional */
}

/* Check if expression is terminal (return/exit) */
bool is_terminal(Cell* expr) {
    /* For now, all expressions are potentially terminal
     * In future: detect unreachable code, infinite loops */
    (void)expr;
    return true;
}

/* Walk expression tree and build CFG
 * Returns block index where this expression ends */
static int cfg_walk(CFGBuilder* builder, Cell* expr, int current_block) {
    if (expr == NULL || cell_is_nil(expr)) {
        /* Empty expression - just return current block */
        return current_block;
    }

    /* Check if this is a branch point (conditional) */
    if (is_branch_point(expr)) {
        /* Expression is: (? test then else)
         *
         * CFG structure:
         *   [test block]
         *      |
         *      +--true--> [then block] --+
         *      |                         |
         *      +--false-> [else block] --+
         *                                |
         *                         [join block]
         */

        /* Add test block */
        Cell* test = cell_car(cell_cdr(expr));  /* Get test expression */
        int test_block = cfg_builder_add_block(builder, test);

        /* Edge from current to test */
        if (current_block >= 0) {
            cfg_builder_add_edge(builder, current_block, test_block, ":unconditional");
        }

        /* Get then and else branches */
        Cell* then_expr = cell_car(cell_cdr(cell_cdr(expr)));
        Cell* else_expr = cell_car(cell_cdr(cell_cdr(cell_cdr(expr))));

        /* Add then block */
        int then_block = cfg_builder_add_block(builder, then_expr);
        cfg_builder_add_edge(builder, test_block, then_block, ":true");

        /* Walk then branch recursively */
        int then_exit = cfg_walk(builder, then_expr, then_block);

        /* Add else block */
        int else_block = cfg_builder_add_block(builder, else_expr);
        cfg_builder_add_edge(builder, test_block, else_block, ":false");

        /* Walk else branch recursively */
        int else_exit = cfg_walk(builder, else_expr, else_block);

        /* Create join block (where both branches meet)
         * For now, we just return the then_exit as the continuation point
         * In a full implementation, we'd create an explicit join block */
        (void)else_exit;  /* TODO: proper join block */

        return then_exit;
    }

    /* Not a branch - it's a basic block (single expression or sequence) */
    int block_idx = cfg_builder_add_block(builder, expr);

    /* Edge from current to this block */
    if (current_block >= 0) {
        cfg_builder_add_edge(builder, current_block, block_idx, ":unconditional");
    }

    /* Check if expr contains nested expressions (function application) */
    if (cell_is_pair(expr)) {
        Cell* op = cell_car(expr);

        /* If it's a lambda application, we might recurse into arguments
         * For simplicity in v1, we treat the whole application as one block */

        /* Check for recursive calls (future: detect cycles) */
        if (cell_is_symbol(op)) {
            /* Could be recursive call - mark in metadata */
        }
    }

    return block_idx;
}

/* Generate CFG for a lambda body */
Cell* generate_cfg(Cell* lambda_body) {
    if (lambda_body == NULL) {
        return cell_error("CFG generation: lambda_body is NULL", cell_nil());
    }

    /* Create CFG builder */
    CFGBuilder* builder = cfg_builder_create();

    /* Set entry block to -1 initially (no parent) */
    int entry_idx = 0;

    /* Walk the lambda body and build CFG */
    int exit_idx = cfg_walk(builder, lambda_body, -1);

    /* Set entry and exit in builder */
    builder->entry_idx = entry_idx;
    builder->exit_idx = exit_idx;

    /* Convert blocks array to list */
    Cell* nodes = cell_nil();
    for (int i = (int)builder->block_count - 1; i >= 0; i--) {
        Cell* block = builder->blocks[i];
        cell_retain(block);
        nodes = cell_cons(block, nodes);
    }

    /* Convert edges array to list */
    Cell* edges = cell_nil();
    for (int i = (int)builder->edge_count - 1; i >= 0; i--) {
        Cell* edge = builder->edges[i];
        cell_retain(edge);
        edges = cell_cons(edge, edges);
    }

    /* Create metadata with entry/exit */
    Cell* entry_cell = cell_number(builder->entry_idx);
    Cell* exit_cell = cell_number(builder->exit_idx);

    Cell* metadata = cell_cons(
        cell_cons(cell_symbol(":entry"), entry_cell),
        cell_cons(
            cell_cons(cell_symbol(":exit"), exit_cell),
            cell_nil()
        )
    );

    /* Create CFG graph */
    Cell* cfg = cell_graph(GRAPH_CFG, nodes, edges, metadata);

    /* Set entry and exit explicitly in graph structure */
    Cell* result = cfg;
    if (result->type == CELL_GRAPH) {
        cell_retain(entry_cell);
        result->data.graph.entry = entry_cell;
        cell_retain(exit_cell);
        result->data.graph.exit = exit_cell;
    }

    /* Cleanup builder */
    cfg_builder_free(builder);

    return result;
}
