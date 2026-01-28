; Test De Bruijn converter handles strings correctly
; Day 40: De Bruijn string support fix

; Test 1: Lambda with string literals should work
(⊨ :string-in-lambda
   "hello"
   ((λ (x) "hello") #42))

; Test 2: Lambda comparing string with parameter
(⊨ :string-compare
   #t
   ((λ (s) (≈≡ s "test")) "test"))

; Test 3: Nested lambda with strings (2 levels)
(⊨ :nested-with-strings-2
   "inner"
   ((λ (outer)
     ((λ (inner)
       inner)
      "inner"))
    "outer"))

; Test 4: Nested lambda with strings (3 levels)
(⊨ :nested-with-strings-3
   "level3"
   ((λ (l1)
     ((λ (l2)
       ((λ (l3)
         l3)
        "level3"))
      "level2"))
    "level1"))

; Test 5: Lambda using string operations
(⊨ :string-ops-in-lambda
   #7
   ((λ (s) (≈# s)) "testing"))

; Test 6: Nested lambda mixing strings and numbers
(⊨ :mixed-nested
   #42
   ((λ (str)
     ((λ (num)
       num)
      #42))
    "text"))

; Test 7: String as return value from nested structure
(⊨ :string-return
   "result"
   ((λ (x)
     (? (≡ x #1)
        "result"
        "other"))
    #1))

; Test 8: Multiple string parameters
(⊨ :multiple-strings
   "ab"
   ((λ (a b) (≈⊕ a b)) "a" "b"))

; Test 9: Deep nesting (4 levels) with strings
(⊨ :deep-nesting-strings
   "d"
   ((λ (a)
     ((λ (b)
       ((λ (c)
         ((λ (d)
           d)
          "d"))
        "c"))
      "b"))
    "a"))

; Test 10: Strings in conditional within lambda
(⊨ :conditional-strings
   "yes"
   ((λ (test)
     (? test "yes" "no"))
    #t))
