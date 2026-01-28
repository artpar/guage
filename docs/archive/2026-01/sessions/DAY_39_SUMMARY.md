# Day 39: S-Expression Parser in Guage - IN PROGRESS

**Date:** 2026-01-27
**Status:** Parser structure complete, character representation challenges identified
**Time:** ~3 hours
**Next Steps:** Resolve character comparison issues or simplify approach

## Summary

Began implementing an S-expression parser written entirely in Guage as the first step toward self-hosting. Created complete parser structure with tokenization and parsing phases, but encountered fundamental challenges with character representation that need to be resolved.

## What Was Built

### Parser Structure (stdlib/parser.scm)

**Phase 1: Character Classification** (4 functions)
- `≈⊙space?` - Whitespace detection
- `≈⊙digit?` - Digit detection (0-9)
- `≈⊙paren?` - Parenthesis detection
- `≈⊙special?` - Special delimiter detection

**Phase 2: Tokenization Helpers** (7 functions)
- `≈⊙→token` - Token construction
- `≈⊙token-type` - Token type extraction
- `≈⊙token-val` - Token value extraction
- `≈⊙skip-ws` - Whitespace skipping
- `≈⊙skip-comment` - Comment skipping (;)
- `≈⊙read-number` - Number tokenization
- `≈⊙read-symbol` - Symbol tokenization
- `≈⊙read-string` - String literal tokenization

**Phase 3: Tokenizer** (2 functions)
- `≈⊙tokenize-one` - Single token extraction
- `≈⊙tokenize` - Full string tokenization

**Phase 4: Parser** (3 functions)
- `≈⊙parse-one` - Single expression parsing
- `≈⊙parse-list` - List parsing (until rparen)
- `≈⊙parse` - Top-level parse function

**Total:** 18 functions, 280 lines of code

### Test Suite (tests/test_parser.scm)

Created 15 test cases covering:
- Tokenization (5 tests)
- Parsing (5 tests)
- Integration (3 tests)
- Error handling (2 tests)

## Technical Challenges Encountered

### Challenge 1: Character Representation in Guage

**Problem:** String indexing with `≈→` returns character symbols, but symbol representation is inconsistent.

**Examples:**
```scheme
(≈→ "hello" #0)  ; → :h
(≈→ "  " #0)     ; → :  (space as symbol)
(≈→ "42" #0)     ; → :4

; But these don't match:
(≡ (≈→ "42" #0) (⌜ :4))      ; → #f (FALSE!)
(≡ (≈→ "42" #0) (⌜ :#4))     ; → #f (FALSE!)

; This works:
(≡ (≈→ "42" #0) (≈→ "4" #0)) ; → #t (TRUE)
```

**Impact:** Cannot use `⌜` (quote) to create character symbols for comparison. Must use `(≈→ string #0)` pattern for ALL character literals.

### Challenge 2: Quoting Parentheses

**Problem:** Cannot quote parenthesis characters directly:
```scheme
(⌜ :#\()  ; Breaks parser - unclosed paren
```

**Solution:** Use string indexing instead:
```scheme
(≈→ "(" #0)  ; Works correctly
```

### Challenge 3: Local Bindings

**Problem:** Initially used `≔` for local bindings inside functions, which creates globals.

**Solution:** Use nested lambda bindings:
```scheme
; Wrong:
(λ (x) (≔ y (compute x)) (use y))

; Correct:
(λ (x) ((λ (y) (use y)) (compute x)))
```

### Challenge 4: Macro Availability

**Problem:** Parser uses `∨…` macro, which must be loaded first.

**Solution:** Require `stdlib/macros.scm` before `stdlib/parser.scm`.

## Lessons Learned

### 1. Symbol Representation Inconsistency

Guage's symbol system has subtle inconsistencies:
- Quoted symbols `(⌜ :name)` create named symbols
- Character symbols from `≈→` are NOT equal to quoted symbols
- Must extract characters from strings for comparison

### 2. String Primitives Work Well

The string primitives (`≈#`, `≈→`, `≈⊂`, `≈⊕`, `≈?`, `≈≡`) are functional and sufficient for parsing, once character comparison issues are resolved.

### 3. Lambda Bindings Are Verbose

Guage's lack of local binding syntax (`let`, `letrec`) makes complex functions deeply nested:
```scheme
((λ (a)
  ((λ (b)
    ((λ (c)
      body)
     compute-c))
   compute-b))
 compute-a)
```

This is correct but hard to read. A `≔↓` (let) macro would help significantly.

### 4. Parser Architecture Is Sound

Despite character issues, the overall parser architecture is solid:
- Separate tokenization and parsing phases
- Token data structures work correctly
- Recursive descent parsing structure is appropriate
- Error handling with `⚠` values integrates well

## Current Status

### What Works ✅
- Parser structure compiles and loads
- Lambda bindings correctly implemented
- Token data structure works
- Nested function definitions work
- String primitives functional

### What Doesn't Work ❌
- Character comparison using `⌜` for quotes
- Parser functions not all loading due to char issues
- Tests cannot run without working parser
- Digit/symbol classification incomplete

### What Needs Fixing
1. **Rewrite character comparisons** - Use `(≈→ "c" #0)` pattern everywhere
2. **Fix digit detection** - Cannot use `(⌜ :#0)` through `(⌜ :#9)`
3. **Fix quote detection** - Cannot use `(⌜ :#')` or `(⌜ :#")`
4. **Test loading** - Verify all 18 functions load correctly
5. **Run tests** - Validate parser behavior

## Next Steps

### Immediate (1-2 hours)
1. Rewrite all character comparisons to use `(≈→ "literal" #0)` pattern
2. Test that all 18 parser functions load successfully
3. Run basic tokenization tests
4. Fix any remaining issues

### Follow-up (2-3 hours)
1. Complete test suite execution
2. Add string-to-number conversion (currently returns strings)
3. Add proper symbol creation (currently returns strings)
4. Test parsing real Guage code

### Alternative Approach
If character issues persist, consider:
- **String-based approach:** Work with string slices instead of characters
- **Simpler parser:** Focus on just lists and atoms, skip strings/comments
- **Native helper:** Add a string classification primitive to C runtime

## Files Modified

- `stdlib/parser.scm` - 280 lines, 18 functions
- `tests/test_parser.scm` - 60 lines, 15 tests
- `test_minimal_parser.scm` - Debug file
- `test_parser_step.scm` - Debug file
- `stdlib/parser_min.scm` - Minimal test version

## Statistics

**Code Written:** ~340 lines Guage code
**Functions Defined:** 18 parser functions
**Test Cases:** 15 comprehensive tests
**Time Spent:** ~3 hours
**Bugs Identified:** 4 major (character representation issues)
**Architecture:** ✅ Sound
**Implementation:** ⚠ Needs character comparison fixes

## Recommendations

### For Completing Day 39

**Option A: Fix Character Issues** (Recommended)
- Time: 1-2 hours
- Systematically rewrite all character comparisons
- Use `(≈→ string #0)` pattern consistently
- Test incrementally as we go

**Option B: Simplify Parser**
- Time: 1 hour
- Remove string literal support
- Remove comment support
- Focus on lists, symbols, numbers only
- Prove basic parsing works

**Option C: Add Native Helper**
- Time: 2 hours
- Add `≈⊙char→` primitive in C
- Convert string index to comparable value
- Update parser to use new primitive

### For Day 40

Once parser works, proceed to simple evaluator:
- Load parsed AST
- Environment as association list
- Implement eval for:
  - Numbers (return as-is)
  - Symbols (lookup in environment)
  - Lists (function application)
  - λ expressions (create closures)

## Commit Message (When Complete)

```
feat: implement S-expression parser in Guage - Day 39 in progress

- Create 18-function parser written in pure Guage
- Tokenization phase: whitespace, comments, parens, strings, numbers, symbols
- Parsing phase: recursive descent for S-expressions
- Identify character representation challenges in Guage

Technical challenges:
- Symbol comparison inconsistencies identified
- Parenthesis quoting issues resolved
- Lambda binding patterns established
- String primitives validated

Ready for character comparison fixes to complete implementation.

Co-Authored-By: Claude Sonnet 4.5 <noreply@anthropic.com>
```

---

**Day 39 Status: IN PROGRESS** ⚠️

**Blocking Issue:** Character representation/comparison needs systematic fix
**Path Forward:** Option A (Fix Character Issues) recommended
**Time to Complete:** 1-2 hours estimated

