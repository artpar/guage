#ifndef GUAGE_STRTABLE_H
#define GUAGE_STRTABLE_H

/*
 * StrTable — C-level string→void* hash table
 *
 * Reuses swisstable.h SIMD group matching + siphash.h keyed PRF.
 * Open-addressing, linear probing by group. No locking (bootstrap is
 * single-threaded). Resize at 87.5% load (7/8).
 *
 * Follows intern.c pattern but stores arbitrary pointers.
 */

#include "siphash.h"
#include "swisstable.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct {
    uint64_t  hash;    /* SipHash-2-4 of key */
    char*     key;     /* strdup'd key (owned) */
    void*     value;   /* user pointer (not owned) */
} StrTableSlot;

typedef struct {
    StrTableSlot* slots;
    uint8_t*      ctrl;     /* control bytes: CTRL_EMPTY / CTRL_DELETED / H2 */
    uint32_t      size;
    uint32_t      cap;      /* always power-of-2, multiple of GROUP_WIDTH */
    uint32_t      mask;     /* cap - 1 */
} StrTable;

/* Round up to next multiple of GROUP_WIDTH that is a power of 2 */
static inline uint32_t strtable_round_cap(uint32_t c) {
    if (c < (uint32_t)GROUP_WIDTH) c = (uint32_t)GROUP_WIDTH;
    /* next power of 2 */
    c--;
    c |= c >> 1; c |= c >> 2; c |= c >> 4;
    c |= c >> 8; c |= c >> 16;
    c++;
    return c;
}

static inline void strtable_init(StrTable* t, uint32_t cap) {
    t->cap  = strtable_round_cap(cap);
    t->mask = t->cap - 1;
    t->size = 0;
    t->slots = (StrTableSlot*)calloc(t->cap, sizeof(StrTableSlot));
    t->ctrl  = (uint8_t*)malloc(t->cap + GROUP_WIDTH); /* extra group for wrap */
    memset(t->ctrl, CTRL_EMPTY, t->cap + GROUP_WIDTH);
}

/* Internal probe — returns slot index or -1 */
static inline int32_t strtable_find(const StrTable* t, const char* key, uint64_t h) {
    uint8_t h2 = H2(h);
    uint32_t pos = (uint32_t)(H1(h) & t->mask);

    for (;;) {
        /* Align pos to group boundary for SIMD load */
        uint32_t group_start = pos & ~((uint32_t)GROUP_WIDTH - 1);
        GroupMask match = guage_group_match(t->ctrl + group_start, h2);
        while (match) {
            int bit = guage_bitmask_next(&match);
            uint32_t idx = (group_start + (uint32_t)bit) & t->mask;
            if (t->slots[idx].hash == h && strcmp(t->slots[idx].key, key) == 0) {
                return (int32_t)idx;
            }
        }
        /* If group has empties, key is absent */
        GroupMask empty = guage_group_match_empty(t->ctrl + group_start);
        if (empty) return -1;
        pos = (group_start + GROUP_WIDTH) & t->mask;
    }
}

/* Internal: find empty slot for insertion */
static inline uint32_t strtable_find_insert(const StrTable* t, uint64_t h) {
    uint32_t pos = (uint32_t)(H1(h) & t->mask);
    for (;;) {
        uint32_t group_start = pos & ~((uint32_t)GROUP_WIDTH - 1);
        GroupMask empty = guage_group_match_empty_or_deleted(t->ctrl + group_start);
        if (empty) {
            int bit = guage_bitmask_next(&empty);
            return (group_start + (uint32_t)bit) & t->mask;
        }
        pos = (group_start + GROUP_WIDTH) & t->mask;
    }
}

static void strtable_resize(StrTable* t) {
    uint32_t old_cap = t->cap;
    StrTableSlot* old_slots = t->slots;
    uint8_t* old_ctrl = t->ctrl;

    t->cap  = old_cap * 2;
    t->mask = t->cap - 1;
    t->slots = (StrTableSlot*)calloc(t->cap, sizeof(StrTableSlot));
    t->ctrl  = (uint8_t*)malloc(t->cap + GROUP_WIDTH);
    memset(t->ctrl, CTRL_EMPTY, t->cap + GROUP_WIDTH);

    for (uint32_t i = 0; i < old_cap; i++) {
        if (old_ctrl[i] != CTRL_EMPTY && old_ctrl[i] != CTRL_DELETED) {
            uint32_t idx = strtable_find_insert(t, old_slots[i].hash);
            t->slots[idx] = old_slots[i];
            t->ctrl[idx] = H2(old_slots[i].hash);
            /* Mirror to overflow zone */
            if (idx < (uint32_t)GROUP_WIDTH)
                t->ctrl[t->cap + idx] = t->ctrl[idx];
        }
    }

    free(old_slots);
    free(old_ctrl);
}

/* Get value by key. Returns NULL if absent. */
static inline void* strtable_get(const StrTable* t, const char* key) {
    if (t->size == 0) return NULL;
    uint64_t h = guage_siphash(key, strlen(key));
    int32_t idx = strtable_find(t, key, h);
    return idx >= 0 ? t->slots[idx].value : NULL;
}

/* Put key→value. Returns old value (or NULL). Key is strdup'd on insert. */
static inline void* strtable_put(StrTable* t, const char* key, void* val) {
    uint64_t h = guage_siphash(key, strlen(key));

    /* Check for existing key */
    int32_t idx = strtable_find(t, key, h);
    if (idx >= 0) {
        void* old = t->slots[idx].value;
        t->slots[idx].value = val;
        return old;
    }

    /* Resize at 87.5% load */
    if (t->size * 8 >= t->cap * 7) {
        strtable_resize(t);
    }

    uint32_t ins = strtable_find_insert(t, h);
    t->slots[ins].hash  = h;
    t->slots[ins].key   = strdup(key);
    t->slots[ins].value = val;
    t->ctrl[ins] = H2(h);
    /* Mirror to overflow zone */
    if (ins < (uint32_t)GROUP_WIDTH)
        t->ctrl[t->cap + ins] = t->ctrl[ins];
    t->size++;
    return NULL;
}

/* Delete key. Returns old value (or NULL). Frees the key copy. */
static inline void* strtable_del(StrTable* t, const char* key) {
    if (t->size == 0) return NULL;
    uint64_t h = guage_siphash(key, strlen(key));
    int32_t idx = strtable_find(t, key, h);
    if (idx < 0) return NULL;

    void* old = t->slots[idx].value;
    free(t->slots[idx].key);
    t->slots[idx].key = NULL;
    t->slots[idx].value = NULL;
    t->ctrl[idx] = CTRL_DELETED;
    if ((uint32_t)idx < (uint32_t)GROUP_WIDTH)
        t->ctrl[t->cap + (uint32_t)idx] = CTRL_DELETED;
    t->size--;
    return old;
}

/* Free all entries. Calls free_val(value) on each if non-NULL. */
static inline void strtable_free(StrTable* t, void (*free_val)(void*)) {
    if (!t->slots) return;
    for (uint32_t i = 0; i < t->cap; i++) {
        if (t->ctrl[i] != CTRL_EMPTY && t->ctrl[i] != CTRL_DELETED) {
            if (free_val && t->slots[i].value)
                free_val(t->slots[i].value);
            free(t->slots[i].key);
        }
    }
    free(t->slots);
    free(t->ctrl);
    t->slots = NULL;
    t->ctrl = NULL;
    t->size = 0;
    t->cap = 0;
}

#endif /* GUAGE_STRTABLE_H */
