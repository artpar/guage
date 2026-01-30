#ifndef GUAGE_SWISSTABLE_H
#define GUAGE_SWISSTABLE_H

/*
 * Swiss Table — portable SIMD hash table core
 * Three-tier abstraction: SSE2 → NEON → SWAR
 *
 * Control byte encoding:
 *   0xFF = EMPTY (slot never used)
 *   0x80 = DELETED (tombstone)
 *   0b0xxxxxxx = FULL (top bit clear, low 7 bits = H2 hash fragment)
 */

#include <stdint.h>
#include <string.h>

/* Control byte constants */
#define CTRL_EMPTY   ((uint8_t)0xFF)
#define CTRL_DELETED ((uint8_t)0x80)

/* Hash splitting */
#define H2(hash) ((uint8_t)((hash) >> 57))
#define H1(hash) ((hash))

/* === Platform detection === */

#if defined(__SSE2__) || defined(_M_X64) || defined(_M_AMD64)
  #define GUAGE_SIMD_SSE2 1
  #define GROUP_WIDTH 16
  #include <immintrin.h>
#elif defined(__aarch64__) || defined(_M_ARM64)
  #define GUAGE_SIMD_NEON 1
  #define GROUP_WIDTH 16
  #include <arm_neon.h>
#else
  #define GUAGE_SIMD_SWAR 1
  #define GROUP_WIDTH 8
#endif

/* === Bitmask iteration === */

typedef uint32_t GroupMask;

static inline int guage_bitmask_next(GroupMask* mask) {
    if (*mask == 0) return -1;
    int idx = __builtin_ctz(*mask);
    *mask &= *mask - 1;  /* Clear lowest set bit */
    return idx;
}

/* === SSE2 Implementation (x86/x86_64) === */

#if defined(GUAGE_SIMD_SSE2)

static inline GroupMask guage_group_match(const uint8_t* ctrl, uint8_t h2) {
    __m128i group = _mm_loadu_si128((const __m128i*)ctrl);
    __m128i needle = _mm_set1_epi8((char)h2);
    return (GroupMask)_mm_movemask_epi8(_mm_cmpeq_epi8(group, needle));
}

static inline GroupMask guage_group_match_empty(const uint8_t* ctrl) {
    __m128i group = _mm_loadu_si128((const __m128i*)ctrl);
    __m128i empty = _mm_set1_epi8((char)CTRL_EMPTY);
    return (GroupMask)_mm_movemask_epi8(_mm_cmpeq_epi8(group, empty));
}

static inline GroupMask guage_group_match_empty_or_deleted(const uint8_t* ctrl) {
    __m128i group = _mm_loadu_si128((const __m128i*)ctrl);
    /* EMPTY=0xFF and DELETED=0x80 both have top bit set */
    return (GroupMask)_mm_movemask_epi8(group);
}

/* === NEON Implementation (ARM64/AArch64) === */

#elif defined(GUAGE_SIMD_NEON)

static inline GroupMask neon_to_bitmask(uint8x16_t cmp) {
    /* Convert 16-byte comparison result to 16-bit bitmask */
    static const uint8_t shift_vals[16] = {1,2,4,8,16,32,64,128,1,2,4,8,16,32,64,128};
    uint8x16_t shift = vld1q_u8(shift_vals);
    uint8x16_t masked = vandq_u8(cmp, shift);
    uint8x8_t lo = vget_low_u8(masked);
    uint8x8_t hi = vget_high_u8(masked);
    lo = vpadd_u8(lo, lo);
    lo = vpadd_u8(lo, lo);
    lo = vpadd_u8(lo, lo);
    hi = vpadd_u8(hi, hi);
    hi = vpadd_u8(hi, hi);
    hi = vpadd_u8(hi, hi);
    return (GroupMask)vget_lane_u8(lo, 0) | ((GroupMask)vget_lane_u8(hi, 0) << 8);
}

static inline GroupMask guage_group_match(const uint8_t* ctrl, uint8_t h2) {
    uint8x16_t group = vld1q_u8(ctrl);
    uint8x16_t needle = vdupq_n_u8(h2);
    return neon_to_bitmask(vceqq_u8(group, needle));
}

static inline GroupMask guage_group_match_empty(const uint8_t* ctrl) {
    uint8x16_t group = vld1q_u8(ctrl);
    uint8x16_t empty = vdupq_n_u8(CTRL_EMPTY);
    return neon_to_bitmask(vceqq_u8(group, empty));
}

static inline GroupMask guage_group_match_empty_or_deleted(const uint8_t* ctrl) {
    uint8x16_t group = vld1q_u8(ctrl);
    /* Top bit set = EMPTY or DELETED (both >= 0x80) */
    uint8x16_t threshold = vdupq_n_u8(0x80);
    return neon_to_bitmask(vcgeq_u8(group, threshold));
}

/* === SWAR Implementation (Portable — all architectures) === */

#elif defined(GUAGE_SIMD_SWAR)

/* For SWAR, we use 64-bit operations on 8 bytes at a time.
 * The bitmask encodes matches as set high-bits per byte,
 * so we extract byte indices via ctz/8. */

static inline uint64_t swar_load(const uint8_t* ctrl) {
    uint64_t group;
    memcpy(&group, ctrl, 8);
    return group;
}

static inline uint64_t swar_match_raw(uint64_t group, uint8_t needle_byte) {
    uint64_t needle = 0x0101010101010101ULL * needle_byte;
    uint64_t xored = group ^ needle;
    /* Zero-byte detection: (v - 0x01..01) & ~v & 0x80..80 */
    return (xored - 0x0101010101010101ULL) & ~xored & 0x8080808080808080ULL;
}

/* SWAR returns a 64-bit mask with high bits set at matching byte positions.
 * We convert to GroupMask (one bit per slot) for unified API. */
static inline GroupMask swar_to_bitmask(uint64_t raw) {
    /* Each matching byte has bit 7 set. Shift down and pack. */
    GroupMask result = 0;
    for (int i = 0; i < 8; i++) {
        if (raw & (0x80ULL << (i * 8)))
            result |= (1u << i);
    }
    return result;
}

static inline GroupMask guage_group_match(const uint8_t* ctrl, uint8_t h2) {
    return swar_to_bitmask(swar_match_raw(swar_load(ctrl), h2));
}

static inline GroupMask guage_group_match_empty(const uint8_t* ctrl) {
    return swar_to_bitmask(swar_match_raw(swar_load(ctrl), CTRL_EMPTY));
}

static inline GroupMask guage_group_match_empty_or_deleted(const uint8_t* ctrl) {
    uint64_t group = swar_load(ctrl);
    /* EMPTY(0xFF) and DELETED(0x80) both have top bit set */
    uint64_t top_bits = group & 0x8080808080808080ULL;
    return swar_to_bitmask(top_bits);
}

#endif /* SIMD selection */

#endif /* GUAGE_SWISSTABLE_H */
