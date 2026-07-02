#!/bin/bash
set -euo pipefail

# ---------------------------------------------------------------------------
# bench_compare.sh — run benchmarks across two git commits, format comparison
#
# Usage:
#   ./bench_compare.sh [commit_baseline] [commit_head]
#
#   commit_baseline  — commit before the optimization (default: HEAD~1)
#   commit_head      — commit with the optimization  (default: HEAD)
#
# Requires: hyperfine (https://github.com/sharkdp/hyperfine)
# ---------------------------------------------------------------------------

REPO_ROOT="$(git rev-parse --show-toplevel)"
PERF_DIR="${REPO_ROOT}/examples/perf"
BUILD_DIR="${REPO_ROOT}/build"

COMMIT_A="${1:-HEAD~1}"
COMMIT_B="${2:-HEAD}"

# Resolve SHAs
SHA_A="$(git rev-parse --short "${COMMIT_A}")"
SHA_B="$(git rev-parse --short "${COMMIT_B}")"

ORIG_BRANCH="$(git rev-parse --abbrev-ref HEAD)"
RESULT_FILE="${PERF_DIR}/bench_results_${SHA_A}_vs_${SHA_B}.md"

# ---------------------------------------------------------------------------
# Sanity checks
# ---------------------------------------------------------------------------
if ! command -v hyperfine &>/dev/null; then
    echo "Error: hyperfine not found. Install from https://github.com/sharkdp/hyperfine"
    exit 1
fi

if ! git diff --quiet; then
    echo "Error: working tree is dirty. Commit or stash changes first."
    exit 1
fi

# ---------------------------------------------------------------------------
# Gather all .l3 benchmark files
# ---------------------------------------------------------------------------
BENCH_FILES=()
for f in "${PERF_DIR}"/*.l3; do
    BENCH_FILES+=("$(basename "${f}")")
done

if [ ${#BENCH_FILES[@]} -eq 0 ]; then
    echo "Error: no .l3 benchmark files found in ${PERF_DIR}"
    exit 1
fi

echo "Found ${#BENCH_FILES[@]} benchmarks: ${BENCH_FILES[*]}"
echo ""
echo "Baseline commit: ${SHA_A} (${COMMIT_A})"
echo "Head commit:     ${SHA_B} (${COMMIT_B})"
echo ""

# ---------------------------------------------------------------------------
# Build and benchmark a single commit
# ---------------------------------------------------------------------------
bench_commit() {
    local sha="$1"
    local ref="$2"
    local label="$3"
    local outdir="${PERF_DIR}/.bench_${sha}"

    echo "--- Building ${label} (${sha}) ---"
    git checkout --quiet "${ref}"
    cmake --build "${BUILD_DIR}" --config Release --target lang3 2>&1 | tail -1

    mkdir -p "${outdir}"
    local lang3_bin="${BUILD_DIR}/bin/Release/lang3"

    echo "--- Benchmarking ${label} (${sha}) ---"
    hyperfine_args=(
        --warmup 2
        --min-runs 5
        --export-json "${outdir}/results.json"
    )
    for f in "${BENCH_FILES[@]}"; do
        hyperfine_args+=("${lang3_bin} ${PERF_DIR}/${f}")
    done
    hyperfine "${hyperfine_args[@]}" 2>&1

    echo ""
}

# ---------------------------------------------------------------------------
# Run
# ---------------------------------------------------------------------------
bench_commit "${SHA_A}" "${COMMIT_A}" "baseline (${SHA_A})"
bench_commit "${SHA_B}" "${COMMIT_B}" "head (${SHA_B})"

# ---------------------------------------------------------------------------
# Format comparison table
# ---------------------------------------------------------------------------
RESULTS_A="${PERF_DIR}/.bench_${SHA_A}/results.json"
RESULTS_B="${PERF_DIR}/.bench_${SHA_B}/results.json"

echo "--- Comparison ---"
echo ""

# Build the table
{
    echo "# Benchmark Comparison"
    echo ""
    echo "**Baseline:** \`${SHA_A}\` — ${COMMIT_A}"
    echo "**Head:**     \`${SHA_B}\` — ${COMMIT_B}"
    echo ""
    echo "| Benchmark | Baseline (${SHA_A}) | Head (${SHA_B}) | Change |"
    echo "|-----------|---------------------|------------------|--------|"

    # Read both JSON results and format rows
    python3 -c "
import json, sys

with open('${RESULTS_A}') as f:
    a = json.load(f)
with open('${RESULTS_B}') as f:
    b = json.load(f)

results_a = {r['command']: r for r in a['results']}
results_b = {r['command']: r for r in b['results']}

for cmd_a, r_a in results_a.items():
    if cmd_a not in results_b:
        continue
    r_b = results_b[cmd_a]
    name = cmd_a.split('/')[-1]

    mean_a = r_a['mean']
    mean_b = r_b['mean']
    std_a  = r_a['stddev']
    std_b  = r_b['stddev']

    change = ((mean_b - mean_a) / mean_a) * 100
    sign = '+' if change > 0 else ''

    print(f'| {name} | {mean_a:.3f}s ± {std_a:.3f}s | {mean_b:.3f}s ± {std_b:.3f}s | {sign}{change:.1f}% |')
" 2>&1
} > "${RESULT_FILE}"

cat "${RESULT_FILE}"

# ---------------------------------------------------------------------------
# Restore original state
# ---------------------------------------------------------------------------
git checkout --quiet "${ORIG_BRANCH}"
echo ""
echo "Results saved to: ${RESULT_FILE}"
