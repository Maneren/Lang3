# Medium Complexity: Execution Speed Optimizations

These changes are interconnected within their domain but largely independent of each other. Each requires touching multiple files but follows a clear design.

---

## 1. Eliminate `shared_ptr<HeapValue>` from `Value`

**The Problem:**

`Value` uses `std::variant<Nil, Primitive, std::shared_ptr<HeapValue>>`. This has two critical performance flaws:

**a) Atomic reference counting overhead:**
Every copy, move, or destroy of a `Value` with a heap-allocated type incurs atomic RMW instructions (`__atomic_add_fetch` / `__atomic_sub_fetch`) on the `shared_ptr` control block. On x86-64, atomic operations cost ~15-30 cycles each and serialize the memory bus. In a tight interpreter loop, this adds significant overhead.

**b) Double memory management:**
The `shared_ptr` ref-counting is **redundant** with the GC mark-sweep. All heap values live inside `GCValue` objects in the chunked storage, and the GC is the one that determines liveness. The `shared_ptr` creates a separate, invisible root set that the GC cannot see, leading to:
- Memory leaks: GC reclaims `GCValue` but `shared_ptr` refcount keeps `HeapValue` alive, leaking it.
- Missed collection: `shared_ptr` pins `HeapValue` even when the GC says it's unreachable.

**The Fix:**

Replace `std::shared_ptr<HeapValue>` with a raw handle into GC storage. Options:

**Option A — Raw `GCValue*` pointer (recommended):**
```cpp
class Value {
    std::variant<Nil, Primitive, GCValue*> inner;
    // ...
};
```
- `GCValue*` is just 8 bytes (same as the pointer part of `shared_ptr`, but without the control block).
- No atomic ref-count operations.
- The `GCValue` manages the `HeapValue` directly (as its `Value` member).
- **Ownership model**: `Value` does NOT own the `HeapValue`. The GC owns all values. `Value` is just a reference.
- **Lifetime**: As long as a `Value` with a `GCValue*` exists on the stack/frames/globals, it acts as a GC root. The `run_gc()` mark phase already traces all these locations, so no change to rooting.

**Option B — Use `Ref` directly:**
`Ref` is already `std::reference_wrapper<GCValue>` (a raw pointer). Make heap-typed `Value` hold a `Ref` instead of `shared_ptr<HeapValue>`. But `Ref` requires the `GCValue` to be alive (iterating GC values), which has subtle lifetime implications for the `Ref` itself.

**Impact Analysis:**

**Files to modify:**
| File | Changes |
|------|---------|
| `src/runtime/src/value.cppm` | Change `inner` variant from `shared_ptr<HeapValue>` to `GCValue*`. Update constructors, accessors, `visit()`, `as_*` methods. |
| `src/runtime/src/value.cpp` | Remove all `shared_ptr` construction/destruction. Simplify `add`, `index`, `type_name`, `compare`, `is_truthy`, `format`, `hash`. |
| `src/runtime/src/gc_value.cppm` | Possibly rename `get_value()` / `get_value_mut()` if changing access patterns. |
| `src/runtime/src/gc_value.cpp` | `GCValue::mark()` — already uses `get_value_mut().visit(...)`. No change needed. |
| `src/runtime/src/storage.cppm` | `GCStorage` — ensure `emplace()` returns something useful. Currently pushes on chunked list. |
| `src/runtime/src/ref_value.cpp` | `Ref::get()` already returns `Value&`. May need to add a method that returns `GCValue&` directly. |
| `src/vm/src/vm.cpp` | All `execute_op` functions that manipulate `Value` objects. Check for any `shared_ptr`-specific code. |
| `src/vm/src/builtins.cpp` | Constructors like `Value(std::string)`, `Value(std::vector<Ref>)`. |
| `src/compiler/src/compiler.cpp` | `make_constant()` — constructs `Value` objects for the constant pool. |
| `src/bytecode/src/bytecode.cppm` | `ProgramBytecode::constants` is `std::vector<runtime::GCValue>` — this stays the same. |

**Key changes in `value.cppm`:**

```cpp
// Before:
class Value {
    std::variant<Nil, Primitive, std::shared_ptr<HeapValue>> inner;
    
    Value(Function &&function)
        : inner{std::make_shared<HeapValue>(std::move(function))} {}
};

// After:
class Value {
    std::variant<Nil, Primitive, GCValue*> inner;
    
    // No more shared_ptr construction
    // GCValue* is set when a value is emplaced in GC storage
    // or retrieved via Ref
};
```

**Building `Value` from heap types:**

Currently heap-typed values are created with `std::make_shared<HeapValue>(...)`. After the change, the creation must go through `GCStorage`:

```cpp
// Before:
Value::Value(std::string s)
    : inner{std::make_shared<HeapValue>(std::move(s))} {}

// After:
// Option 1: Value becomes a "view" — construct from Ref
Value::Value(const Ref &ref)
    : inner{&ref.get_gc()} {}

// Option 2: GCStorage creates the GCValue and returns a handle
// The caller does:
Ref ref = gc_storage.emplace(std::string("hello"));
Value val(ref);
```

**The `HeapValue` class itself:**
May become private to the GC storage implementation, or simplified. Since `HeapValue` is always accessed through `GCValue → Value → HeapValue*`, and `GCValue::get_value()` returns `Value&`, we access the inner variant of `HeapValue` through the existing `Value::visit()` mechanism. The `shared_ptr` removal simplifies the chain to `GCValue* → GCValue → Value → HeapValue*` (one less indirection).

**Testing:**
- All existing tests must pass (snapshot tests verify bytecode and output).
- No behavioral change to the language semantics.
- Run with `valgrind` / `-fsanitize=address` to catch any new lifetime bugs.

---

## 2. Flatten `Value`'s Double-Variant Dispatch

**The Problem:**

`Value::visit()` does a `std::visit` on the outer variant (`Nil`/`Primitive`/`shared_ptr<HeapValue>`), then inside the heap case, does a *second* `std::visit` on `HeapValue::inner` (`unique_ptr<Function>` / `vector<Ref>` / `string`). This adds dispatch overhead for every operation on heap types.

Current dispatch for `value.add(rhs)`:
```
1. outer std::visit on (Nil | Primitive | shared_ptr<HeapValue>)
2. if heap: inner std::visit on (unique_ptr<Function> | vector<Ref> | string)
3. plus the shared_ptr<HeapValue> dereference
```

**The Fix (Option A — Merge variants, recommended if shared_ptr removal is done):**

Merge `HeapValue`'s inner types directly into `Value`'s variant:

```cpp
// Before:
class HeapValue {
    std::variant<std::unique_ptr<Function>, std::vector<Ref>, std::string> inner;
};
class Value {
    std::variant<Nil, Primitive, std::shared_ptr<HeapValue>> inner;
};

// After (with shared_ptr → raw pointer change from §1):
class Value {
    std::variant<
        Nil,
        Primitive,
        Function,        // by value, not unique_ptr
        std::vector<Ref>,
        std::string
    > inner;
};
```

Now `Value::visit()` has a single `std::visit` with 5 alternatives. No double dispatch.

**Key considerations:**

1. **`Function` by value**: Currently `HeapValue::inner` holds `std::unique_ptr<Function>`. Flattening means `Function` is stored directly in the variant. `Function` contains either a `BuiltinFunction` (lightweight — a `std::function`) or a `BytecodeFunction` (contains `std::vector`s, `std::string`, etc.). The variant's largest member sets the size.
   - `BytecodeFunction` with its vectors: ~64-80 bytes.
   - `std::string`: ~32 bytes (with SSO).
   - `std::vector<Ref>`: ~24 bytes.
   - `Primitive`: ~16 bytes.
   - Nil: ~1 byte.
   
   The variant size is dominated by `BytecodeFunction` → ~80 bytes plus variant discriminant → ~96 bytes per `Value`. This is **much larger** than the current 24 bytes.

2. **Large `Value`**: A 96-byte `Value` means the VM stack (`std::vector<Value>`) uses 4x more memory. This impacts cache behavior negatively. Most stack slots are primitives (24 bytes currently) but with flattening they're 96 bytes. **This may be worse overall.**

3. **Alternative**: Keep `Function` heap-allocated but still remove the `shared_ptr` overhead. Use a separate pointer or handle for functions.

**The Fix (Option B — Smart variant layout with custom storage):**

Use a union-like structure where small types are inline and large types are indirectly stored:

```cpp
class Value {
    enum class Tag : uint8_t {
        Nil, Bool, Int, Float,
        String, Vector, Function
    };
    
    struct LargeValue {
        std::variant<std::string, std::vector<Ref>, Function> inner;
    };
    
    Tag tag;
    union {
        bool bool_val;
        int64_t int_val;
        double float_val;
        LargeValue *heap_val;  // separate heap allocation for large types
    };
};
```

This is essentially a manual `std::variant` with size optimization. But it's error-prone and duplicates what std::variant already does.

**Recommended approach:**

Given the size concern with Option A, the best approach is a **staged plan**:

**Stage 1:** Remove `shared_ptr<HeapValue>` → `GCValue*` (§1 above). This alone eliminates the double variant dispatch because the inner `Value` inside `GCValue` is the one that does dispatch. The outer `Value` just holds a `GCValue*` — dispatching on a pointer type is trivial (just check pointer against null/non-null, or use the variant tag).

Wait — actually, with `GCValue*` in the variant, the dispatch becomes:
- `Nil` → immediate
- `Primitive` → immediate
- `GCValue*` → need to dereference to get the actual type (string/vector/function)

So there's still an indirection for heap types. The outer dispatch is still double-shallow (3-way switch), but the heap case requires an extra memory access to reach the `GCValue`.

**Stage 2:** Add small object optimization — `Value` stores small strings and vectors inline.

```cpp
class Value {
    // Small string optimization: strings up to N chars are stored inline
    static constexpr size_t SmallStringCapacity = 14; // 14 chars + 1 byte tag
    
    struct InlineString {
        char data[SmallStringCapacity];
        uint8_t size;
    };
    
    std::variant<
        Nil,
        Primitive,
        InlineString,
        std::string,        // large strings
        std::vector<Ref>,   // always heap for now (could also be SSO)
        Function            // always heap allocated
    > inner;
};
```

**Verification:** Benchmark a tight loop performing string concatenation and function calls. Measure throughput and cache misses before and after.

---

## 3. Peephole Optimizer Pass

**Files to create/modify:**
- `src/compiler/CMakeLists.txt` — add new source file
- `src/compiler/src/peephole.cppm` — module declaration
- `src/compiler/src/peephole.cpp` — implementation

**The Problem:**
The compiler emits straightforward bytecode with known redundant patterns that can be eliminated with a post-processing pass over each chunk's instruction stream.

**Design principle:** A peephole pass is the canonical example of a centralized solution. Instead of scattering if-checks across the compiler (`if count > 0 then emit Pop` in `end_scope`, `compile_continue`, etc.), a single pass catches all instances of a pattern regardless of which compiler function produced them. This also future-proofs against new code introducing the same inefficiency.

**Identified Patterns:**

### Pattern A: Dead `OpConstant{nil}` + `OpSetLocal{n}` from function hoisting
```
CONSTANT nil        ← never read
SET_LOCAL 0
... other code ...
CONSTANT fn         ← real function
SET_LOCAL 0         ← overwrites the nil
```

**Rule:** If `OpConstant{nil}` is immediately followed by `OpSetLocal{n}`, and the next write to local `n` before any read of local `n`, remove the nil-push+set pair.

### Pattern B: `OpGetLocal{n}` + same `OpSetLocal{n}` with no intervening read of `n`
```
GET_LOCAL 0         ← read local
CONSTANT 1          ← some value
ADD                 ← compute
SET_LOCAL 0         ← store back
```
This is the operator-assignment pattern `x += 1`. The GET/SET pair is necessary for operator assignments, but if the local is never captured (no closure), we could compile `x += 1` differently. However, this pattern is hard to optimize post-hoc.

More obvious case is when GET/SET are on the same slot with **no modification in between** — a useless round-trip through the stack:
```
GET_LOCAL 0         ← redundant
SET_LOCAL 0         ← redundant (value unchanged)
```
This should never be emitted in practice, but if found, remove both instructions.

### Pattern C: `OpConstant{x}` + `OpNegate` → `OpConstant{-x}`
```
CONSTANT 5          ← constant
NEGATE              ← negate
→ fold to:
CONSTANT -5         ← folded constant
```

### Pattern D: `OpPop{0}` removal
```
POP 0               ← does nothing
→ remove entirely
```

### Pattern E: Jump-to-jump chains
```
JUMP L1
...
L1: JUMP L2
→ fold to:
JUMP L2
```
and:
```
JUMP_IF_FALSE L1
...
L1: JUMP L2
→ fold to:
JUMP_IF_FALSE L2
```

### Pattern F: Unreachable code elimination
After an unconditional `OpJump`, `OpReturn`, `OpPop {count}` (where count pops all remaining stack values, implying a return), or `OpJumpIf` with both branches leading to the same target — all instructions until the next jump target are dead.

```
JUMP L1
CONSTANT 42          ← dead (never reached)
POP 1
L1: ...
→ remove CONSTANT 42 and POP 1
```

### Pattern G: `OpConstant{x}` + `OpJumpIf{true}` → `OpJump` (unconditional)
```
CONSTANT true
JUMP_IF_TRUE L1      ← always taken
→ fold to:
JUMP L1
```
Similarly for `OpConstant{false}` + `OpJumpIf{false}`.

### Pattern H: Remove `OpPop{0}` no-ops
```
POP 0               ← does nothing
→ remove entirely
```
This handles `OpPop{0}` from any source (end_scope, continue, or future code) in one rule instead of scattering if-checks across the compiler.

### Pattern I: Constant folding for arithmetic
When both operands of an arithmetic op are `OpConstant` with numeric primitives:
```
CONSTANT 5
CONSTANT 3
ADD
→ fold to:
CONSTANT 8
```
Also fold `OpConstant{x}; OpNegate` → `OpConstant{-x}` and `OpConstant{x}; OpConstant{y}; OpSub` → `OpConstant{x - y}`, etc.

This requires evaluating the folded result and adding it to the constant pool (or replacing the constant index). Unlike compile-time folding, this catches cases where constant operands are introduced by other peephole rules (e.g., after inlining, constant propagation).

### Pattern J: Fold `OpConstant{x}; OpNot` on boolean constants
```
CONSTANT true
NOT
→ fold to:
CONSTANT false
```

**Implementation Design:**

```cpp
// peephole.cpp
void optimize_chunk(Chunk &chunk, ProgramBytecode &program) {
    bool changed = true;
    while (changed) {
        changed = false;
        // Multiple passes until no more changes
        for (size_t i = 0; i < chunk.code.size(); i++) {
            changed |= try_constant_folding(i, chunk, program);
            changed |= try_dead_store_elimination(i, chunk, program);
            changed |= try_jump_chain_elimination(i, chunk, program);
            changed |= try_unreachable_code_elimination(i, chunk, program);
            changed |= try_noop_elimination(i, chunk, program);
        }
    }
}
```

**Integration:**

Call `optimize_chunk()` from `Compiler::finalize_chunk()` (or the end of `compile()`) before `deduplicate_constants()`:

```cpp
void Compiler::compile() {
    // ... existing compilation ...
    
    // NEW: peephole optimization
    for (auto &chunk : program.chunks) {
        optimize_chunk(chunk, program);
    }
    
    deduplicate_constants();
}
```

**Relationship with low-hanging fruit optimizations:**
The peephole pass **subsumes** several optimizations from `01_low_hanging_fruit.md`: OpPop{0} removal (Pattern H), dead nil-push elimination (Pattern A), and constant folding (Pattern I). Those entries now delegate to this peephole pass. Implementing the peephole pass makes those scattered if-checks unnecessary.

**Verification:**
- Run all snapshot tests. Expected bytecode files will change (instructions removed/reordered).
- Check that outputs are identical before and after (no semantic change).
- Sample snapshot to check: `functions_closures`, `control_flow`, `comparisons_and_logic`, `range_for`.

**Test cases to create:**
- `peephole_const_fold.l3`: `print(1 + 2)` → should fold to `CONSTANT 3`
- `peephole_dead_code.l3`: `return 1; print(2)` → the `print(2)` should be removed
- `peephole_jump_chain.l3`: nested control flow that creates jump-to-jump chains

---

## 4. GC Improvements

**Files:**
- `src/runtime/src/storage.cppm` — `GCStorage`
- `src/runtime/src/storage.cpp` — sweep logic
- `src/runtime/src/gc_value.cppm` — `GCValue`
- `src/vm/src/vm.cpp` — `maybe_gc()`, `run_gc()`

### 4a. Adaptive GC Threshold

**Problem:**
The GC triggers every 16,384 allocations (`GC_INTERVAL = 16 * 1024`), regardless of live heap size. This means:
- Small heaps: GC runs too often (wasting time when few objects are actually garbage).
- Large heaps: GC runs too infrequently relative to allocation rate (allowing heap to grow unboundedly).

**Fix:**
Track live heap size after each sweep. Set the next threshold based on that:

```cpp
class GCStorage {
    std::size_t live_size = 0;      // objects remaining after last sweep
    std::size_t next_gc_threshold = 1024;  // initial threshold
    std::size_t added_since_last_sweep = 0;
    
    bool should_collect() const {
        return added_since_last_sweep >= next_gc_threshold;
    }
};

// After sweep:
void GCStorage::sweep() {
    // ... existing sweep ...
    live_size = size - erased_count;
    // Next collection: trigger when we've allocated ~2x the live size
    next_gc_threshold = std::max(live_size * 2, std::size_t(1024));
    added_since_last_sweep = 0;
}
```

This is a simple "heap growth factor" heuristic, similar to how generational GCs and `std::vector` grow. The factor of 2 means worst-case memory usage is ~2x live set (tunable: 1.5x for memory-constrained, 3x for throughput).

### 4b. Linear Sweep Instead of Forward-List Iteration

**Problem:**
The sweep iterates a `std::forward_list` via `ChunkedForwardList`, which has poor cache locality. Each `GCValue` is a separate node, and nodes within a chunk are contiguous only at allocation time. After many deallocations and reallocations, the forward-list traversal jumps around memory.

**Fix:**
Replace (or augment) the `ChunkedForwardList` with a dense array where dead slots are marked and reclaimed eagerly. Two approaches:

**Option A — Free list with compaction:**
Keep a `std::vector<GCValue>` for the main storage. Mark dead objects. Compact by moving live objects to the front and updating `Ref` pointers. This is O(n) but much more cache-friendly.

**Option B — Bitmap marking:**
Use a contiguous `std::vector<GCValue>` with a separate `std::vector<bool>` mark bitmap. The sweep iterates the bitmap (cache-friendly scan) and frees unmarked slots. The slots are reused via a free list.

```cpp
class GCStorage {
    std::vector<GCValue> objects;     // dense storage
    std::vector<bool> mark_bits;      // aligned bitmap
    std::vector<size_t> free_slots;   // stack of free indices
    
    size_t allocate_slot() {
        if (!free_slots.empty()) {
            auto idx = free_slots.back();
            free_slots.pop_back();
            return idx;
        }
        auto idx = objects.size();
        objects.emplace_back();
        mark_bits.push_back(false);
        return idx;
    }
    
    void sweep() {
        for (size_t i = 0; i < objects.size(); i++) {
            if (!mark_bits[i]) {
                objects[i].~GCValue();  // destroy
                free_slots.push_back(i); // reclaim
            }
            mark_bits[i] = false; // reset for next cycle
        }
    }
};
```

**Option C — Keep `ChunkedForwardList` but improve cleanup:**
Instead of running cleanup only every `2 * ChunkSize` deallocations, run it more aggressively when the fraction of empty chunks exceeds a threshold.

### 4c. Write Barrier for Stack Stores

**Problem:**
When `OpSetLocal` or `OpSetUpvalue` writes to a captured local, the write goes directly to the `GCValue`'s `Value` member without any GC barrier. If we ever move to generational or incremental GC, this will cause missed references.

**Fix (for future generational GC):**
Add a simple write barrier in `GCValue::set_value()`:

```cpp
void GCValue::set_value(const Value &new_value) {
    // If this object is in the old generation and the new value
    // points to a young-generation object, record this as a cross-generational pointer
    if (is_old && gc.is_young(new_value)) {
        gc.remember(this);
    }
    value = new_value;
}
```

For current stop-the-world GC, no write barrier is needed. This is preparation for future improvements.

**Verification:**
- Existing tests must pass (no semantic change to GC behavior).
- Measure with `perf stat` before/after: reduced GC time, better heap utilization.
- Test allocation-heavy workloads (e.g., building a large vector in a loop).

**Related bugs:** See `plan/04_bugs.md`. In particular, Bug #2 (missing `captured_upvalue_refs` tracing in GC mark phase) must be fixed before or alongside any GC improvements. Any new GC bugs discovered during implementation should be added to `plan/04_bugs.md` with reproducer, root cause, and fix.

---

## 5. Function Call Optimization: Direct Stack Argument Passing

**Files:**
- `src/vm/src/vm.cpp` — `OpCall::execute_op()`, `evaluate()`, `BytecodeVM::execute_loop()`
- `src/vm/src/vm.cppm` — `CallFrame`, `BytecodeVM`

**The Problem:**
`OpCall` copies arguments from the stack into a `std::vector<runtime::Value>`, then calls `evaluate()` which erases them from the stack and pushes them back. This involves:

1. Loop over args to copy into vector (OpCall, lines ~611-618):
```cpp
std::vector<Value> args;
for (std::size_t i = 0; i < op.arg_count; i++) {
    args.push_back(stack_top(op.arg_count - 1 - i));
}
```
2. Erase args from stack (same function):
```cpp
for (std::size_t i = 0; i < op.arg_count; i++)
    stack.pop_back();
```
3. In `evaluate()`, push them back again:
```cpp
for (auto &arg : args)
    stack.push_back(std::move(arg));
```

This is 3 traversals of the argument list per call.

**The Fix:**

Use the stack **in place** for argument passing. The callee frame's `frame_pointer` already establishes the base of locals. If arguments are passed on the stack above the current frame's locals, the callee can access them directly without copying.

**Design:**

```
Stack layout before call:
[locals of caller] [arg1] [arg2] ... [argN] [top]

After call:
[locals of caller] [arg1] [arg2] ... [argN] [callee locals...]
                     ^-- callee frame_pointer points here
```

Arguments are treated as the first locals of the callee. The `evaluate()` method (or inline in `execute_op(OpCall)`) simply:

1. Push a new `CallFrame` with `frame_pointer = stack.size() - op.arg_count`.
2. The callee's `OpGetLocal{0}` accesses the first argument, `OpGetLocal{1}` the second, etc.
3. No copying, no erasing, no re-pushing.

**For currying:**
When `total_args < arity`, curried args must be stored. Store them in the `BytecodeFunction::curried_args` (already a `std::vector<Ref>`). The callee frame's first locals are a mix of curried args and new args — the `evaluate()` method constructs the initial stack region:

```cpp
// In evaluate() or OpCall handler:
auto &bc_func = function.as_bytecode_function();
std::size_t arg_start = stack.size() - new_arg_count;

// If there are curried args, place them first, then new args
if (bc_func.has_curried_args()) {
    // Remove new args from stack temporarily
    std::vector<Value> new_args(stack.end() - new_arg_count, stack.end());
    stack.erase(stack.end() - new_arg_count, stack.end());
    
    // Push curried args (from GC storage) onto the stack
    for (auto &curried_ref : bc_func.curried_args) {
        stack.push_back(curried_ref.get());
    }
    
    // Push new args after curried args
    for (auto &arg : new_args) {
        stack.push_back(std::move(arg));
    }
    
    bc_func.curried_args.clear(); // consumed
}

frames.push_back(CallFrame{
    .chunk_id = bc_func.id,
    .ip = 0,
    .frame_pointer = current_local_base,
    // ...
});
```

**For return values:**
The callee places its return value at `stack[frame_pointer]` (overwriting the first argument/local slot). The caller then adjusts its stack top to just past this value.

**Tail call optimization:**
When `return f(args)` is detected in the bytecode (a call immediately followed by `OpReturn`), reuse the current frame instead of pushing a new one. Detect this in `OpCall::execute_op()`:

```cpp
// If the next instruction is OpReturn and we're in a tail position
if (is_tail_call(frame)) {
    // Reset current frame's IP/chunk_id instead of creating a new frame
    frame.chunk_id = target_chunk_id;
    frame.ip = 0;
    // Arguments are already on the stack above frame_pointer
    // Reset the frame's locals
    continue; // re-enter execute_loop
}
```

**For `OpReturn`:**
Currently returns by popping the frame and pushing the return value. With the new scheme:

```cpp
void BytecodeVM::execute_op(const OpReturn &, CallFrame &frame) {
    auto return_value = stack.back();
    stack.pop_back();
    
    // Pop this frame's locals (everything from frame_pointer to current top)
    stack.resize(frame.frame_pointer);
    
    // Push return value back
    stack.push_back(std::move(return_value));
    
    frames.pop_back();
}
```

**Implementation checklist:**
1. Change `OpCall` to set up frame with `frame_pointer = stack.size() - arg_count`.
2. Remove argument copying in `OpCall::execute_op()`.
3. Change `evaluate()` to not copy/push arguments; instead initialize frame with arguments in place.
4. Change `OpReturn` to pop locals up to `frame_pointer`, leaving the return value at the top.
5. Handle currying by extending the stack with curried args before the new args.
6. Implement tail call optimization in `OpCall` when followed by `OpReturn`.
7. Update `OpGetLocal`/`OpSetLocal` — they already use `frame_pointer + index`, so no change needed.

**Potential issues:**
- **Stack growth**: With currying, the stack may grow because curried args are pushed. Previously they were stored in a vector and copied. With the new scheme, the stack must hold them alongside regular frame data. The stack could fragment if many closures are curried.
- **Frame pointer stability**: When a closure captures a local, it stores a `Ref` to a GC-value copy of that local (already done in the VM). The `frame_pointer` is used for `OpGetLocal`/`OpSetLocal` only within the current frame. No change needed.

**Verification:**
- All function call snapshot tests must produce identical output.
- The `curry` snapshot test must continue to work correctly.
- Recursive functions (fibonacci, factorial) should test deep call stacks.
- Benchmark: a tight loop calling a simple function should show measurable improvement.
