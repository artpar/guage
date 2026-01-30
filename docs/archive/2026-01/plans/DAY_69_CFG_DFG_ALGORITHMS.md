---
Status: BLOCKED
Created: 2026-01-29
Updated: 2026-01-29
Purpose: Plan for CFG/DFG graph algorithm implementation
Blocker: Graph memory corruption in multi-graph scenarios - see DAY_69_PROGRESS.md
---

# Day 69 Plan: CFG/DFG Graph Algorithms

## 🎯 Goal

Implement graph traversal and query algorithms to make CFG/DFG actually useful for metaprogramming.

## 📍 Current State

**What Exists (Phase 2C):**
- ✅ Graph structures (⊝ primitives)
- ✅ Graph creation (⊝≔, ⊝, ⊝⊕, ⊝⊗)
- ✅ Graph queries (⊝→)
- ✅ CFG/DFG generation (⌂⟿, ⌂⇝)

**What's Missing:**
- ❌ Graph traversal (BFS, DFS)
- ❌ Path finding
- ❌ Reachability analysis
- ❌ Dead code detection
- ❌ Cycle detection
- ❌ Dominance analysis

## 🔨 Implementation Plan

### Part 1: Core Traversal (2 hours)

**New Primitives:**

1. **⊝↦** - Graph map/traverse
   ```scheme
   (⊝↦ graph :bfs start-node visitor-fn)
   (⊝↦ graph :dfs start-node visitor-fn)
   ```
   - BFS and DFS traversal
   - Visitor function called on each node
   - Returns list of visited nodes in order

2. **⊝⊃** - Reachability check
   ```scheme
   (⊝⊃ graph from-node to-node)  ; → #t or #f
   ```
   - Can you reach `to-node` from `from-node`?
   - Uses BFS internally
   - Foundation for dead code analysis

**Examples:**
```scheme
; Get CFG for factorial
(≔ ! (λ (n) (? (≡ n #0) #1 (⊗ n (! (⊖ n #1))))))
(≔ cfg (⌂⟿ :!))

; Find all reachable nodes from entry
(⊝↦ cfg :bfs :entry (λ (node) node))
; → ⟨:entry ⟨:cond ⟨:return-1 ⟨:multiply ⟨:recursive-call ⟨:exit ∅⟩⟩⟩⟩⟩⟩

; Check if recursive call is reachable
(⊝⊃ cfg :entry :recursive-call)  ; → #t
```

### Part 2: Path Finding (1 hour)

**New Primitives:**

3. **⊝⇝** - Find path between nodes
   ```scheme
   (⊝⇝ graph from-node to-node)  ; → path or ∅
   ```
   - Returns shortest path as list of nodes
   - Returns ∅ if no path exists
   - BFS-based (guarantees shortest path)

**Examples:**
```scheme
; Find execution path
(⊝⇝ cfg :entry :exit)
; → ⟨:entry ⟨:cond ⟨:return-1 ⟨:exit ∅⟩⟩⟩⟩  (one possible path)

; Find path to recursive call
(⊝⇝ cfg :entry :recursive-call)
; → ⟨:entry ⟨:cond ⟨:multiply ⟨:recursive-call ∅⟩⟩⟩⟩
```

### Part 3: Analysis Queries (1.5 hours)

**New Primitives:**

4. **⊝⊚** - Get node successors
   ```scheme
   (⊝⊚ graph node)  ; → list of successor nodes
   ```
   - Direct successors only
   - Used for manual graph traversal

5. **⊝⊙** - Get node predecessors
   ```scheme
   (⊝⊙ graph node)  ; → list of predecessor nodes
   ```
   - Direct predecessors only
   - Useful for dependency analysis

6. **⊝∘** - Detect cycles
   ```scheme
   (⊝∘ graph)  ; → list of cycles or ∅
   ```
   - Finds all cycles in graph
   - Returns list of node lists (each cycle)
   - Critical for recursion detection

**Examples:**
```scheme
; Find successors of entry node
(⊝⊚ cfg :entry)  ; → ⟨:cond ∅⟩

; Find predecessors of exit node
(⊝⊙ cfg :exit)  ; → ⟨:return-1 ⟨:recursive-call ∅⟩⟩

; Detect recursive cycles
(⊝∘ cfg)
; → ⟨⟨:recursive-call ⟨:multiply ⟨:cond ⟨:recursive-call ∅⟩⟩⟩⟩ ∅⟩
```

### Part 4: Real-World Use Cases (0.5 hours)

**Dead Code Detection:**
```scheme
(≔ find-dead-code (λ (fn-name)
  (≔ cfg (⌂⟿ fn-name))
  (≔ reachable (⊝↦ cfg :bfs :entry (λ (node) node)))
  (≔ all-nodes (⊝→ cfg :nodes))
  ; Find nodes not in reachable set
  (filter (λ (node) (¬ (member node reachable))) all-nodes)))

(find-dead-code :my-function)
; → ⟨:unreachable-branch ∅⟩  (if any)
```

**Recursion Detection:**
```scheme
(≔ is-recursive? (λ (fn-name)
  (≔ cfg (⌂⟿ fn-name))
  (≔ cycles (⊝∘ cfg))
  (¬ (∅? cycles))))

(is-recursive? :!)  ; → #t
(is-recursive? :double)  ; → #f
```

**Path Coverage Analysis:**
```scheme
(≔ count-paths (λ (fn-name)
  (≔ cfg (⌂⟿ fn-name))
  ; Count all paths from entry to exit
  (count-all-paths cfg :entry :exit)))

(count-paths :!)  ; → #2 (base case + recursive case)
```

## 📝 Implementation Steps

### 1. Graph Traversal (BFS/DFS)

**File:** `bootstrap/primitives.c`

```c
// BFS/DFS traversal with visitor function
Cell* prim_graph_traverse(Cell* args, Env* env) {
    // Extract: graph, :bfs or :dfs, start-node, visitor-fn
    // Traverse graph calling visitor on each node
    // Return list of nodes in visit order
}
```

**Add to primitive table:**
```c
{"⊝↦", prim_graph_traverse, "Graph traverse (BFS/DFS)"}
```

### 2. Reachability

**File:** `bootstrap/primitives.c`

```c
// Check if to-node is reachable from from-node
Cell* prim_graph_reachable(Cell* args, Env* env) {
    // BFS from from-node
    // Return #t if to-node found, #f otherwise
}
```

### 3. Path Finding

**File:** `bootstrap/primitives.c`

```c
// Find shortest path between nodes
Cell* prim_graph_path(Cell* args, Env* env) {
    // BFS with parent tracking
    // Reconstruct path from parents
    // Return path as list or ∅
}
```

### 4. Successor/Predecessor

**File:** `bootstrap/primitives.c`

```c
// Get direct successors
Cell* prim_graph_successors(Cell* args, Env* env) {
    // Extract edges from graph
    // Filter by source node
    // Return target nodes
}

// Get direct predecessors
Cell* prim_graph_predecessors(Cell* args, Env* env) {
    // Extract edges from graph
    // Filter by target node
    // Return source nodes
}
```

### 5. Cycle Detection

**File:** `bootstrap/primitives.c`

```c
// Detect cycles using DFS with colors
Cell* prim_graph_cycles(Cell* args, Env* env) {
    // DFS with white/gray/black coloring
    // Track back edges (gray → gray)
    // Return list of cycles
}
```

## 🧪 Testing Strategy

**Test File:** `bootstrap/tests/test_cfg_algorithms.test`

1. **Basic traversal tests** (BFS vs DFS order)
2. **Reachability tests** (reachable, unreachable)
3. **Path finding tests** (simple paths, no path)
4. **Successor/predecessor tests** (multiple edges)
5. **Cycle detection tests** (recursion, no cycles)
6. **Integration tests** (factorial CFG analysis)
7. **Edge cases** (empty graph, single node, disconnected)

**Expected:** 30-40 comprehensive tests

## 📊 Success Criteria

- ✅ All 6 new primitives implemented
- ✅ 30+ tests passing
- ✅ No regressions (68/68 tests still pass)
- ✅ Dead code detection works
- ✅ Recursion detection works
- ✅ SPEC.md updated (113 → 119 primitives)
- ✅ SESSION_HANDOFF.md updated

## 🎯 Impact

**This enables:**
- 🔥 **Dead code elimination** - automatically identify unreachable code
- 🔥 **Recursion detection** - know which functions are recursive
- 🔥 **Path coverage** - count execution paths for testing
- 🔥 **Optimization** - identify hot paths and inline candidates
- 🔥 **Analysis** - foundation for advanced metaprogramming

**Why This Matters:**
- Makes CFG/DFG **actually useful** (not just data structures)
- Foundation for compiler optimizations
- Enables self-optimizing code
- Core to metaprogramming vision

## 📅 Time Estimate

- Part 1: Core traversal - 2 hours
- Part 2: Path finding - 1 hour
- Part 3: Analysis queries - 1.5 hours
- Part 4: Use cases & testing - 0.5 hour
- Documentation & cleanup - 0.5 hour

**Total:** ~5.5 hours (full day)

## 🚀 Next Session (Day 70)

After completing graph algorithms, next logical steps:

1. **Graph transformations** - Simplification, optimization
2. **Dominance analysis** - For optimization
3. **Macro system** - Phase 3 start
4. **Module enhancements** - Namespace isolation

---

**Ready to implement!** Let's make CFG/DFG actually useful for metaprogramming.
