---
Status: REFERENCE
Created: 2026-01-28
Updated: 2026-01-28
Purpose: Comprehensive guide to Guage's auto-test generation system
---

# Auto-Test Generation Guide

## Overview

Guage's `⌂⊨` primitive automatically generates test cases from type signatures using **type-directed generation**. Tests ensure functions work correctly without manual test writing.

## The ⌂⊨ Primitive

**Type:** `:symbol → [tests]`

**Purpose:** Generate test cases automatically from a function's type signature.

**Example:**
```scheme
(⌂⊨ (⌜ ⊕))
; Returns list of test cases:
; ⟨⟨:⊨ ⟨::test-⊕-identity ⟨#t ⟨⟨:ℕ? ⟨⟨:⊕ ⟨#0 ⟨#0 ∅⟩⟩⟩ ∅⟩⟩ ∅⟩⟩⟩⟩
; ⟨⟨:⊨ ⟨::test-⊕-zero ⟨#t ⟨⟨:ℕ? ⟨⟨:⊕ ⟨#0 ⟨#5 ∅⟩⟩⟩ ∅⟩⟩ ∅⟩⟩⟩⟩
; ⟨⟨:⊨ ⟨::test-⊕-normal ⟨#t ⟨⟨:ℕ? ⟨⟨:⊕ ⟨#5 ⟨#3 ∅⟩⟩⟩ ∅⟩⟩ ∅⟩⟩⟩⟩
; ∅⟩⟩⟩
```

## Test Generation Strategy

### Current Implementation

⌂⊨ generates basic type-conformance tests:

1. **Identity tests** - Test with zero/identity values
2. **Zero tests** - Test with zero as one argument
3. **Normal tests** - Test with typical values
4. **Type checks** - Verify result matches expected type

### Test Structure

Each generated test has this structure:
```scheme
⟨:⊨ ⟨:test-name ⟨#t ⟨⟨:type-predicate ⟨result ∅⟩⟩ ∅⟩⟩⟩⟩
```

Where:
- `:⊨` - Test case marker
- `:test-name` - Descriptive test identifier
- `#t` - Expected result (passes if true)
- `:type-predicate` - Type check (ℕ?, 𝔹?, etc.)
- `result` - The function call to test

### Primitives with Auto-Tests

All 74 functional primitives support ⌂⊨:

```scheme
; Arithmetic
(⌂⊨ (⌜ ⊕))  ; Addition tests
(⌂⊨ (⌜ ⊗))  ; Multiplication tests
(⌂⊨ (⌜ ⊘))  ; Division tests

; Comparison
(⌂⊨ (⌜ ≡))  ; Equality tests
(⌂⊨ (⌜ <))  ; Less-than tests

; Logic
(⌂⊨ (⌜ ∧))  ; AND tests
(⌂⊨ (⌜ ¬))  ; NOT tests

; Type predicates
(⌂⊨ (⌜ ℕ?))  ; Number predicate tests
(⌂⊨ (⌜ 𝔹?))  ; Boolean predicate tests

; Lists
(⌂⊨ (⌜ ⟨⟩))  ; Cons tests
(⌂⊨ (⌜ ◁))   ; Car tests
(⌂⊨ (⌜ ▷))   ; Cdr tests
```

## Running Generated Tests

### Execute Single Test

```scheme
(≔ tests (⌂⊨ (⌜ ⊕)))
(≔ first-test (◁ tests))

; Evaluate the test
(⌞ first-test)  ; → #t if passes
```

### Execute All Tests for Symbol

```scheme
(≔ run-all-tests (λ (sym)
  (≔ tests (⌂⊨ sym))
  (≔ run-test (λ (test) (⌞ test)))
  (map run-test tests)))

(run-all-tests (⌜ ⊕))  ; Run all ⊕ tests
```

### Bulk Test Runner

The `bootstrap/tests/test_runner.scm` uses ⌂⊨ for comprehensive testing:

```scheme
; Generate and run tests for all primitives
(≔ arithmetic-tests (λ ()
  (append (⌂⊨ (⌜ ⊕))
  (append (⌂⊨ (⌜ ⊖))
  (append (⌂⊨ (⌜ ⊗))
  (append (⌂⊨ (⌜ ⊘))
  (append (⌂⊨ (⌜ %))
  ∅)))))))
```

## Manual Test Writing

While ⌂⊨ generates basic tests, write manual tests for:

### 1. Business Logic

```scheme
; Auto-generated tests check types
(⌂⊨ (⌜ calculate-discount))  ; Type conformance

; Manual tests check logic
(⊨ :vip-discount-correct
   #t
   (≡ (calculate-discount :vip #100) #80))  ; 20% off

(⊨ :regular-discount-correct
   #t
   (≡ (calculate-discount :regular #100) #90))  ; 10% off
```

### 2. Edge Cases

```scheme
; Auto-tests use typical values
(⌂⊨ (⌜ safe-div))  ; Basic division tests

; Manual tests check edge cases
(⊨ :div-by-zero-error
   #t
   (⚠? (safe-div #10 #0)))  ; Division by zero

(⊨ :negative-numbers
   #t
   (≡ (safe-div #-10 #2) #-5))
```

### 3. Integration Tests

```scheme
; Test complete workflows
(⊨ :user-registration-flow
   #t
   (≔ user (create-user "alice" "pass123"))
   (≔ validated (validate-user user))
   (≔ saved (save-to-db validated))
   (∧ (¬ (⚠? user))
      (∧ (¬ (⚠? validated))
         (¬ (⚠? saved)))))
```

## Property-Based Testing (Future)

Planned enhancements to ⌂⊨:

### Algebraic Properties

```scheme
; Commutative property: f(x, y) = f(y, x)
(⌂⊨-commutative (⌜ ⊕))
; → Tests: (⊕ 3 7) = (⊕ 7 3)

; Associative property: f(f(x, y), z) = f(x, f(y, z))
(⌂⊨-associative (⌜ ⊕))
; → Tests: (⊕ (⊕ 1 2) 3) = (⊕ 1 (⊕ 2 3))

; Identity property: f(x, id) = x
(⌂⊨-identity (⌜ ⊕) #0)
; → Tests: (⊕ x 0) = x
```

### Inverse Properties

```scheme
; Round-trip property: decode(encode(x)) = x
(⌂⊨-inverse (⌜ encode) (⌜ decode))
; → Tests: (decode (encode x)) = x
```

### Boundary Testing

```scheme
; Test with boundary values
(⌂⊨-boundaries (⌜ factorial))
; → Tests with: 0, 1, -1, MAX_INT, MIN_INT
```

### Random Property Testing

```scheme
; QuickCheck-style random testing
(⌂⊨-random (⌜ sort) 100)
; → Generates 100 random inputs, checks:
;    - Length preserved
;    - Elements preserved
;    - Sorted order
```

## Test Coverage Analysis

### Check Test Coverage

```scheme
; How many tests for a symbol?
(≔ test-count (λ (sym)
  (length (⌂⊨ sym))))

(test-count (⌜ ⊕))  ; → #3 tests
```

### Coverage Report

```scheme
; Generate coverage report for module
(≔ coverage-report (λ (module-path)
  (⋘ module-path)
  (≔ symbols (⌂⊚ module-path))
  (map (λ (sym)
    (⟨⟩ sym (test-count sym)))
       symbols)))
```

## Best Practices

### 1. Combine Auto and Manual Tests

```scheme
; Start with auto-generated tests
(≔ base-tests (⌂⊨ (⌜ my-func)))

; Add manual tests for specifics
(≔ manual-tests (⟨⟩
  (⊨ :specific-case #t (≡ (my-func #42) #expected))
  ∅))

; Combine both
(≔ all-tests (append base-tests manual-tests))
```

### 2. Use Descriptive Test Names

```scheme
; Good - describes what's being tested
(⊨ :factorial-of-zero-is-one #t (≡ (! #0) #1))
(⊨ :factorial-of-five #t (≡ (! #5) #120))

; Less clear
(⊨ :test1 #t (≡ (! #0) #1))
(⊨ :test2 #t (≡ (! #5) #120))
```

### 3. Test Error Conditions

```scheme
; Test success case
(⊨ :parse-valid-json #t (¬ (⚠? (parse-json "{}"))))

; Test error case
(⊨ :parse-invalid-json #t (⚠? (parse-json "invalid")))
```

### 4. Document Test Intent

```scheme
; Regression test for bug #123: division overflow
(⊨ :bug-123-no-overflow #t (≡ (safe-div #1000000 #1) #1000000))
```

## Integration with CI/CD

### Automated Testing

```bash
# Run all auto-generated tests
./bootstrap/guage < bootstrap/tests/test_runner.scm

# Check exit code
if [ $? -eq 0 ]; then
    echo "All tests passed"
else
    echo "Tests failed"
    exit 1
fi
```

### Test Reporting

Generate test reports:

```scheme
(≔ test-report (λ ()
  (≋ "Test Report")
  (≋ "============")
  ; Run tests for each primitive...
  (≋ "Total: " (test-count-all))
  (≋ "Passed: " (test-passed))
  (≋ "Failed: " (test-failed))))
```

## Limitations (Current)

1. **Basic tests only** - Currently generates type-conformance tests, not property-based tests
2. **No user function tests** - Works for primitives but limited for user-defined functions
3. **Fixed test values** - Uses predefined values (0, 5, 42) rather than random generation
4. **No mutation testing** - Doesn't verify tests actually catch bugs

## See Also

- [SPEC.md](../../SPEC.md#testing-2-) - Testing primitives specification
- [AUTO_DOCUMENTATION_GUIDE.md](AUTO_DOCUMENTATION_GUIDE.md) - Auto-documentation guide
- [bootstrap/tests/test_runner.scm](../../bootstrap/tests/test_runner.scm) - Example test runner
- [stdlib/testgen.scm](../../stdlib/testgen.scm) - Test generation utilities (WIP)
