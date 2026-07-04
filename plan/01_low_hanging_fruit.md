# Low-Hanging Fruit: Execution Speed Optimizations

These are isolated changes with low risk and clear benefits.

**Design principle:** Prefer centralized solutions (peephole pass over bytecode) over scattering if-checks across the compiler. A peephole pass catches all instances of a pattern in one place, doesn't increase compiler complexity, and applies even to patterns introduced by future code changes. Only use direct checks in the compiler when:
- The peephole pass cannot detect the pattern (e.g., requires AST context, not just bytecode pattern matching).
- The centralized approach would require significantly more code than a direct check.

---

## 1. Eliminate `OpPop{0}` No-Ops

**Status:** Partially Implemented

**Files:**
- `src/compiler/src/compiler.cpp` — `end_scope()` at line 89, `compile_continue_statement()` at line 1024

**Problem:**
Both `end_scope()` and `compile_continue_statement()` emit `OpPop{.count = count}` even when `count == 0`. This produces dead instructions that the VM executes for no effect.

**Current implementation (mixed approach):**
- `end_scope()` at `compiler.cpp:96`: **GUARDED** with `if (count > 0)` — the scattered if-check.
- `compile_continue_statement()` at `compiler.cpp:1031`: **NOT GUARDED** — still emits `OpPop` unconditionally.
- Peephole pass at `src/compiler/src/peephole.cpp:119` (`match_pop_zero`): **CATCHES ANY REMAINING `POP 0`**, including from `compile_continue_statement()`.

This combination works but is inconsistent. The `end_scope()` guard was added directly (the approach the plan advised against), while `compile_continue_statement()` relies on the peephole pass. Consider removing the `end_scope()` guard to fully rely on the peephole pass (true to the design principle), or adding the guard to `compile_continue_statement()` too for consistency.

**Verification:** Snapshot tests no longer show `POP 0` lines.

---

## 2. Dead Nil-Push Elimination from Function Hoisting

**Status:** Not Implemented

**Files:**
- `src/compiler/src/compiler.cpp` — `compile_statements()` at lines 286-302 (hoisting pass)

**Problem:**
The function hoisting pass at `compiler.cpp:292` still emits `OpConstant{make_constant({})}` (nil) then `add_local()` for every named function. Later, when the function body is compiled, another `OpConstant{fn}` / `OpClosure{fn}` + `OpSetLocal{slot}` overwrites the local. The initial nil push is never read — it's a dead store.

Observed in `functions_closures/bytecode.txt`:
```
0000 | CONSTANT      0 (nil)          ← dead
0001 | CONSTANT      0 (nil)          ← dead
0002 | CONSTANT      1 (function <add>)
0003 | SET_LOCAL     0
0004 | CONSTANT      3 (function <curry_add>)
0005 | SET_LOCAL     1
```

**Fix (centralized approach — peephole pass, recommended):**
Handle this in the peephole optimizer (see `02_medium_complexity.md` §3 Pattern A). Add a rule that detects `OpConstant{nil}` immediately followed by `OpSetLocal{n}`, where the next write to local `n` happens before any read of local `n`. Remove the nil-push+set pair.

**Alternative (compiler change — direct fix in hoisting):**
Skip emitting the nil constant during hoisting. Just call `add_local()` to reserve the slot:
```cpp
// Instead of: make_constant({}); add_local(func.get_name(), ...);
// Just do:    add_local(func.get_name(), ...);
```

**Note on current state:** Neither approach has been implemented. The peephole pass exists (see `02_medium_complexity.md` §3) but Pattern A was never added. Consider implementing the simpler compiler-side fix first, as it's a one-line change, and leave the peephole Pattern A for a future pass.

**Verification:** Snapshot tests like `functions_closures` should show the dead `CONSTANT nil` lines gone. Update expected snapshots.

---

## 3. Remove `[[clang::noinline]]` from `execute_op` Overloads

**Status:** Not Implemented

**Files:**
- `src/vm/src/vm.cpp` — every `execute_op` overload (33+ overloads, lines ~248-739)
- `src/vm/src/vm.cppm` — declarations

**Problem:**
Every `execute_op` overload is annotated with `[[clang::noinline]]`. This forces a function call for every instruction dispatch, costing ~5-10 cycles for call/ret overhead per instruction. The annotation was presumably added to prevent code bloat from inlining all 29 handlers into the hot loop, but it guarantees suboptimal dispatch performance.

**Current state:** All 33 `[[clang::noinline]]` annotations are still present in `src/vm/src/vm.cpp`.

**Fix:**
1. Remove `[[clang::noinline]]` from all `execute_op` overloads.
2. Measure code size. If too large, selectively inline only the hottest ops:
   - Keep-inline candidates: `OpConstant`, `OpGetLocal`, `OpSetLocal`, `OpAdd`, `OpSub`, `OpPop`, `OpReturn`, `OpJump` — these are short and executed most frequently.
   - Add `[[clang::noinline]]` only on: `OpCall`, `OpClosure`, `OpForLoop`, `OpMakeArray` — complex ops that handle many branches.

**Alternative:** Use a single function with a switch statement (preparation for future compact-bytecode dispatch). This gives the compiler a single function to optimize with jump table heuristics.

```cpp
void BytecodeVM::execute_instruction(const Instruction &inst, CallFrame &frame) {
    // Instead of std::visit + noinline overloads
    // The compiler will generate a jump table
}
```

**Risk:** Code size may increase. Monitor with `-Os` if needed, but prioritize `-O2`/`-O3`.

**Verification:** Run the benchmark suite (if any) or snapshot tests to ensure no functional change. Measure throughput improvement with `perf stat`.

---

## 4. Constant Folding for Primitive Arithmetic

**Status:** Partially Implemented (peephole pass only, compile-time not done)

**Files:**
- `src/compiler/src/compiler.cpp` — `compile_binary_expression()` around line 343
- `src/compiler/src/peephole.cpp` — `match_unary_fold` (line 150), `match_binary_fold` (line 204)

**Problem:**
Expressions like `1 + 2 * -3` compile to runtime instructions even though all operands are known at compile time:
```
CONSTANT 0 (1)
CONSTANT 1 (2)
CONSTANT 2 (3)
NEGATE
MULTIPLY
ADD
```

**Current implementation (peephole pass — done):**
- `match_unary_fold` (`peephole.cpp:150`): Folds `OpConstant{x} + OpNegate/OpNot` → `OpConstant{folded}`
- `match_binary_fold` (`peephole.cpp:204`): Folds `OpConstant{a} + OpConstant{b} + arithmetic/comparison` → `OpConstant{folded}`
- `match_const_jumpif` (`peephole.cpp:179`): Folds `OpConstant{bool} + OpJumpIf` → `OpJump` or removal

These post-hoc peephole rules catch constant folding opportunities regardless of which compiler function produced the pattern.

**Not implemented (compile-time folding):**
The compiler's `compile_binary_expression()` at `compiler.cpp:343` does no constant folding during AST traversal — it simply compiles both sides and emits the operator. Adding compile-time folding would avoid emitting instructions entirely rather than cleaning them up afterward.

**Remaining work:**
- Add `get_constant_literal()` / `try_fold()` helpers in the compiler
- Add compile-time folding in `compile_binary_expression()`
- Extend to string literal concatenation

**Verification:** A test like `lit_folding.l3` with `print(1 + 2 * 3)` should compile to a single `CONSTANT 7` instead of multiple ops. Update snapshot expected files.

---

## 5. Eliminate Duplicate Expression Compilation in Chained Comparisons

**Status:** Not Implemented

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
