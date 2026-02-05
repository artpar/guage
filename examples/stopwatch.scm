; ============================================================================
; Stopwatch — frame-counting timer with laps
; ============================================================================

(load "bootstrap/stdlib/raylib.scm")

; ============================================================================
; State
; ============================================================================

(define frames  (box #0))
(define running (box #f))
(define laps    (box nil))

; ============================================================================
; Layout constants
; ============================================================================

(define win-w #500)
(define win-h #450)

; ============================================================================
; Colors
; ============================================================================

(define col-bg      (rl-rgb #30 #30 #40))
(define col-time    rl-white)
(define col-label   (rl-rgb #180 #180 #200))
(define col-btn     (rl-rgb #60 #60 #80))
(define col-btn-go  (rl-rgb #0 #180 #60))
(define col-btn-stp (rl-rgb #220 #50 #50))
(define col-btn-txt rl-white)
(define col-lap     (rl-rgb #150 #150 #170))

; ============================================================================
; Time formatting
; ============================================================================

; Convert frame count to total centiseconds (at 60 FPS)
(define frames-to-cs (lambda (f)
  (->integer (/ (* f #100) #60))))

; Pad a number to 2 digits with leading zero
(define pad2 (lambda (n)
  (if (< n #10) (string-append "0" (string n)) (string n))))

; Format centiseconds as MM:SS.cc
(define fmt-time (lambda (cs)
  (define total-secs (quotient cs #100))
  (define centis (% cs #100))
  (define mins (quotient total-secs #60))
  (define secs (% total-secs #60))
  (string-append (pad2 mins) (string-append ":" (string-append (pad2 secs) (string-append "." (pad2 centis)))))))

; ============================================================================
; Input handling
; ============================================================================

(define handle-input (lambda ()
  (begin
    ; SPACE — start/stop toggle
    (if (rl-key-pressed rl-key-space)
       (box-set! running (not (unbox running)))
       nil)
    ; L — record lap
    (if (rl-key-pressed rl-key-l)
       (if (unbox running)
          (box-set! laps (cons (unbox frames) (unbox laps)))
          nil)
       nil)
    ; R — reset
    (if (rl-key-pressed rl-key-r)
       (begin (box-set! frames #0)
           (box-set! running #f)
           (box-set! laps nil))
       nil))))

; ============================================================================
; Update — increment frame counter when running
; ============================================================================

(define update (lambda ()
  (if (unbox running)
     (box-set! frames (+ (unbox frames) #1))
     nil)))

; ============================================================================
; Drawing
; ============================================================================

; Draw a button rectangle with centered text
(define draw-button (lambda (label bx by bw bh color)
  (begin (rl-rect bx by bw bh color)
      (rl-rect-lines bx by bw bh rl-black)
      (define tw (rl-measure-text label #20))
      (rl-text label (+ bx (/ (- bw tw) #2)) (+ by #15) #20 col-btn-txt))))

; Draw the large time display
(define draw-time (lambda ()
  (define cs (frames-to-cs (unbox frames)))
  (define text (fmt-time cs))
  (rl-text-centered text (/ win-w #2) #60 #60 col-time)))

; Draw status and control hints
(define draw-controls (lambda ()
  (define status-text (if (unbox running) "RUNNING" "STOPPED"))
  (define status-color (if (unbox running) col-btn-go col-btn-stp))
  (begin (rl-text-centered status-text (/ win-w #2) #130 #24 status-color)
      ; Button hints
      (draw-button "[SPACE] Start/Stop" #30  #170 #140 #50 (if (unbox running) col-btn-stp col-btn-go))
      (draw-button "[L] Lap"            #180 #170 #140 #50 col-btn)
      (draw-button "[R] Reset"          #330 #170 #140 #50 col-btn))))

; Draw lap list (most recent first, up to 8)
(define draw-laps (lambda ()
  (begin (rl-text "Laps:" #30 #240 #18 col-label)
      (define draw-lap-rec (lambda (lap-list idx)
        (if (null? lap-list) nil
        (if (> idx #7) nil
           (begin (define f (car lap-list))
               (define cs (frames-to-cs f))
               (define lap-text (string-append (string-append "#" (string (+ idx #1))) (string-append "  " (fmt-time cs))))
               (rl-text lap-text #50 (+ #265 (* idx #22)) #16 col-lap)
               (draw-lap-rec (cdr lap-list) (+ idx #1)))))))
      (draw-lap-rec (unbox laps) #0))))

; ============================================================================
; Main loop
; ============================================================================

(rl-init win-w win-h "Stopwatch")
(rl-set-fps #60)
(rl-loop (lambda ()
  (begin (handle-input)
      (update)
      (rl-draw (lambda ()
        (begin (rl-clear col-bg)
            (draw-time)
            (draw-controls)
            (draw-laps)
            (rl-text "SPACE=start/stop  L=lap  R=reset  ESC=exit" #30 (- win-h #20) #12 rl-darkgray)))))))
(rl-close-window)
