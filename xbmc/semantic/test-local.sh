#!/bin/bash
# Local testing script for semantic module
# Run from xbmc root: ./xbmc/semantic/test-local.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
XBMC_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "=== Semantic Module Local Test ==="
echo "XBMC Root: $XBMC_ROOT"
echo ""

# Check if we have cppcheck
if command -v cppcheck &> /dev/null; then
    echo "=== Running cppcheck ==="
    cppcheck --enable=warning,style,performance \
        --suppress=missingIncludeSystem \
        --suppress=missingInclude \
        --suppress=unusedFunction \
        --suppress=noExplicitConstructor \
        --error-exitcode=0 \
        -I "$XBMC_ROOT/xbmc" \
        -I "$XBMC_ROOT/xbmc/semantic" \
        "$XBMC_ROOT/xbmc/semantic/" 2>&1 | tee /tmp/cppcheck-semantic.txt

    ERRORS=$(grep -c '\[error\]' /tmp/cppcheck-semantic.txt || echo 0)
    WARNINGS=$(grep -c '\[warning\]' /tmp/cppcheck-semantic.txt || echo 0)
    echo ""
    echo "cppcheck: $ERRORS errors, $WARNINGS warnings"
else
    echo "cppcheck not found. Install with: brew install cppcheck"
fi

echo ""

# Count files and lines
echo "=== Code Statistics ==="
CPP_FILES=$(find "$XBMC_ROOT/xbmc/semantic" -name "*.cpp" | wc -l | tr -d ' ')
H_FILES=$(find "$XBMC_ROOT/xbmc/semantic" -name "*.h" | wc -l | tr -d ' ')
LINES=$(find "$XBMC_ROOT/xbmc/semantic" -name "*.cpp" -o -name "*.h" | xargs wc -l 2>/dev/null | tail -1 | awk '{print $1}')
TEST_FILES=$(find "$XBMC_ROOT/xbmc/semantic" -name "Test*.cpp" | wc -l | tr -d ' ')

echo "Source files: $CPP_FILES .cpp, $H_FILES .h"
echo "Total lines: $LINES"
echo "Test files: $TEST_FILES"

echo ""

# Check for TODOs
echo "=== Remaining TODOs ==="
TODO_COUNT=$(grep -r "TODO" "$XBMC_ROOT/xbmc/semantic" --include="*.cpp" --include="*.h" | wc -l | tr -d ' ')
echo "TODO comments: $TODO_COUNT"
if [ "$TODO_COUNT" -gt 0 ]; then
    grep -r "TODO" "$XBMC_ROOT/xbmc/semantic" --include="*.cpp" --include="*.h" | head -10
fi

echo ""
echo "=== Done ==="
