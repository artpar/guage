; POSIX Standard Library (SRFI-170 accessors and predicates)
; These wrap the C primitives with higher-level accessors

; --- File Info Predicates ---
; Check if value is a file-info struct
(define ≋⊙? (lambda (x) (struct? x :file-info)))

; File type predicates (check mode bits from stat)
; S_IFMT = 0xF000, S_IFDIR = 0x4000, S_IFREG = 0x8000
; S_IFLNK = 0xA000, S_IFIFO = 0x1000, S_IFSOCK = 0xC000
; S_IFBLK = 0x6000, S_IFCHR = 0x2000

(define ≋⊙⊙? (lambda (fi) (equal? (and (struct-get fi :mode) #61440) #16384)))  ; directory?
(define ≋⊙≋? (lambda (fi) (equal? (and (struct-get fi :mode) #61440) #32768)))  ; regular?
(define ≋⊙→? (lambda (fi) (equal? (and (struct-get fi :mode) #61440) #40960)))  ; symlink?
(define ≋⊙⊞? (lambda (fi) (equal? (and (struct-get fi :mode) #61440) #4096)))   ; fifo?
(define ≋⊙⊟? (lambda (fi) (equal? (and (struct-get fi :mode) #61440) #49152)))  ; socket?
(define ≋⊙◈? (lambda (fi) (or (equal? (and (struct-get fi :mode) #61440) #24576)   ; block device?
                     (equal? (and (struct-get fi :mode) #61440) #8192)))) ; char device?

; --- File Info Accessors ---
(define ≋⊙:device  (lambda (fi) (struct-get fi :device)))
(define ≋⊙:inode   (lambda (fi) (struct-get fi :inode)))
(define ≋⊙:mode    (lambda (fi) (struct-get fi :mode)))
(define ≋⊙:nlinks  (lambda (fi) (struct-get fi :nlinks)))
(define ≋⊙:uid     (lambda (fi) (struct-get fi :uid)))
(define ≋⊙:gid     (lambda (fi) (struct-get fi :gid)))
(define ≋⊙:rdev    (lambda (fi) (struct-get fi :rdev)))
(define ≋⊙:size    (lambda (fi) (struct-get fi :size)))
(define ≋⊙:blksize (lambda (fi) (struct-get fi :blksize)))
(define ≋⊙:blocks  (lambda (fi) (struct-get fi :blocks)))
(define ≋⊙:atime   (lambda (fi) (struct-get fi :atime)))
(define ≋⊙:mtime   (lambda (fi) (struct-get fi :mtime)))
(define ≋⊙:ctime   (lambda (fi) (struct-get fi :ctime)))

; --- User Info Accessors ---
(define ⊙⌂⊕⊙:name  (lambda (ui) (struct-get ui :name)))
(define ⊙⌂⊕⊙:uid   (lambda (ui) (struct-get ui :uid)))
(define ⊙⌂⊕⊙:gid   (lambda (ui) (struct-get ui :gid)))
(define ⊙⌂⊕⊙:home  (lambda (ui) (struct-get ui :home)))
(define ⊙⌂⊕⊙:shell (lambda (ui) (struct-get ui :shell)))

; --- Group Info Accessors ---
(define ⊙⌂⊕⊕⊙:name    (lambda (gi) (struct-get gi :name)))
(define ⊙⌂⊕⊕⊙:gid     (lambda (gi) (struct-get gi :gid)))
(define ⊙⌂⊕⊕⊙:members (lambda (gi) (struct-get gi :members)))

; --- Time Accessors ---
(define ⊙⏱:seconds     (lambda (t) (struct-get t :seconds)))
(define ⊙⏱:nanoseconds (lambda (t) (struct-get t :nanoseconds)))

; --- File Space Accessors ---
(define ≋⊙#:total     (lambda (fs) (struct-get fs :total)))
(define ≋⊙#:free      (lambda (fs) (struct-get fs :free)))
(define ≋⊙#:available (lambda (fs) (struct-get fs :available)))

; --- Temp File Prefix (parameter/global) ---
(define ≋⊙⏱≈ "/tmp/guage-")

; --- call-with-temporary-filename ---
(define ≋⊙⏱λ (lambda (prefix fn)
  (define result (create-temp-file prefix))
  (define port (car result))
  (define path (cdr result))
  (define val (fn path port))
  (port-close port)
  (delete-file path)
  val))
