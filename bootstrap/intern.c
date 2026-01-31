#include "intern.h"
#include "siphash.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

/* =========================================================================
 * HFT-Grade String Intern Table
 *
 * Open-addressing, linear probing. SAHA-style inline keys for short strings.
 * LuaJIT-style string cache (direct-mapped by C pointer hash).
 * Length-first rejection (Lua 5.4). Pre-computed hash per entry.
 * ========================================================================= */

#define INTERN_INITIAL_CAP 512   /* Power of 2, fits in L1 */
#define INTERN_MAX_INLINE   15   /* Inline strings up to 15 bytes */
#define MAX_INTERN_COUNT  4096   /* Max interned symbols */

typedef struct {
    uint64_t hash;                        /*  8B: pre-computed SipHash-2-4 */
    const char* canonical;                /*  8B: stable pointer (identity token) */
    uint16_t id;                          /*  2B: monotonic ID */
    uint8_t len;                          /*  1B: string length */
    char inline_str[INTERN_MAX_INLINE];   /* 15B: SAHA-style inline storage */
} InternEntry;                            /* ~34B per entry */

/* Intern table state */
static InternEntry* intern_table;
static uint32_t intern_size;
static uint32_t intern_cap;
static uint32_t intern_mask;

/* LuaJIT-style string cache: direct-mapped, indexed by C pointer hash */
#define STRCACHE_SIZE 256
#define STRCACHE_MASK 255

typedef struct {
    const char* c_ptr;       /* The C string pointer (from caller) */
    InternResult result;     /* Cached intern result */
} StrCacheEntry;

static StrCacheEntry intern_strcache[STRCACHE_SIZE];

/* ID-indexed parallel arrays for O(1) hash lookup */
static const char* intern_id_canonical[MAX_INTERN_COUNT];
static uint64_t intern_id_hash[MAX_INTERN_COUNT];
static uint16_t intern_next_id = 0;

void intern_init(void) {
    intern_cap = INTERN_INITIAL_CAP;
    intern_mask = intern_cap - 1;
    intern_size = 0;
    intern_table = (InternEntry*)calloc(intern_cap, sizeof(InternEntry));
    memset(intern_strcache, 0, sizeof(intern_strcache));
    intern_next_id = 0;
}

/* Grow table to 2x capacity */
static void intern_grow(void) {
    uint32_t old_cap = intern_cap;
    InternEntry* old_table = intern_table;

    uint32_t new_cap = old_cap * 2;
    InternEntry* new_table = (InternEntry*)calloc(new_cap, sizeof(InternEntry));
    uint32_t new_mask = new_cap - 1;

    /* Reinsert all entries */
    for (uint32_t i = 0; i < old_cap; i++) {
        if (old_table[i].canonical) {
            uint32_t idx = (uint32_t)(old_table[i].hash & new_mask);
            while (new_table[idx].canonical) {
                idx = (idx + 1) & new_mask;
            }
            new_table[idx] = old_table[i];
        }
    }

    free(old_table);
    intern_table = new_table;
    intern_cap = new_cap;
    intern_mask = new_mask;
}

InternResult intern(const char* str) {
    /* 1. Check string cache by C pointer (LuaJIT-style) */
    uint32_t cache_idx = (uint32_t)(((uintptr_t)str >> 4) & STRCACHE_MASK);
    if (intern_strcache[cache_idx].c_ptr == str &&
        intern_strcache[cache_idx].result.canonical != NULL &&
        strcmp(intern_strcache[cache_idx].result.canonical, str) == 0) {
        return intern_strcache[cache_idx].result;
    }

    /* 2. Compute length and hash */
    size_t slen = strlen(str);
    uint8_t len = (uint8_t)(slen > 255 ? 255 : slen);
    uint64_t h = guage_siphash(str, slen);

    /* 3. Probe intern table */
    uint32_t idx = (uint32_t)(h & intern_mask);
    while (intern_table[idx].canonical) {
        /* Length-first rejection: 1 byte compare */
        if (intern_table[idx].len == len &&
            intern_table[idx].hash == h) {
            /* Full comparison using inline storage if available */
            const char* cmp = (len <= INTERN_MAX_INLINE) ?
                intern_table[idx].inline_str : intern_table[idx].canonical;
            if (memcmp(cmp, str, slen) == 0) {
                /* FOUND — update cache and return */
                InternResult r = {
                    intern_table[idx].canonical,
                    intern_table[idx].id,
                    intern_table[idx].hash
                };
                intern_strcache[cache_idx].c_ptr = str;
                intern_strcache[cache_idx].result = r;
                return r;
            }
        }
        idx = (idx + 1) & intern_mask;
    }

    /* 4. Not found — check if resize needed */
    if (intern_size * 4 >= intern_cap * 3) {  /* 75% load factor */
        intern_grow();
        /* Recompute idx after grow */
        idx = (uint32_t)(h & intern_mask);
        while (intern_table[idx].canonical) {
            idx = (idx + 1) & intern_mask;
        }
    }

    /* 5. Insert new entry */
    assert(intern_next_id < MAX_INTERN_COUNT);

    const char* canonical = strdup(str);
    uint16_t id = intern_next_id++;

    intern_table[idx].hash = h;
    intern_table[idx].canonical = canonical;
    intern_table[idx].id = id;
    intern_table[idx].len = len;
    if (len <= INTERN_MAX_INLINE) {
        memcpy(intern_table[idx].inline_str, str, slen);
    }
    intern_size++;

    /* Store in ID-indexed arrays */
    intern_id_canonical[id] = canonical;
    intern_id_hash[id] = h;

    /* Update cache */
    InternResult r = { canonical, id, h };
    intern_strcache[cache_idx].c_ptr = str;
    intern_strcache[cache_idx].result = r;

    return r;
}

uint64_t intern_hash_by_id(uint16_t id) {
    assert(id < intern_next_id);
    return intern_id_hash[id];
}

/* Pre-intern special forms — IDs must match SYM_ID_* constants exactly */
void intern_preload(void) {
    static const char* specials[] = {
        "\xe2\x8c\x9c",                     /* SYM_ID_QUOTE = 0: ⌜ */
        "\xe2\x8c\x9e\xcc\x83",             /* SYM_ID_QUASIQUOTE = 1: ⌞̃ */
        "\xe2\xa7\x89\xe2\x8a\x9c",         /* SYM_ID_MACRO_RULES = 2: ⧉⊜ */
        "\xe2\xa7\x89",                     /* SYM_ID_MACRO = 3: ⧉ */
        "\xe2\x89\x94",                     /* SYM_ID_DEFINE = 4: ≔ */
        "\xe2\x88\x88",                     /* SYM_ID_TYPE_DECL = 5: ∈ */
        "\xe2\x88\x88?",                    /* SYM_ID_TYPE_CHECK = 6: ∈? */
        "\xe2\x88\x88\xe2\x9c\x93",         /* SYM_ID_TYPE_VALIDATE = 7: ∈✓ */
        "\xe2\x88\x88\xe2\x8d\x9c",         /* SYM_ID_TYPE_INFER = 8: ∈⍜ */
        "\xe2\x88\x88\xe2\x8a\xa2",         /* SYM_ID_TYPE_ASSERT = 9: ∈⊢ */
        "\xe2\x88\x88\xe2\x8d\x9c*",        /* SYM_ID_TYPE_INFER_ALL = 10: ∈⍜* */
        ":\xce\xbb-converted",              /* SYM_ID_LAMBDA_CONV = 11: :λ-converted */
        "\xce\xbb",                         /* SYM_ID_LAMBDA = 12: λ */
        "?",                                /* SYM_ID_IF = 13 */
        "\xe2\xaa\xa2",                     /* SYM_ID_SEQUENCE = 14: ⪢ */
        "\xe2\x88\x87",                     /* SYM_ID_RECUR = 15: ∇ */
        "\xe2\x9f\xaa",                     /* SYM_ID_EFFECT_DEF = 16: ⟪ */
        "\xe2\x9f\xaa?",                    /* SYM_ID_EFFECT_Q = 17: ⟪? */
        "\xe2\x9f\xaa\xe2\x86\x92",         /* SYM_ID_EFFECT_GET = 18: ⟪→ */
        "\xe2\x9f\xaa\xe2\x9f\xab",         /* SYM_ID_HANDLE = 19: ⟪⟫ */
        "\xe2\x9f\xaa\xe2\x86\xba\xe2\x9f\xab", /* SYM_ID_RESUME = 20: ⟪↺⟫ */
        "\xe2\x86\xaf",                     /* SYM_ID_PERFORM = 21: ↯ */
        "\xe2\x9f\xaa\xe2\x8a\xb8\xe2\x9f\xab", /* SYM_ID_COMPOSE = 22: ⟪⊸⟫ */
        "\xe2\x8a\xb8",                     /* SYM_ID_PIPE = 23: ⊸ */
        ":__indexed__",                     /* SYM_ID_INDEXED = 24 */
        "~",                                /* SYM_ID_UNQUOTE = 25 */
        "quasiquote",                       /* SYM_ID_QUASIQUOTE_ALT = 26 */
        "unquote",                          /* SYM_ID_UNQUOTE_ALT = 27 */
        "\xe2\x88\xa7",                     /* SYM_ID_AND = 28: ∧ */
        "\xe2\x88\xa8",                     /* SYM_ID_OR = 29: ∨ */
        "\xe2\x9a\xa1?",                    /* SYM_ID_TRY_PROP = 30: ⚡? */
        "\xe2\x88\x88\xe2\x8a\xa1",         /* SYM_ID_REFINE_DEF = 31: ∈⊡ */
    };

    for (int i = 0; i <= MAX_SPECIAL_FORM_ID; i++) {
        InternResult r = intern(specials[i]);
        assert(r.id == (uint16_t)i);  /* Verify IDs assigned in order */
    }
}
