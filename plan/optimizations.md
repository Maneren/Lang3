# Proposed Optimizations

Each entry describes a concrete improvement opportunity with the idea and relevant context.

---

## 1. Dead Nil-Push Elimination from Function Hoisting

**Files:** `src/compiler/src/compiler.cpp` — `compile_statements()` at lines 287-297

The function hoisting pass emits `OpConstant{nil}` + `add_local()` for every named function. Later, when the function body is compiled, `OpClosure{fn}` overwrites the local — the initial nil is never read but still consumes stack space and an instruction. Removing it requires a mechanism to pre-allocate stack slots without pushing values (e.g., `OpReserveN{n}` opcode) or changing let-bindings to use explicit `OpSetLocal`.

---

## 2. Eliminate Duplicate Expression Compilation in Chained Comparisons

**Files:** `src/compiler/src/compiler.cpp` — `compile_comparison()` at line 404

For `a < b < c`, the compiler generates `compile(b)` twice — once for each comparison pair. If `b` is a computation with side effects like `f(x)`, it runs twice. Fix: compile `b` once into a temporary local slot, then reference that slot in both comparisons. Only necessary when `b` has potential side effects.

---

## 3. Peephole Pass — Missing Patterns

**Files:** `src/compiler/src/peephole.cpp`

The peephole pass eliminates redundant instruction patterns post-compilation. Three patterns remain unimplemented:

- **Pattern A (dead nil-push):** `OpConstant{nil}; OpSetLocal{n}` when local `n` is never read between set and next write. Related to issue #1 above.
- **Pattern B (redundant GET/SET pair):** `OpGetLocal{n}; OpSetLocal{n}` — useless round-trip through the stack.
- **Pattern F (unreachable code elimination):** Dead code after unconditional `OpJump`, `OpReturn`, etc.

---

## 4. Function Call Optimization — Remaining Overhead

**Files:** `src/vm/src/vm.cpp` — `OpCall::execute_op()` at line 587

Three sources of overhead remain in function calls:

- **Function reference moved out and erased** before call (line 589-590), then args erased after call (line 671 via `clean_args()`). The return value could overwrite the first arg slot and adjust the stack pointer instead.
- **No tail call optimization:** `OpCall` never checks if the next instruction is `OpReturn` to reuse the current frame.
- **`evaluate()` function** at line 140 is only used from builtins but duplicates the call logic. Could be refactored to share code with the inline path.

---

## 5. Flatten Value Double-Variant Dispatch

**Files:** `src/runtime/src/value.cppm`, `src/runtime/src/stack_value.cppm`

`StackValue::visit()` dispatches on `{Nil, Primitive, GCValue*}`, then heap-typed values require a second dispatch inside the `GCValue` to reach the actual type (string/vector/function). A single flat variant (or NaN-boxing) would eliminate this double dispatch. Requires careful size management — `BytecodeFunction` (~80+ bytes) dominates inline storage.

---

## 6. GC Linear Sweep

**Files:** `src/runtime/src/storage.cppm` (line 12), `src/runtime/src/storage.cpp`

Sweep iterates a `ChunkedForwardList<GCValue, 1024>`, which has poor cache locality — each `GCValue` is a separate node across non-contiguous chunks. Options: free list with compaction using `std::vector<GCValue>`, bitmap marking with contiguous storage, or improving `ChunkedForwardList` traversal.

---

## 7. GC Write Barrier

**Files:** `src/runtime/src/storage.cppm`, `src/vm/src/vm.cpp`

For the current stop-the-world GC, no write barrier is needed. This would prepare for future generational/incremental GC by tracking stores to heap objects.

---

## 8. Compact Bytecode Encoding + Computed-Goto Dispatch

**Files:** `src/bytecode/src/bytecode.cppm`, `src/compiler/src/compiler.cpp`, `src/vm/src/vm.cpp`

Two problems:

- **Instruction size overhead:** Each instruction is a `std::variant` with 29 alternatives and `std::size_t` fields, averaging ~24-40 bytes. Compare to Lua: 4 bytes per instruction. Replace with flat `uint8_t` opcode + packed operands (e.g., fixed 8-byte or variable-width encoding using `append_uint24`/`read_uint24` helpers).
- **Dispatch overhead:** `std::visit` on a 29-alternative variant generates discriminant table lookups + indirect calls. A computed-goto dispatch (GCC/Clang `&&` label pointers) reads the opcode byte, indexes into a static label array, and jumps directly to the handler.

Combined impact: 6-10x smaller bytecode, 2-5x faster dispatch, ~2-3x throughput on tight loops.

---

## 9. NaN-Boxing / Tagged Pointers

**Files:** `src/runtime/src/value.cppm`, `src/runtime/src/stack_value.cppm`, all consumers

Replace the multi-byte `Value` with a single 64-bit `uint64_t` using IEEE 754 NaN payload bits. Non-NaN doubles are stored as-is. All other types use NaN representation with a 4-bit tag in bits 48-51 and the payload (pointer or immediate) in bits 0-47. Eliminates: 24-byte → 8-byte values, all `shared_ptr` ref-counting, double dispatch, and heap allocation for nil/bool/small-int. Type checks become single bitwise AND operations.

---

## 10. String Interning

**Files:** New `src/runtime/src/string_table.cppm`, `src/runtime/src/value.cppm`

Every string runtime operation creates a new `std::string`. String comparison is O(n). A global string table interns all strings into arena-allocated storage with precomputed hashes. `InternedString` handles enable O(1) comparison (pointer equality), faster hash table lookups for globals, and no per-string allocation overhead. Interned strings are permanent — the GC skips them during marking.
