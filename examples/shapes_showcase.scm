; Shapes Showcase Demo
; Shows various shapes that can be drawn with raylib

(load "bootstrap/stdlib/raylib.scm")

(rl-init #800 #600 "Shapes Showcase")
(rl-set-fps #60)

(rl-run (lambda ()
  (rl-clear rl-raywhite)

  ; Title
  (rl-text "Shapes Showcase" #300 #20 #30 rl-darkgray)

  ; Rectangles
  (rl-text "Rectangles:" #50 #80 #20 rl-gray)
  (rl-rect #50 #110 #100 #60 rl-red)
  (rl-rect #170 #110 #100 #60 rl-green)
  (rl-rect #290 #110 #100 #60 rl-blue)
  (rl-rect-lines #410 #110 #100 #60 rl-purple)

  ; Circles
  (rl-text "Circles:" #50 #200 #20 rl-gray)
  (rl-circle #100 #270 #40 rl-orange)
  (rl-circle #200 #270 #40 rl-pink)
  (rl-circle #300 #270 #40 rl-yellow)
  (rl-circle-lines #400 #270 #40 rl-darkgray)

  ; Lines
  (rl-text "Lines:" #50 #340 #20 rl-gray)
  (rl-line #50 #370 #150 #420 rl-red)
  (rl-line #150 #370 #250 #420 rl-green)
  (rl-line #250 #370 #350 #420 rl-blue)

  ; Pixels
  (rl-text "Pixels (zoom in to see):" #50 #450 #20 rl-gray)
  (rl-pixel #60 #480 rl-red)
  (rl-pixel #70 #480 rl-green)
  (rl-pixel #80 #480 rl-blue)
  (rl-pixel #90 #480 rl-yellow)

  ; Instructions
  (rl-text "Press ESC to exit" #300 #560 #16 rl-gray)))

(rl-close-window)
