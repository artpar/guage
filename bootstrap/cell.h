#ifndef GUAGE_CELL_H
#define GUAGE_CELL_H

#include <stdint.h>
#include <stdbool.h>

/* Core Cell Structure
 * Everything in Guage is either an Atom or a Cell ⟨a b⟩
 *
 * Runtime representation:
 * - Atoms: Tagged pointer (low bits indicate type)
 * - Cells: Heap-allocated pair with metadata
 */

/* Cell Types */
typedef enum {
    CELL_ATOM_NUMBER,    /* #n - number */
    CELL_ATOM_BOOL,      /* #t/#f - boolean */
    CELL_ATOM_SYMBOL,    /* :symbol */
    CELL_ATOM_NIL,       /* ∅ - nil */
    CELL_ATOM_STRING,    /* "string" - string literal */
    CELL_PAIR,           /* ⟨a b⟩ - cons cell */
    CELL_LAMBDA,         /* λ - closure */
    CELL_BUILTIN,        /* Built-in primitive */
    CELL_ERROR,          /* ⚠ - error value */
    CELL_STRUCT,         /* ⊙/⊚ - user-defined structure */
    CELL_GRAPH,          /* ⊝ - graph structure (CFG/DFG/etc) */
    CELL_ACTOR,          /* ⟳ - actor (fiber + mailbox) */
    CELL_CHANNEL,        /* ⟿ - channel (typed buffer) */
    CELL_BOX,            /* □ - mutable reference */
    CELL_WEAK_REF,       /* ◇ - weak reference */
    CELL_HASHMAP         /* ⊞ - hash map (Swiss Table) */
} CellType;

/* Linear Type Flags */
typedef enum {
    LINEAR_NONE      = 0,      /* Normal value */
    LINEAR_UNIQUE    = 1 << 0, /* Must be consumed exactly once */
    LINEAR_BORROWED  = 1 << 1, /* Temporary borrow */
    LINEAR_CONSUMED  = 1 << 2  /* Already consumed (error to use) */
} LinearFlags;

/* Capability Flags (compile-time mainly, but tracked at runtime for debugging) */
typedef enum {
    CAP_NONE    = 0,
    CAP_READ    = 1 << 0,
    CAP_WRITE   = 1 << 1,
    CAP_EXECUTE = 1 << 2,
    CAP_SEND    = 1 << 3,  /* Can send across actors */
    CAP_SHARE   = 1 << 4   /* Can share between threads */
} CapabilityFlags;

/* Structure Kinds */
typedef enum {
    STRUCT_LEAF,    /* ⊙ - Simple data (non-recursive) */
    STRUCT_NODE,    /* ⊚ - Recursive data (ADT with variants) */
    STRUCT_GRAPH    /* ⊝ - Graph data (specialized) */
} StructKind;

/* Graph Types */
typedef enum {
    GRAPH_GENERIC,   /* User-defined graph */
    GRAPH_CFG,       /* ⌂⟿ - Control Flow Graph */
    GRAPH_DFG,       /* ⌂⇝ - Data Flow Graph */
    GRAPH_CALL,      /* ⌂⊚ - Call Graph */
    GRAPH_DEP        /* ⌂⊙ - Dependency Graph */
} GraphType;

/* Forward declaration */
typedef struct Cell Cell;

/* Hash map slot (key-value pair) */
typedef struct {
    Cell* key;
    Cell* value;
} HashSlot;

/* Atom data (stored inline in Cell) */
typedef union {
    double number;
    bool boolean;
    const char* symbol;
    const char* string;  /* Immutable string (strdup'd) */
    void* builtin;  /* Pointer to builtin function */
} AtomData;

/* Cell structure */
struct Cell {
    CellType type;

    /* Reference counting for GC */
    uint32_t refcount;

    /* Weak reference counting (zombie support) */
    uint16_t weak_refcount;

    /* Linear type tracking */
    LinearFlags linear_flags;

    /* Capability flags */
    CapabilityFlags caps;

    /* GC mark bit */
    bool marked;

    /* Data */
    union {
        AtomData atom;
        struct {
            Cell* car;  /* Head (◁) */
            Cell* cdr;  /* Tail (▷) */
        } pair;
        struct {
            Cell* env;     /* Lexical environment */
            Cell* body;    /* Lambda body */
            int arity;     /* Number of parameters */
            const char* source_module;  /* Module/file where defined - Day 27 */
            int source_line;            /* Line number in source - Day 27 */
        } lambda;
        struct {
            const char* message;  /* Error message */
            Cell* data;           /* Associated data */
        } error;
        struct {
            StructKind kind;      /* LEAF, NODE, or GRAPH */
            Cell* type_tag;       /* :Point, :List, :Tree, etc */
            Cell* variant;        /* :Nil, :Cons, etc (for ADTs) or NULL */
            Cell* fields;         /* Alist of (field . value) pairs */
        } structure;
        struct {
            GraphType graph_type; /* CFG, DFG, CALL, DEP, or GENERIC */
            Cell* nodes;          /* List of node cells */
            Cell* edges;          /* List of edge cells ⟨from to label⟩ */
            Cell* metadata;       /* Additional properties (alist) */
            Cell* entry;          /* Entry point (for CFG) or NULL */
            Cell* exit;           /* Exit point (for CFG) or NULL */
        } graph;
        struct {
            int actor_id;         /* Actor registry ID */
        } actor;
        struct {
            int channel_id;       /* Channel registry ID */
        } channel;
        struct {
            Cell* value;          /* Mutable box contents */
        } box;
        struct {
            Cell* target;         /* Weak reference target (direct pointer) */
        } weak_ref;
        struct {
            uint8_t* ctrl;        /* Control byte array [capacity + GROUP_WIDTH] (mirrored) */
            HashSlot* slots;      /* Parallel slot array [capacity] */
            uint32_t size;        /* Live entries */
            uint32_t capacity;    /* Total slots (power of 2, min GROUP_WIDTH) */
            uint32_t growth_left; /* Slots remaining before resize */
        } hashmap;
    } data;
};

/* Cell creation functions */
Cell* cell_number(double n);
Cell* cell_bool(bool b);
Cell* cell_symbol(const char* sym);
Cell* cell_string(const char* str);
Cell* cell_nil(void);
Cell* cell_cons(Cell* car, Cell* cdr);
Cell* cell_lambda(Cell* env, Cell* body, int arity, const char* source_module, int source_line);
Cell* cell_builtin(void* fn);
Cell* cell_error(const char* message, Cell* data);
Cell* cell_struct(StructKind kind, Cell* type_tag, Cell* variant, Cell* fields);
Cell* cell_graph(GraphType graph_type, Cell* nodes, Cell* edges, Cell* metadata);
Cell* cell_actor(int actor_id);
Cell* cell_channel(int channel_id);
Cell* cell_box(Cell* value);

/* Cell accessors */
double cell_get_number(Cell* c);
bool cell_get_bool(Cell* c);
const char* cell_get_symbol(Cell* c);
const char* cell_get_string(Cell* c);
Cell* cell_car(Cell* c);  /* ◁ - head */
Cell* cell_cdr(Cell* c);  /* ▷ - tail */

/* Reference counting */
void cell_retain(Cell* c);
void cell_release(Cell* c);

/* Linear type operations */
bool cell_is_linear(Cell* c);
bool cell_is_consumed(Cell* c);
void cell_mark_consumed(Cell* c);
Cell* cell_move(Cell* c);  /* Transfer ownership */
Cell* cell_borrow(Cell* c); /* Temporary borrow */

/* Type predicates */
bool cell_is_number(Cell* c);
bool cell_is_bool(Cell* c);
bool cell_is_symbol(Cell* c);
bool cell_is_string(Cell* c);
bool cell_is_nil(Cell* c);
bool cell_is_pair(Cell* c);
bool cell_is_lambda(Cell* c);
bool cell_is_atom(Cell* c);
bool cell_is_error(Cell* c);
bool cell_is_struct(Cell* c);
bool cell_is_graph(Cell* c);
bool cell_is_actor(Cell* c);
int  cell_get_actor_id(Cell* c);
bool cell_is_channel(Cell* c);
int  cell_get_channel_id(Cell* c);
bool cell_is_box(Cell* c);
Cell* cell_box_get(Cell* c);
Cell* cell_box_set(Cell* c, Cell* new_value);

/* Weak reference operations */
Cell* cell_weak_ref(Cell* target);
bool cell_is_weak_ref(Cell* c);
Cell* cell_get_weak_target(Cell* c);
void cell_weak_retain(Cell* c);
void cell_weak_release(Cell* c);

/* HashMap operations */
Cell* cell_hashmap_new(uint32_t initial_capacity);
bool cell_is_hashmap(Cell* c);
uint64_t cell_hash(Cell* c);
Cell* cell_hashmap_get(Cell* map, Cell* key);
Cell* cell_hashmap_put(Cell* map, Cell* key, Cell* value);
Cell* cell_hashmap_delete(Cell* map, Cell* key);
bool cell_hashmap_has(Cell* map, Cell* key);
uint32_t cell_hashmap_size(Cell* map);
Cell* cell_hashmap_keys(Cell* map);
Cell* cell_hashmap_values(Cell* map);
Cell* cell_hashmap_entries(Cell* map);
Cell* cell_hashmap_merge(Cell* m1, Cell* m2);

/* Error accessors */
const char* cell_error_message(Cell* c);
Cell* cell_error_data(Cell* c);

/* Equality */
bool cell_equal(Cell* a, Cell* b);

/* Structure accessors */
StructKind cell_struct_kind(Cell* c);
Cell* cell_struct_type_tag(Cell* c);
Cell* cell_struct_variant(Cell* c);
Cell* cell_struct_fields(Cell* c);
Cell* cell_struct_get_field(Cell* c, Cell* field_name);

/* Graph accessors */
GraphType cell_graph_type(Cell* c);
Cell* cell_graph_nodes(Cell* c);
Cell* cell_graph_edges(Cell* c);
Cell* cell_graph_metadata(Cell* c);
Cell* cell_graph_entry(Cell* c);
Cell* cell_graph_exit(Cell* c);

/* Graph mutators (return new graph) */
Cell* cell_graph_add_node(Cell* graph, Cell* node);
Cell* cell_graph_add_edge(Cell* graph, Cell* from, Cell* to, Cell* label);
Cell* cell_graph_set_entry(Cell* graph, Cell* entry);
Cell* cell_graph_set_exit(Cell* graph, Cell* exit);

/* Printing */
void cell_print(Cell* c);
void cell_println(Cell* c);

#endif /* GUAGE_CELL_H */
