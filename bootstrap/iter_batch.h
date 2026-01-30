#ifndef GUAGE_ITER_BATCH_H
#define GUAGE_ITER_BATCH_H

/*
 * iter_batch.h — Morsel-Driven Batch Iterator Types (Day 118)
 *
 * Core data structures for the iterator protocol (⊣).
 * Batch-at-a-time with selection vectors, inspired by
 * DuckDB/Velox vectorized execution engines.
 *
 * 256-element batches = 2KB of Cell* pointers → fits L1 cache.
 */

#include "cell.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* Batch capacity: 256 × 8 = 2KB pointers → fits L1 cache */
#define ITER_BATCH_CAP 256

/* IterBatch — hot path structure for batch iteration */
typedef struct {
    Cell*    elems[ITER_BATCH_CAP];  /* Batch elements */
    uint8_t  sel[ITER_BATCH_CAP];    /* Selection vector (indices into elems[]) */
    uint16_t count;       /* Elements in elems[] */
    uint16_t sel_count;   /* Valid entries in sel[] */
    uint16_t cursor;      /* Read position for single-element API */
    bool     use_sel;     /* false = all elements valid (skip sel indirection) */
} IterBatch;

/* Iterator kinds */
typedef enum {
    /* Source iterators */
    ITER_LIST,
    ITER_HASHMAP,
    ITER_HASHSET,
    ITER_DEQUE,
    ITER_VECTOR,
    ITER_HEAP,
    ITER_SORTED_MAP,
    ITER_TRIE,
    ITER_BUFFER,
    ITER_GRAPH,
    /* Transformer iterators */
    ITER_MAP,
    ITER_FILTER,
    ITER_TAKE,
    ITER_DROP,
    ITER_CHAIN,
    ITER_ZIP
} IterKind;

/* Forward declare: batch fill function pointer (the vtable) */
typedef uint16_t (*iter_fill_fn)(Cell* it, IterBatch* batch);

/* IteratorData — per-iterator state */
typedef struct {
    IterKind     kind;
    Cell*        source;       /* Retained collection (or upstream iterator) */
    iter_fill_fn fill;         /* Batch fill function pointer */
    IterBatch    batch;        /* Current batch (embedded) */
    bool         exhausted;

    union {
        /* --- Source states --- */
        struct { Cell* current; } list;
        struct { uint32_t slot_idx; } hashmap;
        struct { uint32_t group; uint8_t slot; } hashset;
        struct { uint32_t vindex; } deque;
        struct { uint32_t index; } vector;
        struct {
            double*  aux_keys;
            uint32_t* aux_idx;
            uint32_t aux_size;
            uint32_t aux_cap;
        } heap;
        struct {
            void* pool;
            uint32_t leaf_idx;
            uint8_t  key_idx;
        } sorted_map;
        struct {
            void*    stack[32];
            uint8_t  child_idx[32];
            uint8_t  prefix[256];
            uint16_t prefix_len;
            int8_t   depth;
        } trie;
        struct { uint32_t byte_idx; } buffer;
        struct { Cell* remaining; } graph;

        /* --- Transformer states --- */
        struct { Cell* upstream; Cell* fn; } map;
        struct { Cell* upstream; Cell* pred; } filter;
        struct { Cell* upstream; uint32_t remaining; } take;
        struct { Cell* upstream; } drop;
        struct { Cell* first; Cell* second; bool on_second; } chain;
        struct { Cell* left; Cell* right; } zip;
    } state;
} IteratorData;

#endif /* GUAGE_ITER_BATCH_H */
