;; Tests for :? (is-symbol) primitive
;; This tests the fix for the keyword/primitive conflict issue

;; Test :? with symbols
(⊨ :test-symbol-keyword #t (:? :test))
(⊨ :test-symbol-Point #t (:? :Point))
(⊨ :test-symbol-x #t (:? :x))
(⊨ :test-symbol-Cons #t (:? :Cons))

;; Test :? with non-symbols
(⊨ :test-not-symbol-number #f (:? #42))
(⊨ :test-not-symbol-bool-true #f (:? #t))
(⊨ :test-not-symbol-bool-false #f (:? #f))
(⊨ :test-not-symbol-nil #f (:? ∅))
(⊨ :test-not-symbol-pair #f (:? (⟨⟩ #1 #2)))

;; Test :? as value (should still self-evaluate as keyword)
(⊨ :test-keyword-self-eval #t (≡ :? :?))

;; Test :? in variables
(≔ sym-test (:? :hello))
(≔ num-test (:? #99))
(⊨ :test-var-symbol #t sym-test)
(⊨ :test-var-number #f num-test)

;; Test :? with function results
(≔ make-symbol (λ (x) :result))
(⊨ :test-function-result #t (:? (make-symbol #1)))

;; Total: 15 tests for :? primitive
