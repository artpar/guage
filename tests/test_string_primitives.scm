; ============================================
; STRING PRIMITIVES TESTS
; ============================================

; Basic String Creation Tests (5 tests)
(⊨ :str-from-number "42" (≈ #42))
(⊨ :str-from-bool-true "#t" (≈ #t))
(⊨ :str-from-bool-false "#f" (≈ #f))
(⊨ :str-from-symbol ":test" (≈ :test))
(⊨ :str-from-nil "∅" (≈ ∅))

; String Concatenation Tests (5 tests)
(⊨ :concat-simple "helloworld" (≈⊕ "hello" "world"))
(⊨ :concat-empty-left "test" (≈⊕ "" "test"))
(⊨ :concat-empty-right "test" (≈⊕ "test" ""))
(⊨ :concat-both-empty "" (≈⊕ "" ""))
(⊨ :concat-with-spaces "hello world" (≈⊕ "hello " "world"))

; String Length Tests (5 tests)
(⊨ :length-empty #0 (≈# ""))
(⊨ :length-single #1 (≈# "a"))
(⊨ :length-normal #5 (≈# "hello"))
(⊨ :length-with-spaces #11 (≈# "hello world"))
(⊨ :length-numbers #3 (≈# "123"))

; String Reference Tests (4 tests)
(⊨ :ref-first (≡ (≈ (≈→ "hello" #0)) "h") #t)
(⊨ :ref-middle (≡ (≈ (≈→ "hello" #2)) "l") #t)
(⊨ :ref-last (≡ (≈ (≈→ "hello" #4)) "o") #t)
(⊨ :ref-space (≡ (≈ (≈→ "a b" #1)) " ") #t)

; String Slice Tests (8 tests)
(⊨ :slice-full "hello" (≈⊂ "hello" #0 #5))
(⊨ :slice-prefix "hel" (≈⊂ "hello" #0 #3))
(⊨ :slice-suffix "llo" (≈⊂ "hello" #2 #5))
(⊨ :slice-middle "ell" (≈⊂ "hello" #1 #4))
(⊨ :slice-single "e" (≈⊂ "hello" #1 #2))
(⊨ :slice-empty "" (≈⊂ "hello" #2 #2))
(⊨ :slice-beyond "hello" (≈⊂ "hello" #0 #100))
(⊨ :slice-negative-start "hello" (≈⊂ "hello" #-5 #5))

; String Predicate Tests (6 tests)
(⊨ :is-string-true #t (≈? "hello"))
(⊨ :is-string-false-number #f (≈? #42))
(⊨ :is-string-false-symbol #f (≈? :test))
(⊨ :is-empty-true #t (≈∅? ""))
(⊨ :is-empty-false #f (≈∅? "hello"))
(⊨ :is-empty-non-string #f (≈∅? #42))

; String Equality Tests (6 tests)
(⊨ :str-equal-same #t (≈≡ "hello" "hello"))
(⊨ :str-equal-diff #f (≈≡ "hello" "world"))
(⊨ :str-equal-case-sensitive #f (≈≡ "Hello" "hello"))
(⊨ :str-equal-empty #t (≈≡ "" ""))
(⊨ :str-equal-with-spaces #t (≈≡ "a b" "a b"))
(⊨ :str-equal-non-string #f (≈≡ "42" #42))

; String Ordering Tests (6 tests)
(⊨ :str-less-true #t (≈< "apple" "banana"))
(⊨ :str-less-false #f (≈< "banana" "apple"))
(⊨ :str-less-equal #f (≈< "apple" "apple"))
(⊨ :str-less-prefix #t (≈< "app" "apple"))
(⊨ :str-less-case #t (≈< "Apple" "apple"))
(⊨ :str-less-numbers #t (≈< "1" "2"))

; Integration Tests (5 tests)
(⊨ :convert-concat-length #7
   (≈# (≈⊕ (≈ #42) (≈ :test))))

(⊨ :slice-concat-equal #t
   (≈≡ (≈⊕ (≈⊂ "hello" #0 #3) (≈⊂ "world" #3 #5))
       "helld"))

(⊨ :nested-concat "abc"
   (≈⊕ (≈⊕ "a" "b") "c"))

(≔ greeting "Hello, World!")
(⊨ :complex-slice "World" (≈⊂ greeting #7 #12))

(⊨ :compare-slices #t
   (≈< (≈⊂ "hello" #0 #3)
       (≈⊂ "world" #0 #3)))

; Total: 50 comprehensive tests
