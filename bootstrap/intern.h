#ifndef GUAGE_INTERN_H
#define GUAGE_INTERN_H

#include <stdint.h>

/* Symbol ID constants for special forms (pre-assigned in order) */
#define SYM_ID_QUOTE           0
#define SYM_ID_QUASIQUOTE      1
#define SYM_ID_MACRO_RULES     2
#define SYM_ID_MACRO           3
#define SYM_ID_DEFINE          4
#define SYM_ID_TYPE_DECL       5
#define SYM_ID_TYPE_CHECK      6
#define SYM_ID_TYPE_VALIDATE   7
#define SYM_ID_TYPE_INFER      8
#define SYM_ID_TYPE_ASSERT     9
#define SYM_ID_TYPE_INFER_ALL 10
#define SYM_ID_LAMBDA_CONV    11
#define SYM_ID_LAMBDA         12
#define SYM_ID_IF             13
#define SYM_ID_SEQUENCE       14
#define SYM_ID_RECUR          15
#define SYM_ID_EFFECT_DEF     16
#define SYM_ID_EFFECT_Q       17
#define SYM_ID_EFFECT_GET     18
#define SYM_ID_HANDLE         19
#define SYM_ID_RESUME         20
#define SYM_ID_PERFORM        21
#define SYM_ID_COMPOSE        22
#define SYM_ID_PIPE           23
#define SYM_ID_INDEXED        24
#define SYM_ID_UNQUOTE        25
#define SYM_ID_QUASIQUOTE_ALT 26
#define SYM_ID_UNQUOTE_ALT   27
#define SYM_ID_AND            28
#define SYM_ID_OR             29
#define SYM_ID_TRY_PROP       30
#define MAX_SPECIAL_FORM_ID   30

/* Intern result — returned by intern() */
typedef struct {
    const char* canonical;  /* Stable pointer (identity token) */
    uint16_t id;            /* Monotonic ID */
    uint64_t hash;          /* Pre-computed SipHash-2-4 */
} InternResult;

/* Initialize intern table (call once before any intern()) */
void intern_init(void);

/* Pre-intern special forms — IDs match SYM_ID_* constants */
void intern_preload(void);

/* Intern a string. Returns canonical pointer, ID, and hash.
 * Cache hit path: 1 pointer compare, zero string access.
 * Miss path: strlen + SipHash + ~1.3 probes avg at 75% load. */
InternResult intern(const char* str);

/* O(1) hash lookup by ID (direct array index) */
uint64_t intern_hash_by_id(uint16_t id);

#endif /* GUAGE_INTERN_H */
