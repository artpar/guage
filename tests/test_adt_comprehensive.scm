;; Comprehensive ADT (Algebraic Data Type) Tests
;; Testing all node structure primitives: ⊚≔, ⊚, ⊚→, ⊚?

;; ============================================================
;; Test 1: Simple ADT (no fields - enum-like)
;; ============================================================

(⊚≔ :Bool (⌜ (:True)) (⌜ (:False)))
; → ::Bool

(≔ t (⊚ :Bool :True))
(≔ f (⊚ :Bool :False))

(⊨ :test-bool-create-true #t (⊚? t :Bool :True))
(⊨ :test-bool-create-false #t (⊚? f :Bool :False))
(⊨ :test-bool-wrong-variant #f (⊚? t :Bool :False))
(⊨ :test-bool-wrong-type #f (⊚? t :Foo :True))

;; ============================================================
;; Test 2: ADT with Single Field
;; ============================================================

(⊚≔ :Option (⌜ (:None)) (⌜ (:Some :value)))
; → ::Option

(≔ none (⊚ :Option :None))
(≔ some-42 (⊚ :Option :Some #42))
(≔ some-t (⊚ :Option :Some #t))

(⊨ :test-option-none #t (⊚? none :Option :None))
(⊨ :test-option-some #t (⊚? some-42 :Option :Some))
(⊨ :test-option-get-value #t (≡ (⊚→ some-42 :value) #42))
(⊨ :test-option-get-bool #t (≡ (⊚→ some-t :value) #t))

;; ============================================================
;; Test 3: Recursive ADT (List)
;; ============================================================

(⊚≔ :List (⌜ (:Nil)) (⌜ (:Cons :head :tail)))
; → ::List

(≔ empty (⊚ :List :Nil))
(≔ single (⊚ :List :Cons #1 empty))
(≔ pair (⊚ :List :Cons #2 single))
(≔ triple (⊚ :List :Cons #3 pair))

(⊨ :test-list-empty #t (⊚? empty :List :Nil))
(⊨ :test-list-single #t (⊚? single :List :Cons))
(⊨ :test-list-head-single #t (≡ (⊚→ single :head) #1))
(⊨ :test-list-tail-single #t (⊚? (⊚→ single :tail) :List :Nil))
(⊨ :test-list-head-pair #t (≡ (⊚→ pair :head) #2))
(⊨ :test-list-nested-head #t (≡ (⊚→ (⊚→ pair :tail) :head) #1))

;; ============================================================
;; Test 4: Binary Tree ADT
;; ============================================================

(⊚≔ :Tree (⌜ (:Leaf :value)) (⌜ (:Node :left :value :right)))
; → ::Tree

(≔ leaf5 (⊚ :Tree :Leaf #5))
(≔ leaf10 (⊚ :Tree :Leaf #10))
(≔ leaf15 (⊚ :Tree :Leaf #15))
(≔ tree (⊚ :Tree :Node leaf5 #10 leaf15))

(⊨ :test-tree-leaf #t (⊚? leaf5 :Tree :Leaf))
(⊨ :test-tree-node #t (⊚? tree :Tree :Node))
(⊨ :test-tree-leaf-value #t (≡ (⊚→ leaf5 :value) #5))
(⊨ :test-tree-node-value #t (≡ (⊚→ tree :value) #10))
(⊨ :test-tree-left #t (⊚? (⊚→ tree :left) :Tree :Leaf))
(⊨ :test-tree-right #t (⊚? (⊚→ tree :right) :Tree :Leaf))
(⊨ :test-tree-left-value #t (≡ (⊚→ (⊚→ tree :left) :value) #5))
(⊨ :test-tree-right-value #t (≡ (⊚→ (⊚→ tree :right) :value) #15))

;; ============================================================
;; Test 5: Expression ADT (multiple variants with fields)
;; ============================================================

(⊚≔ :Expr
     (⌜ (:Num :value))
     (⌜ (:Add :left :right))
     (⌜ (:Mul :left :right)))
; → ::Expr

(≔ num5 (⊚ :Expr :Num #5))
(≔ num3 (⊚ :Expr :Num #3))
(≔ add (⊚ :Expr :Add num5 num3))
(≔ mul (⊚ :Expr :Mul num5 num3))

(⊨ :test-expr-num #t (⊚? num5 :Expr :Num))
(⊨ :test-expr-add #t (⊚? add :Expr :Add))
(⊨ :test-expr-mul #t (⊚? mul :Expr :Mul))
(⊨ :test-expr-num-value #t (≡ (⊚→ num5 :value) #5))
(⊨ :test-expr-add-left #t (⊚? (⊚→ add :left) :Expr :Num))
(⊨ :test-expr-add-right #t (⊚? (⊚→ add :right) :Expr :Num))
(⊨ :test-expr-nested-value #t (≡ (⊚→ (⊚→ add :left) :value) #5))

;; ============================================================
;; Test 6: Result ADT (for error handling)
;; ============================================================

(⊚≔ :Result (⌜ (:Ok :value)) (⌜ (:Err :message)))
; → ::Result

(≔ ok-result (⊚ :Result :Ok #42))
(≔ err-result (⊚ :Result :Err :division-by-zero))

(⊨ :test-result-ok #t (⊚? ok-result :Result :Ok))
(⊨ :test-result-err #t (⊚? err-result :Result :Err))
(⊨ :test-result-ok-value #t (≡ (⊚→ ok-result :value) #42))
(⊨ :test-result-err-message #t (≡ (⊚→ err-result :message) :division-by-zero))

;; ============================================================
;; Test 7: Complex nested structure
;; ============================================================

(⊚≔ :Person (⌜ (:Person :name :age :address)))
(⊚≔ :Address (⌜ (:Address :street :city)))
; → ::Person, ::Address

(≔ addr (⊚ :Address :Address :Main-Street :Springfield))
(≔ person (⊚ :Person :Person :Alice #30 addr))

(⊨ :test-nested-person #t (⊚? person :Person :Person))
(⊨ :test-nested-address #t (⊚? (⊚→ person :address) :Address :Address))
(⊨ :test-nested-name #t (≡ (⊚→ person :name) :Alice))
(⊨ :test-nested-age #t (≡ (⊚→ person :age) #30))
(⊨ :test-nested-city #t (≡ (⊚→ (⊚→ person :address) :city) :Springfield))

;; ============================================================
;; Test 8: Using ADTs with functions
;; ============================================================

; Function to check if option is some
(≔ is-some? (λ (opt) (⊚? opt :Option :Some)))

; Function to unwrap option with default
(≔ unwrap-or (λ (opt default)
  (? (⊚? opt :Option :Some)
     (⊚→ opt :value)
     default)))

(⊨ :test-is-some-some #t (is-some? some-42))
(⊨ :test-is-some-none #f (is-some? none))
(⊨ :test-unwrap-some #t (≡ (unwrap-or some-42 #99) #42))
(⊨ :test-unwrap-none #t (≡ (unwrap-or none #99) #99))

;; ============================================================
;; Summary
;; ============================================================

; Total: 37 ADT tests
; Categories:
; - Simple enums (4 tests)
; - Single field variants (4 tests)
; - Recursive lists (6 tests)
; - Binary trees (8 tests)
; - Expression ADT (7 tests)
; - Result ADT (4 tests)
; - Nested structures (5 tests)
; - Functions with ADTs (4 tests)
