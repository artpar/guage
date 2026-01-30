# Guage TODO List

---
Status: CURRENT
Created: 2026-01-28
Updated: 2026-01-28
Purpose: Active task tracking for Guage development
---

## Current Status 🎉

**MAJOR MILESTONES ACHIEVED:**
- ✅ **Turing Complete** (Day 10) - Lambda calculus, recursion, closures
- ✅ **Pattern Matching** (Days 15-19) - Complete with exhaustiveness checking
- ✅ **Module System** (Days 25-30) - Load, provenance, dependencies, validation
- ✅ **Macro System** (Day 33) - Compile-time code transformation
- ✅ **Parser in Guage** (Days 39-41) - Self-hosting foundation
- ✅ **80 Primitives** - 74 functional + 6 placeholders (effects, actors)
- ✅ **900+ Tests Passing** - Comprehensive test coverage

**CURRENT PHASE:** Self-Hosting Path (Week 6+)

---

## 🎯 Immediate Priorities (Next 1-2 Weeks)

### 1. Self-Hosting Completion - CRITICAL PATH
**Status:** 66% Complete (Tokenizer ✅, Parser ✅, Evaluator ❌)

**Day 42: S-Expression Evaluator in Guage** (3-4 hours)
- [ ] Environment module (env-empty, env-extend, env-lookup)
- [ ] Evaluator core (eval-atom, eval-list, eval-lambda, eval-apply)
- [ ] Integration tests (eval numbers, arithmetic, lambdas, recursion)
- [ ] **Milestone:** Guage can interpret Guage!

**Day 43-44: Self-Hosting Tests & Validation** (2-3 hours)
- [ ] Factorial via self-hosted eval
- [ ] Fibonacci via self-hosted eval
- [ ] Pattern matching via self-hosted eval
- [ ] Standard library via self-hosted eval

**Day 45: REPL Using Self-Hosted Eval** (2-3 hours)
- [ ] Read-eval-print loop in Guage
- [ ] Error handling and display
- [ ] Multi-line input support
- [ ] **Milestone:** Full self-hosting achieved!

---

### 2. Standard Library Expansion (Ongoing)
**Status:** 50+ stdlib functions across 8 modules

**High Priority:**
- [ ] **Math module** - sqrt, pow, trig, logarithms
- [ ] **String utilities** - split, trim, case conversion
- [ ] **Data structures** - maps, sets, trees
- [ ] **File system** - directory ops, path manipulation

**Medium Priority:**
- [ ] **Regex module** - Pattern matching for strings
- [ ] **JSON module** - Parse and generate JSON
- [ ] **HTTP module** - Basic web requests
- [ ] **Testing utilities** - Property-based testing helpers

---

### 3. Parser Improvements (As Needed)
**Status:** Working, but could be enhanced

- [ ] Better error messages with line numbers
- [ ] Unicode symbol parsing improvements
- [ ] Comment handling verification
- [ ] Performance optimization for large files

---

## 📋 Medium-Term Goals (Week 7-10)

### Compiler Pipeline in Guage
**Goal:** Complete compilation toolchain in Guage

1. **Optimizer Module** (1 week)
   - [ ] Constant folding
   - [ ] Dead code elimination
   - [ ] Common subexpression elimination
   - [ ] Inline expansion

2. **Code Generator** (1 week)
   - [ ] Guage → C code generation
   - [ ] Integration with C runtime
   - [ ] Benchmark against interpreter

3. **Type Checker (Gradual)** (2 weeks)
   - [ ] Type inference for primitives
   - [ ] Function type signatures
   - [ ] Gradual typing support
   - [ ] Optional type annotations

---

## 🚀 Long-Term Vision (Month 3-6)

### Advanced Type System
- [ ] Dependent types (Π, Σ)
- [ ] Linear types (⊸, !, ?)
- [ ] Refinement types ({⋅∣φ})
- [ ] Effect tracking (⟪⟫, ↯)
- [ ] Session types (▷, ◁, ⊕, &)

### Effects & Actors Runtime
- [ ] Implement real effect handlers (currently placeholders)
- [ ] Implement real actor system (currently placeholders)
- [ ] Delimited continuations
- [ ] Message passing runtime
- [ ] Concurrent GC

### Metaprogramming Features
**See:** `docs/reference/METAPROGRAMMING_VISION.md`

- [ ] Program synthesis (⊛)
- [ ] Code repair (◂)
- [ ] Time-travel debugging (⊙⊳, ⊆)
- [ ] API evolution (⋈, ⊿)
- [ ] Self-optimizing code (◎)
- [ ] Cross-program analysis (⊙⋈)

---

## 🧪 Testing & Quality

### Test Coverage
- ✅ 900+ tests passing
- ✅ Primitive tests (all 74 primitives)
- ✅ Pattern matching tests (165 tests)
- ✅ Module system tests
- ✅ Macro system tests
- ✅ Parser tests
- [ ] Self-hosting tests (coming Day 42-45)
- [ ] Property-based tests
- [ ] Benchmark suite

### Documentation
- ✅ README.md - Project overview
- ✅ SPEC.md - Language specification
- ✅ CLAUDE.md - Philosophy and principles
- ✅ SESSION_HANDOFF.md - Current status
- ✅ docs/INDEX.md - Documentation navigation
- ✅ docs/reference/* - Deep-dive technical docs
- [ ] TUTORIAL.md - Getting started guide
- [ ] EXAMPLES.md - Real-world code examples
- [ ] API_REFERENCE.md - Complete API documentation

---

## 📦 Tooling & Infrastructure

### REPL Enhancements
- ✅ Help system (:help, :primitives, :modules)
- ✅ Module introspection
- [ ] Multi-line input support
- [ ] Syntax highlighting
- [ ] Tab completion
- [ ] History with search
- [ ] Better error messages with stack traces

### IDE Support
- [ ] VS Code extension
- [ ] Syntax highlighting
- [ ] LSP server for autocomplete
- [ ] De Bruijn → names hover
- [ ] Type hints on hover
- [ ] Error squiggles
- [ ] Code formatter

### Build System
- ✅ Makefile for C bootstrap
- [ ] Guage-based build system
- [ ] Package manager design
- [ ] Module dependency resolution
- [ ] Distribution packaging

---

## 🔧 Technical Debt & Improvements

### Memory Management
- ✅ Reference counting (current)
- [ ] Cycle detection for refcounting
- [ ] Generational GC (future)
- [ ] Concurrent GC (future)

### Performance
- [ ] Benchmark suite
- [ ] Profile-guided optimization
- [ ] JIT compilation (future)
- [ ] SIMD primitives (future)

### Error Handling
- ✅ Errors as values (⚠)
- ✅ First-class error handling
- [ ] Better error messages with context
- [ ] Stack traces for debugging
- [ ] Source location tracking

---

## 📊 Success Metrics

### Short-Term (Next Month)
- ✅ Turing complete
- ✅ Pattern matching working
- ✅ Module system functional
- [ ] Self-hosting complete
- [ ] 100+ stdlib functions
- [ ] 1000+ tests passing

### Mid-Term (3 Months)
- [ ] Type checker working
- [ ] Compiler pipeline complete
- [ ] Real-world applications built
- [ ] Community contributions
- [ ] Documentation complete

### Long-Term (6 Months)
- [ ] Effects system implemented
- [ ] Actor runtime working
- [ ] Native compilation
- [ ] Production-ready v1.0
- [ ] Package ecosystem started

---

## 🎯 Current Week Focus

**Week 6 (Days 41-47):** Self-Hosting Path
- ✅ Day 41: Parser debugging complete
- [ ] Day 42: S-expression evaluator in Guage
- [ ] Day 43-44: Self-hosting tests
- [ ] Day 45: REPL in Guage
- [ ] Day 46-47: Integration & polish

**Next Week:** Compiler pipeline (optimizer, codegen)

---

## Notes

### Design Philosophy (From CLAUDE.md)
1. **Pure symbols only** - No English keywords
2. **First-class everything** - Functions, errors, tests, CFG/DFG
3. **Single source of truth** - No duplication
4. **Values as boundaries** - All interfaces use simple values
5. **Mathematical foundation** - Lambda calculus, De Bruijn indices

### What's Working Well
- ✅ Test-driven development
- ✅ Incremental feature addition
- ✅ Clear documentation structure
- ✅ Daily progress tracking
- ✅ Systematic task completion

### Areas for Improvement
- Need more real-world examples
- Documentation could use more tutorials
- REPL UX could be smoother
- Error messages could be clearer
- Build time could be faster

---

**Last Updated:** 2026-01-28 (Day 41 Complete)
**Next Review:** After Day 45 (Self-hosting complete)
