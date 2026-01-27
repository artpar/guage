; Debug error structure

; Create error
(≔ err (⚠ :test-error :value))

; Check what it is
(⟲ err)
(⟲ (⚠? err))
(⟲ (⟨⟩? err))
(⟲ (#? err))

; Try to get parts if it's a pair
; (⟲ (◁ err))  ; This might crash
