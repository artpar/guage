#!/usr/bin/env bash
#
# Guage vs C Benchmark Suite
# Runs each benchmark 3 times and reports median wall-clock time.
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
GUAGE="$PROJECT_ROOT/bootstrap/guage"
CC="${CC:-gcc}"
CFLAGS="-O2"
RUNS=3

BOLD=$'\033[1m'
RED=$'\033[0;31m'
GREEN=$'\033[0;32m'
YELLOW=$'\033[1;33m'
CYAN=$'\033[0;36m'
NC=$'\033[0m'

TMPDIR=$(mktemp -d)
trap "rm -rf $TMPDIR" EXIT

printf "${BOLD}╔══════════════════════════════════════════════════════╗${NC}\n"
printf "${BOLD}║       Guage vs C Benchmark Suite                    ║${NC}\n"
printf "${BOLD}╚══════════════════════════════════════════════════════╝${NC}\n\n"

printf "Guage:    %s\n" "$GUAGE"
printf "CC:       %s %s\n" "$CC" "$CFLAGS"
printf "Runs:     %d (median)\n\n" "$RUNS"

# Compile C benchmarks
printf "${CYAN}Compiling C benchmarks...${NC}\n"
for src in "$SCRIPT_DIR"/bench_*.c; do
    name=$(basename "$src" .c)
    $CC $CFLAGS -lm -o "$TMPDIR/$name" "$src"
    printf "  %-24s ok\n" "$name"
done
printf "\n"

# Portable sub-second timer (macOS + Linux)
now() {
    perl -MTime::HiRes=time -e 'printf "%.6f\n", time'
}

# Time a command, return seconds
time_cmd() {
    local s e
    s=$(now)
    eval "$@" > /dev/null 2>/dev/null
    e=$(now)
    echo "$e - $s" | bc
}

# Median of 3
median3() {
    echo -e "$1\n$2\n$3" | sort -n | sed -n '2p'
}

# Header
printf "${BOLD}%-20s %10s %10s %8s  %-6s${NC}\n" \
    "Benchmark" "Guage(s)" "C(s)" "Ratio" "Check"
printf "%-20s %10s %10s %8s  %-6s\n" \
    "────────────────" "────────" "────────" "──────" "─────"

BENCHMARKS="fib:Fibonacci(28) tak:Tak(21,14,7) ack:Ackermann(3,7) tco_sum:TCO-Sum(500k) primes:Primes(10k)"

total_g=0
total_c=0

for entry in $BENCHMARKS; do
    key="${entry%%:*}"
    label="${entry##*:}"
    scm="$SCRIPT_DIR/bench_${key}.scm"
    cbin="$TMPDIR/bench_${key}"

    # Get results for correctness check
    g_raw=$("$GUAGE" "$scm" 2>/dev/null | grep '^⟳' | tail -1 | sed 's/^⟳ #//')
    c_raw=$("$cbin")

    # Compare numerically (handles scientific notation like 1.25e+11)
    check=$(perl -e '
        my $gn = $ARGV[0] + 0;
        my $cn = $ARGV[1] + 0;
        if ($cn == 0) { print($gn == 0 ? "PASS" : "FAIL"); exit; }
        my $rel = abs($gn - $cn) / abs($cn);
        print($rel < 1e-3 ? "PASS" : "FAIL");
    ' -- "$g_raw" "$c_raw")

    if [ "$check" = "PASS" ]; then
        check="${GREEN}PASS${NC}"
    else
        check="${RED}${check}${NC}"
    fi

    # Time Guage (3 runs)
    gt=()
    for _ in $(seq 1 $RUNS); do
        t=$(time_cmd "'$GUAGE' '$scm'")
        gt+=("$t")
    done
    gm=$(median3 "${gt[@]}")

    # Time C (3 runs)
    ct=()
    for _ in $(seq 1 $RUNS); do
        t=$(time_cmd "'$cbin'")
        ct+=("$t")
    done
    cm=$(median3 "${ct[@]}")

    # Ratio
    ratio=$(echo "scale=0; $gm / $cm" | bc 2>/dev/null || echo "?")

    total_g=$(echo "$total_g + $gm" | bc)
    total_c=$(echo "$total_c + $cm" | bc)

    printf "%-20s %10.3f %10.4f %7sx  " "$label" "$gm" "$cm" "$ratio"
    printf "${check}\n"
done

printf "%-20s %10s %10s %8s\n" \
    "────────────────" "────────" "────────" "──────"

tr=$(echo "scale=0; $total_g / $total_c" | bc 2>/dev/null || echo "?")
printf "${BOLD}%-20s %10.3f %10.4f %7sx${NC}\n\n" \
    "TOTAL" "$total_g" "$total_c" "$tr"

printf "${YELLOW}What the ratios mean:${NC}\n"
printf "  Tree-walking interpreters typically run 50-500x slower than native C.\n"
printf "  Higher ratios on TCO/primes = per-operation dispatch cost dominates.\n"
printf "  Lower ratios on fib/tak/ack = function-call overhead is well-managed.\n"
printf "  C benchmarks use double arithmetic to match Guage internals.\n"
