;;;
;;; S-Expression Evaluator
;;; Simple evaluator for numbers, booleans, symbols, and basic lists
;;;

(load "bootstrap/stdlib/eval-env.scm")

;; ===================================================================
;; Atom Evaluation
;; ===================================================================

;; Evaluate an atomic expression
;; expr: Expression to evaluate
;; env: Environment for variable lookup
;; Returns: Evaluated value
(define eval-atom (lambda (expr) (lambda (env)
  (if (number? expr)
     expr                           ; Numbers self-evaluate
     (if (boolean? expr)
        expr                        ; Booleans self-evaluate
        (if (null? expr)
           expr                     ; Nil self-evaluates
           (if (symbol? expr)
              ((env-lookup env) expr)  ; Symbol lookup
              expr)))))))          ; Everything else self-evaluates

;; ===================================================================
;; List Evaluation (Function Application)
;; ===================================================================

;; Check if expression is a special form
(define special-form? (lambda (expr)
  (if (null? expr)
     #f
     (if (equal? (car expr) (quote lambda))
        #t
        (if (equal? (car expr) (quote if))
           #t
           (if (equal? (car expr) (quote quote))
              #t
              (if (equal? (car expr) (quote define))
                 #t
                 (if (equal? (car expr) (quote source))
                    #t
                    (if (equal? (car expr) (quote eval))
                       #t
                       #f)))))))))

;; Evaluate a lambda expression
;; Creates a closure: (:closure params body env)
(define eval-lambda (lambda (params) (lambda (body) (lambda (env)
  (cons :closure (cons params (cons body env)))))))

;; Evaluate a conditional expression
(define eval-if (lambda (cond-expr) (lambda (then-expr) (lambda (else-expr) (lambda (env)
  (if ((eval cond-expr) env)
     ((eval then-expr) env)
     ((eval else-expr) env)))))))

;; ===================================================================
;; Letrec Support (source) - Substitution helpers
;; ===================================================================

;; Check if symbol is in a list
(define member? (lambda (x) (lambda (lst)
  (if (null? lst)
     #f
     (if (equal? x (car lst))
        #t
        ((member? x) (cdr lst)))))))

;; Substitute name with replacement in expression
;; Handles lambda shadowing correctly
(define subst (lambda (name) (lambda (replacement) (lambda (expr)
  (if (symbol? expr)
     ; Symbol - check if it matches
     (if (equal? expr name) replacement expr)
     (if (pair? expr)
        ; List - check for lambda (shadowing) or recurse
        (if (null? expr)
           expr
           (if (equal? (car expr) (quote lambda))
              ; Lambda - check if name is shadowed by params
              (if (pair? (cdr expr))
                 (if ((member? name) (car (cdr expr)))
                    expr  ; Name shadowed, don't substitute
                    ; Substitute in body only
                    (cons (quote lambda)
                        (cons (car (cdr expr))
                            (((subst-list name) replacement) (cdr (cdr expr))))))
                 expr)
              ; Not lambda - substitute in all elements
              (((subst-list name) replacement) expr)))
        ; Atom - return unchanged
        expr))))))

;; Substitute in a list of expressions
(define subst-list (lambda (name) (lambda (replacement) (lambda (exprs)
  (if (null? exprs)
     nil
     (cons (((subst name) replacement) (car exprs))
         (((subst-list name) replacement) (cdr exprs))))))))

;; Substitute multiple names at once
(define subst-all (lambda (names) (lambda (replacements) (lambda (expr)
  (if (null? names)
     expr
     (((subst-all (cdr names)) (cdr replacements))
      (((subst (car names)) (car replacements)) expr)))))))

;; ===================================================================
;; Recursive Letrec Support
;; ===================================================================

;; Check if symbol appears anywhere in expression (respects lambda shadowing)
(define contains-symbol? (lambda (sym) (lambda (expr)
  (if (symbol? expr)
     (equal? expr sym)
     (if (pair? expr)
        (if (null? expr)
           #f
           (if (equal? (car expr) (quote lambda))
              (if (pair? (cdr expr))
                 (if ((member? sym) (car (cdr expr)))
                    #f
                    ((contains-symbol-list? sym) (cdr (cdr expr))))
                 #f)
              ((contains-symbol-list? sym) expr)))
        #f)))))

;; Check if symbol appears in any expression in list
(define contains-symbol-list? (lambda (sym) (lambda (exprs)
  (if (null? exprs)
     #f
     (if ((contains-symbol? sym) (car exprs))
        #t
        ((contains-symbol-list? sym) (cdr exprs)))))))

;; Check if a binding is recursive (name appears in body)
(define is-recursive-binding? (lambda (binding)
  (if (pair? binding)
     (if (pair? (cdr binding))
        ((contains-symbol? (car binding)) (car (cdr binding)))
        #f)
     #f)))

;; Transform recursive binding using Y-combinator pattern
;; (lambda (params) body) -> ((lambda (:self) (lambda (params) body')) (lambda (:self) (lambda (params) body')))
;; where body' has `name` replaced with `(:self :self)`
(define transform-recursive-ast (lambda (name) (lambda (lambda-expr)
  (if (equal? (car lambda-expr) (quote lambda))
     (if (pair? (cdr lambda-expr))
        (cons (cons (quote lambda)
                (cons (cons :self nil)
                    (cons (cons (quote lambda)
                            (cons (car (cdr lambda-expr))
                                (((subst-list name) (cons :self (cons :self nil)))
                                 (cdr (cdr lambda-expr)))))
                        nil)))
            (cons (cons (quote lambda)
                    (cons (cons :self nil)
                        (cons (cons (quote lambda)
                                (cons (car (cdr lambda-expr))
                                    (((subst-list name) (cons :self (cons :self nil)))
                                     (cdr (cdr lambda-expr)))))
                            nil)))
                nil))
        lambda-expr)
     lambda-expr))))

;; ===================================================================
;; Mutual Recursion Support
;; ===================================================================

;; Extract all binding names from a list of bindings
;; bindings = ((name1 val1) (name2 val2) ...)
(define collect-binding-names (lambda (bindings)
  (if (null? bindings)
     nil
     (cons (car (car bindings))
         (collect-binding-names (cdr bindings))))))

;; Check if any symbol in a list appears in expression
(define contains-any-symbol? (lambda (syms) (lambda (expr)
  (if (null? syms)
     #f
     (if ((contains-symbol? (car syms)) expr)
        #t
        ((contains-any-symbol? (cdr syms)) expr))))))

;; Check if binding references any name from a list of names
(define binding-references-names? (lambda (binding) (lambda (names)
  (if (pair? binding)
     (if (pair? (cdr binding))
        ((contains-any-symbol? names) (car (cdr binding)))
        #f)
     #f))))

;; Check if bindings form a mutual recursion group
;; Returns #t if any binding references another binding's name
(define is-mutual-recursion? (lambda (bindings)
  (if (null? bindings)
     #f
     (if (null? (cdr bindings))
        #f  ; Only one binding - not mutual
        ; Check if first binding references any other binding's name
        ((is-mutual-recursion-helper? bindings) (collect-binding-names bindings))))))

;; Helper: check if any binding references a name other than its own
(define is-mutual-recursion-helper? (lambda (bindings) (lambda (all-names)
  (if (null? bindings)
     #f
     ; For each binding, check if it references any OTHER name
     (if ((binding-references-other-name? (car bindings)) all-names)
        #t
        ((is-mutual-recursion-helper? (cdr bindings)) all-names))))))

;; Check if binding references any name other than its own
(define binding-references-other-name? (lambda (binding) (lambda (all-names)
  (if (pair? binding)
     (if (pair? (cdr binding))
        ((contains-any-symbol? ((remove-name (car binding)) all-names))
         (car (cdr binding)))
        #f)
     #f))))

;; Remove a name from a list of names
(define remove-name (lambda (name) (lambda (names)
  (if (null? names)
     nil
     (if (equal? name (car names))
        (cdr names)
        (cons (car names) ((remove-name name) (cdr names))))))))

;; Build accessor expression for nth element of pair structure
;; Supports N-function mutual recursion (Day 80)
;; Structure: (cons f0 (cons f1 (cons f2 ... fN-1)))
;; index 0, total N -> (:car (:self :self))
;; index 1, total N -> (:car (:cdr (:self :self)))
;; index N-1, total N -> (:cdr (:cdr ... (:self :self)))  (N-1 tails, no head)
;; Note: Use (quote :car) to get ::◁ (the keyword symbol that matches env bindings)
(define build-accessor (lambda (index) (lambda (total)
  (if (equal? (+ index #1) total)
     ; Last function - just tails, no head
     (build-accessor-tails index)
     ; Not last - tails then head
     (cons (quote :car) (cons (build-accessor-tails index) nil))))))

;; Build nested tail expressions: (:cdr (:cdr ... (:self :self)))
;; n = 0 -> (:self :self)
;; n = 1 -> (:cdr (:self :self))
;; n = 2 -> (:cdr (:cdr (:self :self)))
(define build-accessor-tails (lambda (n)
  (if (equal? n #0)
     (cons :self (cons :self nil))  ; (:self :self)
     (cons (quote :cdr) (cons (build-accessor-tails (- n #1)) nil)))))

;; Count list length
(define list-length (lambda (lst)
  (if (null? lst)
     #0
     (+ #1 (list-length (cdr lst))))))

;; Build substitution pairs for mutual recursion
;; Returns list of (name . accessor) for N-function mutual recursion
;; names = list of binding names, index = current position, total = length of names
(define build-mutual-substitutions (lambda (names) (lambda (index) (lambda (total)
  (if (null? names)
     nil
     (cons (cons (car names) ((build-accessor index) total))
         (((build-mutual-substitutions (cdr names)) (+ index #1)) total)))))))

;; Apply multiple substitutions to expression
;; subs = ((name1 . replacement1) (name2 . replacement2) ...)
(define apply-substitutions (lambda (subs) (lambda (expr)
  (if (null? subs)
     expr
     ((apply-substitutions (cdr subs))
      (((subst (car (car subs))) (cdr (car subs))) expr))))))

;; Transform a lambda body with mutual recursion substitutions
(define transform-mutual-lambda (lambda (lambda-expr) (lambda (subs)
  (if (equal? (car lambda-expr) (quote lambda))
     (if (pair? (cdr lambda-expr))
        (cons (quote lambda)
            (cons (car (cdr lambda-expr))  ; params
                ((apply-substitutions-list subs) (cdr (cdr lambda-expr)))))  ; body
        lambda-expr)
     lambda-expr))))

;; Apply substitutions to list of expressions
(define apply-substitutions-list (lambda (subs) (lambda (exprs)
  (if (null? exprs)
     nil
     (cons ((apply-substitutions subs) (car exprs))
         ((apply-substitutions-list subs) (cdr exprs)))))))

;; Build the pair structure for mutual recursion
;; For 2 bindings: (:cons transformed-lambda1 transformed-lambda2)
;; Note: Use (quote :cons) to get ::⟨⟩ (the keyword symbol that matches env bindings)
(define build-mutual-pair (lambda (bindings) (lambda (subs)
  (if (null? bindings)
     nil
     (if (null? (cdr bindings))
        ; Last binding - just the transformed lambda
        ((transform-mutual-lambda (car (cdr (car bindings)))) subs)
        ; More bindings - cons together
        (cons (quote :cons)
            (cons ((transform-mutual-lambda (car (cdr (car bindings)))) subs)
                (cons ((build-mutual-pair (cdr bindings)) subs)
                    nil))))))))

;; Transform mutually recursive bindings using Y-combinator pattern
;; Returns the transformed expression that produces nested pairs of closures
;; Supports N-function mutual recursion (Day 80)
(define transform-mutual-ast (lambda (bindings)
  (if (null? bindings)
     nil
     (if (null? (cdr bindings))
        ; Single binding - shouldn't happen but handle it
        (car (cdr (car bindings)))
        ; Multiple bindings - build mutual recursion structure
        ; Extract names and total once for efficiency
        ((transform-mutual-ast-helper bindings)
         (collect-binding-names bindings))))))

;; Helper that takes bindings and precomputed names
(define transform-mutual-ast-helper (lambda (bindings) (lambda (names)
  ((lambda (subs)
     (cons (cons (quote lambda)
             (cons (cons :self nil)
                 (cons ((build-mutual-pair bindings) subs)
                     nil)))
         (cons (cons (quote lambda)
                 (cons (cons :self nil)
                     (cons ((build-mutual-pair bindings) subs)
                         nil)))
             nil)))
   (((build-mutual-substitutions names) #0) (list-length names))))))

;; Bind mutual recursion results to names
;; pair-result is the evaluated pair, bindings are the original bindings
;; Returns extended environment
(define bind-mutual-results (lambda (pair-result) (lambda (bindings) (lambda (env)
  (if (null? bindings)
     env
     (if (null? (cdr bindings))
        ; Last binding - bind to the result (not pair, just the function)
        (((env-extend env) (car (car bindings))) pair-result)
        ; First binding - bind to (car pair-result), recurse with (cdr pair-result)
        (((bind-mutual-results (cdr pair-result))
          (cdr bindings))
         (((env-extend env) (car (car bindings))) (car pair-result)))))))))

;; Evaluate mutually recursive bindings
(define eval-mutual-letrec (lambda (bindings) (lambda (body) (lambda (env)
  (if (null? bindings)
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
(define eval-body (lambda (exprs) (lambda (env)
  (if (null? exprs)
     nil                                    ; Empty body returns nil
     (if (null? (cdr exprs))
        ((eval (car exprs)) env)           ; Last expression - evaluate and return
        ; More expressions follow - check for define
        (if (pair? (car exprs))
           (if (equal? (car (car exprs)) (quote define))
              ; It's a define: (define name value) - extend env for rest
              ; (car exprs) = (define name value)
              ; (car (cdr (car exprs))) = name
              ; (car (cdr (cdr (car exprs)))) = value-expr
              ((eval-body (cdr exprs))
               (((env-extend env)
                 (car (cdr (car exprs))))              ; name
                ((eval (car (cdr (cdr (car exprs))))) env))) ; evaluated value
              ; Not a define - skip and continue
              ((eval-body (cdr exprs)) env))
           ; Not a list - skip and continue
           ((eval-body (cdr exprs)) env)))))))

;; ===================================================================
;; Letrec Evaluation (source) - With mutual recursion support
;; ===================================================================

;; Evaluate letrec bindings
;; Handles: non-recursive, single recursive, and mutually recursive bindings
(define eval-letrec (lambda (bindings) (lambda (body) (lambda (env)
  (if (null? bindings)
     ((eval body) env)
     ; Check for mutual recursion first (multiple bindings referencing each other)
     (if (is-mutual-recursion? bindings)
        ; Mutual recursion - transform all bindings together
        (((eval-mutual-letrec bindings) body) env)
        ; Not mutual - check for single recursive binding
        (if (is-recursive-binding? (car bindings))
           ; Recursive - transform using Y-combinator pattern
           (((eval-letrec (cdr bindings)) body)
            (((env-extend env)
              (car (car bindings)))
             ((eval ((transform-recursive-ast (car (car bindings)))
                     (car (cdr (car bindings)))))
              env)))
           ; Non-recursive - simple binding
           (((eval-letrec (cdr bindings)) body)
            (((env-extend env)
              (car (car bindings)))
             ((eval (car (cdr (car bindings)))) env))))))))))

;; ===================================================================
;; Function Application
;; ===================================================================

;; Bind parameters to arguments in environment
(define bind-params (lambda (params) (lambda (args) (lambda (env)
  (if (null? params)
     env
     (((bind-params (cdr params))
       (cdr args))
      (((env-extend env) (car params)) (car args))))))))

;; Apply a function to arguments
;; fn: Function to apply (closure or primitive)
;; args: List of argument values
;; env: Current environment
(define apply-fn (lambda (fn) (lambda (args) (lambda (env)
  (if (symbol? fn)
     (((apply-fn ((env-lookup env) fn)) args) env)  ; Look up and apply
     (if (pair? fn)
        ; fn is a pair - check if it's a closure
        (if (equal? (car fn) :closure)
           ; Closure: extract params, body-exprs, closure-env
           ; fn = (:closure . (params . (body-exprs . closure-env)))
           (if (pair? (cdr fn))
              ; Get params and rest
              (if (pair? (cdr (cdr fn)))
                 ; Use eval-body for body-exprs (supports sequences with define)
                 ((eval-body (car (cdr (cdr fn))))    ; body-exprs
                  (((bind-params (car (cdr fn)))     ; params
                    args)
                   (cdr (cdr (cdr fn)))))              ; closure-env
                 (error :invalid-closure-structure fn))
              (error :invalid-closure-structure fn))
           (error :not-a-closure fn))
        ; fn is not a symbol or pair - must be a primitive
        (apply-primitive fn args)))))))

;; Evaluate list of expressions
(define eval-list-args (lambda (exprs) (lambda (env)
  (if (null? exprs)
     nil
     (cons ((eval (car exprs)) env)
         ((eval-list-args (cdr exprs)) env))))))

;; Evaluate a list expression (function application)
(define eval-list (lambda (expr) (lambda (env)
  ; Check for special forms first
  (if (special-form? expr)
     ; Handle special forms
     (if (equal? (car expr) (quote lambda))
        ; Lambda: (lambda (params...) body-exprs...)
        (if (pair? (cdr expr))
           (if (pair? (cdr (cdr expr)))
              (((eval-lambda (car (cdr expr)))   ; params list
                (cdr (cdr expr)))                ; body-exprs (full list for sequences)
               env)
              (error :lambda-missing-body expr))
           (error :lambda-missing-params expr))
        (if (equal? (car expr) (quote if))
           ; Conditional: (if cond then else)
           (if (pair? (cdr expr))
              (if (pair? (cdr (cdr expr)))
                 (if (pair? (cdr (cdr (cdr expr))))
                    ((((eval-if (car (cdr expr)))       ; cond
                       (car (cdr (cdr expr))))            ; then
                      (car (cdr (cdr (cdr expr)))))         ; else
                     env)
                    (error :if-missing-else expr))
                 (error :if-missing-then expr))
              (error :if-missing-condition expr))
           ; Quote: (quote expr) - return expr unevaluated
           (if (equal? (car expr) (quote quote))
              (if (pair? (cdr expr))
                 (car (cdr expr))            ; Return quoted expression
                 (error :quote-missing-expr expr))
              ; Define: (define name value) - evaluate value and return it
              ; Note: Environment extension only persists in body context (eval-body)
              (if (equal? (car expr) (quote define))
                 (if (pair? (cdr expr))
                    (if (pair? (cdr (cdr expr)))
                       ((eval (car (cdr (cdr expr)))) env)  ; Evaluate and return value
                       (error :define-missing-value expr))
                    (error :define-missing-name expr))
                 ; Letrec: (source ((name1 val1) (name2 val2) ...) body)
                 ; Uses self-application transformation for recursion
                 (if (equal? (car expr) (quote source))
                    (if (pair? (cdr expr))
                       (if (pair? (cdr (cdr expr)))
                          (((eval-letrec (car (cdr expr)))   ; bindings
                            (car (cdr (cdr expr))))            ; body
                           env)
                          (error :letrec-missing-body expr))
                       (error :letrec-missing-bindings expr))
                    ; Meta-eval: (eval expr) - evaluate expr, then evaluate result
                    (if (equal? (car expr) (quote eval))
                       (if (pair? (cdr expr))
                          ((eval ((eval (car (cdr expr))) env)) env)
                          (error :eval-missing-expr expr))
                       (error :unknown-special-form expr)))))))
     ; Regular function application
     (if (null? expr)
        (error :empty-application)
        ; Evaluate function
        (((apply-fn ((eval (car expr)) env))
          ((eval-list-args (cdr expr)) env))
         env))))))

;; ===================================================================
;; Main Evaluator
;; ===================================================================

;; Main evaluation function
;; expr: Expression to evaluate
;; env: Environment
;; Returns: Evaluated result
(define eval (lambda (expr) (lambda (env)
  (if (pair? expr)
     ((eval-list expr) env)
     ((eval-atom expr) env)))))
