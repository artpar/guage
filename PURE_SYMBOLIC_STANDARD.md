# Pure Symbolic Standard - Zero English
## Parameter Names, Variables, Everything

**PRINCIPLE:** If it appears in code or documentation, it must be a symbol. NO EXCEPTIONS.

---

## The Problem

### Before (Wrong - English everywhere)

```scheme
(≔ map (λ (fn lst)          ; ❌ "fn" "lst" are English
  (∇ lst
    [∅ ∅]
    [(⟨⟩ head tail)         ; ❌ "head" "tail" are English
     (⟨⟩ (fn head) (map fn tail))])))
```

### After (Correct - Pure De Bruijn)

```scheme
(≔ ⤇ (λ (λ                  ; ✅ No names - just nested lambdas
  (∇ 0                      ; ✅ 0 = second parameter (list)
    [∅ ∅]
    [(⟨⟩ _ _)               ; ✅ Use wildcard or indices
     (⟨⟩ (1 (◁ 0))         ; ✅ 1 = first param (function)
         ((⤇ 1) (▷ 0)))]))))  ; ✅ Explicit indices
```

---

## De Bruijn Indices (Runtime Reality)

**At runtime, there are NO NAMES:**

```scheme
; Identity function
(λ 0)                       ; Parameter is index 0

; Constant function
(λ (λ 1))                   ; Inner lambda sees outer param as index 1

; Composition
(λ (λ (λ (2 (1 0)))))       ; f, g, x → f(g(x))
```

**This is what actually executes.** No names exist.

---

## Documentation Symbols (For Human Understanding Only)

When showing examples in documentation, use single-character mathematical symbols:

### Function Parameters

| Symbol | Unicode | Meaning | Use |
|--------|---------|---------|-----|
| `ƒ` | U+0192 | Function | First-class function argument |
| `𝕘` | U+1D558 | Function | Second function |
| `𝕙` | U+1D559 | Function | Third function |

### Data Parameters

| Symbol | Unicode | Meaning | Use |
|--------|---------|---------|-----|
| `𝕩` | U+1D569 | Value | Generic parameter |
| `𝕪` | U+1D56A | Value | Second parameter |
| `𝕫` | U+1D56B | Value | Third parameter |
| `𝕨` | U+1D568 | Value | Fourth parameter |

### List/Collection Elements

| Symbol | Unicode | Meaning | Use |
|--------|---------|---------|-----|
| `⊙` | U+2299 | Element | Single element |
| `⊚` | U+229A | Elements | Multiple elements |
| `◁` | U+25C1 | First | Head element |
| `▷` | U+25B7 | Rest | Tail elements |

### Temporary Values

| Symbol | Unicode | Meaning | Use |
|--------|---------|---------|-----|
| `α` | U+03B1 | Type var | Already using for types |
| `β` | U+03B2 | Type var | Already using |
| `γ` | U+03B3 | Type var | Additional type |
| `δ` | U+03B4 | Type var | Additional type |

### Accumulator/State

| Symbol | Unicode | Meaning | Use |
|--------|---------|---------|-----|
| `⊡` | U+22A1 | Accumulator | Fold accumulator |
| `⊠` | U+22A0 | State | Stateful computation |
| `⊞` | U+229E | Counter | Numeric counter |

---

## Standard Library - Corrected

### Map (Transform)

**Symbol:** `⤇` (U+2907)

**Wrong (had English):**
```scheme
(≔ map (λ (fn lst)
  (∇ lst [∅ ∅] [(⟨⟩ h t) (⟨⟩ (fn h) (map fn t))])))
```

**Correct (pure symbols):**
```scheme
; With documentation symbols (for clarity)
(≔ ⤇ (λ (ƒ 𝕩)
  (∇ 𝕩
    [∅ ∅]
    [(⟨⟩ ⊙ ▷) (⟨⟩ (ƒ ⊙) (⤇ ƒ ▷))])))

; As actually compiled (De Bruijn):
(≔ ⤇ (λ (λ
  (∇ 0
    [∅ ∅]
    [(⟨⟩ _ _) (⟨⟩ (1 (◁ 0)) ((⤇ 1) (▷ 0)))]))))
```

### Filter (Select)

**Symbol:** `⊻` (U+22BB)

**Correct:**
```scheme
; Documentation form
(≔ ⊻ (λ (ƒ 𝕩)
  (∇ 𝕩
    [∅ ∅]
    [(⟨⟩ ⊙ ▷)
     (? (ƒ ⊙)
        (⟨⟩ ⊙ (⊻ ƒ ▷))
        (⊻ ƒ ▷))])))

; De Bruijn form (actual code)
(≔ ⊻ (λ (λ
  (∇ 0
    [∅ ∅]
    [(⟨⟩ _ _)
     (? (1 (◁ 0))
        (⟨⟩ (◁ 0) ((⊻ 1) (▷ 0)))
        ((⊻ 1) (▷ 0)))]))))
```

### Fold (Reduce)

**Symbol:** `⥁` (U+2941)

**Correct:**
```scheme
; Documentation form
(≔ ⥁ (λ (ƒ ⊡ 𝕩)
  (∇ 𝕩
    [∅ ⊡]
    [(⟨⟩ ⊙ ▷) (⥁ ƒ (ƒ ⊡ ⊙) ▷)])))

; De Bruijn form
(≔ ⥁ (λ (λ (λ
  (∇ 0
    [∅ 1]
    [(⟨⟩ _ _) (((⥁ 2) (2 1 (◁ 0))) (▷ 0))])))))
```

### Sort (Quicksort)

**Symbol:** `⊼⇅` (sort + split)

**Correct:**
```scheme
; Documentation form
(≔ ⊼⇅ (λ (⊳ α : (⊧⊴)) (λ (𝕩)
  (∇ 𝕩
    [∅ ∅]
    [(⟨⟩ ⊙ ▷)
     (≔ ◁ (⊻ (λ (⊛) (⊴ ⊛ ⊙)) ▷))
     (≔ ▷▷ (⊻ (λ (⊛) (⊵ ⊛ ⊙)) ▷))
     (⊎ (⊼⇅ ◁) (⟨⟩ ⊙ (⊼⇅ ▷▷)))]))))

; De Bruijn form (without ≔ for clarity)
(≔ ⊼⇅ (λ (⊳ α : (⊧⊴)) (λ
  (∇ 0
    [∅ ∅]
    [(⟨⟩ _ _)
     (⊎ (⊼⇅ (⊻ (λ (⊴ 0 (◁ 1))) (▷ 0)))
         (⟨⟩ (◁ 0) (⊼⇅ (⊻ (λ (⊵ 0 (◁ 1))) (▷ 0)))))]))))
```

---

## Pattern Matching Variables

In pattern matching, we still need to bind values. Use symbols:

```scheme
(∇ 𝕩
  [∅ #0]                              ; Empty case
  [(⟨⟩ ⊙ ▷) (⊕ #1 (length ▷))])     ; Recursive case
  ;     ↑  ↑
  ;     |  └─ tail bound to ▷
  ;     └──── head bound to ⊙
```

**But remember:** These are just De Bruijn indices at runtime:
```scheme
(∇ 0
  [∅ #0]
  [(⟨⟩ _ _) (⊕ #1 (length (▷ 0)))])  ; Use ▷ primitive, not binding
```

---

## When Symbols Are Used

### 1. At Runtime: NEVER
De Bruijn indices only. No names exist.

### 2. During Parsing: TEMPORARILY
Parser sees symbols, converts to indices, discards names.

### 3. In Documentation: ALWAYS
Use mathematical symbols for clarity:
- ƒ, 𝕘, 𝕙 for functions
- 𝕩, 𝕪, 𝕫 for values
- ⊙, ▷ for list elements
- ⊡ for accumulators

### 4. In Error Messages: SYMBOLS ONLY
```
⚠ Type mismatch: expected ℕ, got 𝔹
⚠ Pattern match failed on ∇ at position 3
⚠ Unbound symbol: :⊙
```

---

## Complete Example: Fibonacci

### Wrong (English parameters):

```scheme
(≔ fib (λ (n)
  (? (< n #2)
     n
     (⊕ (fib (⊖ n #1)) (fib (⊖ n #2))))))
```

### Correct (De Bruijn):

```scheme
(≔ ! (λ
  (? (< 0 #2)
     0
     (⊕ (! (⊖ 0 #1)) (! (⊖ 0 #2))))))
```

### Documentation Form (for humans):

```scheme
(≔ ! (λ (𝕩)
  (? (< 𝕩 #2)
     𝕩
     (⊕ (! (⊖ 𝕩 #1)) (! (⊖ 𝕩 #2))))))
```

---

## Reserved Symbols for Parameters

### Never Use English

❌ `x`, `y`, `z` - These are English letters
❌ `n`, `m`, `i`, `j` - English letters
❌ `fn`, `func`, `f` - English abbreviations
❌ `lst`, `list`, `arr` - English words
❌ `pred`, `cond` - English abbreviations
❌ `acc`, `val`, `res` - English abbreviations

### Always Use Mathematical Symbols

✅ `𝕩`, `𝕪`, `𝕫` - Mathematical double-struck letters
✅ `ƒ`, `𝕘`, `𝕙` - Mathematical function symbols
✅ `⊙`, `⊚`, `⊛` - Mathematical operators
✅ `◁`, `▷` - Geometric shapes (already primitives)
✅ `α`, `β`, `γ` - Greek letters (for types)
✅ `⊡`, `⊠`, `⊞` - Mathematical box operators

---

## Implementation Strategy

### Parser Changes

**Current (allows English):**
```c
// Accepts any identifier
if (isalpha(c) || c == '_') {
    parse_identifier();
}
```

**New (symbols only):**
```c
// Only accepts mathematical symbols
if (is_unicode_math_symbol(c)) {
    parse_symbol();
}
```

### De Bruijn Conversion

**Keep current approach** - names converted to indices immediately:

```c
Cell* convert_to_debruijn(Cell* expr, SymbolTable* env) {
    if (is_symbol(expr)) {
        int index = lookup_index(env, symbol_name(expr));
        return cell_number(index);  // Replace name with index
    }
    // ...
}
```

### Documentation Generation

**Current (shows indices):**
```
📝 ! :: ℕ → ℕ
   if equals 0 and 0 then 1 else multiply 0 and apply ! to subtract 0 and 1
```

**New (shows symbolic names):**
```
📝 ! :: ℕ → ℕ
   if equals 𝕩 and 0 then 1 else multiply 𝕩 and apply ! to subtract 𝕩 and 1
```

But this is just for display - internally still De Bruijn.

---

## Migration Path

### Phase 1: Update Documentation (Now)
- Replace all English parameter names with symbols
- Create symbol style guide
- Update all examples

### Phase 2: Update Parser (Week 1)
- Reject English identifiers in lambda parameters
- Only accept mathematical symbols
- Update error messages

### Phase 3: Update Compiler (Week 2)
- Ensure De Bruijn conversion handles all symbols
- Update pretty-printer for debugging
- Test with symbolic parameters

### Phase 4: Update Stdlib (Week 3)
- Rewrite all stdlib functions with symbols
- No English anywhere
- Complete test suite

---

## Symbol Style Guide

### For Function Parameters
Use lowercase mathematical letters:
- ƒ (U+0192) - first function
- 𝕘 (U+1D558) - second function
- 𝕩 (U+1D569) - first value
- 𝕪 (U+1D56A) - second value

### For Pattern Bindings
Use geometric/operator symbols:
- ⊙ (U+2299) - single element
- ◁ (U+25C1) - head (already primitive!)
- ▷ (U+25B7) - tail (already primitive!)

### For Type Variables
Use Greek letters:
- α, β, γ, δ (already using)

### For Accumulators
Use box operators:
- ⊡ (U+22A1) - accumulator
- ⊠ (U+22A0) - state

---

## Comparison

### Other Languages (English-based)

```python
def map(fn, lst):
    return [fn(x) for x in lst]
```

```haskell
map :: (a -> b) -> [a] -> [b]
map f [] = []
map f (x:xs) = f x : map f xs
```

```rust
fn map<F, A, B>(f: F, list: Vec<A>) -> Vec<B>
where F: Fn(A) -> B
{
    list.into_iter().map(f).collect()
}
```

### Guage (Pure Symbols)

```scheme
; Documentation form
⤇ :: (α → β) → [α] → [β]
(≔ ⤇ (λ (ƒ 𝕩) (∇ 𝕩 [∅ ∅] [(⟨⟩ ⊙ ▷) (⟨⟩ (ƒ ⊙) (⤇ ƒ ▷))])))

; Runtime form (what actually executes)
(≔ ⤇ (λ (λ (∇ 0 [∅ ∅] [(⟨⟩ _ _) (⟨⟩ (1 (◁ 0)) ((⤇ 1) (▷ 0)))]))))
```

**Zero English. Pure structure. AI-native.**

---

## Conclusion

**THREE LEVELS:**

1. **Runtime:** De Bruijn indices (0, 1, 2...) - NO NAMES
2. **Parsing:** Temporary symbols (ƒ, 𝕩, ⊙) → converted to indices
3. **Documentation:** Symbols for human understanding

**ZERO ENGLISH AT ALL LEVELS.**

This is the true AI-first language.
