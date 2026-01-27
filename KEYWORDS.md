# Keywords in Guage: Self-Evaluating Symbols

**Date:** 2026-01-27
**Status:** ✅ IMPLEMENTED

---

## What are Keywords?

**Keywords** are symbols that start with `:` (colon prefix). They are **self-evaluating** - they evaluate to themselves, like numbers and booleans.

```scheme
:test      ; → :test (self-evaluating)
#42        ; → #42 (self-evaluating)
#t         ; → #t (self-evaluating)
```

## Why Keywords?

Keywords are used as **identifiers, tags, and enum-like values**:

1. **Structure field names**: `:x`, `:y`, `:width`, `:height`
2. **Type tags**: `:Point`, `:Rectangle`, `:List`
3. **Enum values**: `:red`, `:green`, `:blue`
4. **Message tags**: `:ok`, `:error`, `:pending`
5. **Property names**: `:name`, `:age`, `:email`

## Syntax

### Keywords vs Regular Symbols

```scheme
; Regular symbol (variable lookup)
foo        ; → Error: Undefined variable 'foo'

; Keyword (self-evaluating)
:foo       ; → :foo (no lookup, just returns itself)
```

### Before (Required Quoting)

Previously, keywords had to be quoted to prevent variable lookup:

```scheme
; Old way - required quoting
(⊙≔ (⌜ :Point) (⌜ :x) (⌜ :y))  ; Define structure
(⊙ (⌜ :Point) #3 #4)            ; Create instance
(⊙→ p (⌜ :x))                   ; Get field
```

### After (Direct Usage) ✅

Now keywords are self-evaluating, no quoting needed:

```scheme
; New way - direct usage
(⊙≔ :Point :x :y)     ; Define structure ✅
(⊙ :Point #3 #4)      ; Create instance ✅
(⊙→ p :x)             ; Get field ✅
```

## Examples

### Structure Definition

```scheme
; Define Point structure with fields :x and :y
(⊙≔ :Point :x :y)
; → :Point

; Create point instance
(≔ p (⊙ :Point #3 #4))
; → ⊙[:Point ...]

; Get field (keyword as argument)
(⊙→ p :x)
; → #3
```

### Keywords in Data

```scheme
; Keywords in lists
(⟨⟩ :a (⟨⟩ :b (⟨⟩ :c ∅)))
; → ⟨:a ⟨:b ⟨:c ∅⟩⟩⟩

; Keywords in conditionals
(? (⊙? p :Point) :yes :no)
; → :yes

; Keywords as function results
(≔ get-status (λ (x) (? (> x #0) :positive :non-positive)))
(get-status #5)
; → :positive
```

### Keywords vs Strings

Keywords are **not strings**. They are symbolic identifiers:

```scheme
:foo       ; Symbol (keyword)
"foo"      ; String (future feature)

; Keywords can be compared
(≡ :foo :foo)     ; → #t
(≡ :foo :bar)     ; → #f
```

## Implementation

### Parser

The parser already handles keywords correctly - any token starting with `:` is parsed as a symbol:

```c
/* In parse_symbol() - main.c:112-131 */
/* Accept any non-whitespace, non-delimiter character */
/* This includes :symbol tokens */
```

### Evaluator

Keywords are checked before variable lookup:

```c
/* eval.c:940-950 */
if (cell_is_symbol(expr)) {
    const char* name = cell_get_symbol(expr);
    if (name[0] == ':') {
        /* Keywords are self-evaluating */
        cell_retain(expr);
        return expr;
    }
    /* Regular symbols: variable lookup */
    Cell* value = eval_lookup(ctx, name);
    ...
}
```

## Printed Representation

When a keyword is printed after evaluation, it shows with a `:` prefix (quoted form):

```scheme
:test    ; Input
:test    ; Output (self-evaluating)

; When printed from REPL:
guage> :test
::test   ; Shows as quoted (: is the quote marker in output)
```

**Note:** The double `::` in output means "quoted keyword" - the printer adds a `:` prefix to show it's a symbol.

## Use Cases

### 1. Structure Field Names

```scheme
(⊙≔ :Person :name :age :email)
(≔ alice (⊙ :Person :alice-smith #30 :alice@example.com))
(⊙→ alice :age)  ; → #30
```

### 2. Tagged Unions (ADTs)

```scheme
; Future: Node structures with variants
(⊚≔ :Result [:Ok :value] [:Error :message])
(≔ result (⊚ :Result :Ok #42))
(⊚? result :Result :Ok)  ; → #t
```

### 3. Message Passing

```scheme
; Future: Actor messages
(→! actor (⟨⟩ :update (⟨⟩ :value #42)))
```

### 4. Pattern Matching

```scheme
; Future: Pattern matching on keywords
(∇ result
  [(⊚ :Result :Ok val) val]
  [(⊚ :Result :Error msg) (⚠ msg)])
```

## Benefits

### 1. Cleaner Syntax

```scheme
; Before (verbose)
(⊙≔ (⌜ :Point) (⌜ :x) (⌜ :y))

; After (clean)
(⊙≔ :Point :x :y)
```

### 2. Works from Files

Keywords now work identically in REPL and files - no special handling needed.

### 3. Consistent with Lisp Tradition

Keywords (`:foo`) are self-evaluating in Common Lisp, Clojure, Racket, and other Lisps.

### 4. Type-Safe Identifiers

Keywords can't be accidentally shadowed by variables:

```scheme
(≔ x #42)
:x  ; Still :x, not #42
```

## Comparison with Other Languages

### Common Lisp

```lisp
:keyword    ; Self-evaluating
:test       ; → :TEST (keywords are uppercase)
```

### Clojure

```clojure
:keyword    ; Self-evaluating
:test       ; → :test
```

### Racket

```racket
'#:keyword  ; Quoted keyword
'#:test     ; → '#:test
```

### Guage

```scheme
:keyword    ; Self-evaluating (no need for # prefix)
:test       ; → :test
```

## Migration Guide

### For Existing Code

If you have code using quoted keywords:

```scheme
; Old code (still works!)
(⊙≔ (⌜ :Point) (⌜ :x) (⌜ :y))

; New code (preferred)
(⊙≔ :Point :x :y)
```

Both syntaxes work! The quote is redundant but harmless.

### For New Code

Use keywords directly without quoting:

```scheme
; ✅ Good - clean and readable
(⊙≔ :Point :x :y)
(⊙ :Point #3 #4)
(⊙→ p :x)

; ❌ Verbose - unnecessary quotes
(⊙≔ (⌜ :Point) (⌜ :x) (⌜ :y))
(⊙ (⌜ :Point) #3 #4)
(⊙→ p (⌜ :x))
```

## Testing

All existing tests still pass with keywords as self-evaluating:

```bash
./run_tests.sh
# Total:  13
# Passed: 13
# Failed: 0
```

New test added: `tests/structures_from_file.test` (not yet in test runner)

## Future

Keywords enable powerful features:

1. **Pattern matching** - Match on tagged values
2. **ADTs** - Discriminated unions with keyword tags
3. **Error handling** - Tagged error values
4. **Configuration** - Property names
5. **Protocols** - Method names

---

**Status:** ✅ Implemented and tested
**Date:** 2026-01-27
**Author:** Claude Sonnet 4.5
