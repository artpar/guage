#!/opt/homebrew/bin/bash
#
# Guage vs C Benchmark Suite
# Runs each benchmark 3 times and reports median wall-clock time.
# With GUAGE_PROFILE counters: per-benchmark breakdown of eval/alloc/lookup costs.
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
GUAGE="$PROJECT_ROOT/bootstrap/guage"
CC="${CC:-gcc}"
CFLAGS="-O2"
RUNS=3
DIFF_FILE=""
SNAPSHOT_DIR="$SCRIPT_DIR/snapshots"

# Parse flags
while [[ $# -gt 0 ]]; do
    case "$1" in
        --diff) DIFF_FILE="$2"; shift 2 ;;
        *)      shift ;;
    esac
done

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
printf "Runs:     %d (median)\n" "$RUNS"
printf "Profile:  GUAGE_PROFILE=1\n\n"

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

# Human-readable large numbers (52000000 → 52M)
humanize() {
    perl -e '
        my $n = $ARGV[0] + 0;
        if    ($n >= 1e9) { printf "%.0fG", $n/1e9; }
        elsif ($n >= 1e6) { printf "%.0fM", $n/1e6; }
        elsif ($n >= 1e3) { printf "%.0fK", $n/1e3; }
        else              { printf "%d",    $n; }
    ' -- "$1"
}

# Load diff snapshot if provided
declare -A DIFF_GUAGE DIFF_RATIO DIFF_NSEVAL
if [[ -n "$DIFF_FILE" && -f "$DIFF_FILE" ]]; then
    printf "${CYAN}Loading baseline: %s${NC}\n\n" "$DIFF_FILE"
    while IFS= read -r line; do
        key=$(echo "$line" | perl -ne '/"key":"([^"]+)"/ && print $1')
        [[ -z "$key" ]] && continue
        DIFF_GUAGE[$key]=$(echo "$line" | perl -ne '/"guage_s":([\d.]+)/ && print $1')
        DIFF_RATIO[$key]=$(echo "$line" | perl -ne '/"ratio":(\d+)/ && print $1')
        DIFF_NSEVAL[$key]=$(echo "$line" | perl -ne '/"ns_per_eval":([\d.]+)/ && print $1')
    done < <(perl -ne '/"benchmarks":\{(.+)\}/ && do {
        my $b = $1;
        while ($b =~ /"(\w+)":\{([^}]+)\}/g) {
            print "{\"key\":\"$1\",$2}\n";
        }
    }' "$DIFF_FILE")
fi

# ═══════════════════════════════════════════════════════════════════
# Table 1: Timing
# ═══════════════════════════════════════════════════════════════════

printf "${BOLD}═══ Table 1: Timing ═══${NC}\n\n"

if [[ -n "$DIFF_FILE" ]]; then
    printf "${BOLD}%-20s %10s %6s %10s %8s %5s  %-6s${NC}\n" \
        "Benchmark" "Guage(s)" "Δ%" "C(s)" "Ratio" "Δ" "Check"
    printf "%-20s %10s %6s %10s %8s %5s  %-6s\n" \
        "────────────────" "────────" "────" "────────" "──────" "───" "─────"
else
    printf "${BOLD}%-20s %10s %10s %8s  %-6s${NC}\n" \
        "Benchmark" "Guage(s)" "C(s)" "Ratio" "Check"
    printf "%-20s %10s %10s %8s  %-6s\n" \
        "────────────────" "────────" "────────" "──────" "─────"
fi

BENCHMARKS="fib:Fibonacci(28) tak:Tak(21,14,7) ack:Ackermann(3,7) tco_sum:TCO-Sum(500k) primes:Primes(10k)"

total_g=0
total_c=0

# Associative arrays for profile data and timing
declare -A BENCH_GUAGE BENCH_C BENCH_RATIO BENCH_CHECK
declare -A PROF_EVAL PROF_LAMBDA PROF_BUILTIN PROF_TAIL
declare -A PROF_ENVL PROF_ENVS PROF_PRIML PROF_PRIMS
declare -A PROF_ALLOCS PROF_FREES PROF_RETAINS PROF_RELEASES
declare -A PROF_AVGENV PROF_AVGPRIM
declare -A PROF_NSEVAL PROF_NSLAM PROF_ALLOCPERLAM PROF_ENVPERLAM PROF_PRIMPERBUI

for entry in $BENCHMARKS; do
    key="${entry%%:*}"
    label="${entry##*:}"
    scm="$SCRIPT_DIR/bench_${key}.scm"
    cbin="$TMPDIR/bench_${key}"

    # Get results for correctness check
    g_raw=$("$GUAGE" "$scm" 2>/dev/null | grep '^actor-spawn' | tail -1 | sed 's/^actor-spawn #//')
    c_raw=$("$cbin")

    check=$(perl -e '
        my $gn = $ARGV[0] + 0;
        my $cn = $ARGV[1] + 0;
        if ($cn == 0) { print($gn == 0 ? "PASS" : "FAIL"); exit; }
        my $rel = abs($gn - $cn) / abs($cn);
        print($rel < 1e-3 ? "PASS" : "FAIL");
    ' -- "$g_raw" "$c_raw")

    if [ "$check" = "PASS" ]; then
        check_display="${GREEN}PASS${NC}"
    else
        check_display="${RED}${check}${NC}"
    fi

    # Profile run (1 dedicated run with GUAGE_PROFILE=1)
    prof_stderr=$( GUAGE_PROFILE=1 "$GUAGE" "$scm" 2>&1 >/dev/null || true )
    prof_line=$(echo "$prof_stderr" | grep '"t":"profile"' || echo '{}')

    # Extract profile fields
    eval $(echo "$prof_line" | perl -e '
        my $l = <STDIN>;
        my %d;
        while ($l =~ /"(\w+)":(\d+(?:\.\d+)?)/g) { $d{$1} = $2; }
        for my $k (qw(eval_steps lambda_calls builtin_calls tail_calls
                      env_lookups env_steps prim_lookups prim_steps
                      cell_allocs cell_frees retains releases
                      avg_env_depth avg_prim_depth)) {
            my $v = $d{$k} // 0;
            (my $sk = $k) =~ s/_//g;
            print "P_$k=$v\n";
        }
    ')

    # Time Guage (3 runs, no profiling)
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

    ratio=$(echo "scale=0; $gm / $cm" | bc 2>/dev/null || echo "?")

    total_g=$(echo "$total_g + $gm" | bc)
    total_c=$(echo "$total_c + $cm" | bc)

    # Store for later tables
    BENCH_GUAGE[$key]="$gm"
    BENCH_C[$key]="$cm"
    BENCH_RATIO[$key]="$ratio"
    BENCH_CHECK[$key]="$check"
    PROF_EVAL[$key]="${P_eval_steps:-0}"
    PROF_LAMBDA[$key]="${P_lambda_calls:-0}"
    PROF_BUILTIN[$key]="${P_builtin_calls:-0}"
    PROF_TAIL[$key]="${P_tail_calls:-0}"
    PROF_ALLOCS[$key]="${P_cell_allocs:-0}"
    PROF_FREES[$key]="${P_cell_frees:-0}"
    PROF_RETAINS[$key]="${P_retains:-0}"
    PROF_RELEASES[$key]="${P_releases:-0}"
    PROF_AVGENV[$key]="${P_avg_env_depth:-0}"
    PROF_AVGPRIM[$key]="${P_avg_prim_depth:-0}"

    # Derived per-call metrics
    ns_eval=$(perl -e 'printf "%.1f", ($ARGV[1] > 0 ? $ARGV[0]*1e9/$ARGV[1] : 0)' -- "$gm" "${P_eval_steps:-0}")
    ns_lam=$(perl -e 'printf "%.1f", ($ARGV[1] > 0 ? $ARGV[0]*1e9/$ARGV[1] : 0)' -- "$gm" "${P_lambda_calls:-0}")
    alloc_lam=$(perl -e 'printf "%.1f", ($ARGV[1] > 0 ? $ARGV[0]/$ARGV[1] : 0)' -- "${P_cell_allocs:-0}" "${P_lambda_calls:-0}")
    env_lam=$(perl -e 'printf "%.1f", ($ARGV[1] > 0 ? $ARGV[0]/$ARGV[1] : 0)' -- "${P_env_steps:-0}" "${P_lambda_calls:-0}")
    prim_bui=$(perl -e 'printf "%.1f", ($ARGV[1] > 0 ? $ARGV[0]/$ARGV[1] : 0)' -- "${P_prim_steps:-0}" "${P_builtin_calls:-0}")

    PROF_NSEVAL[$key]="$ns_eval"
    PROF_NSLAM[$key]="$ns_lam"
    PROF_ALLOCPERLAM[$key]="$alloc_lam"
    PROF_ENVPERLAM[$key]="$env_lam"
    PROF_PRIMPERBUI[$key]="$prim_bui"

    # Print timing row
    if [[ -n "$DIFF_FILE" && -n "${DIFF_GUAGE[$key]:-}" ]]; then
        dpct=$(perl -e 'printf "%+.0f", (($ARGV[0]-$ARGV[1])/$ARGV[1])*100' -- "$gm" "${DIFF_GUAGE[$key]}")
        dr=$(perl -e 'printf "%+d", $ARGV[0]-$ARGV[1]' -- "$ratio" "${DIFF_RATIO[$key]}")
        printf "%-20s %10.3f %5s%% %10.4f %7sx %4s  " "$label" "$gm" "$dpct" "$cm" "$ratio" "$dr"
    else
        printf "%-20s %10.3f %10.4f %7sx  " "$label" "$gm" "$cm" "$ratio"
    fi
    printf "${check_display}\n"
done

printf "%-20s %10s %10s %8s\n" \
    "────────────────" "────────" "────────" "──────"

tr=$(echo "scale=0; $total_g / $total_c" | bc 2>/dev/null || echo "?")
printf "${BOLD}%-20s %10.3f %10.4f %7sx${NC}\n\n" \
    "TOTAL" "$total_g" "$total_c" "$tr"

# ═══════════════════════════════════════════════════════════════════
# Table 2: Profile Counters
# ═══════════════════════════════════════════════════════════════════

printf "${BOLD}═══ Table 2: Profile Counters ═══${NC}\n\n"
printf "${BOLD}%-20s %10s %8s %8s %8s %10s %10s %8s %8s${NC}\n" \
    "Benchmark" "eval_steps" "lambda" "builtin" "tail" "allocs" "frees" "avg_env" "avg_prim"
printf "%-20s %10s %8s %8s %8s %10s %10s %8s %8s\n" \
    "────────────────" "──────────" "──────" "──────" "──────" "────────" "────────" "──────" "────────"

for entry in $BENCHMARKS; do
    key="${entry%%:*}"
    label="${entry##*:}"
    printf "%-20s %10s %8s %8s %8s %10s %10s %8s %8s\n" \
        "$label" \
        "$(humanize "${PROF_EVAL[$key]}")" \
        "$(humanize "${PROF_LAMBDA[$key]}")" \
        "$(humanize "${PROF_BUILTIN[$key]}")" \
        "$(humanize "${PROF_TAIL[$key]}")" \
        "$(humanize "${PROF_ALLOCS[$key]}")" \
        "$(humanize "${PROF_FREES[$key]}")" \
        "${PROF_AVGENV[$key]}" \
        "${PROF_AVGPRIM[$key]}"
done

printf "\n"

# ═══════════════════════════════════════════════════════════════════
# Table 3: Per-Call Costs (derived)
# ═══════════════════════════════════════════════════════════════════

printf "${BOLD}═══ Table 3: Per-Call Costs ═══${NC}\n\n"

if [[ -n "$DIFF_FILE" ]]; then
    printf "${BOLD}%-20s %10s %6s %10s %14s %14s %16s${NC}\n" \
        "Benchmark" "ns/eval" "Δ%" "ns/lambda" "allocs/lambda" "env_s/lambda" "prim_s/builtin"
    printf "%-20s %10s %6s %10s %14s %14s %16s\n" \
        "────────────────" "──────" "────" "────────" "────────────" "────────────" "──────────────"
else
    printf "${BOLD}%-20s %10s %10s %14s %14s %16s${NC}\n" \
        "Benchmark" "ns/eval" "ns/lambda" "allocs/lambda" "env_s/lambda" "prim_s/builtin"
    printf "%-20s %10s %10s %14s %14s %16s\n" \
        "────────────────" "──────" "────────" "────────────" "────────────" "──────────────"
fi

for entry in $BENCHMARKS; do
    key="${entry%%:*}"
    label="${entry##*:}"
    if [[ -n "$DIFF_FILE" && -n "${DIFF_NSEVAL[$key]:-}" ]]; then
        dpct=$(perl -e 'printf "%+.0f", ($ARGV[1] > 0 ? (($ARGV[0]-$ARGV[1])/$ARGV[1])*100 : 0)' -- "${PROF_NSEVAL[$key]}" "${DIFF_NSEVAL[$key]}")
        printf "%-20s %10s %5s%% %10s %14s %14s %16s\n" \
            "$label" "${PROF_NSEVAL[$key]}" "$dpct" "${PROF_NSLAM[$key]}" \
            "${PROF_ALLOCPERLAM[$key]}" "${PROF_ENVPERLAM[$key]}" "${PROF_PRIMPERBUI[$key]}"
    else
        printf "%-20s %10s %10s %14s %14s %16s\n" \
            "$label" "${PROF_NSEVAL[$key]}" "${PROF_NSLAM[$key]}" \
            "${PROF_ALLOCPERLAM[$key]}" "${PROF_ENVPERLAM[$key]}" "${PROF_PRIMPERBUI[$key]}"
    fi
done

printf "\n"

# ═══════════════════════════════════════════════════════════════════
# Save Snapshot
# ═══════════════════════════════════════════════════════════════════

mkdir -p "$SNAPSHOT_DIR"
TIMESTAMP=$(date +%Y-%m-%d_%H%M%S)
SNAPSHOT="$SNAPSHOT_DIR/${TIMESTAMP}.json"

# Build JSON snapshot
{
    printf '{"timestamp":"%s","benchmarks":{' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    first=1
    for entry in $BENCHMARKS; do
        key="${entry%%:*}"
        [[ $first -eq 0 ]] && printf ","
        first=0
        printf '"%s":{"guage_s":%s,"c_s":%s,"ratio":%s,"check":"%s",' \
            "$key" "${BENCH_GUAGE[$key]}" "${BENCH_C[$key]}" "${BENCH_RATIO[$key]}" "${BENCH_CHECK[$key]}"
        printf '"profile":{"eval_steps":%s,"lambda_calls":%s,"builtin_calls":%s,"tail_calls":%s,' \
            "${PROF_EVAL[$key]}" "${PROF_LAMBDA[$key]}" "${PROF_BUILTIN[$key]}" "${PROF_TAIL[$key]}"
        printf '"cell_allocs":%s,"cell_frees":%s,"retains":%s,"releases":%s,' \
            "${PROF_ALLOCS[$key]}" "${PROF_FREES[$key]}" "${PROF_RETAINS[$key]:-0}" "${PROF_RELEASES[$key]:-0}"
        printf '"avg_env_depth":%s,"avg_prim_depth":%s},' \
            "${PROF_AVGENV[$key]}" "${PROF_AVGPRIM[$key]}"
        printf '"ns_per_eval":%s,"ns_per_lambda":%s,"allocs_per_lambda":%s}' \
            "${PROF_NSEVAL[$key]}" "${PROF_NSLAM[$key]}" "${PROF_ALLOCPERLAM[$key]}"
    done
    printf '}}\n'
} > "$SNAPSHOT"

printf "${CYAN}Snapshot saved: %s${NC}\n" "$SNAPSHOT"

# Create baseline symlink if it doesn't exist
if [[ ! -f "$SNAPSHOT_DIR/baseline.json" ]]; then
    cp "$SNAPSHOT" "$SNAPSHOT_DIR/baseline.json"
    printf "${CYAN}Baseline established: %s${NC}\n" "$SNAPSHOT_DIR/baseline.json"
fi

printf "\n${YELLOW}What the ratios mean:${NC}\n"
printf "  Tree-walking interpreters typically run 50-500x slower than native C.\n"
printf "  Higher ratios on TCO/primes = per-operation dispatch cost dominates.\n"
printf "  Lower ratios on fib/tak/ack = function-call overhead is well-managed.\n"
printf "  C benchmarks use double arithmetic to match Guage internals.\n"
printf "\n${YELLOW}Key profile insights:${NC}\n"
printf "  ns/eval        — Per-dispatch cost (reduce branches in eval loop)\n"
printf "  avg_prim_depth — Primitive lookup O(n) (use hash table / direct index)\n"
printf "  avg_env_depth  — Env lookup O(n) (flat frames for De Bruijn)\n"
printf "  allocs/lambda  — Heap pressure per call (arena / pool allocator)\n"
printf "\n${YELLOW}Diff mode:${NC} ./benchmarks/run_benchmarks.sh --diff benchmarks/snapshots/baseline.json\n"
