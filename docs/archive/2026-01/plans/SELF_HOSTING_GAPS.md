# Self-Hosting Gap Analysis

## Executive Summary

**Can Guage implement itself?** Yes, but missing critical primitives.

**Current Status:** Turing complete but lacks data manipulation and I/O needed for compiler/parser.

---

## What Self-Hosting Requires

### 1. Parser (Text → AST)
```scheme
(≔ parse (λ (text)
  ; "(⊕ 1 2)" → ⟨:⊕ ⟨#1 ⟨#2 ∅⟩⟩⟩
  ...))
```

**Needs:**
- String type (mutable text, not symbols)
- String → character list
- Character classification (whitespace, digit, symbol)
- String slicing/indexing
- String concatenation

### 2. Compiler (AST → Output)
```scheme
(≔ compile (λ (ast)
  ; ⟨:⊕ ⟨#1 ⟨#2 ∅⟩⟩⟩ → C code or bytecode
  ...))
```

**Needs:**
- Pattern matching on AST shapes
- List operations (map, filter, fold, append)
- String building (emit code)
- Recursion (already have ✅)

### 3. I/O (Read source, write output)
```scheme
(≔ compile-file (λ (infile outfile)
  (≔ source (read-file infile))
  (≔ ast (parse source))
  (≔ code (compile ast))
  (write-file outfile code)))
```

**Needs:**
- File reading
- File writing
- STDIN/STDOUT
- Error reporting with file locations

---

## Current Primitives (What We Have)

### ✅ Data Structure Manipulation
```
⟨⟩  - cons (construct pair)
◁   - car (head)
▷   - cdr (tail)
```

### ✅ Control Flow
```
?   - conditional
λ   - lambda
≔   - define (global binding)
```

### ✅ Computation
```
⊕ ⊖ ⊗ ⊘  - Arithmetic
≡ ≢ < > ≤ ≥ - Comparison
∧ ∨ ¬      - Logic
```

### ✅ Introspection
```
ℕ? 𝔹? :? ∅? ⟨⟩? #? - Type predicates
⊙  - type-of
⧉  - arity
⊛  - source code
⌂ ⌂∈ ⌂≔ - Documentation
```

### ✅ Debugging
```
⚠ ⚠? - Errors
⊢    - Assert
⟲    - Trace
⊨ ≟  - Testing
```

---

## Missing Primitives (Critical Gaps)

### 🔴 CRITICAL: String Type & Operations

**Current:** Only have `:symbols` (immutable identifiers)

**Need:** Mutable text strings for parsing

```scheme
; String creation
"hello"              ; String literal
(str :symbol)        ; Symbol → String
(str #42)            ; Number → String

; String operations
(str-length "hello")           ; → #5
(str-ref "hello" #0)           ; → :h (character as symbol)
(str-slice "hello" #1 #4)      ; → "ell"
(str-append "hello" " world")  ; → "hello world"
(str-split "a,b,c" ",")        ; → ["a" "b" "c"]
(str-join ["a" "b"] ",")       ; → "a,b"

; Character operations
(char? :a)                     ; → #t
(char->num :a)                 ; → #97 (ASCII)
(num->char #97)                ; → :a
(whitespace? :space)           ; → #t
(digit? :5)                    ; → #t
```

**Why Critical:** Parser needs to split text into tokens.

### 🔴 CRITICAL: List Operations

**Current:** Only have cons/car/cdr (manual recursion)

**Need:** Standard list library

```scheme
; List construction
(list #1 #2 #3)              ; → ⟨#1 ⟨#2 ⟨#3 ∅⟩⟩⟩
(range #0 #10)               ; → [0 1 2 ... 9]
(repeat #42 #5)              ; → [42 42 42 42 42]

; List operations
(length list)                ; Count elements
(nth list #2)                ; Get element at index
(append list1 list2)         ; Concatenate
(reverse list)               ; Reverse order

; Higher-order
(map f list)                 ; Apply f to each
(filter pred list)           ; Keep elements where pred true
(fold f init list)           ; Reduce with f
(zip list1 list2)            ; Pair elements

; List comprehension (syntactic sugar)
[x | x <- list, (> x #5)]    ; Filter x > 5
```

**Why Critical:** Compiler needs to transform AST lists.

### 🔴 CRITICAL: File I/O

**Current:** No I/O at all

**Need:** Read/write files

```scheme
; File operations
(read-file "source.guage")         ; → String
(write-file "output.c" content)    ; → ∅
(append-file "log.txt" message)    ; → ∅
(file-exists? "test.guage")        ; → 𝔹

; Directory operations
(list-dir "src/")                  ; → ["a.guage" "b.guage"]
(file-type "src/")                 ; → :directory

; Standard streams
(read-line)                        ; Read from STDIN
(print "text")                     ; Write to STDOUT (no newline)
(println "text")                   ; Write to STDOUT (with newline)
(eprint "error")                   ; Write to STDERR
```

**Why Critical:** Compiler needs to read source and write output.

### 🟡 IMPORTANT: Pattern Matching

**Current:** Manual if/else chains

**Need:** Declarative pattern matching

```scheme
; Match on structure
(match ast
  [(:⊕ a b) (compile-add a b)]
  [(:⊗ a b) (compile-mul a b)]
  [(:λ params body) (compile-lambda params body)]
  [_ (error :unknown-ast ast)])

; Match with guards
(match value
  [(n where (> n #0)) (process-positive n)]
  [(n where (< n #0)) (process-negative n)]
  [#0 :zero])
```

**Why Important:** Compiler needs to match AST node types.

### 🟡 IMPORTANT: String Pattern Matching

**Current:** No string operations

**Need:** Regex or string matching

```scheme
; Basic string patterns
(str-starts-with? "hello" "he")   ; → #t
(str-ends-with? "hello" "lo")     ; → #t
(str-contains? "hello" "ell")     ; → #t

; Simple regex (or just manual parsing)
(str-match? "[0-9]+" "123")       ; → #t
(str-extract "[0-9]+" "foo123")   ; → "123"
```

**Why Important:** Lexer needs to recognize tokens.

### 🟢 NICE-TO-HAVE: Hash Maps

**Current:** Only have lists (linear search)

**Need:** O(1) lookup for symbol tables

```scheme
(map-create)                      ; Create empty map
(map-set map :key value)          ; Add/update
(map-get map :key)                ; Lookup
(map-has? map :key)               ; Check exists
(map-keys map)                    ; Get all keys
(map-values map)                  ; Get all values
```

**Why Nice:** Symbol tables for compiler, but can use association lists.

---

## Implementation Priority

### Phase 1: Strings (MUST HAVE)
```c
// In cell.h, add:
CELL_ATOM_STRING,    /* "text" - mutable string */

// In primitives.c, add:
str_length, str_ref, str_slice, str_append, str_split, str_join
char_to_num, num_to_char, whitespace?, digit?
```

**Estimated:** ~200 lines C code, 15 primitives

### Phase 2: List Operations (MUST HAVE)
```c
// Can implement in Guage itself once we have strings!
(≔ map (λ (f list) ...))
(≔ filter (λ (pred list) ...))
(≔ fold (λ (f init list) ...))
```

**Estimated:** ~50 lines Guage code, or 10 primitives in C

### Phase 3: File I/O (MUST HAVE)
```c
// In primitives.c, add:
prim_read_file, prim_write_file, prim_file_exists
prim_read_line, prim_print, prim_println
```

**Estimated:** ~150 lines C code, 8 primitives

### Phase 4: Pattern Matching (IMPORTANT)
```scheme
; Can implement as macro/special form
(≔ match (macro (expr cases) ...))
```

**Estimated:** ~100 lines Guage code (once macros exist)

---

## Minimal Self-Hosting Set

**To write a Guage parser/compiler in Guage, you need:**

### Absolute Minimum (15 primitives):
1. `str` - Create string from value
2. `str-length` - Get string length
3. `str-ref` - Get character at index
4. `str-slice` - Extract substring
5. `str-append` - Concatenate strings
6. `char->num` - Character to ASCII
7. `whitespace?` - Test whitespace
8. `digit?` - Test digit
9. `read-file` - Read file to string
10. `write-file` - Write string to file
11. `list` - Construct list from args
12. `length` - List length
13. `append` - Concatenate lists
14. `map` - Transform list
15. `fold` - Reduce list

### Everything Else Can Be Built:
- Parser → Using string operations
- AST manipulation → Using list operations
- Code generation → Using string building
- Compiler → Combining above

---

## Example: Parser in Guage (Once Strings Exist)

```scheme
(≔ tokenize (λ (text)
  "Split text into tokens"
  (≔ chars (str->list text))
  (≔ tokens (fold (λ (acc char)
    (? (whitespace? char)
       acc  ; Skip whitespace
       (cons char acc))) ∅ chars))
  (reverse tokens)))

(≔ parse-expr (λ (tokens)
  "Parse expression from token list"
  (match (car tokens)
    [:lparen (parse-list (cdr tokens))]
    [:number (parse-number (cdr tokens))]
    [:symbol (parse-symbol (cdr tokens))]
    [_ (error :unexpected-token)])))

(≔ parse (λ (text)
  "Top-level parser"
  (parse-expr (tokenize text))))
```

**This works once we have strings!**

---

## Timeline Estimate

### Week 1: String Type
- Add CELL_ATOM_STRING to cell.h
- Implement 10 string primitives
- Test string operations

### Week 2: List Library
- Implement 5 list operations (in Guage or C)
- Test with list transformations

### Week 3: File I/O
- Add 5 I/O primitives
- Test reading/writing files

### Week 4: Write Parser in Guage
- Tokenizer (~50 lines)
- S-expression parser (~100 lines)
- Test on existing code

### Month 2: Write Compiler in Guage
- AST → C code generator
- De Bruijn conversion
- Test compilation

### Month 3: Bootstrap Complete
- Compile Guage compiler with itself
- Self-hosting achieved!

---

## Conclusion

**Guage CAN implement itself, but needs:**

1. 🔴 **Strings** (15 primitives) - CRITICAL
2. 🔴 **List operations** (5 primitives) - CRITICAL
3. 🔴 **File I/O** (8 primitives) - CRITICAL
4. 🟡 **Pattern matching** (1 special form) - IMPORTANT

**Total: ~28 primitives, ~500 lines of C code**

**After that:** Parser, compiler, type checker all writable in Guage itself.

**Current blocker:** No string type. Start there.
