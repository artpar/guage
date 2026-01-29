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
              (? (≡ (◁ expr) (⌜ ≔))
                 #t
                 (? (≡ (◁ expr) (⌜ ⊛))
                    #t
                    (? (≡ (◁ expr) (⌜ ⌞))
                       #t
                       #f)))))))))

;; Evaluate a lambda expression
;; Creates a closure: (:closure params body env)
(≔ eval-lambda (λ (params) (λ (body) (λ (env)
  (⟨⟩ :closure (⟨⟩ params (⟨⟩ body env)))))))

;; Evaluate a conditional expression
(≔ eval-if (λ (cond-expr) (λ (then-expr) (λ (else-expr) (λ (env)
  (? ((eval cond-expr) env)
     ((eval then-expr) env)
     ((eval else-expr) env)))))))

;; ===================================================================
;; Letrec Support (⊛) - Substitution helpers
;; ===================================================================

;; Check if symbol is in a list
(≔ member? (λ (x) (λ (lst)
  (? (∅? lst)
     #f
     (? (≡ x (◁ lst))
        #t
        ((member? x) (▷ lst)))))))

;; Substitute name with replacement in expression
;; Handles lambda shadowing correctly
(≔ subst (λ (name) (λ (replacement) (λ (expr)
  (? (:? expr)
     ; Symbol - check if it matches
     (? (≡ expr name) replacement expr)
     (? (⟨⟩? expr)
        ; List - check for lambda (shadowing) or recurse
        (? (∅? expr)
           expr
           (? (≡ (◁ expr) (⌜ λ))
              ; Lambda - check if name is shadowed by params
              (? (⟨⟩? (▷ expr))
                 (? ((member? name) (◁ (▷ expr)))
                    expr  ; Name shadowed, don't substitute
                    ; Substitute in body only
                    (⟨⟩ (⌜ λ)
                        (⟨⟩ (◁ (▷ expr))
                            (((subst-list name) replacement) (▷ (▷ expr))))))
                 expr)
              ; Not lambda - substitute in all elements
              (((subst-list name) replacement) expr)))
        ; Atom - return unchanged
        expr))))))

;; Substitute in a list of expressions
(≔ subst-list (λ (name) (λ (replacement) (λ (exprs)
  (? (∅? exprs)
     ∅
     (⟨⟩ (((subst name) replacement) (◁ exprs))
         (((subst-list name) replacement) (▷ exprs))))))))

;; Substitute multiple names at once
(≔ subst-all (λ (names) (λ (replacements) (λ (expr)
  (? (∅? names)
     expr
     (((subst-all (▷ names)) (▷ replacements))
      (((subst (◁ names)) (◁ replacements)) expr)))))))

;; ===================================================================
;; Evaluation helpers
;; ===================================================================

;; Evaluate a body (sequence of expressions)
;; Handles defines by extending environment for subsequent expressions
;; Non-define expressions in non-final position are skipped (no side effects)
;; Returns value of last expression
(≔ eval-body (λ (exprs) (λ (env)
  (? (∅? exprs)
     ∅                                    ; Empty body returns nil
     (? (∅? (▷ exprs))
        ((eval (◁ exprs)) env)           ; Last expression - evaluate and return
        ; More expressions follow - check for define
        (? (⟨⟩? (◁ exprs))
           (? (≡ (◁ (◁ exprs)) (⌜ ≔))
              ; It's a define: (≔ name value) - extend env for rest
              ; (◁ exprs) = (≔ name value)
              ; (◁ (▷ (◁ exprs))) = name
              ; (◁ (▷ (▷ (◁ exprs)))) = value-expr
              ((eval-body (▷ exprs))
               (((env-extend env)
                 (◁ (▷ (◁ exprs))))              ; name
                ((eval (◁ (▷ (▷ (◁ exprs))))) env))) ; evaluated value
              ; Not a define - skip and continue
              ((eval-body (▷ exprs)) env))
           ; Not a list - skip and continue
           ((eval-body (▷ exprs)) env)))))))

;; ===================================================================
;; Letrec Evaluation (⊛) - Simple let-style (no mutual recursion yet)
;; ===================================================================

;; Evaluate letrec as sequential let bindings
;; For recursive single functions, this won't work correctly
;; Full letrec requires Y-combinator transformation
(≔ eval-letrec (λ (bindings) (λ (body) (λ (env)
  (? (∅? bindings)
     ((eval body) env)
     ; Extend env with first binding and recurse
     (((eval-letrec (▷ bindings)) body)
      (((env-extend env)
        (◁ (◁ bindings)))
       ((eval (◁ (▷ (◁ bindings)))) env))))))))

;; ===================================================================
;; Function Application
;; ===================================================================

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
           ; Closure: extract params, body-exprs, closure-env
           ; fn = (:closure . (params . (body-exprs . closure-env)))
           (? (⟨⟩? (▷ fn))
              ; Get params and rest
              (? (⟨⟩? (▷ (▷ fn)))
                 ; Use eval-body for body-exprs (supports sequences with define)
                 ((eval-body (◁ (▷ (▷ fn))))    ; body-exprs
                  (((bind-params (◁ (▷ fn)))     ; params
                    args)
                   (▷ (▷ (▷ fn)))))              ; closure-env
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
        ; Lambda: (λ (params...) body-exprs...)
        (? (⟨⟩? (▷ expr))
           (? (⟨⟩? (▷ (▷ expr)))
              (((eval-lambda (◁ (▷ expr)))   ; params list
                (▷ (▷ expr)))                ; body-exprs (full list for sequences)
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
              ; Define: (≔ name value) - evaluate value and return it
              ; Note: Environment extension only persists in body context (eval-body)
              (? (≡ (◁ expr) (⌜ ≔))
                 (? (⟨⟩? (▷ expr))
                    (? (⟨⟩? (▷ (▷ expr)))
                       ((eval (◁ (▷ (▷ expr)))) env)  ; Evaluate and return value
                       (⚠ :define-missing-value expr))
                    (⚠ :define-missing-name expr))
                 ; Letrec: (⊛ ((name1 val1) (name2 val2) ...) body)
                 ; Uses self-application transformation for recursion
                 (? (≡ (◁ expr) (⌜ ⊛))
                    (? (⟨⟩? (▷ expr))
                       (? (⟨⟩? (▷ (▷ expr)))
                          (((eval-letrec (◁ (▷ expr)))   ; bindings
                            (◁ (▷ (▷ expr))))            ; body
                           env)
                          (⚠ :letrec-missing-body expr))
                       (⚠ :letrec-missing-bindings expr))
                    ; Meta-eval: (⌞ expr) - evaluate expr, then evaluate result
                    (? (≡ (◁ expr) (⌜ ⌞))
                       (? (⟨⟩? (▷ expr))
                          ((eval ((eval (◁ (▷ expr))) env)) env)
                          (⚠ :eval-missing-expr expr))
                       (⚠ :unknown-special-form expr)))))))
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
