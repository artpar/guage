---
Status: CURRENT
Created: 2026-01-27
Updated: 2026-01-30 (Day 110 COMPLETE)
Purpose: Current project status and progress
---

# Session Handoff: Day 110 - First-Class HashSet (2026-01-30)

## Day 110 Progress - HashSet (`⊍`) — Boost-Style Groups-of-15 + Overflow Bloom Byte

**RESULT:** 108/108 test files passing (100%), 10 new tests (HashSet)

### New Feature: First-Class HashSet (⊍) — Boost-Style SOTA Design

Production-grade hash set using Boost `unordered_flat_set` design (groups-of-15 with overflow Bloom byte). Chosen after benchmarking Swiss Table, F14 (Meta), Boost, and Elastic Hashing — Boost wins with 3.2x faster miss lookups (critical for set membership) and tombstone-free deletion.

**New Cell Type:** `CELL_SET` (enum 17) — printed as `⊍[N]`

**New Primitives (11):**
- `⊍` (set-new, variadic) — `(⊍)` → empty set, `(⊍ #1 #2 #3)` → set from values
- `⊍⊕` (set-add) — `(⊍⊕ s val)` → `#t` (new) / `#f` (existed), mutates
- `⊍⊖` (set-remove) — `(⊍⊖ s val)` → `#t` (found) / `#f` (absent)
- `⊍?` (set-is) — Type predicate
- `⊍∋` (set-has) — `(⊍∋ s val)` → `#t`/`#f` membership test
- `⊍#` (set-size) — O(1) size query
- `⊍⊙` (set-elements) — All elements as list
- `⊍∪` (set-union) — New set from s1 ∪ s2
- `⊍∩` (set-intersection) — New set from s1 ∩ s2
- `⊍∖` (set-difference) — New set s1 \ s2
- `⊍⊆` (set-subset) — `(⊍⊆ s1 s2)` → `#t` if s1 ⊆ s2

**Architecture:**
- **Boost groups-of-15**: 15 tag bytes + 1 overflow Bloom byte = 16-byte metadata word (fits SIMD register)
- **Overflow Bloom byte**: bit `(hash % 8)` set when element displaced past its home group → O(1) miss termination (3.2x faster than Swiss Table)
- **Tombstone-free deletion**: Clear tag to EMPTY without modifying overflow bits; stale bits cleaned on rehash
- **Tag encoding**: `0x00=EMPTY`, `0x01=SENTINEL`, `0x02..0xFF=occupied` (reduced hash from top 8 bits)
- **Load factor**: 86.7% (13 elements per group of 15)
- **SIMD reuse**: Existing `swisstable.h` (SSE2/NEON/SWAR) with `& 0x7FFF` mask to exclude overflow byte
- **Hash reuse**: SipHash-2-4 via `cell_hash()`
- **Probing**: Triangular between groups (same as HashMap)

**Files Modified (4) + 1 New:**
- `bootstrap/cell.h` — `CELL_SET` in enum, `hashset` struct in union, 11 function declarations
- `bootstrap/cell.c` — Constructor, Boost-style find/insert/resize/delete, set ops (union/intersection/difference/subset), lifecycle (~300 lines)
- `bootstrap/primitives.h` — 11 hashset primitive declarations
- `bootstrap/primitives.c` — 11 primitive implementations + table entries + typeof updates (prim_type_of + prim_typeof)
- `bootstrap/tests/test_set.test` (NEW) — 10 tests

---

## Day 109 Progress - HashMap (`⊞`) — Swiss Table with Portable SIMD + SipHash-2-4

**RESULT:** 107/107 test files passing (100%), 10 new tests (HashMap)

### New Feature: First-Class HashMap (⊞) — Swiss Table with SIMD

Production-grade hash table using Swiss Table (Google Abseil design) with SipHash-2-4 keyed PRF for HashDoS resistance. Three-tier portable SIMD: SSE2 (x86) → NEON (ARM64) → SWAR (fallback).

**New Cell Type:** `CELL_HASHMAP` (enum 16) — printed as `⊞[N]`

**New Primitives (11):**
- `⊞` (hashmap-new, variadic) — `(⊞)` → empty map, `(⊞ (⟨⟩ :a #1) ...)` → map from pairs
- `⊞→` (hashmap-get) — `(⊞→ m key)` → value or `∅`
- `⊞←` (hashmap-put) — `(⊞← m key value)` → old value or `∅` (mutates in place)
- `⊞⊖` (hashmap-del) — `(⊞⊖ m key)` → old value or `∅`
- `⊞?` (hashmap-is) — Type predicate
- `⊞∋` (hashmap-has) — `(⊞∋ m key)` → `#t`/`#f`
- `⊞#` (hashmap-size) — O(1) size query
- `⊞⊙` (hashmap-keys) — List of keys
- `⊞⊗` (hashmap-vals) — List of values
- `⊞*` (hashmap-entries) — List of `⟨key value⟩` pairs
- `⊞⊕` (hashmap-merge) — New map from m1 + m2 (m2 wins conflicts)

**Architecture:**
- **SipHash-2-4**: 128-bit random key initialized at startup via `arc4random_buf` (macOS) / `/dev/urandom` (Linux)
- **Swiss Table**: Separate control byte metadata array + slot array, group-based probing (16 slots per SIMD op)
- **Control bytes**: 0xFF=EMPTY, 0x80=DELETED, 0b0xxxxxxx=FULL (H2 hash fragment)
- **Probing**: Triangular sequence covers all groups when capacity is power of 2
- **Growth**: 2x at 87.5% load factor, power-of-2 capacity
- **Mirrored control bytes**: First GROUP_WIDTH bytes duplicated at end for unaligned SIMD loads

**Files Modified (5) + 3 New:**
- `bootstrap/siphash.h` (NEW) — Header-only SipHash-2-4 (~110 lines)
- `bootstrap/swisstable.h` (NEW) — Portable SIMD abstraction: SSE2/NEON/SWAR (~180 lines)
- `bootstrap/cell.h` — `CELL_HASHMAP` in enum, `HashSlot` typedef, hashmap struct in union, 12 function declarations
- `bootstrap/cell.c` — `cell_hash()`, constructor, Swiss Table core (find/insert/resize/delete), iteration, merge, lifecycle (~300 lines)
- `bootstrap/primitives.h` — 11 hashmap primitive declarations
- `bootstrap/primitives.c` — 11 primitive implementations + table entries + typeof update (~120 lines)
- `bootstrap/main.c` — `guage_siphash_init()` call at startup
- `bootstrap/tests/test_hashmap.test` (NEW) — 10 tests

---

## Day 108 Progress - Weak References (`◇`, `◇→`, `◇?`, `◇⊙`)

**RESULT:** 106/106 test files passing (100%), 10 new tests (Weak References)

### New Feature: Weak References (◇) — Intrusive Dual-Count Zombie Approach

Weak references allow observing a cell without preventing its collection. Uses Swift pre-4 style zombie approach: when strong refcount→0 but weak_refcount>0, children are released but the cell shell persists for O(1) liveness checks.

**New Cell Type:** `CELL_WEAK_REF` — printed as `◇[alive]` or `◇[dead]`

**New Primitives (4):**
- `◇` (weak-create) — Create weak reference: `(◇ target)` → `◇[alive]`
- `◇→` (weak-deref) — Deref, returns ∅ if dead: `(◇→ w)` → target or `∅`
- `◇?` (weak-alive) — Check liveness without retaining: `(◇? w)` → `#t`/`#f`
- `◇⊙` (weak-is) — Type predicate: `(◇⊙ w)` → `#t`

**Semantics:**
- `◇→` retains the returned target (caller gets a strong ref, preventing collection during use)
- `◇?` is pure observation (no retain, zero side effects)
- Zombie memory: only cell shell (~100 bytes) persists; children released immediately on refcount→0
- `uint16_t weak_refcount` added to Cell struct (2 bytes overhead per cell, zero when unused)
- Single branch in `cell_release`: if `weak_refcount > 0` after releasing children, don't free shell

**Files Modified (4) + 1 New:**
- `bootstrap/cell.h` — `CELL_WEAK_REF` in enum, `weak_refcount` field, `weak_ref` struct in union, function declarations
- `bootstrap/cell.c` — Constructor, weak_retain/release, zombie logic in cell_release, print, equality
- `bootstrap/primitives.h` — 4 weak ref primitive declarations
- `bootstrap/primitives.c` — 4 primitive functions + table entries + typeof updates (prim_type_of + prim_typeof)
- `bootstrap/tests/test_weak.test` (NEW) — 10 tests

---

## Day 107 Progress - Mutable References (`□`, `□→`, `□←`, `□?`, `□⊕`, `□⇌`) + Sequencing (`⪢`)

**RESULT:** 105/105 test files passing (100%), 10 new tests (Mutable References + Sequencing)

### New Feature 1: Mutable References (□) — First-Class Mutable Containers

Boxes are first-class mutable containers usable anywhere (not actor-only). They hold a single mutable value with create/deref/set/update/swap operations.

**New Cell Type:** `CELL_BOX` — printed as `□[value]`

**New Primitives (6):**
- `□` (box) — Create mutable box: `(□ #42)` → `□[#42]`
- `□→` (deref) — Read box value: `(□→ b)` → current value
- `□←` (set!) — Set box, return old: `(□← b #99)` → previous value
- `□?` (box?) — Type predicate: `(□? b)` → `#t`
- `□⊕` (update!) — Apply fn, store result, return old: `(□⊕ b (λ (x) (⊗ x #2)))` → old value
- `□⇌` (swap) — Swap two boxes' contents: `(□⇌ b1 b2)` → `#t`

**Semantics:**
- `□←` returns old value (useful for CAS-like patterns)
- `□⊕` is atomic get-and-update: returns old, stores `(fn old)`
- Equality is identity-only (two boxes are never `≡` unless same object)
- Refcount protocol: `cell_box_set` retains new, returns old without releasing (caller owns old ref)

### New Feature 2: Sequencing (⪢) — Multi-Expression Evaluation

**New Special Form (1):**
- `⪢` (seq) — Evaluate all expressions, return last: `(⪢ e1 e2 ... en)` → `en`

**Semantics:**
- Last expression in tail position (TCO via `goto tail_call`)
- Intermediate results properly released
- Errors in intermediate expressions short-circuit
- Requires at least 1 expression

**Files Modified (5):**
- `bootstrap/cell.h` — `CELL_BOX` in enum, `box` struct in union, function declarations
- `bootstrap/cell.c` — Constructor, release, predicate, getter, setter, equality (identity), print
- `bootstrap/primitives.c` — 6 primitive functions + `box_call_fn` helper + table entries + typeof updates
- `bootstrap/eval.c` — `⪢` special form with TCO tail position
- `bootstrap/tests/test_box.test` (NEW) — 10 tests

---

## Day 106 Progress - Flow Registry (`⟳⊸⊜⊕`, `⟳⊸⊜?`, `⟳⊸⊜⊖`, `⟳⊸⊜*`)

**RESULT:** 104/104 test files passing (100%), 10 new tests (Flow Registry)

### New Feature: Flow Registry — Named Flow Pipelines

Flow Registry allows naming flow pipelines for later lookup, mirroring the process registry pattern. Flows can be registered under symbol names, looked up by name, unregistered, and listed.

**New Primitives (4):**
- `⟳⊸⊜⊕` (flow-register) — Register flow under name: `(⟳⊸⊜⊕ :name flow-id)` → `#t` or `⚠`
- `⟳⊸⊜?` (flow-whereis) — Look up flow by name: `(⟳⊸⊜? :name)` → flow-id or `∅`
- `⟳⊸⊜⊖` (flow-unregister) — Remove name: `(⟳⊸⊜⊖ :name)` → `#t` or `⚠`
- `⟳⊸⊜*` (flow-registered) — List all names: `(⟳⊸⊜*)` → `[:symbol]`

**Semantics:**
- Names are symbols (`:my-pipeline`, `:doubler`, etc.)
- One name → one flow, one flow → one name (no duplicates)
- Whereis on unregistered name returns `∅` (not error)
- Unregister on unknown name returns error
- Register validates both name (must be symbol) and flow-id (must be number pointing to active flow)
- `⟳∅` (reset) clears flow registry for test isolation
- Max 256 registered flow names

**Files Modified (3):**
- `bootstrap/actor.h` — Flow registry API declarations, `MAX_FLOW_REGISTRY`
- `bootstrap/actor.c` — Flow registry implementation (parallel arrays), `flow_registry_register/lookup/unregister_name/list/reset`; `actor_reset_all` calls `flow_registry_reset`
- `bootstrap/primitives.c` — 4 new primitive functions + registration in table

**New Test File (1):**
- `bootstrap/tests/test_flow_registry.test` — 10 tests: flow-register-basic, flow-whereis-unregistered, flow-unregister-basic, flow-register-duplicate-name, flow-register-duplicate-flow, flow-registered-list-count, flow-register-not-symbol, flow-register-not-number, flow-run-by-name, flow-unregister-not-found

---

## Day 105 Progress - Flow (`⟳⊸`, `⟳⊸↦`, `⟳⊸⊲`, `⟳⊸⊕`, `⟳⊸⊙`, `⟳⊸!`)

**RESULT:** 103/103 test files passing (100%), 10 new tests (Flow)

### New Feature: Flow — Lazy Computation Pipelines

Flow provides lazy, composable data processing pipelines. Steps accumulate without executing until `⟳⊸!` (run) is called. Supports map, filter, reduce, and side-effect (each) operations that can be chained in any order.

**New Primitives (6):**
- `⟳⊸` (flow-from) — Create flow from list: `(⟳⊸ list)` → flow-id
- `⟳⊸↦` (flow-map) — Add map step: `(⟳⊸↦ flow-id fn)` → flow-id
- `⟳⊸⊲` (flow-filter) — Add filter step: `(⟳⊸⊲ flow-id pred)` → flow-id
- `⟳⊸⊕` (flow-reduce) — Add reduce step: `(⟳⊸⊕ flow-id init fn)` → flow-id
- `⟳⊸⊙` (flow-each) — Add side-effect step: `(⟳⊸⊙ flow-id fn)` → flow-id
- `⟳⊸!` (flow-run) — Execute flow pipeline: `(⟳⊸! flow-id)` → result

**Semantics:**
- Flows are lazy — steps accumulate until `⟳⊸!` executes the pipeline
- Map applies function to each element, produces new list
- Filter keeps elements where predicate returns truthy
- Reduce folds with init value and 2-arg function, produces single value
- Each calls function for side-effects, produces `∅`
- Steps chain in order: map→filter→reduce composes naturally
- Empty list source produces empty results
- `⟳∅` (reset) clears all flows for test isolation

**Files Modified (3):**
- `bootstrap/actor.h` — Flow struct, FlowStep struct, FlowStepType enum, MAX_FLOWS/MAX_FLOW_STEPS, flow API declarations
- `bootstrap/actor.c` — Flow implementation (global array), flow_create/lookup/add_step/reset_all; actor_reset_all calls flow_reset_all
- `bootstrap/primitives.c` — 6 new primitive functions + flow_call_fn1/flow_call_fn2 helpers + registration

**New Test File (1):**
- `bootstrap/tests/test_flow.test` — 10 tests: flow-from-list, flow-map-basic, flow-filter-basic, flow-map-filter-chain, flow-reduce-sum, flow-each-basic, flow-empty-list, flow-map-filter-reduce, flow-multiple-maps, flow-filter-none-match

---

## Day 104 Progress - DynamicSupervisor (`⟳⊛⊹`, `⟳⊛⊹⊕`, `⟳⊛⊹⊖`, `⟳⊛⊹?`, `⟳⊛⊹#`)

**RESULT:** 102/102 test files passing (100%), 10 new tests (DynamicSupervisor)

### New Feature: DynamicSupervisor — On-Demand Child Spawning with Restart Types

DynamicSupervisor is a specialized supervisor that starts empty and allows children to be added on demand, each with a per-child restart type. Unlike regular supervisors which start with a fixed child spec list, DynamicSupervisor is designed for dynamically-spawned, short-lived or long-lived workers. Always uses one-for-one strategy.

**New Primitives (5):**
- `⟳⊛⊹` (dynsup-start) — Create empty dynamic supervisor: `(⟳⊛⊹)` → supervisor-id
- `⟳⊛⊹⊕` (dynsup-start-child) — Start child with restart type: `(⟳⊛⊹⊕ sup-id behavior :type)` → actor-cell
- `⟳⊛⊹⊖` (dynsup-terminate-child) — Terminate child: `(⟳⊛⊹⊖ sup-id child)` → `#t`
- `⟳⊛⊹?` (dynsup-which-children) — List children: `(⟳⊛⊹? sup-id)` → `[⟨⟳ :type⟩]`
- `⟳⊛⊹#` (dynsup-count) — Count children: `(⟳⊛⊹# sup-id)` → count

**Per-Child Restart Types:**
- `:permanent` — Always restart on any exit (error or normal)
- `:transient` — Restart only on error exit; normal exit removes child
- `:temporary` — Never restart; removed on any exit

**Semantics:**
- Dynamic supervisors start with no children
- Always one-for-one strategy (each child independent)
- `⟳⊛⊹⊕` returns an actor cell (not a number) for direct `→!` usage
- Error exit: permanent/transient restart, temporary removed
- Normal exit: transient/temporary removed, permanent stays (but not restarted since it exited normally)
- Child removal shifts arrays to maintain order
- Reuses Supervisor struct with `is_dynamic` flag and per-child `child_restart[]` array
- `⟳∅` (reset) clears all dynamic supervisors (via existing supervisor cleanup)

**Files Modified (3):**
- `bootstrap/actor.h` — Added `ChildRestartType` enum, `child_restart[]` and `is_dynamic` fields to Supervisor
- `bootstrap/actor.c` — Added `dynsup_remove_child_at()`, modified `supervisor_handle_exit()` to check restart types, modified `actor_notify_exit()` to handle normal exits for dynamic supervisors
- `bootstrap/primitives.c` — 5 new primitive functions + registration in table

**New Test File (1):**
- `bootstrap/tests/test_dynsup.test` — 10 tests: dynsup-start-basic, dynsup-count-empty, dynsup-start-child-permanent, dynsup-which-children, dynsup-terminate-child, dynsup-permanent-restarts, dynsup-temporary-no-restart, dynsup-transient-normal-no-restart, dynsup-transient-error-restarts, dynsup-multiple-children

---

## Day 103 Progress - GenStage (`⟳⊵`, `⟳⊵⊕`, `⟳⊵→`, `⟳⊵⊙`, `⟳⊵?`, `⟳⊵×`)

**RESULT:** 101/101 test files passing (100%), 10 new tests (GenStage)

### New Feature: GenStage — Demand-Driven Producer-Consumer Pipelines

GenStage provides a framework for building data processing pipelines with back-pressure. Stages are stateful C-side entities (like Agents) with three modes: producers generate events on demand, consumers process events, and producer-consumers transform events and forward output downstream. Subscriptions connect stages into pipelines.

**New Primitives (6):**
- `⟳⊵` (stage-new) — Create stage: `(⟳⊵ :producer handler init-state)` → stage-id
- `⟳⊵⊕` (stage-subscribe) — Subscribe downstream to upstream: `(⟳⊵⊕ consumer-id producer-id)` → `#t`
- `⟳⊵→` (stage-ask) — Request events from producer: `(⟳⊵→ stage-id demand)` → events list
- `⟳⊵⊙` (stage-dispatch) — Dispatch events into stage pipeline: `(⟳⊵⊙ stage-id events)` → dispatched count
- `⟳⊵?` (stage-info) — Get stage info: `(⟳⊵? stage-id)` → `⟨:mode state⟩`
- `⟳⊵×` (stage-stop) — Stop stage: `(⟳⊵× stage-id)` → final state

**Stage Modes:**
- `:producer` — Handler: `(λ (demand state) ⟨events new-state⟩)` — generates events
- `:consumer` — Handler: `(λ (events state) new-state)` — processes events
- `:producer-consumer` — Handler: `(λ (events state) ⟨out-events new-state⟩)` — transforms events

**Semantics:**
- Stages are pure C-side state (like Agents), not actors
- Ask calls producer handler with demand + state, returns events, updates state
- Dispatch sends events into a stage: producers forward to subscribers, consumers process, producer-consumers process then forward output to their subscribers
- Subscriptions form directed pipelines — producer-consumer auto-forwards to downstream
- `⟳∅` (reset) clears all stages for test isolation
- Max 64 stages, 16 subscribers per stage

**Files Modified (3):**
- `bootstrap/actor.h` — GenStage struct, StageMode enum, MAX_STAGES/MAX_STAGE_SUBSCRIBERS, stage API declarations
- `bootstrap/actor.c` — GenStage implementation (global array), stage_create/lookup/subscribe/stop/reset_all; actor_reset_all calls stage_reset_all
- `bootstrap/primitives.c` — 6 new primitive functions + stage_call_fn2 helper + stage_dispatch_to_subscribers + registration

**New Test File (1):**
- `bootstrap/tests/test_genstage.test` — 10 tests: stage-new-producer, stage-new-consumer, stage-ask-producer, stage-ask-updates-state, stage-subscribe-basic, stage-dispatch, stage-info, stage-stop, stage-stop-error, stage-producer-consumer

---

## Day 102 Progress - Agent (`⟳⊶`, `⟳⊶?`, `⟳⊶!`, `⟳⊶⊕`, `⟳⊶×`)

**RESULT:** 100/100 test files passing (100%), 10 new tests (Agent)

### New Feature: Agent — Functional State Wrapper

Agents are simple state containers with a functional interface for get/update operations. Inspired by Elixir's Agent module. An agent holds a single state value, initialized from a zero-arg function. State is read via getter functions and modified via updater functions, providing clean functional state management without needing a full GenServer actor loop.

**New Primitives (5):**
- `⟳⊶` (agent-start) — Create agent with initial state: `(⟳⊶ (λ () init))` → agent-id (number)
- `⟳⊶?` (agent-get) — Read state via function: `(⟳⊶? id (λ (s) s))` → `(fn state)`
- `⟳⊶!` (agent-update) — Update state via function: `(⟳⊶! id (λ (s) (⊕ s #1)))` → `#t`
- `⟳⊶⊕` (agent-get-and-update) — Atomic get+update: `(⟳⊶⊕ id (λ (s) (⟨⟩ s new)))` → reply
- `⟳⊶×` (agent-stop) — Stop agent, return final state: `(⟳⊶× id)` → state

**Semantics:**
- Agent ID is a number (not an actor cell — agents are pure C-side state)
- Init function is zero-arg, called immediately to produce initial state
- Get applies getter to current state, returns result without modifying state
- Update applies updater to current state, stores result as new state
- Get-and-update: fn returns `⟨reply new-state⟩` pair — reply returned, state updated
- Stop deactivates agent, releases state, returns final state value
- Operations on stopped/invalid agent return error
- `⟳∅` (reset) clears all agents for test isolation
- Max 64 concurrent agents

**Files Modified (3):**
- `bootstrap/actor.h` — AgentState struct, MAX_AGENTS, agent API declarations
- `bootstrap/actor.c` — Agent implementation (global array), reset integration with `actor_reset_all`
- `bootstrap/primitives.c` — 5 new primitive functions + helper for calling lambdas via temp defines + registration

**New Test File (1):**
- `bootstrap/tests/test_agent.test` — 10 tests: agent-start-basic, agent-get-basic, agent-get-transform, agent-update-basic, agent-get-after-update, agent-get-and-update, agent-get-and-update-verify, agent-stop, agent-stop-get-error, agent-multiple

---

## Day 101 Progress - Task Async/Await (`⟳⊳`, `⟳⊲`, `⟳⊲?`)

**RESULT:** 99/99 test files passing (100%), 10 new tests (Task)

### New Feature: Task — Async/Await Pattern

Tasks are a higher-level abstraction over actors for spawning computations and retrieving their results. Unlike regular actor spawn (`⟳`) which takes a `(self)` behavior function, `⟳⊳` takes a zero-arg function — simpler for fire-and-forget computations. `⟳⊲` provides blocking await (suspends calling actor until target finishes), and `⟳⊲?` provides non-blocking polling.

**New Primitives (3):**
- `⟳⊳` (task-async) — Spawn task from zero-arg function: `(⟳⊳ (λ () expr))` → `⟳[id]`
- `⟳⊲` (task-await) — Block until task finishes: `(⟳⊲ task)` → result (suspends if not done)
- `⟳⊲?` (task-yield) — Non-blocking check: `(⟳⊲? task)` → result or `∅`

**Semantics:**
- Task-async spawns an actor whose body is `(fn)` — no self parameter needed
- Task captures closure variables from definition scope
- Await from actor context suspends via `SUSPEND_TASK_AWAIT` — scheduler polls target liveness
- Await on already-finished task returns result immediately
- Yield returns `∅` if task still running, result if finished
- Both await and yield work on any actor (not just tasks)
- Await/yield on non-actor values returns error

**New Suspend Reason:**
- `SUSPEND_TASK_AWAIT` — fiber yields until target actor's `alive` becomes false
- Scheduler checks each tick: if target dead, resumes with target's result

**Files Modified (3):**
- `bootstrap/fiber.h` — Added `SUSPEND_TASK_AWAIT` to `SuspendReason` enum, `suspend_await_actor_id` field
- `bootstrap/actor.c` — Two new `SUSPEND_TASK_AWAIT` cases in scheduler (skip check + resume logic)
- `bootstrap/primitives.c` — 3 new primitive functions + registration in table

**New Test File (1):**
- `bootstrap/tests/test_task.test` — 10 tests: task-async-basic, task-yield-done, task-yield-pending, task-await-basic, task-await-immediate, task-async-closure, task-await-error, task-await-not-actor, task-yield-not-actor, task-multiple

---

## Day 100 Progress - Application Behavior (`⟳⊚⊕`, `⟳⊚⊖`, `⟳⊚?`, `⟳⊚*`, `⟳⊚⊙`, `⟳⊚←`)

**RESULT:** 98/98 test files passing (100%), 10 new tests (Application)

### New Feature: Application Behavior — OTP Top-Level Container

Applications are named units that wrap a supervision tree. They provide start/stop lifecycle, per-app environment (key-value config), and runtime discovery. This is the OTP Application behavior — the top-level container that ties together supervisors, workers, and configuration.

**New Primitives (6):**
- `⟳⊚⊕` (app-start) — Start named application: `(⟳⊚⊕ :name start-fn)` → `:name` or `⚠`
- `⟳⊚⊖` (app-stop) — Stop application: `(⟳⊚⊖ :name)` → `#t` or `⚠`
- `⟳⊚?` (app-info) — Get app info: `(⟳⊚? :name)` → `⟨:name sup-id⟩` or `∅`
- `⟳⊚*` (app-which) — List running apps: `(⟳⊚*)` → `[:name]`
- `⟳⊚⊙` (app-get-env) — Get app env key: `(⟳⊚⊙ :name :key)` → value or `∅`
- `⟳⊚←` (app-set-env) — Set app env key: `(⟳⊚← :name :key value)` → `#t`

**Semantics:**
- Application name is a symbol (`:myapp`, `:webserver`, etc.)
- Start function is `(λ () supervisor-id)` — must create and return a supervisor
- No duplicate names (error on conflict)
- Stop marks app inactive, releases env, releases stop callback
- App environment is a per-app key-value store (max 64 entries)
- `⟳∅` (reset) clears all applications for test isolation
- Max 16 concurrent applications

**Files Modified (3):**
- `bootstrap/actor.h` — Application struct, MAX_APPLICATIONS/MAX_APP_ENV, app API declarations
- `bootstrap/actor.c` — Application implementation (global array), app_start/stop/lookup/which/get_env/set_env/reset_all; actor_reset_all calls app_reset_all
- `bootstrap/primitives.c` — 6 new primitive functions + registration in table

**New Test File (1):**
- `bootstrap/tests/test_application.test` — 10 tests: app-start-basic, app-which, app-info, app-info-not-running, app-stop, app-stop-not-running, app-duplicate-name, app-set-get-env, app-env-missing, app-with-supervisor-tree

---

## Day 99 Progress - ETS (`⟳⊞⊕`, `⟳⊞⊙`, `⟳⊞?`, `⟳⊞⊖`, `⟳⊞!`, `⟳⊞#`, `⟳⊞*`)

**RESULT:** 97/97 test files passing (100%), 10 new tests (ETS)

### New Feature: ETS (Erlang Term Storage) — Shared Named Tables

Global named key-value tables accessible from any context (actors or top-level). Tables are identified by symbol names and can be shared across multiple actors. Owner tracking enables automatic cleanup when an actor dies.

**New Primitives (7):**
- `⟳⊞⊕` (ets-new) — Create named table: `(⟳⊞⊕ :name)` → `:name` or `⚠`
- `⟳⊞⊙` (ets-insert) — Insert key-value: `(⟳⊞⊙ :table key value)` → `#t`
- `⟳⊞?` (ets-lookup) — Lookup key: `(⟳⊞? :table key)` → value or `∅`
- `⟳⊞⊖` (ets-delete) — Delete key: `(⟳⊞⊖ :table key)` → `#t`
- `⟳⊞!` (ets-delete-table) — Delete entire table: `(⟳⊞! :name)` → `#t`
- `⟳⊞#` (ets-size) — Table size: `(⟳⊞# :name)` → count
- `⟳⊞*` (ets-all) — All entries: `(⟳⊞* :name)` → list of `⟨key value⟩`

**Semantics:**
- Tables identified by symbol name (`:users`, `:cache`, etc.)
- No duplicate names (error on conflict)
- Global scope — accessible without actor context
- Owner tracking: if created inside actor, table auto-destroyed when owner dies
- Insert overwrites existing key (set semantics)
- Delete key is idempotent (no error if key missing)
- Lookup on deleted/nonexistent table returns error
- `⟳∅` (reset) clears all ETS tables for test isolation
- Linear scan for key lookup (fine for ≤256 entries)
- Max 64 tables, 256 entries each

**Files Modified (3):**
- `bootstrap/actor.h` — EtsTable struct, MAX_ETS_TABLES/ENTRIES, ETS API declarations
- `bootstrap/actor.c` — ETS implementation (global table registry), cleanup in `actor_notify_exit` and `actor_reset_all`
- `bootstrap/primitives.c` — 7 new primitive functions + registration in table

**New Test File (1):**
- `bootstrap/tests/test_ets.test` — 10 tests: ets-new-basic, ets-insert-lookup, ets-lookup-missing, ets-insert-overwrite, ets-delete-key, ets-size, ets-all-entries, ets-delete-table, ets-duplicate-name, ets-cross-actor

---

## Day 98 Progress - Process Dictionary (`⟳⊔⊕`, `⟳⊔?`, `⟳⊔⊖`, `⟳⊔*`)

**RESULT:** 96/96 test files passing (100%), 10 new tests (process dictionary)

### New Feature: Process Dictionary — Per-Actor Key-Value State

Erlang-style process dictionary — a per-actor key-value store accessible only from within that actor's context. Enables stateful GenServer patterns without relying solely on closures.

**New Primitives (4):**
- `⟳⊔⊕` (put) — Store key-value in actor dict: `(⟳⊔⊕ key value)` → old-value or `∅`
- `⟳⊔?` (get) — Lookup key in actor dict: `(⟳⊔? key)` → value or `∅`
- `⟳⊔⊖` (erase) — Remove key from actor dict: `(⟳⊔⊖ key)` → old-value or `∅`
- `⟳⊔*` (get-all) — List all dict entries: `(⟳⊔*)` → list of `⟨key value⟩` pairs

**Semantics:**
- Keys can be any value (symbols, numbers, etc.), compared with `cell_equal`
- Per-actor isolation — each actor has its own dictionary, no cross-actor access
- Calling outside actor context returns `⚠ :not-in-actor`
- Auto-cleared when actor is destroyed (keys/values released)
- `⟳∅` (reset) clears all dicts via existing actor destroy path
- Linear scan for key lookup (fine for <=256 entries)

**Files Modified (3):**
- `bootstrap/actor.h` — Added `MAX_DICT_ENTRIES`, dict fields to `Actor` struct
- `bootstrap/actor.c` — Release dict entries in `actor_destroy`
- `bootstrap/primitives.c` — 4 new primitive functions + registration in table

**New Test File (1):**
- `bootstrap/tests/test_proc_dict.test` — 10 tests: put-get-basic, put-overwrite, get-missing, erase-basic, erase-missing, get-all, multiple-keys, not-in-actor, isolation, genserver-state

---

## Day 97 Progress - GenServer (`⟳⇅`, `⟳⇅!`)

**RESULT:** 95/95 test files passing (100%), 10 new tests (GenServer call-reply)

### New Feature: GenServer — Synchronous Call-Reply Pattern

Erlang-style synchronous call-reply between actors. `⟳⇅` (call) sends a tagged `⟨:call caller-id request⟩` message to the target actor and suspends the caller until a reply arrives. `⟳⇅!` (reply) sends the response back to the caller's mailbox.

**New Primitives (2):**
- `⟳⇅` (call) — Synchronous call: `(⟳⇅ target request)` → suspends until reply
- `⟳⇅!` (reply) — Reply to caller: `(⟳⇅! caller-id response)` → sends response

**Semantics:**
- Call sends `⟨:call caller-actor request⟩` to target, then yields on mailbox
- Server receives the message, extracts caller from `(◁ (▷ msg))` and request from `(◁ (▷ (▷ msg)))`
- Reply sends response directly to caller's mailbox
- Calling outside actor context returns error
- Calling dead actor returns error
- Multiple sequential calls work correctly (server handles them in order)

**Files Modified (1):**
- `bootstrap/primitives.c` — 2 new primitive functions + registration in table

**New Test File (1):**
- `bootstrap/tests/test_genserver.test` — 10 tests: call-reply-basic, call-reply-echo, call-reply-multiple, call-not-actor, call-dead-actor, reply-not-actor, call-with-registered, call-message-format, call-with-timer, call-outside-actor

---

## Day 96 Progress - Timers (`⟳⏱`, `⟳⏱×`, `⟳⏱?`)

**RESULT:** 94/94 test files passing (100%), 10 new tests (timers)

### New Feature: Timers — Scheduled Message Delivery

Timers schedule message delivery to an actor after N scheduler ticks. They integrate into the scheduler loop — each tick decrements active timers, and when a timer fires it sends its message to the target actor's mailbox. The scheduler keeps spinning while timers are pending, even if no actors ran.

**New Primitives (3):**
- `⟳⏱` (send-after) — Schedule message after N ticks: `(⟳⏱ ticks target message)` → timer-id
- `⟳⏱×` (cancel-timer) — Cancel a pending timer: `(⟳⏱× timer-id)` → `#t | ⚠`
- `⟳⏱?` (timer-active?) — Check if timer is still pending: `(⟳⏱? timer-id)` → `#t | #f`

**Semantics:**
- Timer IDs are monotonically increasing integers
- Timer fires when remaining_ticks reaches 0 (fires after N+1 ticks from creation)
- Dead actor targets silently drop the message (no crash)
- Cancelled timers immediately release their message
- `timer_tick_all()` called each scheduler tick, returns whether any timer fired
- Scheduler keeps spinning while `timer_any_pending()` is true (prevents early exit)
- `⟳∅` (reset) clears all timers for test isolation

**Files Modified (3):**
- `bootstrap/actor.h` — Timer struct, MAX_TIMERS, timer API declarations
- `bootstrap/actor.c` — Timer implementation (array-based), scheduler integration, timer_tick_all/timer_any_pending
- `bootstrap/primitives.c` — 3 new primitive functions + registration in table

**New Test File (1):**
- `bootstrap/tests/test_timers.test` — 10 tests: timer-basic, timer-immediate, timer-cancel, timer-active, timer-inactive-after-fire, timer-inactive-after-cancel, timer-multiple, timer-cancel-invalid, timer-dead-actor, timer-by-name

---

## Day 95 Progress - Process Registry (`⟳⊜⊕`, `⟳⊜⊖`, `⟳⊜?`, `⟳⊜*`)

**RESULT:** 93/93 test files passing (100%), 10 new tests (process registry)

### New Feature: Named Process Registry

Erlang-style process registry allowing actors to be registered and looked up by name (symbol). Essential for building discoverable services in actor systems.

**New Primitives (4):**
- `⟳⊜⊕` (register) — Register actor under a name: `(⟳⊜⊕ :server actor)` → `#t | ⚠`
- `⟳⊜⊖` (unregister) — Remove name from registry: `(⟳⊜⊖ :server)` → `#t | ⚠`
- `⟳⊜?` (whereis) — Look up actor by name: `(⟳⊜? :server)` → `⟳ | ∅`
- `⟳⊜*` (registered) — List all registered names: `(⟳⊜*)` → `[:symbol]`

**Semantics:**
- Names are symbols (`:server`, `:logger`, etc.)
- One name → one actor, one actor → one name (no duplicates)
- Dead actors auto-deregistered via `actor_notify_exit` hook
- `⟳∅` (reset) clears registry for test isolation
- Whereis on unregistered name returns `∅` (not error)

**Files Modified (3):**
- `bootstrap/actor.h` — Registry API declarations, `MAX_REGISTRY`
- `bootstrap/actor.c` — Registry implementation (parallel arrays), auto-deregister hook in `actor_notify_exit`, reset hook in `actor_reset_all`
- `bootstrap/primitives.c` — 4 new primitive functions + registration in table

**New Test File (1):**
- `bootstrap/tests/test_registry.test` — 10 tests: register-basic, register-send-by-name, whereis-unregistered, unregister-basic, register-duplicate-name, register-duplicate-actor, registered-list, dead-actor-auto-deregister, register-not-symbol, register-dead-actor

---

## Day 94 Progress - Dynamic Child Management (`⟳⊛⊕`, `⟳⊛⊖`) + Rest-for-One Strategy

**RESULT:** 92/92 test files passing (100%), 10 new tests (dynamic supervisor children + rest-for-one)

### New Feature: Dynamic Supervisor Children + Rest-for-One

Supervisors can now add and remove children at runtime, and a third restart strategy `:rest-for-one` restarts the crashed child and all children started after it (preserving earlier siblings).

**New Primitives (2):**
- `⟳⊛⊕` (sup-add-child) — Dynamically add a new child to a supervisor: `(⟳⊛⊕ sup-id behavior)` → new child actor ID
- `⟳⊛⊖` (sup-remove-child) — Remove a child from a supervisor: `(⟳⊛⊖ sup-id child-actor)` → `#t`

**New Strategy:**
- `:rest-for-one` — On child crash, kill all children after the crashed one, then restart from crashed index onward. Earlier siblings are untouched.

**Semantics:**
- `⟳⊛⊕` validates supervisor exists and child_count < MAX_SUP_CHILDREN, stores spec, spawns child, returns actor ID
- `⟳⊛⊖` accepts actor cell or number for child ID, kills actor with `:shutdown`, shifts arrays down, decrements count
- Rest-for-one: on crash at index N, kills children N+1..end with `:shutdown`, respawns N..end from specs

**Files Modified (3):**
- `bootstrap/actor.h` — Added `SUP_REST_FOR_ONE` to `SupervisorStrategy` enum
- `bootstrap/actor.c` — Added rest-for-one case in `supervisor_handle_exit`
- `bootstrap/primitives.c` — 2 new primitive functions (`prim_sup_add_child`, `prim_sup_remove_child`), `:rest-for-one` strategy support in `prim_sup_start`, registration

**New Test File (1):**
- `bootstrap/tests/test_sup_dynamic.test` — 10 tests covering add-basic, add-multiple, add-max-overflow, remove-basic, remove-remaining-supervised, rest-for-one-middle, rest-for-one-first, rest-for-one-last, dynamic-add-with-rest-for-one, remove-then-crash

---

## Day 93 Progress - Supervisor Strategies (`⟳⊛`, `⟳⊛?`, `⟳⊛!`)

**RESULT:** 91/91 test files passing (100%), 8 new tests (supervisor strategies)

### New Feature: Supervisors — Automatic Child Restart on Failure

Supervisors manage groups of child actors and automatically restart them when they crash. Two strategies are supported: one-for-one (restart only the failed child) and one-for-all (restart all children when one fails). Built-in restart limit of 5 prevents infinite restart loops.

**New Primitives (3):**
- `⟳⊛` (sup-start) — Create supervisor with strategy and child spec list: `(⟳⊛ :one-for-one specs)`
- `⟳⊛?` (sup-children) — Get list of current child actor cells: `(⟳⊛? sup-id)`
- `⟳⊛!` (sup-restart-count) — Get number of restarts performed: `(⟳⊛! sup-id)`

**Strategies:**
- `:one-for-one` — Only restart the crashed child; other children unchanged
- `:one-for-all` — Kill all siblings, then restart all children from specs

**Semantics:**
- Supervisor hooks into `actor_notify_exit` — when a supervised child dies with error, restart strategy fires
- Normal exits do NOT trigger restarts
- One-for-all kills siblings with `:shutdown` reason before respawning
- Max 5 restarts per supervisor (SUP_MAX_RESTARTS); exceeding limit stops restarts
- `⟳∅` (reset) cleans up all supervisors

**Files Modified (3):**
- `bootstrap/actor.h` — Supervisor struct, strategy enum, supervisor API declarations
- `bootstrap/actor.c` — `supervisor_create`, `supervisor_spawn_child`, `supervisor_handle_exit`, `supervisor_find_for_child`, `supervisor_lookup`; `actor_notify_exit` checks for supervisor; `actor_reset_all` cleans up supervisors
- `bootstrap/primitives.c` — 3 new primitive functions + registration

**New Test File (1):**
- `bootstrap/tests/test_supervisor.test` — 8 tests covering creation, one-for-one restart, stable-child-unchanged, one-for-all restart, new-ids, restart counts, max-restarts-exceeded

---

## Previous Day: Day 92 - Supervision + Refcount Bugfix (2026-01-30)

**RESULT:** 90/90 test files passing (100%), 8 new tests (supervision)

### Actor Supervision — Linking, Monitoring, Exit Signals

Actors can now monitor and react to other actors' termination. Bidirectional links propagate failure (Erlang-style), monitors provide one-way death notifications, and exit trapping converts signals to messages.

**New Primitives (5):**
- `⟳⊗` (link) — Bidirectional link between current actor and target
- `⟳⊘` (unlink) — Remove bidirectional link
- `⟳⊙` (monitor) — One-way monitor; receive `⟨:DOWN id reason⟩` on death
- `⟳⊜` (trap-exit) — Enable/disable exit trapping (#t/#f)
- `⟳✕` (exit) — Send exit signal to actor with reason

**Semantics:**
- Error exit propagates to linked actors (kills them unless trapping)
- Normal exit does NOT kill linked actors
- Trap-exit converts exit signals to `⟨:EXIT sender-id reason⟩` messages
- Monitors always receive `⟨:DOWN id reason⟩` messages (no death propagation)
- Linking to already-dead actors immediately applies exit semantics
- Monitoring already-dead actors immediately delivers `:DOWN` message

**Files Modified (3):**
- `bootstrap/actor.h` — Links/monitors arrays, trap_exit flag, supervision API
- `bootstrap/actor.c` — `actor_link`, `actor_unlink`, `actor_add_monitor`, `actor_exit_signal`, `actor_notify_exit`; scheduler calls `actor_notify_exit` on actor finish
- `bootstrap/primitives.c` — 5 new primitive functions + registration

**New Test File (1):**
- `bootstrap/tests/test_supervision.test` — 8 tests covering monitor-normal, monitor-error, link-propagation, trap-exit, unlink, exit-signal, exit-trapped, normal-no-kill

---

## Day 91 Progress - Channel Select (`⟿⊞`, `⟿⊞?`)

**RESULT:** 89/89 test files passing (100%), 8 new tests (select)

### New Feature: Channel Select — Wait on Multiple Channels

Select allows waiting on multiple channels simultaneously, returning a `⟨channel value⟩` pair indicating which channel fired. Round-robin fairness prevents starvation.

**New Primitives (2):**
- `⟿⊞` (select, blocking) — Wait on multiple channels, yields if none ready
- `⟿⊞?` (select-try, non-blocking) — Return first ready channel or `∅`

**Scheduler Extension:**
- Added `SUSPEND_SELECT` to `SuspendReason` enum
- Added `suspend_select_ids[]` and `suspend_select_count` to `Fiber` struct
- Both scheduler switch blocks in `actor_run_all()` handle multi-channel polling
- Round-robin start index for fairness across scheduling ticks

**Files Modified (3):**
- `bootstrap/fiber.h` — New enum value + select tracking fields
- `bootstrap/actor.c` — SUSPEND_SELECT handling in both scheduler switches
- `bootstrap/primitives.c` — `prim_chan_select`, `prim_chan_select_try`, registration

**New Test File (1):**
- `bootstrap/tests/test_select.test` — 8 tests covering basic, correct-channel, try-empty, try-data, blocking, all-closed, some-closed, three-channel

---

## Day 90 Progress - Channel Primitives

**RESULT:** 88/88 test files passing (100%), 12 new tests (channels)

### New Feature: Channels — Typed Communication Primitives

Channels are first-class bounded ring buffers that any actor can send to or receive from. They complement the actor mailbox system with shared, named communication endpoints. Blocking semantics are integrated into the cooperative scheduler via `SuspendReason`.

**New Files (2):**
- `bootstrap/channel.h` — Channel struct, registry API
- `bootstrap/channel.c` — Ring buffer operations, registry, create/close/destroy/try_send/try_recv/reset_all

**New Cell Type:**
- `CELL_CHANNEL` — First-class channel values, printed as `⟿[N]`

**New Primitives (5):**
- `⟿⊚` (create) — Create channel with optional capacity (default 64)
- `⟿→` (send) — Send value to channel (yields if buffer full)
- `⟿←` (recv) — Receive from channel (yields if buffer empty)
- `⟿×` (close) — Close channel (no more sends)
- `⟿∅` (reset) — Reset all channels (for testing)

**Scheduler Extension — SuspendReason:**
- Added `SuspendReason` enum to `fiber.h`: `SUSPEND_GENERAL`, `SUSPEND_MAILBOX`, `SUSPEND_CHAN_RECV`, `SUSPEND_CHAN_SEND`
- Scheduler in `actor_run_all()` now dispatches on suspend reason instead of just checking mailbox
- `prim_receive` (←?) now sets `SUSPEND_MAILBOX` before yielding
- Channel recv/send set `SUSPEND_CHAN_RECV`/`SUSPEND_CHAN_SEND` with channel ID

**Examples:**
```scheme
; Create and use a channel
(≔ ch (⟿⊚))
(≔ producer (⟳ (λ (self) (⟿→ ch :hello))))
(≔ consumer (⟳ (λ (self) (⟿← ch))))
(⟳! #100)
(⟳→ consumer)         ; → :hello

; Blocking recv — consumer starts before producer
(≔ ch (⟿⊚))
(≔ c (⟳ (λ (self) (⟿← ch))))   ; blocks
(≔ p (⟳ (λ (self) (⟿→ ch :wakeup))))
(⟳! #100)
(⟳→ c)                ; → :wakeup

; Capacity-1 channel with blocking send
(≔ ch (⟿⊚ #1))
(≔ s (⟳ (λ (self)
  (≫ (⟿→ ch :a) (λ (_)
  (≫ (⟿→ ch :b) (λ (_)  ; blocks until :a drained
    :done)))))))
(≔ d (⟳ (λ (self)
  (≫ (⟿← ch) (λ (v1)
    (≫ (⟿← ch) (λ (v2)
      (⟨⟩ v1 v2))))))))
(⟳! #200)
(⟳→ d)                ; → ⟨:a :b⟩
```

**Design Decisions:**
- Channels are independent of actors — any actor can send/recv on any channel
- Bounded ring buffer (configurable capacity, default 64)
- Scheduler-polled suspension — no wait queues, scheduler checks channel state each tick
- Close semantics: send to closed → error; recv from closed empty → error; recv from closed non-empty → returns buffered values
- `actor_reset_all()` also calls `channel_reset_all()` for clean test isolation

---

## Previous Day: Day 89 - Actor Model with Message Passing

**RESULT:** 87/87 test files passing (100%), 12 new tests (actor model)

Cooperative actor model built on top of the fiber/coroutine infrastructure from Day 88. Actors are fibers with mailboxes, scheduled cooperatively (single-threaded, round-robin).

**New Files:** `bootstrap/actor.h`, `bootstrap/actor.c`
**New Cell Type:** `CELL_ACTOR` — printed as `⟳[N]`

**Primitives (7):** `⟳` (spawn), `→!` (send), `←?` (receive), `⟳!` (run), `⟳?` (alive?), `⟳→` (result), `⟳∅` (reset)

---

## Previous Day: Day 88 - Delimited Continuations via Fibers

**RESULT:** 86/86 test files passing (100%), 21 new tests

Replaced replay-based resumable effects with real delimited continuations using fiber/coroutine-based context switching via `ucontext`. O(n) instead of O(n²).

**New Files:** `bootstrap/fiber.h`, `bootstrap/fiber.c`
**New Special Forms:** `⟪⊸⟫` (reset/prompt), `⊸` (shift/control)
**Rewritten:** `⟪↺⟫` and `↯` now fiber-based

---

## Current Status

**System State:**
- **Primitives:** 226 total (215 + 11 HashSet)
- **Special Forms:** Added ⪢ (sequencing)
- **Cell Types:** 17 total (added CELL_HASHMAP, CELL_SET)
- **Tests:** 108/108 test files passing (100%)
- **Build:** Clean, O2 optimized, 32MB stack

**Core Capabilities:**
- Lambda calculus with De Bruijn indices + TCO
- Algebraic effect system (⟪, ⟪⟫, ↯) with dynamic handler stack
- Resumable effects via fibers (⟪↺⟫) — O(n) delimited continuations
- Delimited continuations (⟪⊸⟫, ⊸) — shift/reset
- Actor model (⟳, →!, ←?, ⟳!) — cooperative round-robin scheduler
- Channels (⟿⊚, ⟿→, ⟿←, ⟿×) — bounded ring buffers with blocking
- Channel select (⟿⊞, ⟿⊞?) — multiplexed channel waiting
- Supervision (⟳⊗, ⟳⊘, ⟳⊙, ⟳⊜, ⟳✕) — linking, monitoring, exit signals
- Supervisor strategies (⟳⊛, ⟳⊛?, ⟳⊛!) — one-for-one, one-for-all, rest-for-one
- Dynamic supervisor children (⟳⊛⊕, ⟳⊛⊖) — runtime add/remove
- Process registry (⟳⊜⊕, ⟳⊜⊖, ⟳⊜?, ⟳⊜*) — named actors
- Timers (⟳⏱, ⟳⏱×, ⟳⏱?) — scheduled message delivery
- GenServer (⟳⇅, ⟳⇅!) — synchronous call-reply
- Process dictionary (⟳⊔⊕, ⟳⊔?, ⟳⊔⊖, ⟳⊔*) — per-actor key-value
- ETS (⟳⊞⊕, ⟳⊞⊙, ⟳⊞?, ⟳⊞⊖, ⟳⊞!, ⟳⊞#, ⟳⊞*) — shared named tables
- Application (⟳⊚⊕, ⟳⊚⊖, ⟳⊚?, ⟳⊚*, ⟳⊚⊙, ⟳⊚←) — OTP top-level container
- Task async/await (⟳⊳, ⟳⊲, ⟳⊲?) — spawn computation and await result
- Agent (⟳⊶, ⟳⊶?, ⟳⊶!, ⟳⊶⊕, ⟳⊶×) — functional state wrapper
- GenStage (⟳⊵, ⟳⊵⊕, ⟳⊵→, ⟳⊵⊙, ⟳⊵?, ⟳⊵×) — producer-consumer pipelines
- DynamicSupervisor (⟳⊛⊹, ⟳⊛⊹⊕, ⟳⊛⊹⊖, ⟳⊛⊹?, ⟳⊛⊹#) — on-demand child spawning with restart types
- Flow (⟳⊸, ⟳⊸↦, ⟳⊸⊲, ⟳⊸⊕, ⟳⊸⊙, ⟳⊸!) — lazy computation pipelines
- Mutable references (□, □→, □←, □?, □⊕, □⇌) — first-class mutable containers
- Sequencing (⪢) — multi-expression evaluation with TCO
- Flow Registry (⟳⊸⊜⊕, ⟳⊸⊜?, ⟳⊸⊜⊖, ⟳⊸⊜*) — named flow pipelines
- Weak references (◇, ◇→, ◇?, ◇⊙) — observe without preventing collection
- HashMap (⊞) — Swiss Table + SipHash-2-4 with portable SIMD
- HashSet (⊍) — Boost groups-of-15 + overflow Bloom byte
- Module system (⋘ load, ⌂⊚ info)
- Structures (⊙ leaf, ⊚ node/ADT)
- Pattern matching (∇) with guards, as-patterns, or-patterns, view patterns
- CFG/DFG graphs (⊝) with traversal, reachability, path finding, cycle detection
- Type system: annotations (Day 83) + validation (Day 84) + inference (Day 85)
- Auto-documentation, property-based testing, mutation testing
- Math library (22 primitives), string operations, REPL with history/completion
- Pattern-based macros (⧉⊜) with unlimited arity via ellipsis
- Stdlib macros: ∧*, ∨*, ⇒*, ≔⇊, ⇤, ⚡, ⊎, ⊲*, etc.

---

## Recent Milestones

| Day | Feature | Tests |
|-----|---------|-------|
| 110 | HashSet (⊍) — Boost groups-of-15 + overflow Bloom byte | 108/108 (100%), 10 new tests |
| 109 | HashMap (⊞) — Swiss Table + SipHash-2-4 + portable SIMD | 107/107 (100%), 10 new tests |
| 108 | Weak References (◇, ◇→, ◇?, ◇⊙) — intrusive dual-count zombie | 106/106 (100%), 10 new tests |
| 107 | Mutable References (□, □→, □←, □?, □⊕, □⇌) + Sequencing (⪢) | 105/105 (100%), 10 new tests |
| 106 | Flow Registry (⟳⊸⊜⊕, ⟳⊸⊜?, ⟳⊸⊜⊖, ⟳⊸⊜*) — named flow pipelines | 104/104 (100%), 10 new tests |
| 105 | Flow (⟳⊸, ⟳⊸↦, ⟳⊸⊲, ⟳⊸⊕, ⟳⊸⊙, ⟳⊸!) — lazy computation pipelines | 103/103 (100%), 10 new tests |
| 104 | DynamicSupervisor (⟳⊛⊹, ⟳⊛⊹⊕, ⟳⊛⊹⊖, ⟳⊛⊹?, ⟳⊛⊹#) — on-demand child spawning | 102/102 (100%), 10 new tests |
| 103 | GenStage (⟳⊵, ⟳⊵⊕, ⟳⊵→, ⟳⊵⊙, ⟳⊵?, ⟳⊵×) — producer-consumer pipelines | 101/101 (100%), 10 new tests |
| 102 | Agent (⟳⊶, ⟳⊶?, ⟳⊶!, ⟳⊶⊕, ⟳⊶×) — functional state wrapper | 100/100 (100%), 10 new tests |
| 101 | Task async/await (⟳⊳, ⟳⊲, ⟳⊲?) — spawn and await computations | 99/99 (100%), 10 new tests |
| 100 | Application (⟳⊚⊕, ⟳⊚⊖, ⟳⊚?, ⟳⊚*, ⟳⊚⊙, ⟳⊚←) — OTP top-level container | 98/98 (100%), 10 new tests |
| 99 | ETS (⟳⊞⊕, ⟳⊞⊙, ⟳⊞?, ⟳⊞⊖, ⟳⊞!, ⟳⊞#, ⟳⊞*) — shared named tables | 97/97 (100%), 10 new tests |
| 98 | Process Dictionary (⟳⊔⊕, ⟳⊔?, ⟳⊔⊖, ⟳⊔*) — per-actor state | 96/96 (100%), 10 new tests |
| 97 | GenServer (⟳⇅, ⟳⇅!) — synchronous call-reply | 95/95 (100%), 10 new tests |
| 96 | Timers (⟳⏱, ⟳⏱×, ⟳⏱?) — scheduled message delivery | 94/94 (100%), 10 new tests |
| 95 | Process Registry (⟳⊜⊕, ⟳⊜⊖, ⟳⊜?, ⟳⊜*) — named actors | 93/93 (100%), 10 new tests |
| 94 | Dynamic Supervisor Children (⟳⊛⊕, ⟳⊛⊖) + rest-for-one strategy | 92/92 (100%), 10 new tests |
| 93 | Supervisor Strategies (⟳⊛, ⟳⊛?, ⟳⊛!) — one-for-one, one-for-all | 91/91 (100%), 8 new tests |
| 92 | Supervision (⟳⊗, ⟳⊘, ⟳⊙, ⟳⊜, ⟳✕) + refcount bugfix | 90/90 (100%), 8 new tests |
| 91 | Channel Select (⟿⊞, ⟿⊞?) — multiplexed waiting | 89/89 (100%), 8 new tests |
| 90 | Channels (⟿⊚, ⟿→, ⟿←, ⟿×, ⟿∅) — bounded ring buffers | 88/88 (100%), 12 new tests |
| 89 | Actor Model (⟳, →!, ←?, ⟳!, ⟳?, ⟳→, ⟳∅) | 87/87 (100%), 12 new tests |
| 88 | Delimited Continuations via Fibers (⟪⊸⟫, ⊸) - O(n) effects | 86/86 (100%), 21 new tests |
| 87 | Resumable Effect Handlers (⟪↺⟫) - replay-based | 85/85 (100%), 30 new tests |
| 86 | Algebraic Effect System (⟪, ⟪⟫, ↯) - dynamic handlers | 84/84 (100%), 35 new tests |
| 85 | Type Inference (∈⍜, ∈⍜⊕, ∈⍜*) - deep/static inference | 83/83 (100%), 73 new tests |
| 84 | Type Validation (∈✓, ∈✓*, ∈⊢) - compiler-level | 82/82 (100%), 35 new tests |
| 83 | Type Annotations (18 primitives for gradual typing) | 81/81 (100%), 55 new tests |
| 82 | Exception Handling Macros (⚡, ⚡⊳, ⚡∅, etc.) + ⚠⊙, ⚠→ | 80/80 (100%), 44 new tests |
| 81 | Iteration Macros (⊎, ⊲*, ⟳, ⊎↦, ⊎⊲, ⟳←) | 79/79 (100%), 31 new tests |
| 80 | Data Flow Analysis + N-Function Mutual Recursion | 77/77 (100%), 56 new tests |
| 79 | Variadic Stdlib Macros (∧*, ∨*, ⇒*, ≔⇊, ⇤) | 76/76 (100%), 58 new tests |
| 78 | Rest Pattern Syntax ($var ... ellipsis) | 75/75 (100%), 51 new tests |
| 77 | Control Flow Macros (∧*, ∨*, ⇒, ⇏) | 74/74 (100%), 46 new tests |
| 76 | Stdlib Pattern Macros (⇒*, ≔⇊, ⇤) | 73/73 (100%), 22 new tests |
| 75 | Pattern-Based Macros (⧉⊜) | 72/72 (100%), 29 new tests |

**Full historical details:** See `docs/archive/2026-01/sessions/DAYS_43_68_HISTORY.md`

---

## Quick Reference

### Build & Test
```bash
make              # Build (O2 optimized, 32MB stack)
make test         # Run full test suite (92 test files)
make repl         # Start interactive REPL
make clean        # Clean build artifacts
make rebuild      # Clean + rebuild
```

### Key Files
```
bootstrap/channel.{h,c}     # Channel implementation (Day 90)
bootstrap/actor.{h,c}       # Actor model (Day 89)
bootstrap/fiber.{h,c}       # Fiber/coroutine infrastructure (Day 88)
bootstrap/eval.c             # Special forms: ⟪, ⟪⟫, ↯, ⟪↺⟫, ⟪⊸⟫, ⊸, ∈, ∈?, ∈✓, ∈⊢, ∈⍜, ∈⍜*
bootstrap/primitives.c       # All primitive operations (146 total)
bootstrap/cell.{h,c}        # Core data structures
bootstrap/macro.{h,c}       # Pattern-based macro system
bootstrap/stdlib/            # Standard library modules
bootstrap/tests/             # Test suite (88 test files)
```

### Documentation
- **README.md** - Project overview
- **SPEC.md** - Language specification (146 primitives)
- **CLAUDE.md** - Philosophy and principles
- **docs/INDEX.md** - Documentation hub
- **docs/reference/** - Deep technical docs

---

## Session Handoff Protocol

**Starting a new session:**
1. Read this file
2. Run `make test` to verify
3. Check `git log --oneline -5` for recent changes

**Ending a session:**
1. Update this file's status section
2. Commit with detailed message
3. Archive detailed notes to `docs/archive/`

---

**Last Updated:** 2026-01-30 (Day 110 complete)
**Next Session:** Day 111 - Continue data structures (trees, queues) or new domain
