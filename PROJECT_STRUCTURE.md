# Guage Project Structure

## Working Directory: ALWAYS Project Root

**Important:** All commands, paths, and operations are designed to work from the project root directory (`/path/to/guage/`). Never change directories.

## Directory Layout

```
guage/                          # PROJECT ROOT (work from here)
├── Makefile                    # Build and test commands (run from root)
├── README.md                   # Project overview
├── SPEC.md                     # Language specification
├── CLAUDE.md                   # Development philosophy
├── SESSION_HANDOFF.md          # Current status
├── PROJECT_STRUCTURE.md        # This file
│
└── bootstrap/                  # C implementation
    ├── Makefile                # Bootstrap-specific build (not used directly)
    ├── run_tests.sh            # Test runner (call from root via make)
    ├── guage                   # Executable (generated)
    ├── *.c, *.h                # Source files
    │
    ├── stdlib/                 # Standard library (Guage code)
    │   ├── list.scm
    │   ├── sort.scm
    │   └── ...
    │
    └── tests/                  # Test suite
        ├── basic.test
        ├── list-advanced.test
        └── ...
```

## Command Reference (All from Project Root)

### Building
```bash
make                 # Build interpreter
make build           # Same as make
make rebuild         # Clean and rebuild
make clean           # Remove build artifacts
```

### Testing
```bash
make test            # Run full test suite (33 tests)
make test-one TEST=bootstrap/tests/basic.test  # Run single test
make test-summary    # Show only summary
make smoke           # Quick smoke test
```

### Running
```bash
make repl            # Start REPL
bootstrap/guage < bootstrap/tests/basic.test   # Run test directly
echo '(⊕ #1 #2)' | bootstrap/guage            # Evaluate expression
```

### Loading Files in REPL
```scheme
(⋘ "bootstrap/stdlib/list.scm")    ; Load from project root
```

## Path Conventions

All paths are **relative to project root**:
- Executable: `bootstrap/guage`
- Test files: `bootstrap/tests/*.test`
- Standard library: `bootstrap/stdlib/*.scm`
- Load in REPL: `(⋘ "bootstrap/stdlib/...")`

## No Dual Paths

**Single source of truth:**
- Work directory: Project root only
- No `cd` commands in Makefile
- No `cd` commands in scripts
- All paths relative to project root
- No context switching between directories

## Quick Start

```bash
# Clone and build (from project root)
git clone <repo>
cd guage
make

# Run tests
make test

# Start REPL
make repl

# In REPL:
(⋘ "bootstrap/stdlib/list.scm")
((↦ (λ (x) (⊗ x x))) (⟨⟩ #1 (⟨⟩ #2 (⟨⟩ #3 ∅))))
```

## Why Project Root?

1. **Clarity:** No confusion about current directory
2. **Consistency:** All commands work the same way
3. **Simplicity:** No directory switching needed
4. **Predictability:** Paths always mean the same thing
5. **No dual paths:** Single source of truth
