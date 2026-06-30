# Low-Hanging Fruit: Execution Speed Optimizations

These are isolated changes with low risk and clear benefits.

**Design principle:** Prefer centralized solutions (peephole pass over bytecode) over scattering if-checks across the compiler. A peephole pass catches all instances of a pattern in one place, doesn't increase compiler complexity, and applies even to patterns introduced by future code changes. Only use direct checks in the compiler when:
- The peephole pass cannot detect the pattern (e.g., requires AST context, not just bytecode pattern matching).
- The centralized approach would require significantly more code than a direct check.

---

## 1. Eliminate `OpPop{0}` No-Ops

**Files:**
- `src/compiler/src/compiler.cpp` — `end_scope()` around line 89-91, `compile_continue_statement()` around line 1016-1025

**Problem:**
Both `end_scope()` and `compile_continue_statement()` emit `OpPop{.count = count}` even when `count == 0`. This produces dead instructions that the VM executes for no effect.

Observed in snapshot `control_flow/bytecode.txt`:
```
0034 | POP           0              ← no-op!
```

**Fix (centralized approach — peephole pass):**
Handle this in the peephole optimizer (see `02_medium_complexity.md` §3 Pattern D). A single rule removes all `OpPop{0}` instructions from the bytecode stream regardless of which compiler function emitted them.

**Alternative (scattered if-check — not recommended):**
```cpp
// end_scope(): if (count > 0) current_chunk().emit(OpPop{.count = count});
// compile_continue_statement(): same pattern
```
This is simpler to implement immediately but adds the check in multiple places and doesn't catch `POP 0` that might be emitted by future code.

**Verdict:** Use the peephole pass (Pattern D). It's a one-line rule `if pop.count == 0 → remove` that handles all sources at once.

## 1. Eliminate `OpPop{0}` No-Ops

**Files:**
- `src/compiler/src/compiler.cpp` — `end_scope()` around line 89-91, `compile_continue_statement()` around line 1016-1025

**Problem:**
Both `end_scope()` and `compile_continue_statement()` emit `OpPop{.count = count}` even when `count == 0`. This produces dead instructions that the VM executes for no effect.

Observed in snapshot `control_flow/bytecode.txt`:
```
0034 | POP           0              ← no-op!
```

**Fix:**
```cpp
// end_scope() — currently:
current_chunk().emit(OpPop{.count = count});

// Fix:
if (count > 0)
    current_chunk().emit(OpPop{.count = count});

// compile_continue_statement() — same pattern
```

**Verification:** Snapshot tests should show the `POP 0` line disappearing. All snapshot expected files need to be updated.

---

## 2. Dead Nil-Push Elimination from Function Hoisting

**Files:**
- `src/compiler/src/compiler.cpp` — `compile_named_function()` around lines 279-289 (hoisting pass), lines 730-745 (second pass)

**Problem:**
The function hoisting pass emits `OpConstant{nil}` then `add_local(func_name)` for every named function. Later, when the function body is compiled, another `OpConstant{fn}` / `OpClosure{fn}` + `OpSetLocal{slot}` overwrites the local. The initial nil push is never read — it's a dead store.

Observed in `functions_closures/bytecode.txt`:
```
0000 | CONSTANT      0 (nil)          ← dead
0001 | CONSTANT      0 (nil)          ← dead
0002 | CONSTANT      1 (function <add>)
0003 | SET_LOCAL     0
0004 | CONSTANT      3 (function <curry_add>)
0005 | SET_LOCAL     1
```

**Fix (centralized approach — peephole pass):**
Handle this in the peephole optimizer (see `02_medium_complexity.md` §3 Pattern A). A single rule detects `OpConstant{nil}` immediately followed by `OpSetLocal{n}`, where the next write to local `n` (via `OpSetLocal`) happens before any read (via `OpGetLocal`). Remove the nil-push+set pair.

This centralized approach:
- Catches **all** dead stores of nil to locals, not just those from function hoisting.
- Doesn't require modifying the hoisting logic (which also serves as the mechanism for reserving function-name slots for mutual recursion).
- Works even if future code changes introduce new sources of dead nil stores.

**Alternative (compiler change — direct fix in hoisting):**
Skip emitting the nil constant during hoisting. Just call `add_local()` to reserve the slot:
```cpp
// Instead of: make_constant({}); add_local(func.get_name(), true, start_depth);
// Just do:    add_local(func.get_name(), true, start_depth);
```
This is simpler but scatters the fix into the compiler's hoisting pass rather than catching the general pattern. Only use if peephole pass is not yet implemented.

**Verification:** Snapshot tests like `functions_closures` should show the dead `CONSTANT nil` lines gone. Update expected snapshots.

---

## 3. Remove `[[clang::noinline]]` from `execute_op` Overloads

**Files:**
- `src/vm/src/vm.cpp` — every `execute_op` overload (29+ overloads, lines ~243-677)
- `src/vm/src/vm.cppm` — declarations

**Problem:**
Every `execute_op` overload is annotated with `[[clang::noinline]]`. This forces a function call for every instruction dispatch, costing ~5-10 cycles for call/ret overhead per instruction. The annotation was presumably added to prevent code bloat from inlining all 29 handlers into the hot loop, but it guarantees suboptimal dispatch performance.

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

**Files:**
- `src/compiler/src/compiler.cpp` — `compile_binary_expression()` around line 340-370

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

**Fix:**
In `compile_binary_expression()`, when both operands are literals with primitive numeric values, compute the result at compile time and emit a single `OpConstant`:

```cpp
void Compiler::compile_binary_expression(const ast::BinaryExpression &expr) {
    // Try constant folding
    if (auto left_lit = get_constant_literal(expr.get_left());
        auto right_lit = get_constant_literal(expr.get_right())) {
        if (auto folded = try_fold(expr.get_operator(), *left_lit, *right_lit)) {
            make_constant(*folded);
            return;
        }
    }
    // Fall through to normal compilation
    compile_expression(expr.get_left());
    compile_expression(expr.get_right());
    emit_binary_op(expr.get_operator());
}
```

Extend to:
- **Unary negation**: Detect `OpConstant{x}` + `OpNegate` → `OpConstant{-x}`
- **Logical expressions**: `true and x` → just `x`, `false and x` → `OpConstant{false}`
- **Unary plus**: `+x` → just `x` (already done in compiler)
- **String concatenation** with two string literals: fold to a single constant

**Helper functions needed:**
- `get_constant_literal()` — checks if an expression is a `Literal` with a primitive value, returns the value if so
- `try_fold()` — given operator and operands, computes result if possible

**Important:** Don't fold expressions with side effects. Literals have no side effects, so this is safe.

**Alternative (peephole pass):** The same optimizations can be done as peephole rules:
- `OpConstant{x}; OpNegate` → `OpConstant{-x}`
- `OpConstant{x}; OpConstant{y}; OpAdd` → `OpConstant{x + y}` (if x, y are numeric)
- `OpConstant{true}; OpJumpIf{false, target}` → `OpJump{target}`

The peephole approach is more centralized (catches patterns regardless of which compiler function emitted them) but requires implementing constant evaluation logic that can run after compilation rather than during AST traversal. The direct compile-time approach is simpler for the initial implementation. If both are present, the peephole pass subsumes the compile-time checks.

**Verdict:** Implement both. The compile-time folding is trivial and avoids emitting instructions at all. The peephole folding catches cases where constant-argument ops are created by other optimization passes or future compiler changes.

**Verification:** A test like `lit_folding.l3` with `print(1 + 2 * 3)` should compile to a single `CONSTANT 7` instead of multiple ops. Update snapshot expected files.

---

## 5. Eliminate Duplicate Expression Compilation in Chained Comparisons

**Files:**
- `src/compiler/src/compiler.cpp` — `compile_chained_comparison()` around line 403-410

**Problem:**
For `a < b < c`, the compiler generates `compile(b)` **twice** — once for each comparison pair:
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

**Files:**
- `src/vm/src/vm.cpp` — `BytecodeVM::run_gc()` around line 194-234
- `src/bytecode/src/bytecode.cppm` — `ProgramBytecode::constants` around line 145

**Problem:**
`ProgramBytecode::constants` is a `std::vector<runtime::GCValue>` that contains the constant pool (functions, strings, vectors, etc.). These `GCValue` objects are **never added as GC roots** during marking. If a constant contains `Ref`-bearing types (vectors with elements, closures with curried args), its referenced objects could be erroneously swept.

**Fix:**
In `run_gc()`, add the program constants to the root set:

```cpp
std::size_t BytecodeVM::run_gc() {
    static const auto mark_value = [](runtime::Value &value) { ... };
    
    // Existing roots:
    for (auto &value : stack)
        mark_value(value);
    for (auto &[_, ref] : global_symbols)
        ref.get_gc_mut().mark();
    for (auto &frame : frames) {
        // ... existing frame marking ...
    }
    
    // NEW: Mark program constants as roots
    if (current_program) {
        for (auto &gc_value : current_program->constants) {
            mark_value(gc_value.get_value_mut());
        }
    }
    
    return gc_storage.sweep();
}
```

**Note:** In practice, program constants live at least as long as the VM, so this is unlikely to fix a crash scenario. However, it is architecturally correct and prevents obscure bugs if constants are later mutated (e.g., by new builtins).

**Verification:** No observable behavioral change expected. Run all existing tests to confirm no regressions.

**Related bugs:** See `plan/04_bugs.md` Bug #1 (GC sweep corrupts upvalues). The missing program-constants root may compound the `captured_upvalue_refs` tracing bug also documented there. Both need to be fixed together for correct closure GC behavior.
