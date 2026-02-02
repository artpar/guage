#!/bin/bash

# Guage Test Harness — exit-code driven, structured JSON Lines
# ALWAYS run from project root: /path/to/guage/

set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

GUAGE="bootstrap/guage"
TESTS_DIR="bootstrap/tests"
TIMEOUT=${TIMEOUT:-10}
FILTER="${1:-*.test}"

# Verify executable
if [ ! -f "$GUAGE" ]; then
    echo -e "${RED}Error: $GUAGE not found. Run 'make' first.${NC}"
    exit 1
fi

# Discover test files (compatible with bash 3.2)
FILES=()
while IFS= read -r line; do
    FILES+=("$line")
done < <(find "$TESTS_DIR" -name "$FILTER" -type f 2>/dev/null | sort)
TOTAL=${#FILES[@]}

if [ "$TOTAL" -eq 0 ]; then
    echo -e "${YELLOW}No test files matching '$FILTER' in $TESTS_DIR${NC}"
    exit 0
fi

# Temp dir for per-file JSON Lines
JSON_DIR=$(mktemp -d)
trap "rm -rf $JSON_DIR" EXIT

PASSED=0
FAILED=0
ERRORS=()

echo "═══ Guage Test Suite ═══"

run_one() {
    local f="$1" idx="$2"
    if timeout "$TIMEOUT" "$GUAGE" --test "$f" 2>"$JSON_DIR/$idx.jsonl" >"$JSON_DIR/$idx.out"; then
        return 0
    else
        return 1
    fi
}

for i in "${!FILES[@]}"; do
    f="${FILES[$i]}"
    name="${f#$TESTS_DIR/}"
    if run_one "$f" "$i"; then
        ((PASSED++))
        echo -e "  ${GREEN}PASS${NC} $name"
    else
        ((FAILED++))
        ERRORS+=("$name")
        echo -e "  ${RED}FAIL${NC} $name"
        # Show captured stdout (has human-readable failure details)
        if [ -s "$JSON_DIR/$i.out" ]; then
            sed 's/^/         /' "$JSON_DIR/$i.out"
        fi
    fi
done

echo ""
echo "═══ $PASSED/$TOTAL passed ═══"

if [ ${#ERRORS[@]} -gt 0 ]; then
    echo ""
    echo -e "${RED}Failed:${NC}"
    for e in "${ERRORS[@]}"; do
        echo "  $e"
    done
fi

# Write combined JSON results if requested
if [ -n "${TEST_RESULTS:-}" ]; then
    cat "$JSON_DIR"/*.jsonl > "$TEST_RESULTS" 2>/dev/null || true
fi

[ "$FAILED" -eq 0 ]
