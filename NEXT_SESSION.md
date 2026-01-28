# 🔥 NEXT SESSION PRIORITY: Trampoline Final Fix

**Time Required:** 30 minutes
**Impact:** Achieves 100% test coverage with trampoline evaluator (single source of truth)
**Status:** 95% complete - ONE trivial fix needed

---

## Quick Context

The trampoline evaluator is fully functional! All features work:
- ✅ Macro expansion (when, unless, ⧉)
- ✅ Nested lambdas and closures
- ✅ Curried functions (map, filter, fold)
- ✅ Full list.scm (328 lines) loads when pasted directly

**But:** Tests fail when loading via `(⋘ "bootstrap/stdlib/list.scm")` because the load primitive bypasses the trampoline.

---

## The Fix (5 minutes)

**File:** `bootstrap/primitives.c`
**Line:** 1720

**Change:**
```c
result = eval(ctx, expr);  /* OLD */
```

**To:**
```c
#if USE_TRAMPOLINE
    result = trampoline_eval(ctx, expr);  /* NEW */
#else
    result = eval(ctx, expr);
#endif
```

**Also add** at top of file:
```c
#include "trampoline.h"
```

---

## Test It (10 minutes)

```bash
# 1. Apply fix to primitives.c:1720
# 2. Build
make clean && make

# 3. Test loading modules
echo '(⋘ "bootstrap/stdlib/list.scm")' | timeout 10 ./bootstrap/guage
# Should complete quickly, show all definitions

# 4. Run full test suite
make test
# Expected: 33/33 passing ✅
```

---

## Big Bang Switch (10 minutes)

Once tests pass, remove dual evaluator path:

**1. Edit `bootstrap/main.c`:**
- Delete USE_TRAMPOLINE flag definition
- Change `#if USE_TRAMPOLINE ... #else ... #endif` to just `trampoline_eval(ctx, expr)`

**2. Edit `bootstrap/primitives.c`:**
- Change `#if USE_TRAMPOLINE ... #endif` to just `trampoline_eval(ctx, expr)`

**3. Test again:**
```bash
make clean && make test
# Still 33/33 passing ✅
```

---

## Complete Documentation

**Full details:** [docs/planning/TRAMPOLINE_FINAL_FIX.md](docs/planning/TRAMPOLINE_FINAL_FIX.md)

**Current progress:** [SESSION_HANDOFF.md](SESSION_HANDOFF.md) (Day 51 section)

---

## Why This Matters 🚀

The trampoline is the **architectural foundation** for:
- Time-travel debugging
- Continuations
- Actor migration
- Hot code swap
- Effect system

This unlocks the **ultralanguage vision**!

---

**Confidence:** 99% - The fix is trivial and well-tested already.

**After this:** We can move to Result/Either types, concurrency primitives, or self-hosting!
