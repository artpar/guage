#ifndef GUAGE_STR_SIMD_H
#define GUAGE_STR_SIMD_H

/*
 * SIMD String Engine — HFT-grade string search primitives
 *
 * Three-tier abstraction reusing swisstable.h platform detection:
 *   SSE2 (x86/x86_64) → NEON (ARM64) → SWAR (portable)
 *
 * Core algorithms:
 *   - find_char: broadcast + cmpeq + movemask + ctz
 *   - find_substr: StringZilla first+last char technique (1/65536 FP rate)
 *   - find_whitespace: OR of 4 cmpeq (space/tab/newline/cr)
 *
 * All functions are pure computation, zero allocation.
 */

#include "swisstable.h"  /* Platform detection: GUAGE_SIMD_SSE2/NEON/SWAR */
#include <stddef.h>

/* ===== find_char: first occurrence of byte c in s[0..len) ===== */

static inline const char* str_simd_find_char(const char* s, size_t len, char c) {
#if defined(GUAGE_SIMD_SSE2)
    __m128i needle = _mm_set1_epi8(c);
    size_t i = 0;
    for (; i + 16 <= len; i += 16) {
        __m128i chunk = _mm_loadu_si128((const __m128i*)(s + i));
        int mask = _mm_movemask_epi8(_mm_cmpeq_epi8(chunk, needle));
        if (mask) return s + i + __builtin_ctz(mask);
    }
    for (; i < len; i++) {
        if (s[i] == c) return s + i;
    }
    return NULL;
#elif defined(GUAGE_SIMD_NEON)
    uint8x16_t needle = vdupq_n_u8((uint8_t)c);
    size_t i = 0;
    for (; i + 16 <= len; i += 16) {
        uint8x16_t chunk = vld1q_u8((const uint8_t*)(s + i));
        uint8x16_t cmp = vceqq_u8(chunk, needle);
        GroupMask mask = neon_to_bitmask(cmp);
        if (mask) return s + i + __builtin_ctz(mask);
    }
    for (; i < len; i++) {
        if (s[i] == c) return s + i;
    }
    return NULL;
#else /* SWAR */
    uint64_t needle64 = 0x0101010101010101ULL * (uint8_t)c;
    size_t i = 0;
    for (; i + 8 <= len; i += 8) {
        uint64_t block;
        memcpy(&block, s + i, 8);
        uint64_t xored = block ^ needle64;
        uint64_t match = (xored - 0x0101010101010101ULL) & ~xored & 0x8080808080808080ULL;
        if (match) return s + i + (__builtin_ctzll(match) >> 3);
    }
    for (; i < len; i++) {
        if (s[i] == c) return s + i;
    }
    return NULL;
#endif
}

/* ===== rfind_char: last occurrence of byte c in s[0..len) ===== */

static inline const char* str_simd_rfind_char(const char* s, size_t len, char c) {
#if defined(GUAGE_SIMD_SSE2)
    __m128i needle = _mm_set1_epi8(c);
    /* Scan 16-byte chunks from end */
    size_t tail = len;
    while (tail >= 16) {
        tail -= 16;
        __m128i chunk = _mm_loadu_si128((const __m128i*)(s + tail));
        int mask = _mm_movemask_epi8(_mm_cmpeq_epi8(chunk, needle));
        if (mask) {
            /* Highest set bit = last match in this chunk */
            int bit = 31 - __builtin_clz(mask);
            return s + tail + bit;
        }
    }
    /* Scalar tail */
    for (size_t i = tail; i > 0; ) {
        i--;
        if (s[i] == c) return s + i;
    }
    return NULL;
#elif defined(GUAGE_SIMD_NEON)
    uint8x16_t needle = vdupq_n_u8((uint8_t)c);
    size_t tail = len;
    while (tail >= 16) {
        tail -= 16;
        uint8x16_t chunk = vld1q_u8((const uint8_t*)(s + tail));
        uint8x16_t cmp = vceqq_u8(chunk, needle);
        GroupMask mask = neon_to_bitmask(cmp);
        if (mask) {
            int bit = 31 - __builtin_clz(mask);
            return s + tail + bit;
        }
    }
    for (size_t i = tail; i > 0; ) {
        i--;
        if (s[i] == c) return s + i;
    }
    return NULL;
#else /* SWAR */
    for (size_t i = len; i > 0; ) {
        i--;
        if (s[i] == c) return s + i;
    }
    return NULL;
#endif
}

/* ===== find_substr: StringZilla first+last char broadcast technique ===== */

static inline const char* str_simd_find_substr(const char* haystack, size_t hlen,
                                                const char* needle, size_t nlen) {
    if (nlen == 0) return haystack;
    if (nlen > hlen) return NULL;
    if (nlen == 1) return str_simd_find_char(haystack, hlen, needle[0]);

    size_t scan_end = hlen - nlen;

#if defined(GUAGE_SIMD_SSE2)
    __m128i first_v = _mm_set1_epi8(needle[0]);
    __m128i last_v  = _mm_set1_epi8(needle[nlen - 1]);
    size_t i = 0;
    for (; i + 16 <= scan_end + 1; i += 16) {
        __m128i bf = _mm_loadu_si128((const __m128i*)(haystack + i));
        __m128i bl = _mm_loadu_si128((const __m128i*)(haystack + i + nlen - 1));
        int mask = _mm_movemask_epi8(_mm_and_si128(
            _mm_cmpeq_epi8(bf, first_v),
            _mm_cmpeq_epi8(bl, last_v)));
        while (mask) {
            int bit = __builtin_ctz(mask);
            if (i + bit <= scan_end) {
                if (nlen <= 2 || memcmp(haystack + i + bit + 1, needle + 1, nlen - 2) == 0)
                    return haystack + i + bit;
            }
            mask &= mask - 1;
        }
    }
    /* Scalar tail */
    for (; i <= scan_end; i++) {
        if (haystack[i] == needle[0] && haystack[i + nlen - 1] == needle[nlen - 1]) {
            if (nlen <= 2 || memcmp(haystack + i + 1, needle + 1, nlen - 2) == 0)
                return haystack + i;
        }
    }
    return NULL;
#elif defined(GUAGE_SIMD_NEON)
    uint8x16_t first_v = vdupq_n_u8((uint8_t)needle[0]);
    uint8x16_t last_v  = vdupq_n_u8((uint8_t)needle[nlen - 1]);
    size_t i = 0;
    for (; i + 16 <= scan_end + 1; i += 16) {
        uint8x16_t bf = vld1q_u8((const uint8_t*)(haystack + i));
        uint8x16_t bl = vld1q_u8((const uint8_t*)(haystack + i + nlen - 1));
        GroupMask mask = neon_to_bitmask(vandq_u8(
            vceqq_u8(bf, first_v),
            vceqq_u8(bl, last_v)));
        while (mask) {
            int bit = __builtin_ctz(mask);
            if (i + bit <= scan_end) {
                if (nlen <= 2 || memcmp(haystack + i + bit + 1, needle + 1, nlen - 2) == 0)
                    return haystack + i + bit;
            }
            mask &= mask - 1;
        }
    }
    for (; i <= scan_end; i++) {
        if (haystack[i] == needle[0] && haystack[i + nlen - 1] == needle[nlen - 1]) {
            if (nlen <= 2 || memcmp(haystack + i + 1, needle + 1, nlen - 2) == 0)
                return haystack + i;
        }
    }
    return NULL;
#else /* SWAR / scalar */
    for (size_t i = 0; i <= scan_end; i++) {
        if (haystack[i] == needle[0] && haystack[i + nlen - 1] == needle[nlen - 1]) {
            if (nlen <= 2 || memcmp(haystack + i + 1, needle + 1, nlen - 2) == 0)
                return haystack + i;
        }
    }
    return NULL;
#endif
}

/* ===== rfind_substr: last occurrence of needle in haystack ===== */

static inline const char* str_simd_rfind_substr(const char* haystack, size_t hlen,
                                                 const char* needle, size_t nlen) {
    if (nlen == 0) return haystack + hlen;
    if (nlen > hlen) return NULL;
    if (nlen == 1) return str_simd_rfind_char(haystack, hlen, needle[0]);

    /* Scan backwards — scalar is fine for reverse since it's rare */
    for (size_t i = hlen - nlen + 1; i > 0; ) {
        i--;
        if (haystack[i] == needle[0] && haystack[i + nlen - 1] == needle[nlen - 1]) {
            if (nlen <= 2 || memcmp(haystack + i + 1, needle + 1, nlen - 2) == 0)
                return haystack + i;
        }
    }
    return NULL;
}

/* ===== count_char: count occurrences of byte c ===== */

static inline size_t str_simd_count_char(const char* s, size_t len, char c) {
    size_t count = 0;
#if defined(GUAGE_SIMD_SSE2)
    __m128i needle = _mm_set1_epi8(c);
    __m128i acc = _mm_setzero_si128();
    size_t i = 0;
    int batch = 0;
    for (; i + 16 <= len; i += 16) {
        __m128i chunk = _mm_loadu_si128((const __m128i*)(s + i));
        /* -1 for match, 0 for no match; subtract to accumulate */
        acc = _mm_sub_epi8(acc, _mm_cmpeq_epi8(chunk, needle));
        if (++batch == 255) {
            /* Horizontal sum before overflow (255 * 16 max) */
            __m128i sum16 = _mm_sad_epu8(acc, _mm_setzero_si128());
            count += (size_t)_mm_extract_epi16(sum16, 0) + (size_t)_mm_extract_epi16(sum16, 4);
            acc = _mm_setzero_si128();
            batch = 0;
        }
    }
    /* Flush accumulator */
    __m128i sum16 = _mm_sad_epu8(acc, _mm_setzero_si128());
    count += (size_t)_mm_extract_epi16(sum16, 0) + (size_t)_mm_extract_epi16(sum16, 4);
    /* Scalar tail */
    for (; i < len; i++) {
        if (s[i] == c) count++;
    }
#else
    for (size_t i = 0; i < len; i++) {
        if (s[i] == c) count++;
    }
#endif
    return count;
}

/* ===== find_whitespace: first whitespace character (space/tab/newline/cr) ===== */

static inline const char* str_simd_find_whitespace(const char* s, size_t len) {
#if defined(GUAGE_SIMD_SSE2)
    __m128i sp  = _mm_set1_epi8(' ');
    __m128i tab = _mm_set1_epi8('\t');
    __m128i nl  = _mm_set1_epi8('\n');
    __m128i cr  = _mm_set1_epi8('\r');
    size_t i = 0;
    for (; i + 16 <= len; i += 16) {
        __m128i chunk = _mm_loadu_si128((const __m128i*)(s + i));
        __m128i m = _mm_or_si128(
            _mm_or_si128(_mm_cmpeq_epi8(chunk, sp), _mm_cmpeq_epi8(chunk, tab)),
            _mm_or_si128(_mm_cmpeq_epi8(chunk, nl), _mm_cmpeq_epi8(chunk, cr)));
        int mask = _mm_movemask_epi8(m);
        if (mask) return s + i + __builtin_ctz(mask);
    }
    for (; i < len; i++) {
        char c = s[i];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') return s + i;
    }
    return NULL;
#elif defined(GUAGE_SIMD_NEON)
    uint8x16_t sp  = vdupq_n_u8(' ');
    uint8x16_t tab = vdupq_n_u8('\t');
    uint8x16_t nl  = vdupq_n_u8('\n');
    uint8x16_t cr  = vdupq_n_u8('\r');
    size_t i = 0;
    for (; i + 16 <= len; i += 16) {
        uint8x16_t chunk = vld1q_u8((const uint8_t*)(s + i));
        uint8x16_t m = vorrq_u8(
            vorrq_u8(vceqq_u8(chunk, sp), vceqq_u8(chunk, tab)),
            vorrq_u8(vceqq_u8(chunk, nl), vceqq_u8(chunk, cr)));
        GroupMask mask = neon_to_bitmask(m);
        if (mask) return s + i + __builtin_ctz(mask);
    }
    for (; i < len; i++) {
        char c = s[i];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') return s + i;
    }
    return NULL;
#else /* SWAR / scalar */
    for (size_t i = 0; i < len; i++) {
        char c = s[i];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') return s + i;
    }
    return NULL;
#endif
}

/* ===== find_non_whitespace: first non-whitespace character ===== */

static inline const char* str_simd_find_non_whitespace(const char* s, size_t len) {
#if defined(GUAGE_SIMD_SSE2)
    __m128i sp  = _mm_set1_epi8(' ');
    __m128i tab = _mm_set1_epi8('\t');
    __m128i nl  = _mm_set1_epi8('\n');
    __m128i cr  = _mm_set1_epi8('\r');
    size_t i = 0;
    for (; i + 16 <= len; i += 16) {
        __m128i chunk = _mm_loadu_si128((const __m128i*)(s + i));
        __m128i ws = _mm_or_si128(
            _mm_or_si128(_mm_cmpeq_epi8(chunk, sp), _mm_cmpeq_epi8(chunk, tab)),
            _mm_or_si128(_mm_cmpeq_epi8(chunk, nl), _mm_cmpeq_epi8(chunk, cr)));
        int mask = _mm_movemask_epi8(ws);
        if (mask != 0xFFFF) {
            /* Some non-whitespace in this chunk */
            int non_ws = ~mask & 0xFFFF;
            return s + i + __builtin_ctz(non_ws);
        }
    }
    for (; i < len; i++) {
        char c = s[i];
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r') return s + i;
    }
    return NULL;
#elif defined(GUAGE_SIMD_NEON)
    uint8x16_t sp  = vdupq_n_u8(' ');
    uint8x16_t tab = vdupq_n_u8('\t');
    uint8x16_t nl  = vdupq_n_u8('\n');
    uint8x16_t cr  = vdupq_n_u8('\r');
    size_t i = 0;
    for (; i + 16 <= len; i += 16) {
        uint8x16_t chunk = vld1q_u8((const uint8_t*)(s + i));
        uint8x16_t ws = vorrq_u8(
            vorrq_u8(vceqq_u8(chunk, sp), vceqq_u8(chunk, tab)),
            vorrq_u8(vceqq_u8(chunk, nl), vceqq_u8(chunk, cr)));
        GroupMask mask = neon_to_bitmask(ws);
        if (mask != 0xFFFF) {
            int non_ws = ~mask & 0xFFFF;
            return s + i + __builtin_ctz(non_ws);
        }
    }
    for (; i < len; i++) {
        char c = s[i];
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r') return s + i;
    }
    return NULL;
#else /* SWAR / scalar */
    for (size_t i = 0; i < len; i++) {
        char c = s[i];
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r') return s + i;
    }
    return NULL;
#endif
}

/* ===== Helper: is_whitespace ===== */

static inline int str_is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

#endif /* GUAGE_STR_SIMD_H */
