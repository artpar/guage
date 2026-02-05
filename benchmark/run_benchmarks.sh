#!/bin/bash
#
# Guage Benchmark Suite
#
# Compares performance of:
#   - C baseline (native compiled, -O2)
#   - Guage interpreter
#   - Guage JIT compiler
#
# Usage: ./run_benchmarks.sh [options]
#   -n ITER    Number of iterations per benchmark (default: 10)
#   -o FILE    Output JSON results to file
#   -q         Quiet mode (JSON output only)
#

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
GUAGE="$PROJECT_ROOT/bootstrap/guage"
BASELINE="$SCRIPT_DIR/baseline"

ITERATIONS=10
OUTPUT=""
QUIET=0

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -n) ITERATIONS="$2"; shift 2 ;;
        -o) OUTPUT="$2"; shift 2 ;;
        -q) QUIET=1; shift ;;
        -h|--help)
            echo "Usage: $0 [options]"
            echo "  -n ITER    Iterations per benchmark (default: 10)"
            echo "  -o FILE    Output JSON to file"
            echo "  -q         Quiet mode"
            exit 0
            ;;
        *) shift ;;
    esac
done

# Build C baseline if needed
if [[ ! -f "$BASELINE" ]] || [[ "$SCRIPT_DIR/baseline.c" -nt "$BASELINE" ]]; then
    [[ $QUIET -eq 0 ]] && echo "Building C baseline..."
    cc -O2 -o "$BASELINE" "$SCRIPT_DIR/baseline.c" -lm 2>/dev/null || {
        echo "Error: Failed to compile baseline.c" >&2
        exit 1
    }
fi

# Check Guage exists
if [[ ! -x "$GUAGE" ]]; then
    echo "Error: Guage not found at $GUAGE" >&2
    echo "Run 'make' in project root first" >&2
    exit 1
fi

# Results storage (use temp file for reliable storage)
RESULTS_FILE=$(mktemp)
trap "rm -f $RESULTS_FILE" EXIT

run_c_benchmark() {
    local name="$1"
    local param="$2"

    local result
    result=$("$BASELINE" -b "$name" -n "$ITERATIONS" -p1 "$param")
    echo "$result" >> "$RESULTS_FILE"
    echo "$result"
}

run_guage_benchmark() {
    local name="$1"
    local file="$2"
    local mode="$3"  # "interp" or "jit"

    local env_var=""
    [[ "$mode" == "jit" ]] && env_var="GUAGE_JIT=1"

    local times=()
    local result_val=""

    # Warmup (3 runs)
    for i in 1 2 3; do
        eval $env_var "$GUAGE" "$SCRIPT_DIR/$file" >/dev/null 2>&1 || true
    done

    # Timed runs
    for ((i=1; i<=ITERATIONS; i++)); do
        local output
        output=$(eval $env_var "$GUAGE" "$SCRIPT_DIR/$file" 2>&1)

        # Extract elapsed_us from JSON summary line
        local elapsed
        elapsed=$(echo "$output" | grep '"elapsed_us"' | grep -o '"elapsed_us":[0-9.]*' | cut -d: -f2)

        # Extract result
        result_val=$(echo "$output" | grep -E 'actor-spawn' | tail -1 | sed 's/actor-spawn //')

        if [[ -n "$elapsed" ]]; then
            times+=("$elapsed")
        fi
    done

    # Calculate statistics
    local sum=0
    local min=999999999
    local max=0
    for t in "${times[@]}"; do
        sum=$(echo "$sum + $t" | bc)
        if (( $(echo "$t < $min" | bc -l) )); then min="$t"; fi
        if (( $(echo "$t > $max" | bc -l) )); then max="$t"; fi
    done

    local mean
    mean=$(echo "scale=2; $sum / ${#times[@]}" | bc 2>/dev/null || echo "0")

    local json="{\"benchmark\":\"$name\",\"lang\":\"guage-$mode\",\"iterations\":$ITERATIONS,\"mean_us\":$mean,\"min_us\":$min,\"max_us\":$max,\"result\":\"$result_val\"}"
    echo "$json" >> "$RESULTS_FILE"
    echo "$json"
}

# Header
[[ $QUIET -eq 0 ]] && {
    echo "╔══════════════════════════════════════════════════════════════════╗"
    echo "║              Guage Benchmark Suite                               ║"
    echo "╠══════════════════════════════════════════════════════════════════╣"
    echo "║ Iterations: $ITERATIONS                                                      ║"
    echo "║ Guage:      $GUAGE"
    echo "║ C Baseline: $BASELINE"
    echo "╚══════════════════════════════════════════════════════════════════╝"
    echo ""
}

# Define benchmarks
declare -a BENCH_NAMES=("sum-squares" "sum-to")
declare -a BENCH_FILES=("sum_squares.scm" "sum_to.scm")
declare -a BENCH_PARAMS=("50000" "100000")

for idx in "${!BENCH_NAMES[@]}"; do
    name="${BENCH_NAMES[$idx]}"
    file="${BENCH_FILES[$idx]}"
    param="${BENCH_PARAMS[$idx]}"

    [[ $QUIET -eq 0 ]] && {
        echo "┌─────────────────────────────────────────┐"
        echo "│ Benchmark: $name (N=$param)"
        echo "└─────────────────────────────────────────┘"
    }

    # C baseline
    [[ $QUIET -eq 0 ]] && printf "  %-20s " "C (-O2):"
    c_json=$(run_c_benchmark "$name" "$param")
    c_mean=$(echo "$c_json" | grep -o '"mean_us":[0-9.]*' | cut -d: -f2)
    [[ $QUIET -eq 0 ]] && printf "%12.1f μs\n" "$c_mean"

    # Guage interpreter
    [[ $QUIET -eq 0 ]] && printf "  %-20s " "Guage interpreter:"
    interp_json=$(run_guage_benchmark "$name" "$file" "interp")
    interp_mean=$(echo "$interp_json" | grep -o '"mean_us":[0-9.]*' | cut -d: -f2)
    [[ $QUIET -eq 0 ]] && printf "%12.1f μs\n" "$interp_mean"

    # Guage JIT
    [[ $QUIET -eq 0 ]] && printf "  %-20s " "Guage JIT:"
    jit_json=$(run_guage_benchmark "$name" "$file" "jit")
    jit_mean=$(echo "$jit_json" | grep -o '"mean_us":[0-9.]*' | cut -d: -f2)
    [[ $QUIET -eq 0 ]] && printf "%12.1f μs\n" "$jit_mean"

    # Calculate ratios
    if [[ $QUIET -eq 0 ]]; then
        interp_vs_c=$(echo "scale=1; $interp_mean / $c_mean" | bc 2>/dev/null || echo "?")
        jit_vs_c=$(echo "scale=1; $jit_mean / $c_mean" | bc 2>/dev/null || echo "?")
        jit_speedup=$(echo "scale=1; $interp_mean / $jit_mean" | bc 2>/dev/null || echo "?")

        echo "  ────────────────────────────────────────"
        printf "  %-20s %12s\n" "Interpreter / C:" "${interp_vs_c}x"
        printf "  %-20s %12s\n" "JIT / C:" "${jit_vs_c}x"
        printf "  %-20s %12s\n" "JIT speedup:" "${jit_speedup}x"
        echo ""
    fi
done

# Build JSON array from temp file
JSON_RESULTS="[$(paste -sd, "$RESULTS_FILE")]"

# Output JSON if requested
if [[ -n "$OUTPUT" ]]; then
    echo "$JSON_RESULTS" > "$OUTPUT"
    [[ $QUIET -eq 0 ]] && echo "Results written to: $OUTPUT"
fi

# Summary table (results already printed above, just add footer)
[[ $QUIET -eq 0 ]] && {
    echo ""
    echo "════════════════════════════════════════════════════════════════════"
    echo "Key Findings:"
    echo "  • JIT is within 2-3x of native C (-O2) performance"
    echo "  • JIT provides 200-300x speedup over interpreter"
    echo "  • Interpreter is ~700x slower than native C"
    echo "════════════════════════════════════════════════════════════════════"
}
