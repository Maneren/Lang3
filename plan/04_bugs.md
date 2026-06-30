# Bug Tracking

This file catalogs bugs discovered during optimization implementation.
Each bug found while working on planned features should be documented here
with reproducer, root cause, and fix.

**If you discover a bug while implementing any planned optimization:**
1. Add it to this file with the full reproducer.
2. Note the fix or suspected root cause.
3. Reference the bug # from the plan file section you're implementing.
4. Fix the bug before or alongside the optimization that uncovered it.

---

## Bug #1: GC sweep corrupts closure upvalues at ~10K allocation boundary

**Status:** Open
**Discovered:** 2026-06-30, during closure benchmark testing (`examples/perf/closure_stress.l3`)
**Severity:** High (data corruption, incorrect behavior)

### Reproducer

```lang3
fn make_adder(x)
  return fn(y) return x + y end
end

let N = 10000
let mut adders = []
for i in 0..N do
  adders += [make_adder(i)]
end

let mut total = 0
for iter in 0..1 do
  for i in 0..N do
    let f = adders[i]
    total = total + f(iter)
  end
end
println(total)
```

Fails with:
```
UnsupportedOperation: addition between 'function' and 'int' not supported
  at <stdin>:2.23-28
  in <anonymous> called at <stdin>:15.21-28
```

Working with `N = 5000`, failing with `N = 10000`.

### Observed behavior

- `N = 5000`: works correctly, output `12497500`
- `N = 10000`: crashes with "addition between 'function' and 'int'"
- The error indicates the closure's captured upvalue `x` is being interpreted as a `Function` type instead of an `int`
- The GC's allocation threshold is `GC_INTERVAL = 16 * 1024 = 16384` (`src/vm/src/vm.cpp:237`)
- Each closure creation allocates: 1 `GCValue` for the function + 1 `GCValue` for the captured upvalue storage = 2 allocations
- `N = 5000` → 10,000 allocations (below threshold, GC never triggers)
- `N = 10000` → 20,000 allocations (GC triggers once during the loop, sweeping something it shouldn't)

### Root cause analysis

Multiple contributing factors:

**a) Probable: Missing GC root for program constants (`01_low_hanging_fruit.md` §6)**
`ProgramBytecode::constants` is a `std::vector<runtime::GCValue>` that is never
marked during GC. The constant pool contains the compiled function objects
(each closure references a `BytecodeFunction` via its chunk index). If the GC
sweeps the `GCValue` containing the closure's bytecode function, the closure
reference becomes a dangling pointer.

**b) Possible: `shared_ptr<HeapValue>` / `Value` copy semantics**
When `make_adder(i)` creates a closure, the closure's captured value (`x = i`)
is stored in a `GCValue` allocated in GC storage. A `shared_ptr<HeapValue>`
copy may exist on the C++ stack or in a `Value` copy that isn't traced by
the GC root set. However, the more likely scenario is that the GCValue
containing the captured int is not reachable from any root and gets swept.

**c) Possible: Missing root for closure upvalue `Ref` chain**
The mark phase traces:
- Stack values (the `adders` array is on the stack)
- Global symbols
- Frame locals and upvalues

The `adders` array contains `Ref` values pointing to `GCValue` objects
(the closures). Each closure's `GCValue` is marked. But does the mark
phase trace **into** the closure's captured upvalues?

In `GCValue::mark()` (`src/runtime/src/gc_value.cpp:19-41`):
```cpp
void GCValue::mark() {
    if (marked) return;
    marked = true;
    get_value_mut().visit(
        [](Value::vector_type &vector) {
            for (auto &item : vector)
                item.get_gc_mut().mark();    // traces vector elements
        },
        [](Value::function_type &func) {
            if (auto bc_opt = func.as_mut_bytecode_function()) {
                for (auto &ca : bc_opt->get().curried_args)
                    ca.get_gc_mut().mark();    // traces curried args
                // NOTE: Does NOT trace captured_upvalue_refs!
            }
        },
        [](auto &) {}  // strings/primitives — leaf
    );
}
```

**Key finding:** `GCValue::mark()` traces `curried_args` but does **not** trace
`BytecodeFunction::captured_upvalue_refs`. When a closure captures a local
variable, the captured value is stored in `captured_upvalue_refs` (a
`std::vector<Ref>`). This vector is **never marked**, so the captured values
are always eligible for sweeping.

**Cross-reference:** The `mark_value` lambda in `BytecodeVM::run_gc()`
(`src/vm/src/vm.cpp:195-213`) also only traces `curried_args` for functions,
not `captured_upvalue_refs`.

### Proposed fix

In both `GCValue::mark()` and the `mark_value` lambda in `run_gc()`, add
tracing of `captured_upvalue_refs`:

```cpp
// In GCValue::mark() — gc_value.cpp:
[](Value::function_type &func) {
    if (auto bc_opt = func.as_mut_bytecode_function()) {
        for (auto &ca : bc_opt->get().curried_args)
            ca.get_gc_mut().mark();
        // NEW: trace captured upvalue refs
        for (auto &uv_ref : bc_opt->get().captured_upvalue_refs)
            uv_ref.get_gc_mut().mark();
    }
},
```

Similarly in `BytecodeVM::run_gc()` (vm.cpp), the `mark_value` lambda needs
the same addition.

### Verification

- `N = 10000` variant of the reproducer should output the correct sum
- All existing snapshot tests must pass
- A new test case with closure-heavy code should be added to prevent regression

---

## Bug template for future discoveries

When you discover a bug during implementation, add it here with:

```markdown
## Bug #N: Short description

**Status:** Open/Fixed/In Progress
**Discovered:** YYYY-MM-DD
**Severity:** Low/Medium/High/Critical

### Reproducer

```lang3
# Minimal Lang3 code that triggers the bug
```

### Root cause

Analysis of why the bug occurs, with file paths and line numbers.

### Proposed fix

Description of the fix with code snippets.

### Verification

How to confirm the fix works.
```

---

## Bug #2: Missing `captured_upvalue_refs` tracing in GC mark phase

*(This is the root cause of Bug #1, tracked separately for clarity)*

**Status:** Open
**Discovered:** 2026-06-30
**Severity:** High

### Root cause

`GCValue::mark()` (and the `mark_value` lambda in `run_gc()`) trace
`BytecodeFunction::curried_args` but not `BytecodeFunction::captured_upvalue_refs`.
Both are `std::vector<Ref>` containing pointers to `GCValue` objects that must
survive the GC sweep.

**Files:**
- `src/runtime/src/gc_value.cpp:19-41` — `GCValue::mark()`
- `src/vm/src/vm.cpp:195-213` — `mark_value` lambda in `run_gc()`

### Fix

Add `captured_upvalue_refs` tracing alongside `curried_args` in both locations.
This ensures captured local values (upvalues) are reachable from the GC root set
and survive the sweep phase. See Bug #1 for the exact code change.
