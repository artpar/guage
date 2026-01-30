; POSIX Standard Library (SRFI-170 accessors and predicates)
; These wrap the C primitives with higher-level accessors

; --- File Info Predicates ---
; Check if value is a file-info struct
(≔ ≋⊙? (λ (x) (⊙? x :file-info)))

; File type predicates (check mode bits from stat)
; S_IFMT = 0xF000, S_IFDIR = 0x4000, S_IFREG = 0x8000
; S_IFLNK = 0xA000, S_IFIFO = 0x1000, S_IFSOCK = 0xC000
; S_IFBLK = 0x6000, S_IFCHR = 0x2000

(≔ ≋⊙⊙? (λ (fi) (≡ (∧ (⊙→ fi :mode) #61440) #16384)))  ; directory?
(≔ ≋⊙≋? (λ (fi) (≡ (∧ (⊙→ fi :mode) #61440) #32768)))  ; regular?
(≔ ≋⊙→? (λ (fi) (≡ (∧ (⊙→ fi :mode) #61440) #40960)))  ; symlink?
(≔ ≋⊙⊞? (λ (fi) (≡ (∧ (⊙→ fi :mode) #61440) #4096)))   ; fifo?
(≔ ≋⊙⊟? (λ (fi) (≡ (∧ (⊙→ fi :mode) #61440) #49152)))  ; socket?
(≔ ≋⊙◈? (λ (fi) (∨ (≡ (∧ (⊙→ fi :mode) #61440) #24576)   ; block device?
                     (≡ (∧ (⊙→ fi :mode) #61440) #8192)))) ; char device?

; --- File Info Accessors ---
(≔ ≋⊙:device  (λ (fi) (⊙→ fi :device)))
(≔ ≋⊙:inode   (λ (fi) (⊙→ fi :inode)))
(≔ ≋⊙:mode    (λ (fi) (⊙→ fi :mode)))
(≔ ≋⊙:nlinks  (λ (fi) (⊙→ fi :nlinks)))
(≔ ≋⊙:uid     (λ (fi) (⊙→ fi :uid)))
(≔ ≋⊙:gid     (λ (fi) (⊙→ fi :gid)))
(≔ ≋⊙:rdev    (λ (fi) (⊙→ fi :rdev)))
(≔ ≋⊙:size    (λ (fi) (⊙→ fi :size)))
(≔ ≋⊙:blksize (λ (fi) (⊙→ fi :blksize)))
(≔ ≋⊙:blocks  (λ (fi) (⊙→ fi :blocks)))
(≔ ≋⊙:atime   (λ (fi) (⊙→ fi :atime)))
(≔ ≋⊙:mtime   (λ (fi) (⊙→ fi :mtime)))
(≔ ≋⊙:ctime   (λ (fi) (⊙→ fi :ctime)))

; --- User Info Accessors ---
(≔ ⊙⌂⊕⊙:name  (λ (ui) (⊙→ ui :name)))
(≔ ⊙⌂⊕⊙:uid   (λ (ui) (⊙→ ui :uid)))
(≔ ⊙⌂⊕⊙:gid   (λ (ui) (⊙→ ui :gid)))
(≔ ⊙⌂⊕⊙:home  (λ (ui) (⊙→ ui :home)))
(≔ ⊙⌂⊕⊙:shell (λ (ui) (⊙→ ui :shell)))

; --- Group Info Accessors ---
(≔ ⊙⌂⊕⊕⊙:name    (λ (gi) (⊙→ gi :name)))
(≔ ⊙⌂⊕⊕⊙:gid     (λ (gi) (⊙→ gi :gid)))
(≔ ⊙⌂⊕⊕⊙:members (λ (gi) (⊙→ gi :members)))

; --- Time Accessors ---
(≔ ⊙⏱:seconds     (λ (t) (⊙→ t :seconds)))
(≔ ⊙⏱:nanoseconds (λ (t) (⊙→ t :nanoseconds)))

; --- File Space Accessors ---
(≔ ≋⊙#:total     (λ (fs) (⊙→ fs :total)))
(≔ ≋⊙#:free      (λ (fs) (⊙→ fs :free)))
(≔ ≋⊙#:available (λ (fs) (⊙→ fs :available)))

; --- Temp File Prefix (parameter/global) ---
(≔ ≋⊙⏱≈ "/tmp/guage-")

; --- call-with-temporary-filename ---
(≔ ≋⊙⏱λ (λ (prefix fn)
  (≔ result (≋⊙⏱ prefix))
  (≔ port (◁ result))
  (≔ path (▷ result))
  (≔ val (fn path port))
  (⊞× port)
  (≋⊖ path)
  val))
