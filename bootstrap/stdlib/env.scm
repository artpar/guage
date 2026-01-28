; Environment Module for Meta-Circular Evaluator
; Implements De Bruijn index-based environments

; Environment structure:
; - Empty env: ∅
; - Extended env: ⟨value rest-env⟩
;
; Lookup by De Bruijn index:
; - Index 0 = most recent binding (car of env)
; - Index 1 = next binding (car of cdr of env)
; - Index n = nth binding from the front

; ≈⊙env∅ :: () → Env
; Create empty environment
(≔ ≈⊙env∅ (λ ∅))

; ≈⊙env⊕ :: α → Env → Env
; Extend environment with new binding
; (env-extend value env) → ⟨value env⟩
(≔ ≈⊙env⊕ (λ (λ (⟨⟩ 1 0))))

; ≈⊙env→ :: Env → ℕ → α | ⚠
; Lookup value by De Bruijn index
; Index 0 = first binding, 1 = second, etc.
(≔ ≈⊙env→ (λ (λ
  (? (∅? 1)
     (⚠ :unbound-index 0)
     (? (≡ 0 #0)
        (◁ 1)
        ((≈⊙env→ (▷ 1)) (⊖ 0 #1)))))))

; Example usage:
; (≔ env (≈⊙env∅))                    ; ∅
; (≔ env (≈⊙env⊕ #42 env))            ; ⟨#42 ∅⟩
; (≔ env (≈⊙env⊕ #99 env))            ; ⟨#99 ⟨#42 ∅⟩⟩
; ((≈⊙env→ env) #0)                   ; #99 (most recent)
; ((≈⊙env→ env) #1)                   ; #42 (previous)
; ((≈⊙env→ env) #2)                   ; ⚠:unbound-index
