; Guage Standard Library: Subprocess Management
; Day 147 — FFI-based wrappers over libc popen/pclose/system
;
; Symbols: ⌁ (FFI)
;          ⊙⌂⊛ (capture), ⊙⌂⊛! (run), ⊙⌂⊛⊙ (capture-status)

; ============================================================================
; FFI Bindings to libc
; ============================================================================

; ⌂: Load libc
(define _libc (ffi-dlopen "libc"))

; ⌂: popen(command, mode) -> FILE*
(define _popen (ffi-bind _libc "popen" (cons :char* (cons :char* nil)) :void*))

; ⌂: pclose(FILE*) -> int (exit status)
(define _pclose (ffi-bind _libc "pclose" (cons :void* nil) :int))

; ⌂: fgets(buf, size, stream) -> char* or NULL
(define _fgets (ffi-bind _libc "fgets" (cons :void* (cons :int (cons :void* nil))) :void*))

; ⌂: system(command) -> int (exit status)
(define _system (ffi-bind _libc "system" (cons :char* nil) :int))

; ⌂: calloc(nmemb, size) -> ptr (zero-initialized)
(define _calloc (ffi-bind _libc "calloc" (cons :size_t (cons :size_t nil)) :void*))

; ⌂: free(ptr)
(define _free (ffi-bind _libc "free" (cons :void* nil) :void))

; ⌂: memset(ptr, val, size) -> ptr
(define _memset (ffi-bind _libc "memset" (cons :void* (cons :int (cons :size_t nil))) :void*))

; ============================================================================
; High-Level: Capture Command Output
; ============================================================================

; ⌂: Run command, capture stdout as string
; ∈: string -> ≈|⚠
; Ex: (process-capture "echo hello") -> "hello\n"
; Ex: (process-capture "ls /nonexistent") -> error
(define process-capture (lambda (cmd)
  (begin
    (define fp (_popen cmd "r"))
    (if (ffi-null? fp)
       (error :popen-failed cmd)
       (begin
         ; Allocate zero-initialized read buffer (4096 bytes)
         (define buf (_calloc #4096 #1))
         (define result (box ""))
         ; Read loop using fgets (null-terminates automatically)
         (define read-loop (lambda ()
           (begin
             (_memset buf #0 #4096)
             (define r (_fgets buf #4096 fp))
             (if (ffi-null? r)
                nil
                (begin
                  (define chunk (ffi-read-cstr buf))
                  (box-set! result (string-append (unbox result) chunk))
                  (read-loop))))))
         (read-loop)
         (_free buf)
         (define status (_pclose fp))
         (define out (unbox result))
         (if (equal? status #0)
            out
            (error :process-exit-status (cons status out))))))))

; ============================================================================
; High-Level: Run Command (No Capture)
; ============================================================================

; ⌂: Run command via system(), return exit status
; ∈: string -> ℕ|⚠
; Ex: (process-run "true") -> #0
; Ex: (process-run "false") -> #1
(define process-run (lambda (cmd)
  (_system cmd)))

; ============================================================================
; High-Level: Capture with Exit Status
; ============================================================================

; ⌂: Run command, return ⟨exit-status output⟩
; ∈: string -> ⟨ℕ ≈⟩|⚠
; Ex: (process-capture-status "echo hi") -> ⟨#0 "hi\n"⟩
(define process-capture-status (lambda (cmd)
  (begin
    (define fp (_popen cmd "r"))
    (if (ffi-null? fp)
       (error :popen-failed cmd)
       (begin
         (define buf (_calloc #4096 #1))
         (define result (box ""))
         (define read-loop (lambda ()
           (begin
             (_memset buf #0 #4096)
             (define r (_fgets buf #4096 fp))
             (if (ffi-null? r)
                nil
                (begin
                  (define chunk (ffi-read-cstr buf))
                  (box-set! result (string-append (unbox result) chunk))
                  (read-loop))))))
         (read-loop)
         (_free buf)
         (define status (_pclose fp))
         (cons status (unbox result)))))))

; ============================================================================
; Symbolic Aliases
; ============================================================================

; ⌂: Symbolic names following Guage conventions
(define ⊙⌂⊛ process-capture)         ; capture command output
(define ⊙⌂⊛! process-run)            ; run command (fire-and-forget)
(define ⊙⌂⊛⊙ process-capture-status) ; capture with status

; ============================================================================
; Module complete - Subprocess convenience via FFI
; ============================================================================
