#!/bin/bash

# Guage Test Harness
# Runs all .test files and reports results

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

PASSED=0
FAILED=0
TOTAL=0

echo "═══════════════════════════════════════"
echo "  Guage Test Harness"
echo "═══════════════════════════════════════"
echo

# Find all .test files
TEST_FILES=$(find . -name "*.test" -type f)

if [ -z "$TEST_FILES" ]; then
    echo "${YELLOW}No .test files found${NC}"
    exit 0
fi

# Run each test file
for test_file in $TEST_FILES; do
    echo "Running: $test_file"
    TOTAL=$((TOTAL + 1))

    # Run test with timeout
    if timeout 5 ./guage < "$test_file" > /tmp/guage_test_$$.out 2>&1; then
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
