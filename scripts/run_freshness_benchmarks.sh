#!/bin/bash

set -e
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"

ROWS=1000000
THREADS=4
OPERATIONS=1000
OLTP=50
MODE="selective_simd"

BATCH_LIST=(1 4 16 64)
REPEAT_COUNT=5

cd "$BUILD_DIR"

for batch in "${BATCH_LIST[@]}"
do
    for run in $(seq 1 "$REPEAT_COUNT")
    do
        echo "-------------"
        echo "Freshness experiment"
        echo "Rows=$ROWS"
        echo "Threads=$THREADS"
        echo "OLTP=$OLTP%"
        echo "Batch=$batch"
        echo "Run=$run/$REPEAT_COUNT"
        echo "-------------"

        ./htap_engine \
        "$ROWS" \
        "$THREADS" \
        "$OPERATIONS" \
        "$OLTP" \
        "$batch" \
        "$run" \
        "$MODE"
    done
done

echo
echo "Freshness benchmark complete."