---
Status: REFERENCE
Created: 2026-01-27
Updated: 2026-01-27
Purpose: Quick reference for documentation naming and organization
---

# Documentation Patterns & Naming Conventions

**Rule:** Document names must be clear, specific, and reflective of content. No vague terms!

## ❌ Avoid Vague Terms

Never use these meaningless qualifiers in document names:
- **advanced**, **basic**, **simple** - Subjective and unclear
- **new**, **old**, **latest** - Becomes outdated immediately
- **better**, **improved**, **updated** - What was improved?
- **temp**, **tmp**, **draft** - Use archive folders instead
- **misc**, **other**, **stuff** - Too vague
- **part1**, **part2** - Use descriptive names instead

## ✅ Use Clear, Descriptive Names

Good names answer: **"What specifically is this about?"**

### Examples: Good vs Bad

**Bad:** `ADVANCED_FEATURES.md`
- What features? Advanced to whom?

**Good:** `METAPROGRAMMING_VISION.md`
- Clear: It's about metaprogramming vision/long-term goals

**Bad:** `IMPROVEMENTS.md`
- What was improved? When?

**Good:** `PHASE_2C_COMPLETE_STATUS.md`
- Clear: Status report for Phase 2C completion

**Bad:** `NEW_DESIGN.md`
- New compared to what? When?

**Good:** `PHASE2B_DESIGN.md`
- Clear: Design document for Phase 2B

**Bad:** `TEMP_NOTES.md`
- Temporary what? Until when?

**Good:** `SESSION_END_DAY_13.md` in `archive/2026-01/sessions/`
- Clear: Session notes from specific day, clearly archived

## Naming Patterns by Category

### Living Documents (Root)
```
README.md                    # Project overview
SPEC.md                      # Language specification
CLAUDE.md                    # Philosophy & principles
SESSION_HANDOFF.md           # Current status
```

**Pattern:** Single-word or well-known abbreviation, ALL CAPS

### Reference Documents (docs/reference/)
```
METAPROGRAMMING_VISION.md    # Long-term metaprogramming features
DATA_STRUCTURES.md           # Structure primitives specification
KEYWORDS.md                  # Self-evaluating symbols
SYMBOLIC_VOCABULARY.md       # Complete symbol catalog
TECHNICAL_DECISIONS.md       # Design rationale log
```

**Pattern:** TOPIC_NAME.md or TOPIC_CATEGORY.md

### Planning Documents (docs/planning/)
```
WEEK_3_ROADMAP.md           # Week/sprint roadmap (number + scope)
TODO.md                     # Task tracking (single file)
PATTERN_MATCHING_IMPL.md    # Feature implementation plan
```

**Pattern:** SCOPE_TYPE.md or FEATURE_IMPL.md

### Archived Documents

**Session Notes:**
```
SESSION_END_DAY_10.md       # Session from Day 10
SESSION_SUMMARY_2026-01-27.md  # Session from specific date
```
**Pattern:** SESSION_END_DAY_N.md or SESSION_SUMMARY_YYYY-MM-DD.md

**Plans:**
```
DAY_13_PLAN.md              # Daily plan
DAY_13_FIXES_COMPLETE.md    # Specific milestone
CONSISTENCY_AUDIT_PLAN.md   # Specific task plan
```
**Pattern:** DAY_N_TYPE.md or SPECIFIC_TASK_PLAN.md

**Audits:**
```
2026-01-27_CONSISTENCY_AUDIT.md    # Dated audit report
2026-01-27_CORRECTNESS_AUDIT.md    # Dated audit report
```
**Pattern:** YYYY-MM-DD_TYPE_AUDIT.md

**Phases:**
```
PHASE1_COMPLETE.md          # Phase completion record
PHASE2B_DESIGN.md           # Phase design document
PHASE2C_PROGRESS.md         # Phase progress tracking
```
**Pattern:** PHASE_NAME_TYPE.md

## File Name Checklist

Before creating a new document, ask:

1. **Is the name self-explanatory?**
   - ✅ Can someone understand the content from the name alone?
   - ❌ Does it use vague qualifiers (advanced, new, temp)?

2. **Will the name age well?**
   - ✅ Will it still make sense in 6 months?
   - ❌ Does it reference "current" or "latest"?

3. **Is it unique and specific?**
   - ✅ Is this the only doc that could have this name?
   - ❌ Could multiple docs have similar names?

4. **Does it follow patterns?**
   - ✅ Does it match existing naming conventions?
   - ❌ Does it introduce a new pattern unnecessarily?

## Examples in Practice

### Good Naming Evolution

1. **Initial:** `METAPROGRAMMING.md`
   - **Issue:** Too broad, what about metaprogramming?

2. **Better:** `ADVANCED_METAPROGRAMMING.md`
   - **Issue:** "Advanced" is vague - advanced compared to what?

3. **Best:** `METAPROGRAMMING_VISION.md`
   - **Clear:** Long-term vision for metaprogramming features
   - **Specific:** Not basic features, not current, but vision/goals
   - **Ages well:** Still makes sense in 2 years

### Real-World Example

**Need:** Document pattern matching implementation
**Bad names:**
- `NEW_FEATURES.md` - What features?
- `IMPROVEMENTS.md` - What was improved?
- `PATTERN_STUFF.md` - Too vague
- `ADVANCED_PATTERNS.md` - Advanced to whom?

**Good names:**
- `PATTERN_MATCHING_IMPLEMENTATION.md` - Clear and specific
- `WEEK_3_PATTERN_MATCHING.md` - Time-scoped and specific
- `PATTERN_MATCHING_DESIGN.md` - Specific phase/aspect

## When in Doubt

1. **Be specific:** What exactly is this about?
2. **Use context:** Phase? Week? Feature name?
3. **Think long-term:** Will this make sense in 6 months?
4. **Ask:** Can I describe the content in the filename itself?

**See also:** [docs/INDEX.md](INDEX.md) for full documentation governance rules
