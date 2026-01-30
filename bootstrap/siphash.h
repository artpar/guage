#ifndef GUAGE_SIPHASH_H
#define GUAGE_SIPHASH_H

/*
 * SipHash-2-4 — keyed PRF for hash table security
 * Based on csiphash by Marek Majkowski (MIT license)
 * https://github.com/majek/csiphash
 *
 * 128-bit key, 2 compression rounds, 4 finalization rounds, 64-bit output
 * Used by Python 3.4+, Rust, Ruby, Linux kernel, Redis, WireGuard
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Runtime random key — initialized once at startup */
static unsigned char guage_siphash_key[16];

/* Platform-aware random key generation */
static inline void guage_siphash_init(void) {
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__)
    arc4random_buf(guage_siphash_key, 16);
#elif defined(_WIN32)
    unsigned int r;
    for (int i = 0; i < 4; i++) {
        rand_s(&r);
        memcpy(guage_siphash_key + i * 4, &r, 4);
    }
#else
    /* Linux / other Unix */
    FILE* f = fopen("/dev/urandom", "rb");
    if (f) {
        (void)fread(guage_siphash_key, 1, 16, f);
        fclose(f);
    }
#endif
}

/* --- SipHash-2-4 core --- */

#define SIPHASH_ROTL(x, b) (uint64_t)(((x) << (b)) | ((x) >> (64 - (b))))

#define SIPHASH_U8TO64_LE(p)                                      \
    (((uint64_t)(p)[0])       | ((uint64_t)(p)[1] <<  8) |        \
     ((uint64_t)(p)[2] << 16) | ((uint64_t)(p)[3] << 24) |        \
     ((uint64_t)(p)[4] << 32) | ((uint64_t)(p)[5] << 40) |        \
     ((uint64_t)(p)[6] << 48) | ((uint64_t)(p)[7] << 56))

#define SIPHASH_SIPROUND            \
    do {                            \
        v0 += v1;                   \
        v1 = SIPHASH_ROTL(v1, 13); \
        v1 ^= v0;                  \
        v0 = SIPHASH_ROTL(v0, 32); \
        v2 += v3;                   \
        v3 = SIPHASH_ROTL(v3, 16); \
        v3 ^= v2;                  \
        v0 += v3;                   \
        v3 = SIPHASH_ROTL(v3, 21); \
        v3 ^= v0;                  \
        v2 += v1;                   \
        v1 = SIPHASH_ROTL(v1, 17); \
        v1 ^= v2;                  \
        v2 = SIPHASH_ROTL(v2, 32); \
    } while (0)

static inline uint64_t siphash24(const void* src, size_t len, const unsigned char key[16]) {
    const uint8_t* data = (const uint8_t*)src;
    uint64_t k0 = SIPHASH_U8TO64_LE(key);
    uint64_t k1 = SIPHASH_U8TO64_LE(key + 8);

    uint64_t v0 = k0 ^ 0x736f6d6570736575ULL;
    uint64_t v1 = k1 ^ 0x646f72616e646f6dULL;
    uint64_t v2 = k0 ^ 0x6c7967656e657261ULL;
    uint64_t v3 = k1 ^ 0x7465646279746573ULL;

    uint64_t m;
    const uint8_t* end = data + len - (len % 8);
    int left = len & 7;

    for (; data != end; data += 8) {
        m = SIPHASH_U8TO64_LE(data);
        v3 ^= m;
        SIPHASH_SIPROUND;
        SIPHASH_SIPROUND;
        v0 ^= m;
    }

    uint64_t b = ((uint64_t)len) << 56;
    switch (left) {
        case 7: b |= ((uint64_t)data[6]) << 48; /* fall through */
        case 6: b |= ((uint64_t)data[5]) << 40; /* fall through */
        case 5: b |= ((uint64_t)data[4]) << 32; /* fall through */
        case 4: b |= ((uint64_t)data[3]) << 24; /* fall through */
        case 3: b |= ((uint64_t)data[2]) << 16; /* fall through */
        case 2: b |= ((uint64_t)data[1]) <<  8; /* fall through */
        case 1: b |= ((uint64_t)data[0]);        break;
        case 0: break;
    }

    v3 ^= b;
    SIPHASH_SIPROUND;
    SIPHASH_SIPROUND;
    v0 ^= b;

    v2 ^= 0xff;
    SIPHASH_SIPROUND;
    SIPHASH_SIPROUND;
    SIPHASH_SIPROUND;
    SIPHASH_SIPROUND;

    return v0 ^ v1 ^ v2 ^ v3;
}

/* Convenience wrapper using global key */
static inline uint64_t guage_siphash(const void* data, size_t len) {
    return siphash24(data, len, guage_siphash_key);
}

#endif /* GUAGE_SIPHASH_H */
