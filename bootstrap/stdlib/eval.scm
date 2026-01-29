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
;; Recursive Letrec Support
;; ===================================================================

;; Check if symbol appears anywhere in expression (respects lambda shadowing)
(≔ contains-symbol? (λ (sym) (λ (expr)
  (? (:? expr)
     (≡ expr sym)
     (? (⟨⟩? expr)
        (? (∅? expr)
           #f
           (? (≡ (◁ expr) (⌜ λ))
              (? (⟨⟩? (▷ expr))
                 (? ((member? sym) (◁ (▷ expr)))
                    #f
                    ((contains-symbol-list? sym) (▷ (▷ expr))))
                 #f)
              ((contains-symbol-list? sym) expr)))
        #f)))))

;; Check if symbol appears in any expression in list
(≔ contains-symbol-list? (λ (sym) (λ (exprs)
  (? (∅? exprs)
     #f
     (? ((contains-symbol? sym) (◁ exprs))
        #t
        ((contains-symbol-list? sym) (▷ exprs)))))))

;; Check if a binding is recursive (name appears in body)
(≔ is-recursive-binding? (λ (binding)
  (? (⟨⟩? binding)
     (? (⟨⟩? (▷ binding))
        ((contains-symbol? (◁ binding)) (◁ (▷ binding)))
        #f)
     #f)))

;; Transform recursive binding using Y-combinator pattern
;; (λ (params) body) → ((λ (:self) (λ (params) body')) (λ (:self) (λ (params) body')))
;; where body' has `name` replaced with `(:self :self)`
(≔ transform-recursive-ast (λ (name) (λ (lambda-expr)
  (? (≡ (◁ lambda-expr) (⌜ λ))
     (? (⟨⟩? (▷ lambda-expr))
        (⟨⟩ (⟨⟩ (⌜ λ)
                (⟨⟩ (⟨⟩ :self ∅)
                    (⟨⟩ (⟨⟩ (⌜ λ)
                            (⟨⟩ (◁ (▷ lambda-expr))
                                (((subst-list name) (⟨⟩ :self (⟨⟩ :self ∅)))
                                 (▷ (▷ lambda-expr)))))
                        ∅)))
            (⟨⟩ (⟨⟩ (⌜ λ)
                    (⟨⟩ (⟨⟩ :self ∅)
                        (⟨⟩ (⟨⟩ (⌜ λ)
                                (⟨⟩ (◁ (▷ lambda-expr))
                                    (((subst-list name) (⟨⟩ :self (⟨⟩ :self ∅)))
                                     (▷ (▷ lambda-expr)))))
                            ∅)))
                ∅))
        lambda-expr)
     lambda-expr))))

;; ===================================================================
;; Mutual Recursion Support
;; ===================================================================

;; Extract all binding names from a list of bindings
;; bindings = ((name1 val1) (name2 val2) ...)
(≔ collect-binding-names (λ (bindings)
  (? (∅? bindings)
     ∅
     (⟨⟩ (◁ (◁ bindings))
         (collect-binding-names (▷ bindings))))))

;; Check if any symbol in a list appears in expression
(≔ contains-any-symbol? (λ (syms) (λ (expr)
  (? (∅? syms)
     #f
     (? ((contains-symbol? (◁ syms)) expr)
        #t
        ((contains-any-symbol? (▷ syms)) expr))))))

;; Check if binding references any name from a list of names
(≔ binding-references-names? (λ (binding) (λ (names)
  (? (⟨⟩? binding)
     (? (⟨⟩? (▷ binding))
        ((contains-any-symbol? names) (◁ (▷ binding)))
        #f)
     #f))))

;; Check if bindings form a mutual recursion group
;; Returns #t if any binding references another binding's name
(≔ is-mutual-recursion? (λ (bindings)
  (? (∅? bindings)
     #f
     (? (∅? (▷ bindings))
        #f  ; Only one binding - not mutual
        ; Check if first binding references any other binding's name
        ((is-mutual-recursion-helper? bindings) (collect-binding-names bindings))))))

;; Helper: check if any binding references a name other than its own
(≔ is-mutual-recursion-helper? (λ (bindings) (λ (all-names)
  (? (∅? bindings)
     #f
     ; For each binding, check if it references any OTHER name
     (? ((binding-references-other-name? (◁ bindings)) all-names)
        #t
        ((is-mutual-recursion-helper? (▷ bindings)) all-names))))))

;; Check if binding references any name other than its own
(≔ binding-references-other-name? (λ (binding) (λ (all-names)
  (? (⟨⟩? binding)
     (? (⟨⟩? (▷ binding))
        ((contains-any-symbol? ((remove-name (◁ binding)) all-names))
         (◁ (▷ binding)))
        #f)
     #f))))

;; Remove a name from a list of names
(≔ remove-name (λ (name) (λ (names)
  (? (∅? names)
     ∅
     (? (≡ name (◁ names))
        (▷ names)
        (⟨⟩ (◁ names) ((remove-name name) (▷ names))))))))

;; Build accessor expression for nth element of pair structure
;; 0 → (:◁ (:self :self))
;; 1 → (:▷ (:self :self))
;; For 2-function mutual recursion only
;; Note: Use (⌜ :◁) to get ::◁ (the keyword symbol that matches env bindings)
(≔ build-accessor (λ (index)
  (? (≡ index #0)
     (⟨⟩ (⌜ :◁) (⟨⟩ (⟨⟩ :self (⟨⟩ :self ∅)) ∅))     ; (:◁ (:self :self))
     (⟨⟩ (⌜ :▷) (⟨⟩ (⟨⟩ :self (⟨⟩ :self ∅)) ∅))))) ; (:▷ (:self :self))

;; Build substitution pairs for mutual recursion
;; Returns list of (name . accessor) where accessor is (◁ (:self :self)) or (▷ (:self :self))
(≔ build-mutual-substitutions (λ (names) (λ (index)
  (? (∅? names)
     ∅
     (⟨⟩ (⟨⟩ (◁ names) (build-accessor index))
         ((build-mutual-substitutions (▷ names)) (⊕ index #1)))))))

;; Apply multiple substitutions to expression
;; subs = ((name1 . replacement1) (name2 . replacement2) ...)
(≔ apply-substitutions (λ (subs) (λ (expr)
  (? (∅? subs)
     expr
     ((apply-substitutions (▷ subs))
      (((subst (◁ (◁ subs))) (▷ (◁ subs))) expr))))))

;; Transform a lambda body with mutual recursion substitutions
(≔ transform-mutual-lambda (λ (lambda-expr) (λ (subs)
  (? (≡ (◁ lambda-expr) (⌜ λ))
     (? (⟨⟩? (▷ lambda-expr))
        (⟨⟩ (⌜ λ)
            (⟨⟩ (◁ (▷ lambda-expr))  ; params
                ((apply-substitutions-list subs) (▷ (▷ lambda-expr)))))  ; body
        lambda-expr)
     lambda-expr))))

;; Apply substitutions to list of expressions
(≔ apply-substitutions-list (λ (subs) (λ (exprs)
  (? (∅? exprs)
     ∅
     (⟨⟩ ((apply-substitutions subs) (◁ exprs))
         ((apply-substitutions-list subs) (▷ exprs)))))))

;; Build the pair structure for mutual recursion
;; For 2 bindings: (:⟨⟩ transformed-lambda1 transformed-lambda2)
;; Note: Use (⌜ :⟨⟩) to get ::⟨⟩ (the keyword symbol that matches env bindings)
(≔ build-mutual-pair (λ (bindings) (λ (subs)
  (? (∅? bindings)
     ∅
     (? (∅? (▷ bindings))
        ; Last binding - just the transformed lambda
        ((transform-mutual-lambda (◁ (▷ (◁ bindings)))) subs)
        ; More bindings - cons together
        (⟨⟩ (⌜ :⟨⟩)
            (⟨⟩ ((transform-mutual-lambda (◁ (▷ (◁ bindings)))) subs)
                (⟨⟩ ((build-mutual-pair (▷ bindings)) subs)
                    ∅))))))))

;; Transform mutually recursive bindings using Y-combinator pattern
;; Returns the transformed expression that produces a pair of closures
(≔ transform-mutual-ast (λ (bindings)
  (? (∅? bindings)
     ∅
     (? (∅? (▷ bindings))
        ; Single binding - shouldn't happen but handle it
        (◁ (▷ (◁ bindings)))
        ; Multiple bindings - build mutual recursion structure
        (⟨⟩ (⟨⟩ (⌜ λ)
                (⟨⟩ (⟨⟩ :self ∅)
                    (⟨⟩ ((build-mutual-pair bindings)
                         ((build-mutual-substitutions (collect-binding-names bindings)) #0))
                        ∅)))
            (⟨⟩ (⟨⟩ (⌜ λ)
                    (⟨⟩ (⟨⟩ :self ∅)
                        (⟨⟩ ((build-mutual-pair bindings)
                             ((build-mutual-substitutions (collect-binding-names bindings)) #0))
                            ∅)))
                ∅))))))

;; Bind mutual recursion results to names
;; pair-result is the evaluated pair, bindings are the original bindings
;; Returns extended environment
(≔ bind-mutual-results (λ (pair-result) (λ (bindings) (λ (env)
  (? (∅? bindings)
     env
     (? (∅? (▷ bindings))
        ; Last binding - bind to the result (not pair, just the function)
        (((env-extend env) (◁ (◁ bindings))) pair-result)
        ; First binding - bind to (◁ pair-result), recurse with (▷ pair-result)
        (((bind-mutual-results (▷ pair-result))
          (▷ bindings))
         (((env-extend env) (◁ (◁ bindings))) (◁ pair-result)))))))))

;; Evaluate mutually recursive bindings
(≔ eval-mutual-letrec (λ (bindings) (λ (body) (λ (env)
  (? (∅? bindings)
     ((eval body) env)
     ; Transform and evaluate the mutual recursion structure
     ((eval body)
      (((bind-mutual-results
         ((eval (transform-mutual-ast bindings)) env))
        bindings)
       env)))))))

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
;; Letrec Evaluation (⊛) - With mutual recursion support
;; ===================================================================

;; Evaluate letrec bindings
;; Handles: non-recursive, single recursive, and mutually recursive bindings
(≔ eval-letrec (λ (bindings) (λ (body) (λ (env)
  (? (∅? bindings)
     ((eval body) env)
     ; Check for mutual recursion first (multiple bindings referencing each other)
     (? (is-mutual-recursion? bindings)
        ; Mutual recursion - transform all bindings together
        (((eval-mutual-letrec bindings) body) env)
        ; Not mutual - check for single recursive binding
        (? (is-recursive-binding? (◁ bindings))
           ; Recursive - transform using Y-combinator pattern
           (((eval-letrec (▷ bindings)) body)
            (((env-extend env)
              (◁ (◁ bindings)))
             ((eval ((transform-recursive-ast (◁ (◁ bindings)))
                     (◁ (▷ (◁ bindings)))))
              env)))
           ; Non-recursive - simple binding
           (((eval-letrec (▷ bindings)) body)
            (((env-extend env)
              (◁ (◁ bindings)))
             ((eval (◁ (▷ (◁ bindings)))) env))))))))))

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
