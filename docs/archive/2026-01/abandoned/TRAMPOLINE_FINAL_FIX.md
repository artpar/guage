# Trampoline Production Fix - Final Step

**Status:** 95% Complete - One trivial fix needed
**Time Required:** 30 minutes
**Impact:** Enables 100% test coverage with trampoline evaluator

---

## Problem Summary

The trampoline evaluator is 95% functional. All features work correctly:
- ✅ Macro expansion (when, unless, ⧉)
- ✅ Nested lambdas and closures
- ✅ Curried functions (3+ levels)
- ✅ Recursive functions (factorial, fibonacci, map, fold)
- ✅ Full list.scm (328 lines) loads when pasted directly

**But:** Tests fail when using `(⋘ "bootstrap/stdlib/list.scm")` to load modules.

**Root Cause:** The `⋘` (load) primitive calls `eval()` directly instead of using the trampoline.

---

## The Fix

### Location
File: `bootstrap/primitives.c`
Line: 1720

### Current Code
```c
/* Parse and evaluate all expressions in the file */
LoadParser parser = {buffer, 0};
Cell* result = cell_nil();
int expression_count = 0;

while (parser.input[parser.pos] != '\0') {
    // ... parsing code ...

    /* Evaluate expression */
    if (expression_count > 0) {
        cell_release(result);
    }
    result = eval(ctx, expr);  /* ❌ PROBLEM: Uses recursive evaluator */
    cell_release(expr);
    // ...
}
```

### Fixed Code
```c
/* Parse and evaluate all expressions in the file */
LoadParser parser = {buffer, 0};
Cell* result = cell_nil();
int expression_count = 0;

while (parser.input[parser.pos] != '\0') {
    // ... parsing code ...

    /* Evaluate expression */
    if (expression_count > 0) {
        cell_release(result);
    }
#if USE_TRAMPOLINE
    result = trampoline_eval(ctx, expr);  /* ✅ Use trampoline when enabled */
#else
    result = eval(ctx, expr);             /* Use recursive by default */
#endif
    cell_release(expr);
    // ...
}
```

### Additional Requirement
Add include to top of file (if not already present):
```c
#include "trampoline.h"
```

---

## Testing Steps

### 1. Apply Fix
```bash
# Edit bootstrap/primitives.c line 1720
# Add conditional compilation as shown above
```

### 2. Build with Trampoline
```bash
make clean && make
# Verify USE_TRAMPOLINE=1 in bootstrap/main.c line 19
```

### 3. Test Loading Modules
```bash
echo '(⋘ "bootstrap/stdlib/list.scm")' | timeout 10 ./bootstrap/guage
# Should complete in ~1 second, show all function definitions
```

### 4. Run Full Test Suite
```bash
make test
# Expected: 33/33 passing ✅
# If not: Debug specific failures
```

### 5. Verify C Unit Tests
```bash
make test-trampoline
# Expected: 21/21 passing ✅
```

---

## After Fix - Big Bang Switch

Once tests pass with USE_TRAMPOLINE=1:

### 1. Remove Dual Path from main.c

**File:** `bootstrap/main.c`

**Before:**
```c
/* Compile-time flag to enable trampoline evaluator */
#ifndef USE_TRAMPOLINE
#define USE_TRAMPOLINE 1  /* 1 = trampoline, 0 = recursive */
#endif

// ... later ...

#if USE_TRAMPOLINE
    result = trampoline_eval(ctx, expr);
#else
    result = eval(ctx, expr);
#endif
```

**After:**
```c
/* Guage uses the trampoline evaluator exclusively.
 * The trampoline architecture provides:
 * - No C stack overflow (explicit stack management)
 * - Foundation for continuations and time-travel debugging
 * - Inspectable and controllable evaluation process
 * - Full support for macros, stdlib, and complex recursive code
 * Status: Production-ready (33/33 tests passing) ✅ */

// ... later ...

    result = trampoline_eval(ctx, expr);
```

### 2. Remove Dual Path from primitives.c

**File:** `bootstrap/primitives.c` line 1720

**Before:**
```c
#if USE_TRAMPOLINE
    result = trampoline_eval(ctx, expr);
#else
    result = eval(ctx, expr);
#endif
```

**After:**
```c
    result = trampoline_eval(ctx, expr);
```

### 3. Keep eval() for Future Use

**Don't delete eval.c!** Keep it for:
- Comparison testing
- Performance benchmarks
- Reference implementation
- Educational purposes

Just make it not the default execution path.

---

## Verification

### Success Criteria
- ✅ 33/33 tests passing (with trampoline only)
- ✅ 21/21 C unit tests passing
- ✅ No performance regression (< 2x slower)
- ✅ No memory leaks (valgrind clean)
- ✅ All stdlib modules load correctly

### Documentation Updates
- [ ] Update SESSION_HANDOFF.md - Mark trampoline production-ready
- [ ] Update CLAUDE.md - Architecture section
- [ ] Update docs/INDEX.md - Evaluator description
- [ ] Update SPEC.md - Note trampoline is implementation

---

## Why This Matters

The trampoline evaluator is **not just a nice-to-have**. It's the architectural foundation for:

1. **No C Stack Overflow** - Deep recursion doesn't crash
2. **Time-Travel Debugging** - Inspect/modify frames mid-execution
3. **Continuations** - Save/restore execution state
4. **Actor Migration** - Serialize frames, move between machines
5. **Effect System** - Intercept/transform operations
6. **Hot Code Swap** - Replace running code without restart

This is the **ultralanguage vision** coming to life! 🚀

---

## Timeline

- **Fix Application:** 5 minutes
- **Build + Test:** 10 minutes
- **Debug (if needed):** 15 minutes
- **Big Bang Switch:** 10 minutes
- **Documentation:** 20 minutes

**Total:** ~30-60 minutes to completion

---

## Contact for Questions

See SESSION_HANDOFF.md for full context and progress.
