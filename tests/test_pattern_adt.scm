;; ADT Pattern Matching Tests (Day 18)
;; Tests for matching leaf structures (⊙) and node/ADT structures (⊚)

;; ============================================================
;; Part 1: Leaf Structure Patterns (⊙)
;; ============================================================

;; Setup: Define Point structure
(⊙≔ :Point :x :y)
(≔ p1 (⊙ :Point #3 #4))
(≔ p2 (⊙ :Point #10 #20))

;; Test 1: Simple leaf pattern with variable bindings
(⊨ :test-leaf-simple
    #7
    (∇ p1 (⌜ (((⊙ :Point x y) (⊕ x y))))))

;; Test 2: Leaf pattern with multiple computations
(⊨ :test-leaf-compute
    #12
    (∇ p1 (⌜ (((⊙ :Point x y) (⊗ x y))))))

;; Test 3: Leaf pattern accessing single field
(⊨ :test-leaf-single-field
    #3
    (∇ p1 (⌜ (((⊙ :Point x _) x)))))

;; Test 4: Leaf pattern with literal match
(⊨ :test-leaf-literal
    #4
    (∇ p1 (⌜ (((⊙ :Point #3 y) y)))))

;; Test 5: Leaf pattern literal mismatch (should try next clause)
(⊨ :test-leaf-literal-fallthrough
    :other
    (∇ p1 (⌜ (((⊙ :Point #5 y) y)
              ((⊙ :Point x y) :other)))))

;; Test 6: Leaf pattern with wildcard
(⊨ :test-leaf-wildcard
    #4
    (∇ p1 (⌜ (((⊙ :Point _ y) y)))))

;; Test 7: Multiple clauses with different patterns
(⊨ :test-leaf-multiple-clauses
    :second
    (∇ p2 (⌜ (((⊙ :Point #3 #4) :first)
              ((⊙ :Point #10 #20) :second)
              (_ :other)))))

;; Test 8: Leaf pattern wrong type (should fail to match)
(⊨ :test-leaf-wrong-type
    #t
    (⚠? (∇ p1 (⌜ (((⊙ :Rectangle x y) x))))))

;; Setup: Rectangle for type mismatch tests
(⊙≔ :Rectangle :width :height)
(≔ rect (⊙ :Rectangle #5 #10))

;; Test 9: Different leaf types don't match
(⊨ :test-leaf-type-mismatch
    #t
    (⚠? (∇ p1 (⌜ (((⊙ :Rectangle x y) x))))))

;; Test 10: Nested leaf structures
(⊙≔ :Line :start :end)
(≔ line (⊙ :Line p1 p2))

(⊨ :test-leaf-nested
    #37
    (∇ line (⌜ (((⊙ :Line (⊙ :Point x1 y1) (⊙ :Point x2 y2))
                  (⊕ (⊕ x1 y1) (⊕ x2 y2)))))))

;; Test 11: Nested with partial wildcards
(⊨ :test-leaf-nested-wildcard
    #10
    (∇ line (⌜ (((⊙ :Line _ (⊙ :Point x _)) x)))))

;; Test 12: Conditional in result with bound variables
(⊨ :test-leaf-conditional
    :big
    (∇ p2 (⌜ (((⊙ :Point x y)
                (? (> x #5) :big :small))))))

;; ============================================================
;; Part 2: Node/ADT Patterns (⊚) - Enum-like (no fields)
;; ============================================================

;; Setup: Bool ADT
(⊚≔ :Bool (⌜ (:True)) (⌜ (:False)))
(≔ t (⊚ :Bool :True))
(≔ f (⊚ :Bool :False))

;; Test 13: Simple enum match - True
(⊨ :test-node-enum-true
    :yes
    (∇ t (⌜ (((⊚ :Bool :True) :yes)
             ((⊚ :Bool :False) :no)))))

;; Test 14: Simple enum match - False
(⊨ :test-node-enum-false
    :no
    (∇ f (⌜ (((⊚ :Bool :True) :yes)
             ((⊚ :Bool :False) :no)))))

;; Test 15: Enum wrong variant (should try next clause)
(⊨ :test-node-enum-fallthrough
    :matched-wildcard
    (∇ f (⌜ (((⊚ :Bool :True) :true-branch)
             (_ :matched-wildcard)))))

;; ============================================================
;; Part 3: Node/ADT Patterns (⊚) - With Fields
;; ============================================================

;; Setup: Option ADT
(⊚≔ :Option (⌜ (:None)) (⌜ (:Some :value)))
(≔ some-42 (⊚ :Option :Some #42))
(≔ some-100 (⊚ :Option :Some #100))
(≔ none (⊚ :Option :None))

;; Test 16: ADT with field - bind variable
(⊨ :test-node-some-bind
    #42
    (∇ some-42 (⌜ (((⊚ :Option :Some v) v)))))

;; Test 17: ADT with field - computation
(⊨ :test-node-some-compute
    #84
    (∇ some-42 (⌜ (((⊚ :Option :Some v) (⊗ v #2))))))

;; Test 18: ADT None variant (no fields)
(⊨ :test-node-none
    :empty
    (∇ none (⌜ (((⊚ :Option :None) :empty)))))

;; Test 19: ADT multiple clauses with default
(⊨ :test-node-option-default
    #99
    (∇ none (⌜ (((⊚ :Option :Some v) v)
                ((⊚ :Option :None) #99)))))

;; Test 20: ADT wrong variant fails to match
(⊨ :test-node-wrong-variant
    #t
    (⚠? (∇ some-42 (⌜ (((⊚ :Option :None) :no))))))

;; Test 21: ADT with literal pattern
(⊨ :test-node-literal
    :matched
    (∇ some-42 (⌜ (((⊚ :Option :Some #42) :matched)
                    ((⊚ :Option :Some _) :other)))))

;; ============================================================
;; Part 4: Recursive ADT (List)
;; ============================================================

;; Setup: List ADT
(⊚≔ :List (⌜ (:Nil)) (⌜ (:Cons :head :tail)))
(≔ empty (⊚ :List :Nil))
(≔ single (⊚ :List :Cons #1 empty))
(≔ pair (⊚ :List :Cons #2 single))
(≔ triple (⊚ :List :Cons #3 pair))

;; Test 22: List empty
(⊨ :test-list-empty
    :empty
    (∇ empty (⌜ (((⊚ :List :Nil) :empty)
                 ((⊚ :List :Cons h t) :cons)))))

;; Test 23: List single element - extract head
(⊨ :test-list-single-head
    #1
    (∇ single (⌜ (((⊚ :List :Cons h t) h)))))

;; Test 24: List single element - check tail is empty
(⊨ :test-list-single-tail
    :yes
    (∇ single (⌜ (((⊚ :List :Cons h (⊚ :List :Nil)) :yes)
                   (_ :no)))))

;; Test 25: List pair - extract head
(⊨ :test-list-pair-head
    #2
    (∇ pair (⌜ (((⊚ :List :Cons h t) h)))))

;; Test 26: List nested destructuring
(⊨ :test-list-nested
    #1
    (∇ pair (⌜ (((⊚ :List :Cons h1 (⊚ :List :Cons h2 t)) h2)))))

;; Test 27: List triple - nested pattern
(⊨ :test-list-triple-nested
    #6
    (∇ triple (⌜ (((⊚ :List :Cons a (⊚ :List :Cons b (⊚ :List :Cons c _)))
                    (⊕ a (⊕ b c)))))))

;; ============================================================
;; Part 5: Complex ADT (Binary Tree)
;; ============================================================

;; Setup: Tree ADT
(⊚≔ :Tree (⌜ (:Leaf :value)) (⌜ (:Node :left :value :right)))
(≔ leaf5 (⊚ :Tree :Leaf #5))
(≔ leaf10 (⊚ :Tree :Leaf #10))
(≔ leaf15 (⊚ :Tree :Leaf #15))
(≔ tree (⊚ :Tree :Node leaf5 #10 leaf15))

;; Test 28: Tree leaf pattern
(⊨ :test-tree-leaf
    #5
    (∇ leaf5 (⌜ (((⊚ :Tree :Leaf v) v)))))

;; Test 29: Tree node pattern
(⊨ :test-tree-node-value
    #10
    (∇ tree (⌜ (((⊚ :Tree :Node l v r) v)))))

;; Test 30: Tree node with nested leaf
(⊨ :test-tree-nested-left
    #5
    (∇ tree (⌜ (((⊚ :Tree :Node (⊚ :Tree :Leaf lv) v r) lv)))))

;; Test 31: Tree node with both leaves
(⊨ :test-tree-both-leaves
    #20
    (∇ tree (⌜ (((⊚ :Tree :Node (⊚ :Tree :Leaf lv) v (⊚ :Tree :Leaf rv))
                  (⊕ lv rv))))))

;; ============================================================
;; Part 6: Mixed Patterns
;; ============================================================

;; Test 32: Pair of structures
(≔ point-pair (⟨⟩ p1 p2))

(⊨ :test-mixed-pair-struct
    #7
    (∇ point-pair (⌜ (((⟨⟩ (⊙ :Point x1 y1) (⊙ :Point x2 y2))
                        (⊕ x1 y1))))))

;; Test 33: ADT containing pair
(⊚≔ :Container (⌜ (:Box :content)))
(≔ box-pair (⊚ :Container :Box (⟨⟩ #1 #2)))

(⊨ :test-mixed-adt-pair
    #3
    (∇ box-pair (⌜ (((⊚ :Container :Box (⟨⟩ a b)) (⊕ a b))))))

;; Test 34: Multiple structure types in one match
(⊨ :test-mixed-multiple-types
    :rect
    (∇ rect (⌜ (((⊙ :Point _ _) :point)
                ((⊙ :Rectangle _ _) :rect)
                (_ :other)))))

;; Test 35: Complex nested ADT with computation
(⊚≔ :Result (⌜ (:Ok :value)) (⌜ (:Err :message)))
(≔ ok-opt (⊚ :Result :Ok some-42))

(⊨ :test-mixed-nested-adt
    #42
    (∇ ok-opt (⌜ (((⊚ :Result :Ok (⊚ :Option :Some v)) v)
                   (_ #0)))))

;; ============================================================
;; Summary
;; ============================================================

;; Total: 35 ADT pattern matching tests
;; Categories:
;; - Leaf structure patterns (12 tests)
;; - Enum-like ADT patterns (3 tests)
;; - ADT with fields (6 tests)
;; - Recursive ADT (List) (6 tests)
;; - Complex ADT (Tree) (4 tests)
;; - Mixed patterns (4 tests)
