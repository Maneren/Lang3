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

**Status:** Fixed
**Discovered:** 2026-06-30, during closure benchmark testing (`examples/perf/closure_stress.l3`)
**Fixed:** 2026-06-30 (resolved by Bug #2 fix)
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

The root cause was **Bug #2** (missing `captured_upvalue_refs` tracing in GC mark phase). See Bug #2 below for full details.

**Contributing factor (also fixed):** Missing GC root for program constants (`01_low_hanging_fruit.md` §6) — `ProgramBytecode::constants` was not marked during GC. This was also fixed alongside the `captured_upvalue_refs` tracing.

### Fix

Fixed by Bug #2 (adding `captured_upvalue_refs` tracing in `GCValue::mark()` at `gc_value.cpp:37-39`) and by adding program constants as GC roots (`vm.cpp:233-237`).

### Verification

- `N = 10000` reproducer now outputs correct sum `49995000`
- All 17+ snapshot tests pass
- Closure-heavy code no longer crashes under GC pressure

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

**Status:** Fixed
**Discovered:** 2026-06-30
**Fixed:** 2026-06-30
**Severity:** High

### Root cause

`GCValue::mark()` traces `BytecodeFunction::curried_args` but not
`BytecodeFunction::captured_upvalue_refs`. Both are `std::vector<Ref>`
containing pointers to `GCValue` objects that must survive the GC sweep.

The `mark_value` lambda in `BytecodeVM::run_gc()` (vm.cpp) already had the
correct tracing for both vectors, but `GCValue::mark()` (gc_value.cpp) was
missing the `captured_upvalue_refs` trace. This caused a GC root coverage
asymmetry: closures rooted through stack/global references were traced
correctly, but closures rooted through other `GCValue` objects (e.g., stored
in arrays) would have their captured upvalues swept.

**Files:**
- `src/runtime/src/gc_value.cpp:32-38` — `GCValue::mark()` (missing trace)
- `src/vm/src/vm.cpp:207-209` — `mark_value` lambda (already correct)

### Fix

Added the missing loop in `GCValue::mark()`:

```cpp
[](Value::function_type &func) {
    if (auto bc_opt = func.as_mut_bytecode_function()) {
        for (auto &ca : bc_opt->get().curried_args)
            ca.get_gc_mut().mark();
        // NEW:
        for (auto &uv : bc_opt->get().captured_upvalue_refs)
            uv.get_gc_mut().mark();
    }
},
```

### Verification

- `N = 10000` closure reproducer now outputs correct sum `49995000`
- All 17 snapshot tests pass
- Example with closures stored in arrays no longer crashes under GC pressure
