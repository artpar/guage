; Environment Module for Meta-Circular Evaluator
; Implements De Bruijn index-based environments

; Environment structure:
; - Empty env: nil
; - Extended env: ⟨value rest-env⟩
;
; Lookup by De Bruijn index:
; - Index 0 = most recent binding (car of env)
; - Index 1 = next binding (car of cdr of env)
; - Index n = nth binding from the front

; ≈⊙env∅ :: () -> Env
; Create empty environment
(define ≈⊙env∅ (lambda nil))

; ≈⊙env⊕ :: α -> Env -> Env
; Extend environment with new binding
; (env-extend value env) -> ⟨value env⟩
(define ≈⊙env⊕ (lambda (lambda (cons 1 0))))

; ≈⊙env→ :: Env -> ℕ -> α | error
; Lookup value by De Bruijn index
; Index 0 = first binding, 1 = second, etc.
(define ≈⊙env→ (lambda (lambda
  (if (null? 1)
     (error :unbound-index 0)
     (if (equal? 0 #0)
        (car 1)
        ((≈⊙env→ (cdr 1)) (- 0 #1)))))))

; Example usage:
; (define env (≈⊙env∅))                    ; nil
; (define env (≈⊙env⊕ #42 env))            ; ⟨#42 ∅⟩
; (define env (≈⊙env⊕ #99 env))            ; ⟨#99 ⟨#42 ∅⟩⟩
; ((≈⊙env→ env) #0)                   ; #99 (most recent)
; ((≈⊙env→ env) #1)                   ; #42 (previous)
; ((≈⊙env→ env) #2)                   ; ⚠:unbound-index
