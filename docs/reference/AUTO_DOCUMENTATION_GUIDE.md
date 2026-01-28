---
Status: REFERENCE
Created: 2026-01-28
Updated: 2026-01-28
Purpose: Comprehensive guide to Guage's auto-documentation system
---

# Auto-Documentation System Guide

## Overview

Guage's auto-documentation system makes every function self-describing through **automatic analysis** of code structure. When you define a function, Guage automatically:

1. ✅ Generates a human-readable description
2. ✅ Infers the type signature
3. ✅ Extracts dependencies
4. ✅ Prints documentation to console

## Documentation Primitives

### ⌂ - Get Description

Returns a human-readable description of what a function does.

**Type:** `:symbol → :symbol`

**Example:**
```scheme
(≔ double (λ (n) (⊗ n #2)))
; Auto-prints: 📝 double :: ℕ → ℕ
;              multiply the argument and 2

(⌂ (⌜ double))  ; → :multiply the argument and 2
```

### ⌂∈ - Get Type Signature

Returns the inferred type signature using strongest typing.

**Type:** `:symbol → :symbol`

**Example:**
```scheme
(⌂∈ (⌜ double))  ; → :ℕ → ℕ
(⌂∈ (⌜ ⊕))       ; → :ℕ → ℕ → ℕ
```

**Type Inference Rules:**
1. `ℕ → ℕ` - Uses only arithmetic (⊕, ⊖, ⊗, ⊘)
2. `α → 𝔹` - Returns boolean (comparisons, predicates)
3. `α → β` - Generic polymorphic (fallback)

### ⌂≔ - Get Dependencies

Returns list of all symbols used in function body.

**Type:** `:symbol → [:symbol]`

**Example:**
```scheme
(≔ ! (λ (n) (? (≡ n #0) #1 (⊗ n (! (⊖ n #1))))))

(⌂≔ (⌜ !))  ; → ⟨:? ⟨:≡ ⟨:⌜ ⟨:⊗ ⟨:! ⟨:⊖ ∅⟩⟩⟩⟩⟩⟩
```

### ⌂⊛ - Get Provenance

Returns metadata about where a symbol was defined.

**Type:** `:symbol → ⊙[::Provenance]`

**Example:**
```scheme
; For primitives
(⌂⊛ (⌜ ⊕))  ; → ⊙[::Provenance ⟨⟨::module "<primitive>"⟩ ∅⟩]

; For module functions
(⋘ "stdlib/list.scm")
(⌂⊛ (⌜ map))
; → ⊙[::Provenance ⟨⟨::module "stdlib/list.scm"⟩
;                    ⟨⟨::line #15⟩
;                     ⟨⟨::load-order #1⟩
;                      ⟨⟨::defined-at #1737584932⟩ ∅⟩⟩⟩⟩]
```

**Provenance Fields:**
- `::module` - Module file path or "<primitive>"
- `::line` - Line number in source (parser enhancement pending)
- `::load-order` - Sequential module load number
- `::defined-at` - Unix timestamp when loaded

### ⌂⊨ - Auto-Generate Tests

Generates type-directed test cases automatically.

**Type:** `:symbol → [tests]`

**Example:**
```scheme
(⌂⊨ (⌜ ⊕))
; → ⟨⟨:⊨ ⟨::test-⊕-identity ⟨#t ⟨⟨:ℕ? ⟨⟨:⊕ ⟨#0 ⟨#0 ∅⟩⟩⟩ ∅⟩⟩ ∅⟩⟩⟩⟩
;    ⟨⟨:⊨ ⟨::test-⊕-zero ⟨#t ⟨⟨:ℕ? ⟨⟨:⊕ ⟨#0 ⟨#5 ∅⟩⟩⟩ ∅⟩⟩ ∅⟩⟩⟩⟩
;    ⟨⟨:⊨ ⟨::test-⊕-normal ⟨#t ⟨⟨:ℕ? ⟨⟨:⊕ ⟨#5 ⟨#3 ∅⟩⟩⟩ ∅⟩⟩ ∅⟩⟩⟩⟩
;    ∅⟩⟩⟩
```

## Auto-Documentation on Definition

When you define a function with `≔`, Guage automatically prints documentation:

```scheme
(≔ factorial (λ (n) (? (≡ n #0) #1 (⊗ n (factorial (⊖ n #1))))))

; Auto-prints:
; 📝 factorial :: ℕ → ℕ
;    if equals the argument and 0 then 1 else
;    multiply the argument and apply factorial to subtract the argument and 1
;    Dependencies: ?, ≡, ⌜, ⊗, factorial, ⊖
```

## Using Documentation in Code

### Query Documentation Programmatically

```scheme
; Check if function exists and has docs
(? (⚠? (⌂ (⌜ my-func)))
   (≋ "No docs available")
   (≋ (⌂ (⌜ my-func))))
```

### Compare Implementations

```scheme
; Find functions with similar type signatures
(≔ find-similar (λ (target-type)
  ; Returns list of functions with matching type
  ...))
```

### Generate API Reference

```scheme
; Document all functions in a module
(≔ doc-module (λ (module-path)
  (⋘ module-path)
  (≋ (⌂⊚ module-path))  ; List all symbols
  ; Then format docs for each...
  ))
```

## Enhanced Documentation Formatting

The `stdlib/doc_format.scm` library provides prettier output:

```scheme
(⋘ "stdlib/doc_format.scm")

; Simple format
(≋ (≈⊙doc-simple (⌜ ⊕)))
; Output:
; Symbol: ⊕
; Type: ℕ → ℕ → ℕ
; Description: Add two numbers

; Fancy format with box drawing
(≋ (≈⊙doc-format (⌜ ⊕)))
; Output:
; ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
; ┃ 📖 ⊕
; ┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
; ┃ Type: ℕ → ℕ → ℕ
; ┃
; ┃ Add two numbers
; ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
```

## Best Practices

### 1. Use Descriptive Parameter Names

While runtime uses De Bruijn indices, use meaningful names in source:

```scheme
; Good - clear intent
(≔ area (λ (width height) (⊗ width height)))
; Auto-generates: "multiply width and height"

; Less clear
(≔ area (λ (x y) (⊗ x y)))
; Auto-generates: "multiply the argument and the argument" (confusing!)
```

### 2. Structure Code for Good Docs

The doc generator reads your code structure:

```scheme
; Clear structure = clear docs
(≔ is-adult? (λ (age)
  (? (≥ age #18) #t #f)))
; Generates: "if greater or equal age and 18 then true else false"

; Simpler but equally clear
(≔ is-adult? (λ (age) (≥ age #18)))
; Generates: "greater or equal the argument and 18"
```

### 3. Add Comments for Context

While auto-docs describe WHAT, add comments for WHY:

```scheme
; Uses binary search for O(log n) lookup
; Assumes sorted input!
(≔ binary-search (λ (arr target) ...))
```

## Limitations (Current)

1. **Provenance for user functions** - ⌂⊛ currently fails on user-defined functions (works for primitives/modules only)
2. **Line numbers** - Parser doesn't track line numbers yet (always returns 0)
3. **Complex expressions** - Very nested code produces verbose descriptions
4. **No manual override** - Can't manually specify docs (working as designed - docs are derived from code)

## Future Enhancements

Planned improvements:

1. **Property-based test generation** - Infer properties (commutativity, associativity) from types
2. **Example extraction** - Extract examples from comments or test files
3. **Markdown export** - Generate markdown API documentation
4. **Cross-reference analysis** - Find similar functions, suggest refactoring
5. **Usage statistics** - Track which functions are used where

## See Also

- [SPEC.md](../../SPEC.md#documentation-5-) - Documentation primitives specification
- [AUTO_TEST_GUIDE.md](AUTO_TEST_GUIDE.md) - Auto-test generation guide
- [stdlib/doc_format.scm](../../stdlib/doc_format.scm) - Documentation formatters
