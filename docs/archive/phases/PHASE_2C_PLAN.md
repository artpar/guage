# Phase 2C: Data Structures as First-Class Citizens

## Timeline: 3 Weeks (Before Pattern Matching)

### Week 1: Cell Infrastructure (Days 1-7)

**Goal:** Extend cell system to support structures and graphs

#### Day 1-2: Cell Type Design
- [ ] Design `CELL_STRUCT` representation
- [ ] Design `CELL_GRAPH` representation
- [ ] Plan memory layout for fields/nodes/edges
- [ ] Design reference counting strategy for cycles

#### Day 3-4: Implement Cell Extensions
- [ ] Add `CELL_STRUCT` to CellType enum
- [ ] Add `CELL_GRAPH` to CellType enum
- [ ] Add `StructData` and `GraphData` to CellData union
- [ ] Implement `make_struct()` constructor
- [ ] Implement `make_graph()` constructor
- [ ] Update `cell_retain()` for new types
- [ ] Update `cell_release()` for new types (handle cycles)

#### Day 5-6: Type Registry
- [ ] Create type registry for structure definitions
- [ ] Store structure schemas (field names, types)
- [ ] Register built-in types (CFG, DFG, CallGraph, DepGraph)
- [ ] Type lookup functions

#### Day 7: Testing
- [ ] Test struct cell creation
- [ ] Test graph cell creation
- [ ] Test reference counting
- [ ] Test cycle detection
- [ ] Verify no memory leaks

**Deliverable:** Cell system supports CELL_STRUCT and CELL_GRAPH with proper memory management

---

### Week 2: Structure Primitives (Days 8-14)

**Goal:** Implement user-facing structure operations

#### Day 8-9: Leaf Structures (⊙)
- [ ] Implement `prim_struct_define_leaf()` - ⊙≔
- [ ] Implement `prim_struct_create()` - ⊙
- [ ] Implement `prim_struct_get()` - ⊙→
- [ ] Implement `prim_struct_set()` - ⊙←
- [ ] Implement `prim_struct_is_type()` - ⊙?
- [ ] Register primitives in primitives.c
- [ ] Test with Point example

**Test Case:**
```scheme
(⊙≔ Point :x :y)
(≔ p (⊙ Point #3 #4))
(⊢ (≡ (⊙→ p :x) #3))
(⊢ (⊙? p Point))
```

#### Day 10-11: Node Structures (⊚) - ADTs
- [ ] Implement `prim_struct_define_node()` - ⊚≔
- [ ] Support multiple variants (sum types)
- [ ] Implement variant tagging
- [ ] Implement `prim_node_create()` - ⊚
- [ ] Implement `prim_node_get()` - ⊚→
- [ ] Implement `prim_node_is_variant()` - ⊚?
- [ ] Register primitives
- [ ] Test with List example

**Test Case:**
```scheme
(⊚≔ List [:Nil] [:Cons :head :tail])
(≔ empty (⊚ List :Nil))
(≔ lst (⊚ List :Cons #1 (⊚ List :Nil)))
(⊢ (⊚? empty List :Nil))
(⊢ (⊚? lst List :Cons))
(⊢ (≡ (⊚→ lst :head) #1))
```

#### Day 12-13: Graph Structures (⊝)
- [ ] Implement `prim_graph_define()` - ⊝≔
- [ ] Implement `prim_graph_create()` - ⊝
- [ ] Implement `prim_graph_add_node()` - ⊝⊕
- [ ] Implement `prim_graph_add_edge()` - ⊝⊗
- [ ] Implement `prim_graph_query()` - ⊝→
- [ ] Implement `prim_graph_is_type()` - ⊝?
- [ ] Register primitives
- [ ] Test with simple graph

**Test Case:**
```scheme
(⊝≔ Graph :nodes :edges)
(≔ g (⊝ Graph ∅ ∅))
(≔ g (⊝⊕ g #0))
(≔ g (⊝⊕ g #1))
(≔ g (⊝⊗ g #0 #1 :edge))
(⊢ (≡ (length (⊝→ g :nodes)) #2))
(⊢ (≡ (length (⊝→ g :edges)) #1))
```

#### Day 14: Integration Testing
- [ ] Test all three structure kinds together
- [ ] Test nested structures
- [ ] Test structure in lists
- [ ] Performance benchmarks
- [ ] Memory leak checks

**Deliverable:** All structure primitives working (⊙, ⊚, ⊝ families)

---

### Week 3: Auto-Generated Graphs (Days 15-21)

**Goal:** Auto-generate CFG/DFG/CallGraph/DepGraph for functions

#### Day 15-16: CFG Generation
- [ ] Create cfg.c/cfg.h
- [ ] Implement `generate_cfg()` function
- [ ] Walk lambda body to find basic blocks
- [ ] Identify branch points (?)
- [ ] Identify entry and exit nodes
- [ ] Create CFG graph structure
- [ ] Add CFG edges (true/false/unconditional)
- [ ] Test with factorial

**CFG Structure:**
```scheme
(⊝ CFG
  :entry #0
  :exit #3
  :nodes [block0 block1 block2 block3]
  :edges [(edge 0 1 :test) (edge 1 2 :true) (edge 1 3 :false) ...])
```

#### Day 17-18: DFG Generation
- [ ] Create dfg.c/dfg.h
- [ ] Implement `generate_dfg()` function
- [ ] Walk lambda body to find operations
- [ ] Track value producers and consumers
- [ ] Create DFG graph structure
- [ ] Add data dependency edges
- [ ] Test with factorial

**DFG Structure:**
```scheme
(⊝ DFG
  :inputs [n]
  :outputs [result]
  :nodes [op0 op1 op2 ...]
  :edges [(dep op0 op1) (dep op1 op2) ...])
```

#### Day 19: Call Graph Generation
- [ ] Create callgraph.c/callgraph.h
- [ ] Implement `generate_callgraph()` function
- [ ] Find all function calls in body
- [ ] Detect recursive calls
- [ ] Create CallGraph structure
- [ ] Add call edges
- [ ] Test with recursive and mutually recursive functions

**CallGraph Structure:**
```scheme
(⊝ CallGraph
  :functions [! helper1 helper2]
  :calls [(call ! helper1) (call helper1 !) ...]
  :recursive #t)
```

#### Day 20: Dep Graph Generation
- [ ] Create depgraph.c/depgraph.h
- [ ] Implement `generate_depgraph()` function
- [ ] Find all symbol references in body
- [ ] Build dependency edges
- [ ] Topological sort
- [ ] Detect circular dependencies
- [ ] Test with complex dependencies

**DepGraph Structure:**
```scheme
(⊝ DepGraph
  :symbols [! ? ≡ ⊗ ⊖]
  :depends [(dep ! ?) (dep ! ≡) (dep ! ⊗) ...]
  :order [? ≡ ⊗ ⊖ !]
  :cycles ∅)
```

#### Day 21: Integration
- [ ] Hook into `handle_define()` in eval.c
- [ ] Auto-generate all 4 graphs on function definition
- [ ] Store graphs in environment metadata
- [ ] Implement ⌂⟿, ⌂⇝, ⌂⊚, ⌂⊙ query primitives
- [ ] Test complete workflow
- [ ] Update documentation

**Integration Test:**
```scheme
(≔ ! (λ (n) (? (≡ n #0) #1 (⊗ n (! (⊖ n #1))))))

; Automatically generated:
(≔ cfg (⌂⟿ (⌜ !)))
(≔ dfg (⌂⇝ (⌜ !)))
(≔ cg (⌂⊚ (⌜ !)))
(≔ dg (⌂⊙ (⌜ !)))

; Query them
(⊢ (⊝? cfg CFG))
(⊢ (⊝? dfg DFG))
(⊢ (> (length (⊝→ cfg :nodes)) #0))
(⊢ (⊝→ cg :recursive))
```

**Deliverable:** Complete auto-generation system for all 4 graph types

---

## File Structure

```
bootstrap/
├── cell.h              # [MODIFY] Add CELL_STRUCT, CELL_GRAPH
├── cell.c              # [MODIFY] Extend constructors, refcount
├── structure.h         # [NEW] Structure primitive declarations
├── structure.c         # [NEW] Structure implementation (⊙, ⊚, ⊝)
├── graph.h             # [NEW] Graph primitive declarations
├── graph.c             # [NEW] Graph implementation
├── cfg.h               # [NEW] CFG generation
├── cfg.c               # [NEW] CFG implementation
├── dfg.h               # [NEW] DFG generation
├── dfg.c               # [NEW] DFG implementation
├── callgraph.h         # [NEW] Call graph generation
├── callgraph.c         # [NEW] Call graph implementation
├── depgraph.h          # [NEW] Dependency graph generation
├── depgraph.c          # [NEW] Dependency graph implementation
├── primitives.c        # [MODIFY] Register new primitives
├── eval.c              # [MODIFY] Auto-gen graphs in handle_define()
└── Makefile            # [MODIFY] Add new .o files
```

## Memory Management Strategy

### Reference Counting
- Structures hold strong refs to field values
- Graphs hold strong refs to nodes
- Edges may hold weak refs to avoid cycles

### Cycle Detection
```c
// graph.c
void break_graph_cycles(Cell* graph) {
    // Mark-and-sweep within graph to find cycles
    // Convert back-edges to weak references
    // Or use generational approach
}
```

### Cleanup
```c
void free_struct(Cell* cell) {
    StructData* s = &cell->data.structure;
    cell_release(s->type_tag);
    cell_release(s->variant);
    cell_release(s->fields);  // Alist
}

void free_graph(Cell* cell) {
    GraphData* g = &cell->data.graph;
    cell_release(g->nodes);
    cell_release(g->edges);
    cell_release(g->metadata);
    if (g->entry) cell_release(g->entry);
    if (g->exit) cell_release(g->exit);
}
```

## Testing Strategy

### Unit Tests
- Structure creation and access
- Graph construction
- Type checking
- Memory management

### Integration Tests
- Structures in expressions
- Graphs as function return values
- Auto-generation pipeline
- Query primitives

### Performance Tests
- Large structures
- Deep graphs
- Memory usage
- No leaks

## Success Criteria

**Phase 2C Complete When:**
- [ ] All 15 structure primitives implemented
- [ ] ⊙ (leaf), ⊚ (node), ⊝ (graph) all working
- [ ] CFG auto-generated on function definition
- [ ] DFG auto-generated on function definition
- [ ] Call graph auto-generated on function definition
- [ ] Dep graph auto-generated on function definition
- [ ] Query primitives (⌂⟿, ⌂⇝, ⌂⊚, ⌂⊙) working
- [ ] No memory leaks
- [ ] All tests passing
- [ ] Ready for pattern matching implementation

## Risk Assessment

### Low Risk ✅
- Cell type extensions (well-understood)
- Reference counting (already working)
- Structure primitives (straightforward)

### Medium Risk ⚠️
- Graph cycle handling (need careful design)
- CFG/DFG generation (complex algorithms)
- Performance impact (auto-generation overhead)

### High Risk 🔴
- Memory leaks in graphs (cycles)
- Integration complexity (many moving parts)
- Testing coverage (need exhaustive tests)

## Mitigation

1. **Start simple** - Get basic structs working first
2. **Test incrementally** - Each primitive gets tests
3. **Handle cycles early** - Don't defer to later
4. **Profile performance** - Measure auto-gen overhead
5. **Comprehensive tests** - Edge cases and stress tests

## Next Phase

After Phase 2C completes, we can begin **Phase 3A: Pattern Matching** with full knowledge of what we're matching on:
- Leaf structures (⊙)
- Node structures/ADTs (⊚)
- Graph structures (⊝)
- CFG/DFG/CallGraph/DepGraph

Pattern matching will destructure these structures cleanly.

---

**Ready to begin:** Week 1, Day 1 - Design CELL_STRUCT and CELL_GRAPH representations.
