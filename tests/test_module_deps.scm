;;;; Day 29: Module Dependency Tracking Tests
;;;; Validates automatic dependency tracking when modules load other modules

;; Create test modules with dependencies
(≋⊲ "../../tests/fixtures/base_module.scm" "(≔ BASE #10)")
(≋⊲ "../../tests/fixtures/derived_module.scm" "(⋘ \"../../tests/fixtures/base_module.scm\") (≔ DERIVED (⊕ BASE #1))")
(≋⊲ "../../tests/fixtures/independent_module.scm" "(≔ INDEPENDENT #42)")

;; Test 1: Load derived module creates dependency
;; When module loads another, dependency is tracked
(⋘ "../../tests/fixtures/derived_module.scm")
(≔ deps (⌂⊚→ "../../tests/fixtures/derived_module.scm"))
(⊢ (¬ (∅? deps))
   "Derived module should have dependencies")

;; Test 2: Dependency list contains base module
;; The dependency should be the specific module that was loaded
(≔ has-base (λ (lst)
  (? (∅? lst)
     #f
     (? (≈≡ (◁ lst) "../../tests/fixtures/base_module.scm")
        #t
        (has-base (▷ lst))))))

(⊢ (has-base deps)
   "Derived module should depend on base module")

;; Test 3: Independent module has no dependencies
;; Module that doesn't load others has empty dependency list
(⋘ "../../tests/fixtures/independent_module.scm")
(≔ indep-deps (⌂⊚→ "../../tests/fixtures/independent_module.scm"))
(⊢ (∅? indep-deps)
   "Independent module should have no dependencies")

;; Test 4: Base module has no dependencies
;; Module loaded first has no dependencies (it didn't load anything)
(≔ base-deps (⌂⊚→ "../../tests/fixtures/base_module.scm"))
(⊢ (∅? base-deps)
   "Base module should have no dependencies")

;; Test 5: Error on unloaded module
;; Querying dependencies of unloaded module returns error
(⊢ (⚠? (⌂⊚→ "../../tests/fixtures/never_loaded.scm"))
   "Dependency query on unloaded module should return error")

;; Test 6: No self-dependencies
;; Module doesn't depend on itself even if it references itself
(≋⊲ "../../tests/fixtures/self_ref.scm" "(≔ recurse (λ (n) (? (≡ n #0) #1 (⊗ n ((recurse (⊖ n #1)))))))")
(⋘ "../../tests/fixtures/self_ref.scm")
(≔ self-deps (⌂⊚→ "../../tests/fixtures/self_ref.scm"))
(⊢ (∅? self-deps)
   "Module with recursion should not depend on itself")

;; Test 7: Transitive dependencies not tracked directly
;; Only direct dependencies (explicit loads) are tracked
;; If A loads B and B loads C, A's deps = {B}, not {B, C}
(≋⊲ "../../tests/fixtures/trans_c.scm" "(≔ C #3)")
(≋⊲ "../../tests/fixtures/trans_b.scm" "(⋘ \"../../tests/fixtures/trans_c.scm\") (≔ B (⊕ C #2))")
(≋⊲ "../../tests/fixtures/trans_a.scm" "(⋘ \"../../tests/fixtures/trans_b.scm\") (≔ A (⊕ B #1))")
(⋘ "../../tests/fixtures/trans_a.scm")

(≔ a-deps (⌂⊚→ "../../tests/fixtures/trans_a.scm"))
(≔ has-b (λ (lst)
  (? (∅? lst)
     #f
     (? (≈≡ (◁ lst) "../../tests/fixtures/trans_b.scm")
        #t
        (has-b (▷ lst))))))

(≔ has-c (λ (lst)
  (? (∅? lst)
     #f
     (? (≈≡ (◁ lst) "../../tests/fixtures/trans_c.scm")
        #t
        (has-c (▷ lst))))))

(⊢ (has-b a-deps)
   "Module A should depend on B (direct load)")

(⊢ (¬ (has-c a-deps))
   "Module A should NOT depend on C (transitive only)")
