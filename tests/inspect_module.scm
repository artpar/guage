; Quick inspection of module registry functionality

; Load the test module
(⋘ "../../tests/test_module_math.scm")

; List all modules
(⌂⊚)

; Find where 'square' is defined
(⌂⊚ :square)

; List all symbols from the test module
(⌂⊚ "../../tests/test_module_math.scm")
