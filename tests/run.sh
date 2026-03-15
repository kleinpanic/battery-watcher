#!/bin/bash
# Test script for battery-watcher

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
BINARY="$ROOT_DIR/battery-watcher"

echo "=== Battery Watcher Tests ==="

# Test 1: Help flag
echo -n "Test 1: --help flag... "
if "$BINARY" -h 2>&1 | grep -q "Battery watcher"; then
    echo "PASS"
else
    echo "FAIL"
    exit 1
fi

# Test 2: Invalid flag
echo -n "Test 2: invalid flag handling... "
if "$BINARY" -z 2>&1; then
    echo "FAIL (should have exited non-zero)"
    exit 1
else
    echo "PASS"
fi

# Test 3: Interval validation
echo -n "Test 3: minimum interval (10s)... "
if timeout 2 "$BINARY" -i 5 -d 2>&1; then
    # Should clamp to 10, not error
    echo "PASS (clamped)"
else
    echo "FAIL"
    exit 1
fi

# Test 4: Binary exists and is executable
echo -n "Test 4: binary is executable... "
if [ -x "$BINARY" ]; then
    echo "PASS"
else
    echo "FAIL"
    exit 1
fi

# Test 5: Check for hardcoded secrets
echo -n "Test 5: no hardcoded secrets... "
if grep -rE "password|secret|token|api.key" "$ROOT_DIR/src" "$ROOT_DIR/include" 2>/dev/null; then
    echo "FAIL (found potential secrets)"
    exit 1
else
    echo "PASS"
fi

echo ""
echo "=== All tests passed ==="
