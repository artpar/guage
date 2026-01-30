#include "cell.h"
#include "siphash.h"
#include "swisstable.h"
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
    c->weak_refcount = 0;
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

Cell* cell_channel(int channel_id) {
    Cell* c = cell_alloc(CELL_CHANNEL);
    c->data.channel.channel_id = channel_id;
    return c;
}

Cell* cell_box(Cell* value) {
    Cell* c = cell_alloc(CELL_BOX);
    c->data.box.value = value;
    if (value) cell_retain(value);
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
            case CELL_CHANNEL:
                /* Channel ID is just an int, no children to release */
                break;
            case CELL_BOX:
                cell_release(c->data.box.value);
                break;
            case CELL_WEAK_REF:
                cell_weak_release(c->data.weak_ref.target);
                break;
            case CELL_HASHMAP: {
                uint32_t cap = c->data.hashmap.capacity;
                uint8_t* ctrl = c->data.hashmap.ctrl;
                HashSlot* slots = c->data.hashmap.slots;
                for (uint32_t i = 0; i < cap; i++) {
                    if ((ctrl[i] & 0x80) == 0) {  /* FULL slot */
                        cell_release(slots[i].key);
                        cell_release(slots[i].value);
                    }
                }
                free(ctrl);
                free(slots);
                break;
            }
            default:
                break;
        }

        /* Zombie: if weak refs still point here, keep shell alive */
        if (c->weak_refcount > 0) return;

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

bool cell_is_channel(Cell* c) {
    return c && c->type == CELL_CHANNEL;
}

int cell_get_channel_id(Cell* c) {
    assert(c->type == CELL_CHANNEL);
    return c->data.channel.channel_id;
}

bool cell_is_box(Cell* c) {
    return c && c->type == CELL_BOX;
}

Cell* cell_box_get(Cell* c) {
    assert(c->type == CELL_BOX);
    return c->data.box.value;
}

Cell* cell_box_set(Cell* c, Cell* new_value) {
    assert(c->type == CELL_BOX);
    /* Retain new first (handles self-assignment) */
    if (new_value) cell_retain(new_value);
    Cell* old = c->data.box.value;
    c->data.box.value = new_value;
    /* Return old WITHOUT releasing — caller inherits the box's old reference */
    return old;
}

/* Weak reference operations */
Cell* cell_weak_ref(Cell* target) {
    Cell* c = cell_alloc(CELL_WEAK_REF);
    c->data.weak_ref.target = target;
    cell_weak_retain(target);
    return c;
}

bool cell_is_weak_ref(Cell* c) {
    return c && c->type == CELL_WEAK_REF;
}

Cell* cell_get_weak_target(Cell* c) {
    assert(c->type == CELL_WEAK_REF);
    return c->data.weak_ref.target;
}

void cell_weak_retain(Cell* c) {
    if (c) c->weak_refcount++;
}

void cell_weak_release(Cell* c) {
    if (!c) return;
    assert(c->weak_refcount > 0);
    c->weak_refcount--;
    if (c->weak_refcount == 0 && c->refcount == 0) {
        free(c);
    }
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
        case CELL_CHANNEL:
            return a->data.channel.channel_id == b->data.channel.channel_id;
        case CELL_BOX:
        case CELL_WEAK_REF:
        case CELL_HASHMAP:
            /* Identity only — handled by a == b at top */
            return false;
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
        case CELL_CHANNEL:
            printf("⟿[%d]", c->data.channel.channel_id);
            break;
        case CELL_BOX:
            printf("□[");
            cell_print(c->data.box.value);
            printf("]");
            break;
        case CELL_WEAK_REF:
            if (c->data.weak_ref.target && c->data.weak_ref.target->refcount > 0) {
                printf("◇[alive]");
            } else {
                printf("◇[dead]");
            }
            break;
        case CELL_HASHMAP:
            printf("⊞[%u]", c->data.hashmap.size);
            break;
    }
}

void cell_println(Cell* c) {
    cell_print(c);
    printf("\n");
}

/* ===== HashMap (Swiss Table) Implementation ===== */

/* Hash a cell value using SipHash-2-4 */
uint64_t cell_hash(Cell* c) {
    if (!c) return 0;
    switch (c->type) {
        case CELL_ATOM_NUMBER: {
            double n = c->data.atom.number;
            return guage_siphash(&n, sizeof(n));
        }
        case CELL_ATOM_SYMBOL:
            return guage_siphash(c->data.atom.symbol, strlen(c->data.atom.symbol));
        case CELL_ATOM_STRING:
            return guage_siphash(c->data.atom.string, strlen(c->data.atom.string));
        case CELL_ATOM_BOOL:
            return c->data.atom.boolean ? 0x0001ULL : 0x0002ULL;
        case CELL_ATOM_NIL:
            return 0x0003ULL;
        case CELL_PAIR: {
            uint64_t h = cell_hash(c->data.pair.car);
            h ^= cell_hash(c->data.pair.cdr) * 0x9E3779B97F4A7C15ULL + (h << 12) + (h >> 4);
            return h;
        }
        default: {
            /* Lambda, error, actor, box, etc. — hash pointer */
            uintptr_t ptr = (uintptr_t)c;
            return guage_siphash(&ptr, sizeof(ptr));
        }
    }
}

bool cell_is_hashmap(Cell* c) {
    return c && c->type == CELL_HASHMAP;
}

Cell* cell_hashmap_new(uint32_t initial_capacity) {
    /* Ensure minimum capacity and power of 2 */
    if (initial_capacity < (uint32_t)GROUP_WIDTH)
        initial_capacity = GROUP_WIDTH;
    /* Round up to power of 2 */
    uint32_t cap = 1;
    while (cap < initial_capacity) cap <<= 1;

    Cell* c = cell_alloc(CELL_HASHMAP);
    c->data.hashmap.capacity = cap;
    c->data.hashmap.size = 0;
    c->data.hashmap.growth_left = cap * 7 / 8;

    /* Allocate control bytes: capacity + GROUP_WIDTH for mirroring */
    c->data.hashmap.ctrl = (uint8_t*)malloc(cap + GROUP_WIDTH);
    memset(c->data.hashmap.ctrl, CTRL_EMPTY, cap + GROUP_WIDTH);

    /* Allocate slots */
    c->data.hashmap.slots = (HashSlot*)calloc(cap, sizeof(HashSlot));

    return c;
}

/* Internal: find slot for key. Returns slot index or -1. */
static int hashmap_find(Cell* map, Cell* key, uint64_t hash) {
    uint32_t cap = map->data.hashmap.capacity;
    uint8_t* ctrl = map->data.hashmap.ctrl;
    HashSlot* slots = map->data.hashmap.slots;
    uint8_t h2 = H2(hash);
    uint32_t group_mask = (cap / GROUP_WIDTH) - 1;
    uint32_t group_idx = (uint32_t)(H1(hash) / GROUP_WIDTH) & group_mask;
    uint32_t probe_offset = 0;
    uint32_t probe_step = 0;

    while (1) {
        uint32_t ctrl_offset = ((group_idx + probe_offset) & group_mask) * GROUP_WIDTH;
        const uint8_t* ctrl_ptr = ctrl + ctrl_offset;

        GroupMask match = guage_group_match(ctrl_ptr, h2);
        while (match) {
            int bit = guage_bitmask_next(&match);
            uint32_t slot_idx = ctrl_offset + (uint32_t)bit;
            if (slot_idx < cap && cell_equal(slots[slot_idx].key, key)) {
                return (int)slot_idx;
            }
        }

        GroupMask empty = guage_group_match_empty(ctrl_ptr);
        if (empty) return -1;  /* Key not present */

        probe_step++;
        probe_offset += probe_step;
    }
}

/* Internal: find an empty or deleted slot for insertion */
static uint32_t hashmap_find_insert_slot(Cell* map, uint64_t hash) {
    uint32_t cap = map->data.hashmap.capacity;
    uint8_t* ctrl = map->data.hashmap.ctrl;
    uint32_t group_mask = (cap / GROUP_WIDTH) - 1;
    uint32_t group_idx = (uint32_t)(H1(hash) / GROUP_WIDTH) & group_mask;
    uint32_t probe_offset = 0;
    uint32_t probe_step = 0;

    while (1) {
        uint32_t ctrl_offset = ((group_idx + probe_offset) & group_mask) * GROUP_WIDTH;
        const uint8_t* ctrl_ptr = ctrl + ctrl_offset;

        GroupMask avail = guage_group_match_empty_or_deleted(ctrl_ptr);
        if (avail) {
            int bit = guage_bitmask_next(&avail);
            return ctrl_offset + (uint32_t)bit;
        }

        probe_step++;
        probe_offset += probe_step;
    }
}

/* Internal: resize the hashmap */
static void hashmap_resize(Cell* map, uint32_t new_cap) {
    uint32_t old_cap = map->data.hashmap.capacity;
    uint8_t* old_ctrl = map->data.hashmap.ctrl;
    HashSlot* old_slots = map->data.hashmap.slots;

    /* Allocate new arrays */
    uint8_t* new_ctrl = (uint8_t*)malloc(new_cap + GROUP_WIDTH);
    memset(new_ctrl, CTRL_EMPTY, new_cap + GROUP_WIDTH);
    HashSlot* new_slots = (HashSlot*)calloc(new_cap, sizeof(HashSlot));

    /* Temporarily swap in new arrays */
    map->data.hashmap.ctrl = new_ctrl;
    map->data.hashmap.slots = new_slots;
    map->data.hashmap.capacity = new_cap;
    map->data.hashmap.growth_left = new_cap * 7 / 8 - map->data.hashmap.size;

    /* Reinsert all entries */
    for (uint32_t i = 0; i < old_cap; i++) {
        if ((old_ctrl[i] & 0x80) == 0) {  /* FULL slot */
            uint64_t hash = cell_hash(old_slots[i].key);
            uint32_t slot = hashmap_find_insert_slot(map, hash);
            uint8_t h2 = H2(hash);
            new_ctrl[slot] = h2;
            if (slot < (uint32_t)GROUP_WIDTH) {
                new_ctrl[new_cap + slot] = h2;  /* Mirror */
            }
            new_slots[slot] = old_slots[i];  /* Move pointer, no retain/release */
        }
    }

    free(old_ctrl);
    free(old_slots);
}

Cell* cell_hashmap_get(Cell* map, Cell* key) {
    assert(map->type == CELL_HASHMAP);
    if (map->data.hashmap.size == 0) return cell_nil();

    uint64_t hash = cell_hash(key);
    int idx = hashmap_find(map, key, hash);
    if (idx < 0) return cell_nil();

    Cell* val = map->data.hashmap.slots[idx].value;
    cell_retain(val);
    return val;
}

Cell* cell_hashmap_put(Cell* map, Cell* key, Cell* value) {
    assert(map->type == CELL_HASHMAP);
    uint64_t hash = cell_hash(key);

    /* Check for existing key */
    int idx = hashmap_find(map, key, hash);
    if (idx >= 0) {
        /* Overwrite — return old value (caller owns ref) */
        Cell* old = map->data.hashmap.slots[idx].value;
        map->data.hashmap.slots[idx].value = value;
        cell_retain(value);
        return old;  /* Old value returned without release */
    }

    /* Need to insert — check if resize needed first */
    if (map->data.hashmap.growth_left == 0) {
        hashmap_resize(map, map->data.hashmap.capacity * 2);
    }

    /* Find insertion slot */
    uint32_t slot = hashmap_find_insert_slot(map, hash);
    uint8_t h2 = H2(hash);
    bool was_empty = (map->data.hashmap.ctrl[slot] == CTRL_EMPTY);

    map->data.hashmap.ctrl[slot] = h2;
    if (slot < (uint32_t)GROUP_WIDTH) {
        map->data.hashmap.ctrl[map->data.hashmap.capacity + slot] = h2;
    }

    map->data.hashmap.slots[slot].key = key;
    map->data.hashmap.slots[slot].value = value;
    cell_retain(key);
    cell_retain(value);

    map->data.hashmap.size++;
    if (was_empty) map->data.hashmap.growth_left--;

    return cell_nil();  /* No old value */
}

Cell* cell_hashmap_delete(Cell* map, Cell* key) {
    assert(map->type == CELL_HASHMAP);
    uint64_t hash = cell_hash(key);
    int idx = hashmap_find(map, key, hash);
    if (idx < 0) return cell_nil();

    Cell* old_value = map->data.hashmap.slots[idx].value;
    cell_release(map->data.hashmap.slots[idx].key);
    map->data.hashmap.slots[idx].key = NULL;
    map->data.hashmap.slots[idx].value = NULL;

    /* Determine tombstone strategy */
    uint32_t cap = map->data.hashmap.capacity;
    uint32_t group_mask = (cap / GROUP_WIDTH) - 1;
    uint32_t next_group = (((uint32_t)idx / GROUP_WIDTH + 1) & group_mask) * GROUP_WIDTH;
    GroupMask next_empty = guage_group_match_empty(map->data.hashmap.ctrl + next_group);

    if (next_empty) {
        map->data.hashmap.ctrl[idx] = CTRL_EMPTY;
        map->data.hashmap.growth_left++;
    } else {
        map->data.hashmap.ctrl[idx] = CTRL_DELETED;
    }
    if ((uint32_t)idx < (uint32_t)GROUP_WIDTH) {
        map->data.hashmap.ctrl[cap + (uint32_t)idx] = map->data.hashmap.ctrl[idx];
    }

    map->data.hashmap.size--;
    return old_value;  /* Caller owns ref */
}

bool cell_hashmap_has(Cell* map, Cell* key) {
    assert(map->type == CELL_HASHMAP);
    if (map->data.hashmap.size == 0) return false;
    uint64_t hash = cell_hash(key);
    return hashmap_find(map, key, hash) >= 0;
}

uint32_t cell_hashmap_size(Cell* map) {
    assert(map->type == CELL_HASHMAP);
    return map->data.hashmap.size;
}

Cell* cell_hashmap_keys(Cell* map) {
    assert(map->type == CELL_HASHMAP);
    Cell* result = cell_nil();
    uint32_t cap = map->data.hashmap.capacity;
    uint8_t* ctrl = map->data.hashmap.ctrl;
    HashSlot* slots = map->data.hashmap.slots;

    for (uint32_t i = 0; i < cap; i++) {
        if ((ctrl[i] & 0x80) == 0) {
            Cell* pair = cell_cons(slots[i].key, result);
            cell_release(result);
            result = pair;
        }
    }
    return result;
}

Cell* cell_hashmap_values(Cell* map) {
    assert(map->type == CELL_HASHMAP);
    Cell* result = cell_nil();
    uint32_t cap = map->data.hashmap.capacity;
    uint8_t* ctrl = map->data.hashmap.ctrl;
    HashSlot* slots = map->data.hashmap.slots;

    for (uint32_t i = 0; i < cap; i++) {
        if ((ctrl[i] & 0x80) == 0) {
            Cell* pair = cell_cons(slots[i].value, result);
            cell_release(result);
            result = pair;
        }
    }
    return result;
}

Cell* cell_hashmap_entries(Cell* map) {
    assert(map->type == CELL_HASHMAP);
    Cell* result = cell_nil();
    uint32_t cap = map->data.hashmap.capacity;
    uint8_t* ctrl = map->data.hashmap.ctrl;
    HashSlot* slots = map->data.hashmap.slots;

    for (uint32_t i = 0; i < cap; i++) {
        if ((ctrl[i] & 0x80) == 0) {
            Cell* entry = cell_cons(slots[i].key, slots[i].value);
            Cell* node = cell_cons(entry, result);
            cell_release(entry);
            cell_release(result);
            result = node;
        }
    }
    return result;
}

Cell* cell_hashmap_merge(Cell* m1, Cell* m2) {
    assert(m1->type == CELL_HASHMAP);
    assert(m2->type == CELL_HASHMAP);

    /* Estimate capacity */
    uint32_t est = m1->data.hashmap.size + m2->data.hashmap.size;
    Cell* result = cell_hashmap_new(est < (uint32_t)GROUP_WIDTH ? GROUP_WIDTH : est * 2);

    /* Insert all from m1 */
    for (uint32_t i = 0; i < m1->data.hashmap.capacity; i++) {
        if ((m1->data.hashmap.ctrl[i] & 0x80) == 0) {
            Cell* old = cell_hashmap_put(result, m1->data.hashmap.slots[i].key, m1->data.hashmap.slots[i].value);
            cell_release(old);
        }
    }
    /* Insert all from m2 (overwrites m1 on conflict) */
    for (uint32_t i = 0; i < m2->data.hashmap.capacity; i++) {
        if ((m2->data.hashmap.ctrl[i] & 0x80) == 0) {
            Cell* old = cell_hashmap_put(result, m2->data.hashmap.slots[i].key, m2->data.hashmap.slots[i].value);
            cell_release(old);
        }
    }
    return result;
}
