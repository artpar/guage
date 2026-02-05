; ─── Core Trait Protocols ───
; Showable, Equatable, Comparable
; Uses FDT-backed dispatch (trait-dispatch-fast) for ~5ns hot path

; ─── :Showable ───
(trait-define :Showable (cons :show nil))

(trait-implement :Number :Showable (cons (cons :show (lambda (x) (string x))) nil))
(trait-implement :Bool   :Showable (cons (cons :show (lambda (x) (if x "#t" "#f"))) nil))
(trait-implement :Symbol :Showable (cons (cons :show (lambda (x) (string x))) nil))
(trait-implement :Nil    :Showable (cons (cons :show (lambda (x) "∅")) nil))
(trait-implement :String :Showable (cons (cons :show (lambda (x) x)) nil))
(trait-implement :Error  :Showable (cons (cons :show (lambda (x) (string x))) nil))
(trait-implement :Pair   :Showable (cons (cons :show (lambda (x) (string x))) nil))
(trait-implement :Lambda :Showable (cons (cons :show (lambda (x) "<λ>")) nil))

; Convenience: uses trait-dispatch-fast (fused fast dispatch)
(define ⊧show (lambda (x) ((trait-dispatch-fast x :Showable :show) x)))

; ─── :Equatable ───
(trait-define :Equatable (cons :equal? nil))

(trait-implement :Number :Equatable (cons (cons :equal? (lambda (a) (lambda (b) (equal? a b)))) nil))
(trait-implement :Bool   :Equatable (cons (cons :equal? (lambda (a) (lambda (b) (equal? a b)))) nil))
(trait-implement :Symbol :Equatable (cons (cons :equal? (lambda (a) (lambda (b) (equal? a b)))) nil))
(trait-implement :Nil    :Equatable (cons (cons :equal? (lambda (a) (lambda (b) (equal? a b)))) nil))
(trait-implement :String :Equatable (cons (cons :equal? (lambda (a) (lambda (b) (string-equal? a b)))) nil))

(define ⊧≡ (lambda (a) (lambda (b) (((trait-dispatch-fast a :Equatable :equal?) a) b))))

; ─── :Comparable ───
(trait-define :Comparable (cons :compare nil))

(trait-implement :Number :Comparable
  (cons (cons :compare (lambda (a) (lambda (b) (if (< a b) :lt (if (equal? a b) :eq :gt))))) nil))
(trait-implement :String :Comparable
  (cons (cons :compare (lambda (a) (lambda (b) (if (string<? a b) :lt (if (string-equal? a b) :eq :gt))))) nil))

; Derived dispatchers
(define ⊧compare (lambda (a) (lambda (b) (((trait-dispatch-fast a :Comparable :compare) a) b))))
(define ⊧<  (lambda (a) (lambda (b) (equal? ((⊧compare a) b) :lt))))
(define ⊧≤  (lambda (a) (lambda (b) (not (equal? ((⊧compare a) b) :gt)))))
(define ⊧>  (lambda (a) (lambda (b) (equal? ((⊧compare a) b) :gt))))
(define ⊧≥  (lambda (a) (lambda (b) (not (equal? ((⊧compare a) b) :lt)))))

; Generic sort using trait dispatch
(define ⊧sort (lambda (lst)
  (if (null? lst) nil
    ((⊙sort→ (lambda (a) (lambda (b) ((⊧≤ a) b)))) lst))))

; ═══════════════════════════════════════════════════════════════
; Extended Trait Protocols (Day 143)
; Mappable, Foldable, Semigroup, Monoid, Filterable, Hashable
; ═══════════════════════════════════════════════════════════════

; ─── :Mappable ───
(trait-define :Mappable (cons :map nil))

(trait-implement :Pair   :Mappable (cons (cons :map (lambda (f) (lambda (coll) ((list-map f) coll)))) nil))
(trait-implement :Nil    :Mappable (cons (cons :map (lambda (f) (lambda (coll) nil))) nil))
(trait-implement :Vector :Mappable (cons (cons :map (lambda (f) (lambda (v) (vector-map v f)))) nil))

(define ⊧↦ (lambda (f) (lambda (coll) (((trait-dispatch-fast coll :Mappable :map) f) coll))))

; ─── :Foldable ───
(trait-define :Foldable (cons :fold-left (cons :fold-right nil)))

(trait-implement :Pair :Foldable
  (cons (cons :fold-left (lambda (f) (lambda (acc) (lambda (lst) (((fold-left f) acc) lst)))))
  (cons (cons :fold-right (lambda (f) (lambda (lst) (lambda (acc) (((fold-right f) lst) acc))))) nil)))
(trait-implement :Nil :Foldable
  (cons (cons :fold-left (lambda (f) (lambda (acc) (lambda (lst) acc))))
  (cons (cons :fold-right (lambda (f) (lambda (lst) (lambda (acc) acc)))) nil)))

(define ⊧⊕← (lambda (f) (lambda (acc) (lambda (coll) ((((trait-dispatch-fast coll :Foldable :fold-left) f) acc) coll)))))
(define ⊧⊕→ (lambda (f) (lambda (coll) (lambda (acc) ((((trait-dispatch-fast coll :Foldable :fold-right) f) coll) acc)))))

; ─── :Semigroup ───
(trait-define :Semigroup (cons :combine nil))

(trait-implement :Number :Semigroup (cons (cons :combine (lambda (a) (lambda (b) (+ a b)))) nil))
(trait-implement :String :Semigroup (cons (cons :combine (lambda (a) (lambda (b) (string-append a b)))) nil))
(trait-implement :Pair   :Semigroup (cons (cons :combine (lambda (a) (lambda (b) ((⧺ b) a)))) nil))
(trait-implement :Nil    :Semigroup (cons (cons :combine (lambda (a) (lambda (b) b))) nil))

(define ⊧⊕⊕ (lambda (a) (lambda (b) (((trait-dispatch-fast a :Semigroup :combine) a) b))))

; ─── :Monoid ───
(trait-define :Monoid (cons :empty nil))

(trait-implement :Number :Monoid (cons (cons :empty (lambda () #0)) nil))
(trait-implement :String :Monoid (cons (cons :empty (lambda () "")) nil))
(trait-implement :Pair   :Monoid (cons (cons :empty (lambda () nil)) nil))
(trait-implement :Nil    :Monoid (cons (cons :empty (lambda () nil)) nil))

(define ⊧∅ (lambda (x) ((trait-dispatch-fast x :Monoid :empty))))

; Generic mconcat: fold using combine + empty
(define ⊧mconcat (lambda (lst)
  (if (null? lst) nil
     (((⊧⊕← (lambda (acc) (lambda (x) ((⊧⊕⊕ acc) x)))) (⊧∅ (car lst))) lst))))

; ─── :Filterable ───
(trait-define :Filterable (cons :filter nil))

(trait-implement :Pair :Filterable (cons (cons :filter (lambda (pred) (lambda (coll) ((list-filter pred) coll)))) nil))
(trait-implement :Nil  :Filterable (cons (cons :filter (lambda (pred) (lambda (coll) nil))) nil))

(define ⊧⊲ (lambda (pred) (lambda (coll) (((trait-dispatch-fast coll :Filterable :filter) pred) coll))))

; ─── :Hashable ───
(trait-define :Hashable (cons :hash nil))

(trait-implement :Number :Hashable (cons (cons :hash (lambda (x) x)) nil))
(trait-implement :String :Hashable (cons (cons :hash (lambda (x) (string-length x))) nil))
(trait-implement :Symbol :Hashable (cons (cons :hash (lambda (x) (string-length (string x)))) nil))
