;;;
;;; Simple S-Expression Evaluator
;;; Minimal version - just numbers, symbols, and primitives
;;;

(load "bootstrap/stdlib/eval-env.scm")

;; Evaluate an expression
;; For now: just handle atoms and don't handle special forms
(define eval-simple (lambda (expr) (lambda (env)
  (if (number? expr)
     expr                                   ; Numbers self-evaluate
     (if (boolean? expr)
        expr                                ; Booleans self-evaluate
        (if (symbol? expr)
           ((env-lookup env) expr)          ; Symbol lookup
           (if (null? expr)
              expr                          ; Nil self-evaluates
              ; Lists - function application
              (if (pair? expr)
                 ; For now, just evaluate function and args
                 ; Don't handle special forms yet
                 expr
                 expr))))))))                ; Default: return as-is
