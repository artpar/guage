#ifndef GUAGE_BTREE_SIMD_H
#define GUAGE_BTREE_SIMD_H

/*
 * SIMD B-Tree rank function — portable 3-tier implementation
 * Computes: number of sort_keys[] < query (the insertion/search index)
 *
 * Tiers: SSE2/SSE4.2 → NEON → SWAR (scalar branchless)
 * Reuses platform detection from swisstable.h
 *
 * Sort-key extraction: Cell* → uint64_t order-preserving key
 * IEEE 754 XOR sign-flip trick for doubles
 */

#include <stdint.h>
#include <string.h>

/* === Platform detection (reuses swisstable.h pattern) === */

#if defined(__SSE2__) || defined(_M_X64) || defined(_M_AMD64)
  #define GUAGE_BTREE_SSE2 1
  #include <immintrin.h>
  #if defined(__SSE4_2__) || defined(__AVX2__)
    #define GUAGE_BTREE_SSE42 1
  #endif
  #if defined(__AVX2__)
    #define GUAGE_BTREE_AVX2 1
  #endif
#elif defined(__aarch64__) || defined(_M_ARM64)
  #define GUAGE_BTREE_NEON 1
  #include <arm_neon.h>
#else
  #define GUAGE_BTREE_SWAR 1
#endif

/* B-tree branching factor: 16 keys per node */
#define BTREE_B 16

/* === IEEE 754 double → order-preserving uint64_t === */

/*
 * XOR sign-flip trick (Lemire):
 * Positive doubles already sort correctly as uint64_t.
 * Negative doubles need all bits flipped.
 * This single branchless operation handles both cases.
 */
static inline uint64_t double_to_sortkey(double d) {
    uint64_t bits;
    memcpy(&bits, &d, 8);
    uint64_t mask = -(bits >> 63) | 0x8000000000000000ULL;
    return bits ^ mask;
}

/* === SIMD Rank: count sort_keys < query === */

#if defined(GUAGE_BTREE_AVX2)

/*
 * AVX2: unsigned 64-bit comparison via sign-flip (XOR 0x80...)
 * _mm256_cmpgt_epi64 is signed, so bias both operands.
 */
static inline unsigned sm_rank16(uint64_t query, const uint64_t* sort_keys) {
    __m256i bias = _mm256_set1_epi64x((long long)0x8000000000000000ULL);
    __m256i q = _mm256_xor_si256(_mm256_set1_epi64x((long long)query), bias);
    unsigned rank = 0;
    for (int i = 0; i < 16; i += 4) {
        __m256i v = _mm256_xor_si256(_mm256_load_si256((const __m256i*)(sort_keys + i)), bias);
        __m256i cmp = _mm256_cmpgt_epi64(q, v);
        int64_t* r = (int64_t*)&cmp;
        for (int j = 0; j < 4; j++) rank += (r[j] != 0);
    }
    return rank;
}

#elif defined(GUAGE_BTREE_SSE42)

/*
 * SSE4.2: unsigned 64-bit comparison via sign-flip (XOR 0x80...)
 */
static inline unsigned sm_rank16(uint64_t query, const uint64_t* sort_keys) {
    __m128i bias = _mm_set1_epi64x((long long)0x8000000000000000ULL);
    __m128i q = _mm_xor_si128(_mm_set1_epi64x((long long)query), bias);
    unsigned rank = 0;
    for (int i = 0; i < 16; i += 2) {
        __m128i v = _mm_xor_si128(_mm_load_si128((const __m128i*)(sort_keys + i)), bias);
        __m128i cmp = _mm_cmpgt_epi64(q, v);
        rank += (unsigned)((_mm_movemask_epi8(cmp) & 0x01) != 0);
        rank += (unsigned)((_mm_movemask_epi8(cmp) & 0x100) != 0);
    }
    return rank;
}

#elif defined(GUAGE_BTREE_SSE2)

/*
 * SSE2: no _mm_cmpgt_epi64, fall back to scalar with SIMD loads
 */
static inline unsigned sm_rank16(uint64_t query, const uint64_t* sort_keys) {
    unsigned rank = 0;
    for (int i = 0; i < 16; i++)
        rank += (sort_keys[i] < query);
    return rank;
}

#elif defined(GUAGE_BTREE_NEON)

/*
 * NEON: unsigned 64-bit comparison via sign-flip trick
 * vcgtq_s64 does signed comparison, so XOR both operands with 0x80...
 * to convert unsigned ordering to signed ordering.
 */
static inline unsigned sm_rank16(uint64_t query, const uint64_t* sort_keys) {
    /* Bias for unsigned→signed conversion */
    int64x2_t bias = vdupq_n_s64((int64_t)0x8000000000000000ULL);
    int64x2_t q = veorq_s64(vdupq_n_s64((int64_t)query), bias);
    unsigned rank = 0;
    for (int i = 0; i < 16; i += 2) {
        int64x2_t v = veorq_s64(vld1q_s64((const int64_t*)(sort_keys + i)), bias);
        uint64x2_t cmp = vcgtq_s64(q, v);
        rank += (unsigned)(vgetq_lane_u64(cmp, 0) != 0);
        rank += (unsigned)(vgetq_lane_u64(cmp, 1) != 0);
    }
    return rank;
}

#else /* GUAGE_BTREE_SWAR — portable scalar */

/*
 * SWAR: branchless scalar loop
 * Compiler will likely auto-vectorize this
 */
static inline unsigned sm_rank16(uint64_t query, const uint64_t* sort_keys) {
    unsigned rank = 0;
    for (int i = 0; i < 16; i++)
        rank += (sort_keys[i] < query);
    return rank;
}

#endif /* SIMD selection */

#endif /* GUAGE_BTREE_SIMD_H */
