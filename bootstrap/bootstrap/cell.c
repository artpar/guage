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

Cell* cell_lambda(Cell* env, Cell* body, int arity) {
    Cell* c = cell_alloc(CELL_LAMBDA);
    c->data.lambda.env = env;
    c->data.lambda.body = body;
    c->data.lambda.arity = arity;

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
                break;
            case CELL_ATOM_SYMBOL:
                free((void*)c->data.atom.symbol);
                break;
            case CELL_ERROR:
                free((void*)c->data.error.message);
                cell_release(c->data.error.data);
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
                 c->type == CELL_ATOM_NIL);
}

bool cell_is_error(Cell* c) {
    return c && c->type == CELL_ERROR;
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
        case CELL_ATOM_NIL:
            return true;
        case CELL_PAIR:
            return cell_equal(a->data.pair.car, b->data.pair.car) &&
                   cell_equal(a->data.pair.cdr, b->data.pair.cdr);
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
    }
}

void cell_println(Cell* c) {
    cell_print(c);
    printf("\n");
}
