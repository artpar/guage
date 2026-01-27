# Consistency, Correctness & Completeness Plan

**Date:** 2026-01-27
**Status:** Post-recursion fix, 10/10 tests passing

---

## Executive Summary

This document outlines a systematic plan to ensure **consistency**, **correctness**, and **completeness** across the Guage codebase as we move from Phase 2C (structures + metaprogramming) through self-hosting and beyond.

---

## I. Current State Analysis

### What's Working ✅

**Phase 2B (Turing Complete):**
- De Bruijn indices evaluation
- Lambda calculus with closures
- Named recursion (self-reference)
- Auto-documentation system
- Multi-line expression parsing (JUST FIXED)

**Phase 2C Week 1 (Complete):**
- All 15 structure primitives
- Type registry
- Leaf/Node/Graph structures
- Immutable operations
- Reference counting
- 46 structure tests passing

**Phase 2C Week 2 Day 8 (Complete):**
- CFG generation algorithm
- ⌂⟿ query primitive
- Built-in graph type checking
- 10 CFG tests passing
- **66 total tests passing** (46 structure + 10 CFG + 10 other)

### What's Inconsistent ⚠️

**1. Naming Conventions**
- **Issue:** Mixed naming styles across codebase
  - Some functions: `cell_car`, `cell_cdr` (snake_case)
  - Some functions: `prim_cons`, `prim_car` (snake_case with prefix)
  - Some structs: `Cell`, `EvalContext` (PascalCase)
  - Some enums: `CELL_NUMBER`, `GRAPH_CFG` (SCREAMING_SNAKE_CASE)

**Action:** Standardize to:
  - Types/Structs: `PascalCase`
  - Functions: `snake_case` with module prefix (`cell_*`, `prim_*`, `eval_*`)
  - Enums/Constants: `SCREAMING_SNAKE_CASE`
  - Private functions: `static` + module prefix

**2. Error Handling**
- **Issue:** Inconsistent error reporting
  - Some functions print to stderr
  - Some return NULL
  - Some assert and crash
  - No consistent error values (⚠ primitives not used in C code)

**Action:** Create error handling policy:
  - Internal errors → assertions (programming bugs)
  - User errors → return error cell (⚠ values)
  - Parse errors → return NULL + set error flag
  - Memory errors → graceful degradation + cleanup

**3. Memory Management**
- **Issue:** Reference counting rules unclear in some places
  - When to retain vs when caller retains
  - Ownership transfer not always documented
  - Some leaks possible in error paths

**Action:** Document ownership conventions:
  - All constructors return +1 reference
  - All accessors return borrowed references
  - Caller must retain if storing
  - Document transfer with `/* transfer */` comments

**4. Documentation**
- **Issue:** Inconsistent documentation
  - Some files have detailed headers
  - Some functions have no comments
  - No consistent format for API docs

**Action:** Standardize documentation format:
```c
/**
 * Brief description.
 *
 * Detailed description with multiple
 * lines if needed.
 *
 * @param name Description
 * @return Description (ownership rules)
 */
```

---

## II. Correctness Issues

### Critical Correctness Issues 🔴

**1. Memory Leaks in Error Paths**
- **Status:** LIKELY EXISTS
- **Evidence:** Not systematically tested
- **Risk:** High (affects long-running programs)
- **Action:**
  - Add Valgrind/AddressSanitizer tests
  - Audit all error paths
  - Test with intentional failures

**2. Reference Counting Edge Cases**
- **Status:** LIKELY EXISTS
- **Evidence:** Complex sharing patterns not fully tested
- **Risk:** Medium (crashes in production)
- **Action:**
  - Test circular references (though we don't have cycles yet)
  - Test deep nesting
  - Test large closures

**3. Parser Edge Cases**
- **Status:** JUST FIXED (multi-line)
- **Evidence:** Only basic tests exist
- **Risk:** Medium (malformed input crashes)
- **Action:**
  - Fuzz test the parser
  - Test deeply nested expressions
  - Test very long symbols
  - Test all UTF-8 edge cases

**4. De Bruijn Index Boundary Cases**
- **Status:** UNKNOWN
- **Evidence:** Limited test coverage
- **Risk:** Medium (incorrect evaluation)
- **Action:**
  - Test deeply nested lambdas (10+ levels)
  - Test all index values (0, 1, 2, ..., 100)
  - Test mixed named/indexed contexts

### Minor Correctness Issues 🟡

**1. Floating Point Precision**
- Numbers use `double` - rounding errors possible
- No control over precision
- **Action:** Consider arbitrary precision or document limitations

**2. Stack Depth**
- No stack depth limit
- Deeply recursive functions can overflow
- **Action:** Add configurable stack depth limit

**3. Symbol Table Size**
- No limit on symbol table size
- Large programs could exhaust memory
- **Action:** Consider weak references or garbage collection

---

## III. Completeness Gaps

### Missing Functionality by Priority

**P0 - Critical for Phase 2C:**
1. **DFG Generation** (Days 9-10)
   - Data flow graph algorithm
   - ⌂⇝ query primitive
   - 10+ DFG tests

2. **Call Graph** (Day 11)
   - Function call tracking
   - Recursion detection
   - ⌂⊚ query primitive

3. **Dependency Graph** (Day 12)
   - Symbol dependency tracking
   - Topological sort
   - ⌂⊙ query primitive

4. **Auto-Generation Hook** (Days 13-14)
   - Generate graphs on function definition
   - Integration with eval.c handle_define()
   - Performance profiling

**P1 - Critical for Phase 3 (Pattern Matching):**
1. **∇ Pattern Matching**
   - Pattern syntax parsing
   - Pattern matching algorithm
   - Exhaustiveness checking
   - Guard clauses

2. **≗ Structural Equality**
   - Deep comparison (already have ≟ for tests)
   - Make it a primitive
   - Handle circular structures

3. **_ Wildcard**
   - Pattern capture
   - Multiple wildcards
   - Nested wildcards

**P2 - Important for Self-Hosting:**
1. **⌞ Eval Primitive**
   - Currently placeholder
   - Need full implementation
   - Security implications

2. **Module System**
   - Import/export
   - Namespace management
   - Dependency resolution

3. **Standard Library**
   - List operations
   - Map/Set
   - String operations
   - I/O primitives

**P3 - Nice to Have:**
1. **Macro System** (⧉, ⧈, `, ,, ,@)
2. **Generic Programming** (⊳, ⊲, ⊧)
3. **Effect System** (⟪⟫, ↯, ⤴, ≫)
4. **Actor System** (⟳, →!, ←?)

---

## IV. Systematic Testing Strategy

### Test Coverage Goals

**Current Coverage:**
- 10 test files
- ~66 tests total
- Manual verification

**Target Coverage:**
- 50+ test files
- 500+ tests total
- Automated coverage measurement

### Test Categories

**1. Unit Tests (Per Module)**
- `cell.test` - Cell operations
- `eval.test` - Evaluation
- `debruijn.test` - De Bruijn conversion
- `primitives.test` - Each primitive
- `cfg.test` ✅
- `dfg.test` (TODO)
- `structures.test` ✅

**2. Integration Tests**
- Multi-module interactions
- End-to-end scenarios
- Performance benchmarks

**3. Property Tests**
- Fuzz testing
- Random input generation
- Invariant checking

**4. Regression Tests**
- Capture all bugs as tests
- Never let same bug happen twice

### Test Infrastructure

**Needed:**
1. **Test Framework**
   - Better assertions (⊢ is good start)
   - Test fixtures
   - Setup/teardown
   - Parametric tests

2. **Coverage Tools**
   - gcov integration
   - Coverage reports
   - Minimum coverage thresholds

3. **CI/CD**
   - Run tests on every commit
   - Run tests on multiple platforms
   - Performance regression detection

---

## V. Code Quality Standards

### Style Guide

**Formatting:**
- Indent: 4 spaces
- Line length: 100 characters
- Braces: K&R style (opening on same line)
- Pointer: `Type* name` not `Type *name`

**Naming:**
- Files: `lowercase.c/h`
- Types: `PascalCase`
- Functions: `module_snake_case`
- Constants: `SCREAMING_SNAKE_CASE`
- Variables: `snake_case`

**Comments:**
- File header with purpose
- Function header with contract
- Inline comments for non-obvious code
- No commented-out code in commits

### Review Checklist

Before committing, verify:
- [ ] All tests pass
- [ ] No compiler warnings
- [ ] No memory leaks (Valgrind)
- [ ] Documentation updated
- [ ] CHANGELOG updated
- [ ] Commit message explains "why"

---

## VI. Implementation Phases

### Phase 2C Completion (Weeks 2-3)

**Week 2 Remaining (Days 9-12):**
- Day 9-10: DFG generation + tests
- Day 11: Call graph + tests
- Day 12: Dependency graph + tests

**Week 3 (Days 13-21):**
- Day 13-14: Auto-generation integration
- Day 15-16: Comprehensive testing
- Day 17-18: Performance optimization
- Day 19-20: Documentation completion
- Day 21: Phase 2C retrospective

**Deliverables:**
- 4 graph types auto-generated (CFG, DFG, Call, Dep)
- 4 query primitives (⌂⟿, ⌂⇝, ⌂⊚, ⌂⊙)
- 100+ tests passing
- Zero memory leaks
- Complete documentation

### Phase 3: Pattern Matching (18 weeks)

**Core Pattern Matching (Weeks 1-6):**
- ∇ pattern match primitive
- Pattern syntax parsing
- Pattern compiler
- Exhaustiveness checking
- Performance optimization

**Macro System (Weeks 7-12):**
- ⧉ macro definition
- ` backquote/unquote
- Macro expansion
- Hygiene system
- Standard macros

**Generic Programming (Weeks 13-18):**
- ⊳ type parameters
- ⊲ instantiation
- ⊧ constraints
- Trait system
- Generic library

### Phase 4: Self-Hosting (12 weeks)

**Parser in Guage (Weeks 1-3):**
- S-expression parser
- Symbol recognition
- Error reporting

**Type Checker in Guage (Weeks 4-6):**
- Type inference
- Constraint solving
- Error messages

**Compiler in Guage (Weeks 7-9):**
- AST transformation
- Optimization passes
- Code generation

**Bootstrap (Weeks 10-12):**
- Compile compiler with itself
- Verify correctness
- Performance comparison

---

## VII. Consistency Audit Plan

### Immediate Actions (This Week)

1. **Fix Recursion Bug** ✅ DONE
   - Multi-line expression parsing
   - Balance parentheses
   - Handle whitespace

2. **Standardize Error Handling**
   - Define error conventions
   - Audit all error paths
   - Add error tests

3. **Document Memory Management**
   - Ownership rules
   - Reference counting guide
   - Audit for leaks

### Short-term Actions (Phase 2C Week 3)

1. **Code Style Cleanup**
   - Run clang-format
   - Fix naming inconsistencies
   - Add missing documentation

2. **Test Coverage**
   - Add coverage measurement
   - Identify gaps
   - Write missing tests

3. **Performance Baseline**
   - Benchmark current performance
   - Identify bottlenecks
   - Document expectations

### Long-term Actions (Phase 3+)

1. **Static Analysis**
   - clang-tidy
   - cppcheck
   - Custom linters

2. **Formal Verification**
   - Property-based testing
   - Proof of correctness
   - Type soundness proof

3. **Continuous Improvement**
   - Regular code reviews
   - Refactoring sprints
   - Technical debt tracking

---

## VIII. Success Metrics

### Consistency Metrics

- [ ] Zero naming inconsistencies (100% compliant)
- [ ] Zero compiler warnings
- [ ] All functions documented (100% coverage)
- [ ] Single error handling strategy

### Correctness Metrics

- [ ] 100+ tests passing
- [ ] Zero known bugs
- [ ] Zero memory leaks (Valgrind clean)
- [ ] Zero undefined behavior (UBSan clean)
- [ ] Fuzz testing passes (1M inputs)

### Completeness Metrics

- [ ] All Phase 2C primitives implemented (19/19)
- [ ] All graph types generated (4/4)
- [ ] Full test coverage (>90%)
- [ ] Complete documentation (100%)
- [ ] Ready for Phase 3

---

## IX. Risk Assessment

### Technical Risks

**High Risk:**
1. Memory leaks in complex scenarios
2. De Bruijn index bugs in deep nesting
3. Reference counting edge cases

**Mitigation:**
- Comprehensive testing
- Memory sanitizers
- Code review

**Medium Risk:**
1. Parser robustness
2. Performance degradation
3. Platform portability

**Mitigation:**
- Fuzz testing
- Performance benchmarks
- CI on multiple platforms

**Low Risk:**
1. Code style inconsistencies
2. Documentation gaps
3. Test flakiness

**Mitigation:**
- Automated formatting
- Documentation reviews
- Deterministic tests

### Schedule Risks

**Risk:** Phase 2C takes longer than expected
**Impact:** Delays self-hosting
**Mitigation:**
- Fixed time boxes
- MVP scope
- Parallel work streams

---

## X. Next Steps (Immediate)

### This Session:

1. ✅ **Fix recursion bug** (DONE)
2. ⬜ **Create this plan** (IN PROGRESS)
3. ⬜ **Commit fixes and plan**
4. ⬜ **Begin DFG implementation**

### Next Session (Days 9-10):

1. **Create `dfg.h`** - DFG interface
2. **Create `dfg.c`** - DFG algorithm (~300 lines)
3. **Add `⌂⇝` primitive** - Query data flow
4. **Create `dfg.test`** - 10+ DFG tests
5. **Update documentation** - Document DFG

### Week 2 (Days 11-12):

1. **Create call graph** - Function call tracking
2. **Create dependency graph** - Symbol dependencies
3. **Add remaining query primitives**
4. **Integration testing**

---

## XI. Conclusion

This plan provides a roadmap for ensuring **consistency**, **correctness**, and **completeness** across the Guage codebase. By following this systematic approach, we will:

1. **Fix immediate issues** (recursion bug ✅)
2. **Standardize practices** (naming, errors, memory)
3. **Complete Phase 2C** (DFG, call graph, dep graph)
4. **Prepare for Phase 3** (pattern matching)
5. **Achieve self-hosting** (Phase 4)

The key is **incremental progress** with **continuous validation** through automated testing and code review.

**Status:** Ready to proceed with DFG implementation (Days 9-10)

---

**Prepared by:** Claude Sonnet 4.5
**Date:** 2026-01-27
**Next Review:** End of Phase 2C (Day 21)
