# Metaprogramming for Guage - Executive Summary

**Date:** 2026-01-27
**Status:** Research complete, ready to implement
**Timeline:** 12-18 weeks to full metaprogramming

---

## What Was Decided

### Three Interconnected Systems

1. **∇ Pattern Matching** (Foundation)
   - Structural destructuring
   - Foundation for macros and generics
   - **Timeline:** 2-4 weeks

2. **⧉ Hygienic Macros** (Code Transformation)
   - Compile-time code generation
   - Automatic hygiene via De Bruijn indices
   - **Timeline:** 4-6 weeks

3. **⊳ Generic Programming** (Parametric Polymorphism)
   - Zero-cost abstractions
   - Trait-based constraints
   - **Timeline:** 6-8 weeks

---

## Key Design Decisions

### 1. Structural, Not Textual

**Traditional (bad):**
```c
#define MAX(a,b) ((a)>(b)?(a):(b))  // Text substitution, bugs possible
```

**Guage (good):**
```scheme
(⧉ max (⧈ (a b) `(? (> ,a ,b) ,a ,b)))  // AST transformation, type-safe
```

### 2. Friendly by Design

**Symbols over words:**
- ❌ `define-syntax` (English, ambiguous)
- ✅ `⧉` (Universal symbol)

**Structure over text:**
- AI can verify transformations
- Can detect bugs automatically
- Can synthesize correct macros

### 3. Zero-Cost Abstractions

**Monomorphization (like Rust):**
```scheme
; One generic function
(≔ id (λ (⊳ T) (λ (x : T) x)))

; Compiler generates separate versions:
(≔ id_Nat (λ (x : ℕ) x))
(≔ id_Bool (λ (x : 𝔹) x))

; No runtime overhead!
```

### 4. Hygiene via De Bruijn

**Key insight:** De Bruijn indices prevent variable capture automatically.

```scheme
; Traditional Lisp: variable capture possible
(defmacro swap (a b)
  `(let ((tmp ,a)) ...))  ; 'tmp' can be captured

; Guage: impossible to capture
(⧉ swap (⧈ (a b)
  `(λ (,a) (λ (,b) ...))))  ; Uses indices, not names
```

---

## New Primitives

### Pattern Matching (3 primitives)

| Symbol | Meaning | Example |
|--------|---------|---------|
| `∇` | Pattern match | `(∇ lst [∅ #0] [(⟨⟩ h t) (⊕ #1 (len t))])` |
| `≗` | Structural equality | `(≗ (⟨⟩ #1 #2) (⟨⟩ #1 #2))` |
| `_` | Wildcard | `(∇ x [_ :any])` |

### Macros (5 primitives)

| Symbol | Meaning | Example |
|--------|---------|---------|
| `⧉` | Define macro | `(⧉ when (⧈ (c b) ...))` |
| `⧈` | Macro params | `(⧈ (condition body) ...)` |
| `` ` `` | Backquote | `` `(? ,condition ,body ∅)`` |
| `,` | Unquote | `,variable` |
| `,@` | Splice | `,@list-of-args` |

### Generics (3 primitives)

| Symbol | Meaning | Example |
|--------|---------|---------|
| `⊳` | Generic param | `(λ (⊳ T) ...)` |
| `⊲` | Instantiate | `(⊲ identity ℕ)` |
| `⊧` | Constraint | `(⊳ T : (⊧ Ord))` |

---

## Power Examples

### Before Patterns (Painful)

```scheme
(≔ length (λ (lst)
  (? (∅? lst)
     #0
     (⊕ #1 (length (▷ lst))))))
```

### After Patterns (Clear)

```scheme
(≔ length (λ (lst)
  (∇ lst
    [∅ #0]
    [(⟨⟩ _ tail) (⊕ #1 (length tail))])))
```

### Control Flow Macro

```scheme
; Define once
(⧉ when (⧈ (condition body)
  `(? ,condition ,body ∅)))

; Use everywhere
(when (> x #0)
  (⊕ x #1))

; Expands to:
(? (> x #0) (⊕ x #1) ∅)
```

### Generic Data Structure

```scheme
; Define once for all types
(≔ Stack (λ (⊳ T)
  (⟨⟩ :empty ∅
      :push (λ (s item) (⟨⟩ item s))
      :pop (λ (s) (▷ s))
      :top (λ (s) (◁ s)))))

; Instantiate
(≔ IntStack (⊲ Stack ℕ))
(≔ BoolStack (⊲ Stack 𝔹))

; Zero runtime overhead!
```

---

## Why This Matters

### Self-Hosting

**Current blocker:** Can't write compiler in Guage without metaprogramming.

**After metaprogramming:**
- Parser uses patterns to match AST shapes
- Compiler uses macros to generate code
- Type checker uses generics for algorithms
- **Self-hosting achieved!**

### Expressiveness

**Current:** Only basic lambda calculus
**After:** DSLs, control flow, abstractions

### Performance

**Current:** Interpreted, slow
**After:** Compile-time optimization, zero-cost abstractions

### Friendliness

**Structural macros:**
- AI can verify correctness
- AI can synthesize from examples
- AI can optimize automatically

---

## Implementation Order (CRITICAL)

```
Week 0: ✅ Research complete
    ↓
Week 1-4: ∇ Pattern Matching
    ↓
Week 5-10: ⧉ Macros
    ↓
Week 11-18: ⊳ Generics
```

**Can't skip pattern matching** - it's the foundation for everything else.

---

## Files Created

1. **METAPROGRAMMING_RESEARCH.md** - 1700 lines of comprehensive research
2. **METAPROGRAMMING_IMPLEMENTATION_PLAN.md** - Detailed 18-week plan
3. **METAPROGRAMMING_SUMMARY.md** - This file
4. **SPEC.md** - Updated with new primitives

---

## Next Actions

### Immediate (This Week)

1. **Review research** - Read METAPROGRAMMING_RESEARCH.md
2. **Discuss approach** - Team alignment on design
3. **Approve plan** - Sign off on implementation strategy

### Week 1 (Start Pattern Matching)

1. Extend cell.h with Pattern type
2. Implement pattern parser
3. Implement pattern matcher
4. Add ∇ primitive
5. Write tests

### Week 2-4 (Finish Pattern Matching)

1. Optimize matcher (decision trees)
2. Add ≗ primitive
3. Rewrite stdlib with patterns
4. Complete test suite
5. Documentation

---

## Questions & Answers

### Q: Why patterns first?

**A:** Macros need patterns to destructure syntax. Generics need patterns to match types. It's the foundation.

### Q: Why not just copy Scheme/Rust?

**A:** Guage is symbol-only and first. We need structural, not textual, and mathematical symbols, not English.

### Q: How long to self-hosting?

**A:** ~18 weeks for metaprogramming + ~12 weeks to write compiler in Guage = **30 weeks total**.

### Q: What about eval (⌞)?

**A:** Not needed for self-hosting. Eval is for runtime metaprogramming. Macros are compile-time.

### Q: Performance cost?

**A:** **Zero.** Monomorphization generates specialized code with no overhead. Same performance as hand-written.

---

## Risk Assessment

### Low Risk
- Pattern matching (well-understood)
- De Bruijn hygiene (already using it)

### Medium Risk
- Macro expansion pipeline (need careful integration)
- Monomorphization code bloat (mitigated by dead code elimination)

### High Risk
- Type inference (but we can start without it)
- Trait system complexity (Phase 3, can simplify if needed)

---

## Success Metrics

### Pattern Matching Success:
- ✅ All list functions use patterns
- ✅ Binary tree traversal clean
- ✅ No manual car/cdr chains

### Macro Success:
- ✅ Control flow macros (when, unless, cond)
- ✅ Let bindings macro
- ✅ Zero variable capture bugs

### Generic Success:
- ✅ Generic List, Tree, Map
- ✅ Generic sort, search, filter
- ✅ Performance = hand-written code

---

## Comparison to Other Languages

| Feature | Scheme | Rust | Haskell | Guage |
|---------|--------|------|---------|-------|
| Macro hygiene | ✅ | ✅ | N/A | ✅ (De Bruijn) |
| Pattern matching | Basic | ✅ | ✅ | ✅ (planned) |
| Zero-cost generics | ❌ | ✅ | Partial | ✅ (planned) |
| Symbolic syntax | ❌ | ❌ | ❌ | ✅ |
| friendly | Low | Medium | Medium | High |
| Self-hosting | ✅ | ✅ | ✅ | 🎯 Goal |

---

## Conclusion

**We have a clear path from "Turing complete lambda calculus" to "industrial-strength metaprogramming language" in 18 weeks.**

The research is complete. The design is sound. The implementation plan is detailed.

**Ready to begin Phase 1: Pattern Matching.**

---

**For detailed information:**
- Research: `METAPROGRAMMING_RESEARCH.md`
- Plan: `METAPROGRAMMING_IMPLEMENTATION_PLAN.md`
- Spec: `SPEC.md` (updated)
- Philosophy: `CLAUDE.md`
