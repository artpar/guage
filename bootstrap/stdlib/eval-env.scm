;;;
;;; Environment Module - Version 2 (Simpler approach)
;;; Avoid pattern matching for now, use direct conditionals
;;;

;; Create empty environment
(≔ env-empty (λ () ∅))

;; Extend environment with new binding
(≔ env-extend (λ (env) (λ (name) (λ (value)
  (⟨⟩ (⟨⟩ name value) env)))))

;; Look up symbol in environment
;; Uses direct recursion with conditionals instead of pattern matching
(≔ env-lookup (λ (env) (λ (key)
  (? (∅? env)
     (⚠ :undefined-variable key)
     (? (≡ (◁ (◁ env)) key)
        (▷ (◁ env))           ; Return value
        ((env-lookup (▷ env)) key))))))  ; Recurse on rest

;; Check if symbol is bound in environment
(≔ env-has? (λ (env) (λ (key)
  (? (∅? env)
     #f
     (? (≡ (◁ (◁ env)) key)
        #t
        ((env-has? (▷ env)) key))))))

;; Create environment from list of bindings
(≔ env-from-list (λ (bindings)
  (? (∅? bindings)
     (env-empty)
     (((env-extend (env-from-list (▷ bindings)))
       (◁ (◁ bindings)))
      (▷ (◁ bindings))))))
