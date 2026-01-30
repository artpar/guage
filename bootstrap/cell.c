#include "cell.h"
#include "intern.h"
#include "siphash.h"
#include "swisstable.h"
#include "btree_simd.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* === Sorted Map B-tree types (forward declarations for release/print/hash) === */

#define SM_NIL UINT32_MAX
#define SM_POOL_INIT 64

typedef struct {
    uint64_t sort_keys[BTREE_B];
    Cell*    keys[BTREE_B];
    Cell*    values[BTREE_B];
    uint32_t children[BTREE_B + 1];
    uint32_t next_leaf;
    uint32_t prev_leaf;
    uint8_t  n_keys;
    uint8_t  is_leaf;
} SMNode;

typedef struct {
    SMNode*  nodes;
    uint32_t capacity;
    uint32_t used;
    uint32_t* free_list;
    uint32_t free_count;
    uint32_t free_cap;
} SMPool;

/* Forward declaration */
static void sm_pool_destroy_impl(SMPool* p);

/* Cell allocation */
static Cell* cell_alloc(CellType type) {
    Cell* c = (Cell*)malloc(sizeof(Cell));
    assert(c != NULL);
    memset(c, 0, sizeof(Cell));

    c->type = type;
    c->refcount = 1;
    c->weak_refcount = 0;
    c->sym_id = 0;
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
    InternResult r = intern(sym);
    c->data.atom.symbol = r.canonical;
    c->sym_id = r.id;
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

uint16_t cell_get_symbol_id(Cell* c) {
    assert(c->type == CELL_ATOM_SYMBOL);
    return c->sym_id;
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
                /* Interned strings are immortal — no free */
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
            case CELL_SET: {
                uint32_t sng = c->data.hashset.n_groups;
                for (uint32_t sg = 0; sg < sng; sg++) {
                    uint8_t* smeta = c->data.hashset.metadata + sg * 16;
                    for (int ss = 0; ss < 15; ss++) {
                        if (smeta[ss] >= 2) {
                            cell_release(c->data.hashset.elements[sg * 15 + ss]);
                        }
                    }
                }
                free(c->data.hashset.metadata);
                free(c->data.hashset.elements);
                break;
            }
            case CELL_DEQUE: {
                uint32_t dsz = c->data.deque.tail - c->data.deque.head;
                uint32_t dcap = c->data.deque.capacity;
                for (uint32_t di = 0; di < dsz; di++) {
                    uint32_t didx = (c->data.deque.head + di) & (dcap - 1);
                    if (c->data.deque.buffer[didx])
                        cell_release(c->data.deque.buffer[didx]);
                }
                free(c->data.deque.buffer);
                break;
            }
            case CELL_BUFFER:
                free(c->data.buffer.bytes);
                break;
            case CELL_VECTOR: {
                Cell** vbuf = (c->data.vector.capacity <= 4)
                    ? c->data.vector.sbo : c->data.vector.heap;
                for (uint32_t vi = 0; vi < c->data.vector.size; vi++) {
                    if (vbuf[vi]) cell_release(vbuf[vi]);
                }
                if (c->data.vector.capacity > 4) {
                    free(c->data.vector.heap);
                }
                break;
            }
            case CELL_HEAP: {
                for (uint32_t hi = 0; hi < c->data.pq.size; hi++) {
                    if (c->data.pq.vals[hi]) cell_release(c->data.pq.vals[hi]);
                }
                free(c->data.pq.keys);
                free(c->data.pq.vals);
                break;
            }
            case CELL_SORTED_MAP: {
                SMPool* pool = (SMPool*)c->data.sorted_map.node_pool;
                if (pool) {
                    /* Release all keys and values in leaf nodes */
                    uint32_t leaf = c->data.sorted_map.first_leaf;
                    while (leaf != SM_NIL) {
                        SMNode* node = &pool->nodes[leaf];
                        for (uint8_t i = 0; i < node->n_keys; i++) {
                            if (node->keys[i]) cell_release(node->keys[i]);
                            if (node->values[i]) cell_release(node->values[i]);
                        }
                        leaf = node->next_leaf;
                    }
                    sm_pool_destroy_impl(pool);
                }
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
    cell_retain(c->data.graph.nodes);
    return c->data.graph.nodes;
}

Cell* cell_graph_edges(Cell* c) {
    assert(c->type == CELL_GRAPH);
    cell_retain(c->data.graph.edges);
    return c->data.graph.edges;
}

Cell* cell_graph_metadata(Cell* c) {
    assert(c->type == CELL_GRAPH);
    cell_retain(c->data.graph.metadata);
    return c->data.graph.metadata;
}

Cell* cell_graph_entry(Cell* c) {
    assert(c->type == CELL_GRAPH);
    if (c->data.graph.entry) cell_retain(c->data.graph.entry);
    return c->data.graph.entry;
}

Cell* cell_graph_exit(Cell* c) {
    assert(c->type == CELL_GRAPH);
    if (c->data.graph.exit) cell_retain(c->data.graph.exit);
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
            return a->data.atom.symbol == b->data.atom.symbol;
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
        case CELL_SET:
            /* Set equality: same size and every element of a is in b */
            if (a->data.hashset.size != b->data.hashset.size) return false;
            return cell_hashset_subset(a, b);
        case CELL_DEQUE: {
            uint32_t asz = a->data.deque.tail - a->data.deque.head;
            uint32_t bsz = b->data.deque.tail - b->data.deque.head;
            if (asz != bsz) return false;
            for (uint32_t i = 0; i < asz; i++) {
                Cell* ae = a->data.deque.buffer[(a->data.deque.head + i) & (a->data.deque.capacity - 1)];
                Cell* be = b->data.deque.buffer[(b->data.deque.head + i) & (b->data.deque.capacity - 1)];
                if (!cell_equal(ae, be)) return false;
            }
            return true;
        }
        case CELL_BUFFER:
            if (a->data.buffer.size != b->data.buffer.size) return false;
            return memcmp(a->data.buffer.bytes, b->data.buffer.bytes, a->data.buffer.size) == 0;
        case CELL_VECTOR: {
            if (a->data.vector.size != b->data.vector.size) return false;
            uint32_t vsz = a->data.vector.size;
            Cell** va = (a->data.vector.capacity <= 4) ? a->data.vector.sbo : a->data.vector.heap;
            Cell** vb = (b->data.vector.capacity <= 4) ? b->data.vector.sbo : b->data.vector.heap;
            for (uint32_t vi = 0; vi < vsz; vi++) {
                if (!cell_equal(va[vi], vb[vi])) return false;
            }
            return true;
        }
        case CELL_HEAP:
            /* Identity only — heap ordering makes structural comparison meaningless */
            return false;
        case CELL_SORTED_MAP:
            /* Identity only — structural comparison too expensive */
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
        case CELL_SET:
            printf("⊍[%u]", c->data.hashset.size);
            break;
        case CELL_DEQUE:
            printf("⊟[%u]", c->data.deque.tail - c->data.deque.head);
            break;
        case CELL_BUFFER:
            printf("◈[%u]", c->data.buffer.size);
            break;
        case CELL_VECTOR:
            printf("⟦%u⟧", c->data.vector.size);
            break;
        case CELL_HEAP:
            printf("△[%u]", c->data.pq.size);
            break;
        case CELL_SORTED_MAP:
            printf("⋔[%u]", c->data.sorted_map.size);
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
            return intern_hash_by_id(c->sym_id);
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
        case CELL_BUFFER:
            return guage_siphash(c->data.buffer.bytes, c->data.buffer.size);
        case CELL_VECTOR: {
            /* Hash all elements */
            uint64_t vh = 0x5678ULL;
            Cell** vbuf = (c->data.vector.capacity <= 4) ? c->data.vector.sbo : c->data.vector.heap;
            for (uint32_t vi = 0; vi < c->data.vector.size; vi++) {
                vh ^= cell_hash(vbuf[vi]) * 0x9E3779B97F4A7C15ULL + (vh << 12) + (vh >> 4);
            }
            return vh;
        }
        case CELL_HEAP: {
            uint64_t hh = 0x9ABCull;
            for (uint32_t hi = 0; hi < c->data.pq.size; hi++) {
                hh ^= guage_siphash(&c->data.pq.keys[hi], sizeof(double));
                hh ^= cell_hash(c->data.pq.vals[hi]) * 0x9E3779B97F4A7C15ULL + (hh << 12) + (hh >> 4);
            }
            return hh;
        }
        case CELL_SORTED_MAP: {
            uint64_t smh = 0xDEF0ull;
            uint32_t sz = c->data.sorted_map.size;
            smh ^= guage_siphash(&sz, sizeof(sz));
            return smh;
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

/* =========================================================================
 * HashSet — Boost-style groups-of-15 + overflow Bloom byte
 *
 * Each group: 16-byte metadata word (15 tag bytes + 1 overflow Bloom byte)
 *             15 element slots (Cell*)
 *
 * Tag encoding: 0x00=EMPTY, 0x01=SENTINEL, 0x02..0xFF=occupied (reduced hash)
 * Overflow byte: bit (hash % 8) set when element displaced past home group
 *   → bit=0: element definitely not here (early termination)
 *   → bit=1: may have overflowed, continue probing
 *
 * This gives 3.2x faster miss lookups than Swiss Table under high load.
 * ========================================================================= */

#define HS_GROUP_SLOTS  15
#define HS_META_SIZE    16   /* 15 tags + 1 overflow byte */
#define HS_TAG_EMPTY    ((uint8_t)0x00)
#define HS_TAG_SENTINEL ((uint8_t)0x01)
#define HS_OFW_OFFSET   15   /* Overflow byte is at index 15 in metadata word */
#define HS_MATCH_MASK   0x7FFF  /* Mask out bit 15 (overflow byte) from SIMD match */

/* Compute tag from hash: map to [2, 255] to avoid EMPTY(0) and SENTINEL(1) */
static inline uint8_t hs_tag(uint64_t hash) {
    uint8_t t = (uint8_t)(hash >> 56);
    return t < 2 ? (t + 2) : t;
}

/* Group index from hash (lower bits) */
static inline uint32_t hs_group_index(uint64_t hash, uint32_t group_mask) {
    return (uint32_t)(hash) & group_mask;
}

/* Overflow Bloom bit for a given hash */
static inline uint8_t hs_overflow_bit(uint64_t hash) {
    return (uint8_t)(1u << ((hash >> 48) & 7));
}

/* Get metadata pointer for group g */
static inline uint8_t* hs_meta(Cell* set, uint32_t g) {
    return set->data.hashset.metadata + (g * HS_META_SIZE);
}

/* Get element pointer for group g, slot s */
static inline Cell** hs_elem(Cell* set, uint32_t g, int s) {
    return &set->data.hashset.elements[g * HS_GROUP_SLOTS + s];
}

bool cell_is_hashset(Cell* c) {
    return c && c->type == CELL_SET;
}

Cell* cell_hashset_new(uint32_t initial_n_groups) {
    if (initial_n_groups < 1) initial_n_groups = 1;
    /* Round up to power of 2 */
    uint32_t ng = 1;
    while (ng < initial_n_groups) ng <<= 1;

    Cell* c = (Cell*)calloc(1, sizeof(Cell));
    c->type = CELL_SET;
    c->refcount = 1;

    /* Allocate 16-byte aligned metadata: ng * 16 bytes */
    c->data.hashset.metadata = (uint8_t*)aligned_alloc(16, ng * HS_META_SIZE);
    memset(c->data.hashset.metadata, HS_TAG_EMPTY, ng * HS_META_SIZE);

    /* Allocate element slots: ng * 15 Cell pointers */
    c->data.hashset.elements = (Cell**)calloc(ng * HS_GROUP_SLOTS, sizeof(Cell*));

    c->data.hashset.size = 0;
    c->data.hashset.n_groups = ng;
    c->data.hashset.ml_left = ng * 13;  /* 86.7% load factor (13/15) */

    return c;
}

/* Internal: find element in set. Returns group*15+slot if found, -1 otherwise. */
static int hashset_find(Cell* set, Cell* val, uint64_t hash) {
    uint32_t ng = set->data.hashset.n_groups;
    uint32_t group_mask = ng - 1;
    uint8_t tag = hs_tag(hash);
    uint32_t g = hs_group_index(hash, group_mask);
    uint8_t ofw_bit = hs_overflow_bit(hash);

    uint32_t probe_offset = 0;
    uint32_t probe_step = 0;

    while (1) {
        uint32_t gi = (g + probe_offset) & group_mask;
        uint8_t* meta = hs_meta(set, gi);

        /* SIMD match: find all slots with matching tag (mask out overflow byte) */
        GroupMask match = guage_group_match(meta, tag) & HS_MATCH_MASK;
        while (match) {
            int bit = guage_bitmask_next(&match);
            if (bit < HS_GROUP_SLOTS && cell_equal(*hs_elem(set, gi, bit), val)) {
                return (int)(gi * HS_GROUP_SLOTS + bit);
            }
        }

        /* Check overflow byte: if our bit is NOT set, element is definitely not here */
        if (!(meta[HS_OFW_OFFSET] & ofw_bit)) {
            return -1;
        }

        probe_step++;
        probe_offset += probe_step;
    }
}

/* Internal: find empty slot for insertion. Also sets overflow bits on bypassed groups. */
static int hashset_find_insert_slot(Cell* set, uint64_t hash) {
    uint32_t ng = set->data.hashset.n_groups;
    uint32_t group_mask = ng - 1;
    uint32_t g = hs_group_index(hash, group_mask);
    uint8_t ofw_bit = hs_overflow_bit(hash);

    uint32_t probe_offset = 0;
    uint32_t probe_step = 0;

    while (1) {
        uint32_t gi = (g + probe_offset) & group_mask;
        uint8_t* meta = hs_meta(set, gi);

        /* SIMD find empty slot (tag == 0x00) */
        GroupMask empty = guage_group_match(meta, HS_TAG_EMPTY) & HS_MATCH_MASK;
        if (empty) {
            int bit = guage_bitmask_next(&empty);
            return (int)(gi * HS_GROUP_SLOTS + bit);
        }

        /* Group is full — set overflow bit (this group was bypassed) */
        meta[HS_OFW_OFFSET] |= ofw_bit;

        probe_step++;
        probe_offset += probe_step;
    }
}

/* Internal: resize (double n_groups) */
static void hashset_resize(Cell* set, uint32_t new_ng) {
    uint32_t old_ng = set->data.hashset.n_groups;
    uint8_t* old_meta = set->data.hashset.metadata;
    Cell** old_elems = set->data.hashset.elements;

    /* Allocate new arrays */
    uint8_t* new_meta = (uint8_t*)aligned_alloc(16, new_ng * HS_META_SIZE);
    memset(new_meta, HS_TAG_EMPTY, new_ng * HS_META_SIZE);
    Cell** new_elems = (Cell**)calloc(new_ng * HS_GROUP_SLOTS, sizeof(Cell*));

    /* Swap in new arrays */
    set->data.hashset.metadata = new_meta;
    set->data.hashset.elements = new_elems;
    set->data.hashset.n_groups = new_ng;
    set->data.hashset.ml_left = new_ng * 13 - set->data.hashset.size;

    /* Reinsert all elements */
    for (uint32_t g = 0; g < old_ng; g++) {
        uint8_t* meta = old_meta + g * HS_META_SIZE;
        for (int s = 0; s < HS_GROUP_SLOTS; s++) {
            if (meta[s] >= 2) {  /* Occupied slot */
                Cell* elem = old_elems[g * HS_GROUP_SLOTS + s];
                uint64_t hash = cell_hash(elem);
                int slot = hashset_find_insert_slot(set, hash);
                uint32_t tg = (uint32_t)slot / HS_GROUP_SLOTS;
                int ts = slot % HS_GROUP_SLOTS;
                hs_meta(set, tg)[ts] = hs_tag(hash);
                *hs_elem(set, tg, ts) = elem;  /* Move pointer, no retain/release */
            }
        }
    }

    free(old_meta);
    free(old_elems);
}

bool cell_hashset_add(Cell* set, Cell* val) {
    assert(set->type == CELL_SET);
    uint64_t hash = cell_hash(val);

    /* Check if already present */
    if (hashset_find(set, val, hash) >= 0) {
        return false;  /* Already exists */
    }

    /* Resize if needed */
    if (set->data.hashset.ml_left == 0) {
        hashset_resize(set, set->data.hashset.n_groups * 2);
    }

    /* Insert */
    int slot = hashset_find_insert_slot(set, hash);
    uint32_t g = (uint32_t)slot / HS_GROUP_SLOTS;
    int s = slot % HS_GROUP_SLOTS;
    hs_meta(set, g)[s] = hs_tag(hash);
    cell_retain(val);
    *hs_elem(set, g, s) = val;
    set->data.hashset.size++;
    set->data.hashset.ml_left--;

    return true;  /* New element added */
}

bool cell_hashset_remove(Cell* set, Cell* val) {
    assert(set->type == CELL_SET);
    uint64_t hash = cell_hash(val);
    int idx = hashset_find(set, val, hash);
    if (idx < 0) return false;  /* Not present */

    uint32_t g = (uint32_t)idx / HS_GROUP_SLOTS;
    int s = idx % HS_GROUP_SLOTS;

    /* Clear tag to EMPTY — tombstone-free! Overflow bits remain (stale is OK). */
    hs_meta(set, g)[s] = HS_TAG_EMPTY;
    cell_release(*hs_elem(set, g, s));
    *hs_elem(set, g, s) = NULL;
    set->data.hashset.size--;
    /* Don't increment ml_left — stale overflow bits eat capacity; rehash reclaims it */

    return true;
}

bool cell_hashset_has(Cell* set, Cell* val) {
    assert(set->type == CELL_SET);
    if (set->data.hashset.size == 0) return false;
    return hashset_find(set, val, cell_hash(val)) >= 0;
}

uint32_t cell_hashset_size(Cell* set) {
    assert(set->type == CELL_SET);
    return set->data.hashset.size;
}

Cell* cell_hashset_elements(Cell* set) {
    assert(set->type == CELL_SET);
    Cell* result = cell_nil();
    uint32_t ng = set->data.hashset.n_groups;
    for (uint32_t g = 0; g < ng; g++) {
        uint8_t* meta = hs_meta(set, g);
        for (int s = HS_GROUP_SLOTS - 1; s >= 0; s--) {
            if (meta[s] >= 2) {
                Cell* elem = *hs_elem(set, g, s);
                cell_retain(elem);
                Cell* pair = cell_cons(elem, result);
                cell_release(result);
                result = pair;
            }
        }
    }
    return result;
}

Cell* cell_hashset_union(Cell* s1, Cell* s2) {
    assert(s1->type == CELL_SET && s2->type == CELL_SET);
    uint32_t est = s1->data.hashset.size + s2->data.hashset.size;
    uint32_t ng = 1;
    while (ng * 13 < est) ng <<= 1;
    Cell* result = cell_hashset_new(ng);

    /* Add all from s1 */
    for (uint32_t g = 0; g < s1->data.hashset.n_groups; g++) {
        uint8_t* meta = hs_meta(s1, g);
        for (int s = 0; s < HS_GROUP_SLOTS; s++) {
            if (meta[s] >= 2) {
                cell_hashset_add(result, *hs_elem(s1, g, s));
            }
        }
    }
    /* Add all from s2 (duplicates ignored by add) */
    for (uint32_t g = 0; g < s2->data.hashset.n_groups; g++) {
        uint8_t* meta = hs_meta(s2, g);
        for (int s = 0; s < HS_GROUP_SLOTS; s++) {
            if (meta[s] >= 2) {
                cell_hashset_add(result, *hs_elem(s2, g, s));
            }
        }
    }
    return result;
}

Cell* cell_hashset_intersection(Cell* s1, Cell* s2) {
    assert(s1->type == CELL_SET && s2->type == CELL_SET);
    Cell* result = cell_hashset_new(1);

    /* Add elements from s1 that also exist in s2 */
    for (uint32_t g = 0; g < s1->data.hashset.n_groups; g++) {
        uint8_t* meta = hs_meta(s1, g);
        for (int s = 0; s < HS_GROUP_SLOTS; s++) {
            if (meta[s] >= 2) {
                Cell* elem = *hs_elem(s1, g, s);
                if (cell_hashset_has(s2, elem)) {
                    cell_hashset_add(result, elem);
                }
            }
        }
    }
    return result;
}

Cell* cell_hashset_difference(Cell* s1, Cell* s2) {
    assert(s1->type == CELL_SET && s2->type == CELL_SET);
    Cell* result = cell_hashset_new(1);

    /* Add elements from s1 that do NOT exist in s2 */
    for (uint32_t g = 0; g < s1->data.hashset.n_groups; g++) {
        uint8_t* meta = hs_meta(s1, g);
        for (int s = 0; s < HS_GROUP_SLOTS; s++) {
            if (meta[s] >= 2) {
                Cell* elem = *hs_elem(s1, g, s);
                if (!cell_hashset_has(s2, elem)) {
                    cell_hashset_add(result, elem);
                }
            }
        }
    }
    return result;
}

bool cell_hashset_subset(Cell* s1, Cell* s2) {
    assert(s1->type == CELL_SET && s2->type == CELL_SET);
    if (s1->data.hashset.size > s2->data.hashset.size) return false;

    for (uint32_t g = 0; g < s1->data.hashset.n_groups; g++) {
        uint8_t* meta = hs_meta(s1, g);
        for (int s = 0; s < HS_GROUP_SLOTS; s++) {
            if (meta[s] >= 2) {
                if (!cell_hashset_has(s2, *hs_elem(s1, g, s))) {
                    return false;
                }
            }
        }
    }
    return true;
}

/* =========================================================================
 * Deque — DPDK-grade cache-optimized circular buffer
 *
 * Power-of-2 capacity with bitmask indexing (branchless wraparound).
 * Virtual indices: head/tail are monotonically increasing uint32_t.
 * Size = tail - head (works via unsigned overflow).
 * Cache-line aligned buffer (64 bytes).
 * Software prefetch hints on push/pop.
 * ========================================================================= */

#define DEQUE_MIN_CAP 8
#define DEQUE_IDX(virt, cap) ((virt) & ((cap) - 1))

bool cell_is_deque(Cell* c) {
    return c && c->type == CELL_DEQUE;
}

Cell* cell_deque_new(uint32_t initial_cap) {
    if (initial_cap < DEQUE_MIN_CAP) initial_cap = DEQUE_MIN_CAP;
    /* Round up to power of 2 */
    uint32_t cap = DEQUE_MIN_CAP;
    while (cap < initial_cap) cap <<= 1;

    Cell* c = cell_alloc(CELL_DEQUE);
    c->data.deque.capacity = cap;
    c->data.deque.head = 0;
    c->data.deque.tail = 0;
    c->data.deque.buffer = (Cell**)aligned_alloc(64, cap * sizeof(Cell*));
    memset(c->data.deque.buffer, 0, cap * sizeof(Cell*));
    return c;
}

/* Internal: grow deque to 2x capacity, unwrapping ring into linear order */
static void deque_grow(Cell* d) {
    uint32_t old_cap = d->data.deque.capacity;
    uint32_t new_cap = old_cap * 2;
    uint32_t size = d->data.deque.tail - d->data.deque.head;

    Cell** new_buf = (Cell**)aligned_alloc(64, new_cap * sizeof(Cell*));
    memset(new_buf, 0, new_cap * sizeof(Cell*));

    /* Unwrap ring into linear at new_buf[0..size-1] */
    uint32_t head_idx = DEQUE_IDX(d->data.deque.head, old_cap);
    uint32_t tail_idx = DEQUE_IDX(d->data.deque.tail, old_cap);

    if (size == 0) {
        /* Empty — nothing to copy */
    } else if (head_idx < tail_idx) {
        /* Contiguous: single memcpy */
        memcpy(new_buf, d->data.deque.buffer + head_idx, size * sizeof(Cell*));
    } else {
        /* Wrapped: two memcpys */
        uint32_t first_chunk = old_cap - head_idx;
        memcpy(new_buf, d->data.deque.buffer + head_idx, first_chunk * sizeof(Cell*));
        memcpy(new_buf + first_chunk, d->data.deque.buffer, tail_idx * sizeof(Cell*));
    }

    free(d->data.deque.buffer);
    d->data.deque.buffer = new_buf;
    d->data.deque.capacity = new_cap;
    d->data.deque.head = 0;
    d->data.deque.tail = size;
}

void cell_deque_push_front(Cell* d, Cell* val) {
    assert(d->type == CELL_DEQUE);
    uint32_t size = d->data.deque.tail - d->data.deque.head;
    if (__builtin_expect(size == d->data.deque.capacity, 0)) {
        deque_grow(d);
    }
    d->data.deque.head--;
    uint32_t idx = DEQUE_IDX(d->data.deque.head, d->data.deque.capacity);
    __builtin_prefetch(&d->data.deque.buffer[idx], 1, 3);
    d->data.deque.buffer[idx] = val;
    cell_retain(val);
}

void cell_deque_push_back(Cell* d, Cell* val) {
    assert(d->type == CELL_DEQUE);
    uint32_t size = d->data.deque.tail - d->data.deque.head;
    if (__builtin_expect(size == d->data.deque.capacity, 0)) {
        deque_grow(d);
    }
    uint32_t idx = DEQUE_IDX(d->data.deque.tail, d->data.deque.capacity);
    __builtin_prefetch(&d->data.deque.buffer[idx], 1, 3);
    d->data.deque.buffer[idx] = val;
    cell_retain(val);
    d->data.deque.tail++;
}

Cell* cell_deque_pop_front(Cell* d) {
    assert(d->type == CELL_DEQUE);
    uint32_t size = d->data.deque.tail - d->data.deque.head;
    if (size == 0) return NULL;
    uint32_t idx = DEQUE_IDX(d->data.deque.head, d->data.deque.capacity);
    __builtin_prefetch(&d->data.deque.buffer[idx], 0, 3);
    Cell* val = d->data.deque.buffer[idx];
    d->data.deque.buffer[idx] = NULL;
    d->data.deque.head++;
    /* Caller inherits the buffer's reference — no release here */
    return val;
}

Cell* cell_deque_pop_back(Cell* d) {
    assert(d->type == CELL_DEQUE);
    uint32_t size = d->data.deque.tail - d->data.deque.head;
    if (size == 0) return NULL;
    d->data.deque.tail--;
    uint32_t idx = DEQUE_IDX(d->data.deque.tail, d->data.deque.capacity);
    __builtin_prefetch(&d->data.deque.buffer[idx], 0, 3);
    Cell* val = d->data.deque.buffer[idx];
    d->data.deque.buffer[idx] = NULL;
    /* Caller inherits the buffer's reference — no release here */
    return val;
}

Cell* cell_deque_peek_front(Cell* d) {
    assert(d->type == CELL_DEQUE);
    uint32_t size = d->data.deque.tail - d->data.deque.head;
    if (size == 0) return NULL;
    uint32_t idx = DEQUE_IDX(d->data.deque.head, d->data.deque.capacity);
    return d->data.deque.buffer[idx];
}

Cell* cell_deque_peek_back(Cell* d) {
    assert(d->type == CELL_DEQUE);
    uint32_t size = d->data.deque.tail - d->data.deque.head;
    if (size == 0) return NULL;
    uint32_t idx = DEQUE_IDX(d->data.deque.tail - 1, d->data.deque.capacity);
    return d->data.deque.buffer[idx];
}

uint32_t cell_deque_size(Cell* d) {
    assert(d->type == CELL_DEQUE);
    return d->data.deque.tail - d->data.deque.head;
}

Cell* cell_deque_to_list(Cell* d) {
    assert(d->type == CELL_DEQUE);
    uint32_t size = d->data.deque.tail - d->data.deque.head;
    uint32_t cap = d->data.deque.capacity;
    /* Build list back-to-front so cons produces front-to-back order */
    Cell* result = cell_nil();
    for (uint32_t i = size; i > 0; i--) {
        uint32_t idx = DEQUE_IDX(d->data.deque.head + i - 1, cap);
        Cell* elem = d->data.deque.buffer[idx];
        Cell* pair = cell_cons(elem, result);
        cell_release(result);
        result = pair;
    }
    return result;
}

/* =========================================================================
 * Buffer — Cache-line aligned raw byte buffer
 * ========================================================================= */

#define BUFFER_MIN_CAP 64  /* One cache line */

bool cell_is_buffer(Cell* c) {
    return c && c->type == CELL_BUFFER;
}

Cell* cell_buffer_new(uint32_t initial_cap) {
    if (initial_cap < BUFFER_MIN_CAP) initial_cap = BUFFER_MIN_CAP;
    /* Round up to power of 2 */
    uint32_t cap = BUFFER_MIN_CAP;
    while (cap < initial_cap) cap <<= 1;

    Cell* c = cell_alloc(CELL_BUFFER);
    c->data.buffer.capacity = cap;
    c->data.buffer.size = 0;
    c->data.buffer.bytes = (uint8_t*)aligned_alloc(64, cap);
    memset(c->data.buffer.bytes, 0, cap);
    return c;
}

static void buffer_grow(Cell* buf) {
    uint32_t new_cap = buf->data.buffer.capacity * 2;
    uint8_t* new_bytes = (uint8_t*)aligned_alloc(64, new_cap);
    memset(new_bytes, 0, new_cap);
    memcpy(new_bytes, buf->data.buffer.bytes, buf->data.buffer.size);
    free(buf->data.buffer.bytes);
    buf->data.buffer.bytes = new_bytes;
    buf->data.buffer.capacity = new_cap;
}

uint8_t cell_buffer_get(Cell* buf, uint32_t idx) {
    assert(buf->type == CELL_BUFFER);
    assert(idx < buf->data.buffer.size);
    __builtin_prefetch(buf->data.buffer.bytes + idx);
    return buf->data.buffer.bytes[idx];
}

void cell_buffer_set(Cell* buf, uint32_t idx, uint8_t val) {
    assert(buf->type == CELL_BUFFER);
    assert(idx < buf->data.buffer.size);
    __builtin_prefetch(buf->data.buffer.bytes + idx);
    buf->data.buffer.bytes[idx] = val;
}

void cell_buffer_append(Cell* buf, uint8_t val) {
    assert(buf->type == CELL_BUFFER);
    if (buf->data.buffer.size >= buf->data.buffer.capacity) {
        buffer_grow(buf);
    }
    buf->data.buffer.bytes[buf->data.buffer.size++] = val;
}

Cell* cell_buffer_concat(Cell* a, Cell* b) {
    assert(a->type == CELL_BUFFER);
    assert(b->type == CELL_BUFFER);
    uint32_t total = a->data.buffer.size + b->data.buffer.size;
    Cell* result = cell_buffer_new(total);
    memcpy(result->data.buffer.bytes, a->data.buffer.bytes, a->data.buffer.size);
    memcpy(result->data.buffer.bytes + a->data.buffer.size, b->data.buffer.bytes, b->data.buffer.size);
    result->data.buffer.size = total;
    return result;
}

Cell* cell_buffer_slice(Cell* buf, uint32_t start, uint32_t end) {
    assert(buf->type == CELL_BUFFER);
    /* Clamp bounds (Python semantics) */
    if (start > buf->data.buffer.size) start = buf->data.buffer.size;
    if (end > buf->data.buffer.size) end = buf->data.buffer.size;
    if (end < start) end = start;
    uint32_t len = end - start;
    Cell* result = cell_buffer_new(len);
    if (len > 0) {
        memcpy(result->data.buffer.bytes, buf->data.buffer.bytes + start, len);
    }
    result->data.buffer.size = len;
    return result;
}

uint32_t cell_buffer_size(Cell* buf) {
    assert(buf->type == CELL_BUFFER);
    return buf->data.buffer.size;
}

Cell* cell_buffer_to_list(Cell* buf) {
    assert(buf->type == CELL_BUFFER);
    Cell* result = cell_nil();
    for (int32_t i = (int32_t)buf->data.buffer.size - 1; i >= 0; i--) {
        Cell* num = cell_number((double)buf->data.buffer.bytes[i]);
        Cell* pair = cell_cons(num, result);
        cell_release(num);
        cell_release(result);
        result = pair;
    }
    return result;
}

const char* cell_buffer_to_string(Cell* buf) {
    assert(buf->type == CELL_BUFFER);
    char* str = (char*)malloc(buf->data.buffer.size + 1);
    memcpy(str, buf->data.buffer.bytes, buf->data.buffer.size);
    str[buf->data.buffer.size] = '\0';
    return str;
}

Cell* cell_buffer_from_string(const char* str) {
    uint32_t len = (uint32_t)strlen(str);
    Cell* buf = cell_buffer_new(len);
    memcpy(buf->data.buffer.bytes, str, len);
    buf->data.buffer.size = len;
    return buf;
}

/* =========================================================================
 * Vector — HFT-grade dynamic array with Small Buffer Optimization
 *
 * SBO: 4 inline Cell* slots (32 bytes, zero alloc for small vectors)
 * Growth: 1.5x (enables memory reuse unlike 2x; used by fbvector/MSVC/Java)
 * Heap: cache-line aligned (64 bytes) for L1 utilization
 * Prefetch: 8 elements ahead for sequential, target for random access
 * Branch hints: OOB and resize marked unlikely
 * ========================================================================= */

#define VEC_SBO_CAP 4
#define VEC_INITIAL_HEAP_CAP 8

/* Get pointer to element buffer (SBO or heap) */
static inline Cell** vec_buf(Cell* v) {
    return __builtin_expect(v->data.vector.capacity <= VEC_SBO_CAP, 1)
        ? v->data.vector.sbo
        : v->data.vector.heap;
}

/* Next capacity: 1.5x growth (integer-only) */
static inline uint32_t vec_next_cap(uint32_t old) {
    return old + (old >> 1);
}

/* Grow vector: SBO→heap or heap→bigger heap */
static void vector_grow(Cell* v) {
    uint32_t old_cap = v->data.vector.capacity;
    uint32_t sz = v->data.vector.size;

    if (old_cap <= VEC_SBO_CAP) {
        /* SBO → heap transition */
        uint32_t new_cap = VEC_INITIAL_HEAP_CAP;
        Cell** new_buf = (Cell**)aligned_alloc(64, new_cap * sizeof(Cell*));
        memset(new_buf, 0, new_cap * sizeof(Cell*));
        /* Copy SBO elements to heap */
        memcpy(new_buf, v->data.vector.sbo, sz * sizeof(Cell*));
        v->data.vector.heap = new_buf;
        v->data.vector.capacity = new_cap;
    } else {
        /* Heap → bigger heap (1.5x) */
        uint32_t new_cap = vec_next_cap(old_cap);
        Cell** old_buf = v->data.vector.heap;
        Cell** new_buf = (Cell**)aligned_alloc(64, new_cap * sizeof(Cell*));
        memset(new_buf, 0, new_cap * sizeof(Cell*));
        memcpy(new_buf, old_buf, sz * sizeof(Cell*));
        free(old_buf);
        v->data.vector.heap = new_buf;
        v->data.vector.capacity = new_cap;
    }
}

bool cell_is_vector(Cell* c) {
    return c && c->type == CELL_VECTOR;
}

Cell* cell_vector_new(uint32_t initial_cap) {
    Cell* c = cell_alloc(CELL_VECTOR);
    c->data.vector.size = 0;

    if (initial_cap <= VEC_SBO_CAP) {
        /* SBO mode — inline storage, zero heap alloc */
        c->data.vector.capacity = VEC_SBO_CAP;
        memset(c->data.vector.sbo, 0, VEC_SBO_CAP * sizeof(Cell*));
    } else {
        /* Heap mode */
        uint32_t cap = VEC_INITIAL_HEAP_CAP;
        while (cap < initial_cap) cap = vec_next_cap(cap);
        c->data.vector.capacity = cap;
        c->data.vector.heap = (Cell**)aligned_alloc(64, cap * sizeof(Cell*));
        memset(c->data.vector.heap, 0, cap * sizeof(Cell*));
    }
    return c;
}

Cell* cell_vector_get(Cell* v, uint32_t idx) {
    assert(v->type == CELL_VECTOR);
    if (__builtin_expect(idx >= v->data.vector.size, 0))
        return NULL;
    Cell** buf = vec_buf(v);
    __builtin_prefetch(&buf[idx], 0, 3);
    return buf[idx];
}

Cell* cell_vector_set(Cell* v, uint32_t idx, Cell* val) {
    assert(v->type == CELL_VECTOR);
    if (__builtin_expect(idx >= v->data.vector.size, 0))
        return NULL;
    Cell** buf = vec_buf(v);
    __builtin_prefetch(&buf[idx], 1, 3);
    Cell* old = buf[idx];
    buf[idx] = val;
    cell_retain(val);
    /* Return old value — caller inherits the vector's reference */
    return old;
}

void cell_vector_push(Cell* v, Cell* val) {
    assert(v->type == CELL_VECTOR);
    if (__builtin_expect(v->data.vector.size == v->data.vector.capacity, 0)) {
        vector_grow(v);
    }
    Cell** buf = vec_buf(v);
    uint32_t idx = v->data.vector.size;
    __builtin_prefetch(&buf[idx], 1, 3);
    buf[idx] = val;
    cell_retain(val);
    v->data.vector.size++;
}

Cell* cell_vector_pop(Cell* v) {
    assert(v->type == CELL_VECTOR);
    if (v->data.vector.size == 0) return NULL;
    v->data.vector.size--;
    Cell** buf = vec_buf(v);
    Cell* val = buf[v->data.vector.size];
    buf[v->data.vector.size] = NULL;
    /* Caller inherits the vector's reference — no release here */
    return val;
}

uint32_t cell_vector_size(Cell* v) {
    assert(v->type == CELL_VECTOR);
    return v->data.vector.size;
}

Cell* cell_vector_to_list(Cell* v) {
    assert(v->type == CELL_VECTOR);
    uint32_t sz = v->data.vector.size;
    Cell** buf = vec_buf(v);
    /* Build list back-to-front for correct order */
    Cell* result = cell_nil();
    for (uint32_t i = sz; i > 0; i--) {
        /* Prefetch 8 elements ahead for sequential traversal */
        if (i > 8) __builtin_prefetch(&buf[i - 9], 0, 3);
        Cell* pair = cell_cons(buf[i - 1], result);
        cell_release(result);
        result = pair;
    }
    return result;
}

Cell* cell_vector_slice(Cell* v, uint32_t start, uint32_t end) {
    assert(v->type == CELL_VECTOR);
    /* Clamp bounds */
    if (start > v->data.vector.size) start = v->data.vector.size;
    if (end > v->data.vector.size) end = v->data.vector.size;
    if (end < start) end = start;
    uint32_t len = end - start;

    Cell* result = cell_vector_new(len);
    Cell** src = vec_buf(v);
    Cell** dst = vec_buf(result);

    for (uint32_t i = 0; i < len; i++) {
        dst[i] = src[start + i];
        cell_retain(dst[i]);
    }
    result->data.vector.size = len;
    return result;
}

/* =========================================================================
 * Heap — 4-ary Min-Heap Priority Queue (HFT-grade)
 *
 * SoA layout: separate keys[] and vals[] arrays, both cache-line aligned.
 * 4-ary: parent = (i-1)>>2, first_child = (i<<2)+1
 * Branchless min-of-4 comparison tree (3 CMOVs).
 * Move-based sift (not swap): 1 write per level instead of 3.
 * Prefetch grandchildren keys during sift-down.
 * ========================================================================= */

#define HEAP4_MIN_CAP 16

static inline uint32_t heap4_parent(uint32_t i) { return (i - 1) >> 2; }
static inline uint32_t heap4_first_child(uint32_t i) { return (i << 2) + 1; }

/* Branchless min-of-4 children — returns index of smallest key */
static inline uint32_t heap4_min_child(const double* k, uint32_t first, uint32_t count) {
    uint32_t i0 = first;
    uint32_t best = i0;
    if (count > 1 && k[first + 1] < k[best]) best = first + 1;
    if (count > 2 && k[first + 2] < k[best]) best = first + 2;
    if (count > 3 && k[first + 3] < k[best]) best = first + 3;
    return best;
}

/* Grow heap arrays to 2x capacity */
static void heap4_grow(Cell* h) {
    uint32_t new_cap = h->data.pq.capacity * 2;
    double* new_keys = (double*)aligned_alloc(64, new_cap * sizeof(double));
    Cell** new_vals = (Cell**)aligned_alloc(64, new_cap * sizeof(Cell*));
    memcpy(new_keys, h->data.pq.keys, h->data.pq.size * sizeof(double));
    memcpy(new_vals, h->data.pq.vals, h->data.pq.size * sizeof(Cell*));
    free(h->data.pq.keys);
    free(h->data.pq.vals);
    h->data.pq.keys = new_keys;
    h->data.pq.vals = new_vals;
    h->data.pq.capacity = new_cap;
}

/* Move-based sift-up: shift parents down, place element once at final pos */
static void heap4_sift_up(double* keys, Cell** vals, uint32_t pos) {
    double key = keys[pos];
    Cell* val = vals[pos];
    while (pos > 0) {
        uint32_t p = heap4_parent(pos);
        if (keys[p] <= key) break;
        keys[pos] = keys[p];
        vals[pos] = vals[p];
        pos = p;
    }
    keys[pos] = key;
    vals[pos] = val;
}

/* Move-based sift-down: shift smallest child up, place element once */
static void heap4_sift_down(double* keys, Cell** vals, uint32_t size, uint32_t pos) {
    double key = keys[pos];
    Cell* val = vals[pos];
    while (1) {
        uint32_t fc = heap4_first_child(pos);
        if (fc >= size) break;
        uint32_t nchildren = size - fc;
        if (nchildren > 4) nchildren = 4;
        /* Prefetch grandchildren keys */
        uint32_t gc = heap4_first_child(fc);
        if (gc < size) __builtin_prefetch(&keys[gc], 0, 3);
        uint32_t mc = heap4_min_child(keys, fc, nchildren);
        if (keys[mc] >= key) break;
        keys[pos] = keys[mc];
        vals[pos] = vals[mc];
        pos = mc;
    }
    keys[pos] = key;
    vals[pos] = val;
}

bool cell_is_heap(Cell* c) {
    return c && c->type == CELL_HEAP;
}

Cell* cell_heap_new(void) {
    Cell* c = cell_alloc(CELL_HEAP);
    c->data.pq.capacity = HEAP4_MIN_CAP;
    c->data.pq.size = 0;
    c->data.pq.keys = (double*)aligned_alloc(64, HEAP4_MIN_CAP * sizeof(double));
    c->data.pq.vals = (Cell**)aligned_alloc(64, HEAP4_MIN_CAP * sizeof(Cell*));
    memset(c->data.pq.vals, 0, HEAP4_MIN_CAP * sizeof(Cell*));
    return c;
}

void cell_heap_push(Cell* h, double priority, Cell* val) {
    assert(h->type == CELL_HEAP);
    if (__builtin_expect(h->data.pq.size == h->data.pq.capacity, 0)) {
        heap4_grow(h);
    }
    uint32_t pos = h->data.pq.size;
    h->data.pq.keys[pos] = priority;
    h->data.pq.vals[pos] = val;
    cell_retain(val);
    h->data.pq.size++;
    heap4_sift_up(h->data.pq.keys, h->data.pq.vals, pos);
}

Cell* cell_heap_pop(Cell* h) {
    assert(h->type == CELL_HEAP);
    if (h->data.pq.size == 0) return NULL;
    double key = h->data.pq.keys[0];
    Cell* val = h->data.pq.vals[0];
    h->data.pq.size--;
    if (h->data.pq.size > 0) {
        h->data.pq.keys[0] = h->data.pq.keys[h->data.pq.size];
        h->data.pq.vals[0] = h->data.pq.vals[h->data.pq.size];
        heap4_sift_down(h->data.pq.keys, h->data.pq.vals, h->data.pq.size, 0);
    }
    /* Return ⟨priority value⟩ pair — caller inherits val's reference */
    Cell* kc = cell_number(key);
    Cell* pair = cell_cons(kc, val);
    cell_release(kc);
    cell_release(val);
    return pair;
}

Cell* cell_heap_peek(Cell* h) {
    assert(h->type == CELL_HEAP);
    if (h->data.pq.size == 0) return NULL;
    Cell* kc = cell_number(h->data.pq.keys[0]);
    Cell* vc = h->data.pq.vals[0];
    cell_retain(vc);
    Cell* pair = cell_cons(kc, vc);
    cell_release(kc);
    cell_release(vc);
    return pair;
}

uint32_t cell_heap_size(Cell* h) {
    assert(h->type == CELL_HEAP);
    return h->data.pq.size;
}

Cell* cell_heap_to_list(Cell* h) {
    assert(h->type == CELL_HEAP);
    uint32_t sz = h->data.pq.size;
    /* Copy into temp arrays, pop all to get sorted order */
    double* tmp_keys = (double*)malloc(sz * sizeof(double));
    Cell** tmp_vals = (Cell**)malloc(sz * sizeof(Cell*));
    memcpy(tmp_keys, h->data.pq.keys, sz * sizeof(double));
    memcpy(tmp_vals, h->data.pq.vals, sz * sizeof(Cell*));
    /* Pop from copy to build sorted list */
    Cell* result = cell_nil();
    uint32_t remaining = sz;
    while (remaining > 0) {
        double key = tmp_keys[0];
        Cell* val = tmp_vals[0];
        remaining--;
        if (remaining > 0) {
            tmp_keys[0] = tmp_keys[remaining];
            tmp_vals[0] = tmp_vals[remaining];
            heap4_sift_down(tmp_keys, tmp_vals, remaining, 0);
        }
        Cell* kc = cell_number(key);
        Cell* pair = cell_cons(kc, val);
        Cell* node = cell_cons(pair, result);
        cell_release(pair);
        cell_release(kc);
        cell_release(result);
        result = node;
    }
    free(tmp_keys);
    free(tmp_vals);
    /* Result is in reverse sorted order — reverse it */
    Cell* reversed = cell_nil();
    Cell* cur = result;
    while (cur && !cell_is_nil(cur)) {
        Cell* head = cell_car(cur);
        Cell* next = cell_cdr(cur);
        Cell* node = cell_cons(head, reversed);
        cell_release(reversed);
        reversed = node;
        cur = next;
    }
    cell_release(result);
    return reversed;
}

Cell* cell_heap_merge(Cell* h1, Cell* h2) {
    assert(h1->type == CELL_HEAP);
    assert(h2->type == CELL_HEAP);
    Cell* result = cell_heap_new();
    /* Push all from h1 */
    for (uint32_t i = 0; i < h1->data.pq.size; i++) {
        cell_heap_push(result, h1->data.pq.keys[i], h1->data.pq.vals[i]);
    }
    /* Push all from h2 */
    for (uint32_t i = 0; i < h2->data.pq.size; i++) {
        cell_heap_push(result, h2->data.pq.keys[i], h2->data.pq.vals[i]);
    }
    return result;
}

/* ===== Sorted Map (⋔) — Algorithmica-Grade SIMD B-Tree ===== */
/* SMNode and SMPool types defined at top of file (needed by release/print/hash) */

static SMPool* sm_pool_new(void) {
    SMPool* p = (SMPool*)malloc(sizeof(SMPool));
    p->capacity = SM_POOL_INIT;
    p->used = 0;
    p->nodes = (SMNode*)aligned_alloc(64, SM_POOL_INIT * sizeof(SMNode));
    memset(p->nodes, 0, SM_POOL_INIT * sizeof(SMNode));
    p->free_list = (uint32_t*)malloc(SM_POOL_INIT * sizeof(uint32_t));
    p->free_count = 0;
    p->free_cap = SM_POOL_INIT;
    return p;
}

static void sm_pool_grow(SMPool* p) {
    uint32_t new_cap = p->capacity * 2;
    SMNode* new_nodes = (SMNode*)aligned_alloc(64, new_cap * sizeof(SMNode));
    memcpy(new_nodes, p->nodes, p->capacity * sizeof(SMNode));
    memset(new_nodes + p->capacity, 0, (new_cap - p->capacity) * sizeof(SMNode));
    free(p->nodes);
    p->nodes = new_nodes;
    /* Grow free list too */
    uint32_t* new_fl = (uint32_t*)realloc(p->free_list, new_cap * sizeof(uint32_t));
    p->free_list = new_fl;
    p->free_cap = new_cap;
    p->capacity = new_cap;
}

static uint32_t sm_pool_alloc(SMPool* p) {
    uint32_t idx;
    if (p->free_count > 0) {
        idx = p->free_list[--p->free_count];
    } else {
        if (p->used >= p->capacity) {
            sm_pool_grow(p);
        }
        idx = p->used++;
    }
    memset(&p->nodes[idx], 0, sizeof(SMNode));
    /* Fill sort_keys with UINT64_MAX so unused slots don't affect rank */
    memset(p->nodes[idx].sort_keys, 0xFF, sizeof(p->nodes[idx].sort_keys));
    for (int i = 0; i <= BTREE_B; i++) p->nodes[idx].children[i] = SM_NIL;
    p->nodes[idx].next_leaf = SM_NIL;
    p->nodes[idx].prev_leaf = SM_NIL;
    return idx;
}

static void sm_pool_free(SMPool* p, uint32_t idx) {
    if (p->free_count >= p->free_cap) {
        uint32_t new_cap = p->free_cap * 2;
        p->free_list = (uint32_t*)realloc(p->free_list, new_cap * sizeof(uint32_t));
        p->free_cap = new_cap;
    }
    p->free_list[p->free_count++] = idx;
}

static void sm_pool_destroy_impl(SMPool* p) {
    free(p->nodes);
    free(p->free_list);
    free(p);
}

/* === cell_compare: total ordering (Erlang term ordering) === */
/* nil < bool (#f < #t) < number < symbol < string < pair < everything else */

int cell_compare(Cell* a, Cell* b) {
    if (a == b) return 0;
    if (!a) return -1;
    if (!b) return 1;

    /* Type ordering */
    static const int type_order[] = {
        [CELL_ATOM_NIL] = 0,
        [CELL_ATOM_BOOL] = 1,
        [CELL_ATOM_NUMBER] = 2,
        [CELL_ATOM_SYMBOL] = 3,
        [CELL_ATOM_STRING] = 4,
        [CELL_PAIR] = 5,
        [CELL_LAMBDA] = 6,
        [CELL_BUILTIN] = 7,
        [CELL_ERROR] = 8,
        [CELL_STRUCT] = 9,
        [CELL_GRAPH] = 10,
        [CELL_ACTOR] = 11,
        [CELL_CHANNEL] = 12,
        [CELL_BOX] = 13,
        [CELL_WEAK_REF] = 14,
        [CELL_HASHMAP] = 15,
        [CELL_SET] = 16,
        [CELL_DEQUE] = 17,
        [CELL_BUFFER] = 18,
        [CELL_VECTOR] = 19,
        [CELL_HEAP] = 20,
        [CELL_SORTED_MAP] = 21,
    };

    int ta = type_order[a->type];
    int tb = type_order[b->type];
    if (ta != tb) return (ta < tb) ? -1 : 1;

    /* Same type — compare within type */
    switch (a->type) {
        case CELL_ATOM_NIL: return 0;
        case CELL_ATOM_BOOL: {
            int va = a->data.atom.boolean ? 1 : 0;
            int vb = b->data.atom.boolean ? 1 : 0;
            return (va < vb) ? -1 : (va > vb) ? 1 : 0;
        }
        case CELL_ATOM_NUMBER: {
            double da = a->data.atom.number;
            double db = b->data.atom.number;
            return (da < db) ? -1 : (da > db) ? 1 : 0;
        }
        case CELL_ATOM_SYMBOL: {
            return strcmp(a->data.atom.symbol, b->data.atom.symbol);
        }
        case CELL_ATOM_STRING: {
            return strcmp(a->data.atom.string, b->data.atom.string);
        }
        case CELL_PAIR: {
            int c = cell_compare(a->data.pair.car, b->data.pair.car);
            if (c != 0) return c;
            return cell_compare(a->data.pair.cdr, b->data.pair.cdr);
        }
        default: {
            /* Pointer-based ordering for remaining types */
            uintptr_t pa = (uintptr_t)a;
            uintptr_t pb = (uintptr_t)b;
            return (pa < pb) ? -1 : (pa > pb) ? 1 : 0;
        }
    }
}

/* === Sort-key extraction: Cell* → uint64_t === */

static uint64_t cell_sort_key(Cell* c) {
    if (!c) return 0;
    uint64_t tag = (uint64_t)c->type << 60;
    switch (c->type) {
        case CELL_ATOM_NIL:    return tag;
        case CELL_ATOM_BOOL:   return tag | (uint64_t)c->data.atom.boolean;
        case CELL_ATOM_NUMBER: return tag | (double_to_sortkey(c->data.atom.number) >> 4);
        case CELL_ATOM_SYMBOL: {
            uint64_t prefix = 0;
            const char* s = c->data.atom.symbol;
            for (int i = 0; i < 7 && s[i]; i++)
                prefix |= (uint64_t)(uint8_t)s[i] << (48 - i * 8);
            return tag | prefix;
        }
        case CELL_ATOM_STRING: {
            uint64_t prefix = 0;
            const char* s = c->data.atom.string;
            for (int i = 0; i < 7 && s[i]; i++)
                prefix |= (uint64_t)(uint8_t)s[i] << (48 - i * 8);
            return tag | prefix;
        }
        default: return tag | (uint64_t)(uintptr_t)c;
    }
}

/* === B-tree search === */

/* Find position of key in node (returns index where key should be).
 * If exact match found, *found is set to 1 and index points to the match. */
static unsigned sm_node_find(SMPool* pool, uint32_t node_idx,
                              uint64_t sk, Cell* key, int* found) {
    SMNode* node = &pool->nodes[node_idx];
    *found = 0;
    unsigned pos = sm_rank16(sk, node->sort_keys);
    /* Check for exact match at pos-1 (rank gives count of keys < query) */
    /* We need to check pos-1 and pos for sort_key ties */
    for (unsigned i = (pos > 0 ? pos - 1 : 0); i < node->n_keys; i++) {
        if (node->sort_keys[i] > sk) break;
        if (node->sort_keys[i] == sk && cell_compare(node->keys[i], key) == 0) {
            *found = 1;
            return i;
        }
    }
    return pos;
}

/* Search for key in B-tree, returns leaf node index and position.
 * If found, *found=1 and returned position is the key's index in the leaf. */
static uint32_t sm_search(SMPool* pool, uint32_t root, uint8_t height,
                           uint64_t sk, Cell* key, unsigned* pos, int* found) {
    uint32_t cur = root;
    *found = 0;
    for (uint8_t h = 0; h < height; h++) {
        SMNode* node = &pool->nodes[cur];
        unsigned rank = sm_rank16(sk, node->sort_keys);
        /* Check exact match in sort keys for early exit on internal nodes */
        for (unsigned i = (rank > 0 ? rank - 1 : 0); i < node->n_keys; i++) {
            if (node->sort_keys[i] > sk) break;
            if (node->sort_keys[i] == sk && cell_compare(node->keys[i], key) == 0) {
                rank = i;
                break;
            }
        }
        /* Prefetch next child */
        uint32_t child = node->children[rank];
        if (child != SM_NIL) {
            __builtin_prefetch(&pool->nodes[child].sort_keys, 0, 3);
        }
        cur = child;
    }
    /* Now at leaf level */
    *pos = sm_node_find(pool, cur, sk, key, found);
    return cur;
}

/* === B-tree split === */

/* Split a full node into two, returning the median key/value and new node index */
static void sm_split(SMPool* pool, uint32_t node_idx, uint32_t* new_idx,
                      Cell** median_key, Cell** median_val, uint64_t* median_sk) {
    SMNode* node = &pool->nodes[node_idx];
    uint8_t mid = BTREE_B / 2;  /* 8 */

    *new_idx = sm_pool_alloc(pool);
    /* Re-fetch pointers after possible realloc */
    node = &pool->nodes[node_idx];
    SMNode* new_node = &pool->nodes[*new_idx];
    new_node->is_leaf = node->is_leaf;

    *median_key = node->keys[mid];
    *median_sk = node->sort_keys[mid];
    if (node->is_leaf) {
        *median_val = node->values[mid];
    } else {
        *median_val = NULL;
    }

    /* Copy upper half to new node */
    uint8_t right_start = mid + 1;
    uint8_t right_count = node->n_keys - right_start;
    for (uint8_t i = 0; i < right_count; i++) {
        new_node->sort_keys[i] = node->sort_keys[right_start + i];
        new_node->keys[i] = node->keys[right_start + i];
        if (node->is_leaf) {
            new_node->values[i] = node->values[right_start + i];
        }
    }
    if (!node->is_leaf) {
        for (uint8_t i = 0; i <= right_count; i++) {
            new_node->children[i] = node->children[right_start + i];
        }
    }
    new_node->n_keys = right_count;
    node->n_keys = mid;

    /* Maintain leaf chain */
    if (node->is_leaf) {
        new_node->next_leaf = node->next_leaf;
        new_node->prev_leaf = node_idx;
        if (node->next_leaf != SM_NIL) {
            pool->nodes[node->next_leaf].prev_leaf = *new_idx;
        }
        node->next_leaf = *new_idx;
    }
}

/* Insert into a non-full node (recursive) */
static void sm_insert_nonfull(SMPool* pool, uint32_t node_idx,
                               Cell* key, Cell* value, uint64_t sk,
                               uint32_t* first_leaf, uint32_t* last_leaf);

/* Insert key-value into leaf node at position pos, shifting right */
static void sm_leaf_insert_at(SMNode* node, unsigned pos,
                               Cell* key, Cell* value, uint64_t sk) {
    /* Shift right */
    for (int i = (int)node->n_keys - 1; i >= (int)pos; i--) {
        node->sort_keys[i + 1] = node->sort_keys[i];
        node->keys[i + 1] = node->keys[i];
        node->values[i + 1] = node->values[i];
    }
    node->sort_keys[pos] = sk;
    node->keys[pos] = key;
    node->values[pos] = value;
    cell_retain(key);
    cell_retain(value);
    node->n_keys++;
}

/* Insert into internal node at position pos, with child split */
static void sm_internal_insert_at(SMNode* node, unsigned pos,
                                    Cell* key, uint64_t sk, uint32_t right_child) {
    /* Shift right */
    for (int i = (int)node->n_keys - 1; i >= (int)pos; i--) {
        node->sort_keys[i + 1] = node->sort_keys[i];
        node->keys[i + 1] = node->keys[i];
        node->children[i + 2] = node->children[i + 1];
    }
    node->sort_keys[pos] = sk;
    node->keys[pos] = key;
    node->children[pos + 1] = right_child;
    node->n_keys++;
}

static void sm_insert_nonfull(SMPool* pool, uint32_t node_idx,
                               Cell* key, Cell* value, uint64_t sk,
                               uint32_t* first_leaf, uint32_t* last_leaf) {
    SMNode* node = &pool->nodes[node_idx];

    if (node->is_leaf) {
        /* Find position and check for duplicate */
        int found = 0;
        unsigned pos = sm_node_find(pool, node_idx, sk, key, &found);
        if (found) {
            /* Overwrite value — release old, retain new */
            node = &pool->nodes[node_idx]; /* re-fetch */
            Cell* old_val = node->values[pos];
            node->values[pos] = value;
            cell_retain(value);
            cell_release(old_val);
            return;
        }
        sm_leaf_insert_at(node, pos, key, value, sk);
    } else {
        /* Internal node — find child */
        unsigned pos = sm_rank16(sk, node->sort_keys);
        /* Check for existing key in internal node (sort key ties) */
        for (unsigned i = (pos > 0 ? pos - 1 : 0); i < node->n_keys; i++) {
            if (node->sort_keys[i] > sk) break;
            if (node->sort_keys[i] == sk && cell_compare(node->keys[i], key) == 0) {
                /* Key exists — for B-tree we only store keys in leaves for simplicity */
                /* Since our B-tree puts all data in leaves, go to child anyway */
                break;
            }
        }
        uint32_t child_idx = node->children[pos];
        SMNode* child = &pool->nodes[child_idx];
        if (child->n_keys == BTREE_B) {
            /* Child is full — split it */
            uint32_t new_idx;
            Cell* med_key;
            Cell* med_val;
            uint64_t med_sk;
            sm_split(pool, child_idx, &new_idx, &med_key, &med_val, &med_sk);
            /* Re-fetch after potential realloc */
            node = &pool->nodes[node_idx];
            /* Insert median into this node */
            sm_internal_insert_at(node, pos, med_key, med_sk, new_idx);
            /* Update leaf chain boundaries */
            if (pool->nodes[new_idx].is_leaf && pool->nodes[new_idx].next_leaf == SM_NIL) {
                *last_leaf = new_idx;
            }
            /* Decide which child to recurse into */
            if (sk > med_sk || (sk == med_sk && cell_compare(key, med_key) > 0)) {
                child_idx = new_idx;
            }
        }
        sm_insert_nonfull(pool, child_idx, key, value, sk, first_leaf, last_leaf);
    }
}

/* === Public API === */

bool cell_is_sorted_map(Cell* c) {
    return c && c->type == CELL_SORTED_MAP;
}

Cell* cell_sorted_map_new(void) {
    Cell* c = cell_alloc(CELL_SORTED_MAP);
    SMPool* pool = sm_pool_new();
    /* Create initial empty leaf as root */
    uint32_t root = sm_pool_alloc(pool);
    pool->nodes[root].is_leaf = 1;

    c->data.sorted_map.node_pool = pool;
    c->data.sorted_map.root_idx = root;
    c->data.sorted_map.first_leaf = root;
    c->data.sorted_map.last_leaf = root;
    c->data.sorted_map.size = 0;
    c->data.sorted_map.height = 0;
    return c;
}

Cell* cell_sorted_map_put(Cell* m, Cell* key, Cell* value) {
    assert(m->type == CELL_SORTED_MAP);
    SMPool* pool = (SMPool*)m->data.sorted_map.node_pool;
    uint64_t sk = cell_sort_key(key);
    uint32_t root = m->data.sorted_map.root_idx;

    /* Check if root is full */
    if (pool->nodes[root].n_keys == BTREE_B) {
        /* Split root — create new root */
        uint32_t old_root = root;
        uint32_t new_root = sm_pool_alloc(pool);
        uint32_t new_child;
        Cell* med_key;
        Cell* med_val;
        uint64_t med_sk;
        sm_split(pool, old_root, &new_child, &med_key, &med_val, &med_sk);
        /* Re-fetch after potential realloc */
        SMNode* nr = &pool->nodes[new_root];
        nr->is_leaf = 0;
        nr->n_keys = 1;
        nr->sort_keys[0] = med_sk;
        nr->keys[0] = med_key;
        nr->children[0] = old_root;
        nr->children[1] = new_child;
        for (int i = 2; i <= BTREE_B; i++) nr->children[i] = SM_NIL;
        nr->next_leaf = SM_NIL;
        nr->prev_leaf = SM_NIL;
        m->data.sorted_map.root_idx = new_root;
        m->data.sorted_map.height++;
        /* Update leaf chain boundaries */
        if (pool->nodes[new_child].is_leaf && pool->nodes[new_child].next_leaf == SM_NIL) {
            m->data.sorted_map.last_leaf = new_child;
        }
        root = new_root;
    }

    /* Check for existing key to determine if this is an insert or update */
    uint32_t old_size = m->data.sorted_map.size;
    sm_insert_nonfull(pool, root, key, value, sk,
                      &m->data.sorted_map.first_leaf,
                      &m->data.sorted_map.last_leaf);

    /* Count: did we actually insert (not just overwrite)? */
    /* Re-count by searching — simpler than tracking in insert */
    /* Actually, check if the size should increment by searching first */
    /* For efficiency, we search before insert to know if it's new */
    /* But we already inserted. Let's just use a search to verify. */
    /* Simpler approach: track insert vs overwrite via return value from search */

    /* Recount: walk leaves to get exact count */
    /* Even simpler: we know old_size, search for key, if it existed it's overwrite */
    unsigned pos;
    int found;
    sm_search(pool, m->data.sorted_map.root_idx, m->data.sorted_map.height,
              sk, key, &pos, &found);
    if (found) {
        /* Key exists in tree — was it there before? */
        /* We always increment and let overwrite path not increment */
        /* Actually this is after insert, key will always be found now */
        /* Track by checking if old_size == current count */
    }
    /* Simple approach: just recount leaves */
    uint32_t count = 0;
    uint32_t leaf = m->data.sorted_map.first_leaf;
    while (leaf != SM_NIL) {
        count += pool->nodes[leaf].n_keys;
        leaf = pool->nodes[leaf].next_leaf;
    }
    Cell* old_val = NULL;
    if (count == old_size) {
        /* Was an overwrite — but we already handled value swap in insert */
        /* Return the old value? We don't have it anymore. Return nil as "existed" marker */
        old_val = cell_nil();  /* Marker: key existed */
    } else {
        m->data.sorted_map.size = count;
        old_val = NULL;  /* New key inserted */
    }

    /* Update first/last leaf caches */
    leaf = m->data.sorted_map.root_idx;
    while (!pool->nodes[leaf].is_leaf) {
        leaf = pool->nodes[leaf].children[0];
    }
    m->data.sorted_map.first_leaf = leaf;

    leaf = m->data.sorted_map.root_idx;
    while (!pool->nodes[leaf].is_leaf) {
        SMNode* n = &pool->nodes[leaf];
        leaf = n->children[n->n_keys];
    }
    m->data.sorted_map.last_leaf = leaf;

    return old_val ? old_val : cell_nil();
}

Cell* cell_sorted_map_get(Cell* m, Cell* key) {
    assert(m->type == CELL_SORTED_MAP);
    SMPool* pool = (SMPool*)m->data.sorted_map.node_pool;
    uint64_t sk = cell_sort_key(key);
    unsigned pos;
    int found;
    uint32_t leaf = sm_search(pool, m->data.sorted_map.root_idx,
                               m->data.sorted_map.height, sk, key, &pos, &found);
    if (found) {
        Cell* val = pool->nodes[leaf].values[pos];
        cell_retain(val);
        return val;
    }
    return NULL; /* Not found */
}

bool cell_sorted_map_has(Cell* m, Cell* key) {
    assert(m->type == CELL_SORTED_MAP);
    SMPool* pool = (SMPool*)m->data.sorted_map.node_pool;
    uint64_t sk = cell_sort_key(key);
    unsigned pos;
    int found;
    sm_search(pool, m->data.sorted_map.root_idx,
              m->data.sorted_map.height, sk, key, &pos, &found);
    return found != 0;
}

uint32_t cell_sorted_map_size(Cell* m) {
    assert(m->type == CELL_SORTED_MAP);
    return m->data.sorted_map.size;
}

/* Delete: remove key, returns old value or NULL */
Cell* cell_sorted_map_del(Cell* m, Cell* key) {
    assert(m->type == CELL_SORTED_MAP);
    SMPool* pool = (SMPool*)m->data.sorted_map.node_pool;
    uint64_t sk = cell_sort_key(key);
    unsigned pos;
    int found;
    uint32_t leaf = sm_search(pool, m->data.sorted_map.root_idx,
                               m->data.sorted_map.height, sk, key, &pos, &found);
    if (!found) return NULL;

    SMNode* node = &pool->nodes[leaf];
    Cell* old_val = node->values[pos];
    Cell* old_key = node->keys[pos];
    /* Shift left */
    for (unsigned i = pos; i < (unsigned)node->n_keys - 1; i++) {
        node->sort_keys[i] = node->sort_keys[i + 1];
        node->keys[i] = node->keys[i + 1];
        node->values[i] = node->values[i + 1];
    }
    node->n_keys--;
    /* Clear the freed slot */
    node->sort_keys[node->n_keys] = UINT64_MAX;
    node->keys[node->n_keys] = NULL;
    node->values[node->n_keys] = NULL;
    m->data.sorted_map.size--;
    cell_release(old_key);
    /* old_val returned to caller — caller gets the reference */

    /* If leaf is empty and it's not the root, we should remove it from the chain.
     * For simplicity (and since rebalancing is complex), we leave empty leaves
     * in place — they'll be garbage when the tree is freed. */
    if (node->n_keys == 0 && m->data.sorted_map.size > 0) {
        /* Unlink from leaf chain */
        if (node->prev_leaf != SM_NIL)
            pool->nodes[node->prev_leaf].next_leaf = node->next_leaf;
        if (node->next_leaf != SM_NIL)
            pool->nodes[node->next_leaf].prev_leaf = node->prev_leaf;
        if (m->data.sorted_map.first_leaf == leaf)
            m->data.sorted_map.first_leaf = node->next_leaf;
        if (m->data.sorted_map.last_leaf == leaf)
            m->data.sorted_map.last_leaf = node->prev_leaf;
        sm_pool_free(pool, leaf);
    }

    return old_val;
}

/* Keys: return sorted list of all keys via leaf chain walk */
Cell* cell_sorted_map_keys(Cell* m) {
    assert(m->type == CELL_SORTED_MAP);
    SMPool* pool = (SMPool*)m->data.sorted_map.node_pool;
    /* Build list in reverse, then reverse */
    Cell* result = cell_nil();
    uint32_t leaf = m->data.sorted_map.last_leaf;
    while (leaf != SM_NIL) {
        SMNode* node = &pool->nodes[leaf];
        for (int i = (int)node->n_keys - 1; i >= 0; i--) {
            Cell* k = node->keys[i];
            cell_retain(k);
            Cell* pair = cell_cons(k, result);
            cell_release(k);
            cell_release(result);
            result = pair;
        }
        leaf = node->prev_leaf;
    }
    return result;
}

/* Values: return list of values in key-sorted order */
Cell* cell_sorted_map_values(Cell* m) {
    assert(m->type == CELL_SORTED_MAP);
    SMPool* pool = (SMPool*)m->data.sorted_map.node_pool;
    Cell* result = cell_nil();
    uint32_t leaf = m->data.sorted_map.last_leaf;
    while (leaf != SM_NIL) {
        SMNode* node = &pool->nodes[leaf];
        for (int i = (int)node->n_keys - 1; i >= 0; i--) {
            Cell* v = node->values[i];
            cell_retain(v);
            Cell* pair = cell_cons(v, result);
            cell_release(v);
            cell_release(result);
            result = pair;
        }
        leaf = node->prev_leaf;
    }
    return result;
}

/* Entries: return list of ⟨key value⟩ pairs in sorted order */
Cell* cell_sorted_map_entries(Cell* m) {
    assert(m->type == CELL_SORTED_MAP);
    SMPool* pool = (SMPool*)m->data.sorted_map.node_pool;
    Cell* result = cell_nil();
    uint32_t leaf = m->data.sorted_map.last_leaf;
    while (leaf != SM_NIL) {
        SMNode* node = &pool->nodes[leaf];
        for (int i = (int)node->n_keys - 1; i >= 0; i--) {
            Cell* k = node->keys[i];
            Cell* v = node->values[i];
            cell_retain(k);
            cell_retain(v);
            Cell* kv = cell_cons(k, v);
            cell_release(k);
            cell_release(v);
            Cell* pair = cell_cons(kv, result);
            cell_release(kv);
            cell_release(result);
            result = pair;
        }
        leaf = node->prev_leaf;
    }
    return result;
}

/* Merge: build new sorted map from both m1 and m2 (m2 wins on conflict) */
Cell* cell_sorted_map_merge(Cell* m1, Cell* m2) {
    assert(m1->type == CELL_SORTED_MAP);
    assert(m2->type == CELL_SORTED_MAP);
    Cell* result = cell_sorted_map_new();
    SMPool* p1 = (SMPool*)m1->data.sorted_map.node_pool;
    SMPool* p2 = (SMPool*)m2->data.sorted_map.node_pool;

    /* Insert all from m1 */
    uint32_t leaf = m1->data.sorted_map.first_leaf;
    while (leaf != SM_NIL) {
        SMNode* node = &p1->nodes[leaf];
        for (uint8_t i = 0; i < node->n_keys; i++) {
            cell_sorted_map_put(result, node->keys[i], node->values[i]);
        }
        leaf = node->next_leaf;
    }
    /* Insert all from m2 (overwrites m1 on conflict) */
    leaf = m2->data.sorted_map.first_leaf;
    while (leaf != SM_NIL) {
        SMNode* node = &p2->nodes[leaf];
        for (uint8_t i = 0; i < node->n_keys; i++) {
            cell_sorted_map_put(result, node->keys[i], node->values[i]);
        }
        leaf = node->next_leaf;
    }
    return result;
}

/* Min: O(1) via first_leaf cache */
Cell* cell_sorted_map_min(Cell* m) {
    assert(m->type == CELL_SORTED_MAP);
    if (m->data.sorted_map.size == 0) return NULL;
    SMPool* pool = (SMPool*)m->data.sorted_map.node_pool;
    SMNode* node = &pool->nodes[m->data.sorted_map.first_leaf];
    if (node->n_keys == 0) return NULL;
    Cell* k = node->keys[0];
    Cell* v = node->values[0];
    cell_retain(k);
    cell_retain(v);
    Cell* pair = cell_cons(k, v);
    cell_release(k);
    cell_release(v);
    return pair;
}

/* Max: O(1) via last_leaf cache */
Cell* cell_sorted_map_max(Cell* m) {
    assert(m->type == CELL_SORTED_MAP);
    if (m->data.sorted_map.size == 0) return NULL;
    SMPool* pool = (SMPool*)m->data.sorted_map.node_pool;
    SMNode* node = &pool->nodes[m->data.sorted_map.last_leaf];
    if (node->n_keys == 0) return NULL;
    uint8_t last = node->n_keys - 1;
    Cell* k = node->keys[last];
    Cell* v = node->values[last];
    cell_retain(k);
    cell_retain(v);
    Cell* pair = cell_cons(k, v);
    cell_release(k);
    cell_release(v);
    return pair;
}

/* Range: return entries where lo <= key <= hi */
Cell* cell_sorted_map_range(Cell* m, Cell* lo, Cell* hi) {
    assert(m->type == CELL_SORTED_MAP);
    SMPool* pool = (SMPool*)m->data.sorted_map.node_pool;
    if (m->data.sorted_map.size == 0) return cell_nil();

    /* Find starting leaf via search for lo */
    uint64_t lo_sk = cell_sort_key(lo);
    unsigned start_pos;
    int found;
    uint32_t start_leaf = sm_search(pool, m->data.sorted_map.root_idx,
                                     m->data.sorted_map.height, lo_sk, lo, &start_pos, &found);

    /* Collect entries in [lo, hi] into array */
    Cell** items = (Cell**)malloc(16 * sizeof(Cell*));
    uint32_t item_count = 0;
    uint32_t item_cap = 16;
    int first = 1;

    uint32_t leaf = start_leaf;
    while (leaf != SM_NIL) {
        SMNode* nd = &pool->nodes[leaf];
        unsigned begin = first ? start_pos : 0;
        first = 0;
        for (unsigned i = begin; i < nd->n_keys; i++) {
            Cell* k = nd->keys[i];
            if (cell_compare(k, hi) > 0) goto range_done;
            if (cell_compare(k, lo) >= 0) {
                Cell* v = nd->values[i];
                cell_retain(k);
                cell_retain(v);
                Cell* kv = cell_cons(k, v);
                cell_release(k);
                cell_release(v);
                if (item_count >= item_cap) {
                    item_cap *= 2;
                    items = (Cell**)realloc(items, item_cap * sizeof(Cell*));
                }
                items[item_count++] = kv;
            }
        }
        leaf = nd->next_leaf;
    }
range_done:;
    /* Build list from items array (reverse to get sorted order) */
    Cell* result = cell_nil();
    for (int i = (int)item_count - 1; i >= 0; i--) {
        Cell* nd = cell_cons(items[i], result);
        cell_release(items[i]);
        cell_release(result);
        result = nd;
    }
    free(items);
    return result;
}

/* Floor: greatest key <= query */
Cell* cell_sorted_map_floor(Cell* m, Cell* key) {
    assert(m->type == CELL_SORTED_MAP);
    SMPool* pool = (SMPool*)m->data.sorted_map.node_pool;
    if (m->data.sorted_map.size == 0) return NULL;

    uint64_t sk = cell_sort_key(key);
    unsigned pos;
    int found;
    uint32_t leaf = sm_search(pool, m->data.sorted_map.root_idx,
                               m->data.sorted_map.height, sk, key, &pos, &found);
    SMNode* node = &pool->nodes[leaf];

    if (found) {
        /* Exact match */
        Cell* k = node->keys[pos];
        Cell* v = node->values[pos];
        cell_retain(k);
        cell_retain(v);
        Cell* pair = cell_cons(k, v);
        cell_release(k);
        cell_release(v);
        return pair;
    }

    /* pos is where key would be inserted — floor is at pos-1 */
    if (pos > 0) {
        Cell* k = node->keys[pos - 1];
        Cell* v = node->values[pos - 1];
        cell_retain(k);
        cell_retain(v);
        Cell* pair = cell_cons(k, v);
        cell_release(k);
        cell_release(v);
        return pair;
    }

    /* Need to go to previous leaf */
    if (node->prev_leaf != SM_NIL) {
        SMNode* prev = &pool->nodes[node->prev_leaf];
        if (prev->n_keys > 0) {
            uint8_t last = prev->n_keys - 1;
            Cell* k = prev->keys[last];
            Cell* v = prev->values[last];
            cell_retain(k);
            cell_retain(v);
            Cell* pair = cell_cons(k, v);
            cell_release(k);
            cell_release(v);
            return pair;
        }
    }
    return NULL; /* No floor exists */
}

/* Ceiling: least key >= query */
Cell* cell_sorted_map_ceiling(Cell* m, Cell* key) {
    assert(m->type == CELL_SORTED_MAP);
    SMPool* pool = (SMPool*)m->data.sorted_map.node_pool;
    if (m->data.sorted_map.size == 0) return NULL;

    uint64_t sk = cell_sort_key(key);
    unsigned pos;
    int found;
    uint32_t leaf = sm_search(pool, m->data.sorted_map.root_idx,
                               m->data.sorted_map.height, sk, key, &pos, &found);
    SMNode* node = &pool->nodes[leaf];

    if (found) {
        Cell* k = node->keys[pos];
        Cell* v = node->values[pos];
        cell_retain(k);
        cell_retain(v);
        Cell* pair = cell_cons(k, v);
        cell_release(k);
        cell_release(v);
        return pair;
    }

    /* pos is insertion point — ceiling is at pos */
    if (pos < node->n_keys) {
        Cell* k = node->keys[pos];
        Cell* v = node->values[pos];
        cell_retain(k);
        cell_retain(v);
        Cell* pair = cell_cons(k, v);
        cell_release(k);
        cell_release(v);
        return pair;
    }

    /* Need to go to next leaf */
    if (node->next_leaf != SM_NIL) {
        SMNode* next = &pool->nodes[node->next_leaf];
        if (next->n_keys > 0) {
            Cell* k = next->keys[0];
            Cell* v = next->values[0];
            cell_retain(k);
            cell_retain(v);
            Cell* pair = cell_cons(k, v);
            cell_release(k);
            cell_release(v);
            return pair;
        }
    }
    return NULL;
}
