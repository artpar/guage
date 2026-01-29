---
Status: ACTIVE
Created: 2026-01-29
Purpose: Quick start guide for continuing Day 69
---

# Day 69 Continuation: Quick Start Guide

## 🎯 Session Goal
Complete graph algorithm implementation by fixing memory corruption bug.

## ⏱️ Estimated Time: 2-3 hours

## 📋 Prerequisites
- Read `DAY_69_PROGRESS.md` for full investigation details
- Current state: 20/35 graph tests passing, root cause identified

## 🔧 Step-by-Step Instructions

### Step 1: Apply Environment Fix (1-2 hours)

**Location:** `bootstrap/eval.c` line 735-738

**Current Code (BUGGY):**
```c
void eval_define(EvalContext* ctx, const char* name, Cell* value) {
    Cell* sym = cell_symbol(name);
    Cell* binding = cell_cons(sym, value);
    ctx->env = cell_cons(binding, ctx->env);  // ← BUG: leaks old env
    ...
}
```

**Fixed Code:**
```c
void eval_define(EvalContext* ctx, const char* name, Cell* value) {
    Cell* sym = cell_symbol(name);
    Cell* binding = cell_cons(sym, value);

    Cell* old_env = ctx->env;
    ctx->env = cell_cons(binding, ctx->env);
    cell_release(old_env);  // ← FIX: Release old environment
    ...
}
```

**Testing:**
```bash
# 1. Apply fix above
# 2. Rebuild
make clean && make

# 3. Test main suite (should still pass)
make test
# Expected: 68/68 passing

# 4. Test graph algorithms
make test-one TEST=bootstrap/tests/test_cfg_algorithms.test
# Expected: 30-35/35 passing (up from 20/35)
```

**If Main Tests Break:**
- Consider REPL-only fix (see DAY_69_PROGRESS.md)

### Step 2: Add Node Validation (15 minutes)

Add to `prim_graph_traverse()` and `prim_graph_reachable()` in `bootstrap/primitives.c`

### Step 3: Final Verification (30 minutes)

Run full test suite, verify 103/103 passing.

### Step 4: Documentation & Commit (15 minutes)

Update SPEC.md, SESSION_HANDOFF.md, archive progress doc, commit.

## ✅ Success Criteria

- [ ] 68/68 main tests still passing
- [ ] 35/35 graph tests passing
- [ ] Documentation updated
- [ ] Changes committed

See DAY_69_PROGRESS.md for complete details.
