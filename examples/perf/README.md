# Performance Benchmarks for Lang3 VM Optimizations

Each file stresses a specific VM or compiler path targeted for optimization.
Run with `just run examples/perf/<file>.l3` or `lang3 examples/perf/<file>.l3`.
Time execution with `time lang3 ...` or `hyperfine lang3 ...`.

## Benchmarks

| File | What it stresses | Optimizations tested |
|------|------------------|---------------------|
| `const_fold.l3` | Arithmetic with all-constant operands in a loop | Constant folding (peephole), instruction dispatch |
| `chained_cmp.l3` | `0 <= i < N` with local variable as middle operand | Duplicate expression elimination in chained comparisons |
| `call_tight.l3` | Tiny function called 1M times | Call optimization (direct stack arg passing), tail calls |
| `string_concat.l3` | String concatenation and equality in O(n²) loop | String interning, Value flatten |
| `alloc_stress.l3` | Allocate/discard many vectors in waves | GC sweep speed, shared_ptr removal, adaptive GC |
| `vec_stress.l3` | 2D matrix transposition and sum | Value::visit dispatch, vector indexing, Ref traversal |
| `closure_stress.l3` | 10K closures with upvalue access | OpClosure dispatch, upvalue capture, peephole/hoisting |
| `dispatch_stress.l3` | Mixed opcodes: arithmetic, comparison, calls, jumps | Computed-goto dispatch, NaN-boxing |

## Usage

```bash
# Time a single benchmark
time ../build/Debug/apps/lang3/lang3 const_fold.l3

# Run all benchmarks with timing
for f in *.l3; do
  echo "=== $f ==="
  time ../build/Debug/apps/lang3/lang3 "$f"
  echo
done
```

## Expected speedup by optimization

| Optimization | Expected speedup | Best benchmark |
|-------------|-----------------|----------------|
| Computed-goto dispatch | 2-5x on mixed opcodes | `dispatch_stress.l3` |
| Call optimization | 1.5-3x on tight calls | `call_tight.l3` |
| NaN-boxing / Value flatten | 1.5-2x on vector-heavy code | `vec_stress.l3` |
| Peephole / constant folding | 1.1-1.5x on arithmetic loops | `const_fold.l3` |
| String interning | 3-10x on string equality | `string_concat.l3` |
| shared_ptr removal | 1.2-2x on allocation-heavy code | `alloc_stress.l3` |
| GC improvements | 1.5-3x on allocation waves | `alloc_stress.l3` |
| Chained comparison fix | 1.2-1.5x on comparison loops | `chained_cmp.l3` |
| Closure/upvalue fixes | 1.1-1.3x on closure-heavy code | `closure_stress.l3` |
