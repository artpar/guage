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
    CELL_PAIR,           /* ⟨a b⟩ - cons cell */
    CELL_LAMBDA,         /* λ - closure */
    CELL_BUILTIN,        /* Built-in primitive */
    CELL_ERROR           /* ⚠ - error value */
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

/* Forward declaration */
typedef struct Cell Cell;

/* Atom data (stored inline in Cell) */
typedef union {
    double number;
    bool boolean;
    const char* symbol;
    void* builtin;  /* Pointer to builtin function */
} AtomData;

/* Cell structure */
struct Cell {
    CellType type;

    /* Reference counting for GC */
    uint32_t refcount;

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
        } lambda;
        struct {
            const char* message;  /* Error message */
            Cell* data;           /* Associated data */
        } error;
    } data;
};

/* Cell creation functions */
Cell* cell_number(double n);
Cell* cell_bool(bool b);
Cell* cell_symbol(const char* sym);
Cell* cell_nil(void);
Cell* cell_cons(Cell* car, Cell* cdr);
Cell* cell_lambda(Cell* env, Cell* body, int arity);
Cell* cell_builtin(void* fn);
Cell* cell_error(const char* message, Cell* data);

/* Cell accessors */
double cell_get_number(Cell* c);
bool cell_get_bool(Cell* c);
const char* cell_get_symbol(Cell* c);
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
bool cell_is_nil(Cell* c);
bool cell_is_pair(Cell* c);
bool cell_is_lambda(Cell* c);
bool cell_is_atom(Cell* c);
bool cell_is_error(Cell* c);

/* Error accessors */
const char* cell_error_message(Cell* c);
Cell* cell_error_data(Cell* c);

/* Equality */
bool cell_equal(Cell* a, Cell* b);

/* Printing */
void cell_print(Cell* c);
void cell_println(Cell* c);

#endif /* GUAGE_CELL_H */
