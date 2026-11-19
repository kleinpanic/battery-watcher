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

# Test 3: Interval validation, plus the daemonized child stays stoppable.
echo -n "Test 3: minimum interval (10s)... "
if [ -n "${XDG_RUNTIME_DIR:-}" ]; then
    PIDFILE="$XDG_RUNTIME_DIR/battery-watcher.pid"
elif [ -n "${HOME:-}" ]; then
    PIDFILE="$HOME/.run/battery-watcher.pid"
else
    PIDFILE="/run/battery-watcher.pid"
fi
rm -f "$PIDFILE"
if ! timeout 2 "$BINARY" -i 5 -d >/dev/null 2>&1; then
    # Should clamp to 10, not error
    echo "FAIL"
    exit 1
fi

# -d forks the worker and the parent exits immediately, so the pid file is the
# only handle on that worker. The daemon must write it where this user can read
# it even when /run is only writable by root, and it must remove that file when
# it reacts to SIGTERM instead of sleeping out the whole poll interval.
left=50
while [ ! -r "$PIDFILE" ] && [ "$left" -gt 0 ]; do
    sleep 0.1
    left=$((left - 1))
done
if [ ! -r "$PIDFILE" ]; then
    echo "FAIL (daemon left no readable pid file at $PIDFILE)"
    exit 1
fi
BW_PID=$(cat "$PIDFILE")
case "$BW_PID" in
    ''|*[!0-9]*) echo "FAIL (pid file holds '$BW_PID')"; exit 1 ;;
esac
if ! kill -0 "$BW_PID" 2>/dev/null; then
    echo "FAIL (pid $BW_PID from $PIDFILE is not running)"
    exit 1
fi
kill -TERM "$BW_PID" 2>/dev/null
left=150
while [ -e "$PIDFILE" ] && [ "$left" -gt 0 ]; do
    sleep 0.1
    left=$((left - 1))
done
if [ -e "$PIDFILE" ]; then
    echo "FAIL (daemon $BW_PID did not stop after SIGTERM)"
    exit 1
fi
echo "PASS (clamped)"

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

# Test 6: one reading below several thresholds sends exactly one notification
echo -n "Test 6: single reading emits exactly one notification... "
TEST_BIN="$ROOT_DIR/bw-threshold-test"
gcc -Wall -Wextra -O2 -I"$ROOT_DIR/include" -c -Dmain=bw_main \
    "$ROOT_DIR/src/main.c" -o "$ROOT_DIR/bw-main.o"
gcc -Wall -Wextra -O2 -I"$ROOT_DIR/include" \
    -o "$TEST_BIN" "$ROOT_DIR/tests/test_thresholds.c" "$ROOT_DIR/bw-main.o"
OUT=$(BATTERY_NOTIFY_CMD=/bin/echo "$TEST_BIN" 2>/dev/null)
rm -f "$TEST_BIN" "$ROOT_DIR/bw-main.o"
NOTICES=$(printf '%s\n' "$OUT" | grep -c 'Battery at' || true)
if [ "$NOTICES" -eq 1 ]; then
    echo "PASS"
else
    echo "FAIL (expected 1 notification, got $NOTICES)"
    exit 1
fi

# Test 7: a staged install is reversible: uninstall must undo the install it is
# paired with inside the same DESTDIR root, not a different tree.
echo -n "Test 7: DESTDIR-aware uninstall... "
if ! sh "$ROOT_DIR/tests/test_destdir.sh" >/dev/null; then
    echo "FAIL"
    exit 1
fi
echo "PASS"

echo ""
echo "=== All tests passed ==="
