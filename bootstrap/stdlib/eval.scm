;;;
;;; S-Expression Evaluator
;;; Simple evaluator for numbers, booleans, symbols, and basic lists
;;;

(⋘ "bootstrap/stdlib/eval-env.scm")

;; ===================================================================
;; Atom Evaluation
;; ===================================================================

;; Evaluate an atomic expression
;; expr: Expression to evaluate
;; env: Environment for variable lookup
;; Returns: Evaluated value
(≔ eval-atom (λ (expr) (λ (env)
  (? (ℕ? expr)
     expr                           ; Numbers self-evaluate
     (? (𝔹? expr)
        expr                        ; Booleans self-evaluate
        (? (∅? expr)
           expr                     ; Nil self-evaluates
           (? (:? expr)
              ((env-lookup env) expr)  ; Symbol lookup
              expr)))))))          ; Everything else self-evaluates

;; ===================================================================
;; List Evaluation (Function Application)
;; ===================================================================

;; Check if expression is a special form
(≔ special-form? (λ (expr)
  (? (∅? expr)
     #f
     (? (≡ (◁ expr) (⌜ λ))
        #t
        (? (≡ (◁ expr) (⌜ ?))
           #t
           (? (≡ (◁ expr) (⌜ ⌜))
              #t
              #f))))))

;; Evaluate a lambda expression
;; Creates a closure: (:closure params body env)
(≔ eval-lambda (λ (params) (λ (body) (λ (env)
  (⟨⟩ :closure (⟨⟩ params (⟨⟩ body env)))))))

;; Evaluate a conditional expression
(≔ eval-if (λ (cond-expr) (λ (then-expr) (λ (else-expr) (λ (env)
  (? ((eval cond-expr) env)
     ((eval then-expr) env)
     ((eval else-expr) env)))))))

;; Bind parameters to arguments in environment
(≔ bind-params (λ (params) (λ (args) (λ (env)
  (? (∅? params)
     env
     (((bind-params (▷ params))
       (▷ args))
      (((env-extend env) (◁ params)) (◁ args))))))))

;; Apply a function to arguments
;; fn: Function to apply (closure or primitive)
;; args: List of argument values
;; env: Current environment
(≔ apply-fn (λ (fn) (λ (args) (λ (env)
  (? (:? fn)
     (((apply-fn ((env-lookup env) fn)) args) env)  ; Look up and apply
     (? (⟨⟩? fn)
        ; fn is a pair - check if it's a closure
        (? (≡ (◁ fn) :closure)
           ; Closure: extract params, body, closure-env
           (? (⟨⟩? (▷ fn))
              ; Get params and rest
              (? (⟨⟩? (▷ (▷ fn)))
                 ; body-env-pair = (body . env)
                 ((eval (◁ (▷ (▷ fn))))    ; body
                  (((bind-params (◁ (▷ fn)))  ; params
                    args)
                   (▷ (▷ (▷ fn)))))          ; closure-env
                 (⚠ :invalid-closure-structure fn))
              (⚠ :invalid-closure-structure fn))
           (⚠ :not-a-closure fn))
        ; fn is not a symbol or pair - must be a primitive
        (⊡ fn args)))))))

;; Evaluate list of expressions
(≔ eval-list-args (λ (exprs) (λ (env)
  (? (∅? exprs)
     ∅
     (⟨⟩ ((eval (◁ exprs)) env)
         ((eval-list-args (▷ exprs)) env))))))

;; Evaluate a list expression (function application)
(≔ eval-list (λ (expr) (λ (env)
  ; Check for special forms first
  (? (special-form? expr)
     ; Handle special forms
     (? (≡ (◁ expr) (⌜ λ))
        ; Lambda: (λ (params...) body)
        (? (⟨⟩? (▷ expr))
           (? (⟨⟩? (▷ (▷ expr)))
              (((eval-lambda (◁ (▷ expr)))   ; params list
                (◁ (▷ (▷ expr))))            ; body (first of body list)
               env)
              (⚠ :lambda-missing-body expr))
           (⚠ :lambda-missing-params expr))
        (? (≡ (◁ expr) (⌜ ?))
           ; Conditional: (? cond then else)
           (? (⟨⟩? (▷ expr))
              (? (⟨⟩? (▷ (▷ expr)))
                 (? (⟨⟩? (▷ (▷ (▷ expr))))
                    ((((eval-if (◁ (▷ expr)))       ; cond
                       (◁ (▷ (▷ expr))))            ; then
                      (◁ (▷ (▷ (▷ expr)))))         ; else
                     env)
                    (⚠ :if-missing-else expr))
                 (⚠ :if-missing-then expr))
              (⚠ :if-missing-condition expr))
           ; Quote: (⌜ expr) - return expr unevaluated
           (? (≡ (◁ expr) (⌜ ⌜))
              (? (⟨⟩? (▷ expr))
                 (◁ (▷ expr))            ; Return quoted expression
                 (⚠ :quote-missing-expr expr))
              (⚠ :unknown-special-form expr))))
     ; Regular function application
     (? (∅? expr)
        (⚠ :empty-application)
        ; Evaluate function
        (((apply-fn ((eval (◁ expr)) env))
          ((eval-list-args (▷ expr)) env))
         env))))))

;; ===================================================================
;; Main Evaluator
;; ===================================================================

;; Main evaluation function
;; expr: Expression to evaluate
;; env: Environment
;; Returns: Evaluated result
(≔ eval (λ (expr) (λ (env)
  (? (⟨⟩? expr)
     ((eval-list expr) env)
     ((eval-atom expr) env)))))
