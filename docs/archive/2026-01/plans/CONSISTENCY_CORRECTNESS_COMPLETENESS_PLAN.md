# Consistency, Correctness, and Completeness Plan
## Guage Language - 2026-01-27

---

## Executive Summary

Based on CAPABILITY_ASSESSMENT.md findings, this plan addresses:

1. **Consistency** - Fix contradictions and ensure uniform behavior
2. **Correctness** - Fix bugs and verify semantics
3. **Completeness** - Add missing features to reach MVP

**Timeline:** 7 weeks to MVP, 21 months to full vision

---

## Part 1: CORRECTNESS (Fix What's Broken)

### Priority 1: Fix List Operations 🔴 CRITICAL

**Problem:** Crashes on basic list access
```
Assertion failed: (cell_is_pair(pair)), function prim_car, file primitives.c, line 58
```

**Investigation Tasks:**
- [ ] Create minimal test case for crash
- [ ] Debug what `list3` actually contains
- [ ] Verify ⟨⟩ creates proper pairs
- [ ] Check ◁ ▷ in isolation
- [ ] Review reference counting in list operations
- [ ] Add assertions to catch type errors earlier

**Expected Outcome:** Lists work reliably
**Time:** 2-3 days
**Blocking:** Everything that uses lists

---

### Priority 2: Fix GCD/Division Semantics 🟡 HIGH

**Problem:** GCD returns infinity instead of 6
```scheme
(gcd #48 #18)  ; → #inf (should be #6)
```

**Investigation Tasks:**
- [ ] Test ⊘ division in isolation
- [ ] Determine if ⊘ is integer division or float division
- [ ] Add % (modulo) primitive if needed
- [ ] Verify overflow/underflow handling
- [ ] Test with negative numbers
- [ ] Document division semantics in SPEC.md

**Expected Outcome:** Division behaves correctly
**Time:** 1 day
**Blocking:** Algorithms that use division

---

### Priority 3: Fix Structure Symbol Parsing 🟡 HIGH

**Problem:** Symbols in structure definitions not parsed correctly
```
Error: Undefined variable 'Point'
Error: Undefined variable ':x'
⚠:⊙≔ type tag must be a symbol
```

**Investigation Tasks:**
- [ ] Test structure primitives in REPL directly
- [ ] Check if : prefix is handled in file vs REPL
- [ ] Verify symbol creation in parser
- [ ] Review tokenization of :symbol syntax
- [ ] Add test for structure definition from file
- [ ] Document correct syntax in examples

**Expected Outcome:** Structure primitives work from files
**Time:** 1 day
**Blocking:** Data structure usage

---

### Priority 4: Comprehensive Testing 🟢 MEDIUM

**Problem:** Missing test coverage for critical paths

**Testing Tasks:**
- [ ] **List Operations Test Suite**
  - cons, car, cdr, nil
  - nested lists
  - list length
  - list traversal
  - list equality

- [ ] **Error Edge Cases**
  - Division by zero
  - Type errors
  - Out of bounds
  - Stack overflow (deep recursion)

- [ ] **Memory Leak Tests**
  - Run long computations
  - Verify refcount = 0 after execution
  - Stress test with large data structures

- [ ] **Integration Tests**
  - Load from file
  - Multi-function programs
  - Recursive data structures

- [ ] **Performance Benchmarks**
  - Fibonacci(30) - recursion depth
  - Factorial(1000) - large numbers
  - List(10000) - memory usage
  - Ackermann(3,8) - stress test

**Expected Outcome:** Confidence in core reliability
**Time:** 3-4 days
**Blocking:** Nothing, but prevents regressions

---

## Part 2: CONSISTENCY (Unify Behavior)

### Priority 1: Standardize Error Handling 🟡 HIGH

**Current State:** Inconsistent error representation

**Issues:**
- Some functions crash (assertions)
- Some return ⚠ error values
- Some return ∅ or special values
- No consistent error API

**Standardization Tasks:**
- [ ] Document error handling philosophy
  - When to use ⚠ vs crash
  - What errors are recoverable
  - Error value structure

- [ ] Convert assertions to ⚠ where appropriate
  - Type errors → ⚠
  - Bounds errors → ⚠
  - Keep only invariant violations as assertions

- [ ] Add error handling primitives
  - ⚠? (already exists) - test if error
  - ⚠→ (new) - extract error data
  - ⚠∈ (new) - get error type

- [ ] Update all primitives for consistency

- [ ] Write error handling guide

**Expected Outcome:** Predictable error behavior
**Time:** 2-3 days
**Blocking:** Robust programs

---

### Priority 2: Unify Symbol Semantics 🟢 MEDIUM

**Current State:** Confusion about symbols vs keywords

**Issues:**
- :symbol syntax not consistently parsed
- Quote (⌜) vs symbol (:foo) unclear
- Keywords vs symbols unclear

**Unification Tasks:**
- [ ] Define symbol semantics clearly
  - :foo is a symbol literal
  - (⌜ foo) quotes identifier
  - Clear distinction

- [ ] Ensure parser handles all contexts
  - In lists
  - In function calls
  - In structure definitions
  - In patterns (future)

- [ ] Document symbol system in SPEC.md

- [ ] Add symbol test suite

**Expected Outcome:** Clear symbol system
**Time:** 1-2 days
**Blocking:** Metaprogramming features

---

### Priority 3: Standardize Type Checking 🟢 MEDIUM

**Current State:** Type predicates exist but inconsistent usage

**Issues:**
- Some primitives check types, some don't
- Error messages not uniform
- No type coercion rules

**Standardization Tasks:**
- [ ] Add type checks to all primitives
  - Arithmetic: require numbers
  - Logic: require bools
  - Lists: require pairs or nil

- [ ] Define coercion rules (if any)
  - Do we coerce #0 to #f?
  - Do we coerce ∅ to #f?
  - Document decisions

- [ ] Uniform error messages
  - "Expected number, got symbol"
  - Include function name
  - Include actual type received

- [ ] Type checking test suite

**Expected Outcome:** Predictable type behavior
**Time:** 2-3 days
**Blocking:** Language usability

---

## Part 3: COMPLETENESS (Add MVP Features)

### Phase 1: Pattern Matching (CRITICAL) 🔴

**Impact:** 10x usability improvement
**Timeline:** 2 weeks
**Difficulty:** High

#### Week 1: Core Pattern Matching

**Day 1-2: Pattern Types**
- [ ] Define Pattern AST node type
- [ ] Number patterns: `#42`
- [ ] Symbol patterns: `:foo`
- [ ] Nil pattern: `∅`
- [ ] Wildcard pattern: `_`
- [ ] Variable binding patterns

**Day 3-4: Pair Patterns**
- [ ] Pair destructuring: `(⟨⟩ a b)`
- [ ] Nested patterns: `(⟨⟩ a (⟨⟩ b c))`
- [ ] Rest patterns (future): `(⟨⟩ head ...rest)`

**Day 5: Pattern Matching Primitive**
- [ ] Implement ∇ (match) primitive
- [ ] Pattern compiler
- [ ] Match evaluation
- [ ] Exhaustiveness checking (basic)

#### Week 2: Integration & Testing

**Day 6-7: Integration**
- [ ] Integrate with evaluator
- [ ] Pattern match in lambda
- [ ] Pattern match in ≔ (destructuring assignment)

**Day 8-10: Testing**
- [ ] Pattern matching test suite (30+ tests)
- [ ] List algorithms using patterns
- [ ] ADT examples
- [ ] Error cases

**Deliverables:**
```scheme
; Basic patterns work
(∇ #42
  [#42 :match]
  [_ :no-match])  ; → :match

; List patterns work
(≔ length (λ (lst)
  (∇ lst
    [∅ #0]
    [(⟨⟩ _ tail) (⊕ #1 (length tail))])))

; Structure patterns work (later)
(∇ point
  [(⊙ Point x y) (⊕ x y)])
```

---

### Phase 2: Strings (CRITICAL) 🔴

**Impact:** Required for real programs
**Timeline:** 1 week
**Difficulty:** Medium

#### Day 1-2: String Cell Type

- [ ] Add CELL_STRING to cell.h
- [ ] String storage (char*, length)
- [ ] Reference counting for strings
- [ ] String creation/destruction

#### Day 3-4: String Primitives

- [ ] String literals in parser: `"hello"`
- [ ] String concatenation: `⊕` (overload)
- [ ] String length: `#`
- [ ] Character at index: `⊇`
- [ ] Substring: slice operation
- [ ] String comparison: `≡` (overload)

#### Day 5: String Operations

- [ ] String predicates: `:?` (is-string)
- [ ] String → list (explode)
- [ ] List → string (implode)
- [ ] String formatting (basic)

**Deliverables:**
```scheme
(≔ greeting "Hello")
(≔ name "World")
(⊕ greeting " " name "!")  ; → "Hello World!"
(# greeting)                ; → #5
(⊇ greeting #0)             ; → "H"
```

---

### Phase 3: Basic I/O (CRITICAL) 🔴

**Impact:** Can interact with world
**Timeline:** 1 week
**Difficulty:** Medium

#### Day 1-2: Console I/O

- [ ] ⊃ (print) primitive
  - Print to stdout
  - Newline vs no newline
  - Format basic types

- [ ] ⊂ (read) primitive
  - Read line from stdin
  - Parse to string
  - Optional: parse to number

#### Day 3-4: File I/O

- [ ] File read: `⊂!` (read-file path)
- [ ] File write: `⊃!` (write-file path data)
- [ ] File exists: `?!` (file-exists path)
- [ ] Error handling for I/O

#### Day 5: Integration

- [ ] Load Guage code from file
- [ ] REPL improvements
- [ ] Script execution mode
- [ ] I/O test suite

**Deliverables:**
```scheme
; Console I/O
(⊃ "Hello, World!")        ; print
(≔ name (⊂))               ; read input

; File I/O
(≔ data (⊂! "input.txt"))  ; read file
(⊃! "output.txt" result)   ; write file
```

---

### Phase 4: Standard Library (HIGH) 🟡

**Impact:** Developer productivity
**Timeline:** 2 weeks
**Difficulty:** Medium (requires above features)

#### Week 1: Core Data Structures

**List Utilities:**
- [ ] map, filter, fold (already working, standardize)
- [ ] reverse, append, zip
- [ ] take, drop, nth
- [ ] all, any, none
- [ ] sort, group-by
- [ ] flatten, concat

**String Utilities:**
- [ ] split, join
- [ ] trim, pad
- [ ] upper, lower
- [ ] starts-with, ends-with
- [ ] replace, match (basic regex future)

**Math Utilities:**
- [ ] abs, min, max (already have)
- [ ] pow, sqrt, log
- [ ] floor, ceil, round
- [ ] gcd, lcm (fix current gcd)
- [ ] prime?, factor

#### Week 2: Higher-Level Features

**Functional Utilities:**
- [ ] compose, pipe
- [ ] curry, partial
- [ ] memoize
- [ ] lazy sequences (future)

**Collection Utilities:**
- [ ] range generator
- [ ] iterate, repeat
- [ ] partition, chunk
- [ ] frequencies, distinct

**I/O Utilities:**
- [ ] read-lines, write-lines
- [ ] read-json (simple)
- [ ] format strings
- [ ] error reporting helpers

**Deliverables:**
```scheme
; stdlib.η file
(≔ stdlib
  (⟨⟩
    (⟨⟩ :map map)
    (⟨⟩ :filter filter)
    (⟨⟩ :fold fold)
    ; ... etc
  ))
```

---

## Timeline Summary

### Week 1-2: CORRECTNESS
- Days 1-3: Fix list operations, division, symbols
- Days 4-7: Comprehensive testing
- Days 8-10: Error handling consistency

**Milestone:** All current features work correctly

### Week 3-4: PATTERN MATCHING
- Days 1-5: Core patterns implementation
- Days 6-10: Integration and testing

**Milestone:** Pattern matching works, huge usability boost

### Week 5: STRINGS
- Days 1-5: String type and primitives

**Milestone:** Can process text

### Week 6: I/O
- Days 1-5: Console and file I/O

**Milestone:** Can interact with outside world

### Week 7: STANDARD LIBRARY
- Days 1-5: Core utilities
- Days 6-7: Documentation and testing

**Milestone:** MVP Complete! 🎉

---

## After MVP: Path to Full Vision

### Weeks 8-10: Macros & Generics
- ⧉ macro system
- ⊳ ⊲ generics
- Hygienic expansion

### Weeks 11-14: Self-Hosting Prep
- Parser in Guage
- Evaluator in Guage
- ⌞ full eval implementation

### Weeks 15-26: Type System
- Dependent types (Π, Σ)
- Refinement types ({⋅∣φ})
- Type inference
- Linear types

### Weeks 27-62: Advanced Metaprogramming
- Program synthesis (⊛)
- Time-travel debugging (⊙⊳)
- Auto-optimization (◎)
- Cross-program analysis (⊙⋈)

### Weeks 63-88: Distribution & Production
- Actor model
- Hot code swapping
- Native compilation
- Package manager

**Total:** 88 weeks (~21 months) to full vision

---

## Success Metrics

### MVP Metrics (Week 7)

**Must Have:**
- ✅ All test suites passing (0 failures)
- ✅ Pattern matching works
- ✅ Strings work
- ✅ I/O works
- ✅ Can write real programs:
  - Text processing scripts
  - File manipulation
  - Algorithm implementations
  - Data analysis (basic)

**Performance:**
- Fibonacci(30) < 1 second
- List(1000) < 100ms
- No memory leaks

**Documentation:**
- SPEC.md complete
- Tutorial with examples
- Standard library docs

### Full Vision Metrics (Month 21)

**Must Have:**
- ✅ Self-hosting (compiler in Guage)
- ✅ Type system with proofs
- ✅ Metaprogramming features
- ✅ Production-ready performance
- ✅ Real-world usage

**Vision Features:**
- Program synthesis works
- Time-travel debugging works
- Auto-optimization works
- Cross-program analysis works

---

## Risk Management

### High Risks

**Risk:** Pattern matching too complex
**Mitigation:** Start simple, iterate. MVP doesn't need exhaustiveness checking.

**Risk:** Performance too slow
**Mitigation:** Profile early, optimize hot paths. JIT later.

**Risk:** Self-hosting too ambitious
**Mitigation:** Incremental approach. Parser first, then evaluator, then optimizer.

### Medium Risks

**Risk:** Scope creep
**Mitigation:** Strict MVP definition. Advanced features can wait.

**Risk:** Breaking changes
**Mitigation:** Careful API design. Semantic versioning.

**Risk:** Memory leaks
**Mitigation:** Comprehensive testing. Valgrind regularly.

---

## Development Principles

### 1. Fix Before Add
- Always fix bugs before adding features
- Test coverage before moving on
- No technical debt accumulation

### 2. Simple First
- MVP features don't need perfection
- Optimize later, correctness first
- Refactor as understanding grows

### 3. Test Everything
- Unit tests for all primitives
- Integration tests for features
- Regression tests for bugs
- Performance benchmarks

### 4. Document as You Go
- Update SPEC.md immediately
- Examples in documentation
- Session handoffs maintain continuity

### 5. Incremental Progress
- Small commits
- Feature branches
- Regular integration
- Continuous validation

---

## Next Immediate Steps

### Today (Day 1)

**Morning:**
1. Fix list operation crash
2. Create minimal test case
3. Debug and fix root cause
4. Verify all list tests pass

**Afternoon:**
1. Fix GCD/division issue
2. Add modulo primitive if needed
3. Test arithmetic edge cases
4. Update documentation

**Evening:**
1. Fix structure symbol parsing
2. Test from file and REPL
3. Create structure examples
4. Commit fixes

### Tomorrow (Day 2)

**Morning:**
1. Write comprehensive list test suite
2. Write error handling tests
3. Run all tests, verify 100% pass rate

**Afternoon:**
1. Plan pattern matching implementation
2. Create pattern AST design
3. Write pattern matching spec
4. Create example programs

**Evening:**
1. Update SESSION_HANDOFF.md
2. Commit progress
3. Prepare for pattern matching work

---

## Conclusion

**Current State:** Proof of Concept (3/10)

**After Week 7:** Minimum Viable Language (6/10)
- Pattern matching
- Strings
- I/O
- Standard library
- All bugs fixed

**After Month 21:** Full Vision (10/10)
- Self-hosting
- Advanced metaprogramming
- Production-ready
- Unique capabilities

**Priority:** Fix correctness issues this week, then march toward MVP.

**Timeline is Ambitious but Achievable:** Features are well-scoped, foundation is solid, path is clear.

---

**Plan Created:** 2026-01-27
**Plan Author:** Claude Sonnet 4.5
**Status:** Ready to Execute
**First Action:** Fix list operations (primitives.c:58)
