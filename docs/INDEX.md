# Guage Documentation Index

---
**Status:** CURRENT - Single source of truth for documentation navigation
**Last Updated:** 2026-01-28 (Day 46)
**Purpose:** Navigation hub + documentation governance
---

## 📍 Start Here

**Starting a new session?**
- Quick start → [../START_SESSION.txt](../START_SESSION.txt) (copy this prompt!)
- Full methodology → [../SESSION_START_PROMPT.md](../SESSION_START_PROMPT.md)

**New to Guage?**
- Project overview → [../README.md](../README.md)
- Language specification → [../SPEC.md](../SPEC.md)
- Philosophy & principles → [../CLAUDE.md](../CLAUDE.md)

**Current State:**
- Latest progress & status → [../SESSION_HANDOFF.md](../SESSION_HANDOFF.md)

## 📚 Reference Documentation (Stable)

Deep-dive technical documentation that changes infrequently:

- [Auto-Documentation Guide](reference/AUTO_DOCUMENTATION_GUIDE.md) - Complete guide to ⌂, ⌂∈, ⌂≔, ⌂⊛ (NEW!)
- [Auto-Test Guide](reference/AUTO_TEST_GUIDE.md) - Complete guide to ⌂⊨ test generation (NEW!)
- [Metaprogramming Vision](reference/METAPROGRAMMING_VISION.md) - Vision, native features, long-term goals
- [Data Structures](reference/DATA_STRUCTURES.md) - Structure primitives (⊙, ⊚, ⊝)
- [Keywords](reference/KEYWORDS.md) - Self-evaluating symbols specification
- [Symbolic Vocabulary](reference/SYMBOLIC_VOCABULARY.md) - Symbol meanings and Unicode
- [Technical Decisions](reference/TECHNICAL_DECISIONS.md) - Design rationale and trade-offs

## 🎯 Active Planning (Current Work)

Current roadmaps and active tasks:

- [Week 3 Roadmap](planning/WEEK_3_ROADMAP.md) - Pattern matching implementation plan
- [TODO](planning/TODO.md) - Task tracking and priorities

## 🗄️ Historical Archive

Organized by date for easy reference:

### January 2026
- [Audits](archive/2026-01/audits/) - Consistency, correctness, completeness audits
- [Plans](archive/2026-01/plans/) - Daily plans and session summaries
- [Sessions](archive/2026-01/sessions/) - Session end notes and handoffs

### Phase Documents
- [Phases](archive/phases/) - Phase 1, 2A, 2B, 2C completion records

---

## 📋 Documentation Patterns & Governance

**Critical:** Follow these rules to prevent duplication and maintain organization.

**Quick Reference:** See [DOC_PATTERNS.md](DOC_PATTERNS.md) for naming conventions and examples.

### Rule 1: Single Source of Truth

Each type of information has ONE canonical location:

| Information Type | Canonical Location | Update Frequency |
|------------------|-------------------|------------------|
| Current progress/status | `SESSION_HANDOFF.md` | Every session |
| Language specification | `SPEC.md` | When primitives/syntax change |
| Philosophy & principles | `CLAUDE.md` | Rarely (fundamental changes only) |
| Project overview | `README.md` | Milestones only |
| Navigation | `docs/INDEX.md` | When docs reorganized |
| Deep-dive technical | `docs/reference/*.md` | When feature complete |
| Active plans/roadmaps | `docs/planning/*.md` | During planning phase |
| Historical records | `docs/archive/YYYY-MM/` | Never (write once) |

### Rule 2: Where New Documents Go

**Before creating a new document, ask:**

1. **Does this information already exist?**
   - If YES → Update existing doc (see Rule 1)
   - If NO → Continue to question 2

2. **How long will this information be relevant?**
   - **1-7 days** → Temporary (goes to `docs/archive/` at session end)
   - **1-4 weeks** → Active planning (goes to `docs/planning/`)
   - **Months/years** → Reference (goes to `docs/reference/`)

3. **What category does it fit?**

   **Temporary (Archive Next Session):**
   - Daily plans → `docs/archive/YYYY-MM/plans/DAY_N_PLAN.md`
   - Session summaries → `docs/archive/YYYY-MM/sessions/SESSION_END_DAY_N.md`
   - Audit reports → `docs/archive/YYYY-MM/audits/YYYY-MM-DD_NAME_AUDIT.md`
   - Phase retrospectives → `docs/archive/phases/PHASE_X_COMPLETE.md`

   **Active Planning (Archive When Complete):**
   - Week/sprint roadmaps → `docs/planning/WEEK_N_ROADMAP.md`
   - TODO lists → `docs/planning/TODO.md` (single file, updated continuously)
   - Implementation plans → `docs/planning/FEATURE_IMPLEMENTATION.md`

   **Reference (Rarely Change):**
   - Feature specifications → `docs/reference/FEATURE_NAME.md`
   - Design rationale → Update `docs/reference/TECHNICAL_DECISIONS.md`
   - Symbol catalogs → Update `docs/reference/SYMBOLIC_VOCABULARY.md`
   - Metaprogramming vision → Update `docs/reference/METAPROGRAMMING_VISION.md`

   **Living Documents (Never Archive):**
   - Current status → Update `SESSION_HANDOFF.md`
   - Language spec → Update `SPEC.md`
   - Philosophy → Update `CLAUDE.md`
   - Project intro → Update `README.md`

### Rule 3: Prevent Duplication

**NEVER copy information between documents.** Instead:

1. **Link to canonical source:**
   ```markdown
   For current status, see [SESSION_HANDOFF.md](../SESSION_HANDOFF.md#current-status)
   ```

2. **Summarize with link:**
   ```markdown
   Brief summary here (2-3 sentences).

   Full details: [SPEC.md](../SPEC.md#section-name)
   ```

3. **Reference, don't repeat:**
   ```markdown
   As documented in [TECHNICAL_DECISIONS.md](reference/TECHNICAL_DECISIONS.md),
   we chose De Bruijn indices because...
   ```

### Rule 4: Status Headers (Required)

**Every document must have a status header:**

```markdown
---
Status: CURRENT | ARCHIVED | REFERENCE
Created: YYYY-MM-DD
Updated: YYYY-MM-DD
Supersedes: [filename] (if replaces older doc)
Archived: YYYY-MM-DD (if archived)
---
```

**Status Definitions:**
- **CURRENT** - Active, up-to-date, check this first (only 4-6 docs should have this)
- **ARCHIVED** - Historical record, for reference only (never updated)
- **REFERENCE** - Stable, infrequently updated, deep-dive content

### Rule 5: When to Archive

**Archive immediately after:**
- Session ends → Move session notes to `archive/YYYY-MM/sessions/`
- Plan completed → Move plan to `archive/YYYY-MM/plans/`
- Audit done → Move to `archive/YYYY-MM/audits/`
- Phase complete → Move to `archive/phases/`
- Week complete → Move roadmap to `archive/YYYY-MM/plans/`

**How to archive:**
1. Move file to appropriate archive folder
2. Add `Archived: YYYY-MM-DD` to status header
3. Update STATUS: CURRENT → ARCHIVED
4. Update INDEX.md if navigation changed
5. Update SESSION_HANDOFF.md to reference new location

### Rule 6: Naming Conventions

**Consistent naming makes files easy to find:**

```
# Temporary/Archived:
DAY_N_PLAN.md                    # Daily plans
DAY_N_SUMMARY.md                 # Daily summaries
SESSION_END_DAY_N.md             # Session notes
YYYY-MM-DD_NAME_AUDIT.md         # Audit reports
PHASE_X_COMPLETE.md              # Phase retrospectives

# Active Planning:
WEEK_N_ROADMAP.md                # Week/sprint plans
FEATURE_IMPLEMENTATION.md        # Implementation plans
TODO.md                          # Single task list

# Reference:
FEATURE_NAME.md                  # Feature deep-dive
TECHNICAL_DECISIONS.md           # Design rationale
SYMBOLIC_VOCABULARY.md           # Symbol catalog
```

### Rule 7: Update INDEX.md

**Update this file when:**
- New reference doc added
- Active plan created/completed
- Navigation structure changes
- Quick Status section needs update

**Don't update for:**
- Archived documents (they're in dated folders)
- Minor doc updates
- Session notes

---

## 📊 Quick Status

**Last Updated:** 2026-01-28 (Day 46 - Stack Overflow Fixed!)

**System State:**
- **Primitives:** 79 functional (added ÷ integer division)
- **Tests:** 27/33 passing (82% - stack overflow fixed!)
- **Build:** O2 optimized, 32MB stack
- **Status:** ✅ Turing complete + Auto-doc + String/List libraries + **Production architecture plan ready!**

**Recent Milestones:**
- Day 46: Stack overflow FIXED (32MB stack + O2) + Sort bugs fixed + Trampoline plan created
- Day 45: Advanced list utilities (14 functions, 47 tests)
- Day 44: String library complete (8 functions, 43 tests)
- Day 43: Provenance fix for REPL functions
- Day 42: Auto-doc deep dive (650+ lines guides)
- Day 41: Parser complete (15 tests, 3 bugs fixed)
- Days 15-19: Pattern matching complete (165 tests)

**Next Steps:**
- Day 43: Fix ⌂⊛ provenance for REPL functions OR expand stdlib
- Day 44-45: Property-based testing primitives or markdown export
- Day 46+: Continue building practical utilities

**Overall Progress:**
- **Auto-Documentation:** Complete guides + working formatters
- **Week 6:** Day 42 complete
- **Focus:** Making Guage truly usable for real development work

---

## 🔗 Quick Links by Task

**I want to...**
- Understand the project → [README.md](../README.md)
- Learn the language → [SPEC.md](../SPEC.md)
- Know current status → [SESSION_HANDOFF.md](../SESSION_HANDOFF.md)
- Understand philosophy → [CLAUDE.md](../CLAUDE.md)
- See what's next → [planning/WEEK_3_ROADMAP.md](planning/WEEK_3_ROADMAP.md)
- Look up a symbol → [reference/SYMBOLIC_VOCABULARY.md](reference/SYMBOLIC_VOCABULARY.md)
- Understand structures → [reference/DATA_STRUCTURES.md](reference/DATA_STRUCTURES.md)
- See long-term vision → [reference/METAPROGRAMMING_VISION.md](reference/METAPROGRAMMING_VISION.md)
- Review past decisions → [reference/TECHNICAL_DECISIONS.md](reference/TECHNICAL_DECISIONS.md)
- Find historical info → [archive/2026-01/](archive/2026-01/)

---

**Documentation Governance:** These rules ensure docs stay organized, up-to-date, and duplicate-free. When in doubt, consult this file or ask in session notes.
