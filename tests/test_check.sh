#!/bin/sh
# Build tests/test_thresholds.c together with src/main.c and run it.
# Run from the repository root: sh tests/test_check.sh
set -eu

gcc -Wall -Wextra -O2 -I./include -c -Dmain=bw_main src/main.c -o bw-main.o
gcc -Wall -Wextra -O2 -I./include -o bw-check-test \
    tests/test_thresholds.c bw-main.o

status=0
./bw-check-test || status=$?
rm -f bw-check-test bw-main.o
exit "$status"
