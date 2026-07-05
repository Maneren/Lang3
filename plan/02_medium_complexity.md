# Medium Complexity: Execution Speed Optimizations

These changes are interconnected within their domain but largely independent of each other. Each requires touching multiple files but follows a clear design.

---

## 1. Eliminate `shared_ptr<HeapValue>` from `Value`

**Status:** Not Implemented

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
  std::variant<Nil, Primitive, GCValue *> inner;
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
  std::variant<Nil, Primitive, GCValue *> inner;

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
Value::Value(const Ref &ref) : inner{&ref.get_gc()} {}

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

**Status:** Not Implemented (depends on §1 — shared_ptr removal must come first)

**The Problem:**

`Value::visit()` does a `std::visit` on the outer variant (`Nil`/`Primitive`/`shared_ptr<HeapValue>`), then inside the heap case, does a _second_ `std::visit` on `HeapValue::inner` (`unique_ptr<Function>` / `vector<Ref>` / `string`). This adds dispatch overhead for every operation on heap types.

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
      Function, // by value, not unique_ptr
      std::vector<Ref>,
      std::string>
      inner;
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
  enum class Tag : uint8_t { Nil, Bool, Int, Float, String, Vector, Function };

  struct LargeValue {
    std::variant<std::string, std::vector<Ref>, Function> inner;
  };

  Tag tag;
  union {
    bool bool_val;
    int64_t int_val;
    double float_val;
    LargeValue *heap_val; // separate heap allocation for large types
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
      std::string,      // large strings
      std::vector<Ref>, // always heap for now (could also be SSO)
      Function          // always heap allocated
      >
      inner;
};
```

**Verification:** Benchmark a tight loop performing string concatenation and function calls. Measure throughput and cache misses before and after.

---

## 3. Peephole Optimizer Pass

**Status:** Partially Implemented

**Files:**

- `src/compiler/src/peephole.cppm` — module declaration (existing)
- `src/compiler/src/peephole.cpp` — implementation (existing)
- `src/compiler/src/compiler.cpp` — integration at line 9 (`import :peephole`) and line 46 (`optimize(program)`)

**The Problem:**
The compiler emits straightforward bytecode with known redundant patterns that can be eliminated with a post-processing pass over each chunk's instruction stream.

**Design principle:** A peephole pass is the canonical example of a centralized solution. Instead of scattering if-checks across the compiler (`if count > 0 then emit Pop` in `end_scope`, `compile_continue`, etc.), a single pass catches all instances of a pattern regardless of which compiler function produced them. This also future-proofs against new code introducing the same inefficiency.

**Current implementation:**

The peephole pass exists at `src/compiler/src/peephole.cpp` and is called from `Compiler::compile()` at `compiler.cpp:46` after all compilation and before `deduplicate_constants()`.

**Implemented patterns (5 of 10):**

| Pattern                                                                       | Implementation        | File:Line          |
| ----------------------------------------------------------------------------- | --------------------- | ------------------ |
| C: `OpConstant{x}` + `OpNegate/OpNot` → `OpConstant{folded}`                  | `match_unary_fold`    | `peephole.cpp:150` |
| D/H: `OpPop{0}` removal                                                       | `match_pop_zero`      | `peephole.cpp:119` |
| E: Jump-to-jump chains                                                        | `match_jump_chaining` | `peephole.cpp:244` |
| G: `OpConstant{bool}` + `OpJumpIf` → `OpJump` or remove                       | `match_const_jumpif`  | `peephole.cpp:179` |
| I: `OpConstant{a}` + `OpConstant{b}` + arithmetic → `OpConstant{folded}`      | `match_binary_fold`   | `peephole.cpp:204` |
| J: `OpConstant{x}` + `OpNot` (handled by `match_unary_fold` via `fold_unary`) | `match_unary_fold`    | `peephole.cpp:150` |

Additional pattern implemented that was not in the original plan:

- **Zero-jump removal** (`match_zero_jump`, `peephole.cpp:132`): Removes `OpJump{next_instruction}` (jump to self, no-op).

**Missing patterns:**

| Pattern                                             | Description                                                                          |
| --------------------------------------------------- | ------------------------------------------------------------------------------------ |
| A: Dead `OpConstant{nil}` + `OpSetLocal{n}`         | Function hoisting dead store — nil is pushed but never read before being overwritten |
| B: Redundant `OpGetLocal{n}` + same `OpSetLocal{n}` | Useless round-trip through stack                                                     |
| F: Unreachable code elimination                     | Dead code after unconditional `OpJump`, `OpReturn`, etc.                             |

**Integration:**

```cpp
// In Compiler::compile() (compiler.cpp:46):
optimize(program);
```

**Relationship with low-hanging fruit optimizations:**
The peephole pass **subsumes** several optimizations from `01_low_hanging_fruit.md`: OpPop{0} removal (Pattern D/H) and constant folding (Pattern C/I/J). Dead nil-push elimination (Pattern A) from function hoisting (LF #2) was planned but never implemented — the peephole pass is the right place for it.

**Verification:**

- Run all snapshot tests. Bytecode outputs may change (instructions removed/reordered).
- Check that program outputs are identical (no semantic change).

**Remaining work:**

- Add Pattern A (dead nil-push from function hoisting)
- Add Pattern F (unreachable code elimination)
- Consider adding Pattern B (redundant GET/SET pair)

---

## 4. GC Improvements

**Status:** Partially Implemented (4b, 4c not done)

**Files:**

- `src/runtime/src/storage.cppm` — `GCStorage`
- `src/runtime/src/storage.cpp` — sweep logic
- `src/runtime/src/gc_value.cppm` — `GCValue`
- `src/vm/src/vm.cpp` — `maybe_gc()`, `run_gc()`

### 4b. Linear Sweep Instead of Forward-List Iteration — **Not Implemented**

**Problem:**
The sweep still iterates a `ChunkedForwardList<GCValue, 1024>` (`storage.cppm:12`), which has poor cache locality. Each `GCValue` is a separate node, and nodes across chunks are not contiguous.

**Fix options (unchanged from original plan):**

- Option A: Free list with compaction using `std::vector<GCValue>`
- Option B: Bitmap marking with contiguous storage
- Option C: Improve `ChunkedForwardList` cleanup

The `ChunkedForwardList` is still used at `storage.cppm:12`. No conversion to dense storage has been made.

### 4c. Write Barrier for Stack Stores — **Not Implemented**

For current stop-the-world GC, no write barrier is needed. This is preparation for future generational/incremental GC. No code changes have been made.

**Verification:**

- Existing tests must pass (no semantic change to GC behavior).
- Measure with `perf stat` before/after: reduced GC time, better heap utilization.

## 5. Function Call Optimization: Direct Stack Argument Passing

**Status:** Partially Implemented (vector copy eliminated; function ref still moved, args still erased)

**Files:**

- `src/vm/src/vm.cpp` — `OpCall::execute_op()` at line 604
- `src/vm/src/vm.cppm` — `CallFrame`, `BytecodeVM`

**The Problem (original):**
`OpCall` copied arguments from the stack into a `std::vector<runtime::Value>`, then called `evaluate()` which erased them from the stack and pushed them back — 3 traversals of the argument list per call.

**Current implementation:**

The `OpCall::execute_op()` handler at `vm.cpp:604-686` has been partially optimized:

- **No vector copy**: Arguments stay on the stack and are accessed via `std::span` (`vm.cpp:612-614`), avoiding the copy-to-vector step.
- **Frame setup**: The new frame uses `base = stack.size() - op.arg_count` as the `frame_pointer` (`vm.cpp:611,650`), so callee accesses arguments directly via `OpGetLocal{0}` = `stack[frame_pointer + 0]`.
- **Curried args**: Shuffled on the stack in place (`vm.cpp:658-673`) rather than in a separate vector.

**Remaining overhead:**

1. **Function reference moved out and erased** (`vm.cpp:607-608`): `auto func_ref = std::move(stack[func_pos])` followed by `stack.erase(...)` — the function reference is removed from the stack before the call.
2. **Args erased after call** (`vm.cpp:681` via `clean_args()` at line 617): After the call returns, the argument region is erased from the stack.
3. **No tail call optimization**: `OpCall` never checks if the next instruction is `OpReturn` to reuse the current frame.
4. **`evaluate()` function** (`vm.cpp:141-192`): Still exists but is NOT called from `OpCall::execute_op()` for `BytecodeFunction` — the inline path handles everything. It may still be used from builtins.

**Remaining work (implementation checklist):**

1. Eliminate function ref move+erase before call.
2. Eliminate `clean_args()` erasure after call — let the return value overwrite the first arg slot and adjust the stack pointer.
3. Implement tail call optimization when `OpCall` is followed by `OpReturn`.
4. Clean up or remove `evaluate()` if no longer needed.

**Verification:**

- All function call snapshot tests must produce identical output.
- The `curry` snapshot test must continue to work correctly.
- Recursive functions (fibonacci, factorial) should test deep call stacks.
