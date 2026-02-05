; Guage Standard Library: Raylib GUI Bindings
; Pure runtime FFI bindings — no C primitives needed
;
; Prefix: rl- (raylib)
; Requires: brew install raylib (or equivalent)
;
; Usage:
;   (load "bootstrap/stdlib/raylib.scm")
;   (rl-init #800 #450 "My App")
;   (rl-set-fps #60)
;   (rl-loop (lambda ()
;     (rl-draw (lambda ()
;       (rl-clear rl-white)
;       (rl-text "Hello!" #10 #10 #20 rl-black)))))

; ============================================================================
; Library Loading
; ============================================================================

(define rl-lib (ffi-dlopen "/opt/homebrew/lib/libraylib.dylib"))

; ============================================================================
; Colors (RGBA as 32-bit unsigned integers)
; ============================================================================

(define rl-white     #0xFFFFFFFF)
(define rl-black     #0x000000FF)
(define rl-red       #0xFF0000FF)
(define rl-green     #0x00FF00FF)
(define rl-blue      #0x0000FFFF)
(define rl-yellow    #0xFFFF00FF)
(define rl-orange    #0xFFA500FF)
(define rl-pink      #0xFF69B4FF)
(define rl-purple    #0x800080FF)
(define rl-gray      #0x808080FF)
(define rl-lightgray #0xC8C8C8FF)
(define rl-darkgray  #0x505050FF)
(define rl-raywhite  #0xF5F5F5FF)

; ============================================================================
; Window Management
; ============================================================================

(define rl-init-window   (ffi-bind rl-lib "InitWindow"        (cons :int (cons :int (cons :char* nil))) :void))
(define rl-close-window  (ffi-bind rl-lib "CloseWindow"       (quote ()) :void))
(define rl-should-close  (ffi-bind rl-lib "WindowShouldClose" (quote ()) :bool))
(define rl-set-fps       (ffi-bind rl-lib "SetTargetFPS"      (cons :int nil) :void))
(define rl-get-width     (ffi-bind rl-lib "GetScreenWidth"    (quote ()) :int))
(define rl-get-height    (ffi-bind rl-lib "GetScreenHeight"   (quote ()) :int))
(define rl-set-title     (ffi-bind rl-lib "SetWindowTitle"    (cons :char* nil) :void))
(define rl-toggle-full   (ffi-bind rl-lib "ToggleFullscreen"  (quote ()) :void))
(define rl-is-resized    (ffi-bind rl-lib "IsWindowResized"   (quote ()) :bool))
(define rl-is-focused    (ffi-bind rl-lib "IsWindowFocused"   (quote ()) :bool))
(define rl-is-ready      (ffi-bind rl-lib "IsWindowReady"     (quote ()) :bool))

; ============================================================================
; Drawing Context
; ============================================================================

(define rl-begin-draw    (ffi-bind rl-lib "BeginDrawing"      (quote ()) :void))
(define rl-end-draw      (ffi-bind rl-lib "EndDrawing"        (quote ()) :void))
(define rl-clear         (ffi-bind rl-lib "ClearBackground"   (cons :uint nil) :void))

; ============================================================================
; Shapes — Integer Coordinates (simple, no structs)
; ============================================================================

(define rl-rect          (ffi-bind rl-lib "DrawRectangle"       (cons :int (cons :int (cons :int (cons :int (cons :uint nil))))) :void))
(define rl-rect-lines    (ffi-bind rl-lib "DrawRectangleLines"  (cons :int (cons :int (cons :int (cons :int (cons :uint nil))))) :void))
(define rl-circle        (ffi-bind rl-lib "DrawCircle"          (cons :int (cons :int (cons :float (cons :uint nil)))) :void))
(define rl-circle-lines  (ffi-bind rl-lib "DrawCircleLines"     (cons :int (cons :int (cons :float (cons :uint nil)))) :void))
(define rl-line          (ffi-bind rl-lib "DrawLine"            (cons :int (cons :int (cons :int (cons :int (cons :uint nil))))) :void))
(define rl-pixel         (ffi-bind rl-lib "DrawPixel"           (cons :int (cons :int (cons :uint nil))) :void))
(define rl-triangle      (ffi-bind rl-lib "DrawTriangle"        (cons :int (cons :int (cons :int (cons :int (cons :int (cons :int (cons :uint nil))))))) :void))

; ============================================================================
; Text
; ============================================================================

(define rl-text          (ffi-bind rl-lib "DrawText"            (cons :char* (cons :int (cons :int (cons :int (cons :uint nil))))) :void))
(define rl-measure-text  (ffi-bind rl-lib "MeasureText"         (cons :char* (cons :int nil)) :int))

; ============================================================================
; Input — Keyboard
; ============================================================================

(define rl-key-pressed   (ffi-bind rl-lib "IsKeyPressed"        (cons :int nil) :bool))
(define rl-key-down      (ffi-bind rl-lib "IsKeyDown"           (cons :int nil) :bool))
(define rl-key-released  (ffi-bind rl-lib "IsKeyReleased"       (cons :int nil) :bool))
(define rl-get-key       (ffi-bind rl-lib "GetKeyPressed"       (quote ()) :int))
(define rl-get-char      (ffi-bind rl-lib "GetCharPressed"      (quote ()) :int))

; Key codes
(define rl-key-space     #32)
(define rl-key-escape    #256)
(define rl-key-enter     #257)
(define rl-key-tab       #258)
(define rl-key-backspace #259)
(define rl-key-right     #262)
(define rl-key-left      #263)
(define rl-key-down      #264)
(define rl-key-up        #265)
(define rl-key-a         #65)
(define rl-key-d         #68)
(define rl-key-s         #83)
(define rl-key-w         #87)

; ============================================================================
; Input — Mouse
; ============================================================================

(define rl-mouse-x       (ffi-bind rl-lib "GetMouseX"           (quote ()) :int))
(define rl-mouse-y       (ffi-bind rl-lib "GetMouseY"           (quote ()) :int))
(define rl-mouse-btn     (ffi-bind rl-lib "IsMouseButtonPressed" (cons :int nil) :bool))
(define rl-mouse-down    (ffi-bind rl-lib "IsMouseButtonDown"    (cons :int nil) :bool))
(define rl-mouse-wheel   (ffi-bind rl-lib "GetMouseWheelMove"    (quote ()) :float))

; Mouse buttons
(define rl-mouse-left    #0)
(define rl-mouse-right   #1)
(define rl-mouse-middle  #2)

; ============================================================================
; Time
; ============================================================================

(define rl-get-time      (ffi-bind rl-lib "GetTime"             (quote ()) :double))
(define rl-get-fps       (ffi-bind rl-lib "GetFPS"              (quote ()) :int))
(define rl-get-delta     (ffi-bind rl-lib "GetFrameTime"        (quote ()) :float))

; ============================================================================
; Helper Functions
; ============================================================================

; Initialize window with width, height, title
(define rl-init (lambda (w h title)
  (rl-init-window w h title)))

; Main loop helper — calls fn each frame until window should close
(define rl-loop (lambda (frame-fn)
  (if (rl-should-close)
      nil
      (begin
        (frame-fn)
        (rl-loop frame-fn)))))

; Drawing context wrapper — handles begin/end automatically
(define rl-draw (lambda (draw-fn)
  (begin
    (rl-begin-draw)
    (draw-fn)
    (rl-end-draw))))

; Combined loop + draw helper for simple apps
(define rl-run (lambda (draw-fn)
  (rl-loop (lambda ()
    (rl-draw draw-fn)))))

; ============================================================================
; Color Construction
; ============================================================================

; Construct RGBA color from components (0-255 each)
(define rl-rgb (lambda (r g b)
  (+ (* r #16777216) (+ (* g #65536) (+ (* b #256) #255)))))

(define rl-rgba (lambda (r g b a)
  (+ (* r #16777216) (+ (* g #65536) (+ (* b #256) a)))))

; Fade a color (multiply alpha by factor 0.0-1.0)
(define rl-fade (lambda (color factor)
  (define a (% color #256))
  (define new-a (->integer (* a factor)))
  (+ (- color a) new-a)))

; ============================================================================
; Additional Colors
; ============================================================================

(define rl-maroon    #0x800000FF)
(define rl-gold      #0xFFD700FF)
(define rl-lime      #0x00FF00FF)
(define rl-skyblue   #0x87CEEBFF)
(define rl-violet    #0xEE82EEFF)
(define rl-beige     #0xF5F5DCFF)
(define rl-brown     #0xA52A2AFF)
(define rl-darkgreen #0x006400FF)
(define rl-darkblue  #0x00008BFF)
(define rl-darkpurple #0x301934FF)
(define rl-magenta   #0xFF00FFFF)

; ============================================================================
; Additional Key Codes
; ============================================================================

(define rl-key-l     #76)
(define rl-key-r     #82)

; ============================================================================
; Additional Drawing
; ============================================================================

(define rl-draw-fps  (ffi-bind rl-lib "DrawFPS"  (cons :int (cons :int nil)) :void))

; ============================================================================
; Text Utilities
; ============================================================================

; Draw text centered at x position
(define rl-text-centered (lambda (text cx y size color)
  (define w (rl-measure-text text size))
  (rl-text text (- cx (/ w #2)) y size color)))

; ============================================================================
; Loop with State
; ============================================================================

; Main loop with state — calls (fn state) each frame, uses return as next state
(define rl-loop-state (lambda (initial-state frame-fn)
  (define loop-inner (lambda (state)
    (if (rl-should-close)
        state
        (loop-inner (frame-fn state)))))
  (loop-inner initial-state)))

; ============================================================================
; Example Usage
; ============================================================================
;
; (load "bootstrap/stdlib/raylib.scm")
;
; (rl-init #800 #450 "Hello Raylib")
; (rl-set-fps #60)
;
; (rl-run (lambda ()
;   (rl-clear rl-raywhite)
;   (rl-text "Hello from Guage!" #280 #200 #30 rl-black)
;   (rl-circle #400 #300 #50 rl-red)))
;
; (rl-close-window)
