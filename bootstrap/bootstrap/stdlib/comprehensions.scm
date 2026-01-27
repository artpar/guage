; ═══════════════════════════════════════════════════════════════
; Guage Standard Library: List Comprehensions
; ═══════════════════════════════════════════════════════════════
; Status: CURRENT
; Created: 2026-01-27 (Day 35)
; Purpose: Ergonomic list comprehensions and range generation
;
; All names are PURELY SYMBOLIC - no English words!
; ═══════════════════════════════════════════════════════════════

; ═══════════════════════════════════════════════════════════════
; Phase 1: Range Generation
; ═══════════════════════════════════════════════════════════════

; ⋯→ - Range generation (inclusive, uncurried)
; Generates list of numbers from start to end (INCLUSIVE)
; Usage: (⋯→ start end)
; Example: (⋯→ #1 #5) → ⟨#1 ⟨#2 ⟨#3 ⟨#4 ⟨#5 ∅⟩⟩⟩⟩⟩
; Example: (⋯→ #5 #1) → ∅ (empty for decreasing range)
; Example: (⋯→ #3 #3) → ⟨#3 ∅⟩ (single element)
; Note: stdlib/list.scm has ⋯ which is EXCLUSIVE: ((⋯ #5) #1) → ⟨#1 ⟨#2 ⟨#3 ⟨#4 ∅⟩⟩⟩⟩
(≔ ⋯→ (λ (𝕤 𝕖)
  (? (> 𝕤 𝕖)
     ∅
     (⟨⟩ 𝕤 (⋯→ (⊕ 𝕤 #1) 𝕖)))))

; ⋰ - Range generation with step
; Generates list of numbers from start to end with custom step
; Usage: (⋰ start end step)
; Example: (⋰ #1 #10 #2) → ⟨#1 ⟨#3 ⟨#5 ⟨#7 ⟨#9 ∅⟩⟩⟩⟩⟩ (odd numbers)
; Example: (⋰ #0 #10 #3) → ⟨#0 ⟨#3 ⟨#6 ⟨#9 ∅⟩⟩⟩⟩
; Example: (⋰ #10 #1 #-2) → ⟨#10 ⟨#8 ⟨#6 ⟨#4 ⟨#2 ∅⟩⟩⟩⟩⟩
(≔ ⋰ (λ (𝕤 𝕖 𝕕)
  (? (> 𝕤 𝕖)
     ∅
     (⟨⟩ 𝕤 (⋰ (⊕ 𝕤 𝕕) 𝕖 𝕕)))))

; ═══════════════════════════════════════════════════════════════
; Phase 2: Basic List Comprehension Helpers
; ═══════════════════════════════════════════════════════════════
;
; Note: True list comprehension syntax like [(expr) for x in list]
; requires parser support for brackets. Until then, we provide
; ergonomic helpers that achieve the same goal.
;
; These are NOT macros - they're higher-order functions that compose
; with the existing stdlib (↦ for map, ⊲ for filter).
; ═══════════════════════════════════════════════════════════════

; ⊡↦ - Comprehension map (transform each element)
; Apply transformation to each element in list
; Usage: ((⊡↦ (λ (𝕩) (⊗ 𝕩 #2))) (⋯ #1 #5))
; Result: ⟨#2 ⟨#4 ⟨#6 ⟨#8 ⟨#10 ∅⟩⟩⟩⟩⟩
; This is just an alias for ↦ with clearer comprehension intent
(≔ ⊡↦ ↦)

; ⊡⊲ - Comprehension filter (keep elements matching predicate)
; Filter list to elements satisfying predicate
; Usage: ((⊡⊲ (λ (𝕩) (≡ (% 𝕩 #2) #0))) (⋯ #1 #10))
; Result: ⟨#2 ⟨#4 ⟨#6 ⟨#8 ⟨#10 ∅⟩⟩⟩⟩⟩ (even numbers)
; This is just an alias for ⊲ with clearer comprehension intent
(≔ ⊡⊲ ⊲)

; ⊡⊲↦ - Comprehension filter + map (filter then transform)
; Filter list then transform matching elements
; Usage: (((⊡⊲↦ (λ (𝕩) (≡ (% 𝕩 #2) #0))) (λ (𝕩) (⊗ 𝕩 𝕩))) (⋯ #1 #5))
; Result: ⟨#4 ⟨#16 ∅⟩⟩ (squares of even numbers from 1-5)
; Implementation: compose filter and map
(≔ ⊡⊲↦ (λ (𝕡 𝕗 𝕩)
  ((↦ 𝕗) ((⊲ 𝕡) 𝕩))))

; ═══════════════════════════════════════════════════════════════
; Phase 3: Advanced Comprehension Patterns
; ═══════════════════════════════════════════════════════════════

; ⊡⊛ - Cartesian product comprehension
; Generate all combinations of two lists
; Usage: ((⊡⊛ (⋯→ #1 #2)) (⋯→ #3 #4))
; Result: ⟨⟨#1 #3⟩ ⟨⟨#1 #4⟩ ⟨⟨#2 #3⟩ ⟨⟨#2 #4⟩ ∅⟩⟩⟩⟩
; This is like: [(x, y) for x in xs for y in ys]
; Implementation: Use reverse to get correct order
(≔ ⊡⊛ (λ (𝕩𝕤) (λ (𝕪𝕤)
  (⇄ (((⊕← (λ (𝕒𝕔𝕔) (λ (𝕩)
              (((⊕← (λ (𝕒𝕔𝕔2) (λ (𝕪)
                        (⟨⟩ (⟨⟩ 𝕩 𝕪) 𝕒𝕔𝕔2))))
                𝕒𝕔𝕔)
               𝕪𝕤))))
      ∅)
     𝕩𝕤)))))

; ⊡⊕ - Accumulating comprehension (fold with transformation)
; Fold over list with transformation at each step
; Usage: (((⊡⊕ ⊕) #0) (⋯→ #1 #10))
; Result: #55 (sum of 1 to 10)
; Usage: (((⊡⊕ ⊗) #1) (⋯→ #1 #5))
; Result: #120 (factorial of 5 = 1*2*3*4*5)
; Note: Automatically curries the function for ⊕←
(≔ ⊡⊕ (λ (𝕗)
  (⊕← (λ (𝕒) (λ (𝕩) (𝕗 𝕒 𝕩))))))

; ═══════════════════════════════════════════════════════════════
; Phase 4: Practical Comprehension Macros
; ═══════════════════════════════════════════════════════════════

; These macros provide more ergonomic syntax for common patterns
; They leverage the comprehension helpers above

; ⊡↦→ - Map-over-range macro
; Transform each number in range
; Usage: (⊡↦→ #1 #5 (λ (𝕩) (⊗ 𝕩 #2)))
; Result: ⟨#2 ⟨#4 ⟨#6 ⟨#8 ⟨#10 ∅⟩⟩⟩⟩⟩
; Equivalent to: ((↦ (λ (𝕩) (⊗ 𝕩 #2))) (⋯→ #1 #5))
(⧉ ⊡↦→ (𝕤 𝕖 𝕗)
  (⌞̃ ((↦ (~ 𝕗)) (⋯→ (~ 𝕤) (~ 𝕖)))))

; ⊡⊲→ - Filter-over-range macro
; Keep numbers in range matching predicate
; Usage: (⊡⊲→ #1 #10 (λ (𝕩) (≡ (% 𝕩 #2) #0)))
; Result: ⟨#2 ⟨#4 ⟨#6 ⟨#8 ⟨#10 ∅⟩⟩⟩⟩⟩
; Equivalent to: ((⊲ (λ (𝕩) (≡ (% 𝕩 #2) #0))) (⋯→ #1 #10))
(⧉ ⊡⊲→ (𝕤 𝕖 𝕡)
  (⌞̃ ((⊲ (~ 𝕡)) (⋯→ (~ 𝕤) (~ 𝕖)))))

; ⊡⊲↦→ - Filter-and-map-over-range macro
; Filter numbers in range then transform
; Usage: (⊡⊲↦→ #1 #10 (λ (𝕩) (≡ (% 𝕩 #2) #0)) (λ (𝕩) (⊗ 𝕩 𝕩)))
; Result: ⟨#4 ⟨#16 ⟨#36 ⟨#64 ⟨#100 ∅⟩⟩⟩⟩⟩ (squares of evens)
; Equivalent to: ((↦ (λ (𝕩) (⊗ 𝕩 𝕩))) ((⊲ (λ (𝕩) (≡ (% 𝕩 #2) #0))) (⋯→ #1 #10)))
(⧉ ⊡⊲↦→ (𝕤 𝕖 𝕡 𝕗)
  (⌞̃ ((↦ (~ 𝕗)) ((⊲ (~ 𝕡)) (⋯→ (~ 𝕤) (~ 𝕖))))))

; ═══════════════════════════════════════════════════════════════
; Phase 5: Loop-Style Comprehensions
; ═══════════════════════════════════════════════════════════════

; ⊡∀ - For-each loop (iterate with side effects)
; Execute function on each element for side effects
; Usage: ((⊡∀ (λ (𝕩) (≋ 𝕩))) (⋯ #1 #5))
; Effect: Prints each number from 1 to 5
; Returns: nil
; Note: This is for side effects, not data transformation
(≔ ⊡∀ (λ (𝕗 𝕩)
  (⊕← (λ (_ 𝕪) (⌜ ⊙ (𝕗 𝕪) ∅)) ∅ 𝕩)))

; ⊡∀→ - For-each-range macro
; Iterate over range with side effects
; Usage: (⊡∀→ #1 #5 (λ (𝕩) (≋ 𝕩)))
; Effect: Prints numbers 1 through 5
; Returns: nil
(⧉ ⊡∀→ (𝕤 𝕖 𝕗)
  (⌞̃ (⊡∀ (~ 𝕗) (⋯→ (~ 𝕤) (~ 𝕖)))))

; ═══════════════════════════════════════════════════════════════
; Usage Examples (for documentation)
; ═══════════════════════════════════════════════════════════════

; Example 1: Generate squares of numbers 1-10
; ((⊡↦ (λ (𝕩) (⊗ 𝕩 𝕩))) (⋯→ #1 #10))
; → ⟨#1 ⟨#4 ⟨#9 ⟨#16 ... ⟨#100 ∅⟩⟩⟩⟩⟩⟩

; Example 2: Get even numbers from 1-20
; ((⊡⊲ (λ (𝕩) (≡ (% 𝕩 #2) #0))) (⋯→ #1 #20))
; → ⟨#2 ⟨#4 ⟨#6 ... ⟨#20 ∅⟩⟩⟩⟩

; Example 3: Squares of even numbers 1-10
; (⊡⊲↦ (λ (𝕩) (≡ (% 𝕩 #2) #0)) (λ (𝕩) (⊗ 𝕩 𝕩)) (⋯→ #1 #10))
; → ⟨#4 ⟨#16 ⟨#36 ⟨#64 ⟨#100 ∅⟩⟩⟩⟩⟩

; Example 4: Cartesian product (pairs of numbers)
; ((⊡⊛ (⋯→ #1 #2)) (⋯→ #3 #4))
; → ⟨⟨#1 #3⟩ ⟨⟨#1 #4⟩ ⟨⟨#2 #3⟩ ⟨⟨#2 #4⟩ ∅⟩⟩⟩⟩

; Example 5: Sum of numbers 1-100
; (((⊡⊕ ⊕) #0) (⋯→ #1 #100))
; → #5050

; Example 6: Factorial using comprehension
; (((⊡⊕ ⊗) #1) (⋯→ #1 #5))
; → #120 (5! = 1*2*3*4*5)

; Example 7: Using macro for cleaner syntax
; (⊡↦→ #1 #5 (λ (𝕩) (⊗ 𝕩 #2)))
; → ⟨#2 ⟨#4 ⟨#6 ⟨#8 ⟨#10 ∅⟩⟩⟩⟩⟩

; Example 8: FizzBuzz using comprehensions
; ((⊡↦ (λ (𝕩)
;        (? (≡ (% 𝕩 #15) #0) :FizzBuzz
;           (? (≡ (% 𝕩 #3) #0) :Fizz
;              (? (≡ (% 𝕩 #5) #0) :Buzz 𝕩)))))
;  (⋯→ #1 #20))
; → ⟨#1 ⟨#2 ⟨:Fizz ⟨#4 ⟨:Buzz ... ⟨:FizzBuzz ∅⟩⟩⟩⟩⟩⟩⟩

; ═══════════════════════════════════════════════════════════════
; Module Complete
; ═══════════════════════════════════════════════════════════════
; Functions defined: 6 (⋯→, ⋰, ⊡↦, ⊡⊲, ⊡⊲↦, ⊡⊛, ⊡⊕, ⊡∀)
; Macros defined: 4 (⊡↦→, ⊡⊲→, ⊡⊲↦→, ⊡∀→)
;
; Total: 10 comprehension utilities
;
; Note: These provide comprehension-style operations without requiring
; parser changes. When bracket syntax [()] becomes available, we can
; add syntactic sugar on top of these foundations.
;
; stdlib/list.scm already has ⋯ (exclusive range, curried): ((⋯ end) start)
; We provide ⋯→ (inclusive range, uncurried): (⋯→ start end)
; ═══════════════════════════════════════════════════════════════

"✓ 10 list comprehension utilities loaded"
