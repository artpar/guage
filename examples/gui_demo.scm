; Simple Raylib GUI Demo
; Run: ./bootstrap/guage examples/gui_demo.scm

(load "bootstrap/stdlib/raylib.scm")

; Initialize window
(rl-init #800 #450 "Guage GUI Demo")
(rl-set-fps #60)

; Main loop
(rl-run (lambda ()
  (rl-clear rl-raywhite)
  (rl-text "Hello from Guage!" #280 #180 #30 rl-darkgray)
  (rl-text "Press ESC to exit" #300 #220 #20 rl-gray)
  (rl-circle #400 #300 #50 rl-red)
  (rl-rect #350 #350 #100 #30 rl-blue)))

; Cleanup
(rl-close-window)
