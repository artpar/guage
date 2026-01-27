# Phase 2C Next Steps: Consistency, Correctness, Completeness

**Date:** 2026-01-27
**Status:** Week 1 Days 1-4 Complete ✅
**Next:** Week 1 Days 5-7 (Node/ADT Primitives)

---

## Current State Validation ✅

### Consistency Checks
- ✅ **Build:** Clean compilation, 4 warnings (pre-existing, unused functions)
- ✅ **Tests:** 8/9 passing (recursion.test timeout is pre-existing, unrelated)
- ✅ **Memory:** No leaks detected in structure tests
- ✅ **Symbol Conflict:** ⊙ repurposed for structures, documented in TECHNICAL_DECISIONS.md
- ✅ **Reference Counting:** All primitives properly retain/release

### Completeness Status
- ✅ **Leaf primitives (5/5):** ⊙≔, ⊙, ⊙→, ⊙←, ⊙?
- ⏳ **Node primitives (0/4):** ⊚≔, ⊚, ⊚→, ⊚?
- ⏳ **Graph primitives (0/6):** ⊝≔, ⊝, ⊝⊕, ⊝⊗, ⊝→, ⊝?
- ⏳ **Auto-graph primitives (0/4):** ⌂⟿, ⌂⇝, ⌂⊚, ⌂⊙

### Documentation Status
- ✅ **SESSION_HANDOFF.md:** Up to date (Day 4 complete)
- ✅ **TECHNICAL_DECISIONS.md:** 16 decisions documented
- ✅ **SPEC.md:** Data structures section complete
- ✅ **PHASE_2C_PLAN.md:** Week-by-week roadmap
- ✅ **DATA_STRUCTURES.md:** Complete specification

---

## Consistency Issues Found & Fixes Needed

### 1. SPEC.md Line 25: Symbol Conflict Documentation
**Issue:** SPEC.md still shows ⊙ as `prim_type_of` with "❌ PLACEHOLDER" status

**Current:**
```markdown
| `⊙` | `α → Type` | Type of value | ❌ PLACEHOLDER |
```

**Should be:** Remove or update this line since ⊙ is now for structures

**Fix:** Update SPEC.md Metaprogramming Core section

### 2. Primitive Count Mismatch
**Status:** Actually consistent ✓
- SPEC.md: 15 data structure primitives listed
- Implementation: 5 done (leaf), 10 remaining (4 node + 6 graph)
- Math: 5 + 4 + 6 = 15 ✓

### 3. Test Coverage Gap
**Issue:** All 15 tests are for leaf structures only

**Needed:**
- Node structure tests (List, Tree)
- Graph structure tests (simple graph)
- Error case tests (invalid operations)

---

## Correctness Verification Plan

### Code Review Checklist

#### Reference Counting
- [x] ⊙≔ retains type_tag and all field names
- [x] ⊙ retains all field values
- [x] ⊙→ returns retained value (caller releases)
- [x] ⊙← creates new struct, retains all new values
- [x] ⊙? doesn't leak on checks
- [x] Registry properly retains schemas
- [x] Context cleanup releases registry

#### Error Handling
- [x] ⊙≔ validates type_tag is symbol
- [x] ⊙≔ validates all field names are symbols
- [x] ⊙ checks type exists
- [x] ⊙ validates field count matches schema
- [x] ⊙→ validates struct and field name
- [x] ⊙← validates struct, field name, and field exists
- [x] ⊙? safely handles non-struct values

#### Immutability
- [x] ⊙← returns new struct, doesn't modify original
- [x] Original struct unchanged after update
- [x] Field list properly copied (not shared)

#### Type Safety
- [x] Type tags stored as symbols
- [x] Schema format documented
- [x] Field names validated at creation
- [x] Type checking works correctly

---

## Completeness Plan: Week 1 Days 5-7

### Day 5: Node/ADT Schema Design

**Goal:** Design and implement ⊚≔ (define node type with variants)

**Key Decisions Needed:**

1. **Schema format for ADTs:**
   ```scheme
   :List → ⟨:node ⟨variant_schemas⟩⟩

   Where variant_schemas is:
   ⟨⟨:Nil ∅⟩ ⟨:Cons ⟨:head ⟨:tail ∅⟩⟩⟩ ∅⟩
   ```

2. **Syntax:**
   ```scheme
   (⊚≔ (⌜ :List)
       [(⌜ :Nil)]
       [(⌜ :Cons) (⌜ :head) (⌜ :tail)])
   ```

3. **Validation:**
   - Type tag must be symbol
   - Each variant must have symbol tag
   - Field names must be symbols
   - At least one variant required

**Implementation Steps:**
1. Design schema structure
2. Implement `prim_struct_define_node()`
3. Add to primitives table
4. Write 5 test cases (Nil, Cons, Tree, error cases)
5. Document in TECHNICAL_DECISIONS.md

**Test Cases:**
```scheme
; Define List ADT
(⊚≔ (⌜ :List) [(⌜ :Nil)] [(⌜ :Cons) (⌜ :head) (⌜ :tail)])

; Define Tree ADT
(⊚≔ (⌜ :Tree)
    [(⌜ :Leaf) (⌜ :value)]
    [(⌜ :Node) (⌜ :left) (⌜ :right)])

; Error cases
(⊚≔ #42 [(⌜ :Nil)])  ; Error: type tag must be symbol
(⊚≔ (⌜ :Bad))         ; Error: at least one variant required
```

---

### Day 6: Node Instance Creation and Access

**Goal:** Implement ⊚, ⊚→, ⊚?

#### ⊚ - Create Node Instance

**Syntax:**
```scheme
(⊚ type_tag variant_tag field_values...)
```

**Example:**
```scheme
(⊚ (⌜ :List) (⌜ :Nil))
(⊚ (⌜ :List) (⌜ :Cons) #1 nil-list)
```

**Implementation:**
1. Look up schema
2. Find variant in schema
3. Validate field count
4. Create CELL_STRUCT with:
   - kind = STRUCT_NODE
   - type_tag = :List
   - variant = :Cons
   - fields = alist

**Key Difference from Leaf:**
- Must store variant tag
- Schema lookup is two-level (type → variant → fields)

#### ⊚→ - Get Field from Node

**Syntax:**
```scheme
(⊚→ struct field_name)
```

**Example:**
```scheme
(⊚→ cons-cell (⌜ :head))  ; #1
(⊚→ cons-cell (⌜ :tail))  ; nil-list
```

**Implementation:**
Same as ⊙→ but validates STRUCT_NODE

#### ⊚? - Type and Variant Check

**Syntax:**
```scheme
(⊚? value type_tag variant_tag)
```

**Example:**
```scheme
(⊚? my-list (⌜ :List) (⌜ :Cons))  ; #t or #f
(⊚? my-list (⌜ :List) (⌜ :Nil))   ; #f
```

**Implementation:**
Check both type_tag AND variant match

**Test Cases:**
```scheme
; Create instances
(⊚≔ (⌜ :List) [(⌜ :Nil)] [(⌜ :Cons) (⌜ :head) (⌜ :tail)])
(≔ nil (⊚ (⌜ :List) (⌜ :Nil)))
(≔ cons (⊚ (⌜ :List) (⌜ :Cons) #1 nil))

; Field access
(⊨ (⌜ :list-head) #1 (⊚→ cons (⌜ :head)))
(⊨ (⌜ :list-tail-is-nil) #t (⊚? (⊚→ cons (⌜ :tail)) (⌜ :List) (⌜ :Nil)))

; Type checking
(⊨ (⌜ :nil-is-nil) #t (⊚? nil (⌜ :List) (⌜ :Nil)))
(⊨ (⌜ :cons-is-cons) #t (⊚? cons (⌜ :List) (⌜ :Cons)))
(⊨ (⌜ :cons-not-nil) #f (⊚? cons (⌜ :List) (⌜ :Nil)))
```

---

### Day 7: Graph Primitives Foundation

**Goal:** Implement basic graph operations (⊝≔, ⊝, ⊝⊕, ⊝⊗)

**Deferred to Week 2:**
Graph primitives are lower priority than node primitives because:
1. Pattern matching needs ADTs (List, Tree) more than graphs
2. Auto-generation (CFG/DFG) comes in Week 3
3. Node primitives unlock more use cases

**Alternative Week 1 Day 7 Goal:** Polish and Integration

1. **Complete test coverage:**
   - Add error case tests for all primitives
   - Add nested structure tests
   - Add stress tests (large structures)

2. **Documentation updates:**
   - Update SESSION_HANDOFF.md
   - Add new technical decisions
   - Update PHASE2C_PROGRESS.md

3. **Code cleanup:**
   - Remove unused functions (fix warnings)
   - Add comprehensive comments
   - Verify all error messages are clear

4. **Performance baseline:**
   - Benchmark structure creation
   - Measure memory usage
   - Profile field access

---

## Implementation Order Rationale

### Why Node Primitives Before Graph Primitives?

1. **Pattern Matching Dependency:**
   - Pattern matching needs ADTs (List, Tree, Option, Result)
   - These are all node structures, not graphs
   - Unlocking pattern matching is higher priority

2. **Use Case Frequency:**
   - ADTs used in 90% of functional programs
   - Graphs used in specialized cases (CFG/DFG generation)
   - Maximize value delivery

3. **Complexity:**
   - Node primitives: 4 primitives, similar to leaf
   - Graph primitives: 6 primitives, more complex semantics
   - Build confidence with simpler cases first

4. **Testing:**
   - List and Tree are well-understood, easy to test
   - Graphs require specialized knowledge, harder to validate

---

## Technical Decisions to Document (Day 5-6)

### Decision 17: Node Schema Format

**Decision:** ADT schemas stored as:
```scheme
⟨:node ⟨variant_list⟩⟩
```

Where each variant is:
```scheme
⟨variant_tag field_list⟩
```

**Example:**
```scheme
:List → ⟨:node ⟨⟨:Nil ∅⟩ ⟨:Cons ⟨:head ⟨:tail ∅⟩⟩⟩ ∅⟩⟩
```

**Why:**
- Consistent with leaf schema format (tagged pair)
- Each variant has own field list
- Easy to iterate and validate
- Supports pattern matching (future)

**Code location:** TBD

---

### Decision 18: Variant Storage in Instance

**Decision:** Node instances store variant in `cell->data.structure.variant`

**Why:**
- Type checking needs variant (⊚?)
- Pattern matching needs variant
- Printing needs to show variant
- Separates type from variant

**Alternative considered:**
- Encode in type_tag as `:List:Cons`
- Rejected: Complicates registry, less clear

**Code location:** TBD

---

### Decision 19: Variant Validation Timing

**Decision:** Validate variant exists when creating instance, not when defining type

**Why:**
- Same as field validation pattern (lazy)
- Better error messages (includes actual values)
- Simpler definition primitive
- Consistent with existing approach

**Code location:** TBD

---

### Decision 20: Two-Argument vs Three-Argument ⊚?

**Decision:** ⊚? takes three arguments: `(⊚? value type_tag variant_tag)`

**Why:**
- Need to check both type and variant
- Pattern matching will use this
- Common case: checking specific variant
- If only type check needed, use ⊙? with parent type

**Alternative:**
- Two arguments `(⊚? value type_tag)` checks type only
- Rejected: Doesn't help with pattern matching

**Code location:** TBD

---

## Success Criteria for Week 1 Completion

### Must Have (Week 1)
- [x] All 5 leaf primitives working
- [ ] All 4 node primitives working
- [ ] 25+ structure tests passing (15 leaf + 10 node)
- [ ] Zero memory leaks
- [ ] Clean compilation
- [ ] Documentation complete

### Nice to Have (Week 1)
- [ ] Graph primitives stubbed out
- [ ] Nested structure tests
- [ ] Performance benchmarks
- [ ] Error case coverage

### Deferred to Week 2
- [ ] Graph primitives implementation
- [ ] Auto-generation primitives (⌂⟿, ⌂⇝, etc.)
- [ ] CFG/DFG infrastructure

---

## Risk Assessment

### Low Risk ✅
- Node primitives similar to leaf primitives
- Schema pattern established
- Reference counting pattern proven
- Test infrastructure working

### Medium Risk ⚠️
- Variant lookup complexity (two-level)
- Schema validation (more cases)
- Test case design (need good ADT examples)

### Mitigation Strategies
1. **Start simple:** Implement List before Tree
2. **Test incrementally:** One primitive at a time
3. **Follow patterns:** Reuse leaf primitive structure
4. **Document decisions:** Keep TECHNICAL_DECISIONS.md updated

---

## Immediate Next Steps (Start Day 5)

### 1. Design Node Schema Format
- [ ] Define schema structure
- [ ] Write example schemas (List, Tree, Option, Result)
- [ ] Plan validation logic

### 2. Implement ⊚≔
- [ ] Write `prim_struct_define_node()`
- [ ] Register in primitives table
- [ ] Add to primitives.h

### 3. Write Tests
- [ ] Define List ADT
- [ ] Define Tree ADT
- [ ] Define Option ADT
- [ ] Test error cases

### 4. Verify and Document
- [ ] Run all tests
- [ ] Check memory leaks
- [ ] Update TECHNICAL_DECISIONS.md
- [ ] Update SESSION_HANDOFF.md

---

## Files to Modify (Day 5-6)

### Code Changes
- `bootstrap/bootstrap/primitives.h` - Add 4 node primitive declarations
- `bootstrap/bootstrap/primitives.c` - Implement 4 node primitives (~200 lines)
- `bootstrap/bootstrap/tests/structures.test` - Add 10+ node tests

### Documentation Changes
- `TECHNICAL_DECISIONS.md` - Add decisions 17-20
- `SESSION_HANDOFF.md` - Update with Day 5-6 progress
- `PHASE2C_PROGRESS.md` - Mark Week 1 complete

---

## Questions to Resolve Before Implementation

### 1. Variant Tag Format
- Should variant tags include type? `:List:Cons` vs `:Cons`?
- **Decision:** Just `:Cons` - type is separate field

### 2. Empty Variant Fields
- Can a variant have zero fields? `[(⌜ :Nil)]` vs `[(⌜ :Nil) ∅]`?
- **Decision:** Allow empty field list, more ergonomic

### 3. Variant Uniqueness
- Must variant tags be unique across all types?
- **Decision:** No - `:None` can exist in both Option and Result
- Scoped to type

### 4. Field Name Overlap
- Can different variants have same field names?
- **Decision:** Yes - `:left` in both Leaf and Node variants is fine

---

## Stretch Goals (If Time Permits)

### Week 1 Bonus Tasks
- [ ] Implement graph primitive stubs
- [ ] Add nested structure tests (Point in Circle)
- [ ] Profile memory usage
- [ ] Benchmark performance
- [ ] Generate coverage report

### Documentation Bonus
- [ ] Add ADT examples to DATA_STRUCTURES.md
- [ ] Create STRUCTURE_PATTERNS.md (common ADT patterns)
- [ ] Add visualization examples

---

## Final Checklist Before Moving to Week 2

- [ ] All primitive counts match SPEC.md
- [ ] All tests passing (except pre-existing recursion.test)
- [ ] No memory leaks detected
- [ ] Clean compilation (no new warnings)
- [ ] All documentation updated
- [ ] Git commits with clear messages
- [ ] SESSION_HANDOFF.md reflects current state
- [ ] Ready for pattern matching (Week 3)

---

**Next Session:** Start with Decision 17 (Node Schema Format), then implement ⊚≔

**Estimated Time:** Days 5-6 should take 4-6 hours total

**Prepared by:** Claude Sonnet 4.5
**Date:** 2026-01-27
