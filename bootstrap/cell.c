#include "cell.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* Cell allocation */
static Cell* cell_alloc(CellType type) {
    Cell* c = (Cell*)malloc(sizeof(Cell));
    assert(c != NULL);

    c->type = type;
    c->refcount = 1;
    c->linear_flags = LINEAR_NONE;
    c->caps = CAP_READ | CAP_WRITE | CAP_SHARE;  /* Default capabilities */
    c->marked = false;

    return c;
}

/* Cell creation */
Cell* cell_number(double n) {
    Cell* c = cell_alloc(CELL_ATOM_NUMBER);
    c->data.atom.number = n;
    return c;
}

Cell* cell_bool(bool b) {
    Cell* c = cell_alloc(CELL_ATOM_BOOL);
    c->data.atom.boolean = b;
    return c;
}

Cell* cell_symbol(const char* sym) {
    Cell* c = cell_alloc(CELL_ATOM_SYMBOL);
    c->data.atom.symbol = strdup(sym);
    return c;
}

Cell* cell_string(const char* str) {
    Cell* c = cell_alloc(CELL_ATOM_STRING);
    c->data.atom.string = strdup(str);
    return c;
}

Cell* cell_nil(void) {
    Cell* c = cell_alloc(CELL_ATOM_NIL);
    return c;
}

Cell* cell_cons(Cell* car, Cell* cdr) {
    /* Check linearity: can't use consumed values */
    assert(!cell_is_consumed(car));
    assert(!cell_is_consumed(cdr));

    Cell* c = cell_alloc(CELL_PAIR);
    c->data.pair.car = car;
    c->data.pair.cdr = cdr;

    cell_retain(car);
    cell_retain(cdr);

    return c;
}

Cell* cell_lambda(Cell* env, Cell* body, int arity, const char* source_module, int source_line) {
    Cell* c = cell_alloc(CELL_LAMBDA);
    c->data.lambda.env = env;
    c->data.lambda.body = body;
    c->data.lambda.arity = arity;
    c->data.lambda.source_module = source_module ? strdup(source_module) : NULL;  /* Day 27 */
    c->data.lambda.source_line = source_line;  /* Day 27 */

    if (env) cell_retain(env);
    cell_retain(body);

    return c;
}

Cell* cell_builtin(void* fn) {
    Cell* c = cell_alloc(CELL_BUILTIN);
    c->data.atom.builtin = fn;
    return c;
}

Cell* cell_error(const char* message, Cell* data) {
    Cell* c = cell_alloc(CELL_ERROR);
    c->data.error.message = strdup(message);
    c->data.error.data = data;
    if (data) cell_retain(data);
    return c;
}

Cell* cell_struct(StructKind kind, Cell* type_tag, Cell* variant, Cell* fields) {
    Cell* c = cell_alloc(CELL_STRUCT);
    c->data.structure.kind = kind;
    c->data.structure.type_tag = type_tag;
    c->data.structure.variant = variant;
    c->data.structure.fields = fields;

    if (type_tag) cell_retain(type_tag);
    if (variant) cell_retain(variant);
    if (fields) cell_retain(fields);

    return c;
}

Cell* cell_graph(GraphType graph_type, Cell* nodes, Cell* edges, Cell* metadata) {
    Cell* c = cell_alloc(CELL_GRAPH);
    c->data.graph.graph_type = graph_type;
    c->data.graph.nodes = nodes ? nodes : cell_nil();
    c->data.graph.edges = edges ? edges : cell_nil();
    c->data.graph.metadata = metadata ? metadata : cell_nil();
    c->data.graph.entry = NULL;
    c->data.graph.exit = NULL;

    cell_retain(c->data.graph.nodes);
    cell_retain(c->data.graph.edges);
    cell_retain(c->data.graph.metadata);

    return c;
}

Cell* cell_actor(int actor_id) {
    Cell* c = cell_alloc(CELL_ACTOR);
    c->data.actor.actor_id = actor_id;
    return c;
}

/* Cell accessors */
double cell_get_number(Cell* c) {
    assert(c->type == CELL_ATOM_NUMBER);
    return c->data.atom.number;
}

bool cell_get_bool(Cell* c) {
    assert(c->type == CELL_ATOM_BOOL);
    return c->data.atom.boolean;
}

const char* cell_get_symbol(Cell* c) {
    assert(c->type == CELL_ATOM_SYMBOL);
    return c->data.atom.symbol;
}

const char* cell_get_string(Cell* c) {
    assert(c->type == CELL_ATOM_STRING);
    return c->data.atom.string;
}

Cell* cell_car(Cell* c) {
    assert(c->type == CELL_PAIR);
    assert(!cell_is_consumed(c));
    return c->data.pair.car;
}

Cell* cell_cdr(Cell* c) {
    assert(c->type == CELL_PAIR);
    assert(!cell_is_consumed(c));
    return c->data.pair.cdr;
}

/* Reference counting */
void cell_retain(Cell* c) {
    if (c == NULL) return;
    c->refcount++;
}

void cell_release(Cell* c) {
    if (c == NULL) return;

    assert(c->refcount > 0);
    c->refcount--;

    if (c->refcount == 0) {
        /* Free children first */
        switch (c->type) {
            case CELL_PAIR:
                cell_release(c->data.pair.car);
                cell_release(c->data.pair.cdr);
                break;
            case CELL_LAMBDA:
                cell_release(c->data.lambda.env);
                cell_release(c->data.lambda.body);
                if (c->data.lambda.source_module) {
                    free((void*)c->data.lambda.source_module);  /* Day 27 */
                }
                break;
            case CELL_ATOM_SYMBOL:
                free((void*)c->data.atom.symbol);
                break;
            case CELL_ATOM_STRING:
                free((void*)c->data.atom.string);
                break;
            case CELL_ERROR:
                free((void*)c->data.error.message);
                cell_release(c->data.error.data);
                break;
            case CELL_STRUCT:
                cell_release(c->data.structure.type_tag);
                cell_release(c->data.structure.variant);
                cell_release(c->data.structure.fields);
                break;
            case CELL_GRAPH:
                cell_release(c->data.graph.nodes);
                cell_release(c->data.graph.edges);
                cell_release(c->data.graph.metadata);
                cell_release(c->data.graph.entry);
                cell_release(c->data.graph.exit);
                break;
            case CELL_ACTOR:
                /* Actor ID is just an int, no children to release */
                break;
            default:
                break;
        }

        free(c);
    }
}

/* Linear type operations */
bool cell_is_linear(Cell* c) {
    return (c->linear_flags & LINEAR_UNIQUE) != 0;
}

bool cell_is_consumed(Cell* c) {
    return (c->linear_flags & LINEAR_CONSUMED) != 0;
}

void cell_mark_consumed(Cell* c) {
    c->linear_flags |= LINEAR_CONSUMED;
}

Cell* cell_move(Cell* c) {
    /* Move ownership - original becomes consumed */
    assert(!cell_is_consumed(c));

    if (cell_is_linear(c)) {
        cell_mark_consumed(c);
        /* Create new cell with transferred ownership */
        /* For now, just return the same cell */
        /* In a real implementation, we'd transfer metadata */
    }

    return c;
}

Cell* cell_borrow(Cell* c) {
    /* Temporary borrow - doesn't consume */
    assert(!cell_is_consumed(c));
    c->linear_flags |= LINEAR_BORROWED;
    return c;
}

/* Type predicates */
bool cell_is_number(Cell* c) {
    return c && c->type == CELL_ATOM_NUMBER;
}

bool cell_is_bool(Cell* c) {
    return c && c->type == CELL_ATOM_BOOL;
}

bool cell_is_symbol(Cell* c) {
    return c && c->type == CELL_ATOM_SYMBOL;
}

bool cell_is_string(Cell* c) {
    return c && c->type == CELL_ATOM_STRING;
}

bool cell_is_nil(Cell* c) {
    return c && c->type == CELL_ATOM_NIL;
}

bool cell_is_pair(Cell* c) {
    return c && c->type == CELL_PAIR;
}

bool cell_is_lambda(Cell* c) {
    return c && c->type == CELL_LAMBDA;
}

bool cell_is_atom(Cell* c) {
    return c && (c->type == CELL_ATOM_NUMBER ||
                 c->type == CELL_ATOM_BOOL ||
                 c->type == CELL_ATOM_SYMBOL ||
                 c->type == CELL_ATOM_STRING ||
                 c->type == CELL_ATOM_NIL);
}

bool cell_is_error(Cell* c) {
    return c && c->type == CELL_ERROR;
}

bool cell_is_struct(Cell* c) {
    return c && c->type == CELL_STRUCT;
}

bool cell_is_graph(Cell* c) {
    return c && c->type == CELL_GRAPH;
}

bool cell_is_actor(Cell* c) {
    return c && c->type == CELL_ACTOR;
}

int cell_get_actor_id(Cell* c) {
    assert(c->type == CELL_ACTOR);
    return c->data.actor.actor_id;
}

/* Error accessors */
const char* cell_error_message(Cell* c) {
    assert(c->type == CELL_ERROR);
    return c->data.error.message;
}

Cell* cell_error_data(Cell* c) {
    assert(c->type == CELL_ERROR);
    return c->data.error.data;
}

/* Structure accessors */
StructKind cell_struct_kind(Cell* c) {
    assert(c->type == CELL_STRUCT);
    return c->data.structure.kind;
}

Cell* cell_struct_type_tag(Cell* c) {
    assert(c->type == CELL_STRUCT);
    return c->data.structure.type_tag;
}

Cell* cell_struct_variant(Cell* c) {
    assert(c->type == CELL_STRUCT);
    return c->data.structure.variant;
}

Cell* cell_struct_fields(Cell* c) {
    assert(c->type == CELL_STRUCT);
    return c->data.structure.fields;
}

Cell* cell_struct_get_field(Cell* c, Cell* field_name) {
    assert(c->type == CELL_STRUCT);
    assert(cell_is_symbol(field_name));

    /* Search in alist of fields */
    Cell* fields = c->data.structure.fields;
    while (fields && !cell_is_nil(fields)) {
        Cell* pair = cell_car(fields);  /* (field . value) */
        Cell* fname = cell_car(pair);
        if (cell_equal(fname, field_name)) {
            return cell_cdr(pair);  /* Return value */
        }
        fields = cell_cdr(fields);
    }

    /* Field not found */
    return NULL;
}

/* Graph accessors */
GraphType cell_graph_type(Cell* c) {
    assert(c->type == CELL_GRAPH);
    return c->data.graph.graph_type;
}

Cell* cell_graph_nodes(Cell* c) {
    assert(c->type == CELL_GRAPH);
    return c->data.graph.nodes;
}

Cell* cell_graph_edges(Cell* c) {
    assert(c->type == CELL_GRAPH);
    return c->data.graph.edges;
}

Cell* cell_graph_metadata(Cell* c) {
    assert(c->type == CELL_GRAPH);
    return c->data.graph.metadata;
}

Cell* cell_graph_entry(Cell* c) {
    assert(c->type == CELL_GRAPH);
    return c->data.graph.entry;
}

Cell* cell_graph_exit(Cell* c) {
    assert(c->type == CELL_GRAPH);
    return c->data.graph.exit;
}

/* Graph mutators (return modified graph - immutable style) */
Cell* cell_graph_add_node(Cell* graph, Cell* node) {
    assert(graph->type == CELL_GRAPH);

    /* Create new node list with node prepended */
    Cell* new_nodes = cell_cons(node, graph->data.graph.nodes);

    /* Create new graph with updated nodes */
    Cell* new_graph = cell_graph(
        graph->data.graph.graph_type,
        new_nodes,
        graph->data.graph.edges,
        graph->data.graph.metadata
    );

    /* Copy entry/exit if they exist */
    if (graph->data.graph.entry) {
        new_graph->data.graph.entry = graph->data.graph.entry;
        cell_retain(new_graph->data.graph.entry);
    }
    if (graph->data.graph.exit) {
        new_graph->data.graph.exit = graph->data.graph.exit;
        cell_retain(new_graph->data.graph.exit);
    }

    cell_release(new_nodes);  /* cell_graph retained it */
    return new_graph;
}

Cell* cell_graph_add_edge(Cell* graph, Cell* from, Cell* to, Cell* label) {
    assert(graph->type == CELL_GRAPH);

    /* Create edge as ⟨from ⟨to ⟨label ∅⟩⟩⟩ (proper list) */
    Cell* edge = cell_cons(from, cell_cons(to, cell_cons(label, cell_nil())));

    /* Create new edge list with edge prepended */
    Cell* new_edges = cell_cons(edge, graph->data.graph.edges);

    /* Create new graph with updated edges */
    Cell* new_graph = cell_graph(
        graph->data.graph.graph_type,
        graph->data.graph.nodes,
        new_edges,
        graph->data.graph.metadata
    );

    /* Copy entry/exit */
    if (graph->data.graph.entry) {
        new_graph->data.graph.entry = graph->data.graph.entry;
        cell_retain(new_graph->data.graph.entry);
    }
    if (graph->data.graph.exit) {
        new_graph->data.graph.exit = graph->data.graph.exit;
        cell_retain(new_graph->data.graph.exit);
    }

    cell_release(edge);
    cell_release(new_edges);
    return new_graph;
}

Cell* cell_graph_set_entry(Cell* graph, Cell* entry) {
    assert(graph->type == CELL_GRAPH);

    /* Create new graph with entry set */
    Cell* new_graph = cell_graph(
        graph->data.graph.graph_type,
        graph->data.graph.nodes,
        graph->data.graph.edges,
        graph->data.graph.metadata
    );

    if (new_graph->data.graph.entry) {
        cell_release(new_graph->data.graph.entry);
    }
    new_graph->data.graph.entry = entry;
    if (entry) cell_retain(entry);

    if (graph->data.graph.exit) {
        new_graph->data.graph.exit = graph->data.graph.exit;
        cell_retain(new_graph->data.graph.exit);
    }

    return new_graph;
}

Cell* cell_graph_set_exit(Cell* graph, Cell* exit) {
    assert(graph->type == CELL_GRAPH);

    /* Create new graph with exit set */
    Cell* new_graph = cell_graph(
        graph->data.graph.graph_type,
        graph->data.graph.nodes,
        graph->data.graph.edges,
        graph->data.graph.metadata
    );

    if (new_graph->data.graph.exit) {
        cell_release(new_graph->data.graph.exit);
    }
    new_graph->data.graph.exit = exit;
    if (exit) cell_retain(exit);

    if (graph->data.graph.entry) {
        new_graph->data.graph.entry = graph->data.graph.entry;
        cell_retain(new_graph->data.graph.entry);
    }

    return new_graph;
}

/* Equality */
bool cell_equal(Cell* a, Cell* b) {
    if (a == b) return true;
    if (a == NULL || b == NULL) return false;
    if (a->type != b->type) return false;

    switch (a->type) {
        case CELL_ATOM_NUMBER:
            return a->data.atom.number == b->data.atom.number;
        case CELL_ATOM_BOOL:
            return a->data.atom.boolean == b->data.atom.boolean;
        case CELL_ATOM_SYMBOL:
            return strcmp(a->data.atom.symbol, b->data.atom.symbol) == 0;
        case CELL_ATOM_STRING:
            return strcmp(a->data.atom.string, b->data.atom.string) == 0;
        case CELL_ATOM_NIL:
            return true;
        case CELL_PAIR:
            return cell_equal(a->data.pair.car, b->data.pair.car) &&
                   cell_equal(a->data.pair.cdr, b->data.pair.cdr);
        case CELL_STRUCT:
            /* Structures equal if same type, variant, and fields */
            return cell_equal(a->data.structure.type_tag, b->data.structure.type_tag) &&
                   cell_equal(a->data.structure.variant, b->data.structure.variant) &&
                   cell_equal(a->data.structure.fields, b->data.structure.fields);
        case CELL_GRAPH:
            /* Graphs equal if same type and structure (deep comparison) */
            return a->data.graph.graph_type == b->data.graph.graph_type &&
                   cell_equal(a->data.graph.nodes, b->data.graph.nodes) &&
                   cell_equal(a->data.graph.edges, b->data.graph.edges);
        case CELL_ACTOR:
            return a->data.actor.actor_id == b->data.actor.actor_id;
        case CELL_LAMBDA:
        case CELL_BUILTIN:
        case CELL_ERROR:
        default:
            return false;
    }
}

/* Printing */
void cell_print(Cell* c) {
    if (c == NULL) {
        printf("NULL");
        return;
    }

    switch (c->type) {
        case CELL_ATOM_NUMBER:
            printf("#%g", c->data.atom.number);
            break;
        case CELL_ATOM_BOOL:
            printf(c->data.atom.boolean ? "#t" : "#f");
            break;
        case CELL_ATOM_SYMBOL:
            printf(":%s", c->data.atom.symbol);
            break;
        case CELL_ATOM_STRING:
            printf("\"%s\"", c->data.atom.string);
            break;
        case CELL_ATOM_NIL:
            printf("∅");
            break;
        case CELL_PAIR:
            printf("⟨");
            cell_print(c->data.pair.car);
            printf(" ");
            cell_print(c->data.pair.cdr);
            printf("⟩");
            break;
        case CELL_LAMBDA:
            printf("λ[%d]", c->data.lambda.arity);
            break;
        case CELL_BUILTIN:
            printf("<builtin>");
            break;
        case CELL_ERROR:
            printf("⚠:%s", c->data.error.message);
            if (c->data.error.data && !cell_is_nil(c->data.error.data)) {
                printf(":");
                cell_print(c->data.error.data);
            }
            break;
        case CELL_STRUCT:
            /* Print structure as (⊙/⊚ Type :variant fields) */
            if (c->data.structure.kind == STRUCT_LEAF) {
                printf("⊙");
            } else if (c->data.structure.kind == STRUCT_NODE) {
                printf("⊚");
            } else {
                printf("⊝");
            }
            printf("[");
            cell_print(c->data.structure.type_tag);
            if (c->data.structure.variant && !cell_is_nil(c->data.structure.variant)) {
                printf(" ");
                cell_print(c->data.structure.variant);
            }
            if (c->data.structure.fields && !cell_is_nil(c->data.structure.fields)) {
                printf(" ");
                cell_print(c->data.structure.fields);
            }
            printf("]");
            break;
        case CELL_GRAPH:
            /* Print graph as (⊝ type nodes:N edges:E) */
            printf("⊝[");
            switch (c->data.graph.graph_type) {
                case GRAPH_CFG: printf("CFG"); break;
                case GRAPH_DFG: printf("DFG"); break;
                case GRAPH_CALL: printf("CallGraph"); break;
                case GRAPH_DEP: printf("DepGraph"); break;
                default: printf("Graph"); break;
            }

            /* Count nodes and edges */
            int node_count = 0, edge_count = 0;
            Cell* n = c->data.graph.nodes;
            while (n && !cell_is_nil(n)) {
                node_count++;
                n = cell_cdr(n);
            }
            Cell* e = c->data.graph.edges;
            while (e && !cell_is_nil(e)) {
                edge_count++;
                e = cell_cdr(e);
            }

            printf(" N:%d E:%d", node_count, edge_count);
            printf("]");
            break;
        case CELL_ACTOR:
            printf("⟳[%d]", c->data.actor.actor_id);
            break;
    }
}

void cell_println(Cell* c) {
    cell_print(c);
    printf("\n");
}
