;;;
;;; Environment Module - Version 2 (Simpler approach)
;;; Avoid pattern matching for now, use direct conditionals
;;;

;; Create empty environment
(define env-empty (lambda () nil))

;; Extend environment with new binding
(define env-extend (lambda (env) (lambda (name) (lambda (value)
  (cons (cons name value) env)))))

;; Look up symbol in environment
;; Uses direct recursion with conditionals instead of pattern matching
(define env-lookup (lambda (env) (lambda (key)
  (if (null? env)
     (error :undefined-variable key)
     (if (equal? (car (car env)) key)
        (cdr (car env))           ; Return value
        ((env-lookup (cdr env)) key))))))  ; Recurse on rest

;; Check if symbol is bound in environment
(define env-has? (lambda (env) (lambda (key)
  (if (null? env)
     #f
     (if (equal? (car (car env)) key)
        #t
        ((env-has? (cdr env)) key))))))

;; Create environment from list of bindings
(define env-from-list (lambda (bindings)
  (if (null? bindings)
     (env-empty)
     (((env-extend (env-from-list (cdr bindings)))
       (car (car bindings)))
      (cdr (car bindings))))))
