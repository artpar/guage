; Guage Standard Library: Subprocess Management
; Day 147 — FFI-based wrappers over libc popen/pclose/system
;
; Symbols: ⌁ (FFI)
;          ⊙⌂⊛ (capture), ⊙⌂⊛! (run), ⊙⌂⊛⊙ (capture-status)

; ============================================================================
; FFI Bindings to libc
; ============================================================================

; ⌂: Load libc
(≔ _libc (⌁⊳ "libc"))

; ⌂: popen(command, mode) → FILE*
(≔ _popen (⌁→ _libc "popen" (⟨⟩ :char* (⟨⟩ :char* ∅)) :void*))

; ⌂: pclose(FILE*) → int (exit status)
(≔ _pclose (⌁→ _libc "pclose" (⟨⟩ :void* ∅) :int))

; ⌂: fgets(buf, size, stream) → char* or NULL
(≔ _fgets (⌁→ _libc "fgets" (⟨⟩ :void* (⟨⟩ :int (⟨⟩ :void* ∅))) :void*))

; ⌂: system(command) → int (exit status)
(≔ _system (⌁→ _libc "system" (⟨⟩ :char* ∅) :int))

; ⌂: calloc(nmemb, size) → ptr (zero-initialized)
(≔ _calloc (⌁→ _libc "calloc" (⟨⟩ :size_t (⟨⟩ :size_t ∅)) :void*))

; ⌂: free(ptr)
(≔ _free (⌁→ _libc "free" (⟨⟩ :void* ∅) :void))

; ⌂: memset(ptr, val, size) → ptr
(≔ _memset (⌁→ _libc "memset" (⟨⟩ :void* (⟨⟩ :int (⟨⟩ :size_t ∅))) :void*))

; ============================================================================
; High-Level: Capture Command Output
; ============================================================================

; ⌂: Run command, capture stdout as string
; ∈: ≈ → ≈|⚠
; Ex: (process-capture "echo hello") → "hello\n"
; Ex: (process-capture "ls /nonexistent") → ⚠
(≔ process-capture (λ (cmd)
  (⪢
    (≔ fp (_popen cmd "r"))
    (? (⌁∅? fp)
       (⚠ :popen-failed cmd)
       (⪢
         ; Allocate zero-initialized read buffer (4096 bytes)
         (≔ buf (_calloc #4096 #1))
         (≔ result (□ ""))
         ; Read loop using fgets (null-terminates automatically)
         (≔ read-loop (λ ()
           (⪢
             (_memset buf #0 #4096)
             (≔ r (_fgets buf #4096 fp))
             (? (⌁∅? r)
                ∅
                (⪢
                  (≔ chunk (⌁≈→ buf))
                  (□← result (≈⊕ (□→ result) chunk))
                  (read-loop))))))
         (read-loop)
         (_free buf)
         (≔ status (_pclose fp))
         (≔ out (□→ result))
         (? (≡ status #0)
            out
            (⚠ :process-exit-status (⟨⟩ status out))))))))

; ============================================================================
; High-Level: Run Command (No Capture)
; ============================================================================

; ⌂: Run command via system(), return exit status
; ∈: ≈ → ℕ|⚠
; Ex: (process-run "true") → #0
; Ex: (process-run "false") → #1
(≔ process-run (λ (cmd)
  (_system cmd)))

; ============================================================================
; High-Level: Capture with Exit Status
; ============================================================================

; ⌂: Run command, return ⟨exit-status output⟩
; ∈: ≈ → ⟨ℕ ≈⟩|⚠
; Ex: (process-capture-status "echo hi") → ⟨#0 "hi\n"⟩
(≔ process-capture-status (λ (cmd)
  (⪢
    (≔ fp (_popen cmd "r"))
    (? (⌁∅? fp)
       (⚠ :popen-failed cmd)
       (⪢
         (≔ buf (_calloc #4096 #1))
         (≔ result (□ ""))
         (≔ read-loop (λ ()
           (⪢
             (_memset buf #0 #4096)
             (≔ r (_fgets buf #4096 fp))
             (? (⌁∅? r)
                ∅
                (⪢
                  (≔ chunk (⌁≈→ buf))
                  (□← result (≈⊕ (□→ result) chunk))
                  (read-loop))))))
         (read-loop)
         (_free buf)
         (≔ status (_pclose fp))
         (⟨⟩ status (□→ result)))))))

; ============================================================================
; Symbolic Aliases
; ============================================================================

; ⌂: Symbolic names following Guage conventions
(≔ ⊙⌂⊛ process-capture)         ; capture command output
(≔ ⊙⌂⊛! process-run)            ; run command (fire-and-forget)
(≔ ⊙⌂⊛⊙ process-capture-status) ; capture with status

; ============================================================================
; Module complete - Subprocess convenience via FFI
; ============================================================================
