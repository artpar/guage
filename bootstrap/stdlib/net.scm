; Guage Standard Library: Networking
; Day 126 — High-level convenience wrappers over socket + ring primitives
;
; Symbols: consume (multimap — network arrow)
;          ⊸⊚ (ring operations)

; ============================================================================
; TCP Client Helpers
; ============================================================================

; ⌂: Connect to a TCP host:port, returning a connected socket fd
; ∈: string -> ℕ -> ℕ|⚠
; Ex: (⊸:tcp-connect "127.0.0.1" #8080) -> fd
(define ⊸:tcp-connect (lambda (host port)
  (define addr (net-addr host port))
  (if (error? addr) addr
     (define fd (net-socket :inet :stream #0))
     (if (error? fd) fd
        (define rc (net-connect fd addr))
        (if (error? rc)
           rc    ; connect failed, return error
           fd))))) ; success, return fd

; ============================================================================
; TCP Server Helpers
; ============================================================================

; ⌂: Create a TCP listener on host:port with SO_REUSEADDR
; ∈: string -> ℕ -> ℕ|⚠
; Ex: (⊸:tcp-listen "0.0.0.0" #8080) -> fd
(define ⊸:tcp-listen (lambda (host port)
  (define fd (net-socket :inet :stream #0))
  (if (error? fd) fd
     (net-setsockopt fd :reuse-addr #1)
     (define addr (net-addr host port))
     (if (error? addr) addr
        (define rc (net-bind-addr fd addr))
        (if (error? rc) rc
           (define rc2 (net-listen fd #128))
           (if (error? rc2) rc2
              fd))))))

; ⌂: Accept a connection, returning ⟨client-fd addr⟩
; ∈: ℕ -> ⟨ℕ ◈⟩|⚠
(define ⊸:tcp-accept (lambda (listen-fd)
  (net-accept listen-fd)))

; ============================================================================
; Ring-Based Echo Server (Example)
; ============================================================================

; ⌂: Single-iteration ring echo: accept one client, echo one message, close
; ∈: ⊸⊚ -> ℕ -> 𝔹|⚠
; Ex: (⊸:ring-echo-once ring listen-fd)
(define ⊸:ring-echo-once (lambda (ring listen-fd)
  ; Accept synchronously
  (define client-pair (net-accept listen-fd))
  (if (error? client-pair) client-pair
     (define cfd (car client-pair))
     ; Set nonblock
     (net-setsockopt cfd :nonblock #1)
     ; Receive synchronously
     (define data (net-recv cfd #4096 #0))
     (if (error? data) data
        ; Echo back via ring
        (ring-send ring cfd data #1)
        (ring-submit ring)
        (ring-complete ring #1 #1000)
        (net-close cfd)
        #t))))

; ============================================================================
; Convenience: Send/Recv Strings
; ============================================================================

; ⌂: Send a string on a socket
; ∈: ℕ -> string -> ℕ|⚠
(define ⊸:send-string (lambda (fd str)
  (net-send fd (string->bytebuf str) #0)))

; ⌂: Receive up to N bytes as a string
; ∈: ℕ -> ℕ -> ≈|⚠
(define ⊸:recv-string (lambda (fd maxlen)
  (define buf (net-recv fd maxlen #0))
  (if (error? buf) buf
     (bytebuf->string buf))))
