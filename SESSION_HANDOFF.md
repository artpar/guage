---
Status: CURRENT
Created: 2026-01-27
Updated: 2026-01-30 (Day 96 COMPLETE)
Purpose: Current project status and progress
---

# Session Handoff: Day 96 - Timers / Send-After (2026-01-30)

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
- **Primitives:** 165 total (146 + 5 supervision + 2 select + 3 supervisor + 2 dynamic supervisor + 4 registry + 3 timers)
- **Tests:** 94/94 test files passing (100%)
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

**Last Updated:** 2026-01-30 (Day 96 complete)
**Next Session:** Day 97 - GenServer pattern, supervisor trees, or optimizer
