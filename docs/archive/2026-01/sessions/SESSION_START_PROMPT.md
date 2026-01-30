## Context

I'm working on **Guage**, a Turing-complete ultralanguage with pure symbolic syntax.

**Read these files first (in order):**
1. `SESSION_HANDOFF.md` - Current status, recent progress, what's next
2. `SPEC.md` - Language specification, primitives, syntax
3. `docs/INDEX.md` - Documentation navigation and governance

**Reference as needed:**
- `CLAUDE.md` - Philosophy and principles
- `docs/reference/` - Deep technical docs
- `docs/planning/` - Active roadmaps

## Current State

**Read `SESSION_HANDOFF.md` for up-to-date status** - it contains:
- Primitive count and test count
- Recent milestones and achievements
- What's next (Day N+1 options)
- Complete progress history

**Quick check:** Look for the "Executive Summary" and "What's Next 🎯" sections.

## Working Methodology

Follow this approach for **predictable, incremental progress:**

### 1. Feature-by-Feature Development

**For each feature:**

1. **Read context:**
   - Check if feature is in SPEC.md
   - Check if implementation exists in code
   - Check for related tests
   - Check for related docs

2. **Plan incrementally:**
   - Break into smallest testable units
   - Identify dependencies
   - Design test strategy first

3. **Implement test-first:**
   - Write test in `tests/test_[feature].scm`
   - Run test (should fail)
   - Implement feature
   - Run test (should pass)
   - Verify no regressions

4. **Document changes:**
   - Update SPEC.md if syntax/primitives changed
   - Add to SESSION_HANDOFF.md under "What's New"
   - Update relevant reference docs if needed

5. **Verify:**
   - All existing tests still pass
   - New tests pass
   - No memory leaks
   - Clean compilation

### 2. Test-by-Test Validation

**Never skip testing:**

```bash
# Single test file
./guage < tests/test_[feature].scm

# All tests
./run_tests.sh

# Memory check (if available)
valgrind --leak-check=full ./guage < tests/test_[feature].scm
```

**Test patterns:**
- Unit tests for primitives
- Integration tests for features
- Edge cases (zero, nil, empty, error)
- Regression tests for fixed bugs

### 3. Documentation Updates

**Update documentation systematically:**

**After every feature:**
- `SESSION_HANDOFF.md` - Add to "What's New This Session"
- Update primitive count/test count
- Add example if significant

**When primitives/syntax change:**
- `SPEC.md` - Update primitive table
- `SPEC.md` - Add examples
- Mark primitive status (✅ DONE)

**When adding reference material:**
- Create/update docs in `docs/reference/`
- Add status header
- Link from `docs/INDEX.md`

**Session end:**
- Update `SESSION_HANDOFF.md` executive summary
- Archive completed plans to `docs/archive/YYYY-MM/`
- Update `docs/INDEX.md` Quick Status

### 4. Commit Strategy

**Commit after each complete feature:**

```bash
# Stage changes
git add [specific files]

# Commit with clear message
git commit -m "feat: [feature name] - [brief description]

- [What was implemented]
- [What was tested]
- [What was documented]

Co-Authored-By: Claude Sonnet 4.5 <noreply@anthropic.com>"
```

**Commit types:**
- `feat:` - New feature
- `fix:` - Bug fix
- `test:` - Add/update tests
- `docs:` - Documentation only
- `refactor:` - Code restructure

## Documentation Governance

**Follow these rules (from docs/INDEX.md):**

1. **Single Source of Truth:**
   - Current status → `SESSION_HANDOFF.md`
   - Language spec → `SPEC.md`
   - Philosophy → `CLAUDE.md`
   - Navigation → `docs/INDEX.md`

2. **Where New Docs Go:**
   - Temporary (1-7 days) → Archive after session
   - Active planning (1-4 weeks) → `docs/planning/`
   - Reference (months/years) → `docs/reference/`
   - Never duplicate → Link instead

3. **Naming Conventions:**
   - No vague terms (advanced, new, better, temp)
   - Be specific and descriptive
   - See `docs/DOC_PATTERNS.md` for examples

4. **Status Headers:**
   ```markdown
   ---
   Status: CURRENT | ARCHIVED | REFERENCE
   Created: YYYY-MM-DD
   Updated: YYYY-MM-DD
   Purpose: Brief description
   ---
   ```

5. **Archive Immediately:**
   - After session → Move to `docs/archive/YYYY-MM/sessions/`
   - After plan complete → Move to `docs/archive/YYYY-MM/plans/`
   - After audit → Move to `docs/archive/YYYY-MM/audits/`

## Principles (From CLAUDE.md)

**Core principles to follow:**

1. **Pure Symbols Only** - No English keywords
2. **First-Class Everything** - Functions, errors, tests, CFG/DFG all values
3. **Single Source of Truth** - No duplication
4. **Development-First** - Never lint, human tests, hot-reload assumed
5. **Values as Boundaries** - All interfaces use simple values
6. **Mathematical Foundation** - Lambda calculus, De Bruijn indices

**Code style:**
- De Bruijn indices in runtime (0, 1, 2...)
- Named params in documentation only (𝕩, 𝕪, 𝕫)
- Reference counting for memory management
- Errors as values, not exceptions
- Tests as first-class values (data)

## Session Workflow

**Start of session:**
1. Read `SESSION_HANDOFF.md`
2. Check "What's Next" section
3. Verify current state (tests passing, clean build)
4. Choose next feature from plan
5. Follow feature-by-feature methodology

**During session:**
- Work on ONE feature at a time
- Test after each change
- Document as you go
- Commit when feature complete

**End of session:**
1. Update `SESSION_HANDOFF.md`:
   - Executive summary
   - "What's New This Session"
   - Test counts
   - "What's Next"
   - Status line at end

2. Archive session notes:
   ```bash
   mv SESSION_NOTES.md docs/archive/YYYY-MM/sessions/SESSION_END_DAY_N.md
   ```

3. Update `docs/INDEX.md` Quick Status:
   - Primitives count
   - Tests count
   - Recent milestones

4. Commit documentation:
   ```bash
   git add SESSION_HANDOFF.md docs/
   git commit -m "docs: Session [N] complete - [brief summary]"
   ```

## Success Criteria

**Every feature should:**
- ✅ Have passing tests
- ✅ Be documented in SPEC.md
- ✅ Be recorded in SESSION_HANDOFF.md
- ✅ Not break existing tests
- ✅ Follow naming conventions
- ✅ Use symbolic syntax only

**Every session should:**
- ✅ Update SESSION_HANDOFF.md
- ✅ Archive completed work
- ✅ Commit changes
- ✅ Leave system in clean state
- ✅ Document what's next

## Common Tasks

### Adding a New Primitive

1. **Design:**
   - Choose symbol (check SYMBOLIC_VOCABULARY.md)
   - Define type signature
   - Write specification

2. **Implement:**
   ```c
   // In primitives.c
   Cell* prim_symbol(Cell* args, Env* env) {
       // Implementation
   }

   // Register in init_primitives()
   env_define(env, symbol_create("symbol"),
              primitive_create(prim_symbol, "symbol"));
   ```

3. **Test:**
   ```scheme
   ; tests/test_primitive.scm
   (⊨ :test-basic #t (primitive-works))
   (⊨ :test-edge #t (handles-edge-case))
   ```

4. **Document:**
   - Update SPEC.md primitive table
   - Add to SESSION_HANDOFF.md
   - Update primitive count

5. **Verify:**
   ```bash
   ./guage < tests/test_primitive.scm
   ./run_tests.sh
   ```

### Fixing a Bug

1. **Reproduce:**
   - Write failing test first
   - Confirm bug exists

2. **Fix:**
   - Implement fix
   - Test passes

3. **Regress:**
   - Add regression test
   - Document in SESSION_HANDOFF.md

4. **Commit:**
   ```bash
   git commit -m "fix: [bug description]

   - [What was broken]
   - [How it was fixed]
   - [Regression test added]"
   ```

### Adding Documentation

1. **Check existing:**
   - Does this info exist already?
   - If yes → Update existing (single source of truth)
   - If no → Continue

2. **Determine category:**
   - Temporary → Will archive next session
   - Planning → `docs/planning/`
   - Reference → `docs/reference/`

3. **Name clearly:**
   - No vague terms (advanced, new, temp)
   - Specific and descriptive
   - See `docs/DOC_PATTERNS.md`

4. **Add status header:**
   ```markdown
   ---
   Status: CURRENT | ARCHIVED | REFERENCE
   Created: YYYY-MM-DD
   Updated: YYYY-MM-DD
   Purpose: Brief description
   ---
   ```

5. **Link from INDEX.md** (if reference doc)

## Quick Reference Commands

```bash
# Build
make clean && make

# Run REPL
./guage

# Run specific test
./guage < tests/test_[feature].scm

# Run all tests
./run_tests.sh

# Check documentation structure
ls -R docs/

# Verify links in markdown
grep -r "\.md" --include="*.md" docs/ | grep -v "archive"

# Count primitives
grep "✅ DONE" SPEC.md | wc -l

# Count tests
find tests/ -name "*.scm" | wc -l
```

## Error Recovery

**If tests fail:**
1. Don't commit
2. Fix the issue
3. Verify fix with tests
4. Document what was wrong

**If documentation is unclear:**
1. Check `docs/INDEX.md` for navigation
2. Check `docs/DOC_PATTERNS.md` for conventions
3. Ask in session notes

**If unsure about design:**
1. Check `SPEC.md` for precedent
2. Check `docs/reference/TECHNICAL_DECISIONS.md`
3. Document decision in SESSION_HANDOFF.md
4. Can add to TECHNICAL_DECISIONS.md if significant

## Ready to Start

With this context, I'm ready to:
1. Read `SESSION_HANDOFF.md` to understand current state
2. Work feature-by-feature with tests
3. Document changes systematically
4. Follow governance rules
5. Leave system in clean, documented state

**What should we work on next?**