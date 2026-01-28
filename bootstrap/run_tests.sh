#!/bin/bash

# Guage Test Harness
# Runs all .test files and reports results
# ALWAYS run from project root: /path/to/guage/

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

PASSED=0
FAILED=0
TOTAL=0

# Verify we have the executable (must be at project root)
if [ ! -f "bootstrap/guage" ]; then
    echo "${RED}Error: bootstrap/guage not found${NC}"
    echo "You must run this script from the project root directory"
    echo "Current directory: $(pwd)"
    exit 1
fi

GUAGE="bootstrap/guage"
TESTS_DIR="bootstrap/tests"

echo "═══════════════════════════════════════"
echo "  Guage Test Harness"
echo "═══════════════════════════════════════"
echo "  Root: $(pwd)"
echo "  Executable: $GUAGE"
echo "  Tests: $TESTS_DIR"
echo "═══════════════════════════════════════"
echo

# Find all .test files (relative to project root)
TEST_FILES=$(find "$TESTS_DIR" -name "*.test" -type f 2>/dev/null | sort)

if [ -z "$TEST_FILES" ]; then
    echo "${YELLOW}No .test files found in $TESTS_DIR${NC}"
    exit 0
fi

# Run each test file (all paths relative to project root)
for test_file in $TEST_FILES; do
    echo "Running: $test_file"
    TOTAL=$((TOTAL + 1))

    # Run test with timeout (no cd, everything from project root)
    if timeout 5 "$GUAGE" < "$test_file" > /tmp/guage_test_$$.out 2>&1; then
        # Check for failures in output
        if grep -q "✗ FAIL" /tmp/guage_test_$$.out || grep -q "⚠:test-failed" /tmp/guage_test_$$.out; then
            echo "${RED}  ✗ FAILED${NC}"
            FAILED=$((FAILED + 1))
            cat /tmp/guage_test_$$.out
        else
            echo "${GREEN}  ✓ PASSED${NC}"
            PASSED=$((PASSED + 1))
        fi
    else
        echo "${RED}  ✗ TIMEOUT/ERROR${NC}"
        FAILED=$((FAILED + 1))
    fi
    echo
done

rm -f /tmp/guage_test_$$.out

# Summary
echo "═══════════════════════════════════════"
echo "  Test Summary"
echo "═══════════════════════════════════════"
echo "Total:  $TOTAL"
echo "${GREEN}Passed: $PASSED${NC}"
echo "${RED}Failed: $FAILED${NC}"
echo

if [ $FAILED -eq 0 ]; then
    echo "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo "${RED}Some tests failed!${NC}"
    exit 1
fi
