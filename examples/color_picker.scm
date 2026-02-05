; ============================================================================
; Color Picker — RGB sliders with preview swatch
; ============================================================================

(load "bootstrap/stdlib/raylib.scm")

; ============================================================================
; State
; ============================================================================

(define r-val (box #128))
(define g-val (box #128))
(define b-val (box #128))

; ============================================================================
; Layout constants
; ============================================================================

(define win-w #600)
(define win-h #400)
(define slider-x #120)
(define slider-w #400)
(define slider-h #30)
(define slider-gap #60)
(define slider-y0 #40)
(define preview-x #120)
(define preview-y #240)
(define preview-w #400)
(define preview-h #120)

; ============================================================================
; Colors
; ============================================================================

(define col-bg     (rl-rgb #240 #240 #240))
(define col-track  (rl-rgb #180 #180 #180))
(define col-label  rl-black)

; ============================================================================
; Helpers
; ============================================================================

; Clamp value to 0-255
(define clamp255 (lambda (v)
  (if (< v #0) #0
  (if (> v #255) #255
     v))))

; Map mouse x position to 0-255 range within slider
(define x-to-val (lambda (mx)
  (clamp255 (->integer (/ (* (- mx slider-x) #255) slider-w)))))

; Map value 0-255 to pixel width within slider
(define val-to-w (lambda (v)
  (->integer (/ (* v slider-w) #255))))

; Format a single byte as 2-digit hex
(define hex-digit (lambda (n)
  (if (< n #10) (string n)
  (if (equal? n #10) "A"
  (if (equal? n #11) "B"
  (if (equal? n #12) "C"
  (if (equal? n #13) "D"
  (if (equal? n #14) "E"
     "F"))))))))

(define byte-hex (lambda (v)
  (string-append (hex-digit (quotient v #16)) (hex-digit (% v #16)))))

; Format full hex color string
(define fmt-hex (lambda (r g b)
  (string-append "#" (string-append (byte-hex r) (string-append (byte-hex g) (byte-hex b))))))

; Hit test for slider track
(define in-slider? (lambda (my sy)
  (and (>= my sy) (< my (+ sy slider-h)))))

; ============================================================================
; Drawing
; ============================================================================

; Draw one slider — label, track, filled portion, value text
(define draw-slider (lambda (label sy val fill-color)
  (begin ; Label
      (rl-text label #20 (+ sy #5) #20 col-label)
      ; Track background
      (rl-rect slider-x sy slider-w slider-h col-track)
      ; Filled portion
      (rl-rect slider-x sy (val-to-w val) slider-h fill-color)
      ; Outline
      (rl-rect-lines slider-x sy slider-w slider-h rl-black)
      ; Value text
      (rl-text (string val) (+ slider-x (+ slider-w #15)) (+ sy #5) #20 col-label))))

; Draw the preview swatch and info text
(define draw-preview (lambda ()
  (define r (unbox r-val))
  (define g (unbox g-val))
  (define b (unbox b-val))
  (define color (rl-rgb r g b))
  (begin ; Preview rectangle
      (rl-rect preview-x preview-y preview-w preview-h color)
      (rl-rect-lines preview-x preview-y preview-w preview-h rl-black)
      ; RGB text
      (define rgb-text (string-append "RGB(" (string-append (string r) (string-append ", " (string-append (string g) (string-append ", " (string-append (string b) ")")))))))
      (rl-text rgb-text preview-x (+ preview-y (+ preview-h #10)) #18 col-label)
      ; Hex text
      (define hex-text (fmt-hex r g b))
      (rl-text hex-text (+ preview-x #250) (+ preview-y (+ preview-h #10)) #18 col-label))))

; ============================================================================
; Input — drag sliders
; ============================================================================

(define handle-input (lambda ()
  (if (rl-mouse-down rl-mouse-left)
     (begin (define mx (rl-mouse-x))
         (define my (rl-mouse-y))
         ; Check if within slider x range
         (if (and (>= mx slider-x) (<= mx (+ slider-x slider-w)))
            (begin (define v (x-to-val mx))
                (define sy0 slider-y0)
                (define sy1 (+ slider-y0 slider-gap))
                (define sy2 (+ slider-y0 (* #2 slider-gap)))
                (if (in-slider? my sy0) (box-set! r-val v)
                (if (in-slider? my sy1) (box-set! g-val v)
                (if (in-slider? my sy2) (box-set! b-val v)
                   nil))))
            nil))
     nil)))

; ============================================================================
; Main loop
; ============================================================================

(rl-init win-w win-h "Color Picker")
(rl-set-fps #60)
(rl-loop (lambda ()
  (begin (handle-input)
      (rl-draw (lambda ()
        (begin (rl-clear col-bg)
            (rl-text "Color Picker" #20 #10 #24 col-label)
            (draw-slider "Red"   slider-y0                        (unbox r-val) (rl-rgb (unbox r-val) #0 #0))
            (draw-slider "Green" (+ slider-y0 slider-gap)        (unbox g-val) (rl-rgb #0 (unbox g-val) #0))
            (draw-slider "Blue"  (+ slider-y0 (* #2 slider-gap)) (unbox b-val) (rl-rgb #0 #0 (unbox b-val)))
            (draw-preview)
            (rl-text "Click and drag sliders - ESC to exit" #20 (- win-h #20) #12 rl-darkgray)))))))
(rl-close-window)
