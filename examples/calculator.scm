; ============================================================================
; Calculator — 4-function calculator with clickable button grid
; ============================================================================

(load "bootstrap/stdlib/raylib.scm")

; --- State ---
(define display     (box "0"))
(define accumulator (box #0))
(define op          (box :none))
(define fresh       (box #t))

; --- Helpers ---
(define calc-apply (lambda (acc cur oper)
  (if (equal? oper :add) (+ acc cur)
  (if (equal? oper :sub) (- acc cur)
  (if (equal? oper :mul) (* acc cur)
  (if (equal? oper :div) (if (equal? cur #0) #0 (/ acc cur))
     cur))))))

; --- Button actions ---
(define press-digit (lambda (d)
  (if (unbox fresh)
     (begin (box-set! display (string d)) (box-set! fresh #f))
     (begin (define cur (unbox display))
         (box-set! display (if (equal? cur "0") (string d) (string-append cur (string d))))))))

(define press-dot (lambda ()
  (if (unbox fresh)
     (begin (box-set! display "0.") (box-set! fresh #f))
     (box-set! display (string-append (unbox display) ".")))))

(define press-op (lambda (new-op)
  (begin (define cur (string->number (unbox display)))
      (define prev-op (unbox op))
      (if (equal? prev-op :none) (box-set! accumulator cur) (box-set! accumulator (calc-apply (unbox accumulator) cur prev-op)))
      (box-set! op new-op)
      (box-set! display (string (unbox accumulator)))
      (box-set! fresh #t))))

(define press-eq (lambda ()
  (begin (define cur (string->number (unbox display)))
      (define prev-op (unbox op))
      (define result (if (equal? prev-op :none) cur (calc-apply (unbox accumulator) cur prev-op)))
      (box-set! accumulator #0)
      (box-set! op :none)
      (box-set! display (string result))
      (box-set! fresh #t))))

(define press-clear (lambda ()
  (begin (box-set! display "0") (box-set! accumulator #0) (box-set! op :none) (box-set! fresh #t))))

(define press-negate (lambda ()
  (begin (define cur (string->number (unbox display))) (box-set! display (string (- #0 cur))))))

(define press-pct (lambda ()
  (begin (define cur (string->number (unbox display))) (box-set! display (string (/ cur #100))))))

; --- Grid row handlers ---
(define row0 (lambda (c) (if (equal? c #0) (press-clear) (if (equal? c #1) (press-negate) (if (equal? c #2) (press-pct) (press-op :div))))))
(define row1 (lambda (c) (if (equal? c #0) (press-digit #7) (if (equal? c #1) (press-digit #8) (if (equal? c #2) (press-digit #9) (press-op :mul))))))
(define row2 (lambda (c) (if (equal? c #0) (press-digit #4) (if (equal? c #1) (press-digit #5) (if (equal? c #2) (press-digit #6) (press-op :sub))))))
(define row3 (lambda (c) (if (equal? c #0) (press-digit #1) (if (equal? c #1) (press-digit #2) (if (equal? c #2) (press-digit #3) (press-op :add))))))
(define row4 (lambda (c) (if (<= c #1) (press-digit #0) (if (equal? c #2) (press-dot) (press-eq)))))

(define grid-click (lambda (r c)
  (if (equal? r #0) (row0 c) (if (equal? r #1) (row1 c) (if (equal? r #2) (row2 c) (if (equal? r #3) (row3 c) (if (equal? r #4) (row4 c) nil)))))))

; --- Drawing helpers ---
(define draw-btn (lambda (label bx by bw bh color)
  (begin (rl-rect bx by bw bh color)
      (rl-rect-lines bx by bw bh rl-black)
      (define tw (rl-measure-text label #28))
      (rl-text label (+ bx (/ (- bw tw) #2)) (+ by #20) #28 rl-white))))

; --- Colors ---
(define col-bg      (rl-rgb #30 #30 #30))
(define col-display (rl-rgb #20 #20 #20))
(define col-gray    (rl-rgb #100 #100 #100))
(define col-dark    (rl-rgb #60 #60 #60))
(define col-orange  (rl-rgb #255 #149 #0))

; --- Input ---
(define handle-click (lambda ()
  (begin (define mx (rl-mouse-x))
      (define my (rl-mouse-y))
      (if (and (>= mx #20) (>= my #120))
         (begin (define c (quotient (- mx #20) #190))
             (define r (quotient (- my #120) #75))
             (if (and (<= c #3) (<= r #4))
                (grid-click r c)
                nil))
         nil))))

; --- Main ---
(rl-init #800 #500 "Calculator")
(rl-set-fps #60)
(rl-loop (lambda ()
  (begin (if (rl-mouse-btn rl-mouse-left) (handle-click) nil)
      (rl-draw (lambda ()
        (begin (rl-clear col-bg)
            ; Display
            (rl-rect #20 #20 #760 #80 col-display)
            (define txt (unbox display))
            (define tw (rl-measure-text txt #48))
            (rl-text txt (- #770 (+ tw #10)) #35 #48 rl-white)
            ; Row 0
            (draw-btn "C"  #20  #120 #185 #70 col-gray)
            (draw-btn "+/-" #210 #120 #185 #70 col-gray)
            (draw-btn "%"  #400 #120 #185 #70 col-gray)
            (draw-btn "/"  #590 #120 #185 #70 col-orange)
            ; Row 1
            (draw-btn "7"  #20  #195 #185 #70 col-dark)
            (draw-btn "8"  #210 #195 #185 #70 col-dark)
            (draw-btn "9"  #400 #195 #185 #70 col-dark)
            (draw-btn "x"  #590 #195 #185 #70 col-orange)
            ; Row 2
            (draw-btn "4"  #20  #270 #185 #70 col-dark)
            (draw-btn "5"  #210 #270 #185 #70 col-dark)
            (draw-btn "6"  #400 #270 #185 #70 col-dark)
            (draw-btn "-"  #590 #270 #185 #70 col-orange)
            ; Row 3
            (draw-btn "1"  #20  #345 #185 #70 col-dark)
            (draw-btn "2"  #210 #345 #185 #70 col-dark)
            (draw-btn "3"  #400 #345 #185 #70 col-dark)
            (draw-btn "+"  #590 #345 #185 #70 col-orange)
            ; Row 4
            (draw-btn "0"  #20  #420 #375 #70 col-dark)
            (draw-btn "."  #400 #420 #185 #70 col-dark)
            (draw-btn "="  #590 #420 #185 #70 col-orange)
            (rl-text "ESC to exit" #10 #480 #12 rl-darkgray)))))))
(rl-close-window)
