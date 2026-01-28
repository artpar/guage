;;;
;;; Simple S-Expression Evaluator
;;; Minimal version - just numbers, symbols, and primitives
;;;

(⋘ "bootstrap/stdlib/eval-env.scm")

;; Evaluate an expression
;; For now: just handle atoms and don't handle special forms
(≔ eval-simple (λ (expr) (λ (env)
  (? (ℕ? expr)
     expr                                   ; Numbers self-evaluate
     (? (𝔹? expr)
        expr                                ; Booleans self-evaluate
        (? (:? expr)
           ((env-lookup env) expr)          ; Symbol lookup
           (? (∅? expr)
              expr                          ; Nil self-evaluates
              ; Lists - function application
              (? (⟨⟩? expr)
                 ; For now, just evaluate function and args
                 ; Don't handle special forms yet
                 expr
                 expr))))))))                ; Default: return as-is
