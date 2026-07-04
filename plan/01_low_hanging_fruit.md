# Low-Hanging Fruit: Execution Speed Optimizations

These are isolated changes with low risk and clear benefits.

**Design principle:** Prefer centralized solutions (peephole pass over bytecode) over scattering if-checks across the compiler. A peephole pass catches all instances of a pattern in one place, doesn't increase compiler complexity, and applies even to patterns introduced by future code changes. Only use direct checks in the compiler when:
- The peephole pass cannot detect the pattern (e.g., requires AST context, not just bytecode pattern matching).
- The centralized approach would require significantly more code than a direct check.

---

## 1. Eliminate `OpPop{0}` No-Ops

**Status:** Implemented

**Files:**
- `src/compiler/src/compiler.cpp` — `end_scope()` at line 89, `compile_continue_statement()` at line 1024
- `src/compiler/src/peephole.cpp` — `match_pop_zero` at line 119

**Implementation:**
- The `if (count > 0)` guard was removed from `end_scope()` — `OpPop{0}` is now always emitted unconditionally.
- `match_pop_zero` in the peephole pass catches all `OpPop{0}` instructions and removes them.
- `compile_continue_statement()` also emits `OpPop` unconditionally (relying on the peephole pass).
- The fallback return check in `compile_function_body()` was updated to scan past trailing `OpPop` instructions when checking if a function already ends with `RETURN`, preventing spurious dead-code emission.

**Design principle:** Fully centralized in the peephole pass — no scattered if-checks in the compiler.

**Verification:** Snapshot tests no longer show `POP 0` lines.

---

## 2. Dead Nil-Push Elimination from Function Hoisting

**Status:** Deferred — needs stack layout rearchitecture

**Files:**
- `src/compiler/src/compiler.cpp` — `compile_statements()` at lines 286-302 (hoisting pass)

**Problem (confirmed):**
The function hoisting pass emits `OpConstant{make_constant({})}` (nil) then `add_local()` for every named function. Later, when the function body is compiled, `OpConstant{fn}` / `OpClosure{fn}` + `OpSetLocal{slot}` overwrites the local. The initial nil value is never read.

**Attempted fix — tried and reverted:**
Removing the nil-push naively causes runtime crashes. The nil-push serves a dual role:
1. **Pre-allocate stack slots:** Local variables are stored at fixed `stack[fp + index]` positions. The nil-push grows the stack to ensure those positions exist.
2. **Initialize slot values:** `let x = expr` leaves the expression result on the stack AS the local's value — there is no `SET_LOCAL` for let-bindings. The nil-push ensures the stack is the correct size before the first let-binding's expression is evaluated.

Without the nil-push, later `GET_LOCAL` instructions try to read from non-existent stack positions. Simply making `GET_LOCAL`/`SET_LOCAL`/`OpClosure` resize the stack (as a safety net) is insufficient because the STACK LAYOUT is misaligned — the values pushed by let-bindings occupy different positions than the compiler expects.

**Requirements for a correct fix:**
- Option A: Replace per-function nil-push with a single `OpReserveN{n}` instruction at scope entry that pre-allocates N slots. Requires new VM opcode.
- Option B: Make all let-bindings use `OpSetLocal` explicitly (pop expression value, store at local slot), so the stack doesn't need pre-allocation. Larger compiler change.
- Option C: Add a peephole pass (Pattern A from `02_medium_complexity.md`) that detects `OpConstant{nil}; OpSetLocal{n}` and removes the pair when local `n` is never read between set and next write. But the nil is still needed for the stack layout.

**Current state:** The nil-push was re-added to `compile_statements()` after trying removal. The stack resize guards in the VM (`OpGetLocal`, `OpSetLocal`, `OpClosure`) remain as safety nets.

**Verification:** Snapshot tests pass with nil-push present.

---

## 3. Remove `[[clang::noinline]]` from `execute_op` Overloads

**Status:** Implemented

**Files:**
- `src/vm/src/vm.cpp` — every `execute_op` overload (33+ overloads, lines ~248-739)

**Implementation:**
All 33 `[[clang::noinline]]` annotations were removed from `execute_op` overloads in `src/vm/src/vm.cpp`. The compiler is now free to inline hot instruction handlers (e.g., `OpConstant`, `OpGetLocal`, `OpSetLocal`, `OpAdd`) into the dispatch loop.

**Verification:** All snapshot tests pass. No functional change.

---

## 4. Constant Folding for Primitive Arithmetic

**Status:** Implemented (both peephole and compile-time)

**Files:**
- `src/compiler/src/compiler.cpp` — `compile_binary_expression()` around line 378
- `src/compiler/src/peephole.cpp` — `match_unary_fold` (line 150), `match_binary_fold` (line 204)

**Implementation (dual approach):**

1. **Peephole pass (post-hoc):**
   - `match_unary_fold`: Folds `OpConstant{x} + OpNegate/OpNot` → `OpConstant{folded}`
   - `match_binary_fold`: Folds `OpConstant{a} + OpConstant{b} + arithmetic/comparison` → `OpConstant{folded}`
   - `match_const_jumpif`: Folds `OpConstant{bool} + OpJumpIf` → `OpJump` or removal

2. **Compile-time folding (at emission):**
   - Added `try_extract_literal_value()` and `fold_binary_op()` helpers in `compiler.cpp`
   - `compile_binary_expression()` now checks if both operands are `ast::Literal` values; if so, it folds them at compile-time and emits a single `OpConstant{result}`
   - Uses `ast::visit` instead of `std::get_if` for cleaner Literal extraction
   - Excluded `Power` operator (not foldable at compile time)

**Verification:** Snapshot tests show constant expressions like `1 + 2` already folded to a single `CONSTANT` in the output. Run all snapshot tests.

---

## 5. Eliminate Duplicate Expression Compilation in Chained Comparisons

**Status:** Deferred — needs temp local infrastructure or stack manipulation primitives

**Files:**
- `src/compiler/src/compiler.cpp` — `compile_comparison()` around line 402

**Problem:**
For `a < b < c`, the compiler at `compiler.cpp:402-451` generates `compile(b)` **twice** — once for each comparison pair:
```
compile(a)
compile(b)          ← first evaluation
OpLess
OpJumpIf{false}
compile(b)          ← SECOND evaluation (identical)
compile(c)
OpLess
```
If `b` is a computation with side effects like `f(x)`, it runs twice. Even for simple locals, it produces a duplicate `OpGetLocal`.

**Fix:**
Compile `b` once into a temporary local slot, then reference that slot in both comparisons:

```cpp
// Instead of:
compile_expression(expr.get_middle());
// Do:
compile_expression(expr.get_middle());
auto temp_slot = add_local("<chained_temp>", false, current_scope_depth());
// Now use get_local(temp_slot) for all middle references
```

After the chained comparison completes, pop the temp local (or let scoping handle it).

**Alternative:** If `b` is a simple identifier (no side effects), the duplicate `OpGetLocal` is cheap and this optimization is unnecessary. Only create the temp when `b` has potential side effects (function call, array index, etc.).

**Verification:** Snapshot tests for `comparisons_and_logic` should show the middle operand compiled once instead of twice.

---

## 6. Fix Missing GC Root: Program Constants

**Status:** Implemented (Fixed)

**Files:**
- `src/vm/src/vm.cpp` — `BytecodeVM::run_gc()` at line 233

**Problem:**
`ProgramBytecode::constants` is a `std::vector<runtime::GCValue>` that contains the constant pool (functions, strings, vectors, etc.). These `GCValue` objects were **never added as GC roots** during marking. If a constant contains `Ref`-bearing types (vectors with elements, closures with curried args), its referenced objects could be erroneously swept.

**Fix (implemented at `vm.cpp:233-237`):**
```cpp
if (current_program) {
    for (auto &gc_val : current_program->constants) {
        gc_val.mark();
    }
}
```

In practice, program constants live at least as long as the VM, so this was unlikely to cause a crash on its own. However, it is architecturally correct and prevents obscure bugs if constants are later mutated (e.g., by new builtins).

**Verification:** No observable behavioral change expected. Run all existing tests to confirm no regressions.

**Related bugs:** See `plan/04_bugs.md` Bug #1 (GC sweep corrupts upvalues). The missing program-constants root may compound the `captured_upvalue_refs` tracing bug also documented there. Both needed to be fixed together for correct closure GC behavior — both are now fixed.
