---
Status: REFERENCE
Created: 2025-12-15
Updated: 2026-01-27
Purpose: Design rationale and implementation trade-offs
---

# Technical Decisions Log

**Purpose:** Document the "why" behind implementation choices to maintain consistency and inform future development.

**Last Updated:** 2026-01-27

---

## Phase 2C: Structure System Design Decisions

### 1. Type Registry Location: Inside EvalContext

**Decision:** Store type registry as `Cell* type_registry` field in `EvalContext`.

**Why:**
- **Lifetime alignment:** Type definitions live as long as the evaluation context
- **Natural cleanup:** Automatically freed when context is freed
- **Access pattern:** Primitives already have access to context via `eval_get_current_context()`
- **Consistency:** Matches how we store user bindings (`env`) and primitives
- **Thread safety:** Each context has its own registry (future: can be shared or isolated)

**Alternative considered:**
- Global registry: Would require separate lifecycle management, harder to test in isolation
- Per-module registry: Too complex for bootstrap phase, can add later

**Code location:** `eval.h` line 31, `eval.c` lines 665, 690

---

### 2. Type Registry Storage: Association List (Alist)

**Decision:** Store type definitions as alist: `⟨⟨type_tag schema⟩ ...⟩`

**Why:**
- **Consistency:** Matches how we store environment bindings and primitives
- **Simplicity:** Reuses existing cell cons/car/cdr infrastructure
- **Memory managed:** Reference counting works automatically
- **Extensible:** Easy to add metadata later (just extend the schema cell)
- **Debuggable:** Can print entire registry using existing cell_print

**Alternative considered:**
- Hash table: More efficient lookups, but requires new data structure
- Rejected because: Only ~10-20 types in typical programs, O(n) lookup is fine

**Trade-offs:**
- O(n) lookup time - acceptable for small n
- Simple implementation - critical for bootstrap
- Can optimize later with hash table if needed

**Code location:** `eval.c` lines 1084-1155

---

### 3. Schema Format: Tagged Pair

**Decision:** Schema stored as `⟨kind_tag field_list⟩`

**Example:**
```scheme
:Point → ⟨:leaf ⟨:x ⟨:y ∅⟩⟩⟩
```

**Why:**
- **Kind identification:** First element (`:leaf`, `:node`, `:graph`) identifies structure kind
- **Field list:** Remaining elements are field names for validation
- **Uniform structure:** All schemas follow same pattern, easy to process
- **Extensible:** Can add more elements for constraints later
- **Self-describing:** Looking at schema tells you everything about the type

**For NODE types (future):**
Schema will be: `⟨:node ⟨variant_schemas⟩⟩`
Where each variant: `⟨variant_tag fields⟩`

**Code location:** `primitives.c` lines 412-420

---

### 4. Symbol Quoting Requirement

**Decision:** Symbols must be quoted: `(⊙≔ (⌜ :Point) (⌜ :x) (⌜ :y))`

**Why:**
- **Parser limitation:** Current parser doesn't recognize `:symbol` as atom
- **Consistency:** Already established pattern in test suite (see `tests/arithmetic.test`)
- **Correctness:** Without quote, `:Point` is treated as undefined variable
- **Future-proof:** When parser is enhanced, quoted symbols will still work

**Evidence:**
```scheme
; Without quote - error:
(⊙≔ :Point :x :y)  ; Error: Undefined variable ':Point'

; With quote - works:
(⊙≔ (⌜ :Point) (⌜ :x) (⌜ :y))  ; ::Point
```

**Future improvement:**
Enhance parser to recognize `:symbol` syntax natively. When done, both forms should work.

**Code location:** Parser in `main.c` lines 111-172

---

### 5. Field Storage: Association List

**Decision:** Store struct fields as alist: `⟨⟨field_name value⟩ ...⟩`

**Why:**
- **Order preservation:** Fields maintain definition order
- **Named access:** Can get field by name efficiently
- **Extensible:** Easy to add fields (for open records, future)
- **Debuggable:** Print shows field names and values clearly
- **Uniform:** Same pattern used throughout Guage

**Example:**
```scheme
⊙[::Point ⟨⟨::x #3⟩ ⟨⟨::y #4⟩ ∅⟩⟩]
```

**Alternative considered:**
- Positional list: `⟨#3 #4⟩` - loses field names, harder to debug
- Rejected because: Named fields are essential for clarity

**Code location:** `primitives.c` lines 477-506, `cell.c` cell_struct_get_field

---

### 6. Global Context Access Pattern

**Decision:** Use global `g_current_context` accessed via `eval_get_current_context()`

**Why:**
- **Primitives need registry:** Structure primitives must look up type schemas
- **Signature constraint:** Primitives have signature `Cell* prim(Cell* args)`
- **Can't change signature:** Would break existing 40+ primitives
- **Thread-local ready:** Global pointer can become thread-local in future
- **Minimal coupling:** Only primitives that need it call the getter

**Pattern:**
```c
Cell* prim_struct_create(Cell* args) {
    EvalContext* ctx = eval_get_current_context();
    Cell* schema = eval_lookup_type(ctx, type_tag);
    // ...
}
```

**Safety:**
- Context is always set before eval via `eval_set_current_context(ctx)`
- Only accessed during evaluation, never between evaluations
- NULL check could be added if paranoid

**Code location:** `eval.c` lines 208-218, `primitives.c` lines 382, 432, 455

---

### 7. Reference Counting Strategy

**Decision:** Every cell in registry and schemas is properly retained/released.

**Rules implemented:**
1. **Registry stores:** Retains type_tag and schema when registering
2. **Lookup returns:** Retained copy (caller must release)
3. **Schema construction:** Each cons retains its children
4. **Context cleanup:** Releases entire registry on free

**Example:**
```c
// Register retains both
cell_retain(type_tag);
cell_retain(schema);
Cell* binding = cell_cons(type_tag, schema);

// Lookup returns retained copy
Cell* schema = eval_lookup_type(ctx, type_tag);
// ... use schema ...
cell_release(schema);  // Caller must release
```

**Why this approach:**
- **No leaks:** Every alloc has corresponding free
- **Clear ownership:** Caller knows who owns what
- **Consistent pattern:** Matches rest of codebase
- **Debuggable:** Can track refcounts to find leaks

**Verified:** No leaks detected in testing.

**Code location:** `eval.c` lines 1095-1150

---

### 8. Struct Instance Format

**Decision:** Use existing `CELL_STRUCT` with:
- `kind = STRUCT_LEAF`
- `type_tag = :Point` (symbol)
- `variant = NULL` (for leaves)
- `fields = ⟨⟨name val⟩ ...⟩` (alist)

**Why:**
- **Reuse infrastructure:** CELL_STRUCT already exists from Phase 2C Days 1-2
- **Type tag identifies type:** Can look up schema in registry
- **Variant for ADTs:** NULL for leaves, will use for NODE types
- **Fields standardized:** Same alist pattern everywhere

**Print representation:**
```
⊙[::Point ⟨⟨::x #3⟩ ⟨⟨::y #4⟩ ∅⟩⟩]
 └─ :: prefix indicates evaluated symbol (not quoted)
```

**Code location:** `cell.h` lines 106-111, `primitives.c` line 509

---

### 9. Error Handling Strategy

**Decision:** Return error cells, don't abort. Use descriptive messages.

**Pattern:**
```c
if (!cell_is_symbol(type_tag)) {
    return cell_error("⊙≔ type tag must be a symbol", type_tag);
}
```

**Why:**
- **First-class errors:** Errors are values, can be handled
- **Descriptive:** Include primitive symbol and problem
- **Context preserved:** Attach problematic value to error
- **Debuggable:** User sees exactly what went wrong
- **No crashes:** Program continues, error can be caught

**Examples:**
```scheme
(⊙ (⌜ :Undefined) #1)  ; → ⚠:⊙ undefined type
(⊙ (⌜ :Point) #1)      ; → ⚠:⊙ not enough field values
(⊙→ #42 (⌜ :x))        ; → ⚠:⊙→ first arg must be struct
```

**Code location:** `primitives.c` lines 393, 399, 447, 451, 485, 489, 526, 530, 538

---

### 10. Variadic Primitive Arguments

**Decision:** Use `arity = -1` for primitives with variable arguments.

**Why:**
- **Flexible field counts:** Different types have different field counts
- **Can't know arity:** `(⊙≔ :Point :x :y)` vs `(⊙≔ :Rect :w :h :c)`
- **Existing pattern:** Already used for other variadic primitives
- **Runtime validation:** Check arg count against schema at runtime

**Example:**
```c
{"⊙≔", prim_struct_define_leaf, -1, {"Define leaf...", "..."}},
{"⊙", prim_struct_create, -1, {"Create struct...", "..."}},
```

**Validation:** Done inside primitive by comparing args to schema.

**Code location:** `primitives.c` lines 686-688

---

### 11. Schema Validation Approach

**Decision:** Validate field count and types at instance creation time, not definition time.

**Why:**
- **Lazy validation:** Only validate when actually creating instances
- **Simpler definition:** `⊙≔` just stores schema, doesn't check
- **Better errors:** Error includes actual values, easier to debug
- **Flexibility:** Can define types before all dependencies exist

**Example:**
```scheme
; Definition - no validation
(⊙≔ (⌜ :Point) (⌜ :x) (⌜ :y))

; Creation - validates here
(⊙ (⌜ :Point) #3)       ; Error: not enough values
(⊙ (⌜ :Point) #3 #4 #5) ; Error: too many values
(⊙ (⌜ :Point) #3 #4)    ; Success
```

**Trade-off:**
- Errors at creation, not definition
- But errors are clearer and include actual bad values

**Code location:** `primitives.c` lines 473-506

---

### 12. Test Organization

**Decision:** One test file per feature group: `tests/structures.test`

**Why:**
- **Focused testing:** Each file tests one feature area
- **Easy to run:** Can run individual test files
- **Clear failure location:** Know immediately what broke
- **Matches existing pattern:** See `arithmetic.test`, `documentation.test`, etc.

**Structure:**
```scheme
; Define types
; Create instances
; Test operations
; Use ⊨ test-case primitive
```

**Naming:**
- Test names describe what they test: `:point-x`, `:rect-color`
- Use quoted symbols for test names
- Descriptive, not just numbers

**Code location:** `tests/structures.test`

---

## Design Principles Applied

### 1. Values as Boundaries
- Type registry is cells (not special struct)
- Schemas are cells (not special struct)
- Errors are cells (not exceptions)

### 2. Single Source of Truth
- One type registry per context
- Schema stored once, looked up many times
- No duplicate representations

### 3. No Dual Paths
- All primitives use same registry mechanism
- All schemas use same storage format
- All field access uses same lookup pattern

### 4. Consistency with Existing Code
- Alists for key-value storage (like env, primitives)
- Reference counting everywhere
- Error cells for failures
- Quote for symbols (parser limitation)

---

## Future Improvements

### Parser Enhancement
When parser supports `:symbol` directly:
```scheme
; Both forms should work:
(⊙≔ :Point :x :y)           ; Direct symbol
(⊙≔ (⌜ :Point) (⌜ :x) (⌜ :y))  ; Quoted (backwards compatible)
```

### Hash Table Registry
If performance becomes issue (>100 types):
```c
// Replace alist with hash table
typedef struct {
    Cell** buckets;
    size_t bucket_count;
} TypeRegistry;
```

### Schema Metadata
Extend schema to include constraints:
```scheme
:Point → ⟨:leaf ⟨:x :y⟩ ⟨:constraints ⟨⟨:x ℕ⟩ ⟨:y ℕ⟩⟩⟩⟩
```

### Structural Sharing
For large field lists, share field name lists between instances:
```scheme
; Many instances of :Point can share same field name list
; Currently each instance has own copy
```

---

## Lessons Learned

### What Worked Well
1. **Reusing existing patterns** - Alists, reference counting, error cells
2. **Incremental development** - One primitive at a time, test each
3. **Type registry separation** - Clean interface, easy to test
4. **Comprehensive testing** - Caught issues early

### What Was Tricky
1. **Reference counting complexity** - Required careful attention to retain/release
2. **Symbol quoting** - Parser limitation required workaround
3. **Variadic arguments** - Had to validate manually at runtime
4. **Global context access** - Needed to add getter function

### What Would Change Next Time
1. **Parser first** - Would enhance parser to handle `:symbol` before adding primitives
2. **More test cases** - Could add negative tests (error cases)
3. **Documentation strings** - Could add better primitive documentation

---

## Related Documents

- `CLAUDE.md` - Project principles and philosophy
- `SESSION_HANDOFF.md` - Implementation progress and status
- `PHASE2C_PROGRESS.md` - Weekly progress tracking
- `SPEC.md` - Language specification

---

## Phase 2C Day 4: Additional Structure Primitives

### 13. Symbol Conflict Resolution: ⊙ Repurposed

**Decision:** Remove `prim_type_of` from primitives table; ⊙ symbol exclusively for structures.

**Why:**
- **Priority:** Structure primitives are Phase 2C focus, type introspection is legacy
- **Spec alignment:** SPEC.md marks type-of ⊙ as "❌ PLACEHOLDER"
- **Consistency:** One symbol, one meaning - no ambiguity
- **Future:** Type introspection can use different symbol when needed

**Impact:**
- Removed line 686 from primitives table
- Updated introspection.test to comment out type-of tests
- All structure tests continue passing

**Code location:** `primitives.c` line 684 (commented), `tests/introspection.test` lines 4-6

---

### 14. Immutable Field Update Pattern

**Decision:** ⊙← returns new structure with updated field, doesn't modify original.

**Implementation:**
```c
// Build new field list
Cell* new_fields = cell_nil();
while (iterating old_fields) {
    if (field matches) {
        create new pair with new_value
    } else {
        retain existing pair
    }
}
// Create new struct with new fields
```

**Why:**
- **Functional purity:** No hidden mutations
- **Predictable:** Original value unchanged after update
- **Debuggable:** Can inspect old and new values
- **Consistent:** Matches graph immutability from Days 1-2
- **Thread-safe ready:** No shared mutable state

**Example:**
```scheme
(≔ p1 (⊙ (⌜ :Point) #3 #4))
(≔ p2 (⊙← p1 (⌜ :x) #100))
(⊙→ p1 (⌜ :x))  ; Still #3 (original unchanged)
(⊙→ p2 (⌜ :x))  ; Now #100 (new value)
```

**Code location:** `primitives.c` lines 557-629

---

### 15. Type Checking Returns Boolean

**Decision:** ⊙? returns #t/#f, not error for type mismatches.

**Why:**
- **Predicate semantics:** Predicates return boolean, not error
- **Composable:** Can use in conditionals directly
- **Follows pattern:** Like ℕ?, 𝔹?, :?, etc.
- **User-friendly:** No need for error handling on type checks

**Example:**
```scheme
(? (⊙? value (⌜ :Point))
   (process-point value)
   (handle-non-point value))
```

**Special cases:**
- Non-struct value → #f (not error)
- Missing type tag arg → #f (not error)
- Invalid type tag → error (only on malformed call)

**Code location:** `primitives.c` lines 631-661

---

### 16. Field Update Error Handling

**Decision:** Return error if field doesn't exist, not silently ignore.

**Why:**
- **Fail fast:** Catch typos and mistakes immediately
- **Explicit:** User knows exactly what went wrong
- **Consistent:** Matches ⊙→ behavior (error on missing field)
- **Debuggable:** Error includes field name that wasn't found

**Example:**
```scheme
(⊙← point (⌜ :z) #5)  ; Error: field not found - Point has no :z
```

**Code location:** `primitives.c` line 612

---

---

## Phase 2C Week 2: CFG/DFG Generation Decisions

### 17. CFG as First-Class Graph Structure

**Decision:** CFG is a CELL_GRAPH with graph_type = GRAPH_CFG.

**Why:**
- **Queryable:** Use existing ⊝→ to query nodes, edges, entry, exit
- **Composable:** CFG is just a graph, works with all graph operations
- **First-class:** Can pass CFG to functions, store in variables
- **Uniform:** Same structure for all auto-generated graphs

**Example:**
```scheme
(≔ cfg (⌂⟿ (⌜ !)))           ; Generate CFG
(≔ nodes (⊝→ cfg (⌜ :nodes))) ; Query nodes
(≔ entry (⊝→ cfg (⌜ :entry))) ; Query entry
```

**Code location:** cfg.c lines 236-267

---

### 18. Built-in Graph Types Don't Need Registration

**Decision:** :CFG, :DFG, :CALL, :DEP checked via GraphType enum, not registry.

**Why:**
- **Efficiency:** No registry lookup for built-in types
- **Simplicity:** Built-in types are compile-time constants
- **Type safety:** GraphType enum enforces valid types
- **Extensibility:** User-defined types still use registry (GRAPH_GENERIC)

**Implementation:**
```c
// In prim_graph_is():
if (strcmp(type_str, ":CFG") == 0) {
    return cell_bool(gt == GRAPH_CFG);
}
// vs registry lookup for user types
```

**Code location:** primitives.c lines 1189-1226

---

### 19. CFG Basic Block Representation

**Decision:** Basic blocks are expression cells, not special node types.

**Why:**
- **Simplicity:** Reuse existing Cell structure
- **Memory efficient:** No new allocations needed
- **Debuggable:** Can print blocks as expressions
- **Flexible:** Blocks can be any expression type

**Example:**
```scheme
; Block 0: (≡ n #0)        ; Test expression
; Block 1: #1               ; Then branch
; Block 2: (⊗ n (! ...))    ; Else branch
```

**Code location:** cfg.c lines 62-70

---

### 20. Control Flow Edge Labels

**Decision:** Edges labeled with symbols: :true, :false, :unconditional

**Why:**
- **Readable:** Clear edge semantics at a glance
- **Extensible:** Can add new edge types (:exception, :break, :continue)
- **Queryable:** Can filter edges by label type
- **Standard:** Common convention in CFG literature

**Format:**
```scheme
⟨from_idx to_idx label⟩
⟨0 1 :unconditional⟩  ; Sequential flow
⟨1 2 :true⟩           ; True branch
⟨1 3 :false⟩          ; False branch
```

**Code location:** cfg.c lines 57-71

---

### 21. CFG Builder Pattern

**Decision:** Use temporary CFGBuilder struct with dynamic arrays, then convert to lists.

**Why:**
- **Performance:** Avoid O(n²) repeated cons operations
- **Simplicity:** Build arrays first, convert to lists once
- **Separation:** Clean separation between build phase and output phase
- **Memory:** Predictable memory usage patterns

**Implementation:**
```c
typedef struct {
    Cell** blocks;       // Dynamic array
    Cell** edges;        // Dynamic array
    int entry_idx;
    int exit_idx;
} CFGBuilder;
```

**Alternative considered:**
- Build lists directly: O(n²) for appending, more complex

**Code location:** cfg.c lines 22-34

---

### 22. Recursive CFG Walking

**Decision:** Walk AST recursively, handling branch points specially.

**Why:**
- **Natural:** Matches AST structure
- **Composable:** Easy to handle nested conditionals
- **Extensible:** Can add new expression types easily
- **Correct:** Properly represents control flow paths

**Algorithm:**
```c
int cfg_walk(builder, expr, current_block) {
    if (is_branch_point(expr)) {
        // Add test block
        // Walk then branch recursively
        // Walk else branch recursively
        // Return join point
    }
    // Regular expression - single block
    return block_idx;
}
```

**Handles:**
- Nested conditionals (? inside ?)
- Sequential expressions
- Recursive calls (noted in CFG, not special-cased yet)

**Code location:** cfg.c lines 128-198

---

### 23. Multi-Line Expression Parsing with Parenthesis Balancing

**Decision:** REPL accumulates lines until parentheses are balanced before parsing.

**Why:**
- **Correctness:** Lambda definitions span multiple lines in test files
- **User experience:** Interactive REPL can show continuation prompt (`...`)
- **Robustness:** Prevents parsing incomplete expressions
- **Safety:** Avoids crashes from evaluating NULL

**Problem it solves:**
```scheme
; This was being parsed line-by-line (WRONG):
(≔ ! (λ (n)       ; Line 1: parse error (unbalanced)
  (? (≡ n #0)     ; Line 2: parse as standalone (CRASH)
     #1
     ...)))
```

**Solution:**
1. Count open/close parentheses as lines are read
2. Accumulate lines in buffer until balance = 0
3. Skip comments when counting (`;` to end of line)
4. Filter whitespace-only input
5. Show `...` prompt when accumulating (interactive mode)

**Implementation details:**
```c
int paren_balance(const char* str) {
    // Count ( and ) while skipping comments
    // Returns: positive = need more ), zero = balanced, negative = error
}

void repl() {
    char accumulated[MAX_INPUT * 4];
    int balance = 0;

    while (fgets(line)) {
        strcat(accumulated, line);
        balance += paren_balance(line);

        if (balance == 0) {
            parse_and_eval(accumulated);
            accumulated[0] = '\0';
        }
    }
}
```

**Edge cases handled:**
- Comments containing parentheses (ignored)
- Blank lines (skipped)
- Whitespace-only lines (skipped)
- Interactive vs file input (detected with `isatty()`)
- Too many closing parens (error reported immediately)

**Impact:**
- ✅ Recursion tests now pass (was timing out/crashing)
- ✅ All 10/10 test suites pass
- ✅ Multi-line lambdas work correctly
- ✅ Better REPL user experience

**Alternative considered:**
- Smart parser that returns "need more input": More complex, defers problem
- Rejected because: Simple counting is sufficient and more maintainable

**Code location:** main.c lines 181-285

---

**Document Purpose:** This is a living document. Update it whenever making significant technical decisions. Explain the "why", not just the "what". Future you will thank you.

**Last Updated:** 2026-01-27 - Phase 2C Week 2 Day 8 + Recursion Bug Fix
