#ifndef GUAGE_ART_SIMD_H
#define GUAGE_ART_SIMD_H

/*
 * ART (Adaptive Radix Tree) — SIMD Node16 search
 * Reuses swisstable.h SIMD infrastructure (SSE2/NEON/SWAR)
 *
 * Node16 stores up to 16 sorted key bytes.
 * SIMD parallel comparison finds matching byte in one instruction.
 */

#include "swisstable.h"

/* Find byte in sorted Node16 keys array.
 * Returns index [0..num_children-1] or -1 if not found.
 * Uses guage_group_match from swisstable.h for SIMD parallel comparison. */
static inline int art_node16_find(const uint8_t* keys, uint8_t num_children, uint8_t byte) {
    GroupMask mask = guage_group_match(keys, byte);
    /* Mask out invalid positions beyond num_children */
    mask &= (1u << num_children) - 1;
    if (mask == 0) return -1;
    return __builtin_ctz(mask);
}

/* Find insertion point in sorted Node16 keys.
 * Returns index where byte should be inserted to maintain sort order. */
static inline int art_node16_lower_bound(const uint8_t* keys, uint8_t num_children, uint8_t byte) {
    int lo = 0, hi = (int)num_children;
    while (lo < hi) {
        int mid = (lo + hi) >> 1;
        if (keys[mid] < byte) lo = mid + 1;
        else hi = mid;
    }
    return lo;
}

#endif /* GUAGE_ART_SIMD_H */
