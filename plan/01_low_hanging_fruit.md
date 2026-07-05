# Low-Hanging Fruit: Execution Speed Optimizations

These are isolated changes with low risk and clear benefits.

**Design principle:** Prefer centralized solutions (peephole pass over bytecode) over scattering if-checks across the compiler. A peephole pass catches all instances of a pattern in one place, doesn't increase compiler complexity, and applies even to patterns introduced by future code changes. Only use direct checks in the compiler when:

- The peephole pass cannot detect the pattern (e.g., requires AST context, not just bytecode pattern matching).
- The centralized approach would require significantly more code than a direct check.

---

## 1. Dead Nil-Push Elimination from Function Hoisting

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

## 2. Eliminate Duplicate Expression Compilation in Chained Comparisons

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
