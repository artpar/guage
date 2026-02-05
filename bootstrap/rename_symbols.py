#!/usr/bin/env python3
"""
Guage Unicode→English Symbol Replacement Script

Replaces ALL Unicode symbols with English/Scheme-like equivalents.
Operates on both C source files and Guage (.test/.scm) files.

Usage:
    python3 rename_symbols.py --c        # Replace in C source files
    python3 rename_symbols.py --guage    # Replace in .test/.scm files
    python3 rename_symbols.py --all      # Replace in everything
    python3 rename_symbols.py --dry-run  # Show what would change
"""

import sys, os, re, glob

# ============================================================
# COMPLETE SYMBOL MAPPING: Unicode → English
# ============================================================

# --- 34 Special Forms (intern.c preload order, IDs 0-33) ---
SPECIAL_FORMS = {
    "⌜": "quote",                  # 0
    "⌞̃": "quasiquote-tilde",      # 1
    "⧉⊜": "macro-rules",          # 2
    "⧉": "macro",                  # 3  (note: also arity primitive)
    "≔": "define",                 # 4
    "∈": "type-decl",             # 5  (also primitive)
    "∈?": "type-check",           # 6
    "∈✓": "type-validate",        # 7
    "∈⍜": "type-infer",           # 8
    "∈⊢": "type-assert",          # 9
    "∈⍜*": "type-infer-all",      # 10
    ":λ-converted": ":lambda-converted",  # 11
    "λ": "lambda",                 # 12
    # "?" stays as "if" — handled specially
    "⪢": "begin",                  # 14
    "∇": "match",                  # 15
    "⟪": "effect-def",            # 16
    "⟪?": "effect?",              # 17
    "⟪→": "effect-get",           # 18
    "⟪⟫": "handle",               # 19
    "⟪↺⟫": "handle-resume",       # 20
    "↯": "perform",               # 21
    "⟪⊸⟫": "handle-linear",       # 22
    "⊸": "consume",               # 23
    # ":__indexed__" stays          # 24
    # "~" stays                     # 25
    # "quasiquote" stays            # 26
    # "unquote" stays               # 27
    "∧": "and",                    # 28
    "∨": "or",                     # 29
    "⚡?": "try",                  # 30  (special form)
    "∈⊡": "refine-def",           # 31
    "⊳": "generic-param",         # 32
    "≫": "bind",                   # 33
}

# --- Stdlib Macro Names (user-defined, not in C) ---
STDLIB_MACROS = {
    # Macros from macros.scm
    "?¬": "unless",
    "∧…": "and-all",
    "∨…": "or-all",
    "⊳→": "thread-first",
    "≔↓": "let-local",
    "≔↻": "letrec-local",
    "∘": "compose",
    "⊰": "partial",
    "↔": "flip",
    # Comprehensions from comprehensions.scm (longest first)
    "⊡⊲↦→": "comp-filter-map-to",
    "⊡⊲↦": "comp-filter-map",
    "⊡⊲→": "comp-filter-to",
    "⊡↦→": "comp-map-to",
    "⊡⊲": "comp-filter",
    "⊡↦": "comp-map",
    "⋯→": "range-inclusive",
    "⋰": "range-step",
    # List functions from list.scm
    "↦": "list-map",
    "⊲": "list-filter",
    "⊕←": "fold-left",
    "⊕→": "fold-right",
    "⋯": "range",
    # Exception macros from macros_exception.scm (longest first)
    "⚡⇒-find-handler": "match-error-find-handler",
    "⚡⇒-apply": "match-error-apply",
    "⚡?-impl": "error-type-impl",
    "⚡⊙-impl": "error-data-impl",
    "⚡↦-impl": "map-errors-impl",
    "⚡↺-impl": "retry-impl",
    "⚠⊙ignored": "err-ignored",  # Internal variable name
    "⚠⊙result": "err-result",    # Internal variable name
    "⚠⊙r1": "err-r1",            # Internal variable name
    "⚡⇒": "match-error",
    "⚡⟲": "trace-error",
    "⚡⊳": "try-or",
    "⚡∅": "ignore-errors",
    "⚡↦": "map-errors",
    "⚡↺": "retry",
    "⚡∧": "and-errors",
    "⚡∨": "or-errors",
    "⚡⊙": "error-data",
    "⚡": "try-with",
}

# --- 558 Primitives (primitives.c) ---
PRIMITIVES = {
    # Core Lambda Calculus
    "⟨⟩": "cons",
    "◁": "car",
    "▷": "cdr",

    # Metaprogramming
    # "⌜" already in special forms (quote)
    "⌞": "eval",
    "⊡": "apply-primitive",

    # Comparison & Logic
    "≡": "equal?",
    "≢": "not-equal?",
    "¬": "not",

    # Arithmetic
    "⊕": "+",
    "⊖": "-",
    "⊗": "*",
    "⊘": "/",
    "÷": "quotient",
    # "%" stays
    # "<" stays
    # ">" stays
    "≤": "<=",
    "≥": ">=",

    # Math
    "√": "sqrt",
    # "^" stays
    # "|" → "abs" handled separately (ASCII single-char, conflicts with C bitwise OR)
    "⌊⌋": "floor",
    "⌈⌉": "ceil",
    "⌊⌉": "round",
    # "min" stays
    # "max" stays
    # "sin" stays
    # "cos" stays
    # "tan" stays
    # "asin" stays
    # "acos" stays
    # "atan" stays
    # "atan2" stays
    # "log" stays
    # "log10" stays
    # "exp" stays
    "π": "pi",
    # "e" stays
    # "rand" stays
    # "rand-int" stays

    # Type predicates
    "ℕ?": "number?",
    "𝔹?": "boolean?",
    ":?": "symbol?",
    "∅?": "null?",
    "⟨⟩?": "pair?",
    "#?": "atom?",

    # Type constants
    "ℤ": "Int",
    "𝔹": "Bool",
    "𝕊": "String",
    "⊤": "Any",
    "∅ₜ": "Nil-type",
    "→": "->",
    "[]ₜ": "List-type",
    "⟨⟩ₜ": "Pair-type",
    "∪ₜ": "Union-type",
    "∈⊙": "type-of",
    "∈≡": "type-equal?",
    "∈⊆": "type-subtype?",
    "∈!": "type-assert!",
    # "∈" already in special forms
    # "∈?" already in special forms
    "∈◁": "type-domain",
    "∈▷": "type-codomain",
    "∈⊙ₜ": "type-element",
    # "∈✓" already in special forms
    "∈✓*": "type-validate-all",
    # "∈⊢" already in special forms

    # Type Inference
    # "∈⍜" already in special forms
    "∈⍜⊕": "type-prim-sig",

    # Debug & Error
    "⚠": "error",
    "⚠?": "error?",
    "⚠⊙": "error-type",
    "⚠→": "error-data",
    "⚡⊕": "error-wrap",
    "⚠⊸": "error-cause",
    "⚠⊸*": "error-root-cause",
    "⚠⟲": "error-trace",
    "⚠⊙?": "error-chain-match?",
    "⊢": "assert",
    "⟲": "trace",

    # Self-Introspection
    "⧉": "arity",     # NOTE: conflicts with macro special form
    "⊛": "source",

    # Macro System
    "⊛⊙": "gensym",
    "⧉→": "macro-expand",
    "⧉?": "macro-list",

    # Testing
    "≟": "deep-equal?",
    "⊨": "test-case",

    # Test Runner
    "⊨⊕⊙": "test-register",
    "⊨⊕!": "test-run-registry",
    "⊨⊜": "test-results",
    "⊨⊜∅": "test-reset",
    "⊨⊜#": "test-count",
    "⊨⊜×": "test-exit",

    # Property Testing
    # "gen-int" stays
    # "gen-bool" stays
    # "gen-symbol" stays
    # "gen-list" stays
    # "gen-int-shrink" stays
    # "gen-list-shrink" stays
    "⊨-prop": "test-property",

    # Effects
    "⤴": "effect-pure",

    # Actors
    "⟳": "actor-spawn",
    "→!": "actor-send",
    "←?": "actor-receive",
    "⟳!": "actor-run",
    "⟳#": "sched-count",
    "⟳#⊙": "sched-id",
    "⟳#?": "sched-stats",
    "⟳⊞⊛": "cpu-count",
    "⟳?": "actor-alive?",
    "⟳→": "actor-result",
    "⟳⚐": "actor-wait-flag",
    "⟳∅": "actor-reset",

    # Supervision
    "⟳⊗": "actor-link",
    "⟳⊘": "actor-unlink",
    "⟳⊙": "actor-monitor",
    "⟳⊜": "actor-trap-exit",
    "⟳✕": "actor-exit",

    # Supervisor
    "⟳⊛": "sup-start",
    "⟳⊛?": "sup-children",
    "⟳⊛!": "sup-restart-count",
    "⟳⊛⊕": "sup-add-child",
    "⟳⊛⊖": "sup-remove-child",

    # DynamicSupervisor
    "⟳⊛⊹": "dynsup-start",
    "⟳⊛⊹⊕": "dynsup-start-child",
    "⟳⊛⊹⊖": "dynsup-terminate-child",
    "⟳⊛⊹?": "dynsup-which-children",
    "⟳⊛⊹#": "dynsup-count",

    # Process Registry
    "⟳⊜⊕": "registry-register",
    "⟳⊜⊖": "registry-unregister",
    "⟳⊜?": "registry-whereis",
    "⟳⊜*": "registry-list",
    "⟳⇅": "actor-call",
    "⟳⇅!": "actor-reply",
    "⟳⏱": "timer-send-after",
    "⟳⏱×": "timer-cancel",
    "⟳⏱?": "timer-active?",

    # Process Dictionary
    "⟳⊔⊕": "proc-dict-put",
    "⟳⊔?": "proc-dict-get",
    "⟳⊔⊖": "proc-dict-erase",
    "⟳⊔*": "proc-dict-all",

    # ETS
    "⟳⊞⊕": "ets-new",
    "⟳⊞⊙": "ets-insert",
    "⟳⊞?": "ets-lookup",
    "⟳⊞⊖": "ets-delete-key",
    "⟳⊞!": "ets-delete-table",
    "⟳⊞#": "ets-size",
    "⟳⊞*": "ets-all",

    # Application
    "⟳⊚⊕": "app-start",
    "⟳⊚⊖": "app-stop",
    "⟳⊚?": "app-info",
    "⟳⊚*": "app-which",
    "⟳⊚⊙": "app-get-env",
    "⟳⊚←": "app-set-env",

    # Task
    "⟳⊳": "task-async",
    "⟳⊲": "task-await",
    "⟳⊲?": "task-yield",

    # Agent
    "⟳⊶": "agent-start",
    "⟳⊶?": "agent-get",
    "⟳⊶!": "agent-update",
    "⟳⊶⊕": "agent-get-and-update",
    "⟳⊶×": "agent-stop",

    # GenStage
    "⟳⊵": "stage-new",
    "⟳⊵⊕": "stage-subscribe",
    "⟳⊵→": "stage-ask",
    "⟳⊵⊙": "stage-dispatch",
    "⟳⊵?": "stage-info",
    "⟳⊵×": "stage-stop",

    # Flow
    "⟳⊸": "flow-from",
    "⟳⊸↦": "flow-map",
    "⟳⊸⊲": "flow-filter",
    "⟳⊸⊕": "flow-reduce",
    "⟳⊸⊙": "flow-each",
    "⟳⊸!": "flow-run",

    # Flow Registry
    "⟳⊸⊜⊕": "flow-registry-register",
    "⟳⊸⊜⊖": "flow-registry-unregister",
    "⟳⊸⊜?": "flow-registry-whereis",
    "⟳⊸⊜*": "flow-registry-list",

    # Channel
    "⟿⊚": "chan-create",
    "⟿→": "chan-send",
    "⟿←": "chan-recv",
    "⟿×": "chan-close",
    "⟿∅": "chan-reset",
    "⟿⊞": "chan-select",
    "⟿⊞?": "chan-select-try",

    # Documentation
    "⌂": "doc",
    "⌂∈": "doc-type",
    "⌂≔": "doc-deps",
    "⌂⊛": "doc-source",
    "⌂⊨": "doc-tests",
    "⌂⊨!": "doc-tests-run",
    "⌂⊨⊗": "mutation-test",
    "📖": "doc-generate",
    "📖→": "doc-export",
    "📖⊛": "doc-index",
    "⌂⊚": "module-info",

    # CFG/DFG
    "⌂⟿": "query-cfg",
    "⌂⇝": "query-dfg",

    # Structure - Leaf
    "⊙≔": "struct-define",
    "⊙": "struct-create",
    "⊙→": "struct-get",
    "⊙←": "struct-set",
    "⊙?": "struct?",

    # Structure - ADT
    "⊚≔": "adt-define",
    "⊚": "adt-create",
    "⊚→": "adt-get",
    "⊚?": "adt?",

    # Graph
    "⊝≔": "graph-define",
    "⊝": "graph-create",
    "⊝⊕": "graph-add-node",
    "⊝⊗": "graph-add-edge",
    "⊝→": "graph-query",
    "⊝?": "graph?",

    # Graph Algorithms
    "⊝↦": "graph-traverse",
    "⊝⊃": "graph-reachable?",
    "⊝⊚": "graph-successors",
    "⊝⊙": "graph-predecessors",
    "⊝⇝": "graph-path",
    "⊝∘": "graph-cycles",

    # String
    "≈": "string",
    "≈⊕": "string-append",
    "≈#": "string-length",
    "≈→": "string-ref",
    "≈⊂": "string-slice",
    "≈?": "string?",
    "≈∅?": "string-empty?",
    "≈≡": "string-equal?",
    "≈<": "string<?",
    "≈→#": "string-char-code",
    "#→≈": "code->char",
    "≈→ℕ": "string->number",
    "≈→:": "string->symbol",
    "≈↑": "string-upcase",
    "≈↓": "string-downcase",

    # String SDK
    "≈⊳": "string-find",
    "≈⊲": "string-rfind",
    "≈∈?": "string-contains?",
    "≈⊲?": "string-starts-with?",
    "≈⊳?": "string-ends-with?",
    "≈⊳#": "string-count",
    "≈⇄": "string-reverse",
    "≈⊛": "string-repeat",
    "≈⇔": "string-replace",
    "≈⇔#": "string-replace-n",
    "≈⊏": "string-trim-left",
    "≈⊐": "string-trim-right",
    "≈⊏⊐": "string-trim",
    "≈÷": "string-split",
    "≈÷#": "string-split-n",
    "≈÷⊔": "string-fields",
    "≈⊏⊕": "string-pad-left",
    "≈⊐⊕": "string-pad-right",
    "≈⊏⊖": "string-strip-prefix",
    "≈⊐⊖": "string-strip-suffix",

    # I/O Console
    "≋": "print",
    "≋≈": "display",
    "≋←": "read-line",

    # I/O Files
    "≋⊳": "read-file",
    "≋⊲": "write-file",
    "≋⊕": "append-file",
    "≋?": "file-exists?",
    "≋∅?": "file-empty?",

    # Module
    "⋘": "load",
    "⋖": "module-import",
    "⌂⊚→": "module-dependencies",
    "⌂⊚#": "module-version",
    "⌂⊚↑": "module-exports",
    "⌂⊚⊛": "module-cycles",
    "⋘?": "module-loaded?",
    "⊞◇": "module-define",
    "⋘⊳": "module-import-validated",

    # Mutable Refs
    "□": "box",
    "□→": "unbox",
    "□←": "box-set!",
    "□?": "box?",
    "□⊕": "box-update!",
    "□⇌": "box-swap!",

    # Weak Refs
    "◇": "weak-ref",
    "◇→": "weak-deref",
    "◇?": "weak-alive?",
    "◇⊙": "weak-ref?",

    # HashMap
    "⊞": "hashmap",
    "⊞→": "hashmap-get",
    "⊞←": "hashmap-put",
    "⊞⊖": "hashmap-del",
    "⊞?": "hashmap?",
    "⊞∋": "hashmap-has?",
    "⊞#": "hashmap-size",
    "⊞⊙": "hashmap-keys",
    "⊞⊗": "hashmap-vals",
    "⊞*": "hashmap-entries",
    "⊞⊕": "hashmap-merge",

    # HashSet
    "⊍": "set",
    "⊍⊕": "set-add",
    "⊍⊖": "set-remove",
    "⊍?": "set?",
    "⊍∋": "set-has?",
    "⊍#": "set-size",
    "⊍⊙": "set-elements",
    "⊍∪": "set-union",
    "⊍∩": "set-intersection",
    "⊍∖": "set-difference",
    "⊍⊆": "set-subset?",

    # Deque
    "⊟": "deque",
    "⊟◁": "deque-push-front",
    "⊟▷": "deque-push-back",
    "⊟◁⊖": "deque-pop-front",
    "⊟▷⊖": "deque-pop-back",
    "⊟◁?": "deque-peek-front",
    "⊟▷?": "deque-peek-back",
    "⊟#": "deque-size",
    "⊟?": "deque?",
    "⊟⊙": "deque-to-list",
    "⊟∅?": "deque-empty?",

    # Buffer
    "◈": "bytebuf",
    "◈←": "bytebuf-get",
    "◈→": "bytebuf-set",
    "◈⊕": "bytebuf-append",
    "◈⊕⊕": "bytebuf-concat",
    "◈#": "bytebuf-size",
    "◈?": "bytebuf?",
    "◈⊂": "bytebuf-slice",
    "◈⊙": "bytebuf-to-list",
    "◈≈": "bytebuf->string",
    "≈◈": "string->bytebuf",

    # Vector
    "⟦⟧": "vector",
    "⟦→": "vector-ref",
    "⟦←": "vector-set!",
    "⟦⊕": "vector-push!",
    "⟦⊖": "vector-pop!",
    "⟦#": "vector-length",
    "⟦?": "vector?",
    "⟦⊙": "vector->list",
    "⟦∅?": "vector-empty?",
    "⟦⊞": "vector-slice",
    "⟦↦": "vector-map",

    # Heap
    "△": "heap",
    "△⊕": "heap-push!",
    "△⊖": "heap-pop!",
    "△◁": "heap-peek",
    "△#": "heap-size",
    "△?": "heap?",
    "△∅?": "heap-empty?",
    "△⊙": "heap->list",
    "△⊕*": "heap-merge",

    # Sorted Map
    "⋔": "sorted-map",
    "⋔→": "sorted-map-get",
    "⋔←": "sorted-map-put",
    "⋔⊖": "sorted-map-del",
    "⋔?": "sorted-map?",
    "⋔∋": "sorted-map-has?",
    "⋔#": "sorted-map-size",
    "⋔⊙": "sorted-map-keys",
    "⋔⊗": "sorted-map-vals",
    "⋔*": "sorted-map-entries",
    "⋔⊕": "sorted-map-merge",
    "⋔◁": "sorted-map-min",
    "⋔▷": "sorted-map-max",
    "⋔⊂": "sorted-map-range",
    "⋔≤": "sorted-map-floor",
    "⋔≥": "sorted-map-ceiling",

    # Trie
    "⊮": "trie",
    "⊮→": "trie-get",
    "⊮←": "trie-put",
    "⊮⊖": "trie-del",
    "⊮?": "trie?",
    "⊮∋": "trie-has?",
    "⊮#": "trie-size",
    "⊮⊕": "trie-merge",
    "⊮⊙": "trie-prefix-keys",
    "⊮⊗": "trie-prefix-count",
    "⊮≤": "trie-longest-prefix",
    "⊮*": "trie-entries",
    "⊮⊙*": "trie-keys",
    "⊮⊗*": "trie-vals",

    # Iterator
    "⊣": "iter",
    "⊣→": "iter-next",
    "⊣?": "iter?",
    "⊣∅?": "iter-done?",
    "⊣⊕": "iter-collect",
    "⊣#": "iter-count",
    "⊣↦": "iter-map",
    "⊣⊲": "iter-filter",
    "⊣↑": "iter-take",
    "⊣↓": "iter-drop",
    "⊣⊕⊕": "iter-chain",
    "⊣⊗": "iter-zip",
    "⊣Σ": "iter-reduce",
    "⊣∃": "iter-any?",
    "⊣∀": "iter-all?",
    "⊣⊙": "iter-find",

    # Ports
    "⊞⊳": "port-open",
    "⊞⊳#": "fd->port",
    "⊞⊳←": "port-read-line",
    "⊞←◈": "port-read-bytes",
    "⊞←*": "port-read-all",
    "⊞⊳→": "port-write",
    "⊞→◈": "port-write-bytes",
    "⊞×": "port-close",
    "⊞∅?": "port-eof?",
    "⊞⊳⊙": "port-flush",
    "⊞⊳₀": "stdin-port",
    "⊞⊲₀": "stdout-port",
    "⊞⊲₁": "stderr-port",

    # Filesystem
    "≋⊙⊕": "mkdir",
    "≋⊙⊘": "rmdir",
    "≋⇔": "rename-file",
    "≋⊙≔": "chmod",
    "≋⊙⊕≔": "chown",
    "≋⏱≔": "utimes",
    "≋⊂": "truncate",
    "≋⊕⊝": "link",
    "≋⊕→": "symlink",
    "≋→": "readlink",
    "≋⊙⊕⊞": "mkfifo",
    "≋⊙": "file-info",
    "≋⊙*": "directory-files",
    "≋⊙⊳": "opendir",
    "≋⊙←": "readdir",
    "≋⊙×": "closedir",
    "≋⊙⊣": "directory-generator",
    "≋⊙⊕→": "realpath",
    "≋⊙#": "file-space",
    "≋⊙⏱": "create-temp-file",
    "≋⊖": "delete-file",

    # Process State
    "⊙⌂⊙": "umask",
    "⊙⌂⊙≔": "umask-set!",
    "⊙⌂⊘": "cwd",
    "⊙⌂⊘≔": "chdir",
    "⊙⌂#": "pid",
    "⊙⌂△": "nice",
    "⊙⌂⊕": "uid",
    "⊙⌂⊕⊕": "gid",
    "⊙⌂⊕*": "euid",
    "⊙⌂⊕⊕*": "egid",
    "⊙⌂⊕⊕*⊕": "groups",

    # User/Group
    "⊙⌂⊕⊙": "user-info",
    "⊙⌂⊕⊕⊙": "group-info",

    # Time
    "⊙⏱": "posix-time",
    "⊙⏱⊕": "monotonic-time",

    # Environment
    "⊙⌂≋": "getenv",
    "⊙⌂≋≔": "setenv",
    "⊙⌂≋⊘": "unsetenv",

    # Terminal
    "⊞⊙?": "terminal?",

    # System
    "⊙⌂": "argv",
    "⊙⊘": "exit",
    "⊙⏱≈": "current-second",
    "⊙⏱⊕#": "jiffy",
    "⊙⏱⊕≈": "jiffies-per-second",

    # FFI
    "⌁⊳": "ffi-dlopen",
    "⌁×": "ffi-dlclose",
    "⌁→": "ffi-bind",
    "⌁!": "ffi-call",
    "⌁?": "ffi-ptr?",
    "⌁⊙": "ffi-type-tag",
    "⌁⊞": "ffi-wrap",
    "⌁⊞×": "ffi-wrap-fin",
    "⌁∅": "ffi-null",
    "⌁∅?": "ffi-null?",
    "⌁#": "ffi-addr",
    "⌁≈→": "ffi-read-cstr",
    "⌁→≈": "ffi-str->ptr",
    "⌁◈→": "ffi-read-buf",
    "⌁→◈": "ffi-buf->ptr",

    # Networking - Socket lifecycle
    "⊸⊕": "net-socket",
    "⊸×": "net-close",
    "⊸×→": "net-shutdown",
    "⊸⊕⊞": "net-socketpair",
    "⊸?": "net-socket?",

    # Networking - Address
    "⊸⊙": "net-addr",
    "⊸⊙₆": "net-addr6",
    "⊸⊙⊘": "net-addr-unix",

    # Networking - Client/Server
    "⊸→⊕": "net-connect",
    "⊸←≔": "net-bind-addr",
    "⊸←⊕": "net-listen",
    "⊸←": "net-accept",
    "⊸⊙→": "net-resolve",

    # Networking - I/O
    "⊸→": "net-send",
    "⊸←◈": "net-recv",
    "⊸→⊙": "net-sendto",
    "⊸←⊙": "net-recvfrom",

    # Networking - Options
    "⊸≔": "net-setsockopt",
    "⊸≔→": "net-getsockopt",
    "⊸#": "net-peername",

    # Async Ring
    "⊸⊚⊕": "ring-create",
    "⊸⊚×": "ring-destroy",
    "⊸⊚?": "ring?",

    # Buffer Pool
    "⊸⊚◈⊕": "ring-buf-create",
    "⊸⊚◈×": "ring-buf-destroy",
    "⊸⊚◈→": "ring-buf-get",
    "⊸⊚◈←": "ring-buf-return",

    # Async Ring Operations
    "⊸⊚←": "ring-accept",
    "⊸⊚←◈": "ring-recv",
    "⊸⊚→": "ring-send",
    "⊸⊚→∅": "ring-send-zc",
    "⊸⊚→⊕": "ring-connect",
    "⊸⊚→×": "ring-close",
    "⊸⊚!": "ring-submit",
    "⊸⊚⊲": "ring-complete",

    # Refinement Types
    # "∈⊡" already in special forms
    "∈⊡?": "refine-check?",
    "∈⊡!": "refine-assert!",
    "∈⊡⊙": "refine-base",
    "∈⊡→": "refine-pred",
    "∈⊡⊢": "refine-constraint",
    "∈⊡∧": "refine-intersect",
    "∈⊡∨": "refine-union",
    "∈⊡∀": "refine-list",
    "∈⊡∈": "refine-find",
    "∈⊡⊆": "refine-subtype?",

    # Execution Trace
    "⟳⊳⊳!": "trace-enable!",
    "⟳⊳⊳?": "trace-read",
    "⟳⊳⊳∅": "trace-clear",
    "⟳⊳⊳#": "trace-count",
    "⟳⊳⊳⊛": "trace-snapshot",
    "⟳⊳⊳⊗": "trace-causal",
    "⟳⊳⊳⊞": "trace-capacity",

    # Global Trace
    "⟳⊳⊳⊕": "trace-global-read",
    "⟳⊳⊳⊕#": "trace-global-count",

    # Traits
    "⊧≔": "trait-define",
    "⊧⊕": "trait-implement",
    "⊧?": "trait?",
    "⊧⊙": "trait-ops",
    "⊧→": "trait-dispatch",
    "⊧→!": "trait-dispatch-fast",
    "⊧∈": "runtime-type-of",
    "⊧⊙?": "trait-defaults",

    # Bitwise
    "⊓": "bit-and",
    "⊔": "bit-or",
    "⊻": "bit-xor",
    "⊬": "bit-not",
    "≪": "bit-shl",
    "⊓≫": "bit-shr",
    "⊓≫ᵤ": "bit-ushr",
    "⊓#": "bit-popcount",
    "⊓◁": "bit-clz",
    "⊓▷": "bit-ctz",
    "⊓⟲": "bit-rotl",
    "⊓⟳": "bit-rotr",
    "→ℤ": "->integer",
    "→ℝ": "->double",
    "ℤ?": "integer?",

    # FFI Struct
    "⌁⊙⊜": "ffi-struct-define",
    "⌁⊙→": "ffi-struct-read",
    "⌁⊙←": "ffi-struct-write",
    "⌁⊙⊞": "ffi-struct-alloc",
    "⌁⊙#": "ffi-struct-size",
    "⌁⊙⊳": "ffi-struct->guage",
    "⌁⊙⊲": "ffi-struct-from-guage",

    # FFI Callback
    "⌁⤺": "ffi-callback-create",
    "⌁⤺×": "ffi-callback-free",

    # Signal
    "⚡⟳": "signal-register",
    "⚡×": "signal-unregister",
    # "⚡?" conflicts — signal-list vs try special form
    # The special form ⚡? maps to "try", this primitive ⚡? maps to "signal-list"
    # They have the same Unicode but different meanings... need to check

    # Discovery
    "⌂*": "discovery-all",
    "⌂⊳": "discovery-search",
    "⌂⊳⊜": "discovery-category",
}

# Nil is special — it's both a symbol and a printed representation
NIL_MAPPING = {
    "∅": "nil",
}

# The standalone "?" → "if" mapping (applied last, only at word boundaries)
QUESTION_TO_IF = {
    "?": "if",
}

# ============================================================
# Merge all mappings, sort by key length descending
# ============================================================


# Symbols that are single ASCII chars and dangerous for global replacement
# These must only be replaced as complete tokens, never inside C code
ASCII_TOKEN_REPLACEMENTS = {
    "|": "abs",       # C bitwise OR
    "?": "if",        # C ternary operator
}

def build_sorted_mapping():
    """Build the complete mapping sorted by key length (longest first).
    Excludes dangerous single-char ASCII symbols."""
    all_mappings = {}
    all_mappings.update(SPECIAL_FORMS)
    all_mappings.update(STDLIB_MACROS)
    all_mappings.update(PRIMITIVES)
    all_mappings.update(NIL_MAPPING)

    # Sort by key length descending (longest first to avoid partial matches)
    sorted_items = sorted(all_mappings.items(), key=lambda x: len(x[0].encode('utf-8')), reverse=True)
    return sorted_items

def is_all_ascii(s):
    """Check if string is pure ASCII."""
    return all(ord(c) < 128 for c in s)

def replace_in_guage_source(content, mapping):
    """Replace symbols in Guage source code (.test/.scm files).

    Uses TOKEN-BASED replacement: only replaces complete tokens,
    not substrings within longer symbol names.

    A token boundary in Lisp is: start/end of string, whitespace, or parentheses.
    Also handles :keyword syntax where the keyword is a primitive name.
    """

    for old, new in mapping:
        # Skip single-char ASCII that are also C operators
        if len(old) == 1 and is_all_ascii(old) and old in '|?':
            continue

        # Escape special regex characters in the old symbol
        escaped_old = re.escape(old)

        # Pattern 1: bare symbol as token
        # Token boundaries: start/end, whitespace, parens
        pattern = r'(?<=[(\s])' + escaped_old + r'(?=[)\s]|$)'
        content = re.sub(r'^' + escaped_old + r'(?=[)\s]|$)', new, content)
        content = re.sub(pattern, new, content)

        # Pattern 2: keyword-prefixed symbol (:old → :new)
        # Only if old is a primitive (not already starting with :)
        if not old.startswith(':'):
            kw_pattern = r'(?<=[(\s]):' + escaped_old + r'(?=[)\s]|$)'
            content = re.sub(kw_pattern, ':' + new, content)

    # Handle standalone "?" → "if" (the conditional special form)
    # Only when preceded by "(" and followed by whitespace
    content = re.sub(r'\(\?(\s)', r'(if\1', content)

    # Handle standalone "|" → "abs" as a Guage primitive
    # Only when it appears as a complete token: (| expr)
    content = re.sub(r'\(\|(\s)', r'(abs\1', content)

    return content

def replace_in_c_strings(content, mapping):
    """Replace Unicode symbols ONLY inside C string literals.
    This prevents breaking C operators like |, ?, etc."""
    # Strategy: find all string literals, replace symbols within them

    # Build the mapping dict for quick lookup
    mapping_dict = dict(mapping)

    def replace_in_string_literal(match):
        """Replace symbols inside a matched C string literal."""
        s = match.group(0)  # The full "..." string including quotes
        inner = s[1:-1]     # Content without quotes

        # Apply all Unicode replacements (longest first)
        for old, new in mapping:
            if len(old) == 1 and is_all_ascii(old):
                # For single ASCII chars, only replace exact match
                if inner == old:
                    inner = new
            else:
                inner = inner.replace(old, new)

        # Handle "?" → "if" for exact string match
        if inner == "?":
            inner = "if"

        # Handle "|" → "abs" for exact string match
        if inner == "|":
            inner = "abs"

        return '"' + inner + '"'

    # Match C string literals (handling escaped quotes)
    # This regex matches "..." where \" inside is escaped
    result = re.sub(r'"(?:[^"\\]|\\.)*"', replace_in_string_literal, content)
    return result

def replace_intern_preload(content):
    """Replace UTF-8 byte escape sequences in intern_preload with ASCII strings."""
    # Build complete mapping for intern preload
    all_dict = {}
    all_dict.update(SPECIAL_FORMS)
    all_dict.update(STDLIB_MACROS)
    all_dict.update(PRIMITIVES)
    all_dict.update(NIL_MAPPING)
    all_dict.update(ASCII_TOKEN_REPLACEMENTS)

    # Build a mapping from UTF-8 C-escaped sequences to English names
    byte_map = {}
    for unicode_sym, english_name in all_dict.items():
        utf8_bytes = unicode_sym.encode('utf-8')
        # Build the C escape string representation
        c_escape = ""
        for b in utf8_bytes:
            if b < 128 and chr(b).isprintable() and chr(b) != '"' and chr(b) != '\\':
                c_escape += chr(b)
            else:
                c_escape += f"\\x{b:02x}"
        byte_map[c_escape] = english_name

    # Sort by length descending to avoid partial matches
    for c_escape, english in sorted(byte_map.items(), key=lambda x: len(x[0]), reverse=True):
        content = content.replace(f'"{c_escape}"', f'"{english}"')

    return content

def process_file(filepath, mapping, dry_run=False):
    """Process a single file."""
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            original = f.read()
    except (UnicodeDecodeError, FileNotFoundError):
        return 0

    if filepath.endswith('.c') or filepath.endswith('.h'):
        modified = replace_in_c_strings(original, mapping)
    else:
        modified = replace_in_guage_source(original, mapping)

    if original != modified:
        if dry_run:
            print(f"  WOULD MODIFY: {filepath}")
        else:
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(modified)
            print(f"  MODIFIED: {filepath}")
        return 1
    return 0

def main():
    import argparse
    parser = argparse.ArgumentParser(description='Replace Unicode symbols with English')
    parser.add_argument('--c', action='store_true', help='Process C source files')
    parser.add_argument('--guage', action='store_true', help='Process .test/.scm files')
    parser.add_argument('--all', action='store_true', help='Process all files')
    parser.add_argument('--dry-run', action='store_true', help='Show changes without applying')
    parser.add_argument('--intern', action='store_true', help='Process intern.c specially')
    args = parser.parse_args()

    if not (args.c or args.guage or args.all or args.intern):
        parser.print_help()
        sys.exit(1)

    base = os.path.dirname(os.path.abspath(__file__))
    mapping = build_sorted_mapping()
    total = 0

    if args.intern or args.all:
        print("=== Processing intern.c (byte escape sequences) ===")
        intern_path = os.path.join(base, 'intern.c')
        try:
            with open(intern_path, 'r', encoding='utf-8') as f:
                content = f.read()
            content = replace_intern_preload(content)
            if not args.dry_run:
                with open(intern_path, 'w', encoding='utf-8') as f:
                    f.write(content)
                print(f"  MODIFIED: {intern_path}")
            else:
                print(f"  WOULD MODIFY: {intern_path}")
            total += 1
        except FileNotFoundError:
            print(f"  NOT FOUND: {intern_path}")

    if args.c or args.all:
        print("\n=== Processing C source files ===")
        c_files = glob.glob(os.path.join(base, '*.c')) + glob.glob(os.path.join(base, '*.h'))
        for f in sorted(c_files):
            if f.endswith('rename_symbols.py'):
                continue
            total += process_file(f, mapping, args.dry_run)

    if args.guage or args.all:
        print("\n=== Processing test files ===")
        for f in sorted(glob.glob(os.path.join(base, 'tests', '*.test'))):
            total += process_file(f, mapping, args.dry_run)

        print("\n=== Processing stdlib files ===")
        for f in sorted(glob.glob(os.path.join(base, 'stdlib', '*.scm'))):
            total += process_file(f, mapping, args.dry_run)

        print("\n=== Processing example files ===")
        examples = os.path.join(os.path.dirname(base), 'examples')
        for f in sorted(glob.glob(os.path.join(examples, '*.scm'))):
            total += process_file(f, mapping, args.dry_run)

    print(f"\n{'Would modify' if args.dry_run else 'Modified'}: {total} files")

if __name__ == '__main__':
    main()
