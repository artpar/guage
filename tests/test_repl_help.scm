; Test Suite: REPL Help System
; Tests for enhanced REPL commands (Day 32)
; This file documents the REPL help commands - run manually to test

; NOTE: This is NOT a test suite that can be run automatically.
; These are REPL commands that must be tested interactively.
; Run with: echo ":help" | ./guage

; ============================================================================
; REPL Commands Documentation
; ============================================================================

; Command: :help
; Description: Show all REPL commands
; Usage: :help
; Expected: Display help menu with all available commands

; Command: :help <symbol>
; Description: Show documentation for a specific primitive
; Usage: :help ⊕
; Expected: Display description, type signature, and arity for ⊕

; Command: :primitives
; Description: List all primitive symbols organized by category
; Usage: :primitives
; Expected: Display categorized list of all 78 primitives

; Command: :modules
; Description: List all loaded modules
; Usage: :modules
; Expected: Display numbered list of loaded module paths

; ============================================================================
; Test Cases
; ============================================================================

; Test 1: Basic help command
; Input: :help
; Expected Output:
;   ╔═══════════════════════════════════════════════════════════╗
;   ║  Guage REPL Help System                                   ║
;   ╠═══════════════════════════════════════════════════════════╣
;   ║  REPL Commands:                                           ║
;   ║    :help              List all REPL commands              ║
;   ...

; Test 2: Help for specific primitive (addition)
; Input: :help ⊕
; Expected Output:
;   ┌─────────────────────────────────────────────────────────┐
;   │ Primitive: ⊕
;   ├─────────────────────────────────────────────────────────┤
;   │ Description: Add two numbers
;   │ Type: ℕ → ℕ → ℕ
;   │ Arity: 2
;   └─────────────────────────────────────────────────────────┘

; Test 3: Help for comparison primitive
; Input: :help ≡
; Expected: Documentation for equality primitive

; Test 4: Help for list primitive
; Input: :help ⟨⟩
; Expected: Documentation for cons primitive

; Test 5: Help for unknown primitive
; Input: :help λ
; Expected: "Unknown primitive: λ" (λ is a special form, not a primitive)

; Test 6: List all primitives
; Input: :primitives
; Expected: Categorized list of all 78 primitives

; Test 7: Modules with no modules loaded
; Input: :modules
; Expected: "(no modules loaded)"

; Test 8: Modules after loading a module
; Input sequence:
;   (⋘ "../../stdlib/list.scm")
;   :modules
; Expected: "1. ../../stdlib/list.scm"

; Test 9: Modules after loading multiple modules
; Input sequence:
;   (⋘ "../../stdlib/list.scm")
;   (⋘ "../../stdlib/math.scm")
;   :modules
; Expected: Both modules listed

; Test 10: Unknown command
; Input: :unknown
; Expected: "Unknown command: :unknown"

; ============================================================================
; Integration Test Script
; ============================================================================

; To test all commands at once, run:
;
; echo -e ':help\n:primitives\n:modules\n(⋘ "../../stdlib/list.scm")\n:modules\n:help ⊕\n:help ≡' | ./guage
;
; This will:
; 1. Show help menu
; 2. List all primitives
; 3. Show no modules loaded
; 4. Load stdlib/list.scm
; 5. Show loaded module
; 6. Show docs for ⊕
; 7. Show docs for ≡

; ============================================================================
; Success Criteria
; ============================================================================

; ✓ :help displays formatted help menu
; ✓ :help <symbol> shows primitive documentation
; ✓ :help <unknown> shows helpful error message
; ✓ :primitives lists all 78 primitives in categories
; ✓ :modules shows "(no modules loaded)" when empty
; ✓ :modules lists loaded modules after ⋘
; ✓ Unknown command shows error with suggestion
; ✓ All commands work in interactive and non-interactive mode
; ✓ Commands do not interfere with normal expression evaluation
; ✓ Multi-line expressions still work correctly

; ============================================================================
; Future Enhancements (Not Yet Implemented)
; ============================================================================

; :doc <name> - Show user-defined function documentation
; :type <expr> - Show type of expression
; :exports <module> - Show symbols exported by module
; :imports - Show all imported symbols
; :provenance <symbol> - Show where symbol is defined
; :history - Show command history
; :clear - Clear screen
; :quit - Exit REPL (alternative to Ctrl+D)

; End of test documentation
