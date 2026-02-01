#ifndef GUAGE_PRIMITIVES_H
#define GUAGE_PRIMITIVES_H

#include "cell.h"

/* The 42 Primitives of Guage
 *
 * Core Lambda Calculus (6)
 * Metaprogramming (3)
 * Type Constructors (9) - compile-time only
 * Linear Logic (4) - compile-time only
 * Session Types (5) - compile-time only
 * Effects (4)
 * Refinement Types (4) - compile-time only
 * Actors (3)
 * Comparison & Logic (4)
 */

/* Runtime primitives (evaluated at runtime) */

/* Core Lambda Calculus */
Cell* prim_cons(Cell* args);      /* ⟨ ⟩ - construct cell */
Cell* prim_car(Cell* args);       /* ◁ - head */
Cell* prim_cdr(Cell* args);       /* ▷ - tail */
Cell* prim_lambda(Cell* args);    /* λ - abstraction */
Cell* prim_apply(Cell* args);     /* · - application */

/* Metaprogramming */
Cell* prim_quote(Cell* args);     /* ⌜⌝ - quote */
Cell* prim_eval(Cell* args);      /* ⌞⌟ - eval */
Cell* prim_prim_apply(Cell* args); /* ⊡ - prim-apply (apply primitive to args) */

/* Comparison & Logic */
Cell* prim_equal(Cell* args);     /* ≡ - equality */
Cell* prim_not_equal(Cell* args); /* ≢ - inequality */
/* ∧ and ∨ are now special forms in eval.c (short-circuit + TCO) */
Cell* prim_not(Cell* args);       /* ¬ - logical NOT */

/* Arithmetic (derived, not primitive, but needed) */
Cell* prim_add(Cell* args);       /* ⊕ - addition */
Cell* prim_sub(Cell* args);       /* ⊖ - subtraction */
Cell* prim_mul(Cell* args);       /* ⊗ - multiplication */
Cell* prim_div(Cell* args);       /* ⊘ - division */
Cell* prim_mod(Cell* args);       /* % - modulo */
Cell* prim_lt(Cell* args);        /* < - less than */
Cell* prim_gt(Cell* args);        /* > - greater than */
Cell* prim_le(Cell* args);        /* ≤ - less than or equal */
Cell* prim_ge(Cell* args);        /* ≥ - greater than or equal */

/* Test Runner primitives (Day 123) */
Cell* prim_test_register(Cell* args);      /* ⊨⊕⊙ - register test in trie */
Cell* prim_test_run_registry(Cell* args);  /* ⊨⊕! - run registered tests */
Cell* prim_test_results(Cell* args);       /* ⊨⊜ - return accumulated results */
Cell* prim_test_reset(Cell* args);         /* ⊨⊜∅ - clear test state */
Cell* prim_test_count(Cell* args);         /* ⊨⊜# - return pass/fail/total */
Cell* prim_test_exit(Cell* args);          /* ⊨⊜× - exit with status code */

/* Property-Based Testing Generators */
Cell* prim_gen_int(Cell* args);        /* gen-int - random integer in range */
Cell* prim_gen_bool(Cell* args);       /* gen-bool - random boolean */
Cell* prim_gen_symbol(Cell* args);     /* gen-symbol - random from list */
Cell* prim_gen_list(Cell* args);       /* gen-list - random list */
Cell* prim_test_property(Cell* args);  /* ⊨-prop - property-based test */

/* Type predicates */
Cell* prim_is_number(Cell* args);
Cell* prim_is_bool(Cell* args);
Cell* prim_is_symbol(Cell* args);
Cell* prim_is_nil(Cell* args);
Cell* prim_is_pair(Cell* args);
Cell* prim_is_atom(Cell* args);

/* Effect primitives (⟪, ⟪⟫, ↯, ⟪?, ⟪→ are special forms in eval.c) */
Cell* prim_effect_pure(Cell* args);    /* ⤴ - pure lift */
Cell* prim_effect_bind(Cell* args);    /* ≫ - effect bind */

/* Actor primitives */
Cell* prim_spawn(Cell* args);          /* ⟳ - spawn actor */
Cell* prim_send(Cell* args);           /* →! - send message */
Cell* prim_receive(Cell* args);        /* ←? - receive message */
Cell* prim_actor_run(Cell* args);      /* ⟳! - run scheduler */
Cell* prim_actor_alive(Cell* args);    /* ⟳? - check alive */
Cell* prim_actor_result(Cell* args);   /* ⟳→ - get result */
Cell* prim_actor_reset(Cell* args);    /* ⟳∅ - reset all actors */

/* Channel primitives */
Cell* prim_chan_create(Cell* args);     /* ⟿⊚ - create channel */
Cell* prim_chan_send(Cell* args);       /* ⟿→ - send to channel */
Cell* prim_chan_recv(Cell* args);       /* ⟿← - receive from channel */
Cell* prim_chan_close(Cell* args);      /* ⟿× - close channel */
Cell* prim_chan_reset(Cell* args);      /* ⟿∅ - reset all channels */

/* Documentation primitives */
Cell* prim_doc_get(Cell* args);        /* ⌂ - get documentation */
Cell* prim_doc_type(Cell* args);       /* ⌂∈ - get type signature */
Cell* prim_doc_deps(Cell* args);       /* ⌂≔ - get dependencies */
Cell* prim_doc_source(Cell* args);     /* ⌂⊛ - get source code */

/* Structure primitives - Leaf */
Cell* prim_struct_define_leaf(Cell* args);  /* ⊙≔ - define leaf type */
Cell* prim_struct_create(Cell* args);       /* ⊙ - create struct instance */
Cell* prim_struct_get_field(Cell* args);    /* ⊙→ - get field value */
Cell* prim_struct_update_field(Cell* args); /* ⊙← - update field (immutable) */
Cell* prim_struct_type_check(Cell* args);   /* ⊙? - check structure type */

/* Structure primitives - Node/ADT */
Cell* prim_struct_define_node(Cell* args);  /* ⊚≔ - define node/ADT type */
Cell* prim_struct_create_node(Cell* args);  /* ⊚ - create node instance */
Cell* prim_struct_get_node(Cell* args);     /* ⊚→ - get node field value */
Cell* prim_struct_is_node(Cell* args);      /* ⊚? - check node type and variant */

/* Structure primitives - Graph */
Cell* prim_graph_define(Cell* args);        /* ⊝≔ - define graph type */
Cell* prim_graph_create(Cell* args);        /* ⊝ - create graph instance */
Cell* prim_graph_add_node(Cell* args);      /* ⊝⊕ - add node to graph */
Cell* prim_graph_add_edge(Cell* args);      /* ⊝⊗ - add edge to graph */
Cell* prim_graph_query(Cell* args);         /* ⊝→ - query graph property */
Cell* prim_graph_is(Cell* args);            /* ⊝? - check graph type */

/* I/O primitives - Console */
Cell* prim_print(Cell* args);               /* ≋ - print with newline */
Cell* prim_print_str(Cell* args);           /* ≋≈ - print string */
Cell* prim_read_line(Cell* args);           /* ≋← - read line from stdin */

/* I/O primitives - Files */
Cell* prim_read_file(Cell* args);           /* ≋⊳ - read file */
Cell* prim_write_file(Cell* args);          /* ≋⊲ - write file */
Cell* prim_append_file(Cell* args);         /* ≋⊕ - append to file */
Cell* prim_file_exists(Cell* args);         /* ≋? - file exists */
Cell* prim_file_empty(Cell* args);          /* ≋∅? - file empty */

/* Module System */
Cell* prim_load(Cell* args);                /* ⋘ - load and evaluate file */
Cell* prim_module_import(Cell* args);       /* ⋖ - selective import */
Cell* prim_module_info(Cell* args);         /* ⌂⊚ - module information */
Cell* prim_module_dependencies(Cell* args); /* ⌂⊚→ - module dependencies */

/* Weak reference primitives */
Cell* prim_weak_create(Cell* args);    /* ◇ - create weak reference */
Cell* prim_weak_deref(Cell* args);     /* ◇→ - deref weak ref */
Cell* prim_weak_alive(Cell* args);     /* ◇? - check if alive */
Cell* prim_weak_is(Cell* args);        /* ◇⊙ - type predicate */

/* HashMap primitives (Day 109) */
Cell* prim_hashmap_new(Cell* args);       /* ⊞ - create hashmap */
Cell* prim_hashmap_get(Cell* args);       /* ⊞→ - get value */
Cell* prim_hashmap_put(Cell* args);       /* ⊞← - put key-value */
Cell* prim_hashmap_del(Cell* args);       /* ⊞⊖ - delete key */
Cell* prim_hashmap_is(Cell* args);        /* ⊞? - type predicate */
Cell* prim_hashmap_has(Cell* args);       /* ⊞∋ - has key */
Cell* prim_hashmap_size(Cell* args);      /* ⊞# - size */
Cell* prim_hashmap_keys(Cell* args);      /* ⊞⊙ - keys list */
Cell* prim_hashmap_vals(Cell* args);      /* ⊞⊗ - values list */
Cell* prim_hashmap_entries(Cell* args);   /* ⊞* - entries list */
Cell* prim_hashmap_merge(Cell* args);     /* ⊞⊕ - merge two maps */

/* HashSet primitives (Boost-style groups-of-15 + overflow Bloom byte) */
Cell* prim_set_new(Cell* args);           /* ⊍ - create set */
Cell* prim_set_add(Cell* args);           /* ⊍⊕ - add element */
Cell* prim_set_remove(Cell* args);        /* ⊍⊖ - remove element */
Cell* prim_set_is(Cell* args);            /* ⊍? - type predicate */
Cell* prim_set_has(Cell* args);           /* ⊍∋ - membership test */
Cell* prim_set_size(Cell* args);          /* ⊍# - size */
Cell* prim_set_elements(Cell* args);      /* ⊍⊙ - elements list */
Cell* prim_set_union(Cell* args);         /* ⊍∪ - union */
Cell* prim_set_intersection(Cell* args);  /* ⊍∩ - intersection */
Cell* prim_set_difference(Cell* args);    /* ⊍∖ - difference */
Cell* prim_set_subset(Cell* args);        /* ⊍⊆ - subset test */

/* Deque primitives (Day 111 — DPDK-grade cache-optimized circular buffer) */
Cell* prim_deque_new(Cell* args);          /* ⊟ - create deque */
Cell* prim_deque_push_front(Cell* args);   /* ⊟◁ - push front */
Cell* prim_deque_push_back(Cell* args);    /* ⊟▷ - push back */
Cell* prim_deque_pop_front(Cell* args);    /* ⊟◁⊖ - pop front */
Cell* prim_deque_pop_back(Cell* args);     /* ⊟▷⊖ - pop back */
Cell* prim_deque_peek_front(Cell* args);   /* ⊟◁? - peek front */
Cell* prim_deque_peek_back(Cell* args);    /* ⊟▷? - peek back */
Cell* prim_deque_size(Cell* args);         /* ⊟# - size */
Cell* prim_deque_is(Cell* args);           /* ⊟? - type predicate */
Cell* prim_deque_to_list(Cell* args);      /* ⊟⊙ - to list */
Cell* prim_deque_empty(Cell* args);        /* ⊟∅? - empty predicate */

/* Buffer primitives (Day 113 — cache-line aligned raw byte buffer) */
Cell* prim_buffer_new(Cell* args);          /* ◈ - create buffer */
Cell* prim_buffer_get(Cell* args);          /* ◈← - read byte at index */
Cell* prim_buffer_set(Cell* args);          /* ◈→ - write byte at index */
Cell* prim_buffer_append(Cell* args);       /* ◈⊕ - append byte */
Cell* prim_buffer_concat(Cell* args);       /* ◈⊕⊕ - concat buffers */
Cell* prim_buffer_size(Cell* args);         /* ◈# - byte count */
Cell* prim_buffer_is(Cell* args);           /* ◈? - type predicate */
Cell* prim_buffer_slice(Cell* args);        /* ◈⊂ - slice */
Cell* prim_buffer_to_list(Cell* args);      /* ◈⊙ - bytes as list */
Cell* prim_buffer_to_string(Cell* args);    /* ◈≈ - interpret as UTF-8 */
Cell* prim_buffer_from_string(Cell* args);  /* ≈◈ - string to buffer */

/* Vector primitives (Day 114 — HFT-grade dynamic array with SBO) */
Cell* prim_vector_new(Cell* args);         /* ⟦⟧ - create vector */
Cell* prim_vector_get(Cell* args);         /* ⟦→ - get at index */
Cell* prim_vector_set(Cell* args);         /* ⟦← - set at index (mutates) */
Cell* prim_vector_push(Cell* args);        /* ⟦⊕ - push back (mutates) */
Cell* prim_vector_pop(Cell* args);         /* ⟦⊖ - pop back */
Cell* prim_vector_size(Cell* args);        /* ⟦# - size */
Cell* prim_vector_is(Cell* args);          /* ⟦? - type predicate */
Cell* prim_vector_to_list(Cell* args);     /* ⟦⊙ - to cons list */
Cell* prim_vector_empty(Cell* args);       /* ⟦∅? - empty predicate */
Cell* prim_vector_slice(Cell* args);       /* ⟦⊞ - slice [start,end) */
Cell* prim_vector_map(Cell* args);         /* ⟦↦ - map fn over vector */

/* Heap primitives (Day 115 — 4-ary min-heap priority queue) */
Cell* prim_heap_new(Cell* args);
Cell* prim_heap_push(Cell* args);
Cell* prim_heap_pop(Cell* args);
Cell* prim_heap_peek(Cell* args);
Cell* prim_heap_size(Cell* args);
Cell* prim_heap_is(Cell* args);
Cell* prim_heap_empty(Cell* args);
Cell* prim_heap_to_list(Cell* args);
Cell* prim_heap_merge(Cell* args);

/* Sorted Map primitives (Day 116 — Algorithmica-grade SIMD B-tree) */
Cell* prim_sorted_map_new(Cell* args);
Cell* prim_sorted_map_get(Cell* args);
Cell* prim_sorted_map_put(Cell* args);
Cell* prim_sorted_map_del(Cell* args);
Cell* prim_sorted_map_is(Cell* args);
Cell* prim_sorted_map_has(Cell* args);
Cell* prim_sorted_map_size(Cell* args);
Cell* prim_sorted_map_keys(Cell* args);
Cell* prim_sorted_map_vals(Cell* args);
Cell* prim_sorted_map_entries(Cell* args);
Cell* prim_sorted_map_merge(Cell* args);
Cell* prim_sorted_map_min(Cell* args);
Cell* prim_sorted_map_max(Cell* args);
Cell* prim_sorted_map_range(Cell* args);
Cell* prim_sorted_map_floor(Cell* args);
Cell* prim_sorted_map_ceiling(Cell* args);

/* Trie primitives (Day 117 — ART with SIMD Node16 + path compression) */
Cell* prim_trie_new(Cell* args);
Cell* prim_trie_get(Cell* args);
Cell* prim_trie_put(Cell* args);
Cell* prim_trie_del(Cell* args);
Cell* prim_trie_is(Cell* args);
Cell* prim_trie_has(Cell* args);
Cell* prim_trie_size(Cell* args);
Cell* prim_trie_merge(Cell* args);
Cell* prim_trie_prefix_keys(Cell* args);
Cell* prim_trie_prefix_count(Cell* args);
Cell* prim_trie_longest_prefix(Cell* args);
Cell* prim_trie_entries(Cell* args);
Cell* prim_trie_keys(Cell* args);
Cell* prim_trie_vals(Cell* args);

/* Char/Case primitives (Day 119) */
Cell* prim_str_char_code(Cell* args);    /* ≈→# - char code at index */
Cell* prim_code_to_char(Cell* args);     /* #→≈ - code to single-char string */
Cell* prim_str_upcase(Cell* args);       /* ≈↑ - string to uppercase */
Cell* prim_str_downcase(Cell* args);     /* ≈↓ - string to lowercase */

/* String SDK primitives (Day 121 — SIMD-accelerated complete string API) */
/* Tier 1: Search */
Cell* prim_str_find(Cell* args);         /* ≈⊳ - find substring index */
Cell* prim_str_rfind(Cell* args);        /* ≈⊲ - rfind substring index */
Cell* prim_str_contains(Cell* args);     /* ≈∈? - contains substring */
Cell* prim_str_starts_with(Cell* args);  /* ≈⊲? - starts with prefix */
Cell* prim_str_ends_with(Cell* args);    /* ≈⊳? - ends with suffix */
Cell* prim_str_count(Cell* args);        /* ≈⊳# - count occurrences */
/* Tier 2: Transform */
Cell* prim_str_reverse(Cell* args);      /* ≈⇄ - reverse string */
Cell* prim_str_repeat(Cell* args);       /* ≈⊛ - repeat n times */
Cell* prim_str_replace(Cell* args);      /* ≈⇔ - replace all occurrences */
Cell* prim_str_replacen(Cell* args);     /* ≈⇔# - replace first n occurrences */
/* Tier 3: Trim */
Cell* prim_str_trim_left(Cell* args);    /* ≈⊏ - trim leading whitespace */
Cell* prim_str_trim_right(Cell* args);   /* ≈⊐ - trim trailing whitespace */
Cell* prim_str_trim(Cell* args);         /* ≈⊏⊐ - trim both sides */
/* Tier 4: Split */
Cell* prim_str_split(Cell* args);        /* ≈÷ - split by delimiter */
Cell* prim_str_splitn(Cell* args);       /* ≈÷# - split max n parts */
Cell* prim_str_fields(Cell* args);       /* ≈÷⊔ - split by whitespace runs */
/* Tier 5: Pad */
Cell* prim_str_pad_left(Cell* args);     /* ≈⊏⊕ - pad left to width */
Cell* prim_str_pad_right(Cell* args);    /* ≈⊐⊕ - pad right to width */
/* Tier 6: Strip prefix/suffix */
Cell* prim_str_strip_prefix(Cell* args); /* ≈⊏⊖ - strip prefix if present */
Cell* prim_str_strip_suffix(Cell* args); /* ≈⊐⊖ - strip suffix if present */

/* Iterator primitives (Day 118 — morsel-driven batch iteration) */
Cell* prim_iter(Cell* args);             /* ⊣ - create iterator */
Cell* prim_iter_next(Cell* args);        /* ⊣→ - next element */
Cell* prim_iter_is(Cell* args);          /* ⊣? - type predicate */
Cell* prim_iter_done(Cell* args);        /* ⊣∅? - exhausted check */
Cell* prim_iter_collect(Cell* args);     /* ⊣⊕ - collect to list */
Cell* prim_iter_count(Cell* args);       /* ⊣# - count remaining */
Cell* prim_iter_map(Cell* args);         /* ⊣↦ - lazy map */
Cell* prim_iter_filter(Cell* args);      /* ⊣⊲ - lazy filter */
Cell* prim_iter_take(Cell* args);        /* ⊣↑ - take n */
Cell* prim_iter_drop(Cell* args);        /* ⊣↓ - drop n */
Cell* prim_iter_chain(Cell* args);       /* ⊣⊕⊕ - concatenate */
Cell* prim_iter_zip(Cell* args);         /* ⊣⊗ - zip */
Cell* prim_iter_reduce(Cell* args);      /* ⊣Σ - fold/reduce */
Cell* prim_iter_any(Cell* args);         /* ⊣∃ - any match */
Cell* prim_iter_all(Cell* args);         /* ⊣∀ - all match */
Cell* prim_iter_find(Cell* args);        /* ⊣⊙ - find first */

/* POSIX Port I/O primitives (§3.2) */
Cell* prim_port_open(Cell* args);         /* ⊞⊳ - open file as port */
Cell* prim_fd_to_port(Cell* args);        /* ⊞⊳# - fd to port */
Cell* prim_port_read_line(Cell* args);    /* ⊞← - read line from port */
Cell* prim_port_read_bytes(Cell* args);   /* ⊞←◈ - read N bytes */
Cell* prim_port_read_all(Cell* args);     /* ⊞←* - read all remaining */
Cell* prim_port_write(Cell* args);        /* ⊞→ - write string to port */
Cell* prim_port_write_bytes(Cell* args);  /* ⊞→◈ - write bytes to port */
Cell* prim_port_close(Cell* args);        /* ⊞× - close port */
Cell* prim_port_eof(Cell* args);          /* ⊞∅? - at eof? */
Cell* prim_port_flush(Cell* args);        /* ⊞⊙ - flush port */
Cell* prim_port_stdin(Cell* args);        /* ⊞⊳₀ - stdin port */
Cell* prim_port_stdout(Cell* args);       /* ⊞⊲₀ - stdout port */
Cell* prim_port_stderr(Cell* args);       /* ⊞⊲₁ - stderr port */

/* POSIX File System primitives (§3.3) */
Cell* prim_mkdir(Cell* args);             /* ≋⊙⊕ - create directory */
Cell* prim_rmdir(Cell* args);             /* ≋⊙⊘ - delete directory */
Cell* prim_rename(Cell* args);            /* ≋⇔ - rename file */
Cell* prim_chmod(Cell* args);             /* ≋⊙≔ - set file mode */
Cell* prim_chown(Cell* args);             /* ≋⊙⊕≔ - set file owner */
Cell* prim_utimes(Cell* args);            /* ≋⏱≔ - set file times */
Cell* prim_truncate(Cell* args);          /* ≋⊂ - truncate file */
Cell* prim_link(Cell* args);              /* ≋⊕ - create hard link */
Cell* prim_symlink(Cell* args);           /* ≋⊕→ - create symlink */
Cell* prim_readlink(Cell* args);          /* ≋→ - read symlink */
Cell* prim_mkfifo(Cell* args);            /* ≋⊙⊕⊞ - create FIFO */
Cell* prim_file_info(Cell* args);         /* ≋⊙ - stat file */
Cell* prim_directory_files(Cell* args);   /* ≋⊙* - list directory */
Cell* prim_opendir(Cell* args);           /* ≋⊙⊳ - open directory */
Cell* prim_readdir(Cell* args);           /* ≋⊙← - read directory entry */
Cell* prim_closedir_prim(Cell* args);     /* ≋⊙× - close directory */
Cell* prim_directory_generator(Cell* args); /* ≋⊙⊣ - directory generator */
Cell* prim_realpath(Cell* args);          /* ≋⊙⊕→ - resolve path */
Cell* prim_file_space(Cell* args);        /* ≋⊙# - filesystem space */
Cell* prim_create_temp_file(Cell* args);  /* ≋⊙⏱ - create temp file */
Cell* prim_delete_file(Cell* args);       /* ≋⊖ - delete/unlink file */

/* POSIX Process State primitives (§3.5) */
Cell* prim_umask_get(Cell* args);         /* ⊙⌂⊙ - get umask */
Cell* prim_umask_set(Cell* args);         /* ⊙⌂⊙≔ - set umask */
Cell* prim_cwd(Cell* args);              /* ⊙⌂⊘ - current directory */
Cell* prim_chdir(Cell* args);            /* ⊙⌂⊘≔ - change directory */
Cell* prim_pid(Cell* args);              /* ⊙⌂# - process id */
Cell* prim_nice(Cell* args);             /* ⊙⌂△ - adjust priority */
Cell* prim_uid(Cell* args);              /* ⊙⌂⊕ - user id */
Cell* prim_gid(Cell* args);              /* ⊙⌂⊕⊕ - group id */
Cell* prim_euid(Cell* args);             /* ⊙⌂⊕* - effective uid */
Cell* prim_egid(Cell* args);             /* ⊙⌂⊕⊕* - effective gid */
Cell* prim_groups(Cell* args);           /* ⊙⌂⊕⊕*⊕ - supplementary gids */

/* POSIX User/Group Database primitives (§3.6) */
Cell* prim_user_info(Cell* args);        /* ⊙⌂⊕⊙ - user info */
Cell* prim_group_info(Cell* args);       /* ⊙⌂⊕⊕⊙ - group info */

/* POSIX Time primitives (§3.10) */
Cell* prim_posix_time(Cell* args);       /* ⊙⏱ - posix time */
Cell* prim_monotonic_time(Cell* args);   /* ⊙⏱⊕ - monotonic time */

/* POSIX Environment primitives (§3.11) */
Cell* prim_getenv(Cell* args);           /* ⊙⌂≋ - get env var */
Cell* prim_setenv(Cell* args);           /* ⊙⌂≋≔ - set env var */
Cell* prim_unsetenv(Cell* args);         /* ⊙⌂≋⊘ - delete env var */

/* POSIX Terminal primitive (§3.12) */
Cell* prim_is_terminal(Cell* args);      /* ⊞⊙? - is port a terminal? */

/* R7RS System Extras */
Cell* prim_argv(Cell* args);             /* ⊙⌂ - command line args */
Cell* prim_exit_process(Cell* args);     /* ⊙⊘ - exit */
Cell* prim_current_second(Cell* args);   /* ⊙⏱≈ - seconds since epoch */
Cell* prim_jiffy(Cell* args);            /* ⊙⏱⊕# - high-res counter */
Cell* prim_jps(Cell* args);              /* ⊙⏱⊕≈ - jiffies per second */

/* Error chain primitives */
Cell* prim_error_wrap(Cell* args);       /* ⚡⊕ - wrap error with context */
Cell* prim_error_cause(Cell* args);      /* ⚠⊸ - get error cause */
Cell* prim_error_root_cause(Cell* args); /* ⚠⊸* - get root cause */
Cell* prim_error_trace(Cell* args);      /* ⚠⟲ - get return trace as list */
Cell* prim_error_chain_match(Cell* args);/* ⚠⊙? - check if error chain matches type */

/* FFI primitives (Day 125 — JIT-compiled stubs) */
Cell* prim_ffi_dlopen(Cell* args);      /* ⌁⊳ - load shared library */
Cell* prim_ffi_dlclose(Cell* args);     /* ⌁× - close library handle */
Cell* prim_ffi_bind(Cell* args);        /* ⌁→ - bind function (JIT stub) */
Cell* prim_ffi_call(Cell* args);        /* ⌁! - call bound FFI function */
Cell* prim_ffi_is(Cell* args);          /* ⌁? - FFI_PTR type predicate */
Cell* prim_ffi_type_tag(Cell* args);    /* ⌁⊙ - get FFI_PTR type tag */
Cell* prim_ffi_wrap(Cell* args);        /* ⌁⊞ - wrap address as FFI_PTR */
Cell* prim_ffi_wrap_fin(Cell* args);    /* ⌁⊞× - wrap with finalizer */
Cell* prim_ffi_null(Cell* args);        /* ⌁∅ - NULL pointer constant */
Cell* prim_ffi_null_p(Cell* args);      /* ⌁∅? - test if NULL */
Cell* prim_ffi_addr(Cell* args);        /* ⌁# - get address as number */
Cell* prim_ffi_read_cstr(Cell* args);   /* ⌁≈→ - read C string from ptr */
Cell* prim_ffi_str_to_ptr(Cell* args);  /* ⌁→≈ - string to strdup'd ptr */
Cell* prim_ffi_read_buf(Cell* args);    /* ⌁◈→ - read N bytes from ptr */
Cell* prim_ffi_buf_to_ptr(Cell* args);  /* ⌁→◈ - buffer to malloc'd ptr */

/* Networking primitives (Day 126 — HFT-grade io_uring/kqueue/IOCP) */

/* Socket lifecycle (5) */
Cell* prim_net_socket(Cell* args);      /* ⊸⊕ - create socket */
Cell* prim_net_close(Cell* args);       /* ⊸× - close socket */
Cell* prim_net_shutdown(Cell* args);    /* ⊸×→ - shutdown socket */
Cell* prim_net_socketpair(Cell* args);  /* ⊸⊕⊞ - connected pair */
Cell* prim_net_is_socket(Cell* args);   /* ⊸? - socket predicate */

/* Address construction (3) */
Cell* prim_net_addr(Cell* args);        /* ⊸⊙ - IPv4 address */
Cell* prim_net_addr6(Cell* args);       /* ⊸⊙₆ - IPv6 address */
Cell* prim_net_addr_unix(Cell* args);   /* ⊸⊙⊘ - Unix domain address */

/* Synchronous client/server (5) */
Cell* prim_net_connect(Cell* args);     /* ⊸→⊕ - sync connect */
Cell* prim_net_bind(Cell* args);        /* ⊸←≔ - bind to address */
Cell* prim_net_listen(Cell* args);      /* ⊸←⊕ - listen(backlog) */
Cell* prim_net_accept(Cell* args);      /* ⊸← - sync accept */
Cell* prim_net_resolve(Cell* args);     /* ⊸⊙→ - DNS resolve */

/* Synchronous I/O (4) */
Cell* prim_net_send(Cell* args);        /* ⊸→ - send data */
Cell* prim_net_recv(Cell* args);        /* ⊸←◈ - receive data */
Cell* prim_net_sendto(Cell* args);      /* ⊸→⊙ - UDP sendto */
Cell* prim_net_recvfrom(Cell* args);    /* ⊸←⊙ - UDP recvfrom */

/* Socket options (3) */
Cell* prim_net_setsockopt(Cell* args);  /* ⊸≔ - set socket option */
Cell* prim_net_getsockopt(Cell* args);  /* ⊸≔→ - get socket option */
Cell* prim_net_peername(Cell* args);    /* ⊸# - getpeername */

/* Event ring lifecycle (3) */
Cell* prim_ring_create(Cell* args);     /* ⊸⊚⊕ - create ring */
Cell* prim_ring_destroy(Cell* args);    /* ⊸⊚× - destroy ring */
Cell* prim_ring_is(Cell* args);         /* ⊸⊚? - ring predicate */

/* Buffer pool (4) */
Cell* prim_ring_buf_create(Cell* args); /* ⊸⊚◈⊕ - create buffer pool */
Cell* prim_ring_buf_destroy(Cell* args);/* ⊸⊚◈× - destroy buffer pool */
Cell* prim_ring_buf_get(Cell* args);    /* ⊸⊚◈→ - get buffer by ID */
Cell* prim_ring_buf_return(Cell* args); /* ⊸⊚◈← - return buffer */

/* Async ring operations (8) */
Cell* prim_ring_accept(Cell* args);     /* ⊸⊚← - async accept */
Cell* prim_ring_recv(Cell* args);       /* ⊸⊚←◈ - async recv */
Cell* prim_ring_send(Cell* args);       /* ⊸⊚→ - async send */
Cell* prim_ring_send_zc(Cell* args);    /* ⊸⊚→∅ - zero-copy send */
Cell* prim_ring_connect(Cell* args);    /* ⊸⊚→⊕ - async connect */
Cell* prim_ring_close(Cell* args);      /* ⊸⊚→× - async close */
Cell* prim_ring_submit(Cell* args);     /* ⊸⊚! - flush submissions */
Cell* prim_ring_complete(Cell* args);   /* ⊸⊚⊲ - harvest completions */

/* Execution Trace primitives (Day 136 — HFT-grade execution tracing) */
Cell* prim_trace_enable(Cell* args);     /* ⟳⊳⊳! - enable/disable tracing */
Cell* prim_trace_read(Cell* args);       /* ⟳⊳⊳? - read trace events */
Cell* prim_trace_clear(Cell* args);      /* ⟳⊳⊳∅ - clear trace buffer */
Cell* prim_trace_count(Cell* args);      /* ⟳⊳⊳# - count trace events */
Cell* prim_trace_snapshot(Cell* args);   /* ⟳⊳⊳⊛ - flight recorder snapshot */
Cell* prim_trace_causal(Cell* args);     /* ⟳⊳⊳⊗ - enable causal tracing */
Cell* prim_trace_capacity(Cell* args);   /* ⟳⊳⊳⊞ - buffer capacity */

/* Refinement Type primitives (Day 127 — HFT-grade gradual dependent types) */
Cell* prim_refine_def(Cell* args);        /* ∈⊡ - define refinement type (special form) */
Cell* prim_refine_check(Cell* args);      /* ∈⊡? - check value against refinement */
Cell* prim_refine_assert(Cell* args);     /* ∈⊡! - assert value satisfies refinement */
Cell* prim_refine_base(Cell* args);       /* ∈⊡⊙ - get base type of refinement */
Cell* prim_refine_pred(Cell* args);       /* ∈⊡→ - get predicate lambda */
Cell* prim_refine_constraint(Cell* args); /* ∈⊡⊢ - get constraint tree as value */
Cell* prim_refine_intersect(Cell* args);  /* ∈⊡∧ - intersect two refinements */
Cell* prim_refine_union(Cell* args);      /* ∈⊡∨ - union two refinements */
Cell* prim_refine_list(Cell* args);       /* ∈⊡∀ - list all refinements */
Cell* prim_refine_find(Cell* args);       /* ∈⊡∈ - find matching refinement */
Cell* prim_refine_subtype(Cell* args);    /* ∈⊡⊆ - refinement subtype check */

/* Primitive documentation structure */
typedef struct {
    const char* description;     /* What this primitive does */
    const char* type_signature;  /* Type signature */
} PrimitiveDoc;

/* Environment for primitives */
typedef struct {
    const char* name;
    Cell* (*fn)(Cell*);
    int arity;                   /* Number of arguments (-1 for variadic) */
    PrimitiveDoc doc;            /* MANDATORY documentation */
} Primitive;

/* Initialize primitive environment */
Cell* primitives_init(void);

/* Lookup primitive by symbol */
Cell* primitives_lookup(Cell* env, const char* sym);

/* Lookup primitive by name (returns NULL if not found) */
const Primitive* primitive_lookup_by_name(const char* name);

#endif /* GUAGE_PRIMITIVES_H */
