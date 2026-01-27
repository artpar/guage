# Day 13 Critical Fixes Plan
## Date: 2026-01-27

## Executive Summary

**Goal:** Fix 3 critical issues blocking Week 3 pattern matching
**Duration:** 5-6 hours estimated
**Priority:** CRITICAL - Must complete before Week 3 starts

---

## Issues Identified

### Issue 1: `:?` Primitive Returns Error 🔴
**Severity:** CRITICAL
**Category:** Type Predicates
**Effort:** 1-2 hours

**Problem:**
```scheme
(:? :test)
; → ⚠:not-a-function:::?
```

**Root Cause Analysis:**
The symbol `:?` starts with a colon, which triggers keyword self-evaluation. When the evaluator sees `:?` it treats it as the keyword `:?` (self-evaluating), not as a reference to the primitive function.

**Why This Happens:**
1. Keywords (starting with `:`) self-evaluate
2. `:?` is parsed as keyword `:?`, not as identifier
3. When applied as function `(:? arg)`, it tries to call keyword `:?` as function
4. Keywords are not functions → error "not-a-function"

**Solution Options:**

**Option A: Parser Special Case** (RECOMMENDED)
- Add special case in parser: `:?` followed by whitespace = primitive reference
- Only `:?` used as identifier refers to primitive
- `:?` in other contexts still a keyword
- Minimal change, backwards compatible

**Option B: Rename Primitive**
- Change `:?` to `symbol?` or `⊙?` (but ⊙ already used for structures)
- Breaking change
- Violates "no English" principle if using `symbol?`
- Not recommended

**Option C: Quote Reference**
- Require `(⌜ :?)` to reference the primitive
- Then apply: `((⌜ :?) value)`
- Too verbose, poor UX

**Recommended Fix: Option A**

**Implementation Steps:**
1. Identify where `:?` is parsed (main.c or eval.c)
2. Add special case: if symbol is `:?` in function position, look up primitive
3. Test that `(:? :test)` returns `#t`
4. Test that `:?` alone still evaluates to `:?` (keyword)
5. Add regression tests

**Files to Modify:**
- `bootstrap/bootstrap/eval.c` (eval function, application case)
- `tests/test_type_predicates.scm` (add tests)

**Test Cases:**
```scheme
; Should work
(:? :symbol)      ; → #t
(:? #42)          ; → #f
(:? #t)           ; → #f
(:? ∅)            ; → #f

; Keyword behavior preserved
:?                ; → :? (still self-evaluates)
(≡ :? :?)         ; → #t (keywords still compare)
```

---

### Issue 2: ADT Variant Syntax Broken 🔴
**Severity:** CRITICAL
**Category:** Node Structures (ADT)
**Effort:** 3-4 hours

**Problem:**
```scheme
(⊚≔ :List [:Nil] [:Cons :head :tail])
; → ⚠:⊚≔ each variant must be a list:⚠:undefined-variable::[:Nil]
```

**Root Cause Analysis:**
Square bracket syntax `[...]` is being parsed incorrectly:
1. Parser sees `[:Nil]`
2. Treats `[` as undefined variable (not special syntax)
3. Returns error before ⊚≔ primitive even executes
4. Square brackets NOT implemented as list syntax

**Current Parser Behavior:**
- `(...)` → Creates pair/list
- `[...]` → NOT RECOGNIZED (tries to parse as variable)
- Expected: `[...]` should create list like `(...)`

**Solution Options:**

**Option A: Implement Square Bracket Syntax** (RECOMMENDED)
- Add `[...]` as sugar for `(...)`
- Common in Lisp/Scheme for readability
- Makes variants visually distinct: `[:Cons :head :tail]`
- Good for documentation

**Option B: Use Parentheses Instead**
- Document that syntax is `(:Nil)` not `[:Nil]`
- Change SPEC.md examples
- No code changes needed
- Less visual distinction

**Option C: Use Quoted Lists**
- Syntax: `(⊚≔ :List (⌜ (:Nil)) (⌜ (:Cons :head :tail)))`
- Requires quoting each variant
- More explicit but verbose

**Recommended Fix: Option B (Short-term), Option A (Long-term)**

**Implementation Steps (Option B - Quick Fix):**

1. **Update SPEC.md**
   - Change all `[...]` to `(...)` in ADT examples
   - Document correct syntax

2. **Test ADT with Parentheses**
   ```scheme
   (⊚≔ :List (:Nil) (:Cons :head :tail))
   ```

3. **Verify all ADT primitives work**
   - ⊚≔ - Define
   - ⊚ - Create instance
   - ⊚→ - Get field
   - ⊚? - Check type/variant

4. **Add comprehensive tests**
   - Simple ADT (no fields)
   - ADT with fields
   - Multiple variants
   - Recursive ADT (List, Tree)

**Implementation Steps (Option A - Full Fix):**

1. **Update Parser (main.c)**
   - Add `[` and `]` to tokenizer
   - Treat `[...]` same as `(...)`
   - Build pair structure

2. **Test Bracket Syntax**
   ```scheme
   [:Nil]                    ; → (:Nil)
   [:Cons :head :tail]       ; → (:Cons :head :tail)
   [#1 #2 #3]                ; → (#1 #2 #3)
   ```

3. **Update SPEC.md**
   - Document that `[...]` and `(...)` are equivalent
   - Use `[...]` for ADT variants by convention

4. **Add tests for both syntaxes**

**Files to Modify:**
- `SPEC.md` (update examples - Option B)
- `bootstrap/bootstrap/main.c` (add bracket parsing - Option A)
- `tests/test_adt.scm` (comprehensive ADT tests)

**Test Cases:**
```scheme
; Simple ADT (no fields)
(⊚≔ :Bool (:True) (:False))
(≔ t (⊚ :Bool :True))
(⊚? t :Bool :True)        ; → #t

; ADT with fields
(⊚≔ :Point (:Point2D :x :y) (:Point3D :x :y :z))
(≔ p2 (⊚ :Point :Point2D #3 #4))
(⊚→ p2 :x)                 ; → #3
(⊚? p2 :Point :Point2D)    ; → #t

; Recursive ADT
(⊚≔ :List (:Nil) (:Cons :head :tail))
(≔ l (⊚ :List :Cons #1 (⊚ :List :Nil)))
(⊚→ l :head)               ; → #1
(⊚? l :List :Cons)         ; → #t

; Tree ADT
(⊚≔ :Tree (:Leaf :value) (:Node :left :value :right))
(≔ leaf (⊚ :Tree :Leaf #42))
(≔ tree (⊚ :Tree :Node leaf #10 leaf))
(⊚→ tree :value)           ; → #10
```

---

### Issue 3: Graph Type Restrictions ⚠️
**Severity:** HIGH (Not blocking, needs clarification)
**Category:** Graph Structures
**Effort:** 1 hour (documentation) OR 2-3 hours (implementation)

**Problem:**
```scheme
(⊝≔ :SocialGraph :MyGraph :nodes :edges)
; → ⚠:⊝≔ graph type must be :generic, :cfg, :dfg, :call, or :dep:::MyGraph
```

**Root Cause Analysis:**
Graph type is restricted to 5 predefined values in primitives.c:
- `:generic` - User-defined graphs
- `:cfg` - Control Flow Graph
- `:dfg` - Data Flow Graph
- `:call` - Call Graph
- `:dep` - Dependency Graph

**Design Question:**
Is this restriction intentional or a bug?

**Arguments FOR Restriction:**
1. Graphs are metaprogramming constructs (CFG/DFG for code analysis)
2. Type safety - known graph types have known properties
3. Optimization - specific graph algorithms per type
4. Current use case - only compiler graphs needed

**Arguments AGAINST Restriction:**
1. SPEC.md shows user-defined graph examples
2. "First-class everything" - users should define graph types
3. Social graphs, knowledge graphs, etc. are valid use cases
4. Flexibility - don't limit users

**Solution Options:**

**Option A: Document as Design Decision** (RECOMMENDED SHORT-TERM)
- This IS intentional for metaprogramming
- Use `:generic` for user graphs
- Document the 5 types and when to use each
- Update SPEC.md to clarify

**Option B: Remove Restriction** (LONG-TERM)
- Allow any symbol as graph type
- Keep 5 built-in types special (optimized)
- User types behave like `:generic`
- More flexible, aligns with "first-class everything"

**Option C: Type Registry System**
- Users register graph types: `(⊝⊙ :SocialGraph ...)`
- Validation ensures type is registered
- More structure, better error messages

**Recommended Fix: Option A (Now), Option B (Week 4+)**

**Implementation Steps (Option A):**

1. **Document Graph Types in SPEC.md**
   ```markdown
   Graph types are predefined for metaprogramming:
   - :generic - General-purpose graphs
   - :cfg - Control Flow Graphs (from ⌂⟿)
   - :dfg - Data Flow Graphs (from ⌂⇝)
   - :call - Call Graphs
   - :dep - Dependency Graphs

   User-defined graphs use :generic type.
   ```

2. **Add Examples**
   ```scheme
   ; Social graph (use :generic)
   (⊝≔ :SocialGraph :generic :nodes :edges)
   (≔ sg (⊝ :SocialGraph))
   (≔ sg (⊝⊕ sg (⟨⟩ :user :alice)))
   ```

3. **Update Audit Files**
   - Mark Issue 3 as "By Design"
   - Document workaround (use :generic)

**Implementation Steps (Option B - Future):**

1. **Modify primitives.c**
   - Remove type check in `prim_graph_define`
   - Allow any symbol as graph type
   - Keep 5 built-in types for optimization

2. **Add Type Registry**
   - Track registered graph types
   - Validate type on graph creation
   - Better error messages

**Files to Modify:**
- `SPEC.md` (document types - Option A)
- `CONSISTENCY_AUDIT.md` (update Issue 3 status)
- `bootstrap/bootstrap/primitives.c` (remove check - Option B)

**Test Cases:**
```scheme
; Built-in types work
(⊝≔ :MyCFG :cfg :nodes :edges)       ; → :MyCFG
(⊝≔ :MyDFG :dfg :nodes :edges)       ; → :MyDFG

; Generic type works
(⊝≔ :Social :generic :nodes :edges)  ; → :Social
(≔ g (⊝ :Social))
(⊝? g :Social)                        ; → #t

; Full example
(⊝≔ :Graph :generic :nodes :edges)
(≔ g (⊝ :Graph))
(≔ g (⊝⊕ g :node1))
(≔ g (⊝⊕ g :node2))
(≔ g (⊝⊗ g :node1 :node2 :edge-label))
(⊝→ g :nodes)                         ; → list of nodes
```

---

## Implementation Order

### Phase 1: Quick Wins (2-3 hours)
**Goal:** Fix most critical issue and document workaround

1. **Fix :? Primitive** (1-2 hours) ⚡
   - Modify eval.c application handling
   - Special case for `:?` in function position
   - Add regression tests
   - **Priority: DO THIS FIRST**

2. **Document Graph Types** (30 min)
   - Update SPEC.md with correct graph type usage
   - Add examples using `:generic`
   - Mark as design decision

### Phase 2: ADT Fix (3-4 hours)
**Goal:** Get ADTs working correctly

3. **Quick ADT Fix - Use Parentheses** (1 hour)
   - Update SPEC.md examples: `[...]` → `(...)`
   - Test ADT with parentheses syntax
   - Document as correct syntax

4. **Comprehensive ADT Testing** (2-3 hours)
   - Create test_adt.scm
   - Test all 4 ADT primitives
   - Test recursive ADTs (List, Tree)
   - Test multiple variants
   - Verify pattern matching prerequisites

### Phase 3: Optional Enhancements (Future)
**Goal:** Improve UX if time permits

5. **Add Square Bracket Syntax** (2-3 hours) 📋
   - Modify parser to handle `[...]`
   - Update tests
   - Optional - can defer to Week 4

6. **Remove Graph Type Restrictions** (2-3 hours) 📋
   - Modify prim_graph_define
   - Allow arbitrary graph types
   - Optional - can defer to Week 4

---

## Success Criteria

### Must Have (Blocking Week 3)
- ✅ `:?` primitive working: `(:? :symbol)` → `#t`
- ✅ ADT working: Can define and use recursive data types
- ✅ All 4 ADT primitives working (⊚≔, ⊚, ⊚→, ⊚?)
- ✅ Can create List and Tree ADTs
- ✅ All tests passing

### Should Have (Important)
- ✅ Graph types documented (use `:generic`)
- ✅ Comprehensive ADT test suite
- ✅ SPEC.md updated with correct syntax
- ✅ Regression tests for fixes

### Nice to Have (Optional)
- 📋 Square bracket syntax `[...]` working
- 📋 Graph types unrestricted
- 📋 Enhanced error messages

---

## Risk Assessment

### Low Risk ✅
- `:?` fix is localized to eval.c
- ADT syntax fix is documentation-only
- Graph type documentation is clarification

### Medium Risk ⚠️
- ADT testing might reveal more issues
- Pattern matching might need ADT features we haven't tested

### Mitigation
1. Test ADTs thoroughly before declaring done
2. Create pattern matching test plan
3. Verify all pattern matching prerequisites

---

## Timeline

**Total Estimated Time:** 5-6 hours

| Phase | Task | Time | Priority |
|-------|------|------|----------|
| 1 | Fix `:?` primitive | 1-2h | ⚡ CRITICAL |
| 1 | Document graph types | 0.5h | HIGH |
| 2 | Fix ADT syntax (parens) | 1h | ⚡ CRITICAL |
| 2 | Comprehensive ADT tests | 2-3h | ⚡ CRITICAL |
| 3 | Square bracket syntax | 2-3h | 📋 OPTIONAL |
| 3 | Remove graph restrictions | 2-3h | 📋 OPTIONAL |

**Critical Path:** 4.5-6.5 hours (Phases 1-2 only)
**With Optional:** 8.5-12.5 hours

---

## Next Steps After Fixes

### Immediate (Day 14)
1. ⏳ **Implement ⌞ (eval)** - 2-3 days
   - Required for test automation
   - Not blocking pattern matching
   - Can proceed in parallel

### Week 3 Ready
2. 🎯 **Start Pattern Matching** - Days 15-21
   - ADT working (✅ after today)
   - Type predicates working (✅ after today)
   - Can implement ∇, ≗, _ primitives

---

## Files to Create/Modify

### Create
- ✅ `DAY_13_FIXES_PLAN.md` (this file)
- `tests/test_type_predicates_fixed.scm`
- `tests/test_adt_comprehensive.scm`

### Modify
- `bootstrap/bootstrap/eval.c` (`:?` fix)
- `SPEC.md` (ADT syntax, graph types)
- `CONSISTENCY_AUDIT.md` (update Issue 3 status)
- `SESSION_HANDOFF.md` (update after fixes)

---

## Commit Strategy

```bash
# After Phase 1
git add bootstrap/bootstrap/eval.c tests/test_type_predicates_fixed.scm
git commit -m "fix: :? primitive now works correctly (special case in eval)"

git add SPEC.md CONSISTENCY_AUDIT.md
git commit -m "docs: clarify graph types and ADT syntax"

# After Phase 2
git add tests/test_adt_comprehensive.scm
git commit -m "test: comprehensive ADT test suite"

git commit -m "fix: ADT now working with parentheses syntax"

# Final
git add SESSION_HANDOFF.md
git commit -m "docs: Day 13 critical fixes complete"
```

---

## Conclusion

These 3 fixes are **critical for Week 3 pattern matching**. Focus on Phases 1-2 (5-6 hours) to unblock. Phase 3 enhancements can wait.

**After these fixes:**
- ✅ Type predicates 100% working
- ✅ ADTs 100% working
- ✅ Pattern matching can start Week 3
- ✅ Foundation solid for standard library

**Priority:** DO THIS NOW before starting Week 3! 🚀
