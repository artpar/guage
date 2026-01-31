; Guage Standard Library: Networking
; Day 126 — High-level convenience wrappers over socket + ring primitives
;
; Symbols: ⊸ (multimap — network arrow)
;          ⊸⊚ (ring operations)

; ============================================================================
; TCP Client Helpers
; ============================================================================

; ⌂: Connect to a TCP host:port, returning a connected socket fd
; ∈: ≈ → ℕ → ℕ|⚠
; Ex: (⊸:tcp-connect "127.0.0.1" #8080) → fd
(≔ ⊸:tcp-connect (λ (host port)
  (≔ addr (⊸⊙ host port))
  (? (⚠? addr) addr
     (≔ fd (⊸⊕ :inet :stream #0))
     (? (⚠? fd) fd
        (≔ rc (⊸→⊕ fd addr))
        (? (⚠? rc)
           rc    ; connect failed, return error
           fd))))) ; success, return fd

; ============================================================================
; TCP Server Helpers
; ============================================================================

; ⌂: Create a TCP listener on host:port with SO_REUSEADDR
; ∈: ≈ → ℕ → ℕ|⚠
; Ex: (⊸:tcp-listen "0.0.0.0" #8080) → fd
(≔ ⊸:tcp-listen (λ (host port)
  (≔ fd (⊸⊕ :inet :stream #0))
  (? (⚠? fd) fd
     (⊸≔ fd :reuse-addr #1)
     (≔ addr (⊸⊙ host port))
     (? (⚠? addr) addr
        (≔ rc (⊸←≔ fd addr))
        (? (⚠? rc) rc
           (≔ rc2 (⊸←⊕ fd #128))
           (? (⚠? rc2) rc2
              fd))))))

; ⌂: Accept a connection, returning ⟨client-fd addr⟩
; ∈: ℕ → ⟨ℕ ◈⟩|⚠
(≔ ⊸:tcp-accept (λ (listen-fd)
  (⊸← listen-fd)))

; ============================================================================
; Ring-Based Echo Server (Example)
; ============================================================================

; ⌂: Single-iteration ring echo: accept one client, echo one message, close
; ∈: ⊸⊚ → ℕ → 𝔹|⚠
; Ex: (⊸:ring-echo-once ring listen-fd)
(≔ ⊸:ring-echo-once (λ (ring listen-fd)
  ; Accept synchronously
  (≔ client-pair (⊸← listen-fd))
  (? (⚠? client-pair) client-pair
     (≔ cfd (◁ client-pair))
     ; Set nonblock
     (⊸≔ cfd :nonblock #1)
     ; Receive synchronously
     (≔ data (⊸←◈ cfd #4096 #0))
     (? (⚠? data) data
        ; Echo back via ring
        (⊸⊚→ ring cfd data #1)
        (⊸⊚! ring)
        (⊸⊚⊲ ring #1 #1000)
        (⊸× cfd)
        #t))))

; ============================================================================
; Convenience: Send/Recv Strings
; ============================================================================

; ⌂: Send a string on a socket
; ∈: ℕ → ≈ → ℕ|⚠
(≔ ⊸:send-string (λ (fd str)
  (⊸→ fd (≈◈ str) #0)))

; ⌂: Receive up to N bytes as a string
; ∈: ℕ → ℕ → ≈|⚠
(≔ ⊸:recv-string (λ (fd maxlen)
  (≔ buf (⊸←◈ fd maxlen #0))
  (? (⚠? buf) buf
     (◈≈ buf))))
