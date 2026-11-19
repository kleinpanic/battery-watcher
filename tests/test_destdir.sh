#!/bin/sh
# Regression check for staged installs: `make uninstall DESTDIR=DIR` must undo
# exactly what `make install DESTDIR=DIR` wrote inside that same staging root.
# Run from the repository root: sh tests/test_destdir.sh
set -eu

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
STAGE="${TMPDIR:-/tmp}/bw-destdir-$$"

rm -rf "$STAGE"
make -C "$ROOT_DIR" install DESTDIR="$STAGE" >/dev/null

if [ ! -x "$STAGE/usr/local/bin/battery-watcher" ]; then
    echo "FAIL (install did not stage the binary under $STAGE)"
    exit 1
fi

make -C "$ROOT_DIR" uninstall DESTDIR="$STAGE" >/dev/null

if [ -e "$STAGE/usr/local/bin/battery-watcher" ]; then
    echo "FAIL (uninstall left $STAGE/usr/local/bin/battery-watcher behind)"
    exit 1
fi

rm -rf "$STAGE"
echo "PASS (DESTDIR honored by both targets)"
