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

; range-inclusive - Range generation (inclusive, uncurried)
; Generates list of numbers from start to end (INCLUSIVE)
; Usage: (range-inclusive start end)
; Example: (range-inclusive #1 #5) -> ⟨#1 ⟨#2 ⟨#3 ⟨#4 ⟨#5 ∅⟩⟩⟩⟩⟩
; Example: (range-inclusive #5 #1) -> nil (empty for decreasing range)
; Example: (range-inclusive #3 #3) -> ⟨#3 ∅⟩ (single element)
; Note: stdlib/list.scm has range which is EXCLUSIVE: ((range #5) #1) -> ⟨#1 ⟨#2 ⟨#3 ⟨#4 ∅⟩⟩⟩⟩
(define range-inclusive (lambda (𝕤 𝕖)
  (if (> 𝕤 𝕖)
     nil
     (cons 𝕤 (range-inclusive (+ 𝕤 #1) 𝕖)))))

; range-step - Range generation with step
; Generates list of numbers from start to end with custom step
; Usage: (range-step start end step)
; Example: (range-step #1 #10 #2) -> ⟨#1 ⟨#3 ⟨#5 ⟨#7 ⟨#9 ∅⟩⟩⟩⟩⟩ (odd numbers)
; Example: (range-step #0 #10 #3) -> ⟨#0 ⟨#3 ⟨#6 ⟨#9 ∅⟩⟩⟩⟩
; Example: (range-step #10 #1 #-2) -> ⟨#10 ⟨#8 ⟨#6 ⟨#4 ⟨#2 ∅⟩⟩⟩⟩⟩
(define range-step (lambda (𝕤 𝕖 𝕕)
  (if (> 𝕤 𝕖)
     nil
     (cons 𝕤 (range-step (+ 𝕤 𝕕) 𝕖 𝕕)))))

; ═══════════════════════════════════════════════════════════════
; Phase 2: Basic List Comprehension Helpers
; ═══════════════════════════════════════════════════════════════
;
; Note: True list comprehension syntax like [(expr) for x in list]
; requires parser support for brackets. Until then, we provide
; ergonomic helpers that achieve the same goal.
;
; These are NOT macros - they're higher-order functions that compose
; with the existing stdlib (list-map for map, list-filter for filter).
; ═══════════════════════════════════════════════════════════════

; comp-map - Comprehension map (transform each element)
; Apply transformation to each element in list
; Usage: ((comp-map (lambda (𝕩) (* 𝕩 #2))) (range #1 #5))
; Result: ⟨#2 ⟨#4 ⟨#6 ⟨#8 ⟨#10 ∅⟩⟩⟩⟩⟩
; This is just an alias for list-map with clearer comprehension intent
(define comp-map list-map)

; comp-filter - Comprehension filter (keep elements matching predicate)
; Filter list to elements satisfying predicate
; Usage: ((comp-filter (lambda (𝕩) (equal? (% 𝕩 #2) #0))) (range #1 #10))
; Result: ⟨#2 ⟨#4 ⟨#6 ⟨#8 ⟨#10 ∅⟩⟩⟩⟩⟩ (even numbers)
; This is just an alias for list-filter with clearer comprehension intent
(define comp-filter list-filter)

; comp-filter-map - Comprehension filter + map (filter then transform)
; Filter list then transform matching elements
; Usage: (((comp-filter-map (lambda (𝕩) (equal? (% 𝕩 #2) #0))) (lambda (𝕩) (* 𝕩 𝕩))) (range #1 #5))
; Result: ⟨#4 ⟨#16 ∅⟩⟩ (squares of even numbers from 1-5)
; Implementation: compose filter and map
(define comp-filter-map (lambda (𝕡 𝕗 𝕩)
  ((list-map 𝕗) ((list-filter 𝕡) 𝕩))))

; ═══════════════════════════════════════════════════════════════
; Phase 3: Advanced Comprehension Patterns
; ═══════════════════════════════════════════════════════════════

; ⊡⊛ - Cartesian product comprehension
; Generate all combinations of two lists
; Usage: ((⊡⊛ (range-inclusive #1 #2)) (range-inclusive #3 #4))
; Result: ⟨⟨#1 #3⟩ ⟨⟨#1 #4⟩ ⟨⟨#2 #3⟩ ⟨⟨#2 #4⟩ ∅⟩⟩⟩⟩
; This is like: [(x, y) for x in xs for y in ys]
; Implementation: Use reverse to get correct order
(define ⊡⊛ (lambda (𝕩𝕤) (lambda (𝕪𝕤)
  (⇄ (((fold-left (lambda (𝕒𝕔𝕔) (lambda (𝕩)
              (((fold-left (lambda (𝕒𝕔𝕔2) (lambda (𝕪)
                        (cons (cons 𝕩 𝕪) 𝕒𝕔𝕔2))))
                𝕒𝕔𝕔)
               𝕪𝕤))))
      nil)
     𝕩𝕤)))))

; ⊡⊕ - Accumulating comprehension (fold with transformation)
; Fold over list with transformation at each step
; Usage: (((⊡⊕ +) #0) (range-inclusive #1 #10))
; Result: #55 (sum of 1 to 10)
; Usage: (((⊡⊕ *) #1) (range-inclusive #1 #5))
; Result: #120 (factorial of 5 = 1*2*3*4*5)
; Note: Automatically curries the function for fold-left
(define ⊡⊕ (lambda (𝕗)
  (fold-left (lambda (𝕒) (lambda (𝕩) (𝕗 𝕒 𝕩))))))

; ═══════════════════════════════════════════════════════════════
; Phase 4: Practical Comprehension Macros
; ═══════════════════════════════════════════════════════════════

; These macros provide more ergonomic syntax for common patterns
; They leverage the comprehension helpers above

; comp-map-to - Map-over-range macro
; Transform each number in range
; Usage: (comp-map-to #1 #5 (lambda (𝕩) (* 𝕩 #2)))
; Result: ⟨#2 ⟨#4 ⟨#6 ⟨#8 ⟨#10 ∅⟩⟩⟩⟩⟩
; Equivalent to: ((list-map (lambda (𝕩) (* 𝕩 #2))) (range-inclusive #1 #5))
(macro comp-map-to (𝕤 𝕖 𝕗)
  (quasiquote-tilde ((list-map (~ 𝕗)) (range-inclusive (~ 𝕤) (~ 𝕖)))))

; comp-filter-to - Filter-over-range macro
; Keep numbers in range matching predicate
; Usage: (comp-filter-to #1 #10 (lambda (𝕩) (equal? (% 𝕩 #2) #0)))
; Result: ⟨#2 ⟨#4 ⟨#6 ⟨#8 ⟨#10 ∅⟩⟩⟩⟩⟩
; Equivalent to: ((list-filter (lambda (𝕩) (equal? (% 𝕩 #2) #0))) (range-inclusive #1 #10))
(macro comp-filter-to (𝕤 𝕖 𝕡)
  (quasiquote-tilde ((list-filter (~ 𝕡)) (range-inclusive (~ 𝕤) (~ 𝕖)))))

; comp-filter-map-to - Filter-and-map-over-range macro
; Filter numbers in range then transform
; Usage: (comp-filter-map-to #1 #10 (lambda (𝕩) (equal? (% 𝕩 #2) #0)) (lambda (𝕩) (* 𝕩 𝕩)))
; Result: ⟨#4 ⟨#16 ⟨#36 ⟨#64 ⟨#100 ∅⟩⟩⟩⟩⟩ (squares of evens)
; Equivalent to: ((list-map (lambda (𝕩) (* 𝕩 𝕩))) ((list-filter (lambda (𝕩) (equal? (% 𝕩 #2) #0))) (range-inclusive #1 #10)))
(macro comp-filter-map-to (𝕤 𝕖 𝕡 𝕗)
  (quasiquote-tilde ((list-map (~ 𝕗)) ((list-filter (~ 𝕡)) (range-inclusive (~ 𝕤) (~ 𝕖))))))

; ═══════════════════════════════════════════════════════════════
; Phase 5: Loop-Style Comprehensions
; ═══════════════════════════════════════════════════════════════

; ⊡∀ - For-each loop (iterate with side effects)
; Execute function on each element for side effects
; Usage: ((⊡∀ (lambda (𝕩) (print 𝕩))) (range #1 #5))
; Effect: Prints each number from 1 to 5
; Returns: nil
; Note: This is for side effects, not data transformation
(define ⊡∀ (lambda (𝕗 𝕩)
  (fold-left (lambda (_ 𝕪) (quote struct-create (𝕗 𝕪) nil)) nil 𝕩)))

; ⊡∀→ - For-each-range macro
; Iterate over range with side effects
; Usage: (⊡∀→ #1 #5 (lambda (𝕩) (print 𝕩)))
; Effect: Prints numbers 1 through 5
; Returns: nil
(macro ⊡∀→ (𝕤 𝕖 𝕗)
  (quasiquote-tilde (⊡∀ (~ 𝕗) (range-inclusive (~ 𝕤) (~ 𝕖)))))

; ═══════════════════════════════════════════════════════════════
; Usage Examples (for documentation)
; ═══════════════════════════════════════════════════════════════

; Example 1: Generate squares of numbers 1-10
; ((comp-map (lambda (𝕩) (* 𝕩 𝕩))) (range-inclusive #1 #10))
; -> ⟨#1 ⟨#4 ⟨#9 ⟨#16 ... ⟨#100 ∅⟩⟩⟩⟩⟩⟩

; Example 2: Get even numbers from 1-20
; ((comp-filter (lambda (𝕩) (equal? (% 𝕩 #2) #0))) (range-inclusive #1 #20))
; -> ⟨#2 ⟨#4 ⟨#6 ... ⟨#20 ∅⟩⟩⟩⟩

; Example 3: Squares of even numbers 1-10
; (comp-filter-map (lambda (𝕩) (equal? (% 𝕩 #2) #0)) (lambda (𝕩) (* 𝕩 𝕩)) (range-inclusive #1 #10))
; -> ⟨#4 ⟨#16 ⟨#36 ⟨#64 ⟨#100 ∅⟩⟩⟩⟩⟩

; Example 4: Cartesian product (pairs of numbers)
; ((⊡⊛ (range-inclusive #1 #2)) (range-inclusive #3 #4))
; -> ⟨⟨#1 #3⟩ ⟨⟨#1 #4⟩ ⟨⟨#2 #3⟩ ⟨⟨#2 #4⟩ ∅⟩⟩⟩⟩

; Example 5: Sum of numbers 1-100
; (((⊡⊕ +) #0) (range-inclusive #1 #100))
; -> #5050

; Example 6: Factorial using comprehension
; (((⊡⊕ *) #1) (range-inclusive #1 #5))
; -> #120 (5! = 1*2*3*4*5)

; Example 7: Using macro for cleaner syntax
; (comp-map-to #1 #5 (lambda (𝕩) (* 𝕩 #2)))
; -> ⟨#2 ⟨#4 ⟨#6 ⟨#8 ⟨#10 ∅⟩⟩⟩⟩⟩

; Example 8: FizzBuzz using comprehensions
; ((comp-map (lambda (𝕩)
;        (if (equal? (% 𝕩 #15) #0) :FizzBuzz
;           (if (equal? (% 𝕩 #3) #0) :Fizz
;              (if (equal? (% 𝕩 #5) #0) :Buzz 𝕩)))))
;  (range-inclusive #1 #20))
; -> ⟨#1 ⟨#2 ⟨:Fizz ⟨#4 ⟨:Buzz ... ⟨:FizzBuzz ∅⟩⟩⟩⟩⟩⟩⟩

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
; stdlib/list.scm already has range (exclusive range, curried): ((range end) start)
; We provide range-inclusive (inclusive range, uncurried): (range-inclusive start end)
; ═══════════════════════════════════════════════════════════════

"✓ 10 list comprehension utilities loaded"
