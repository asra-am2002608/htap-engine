#!/bin/bash

set -e

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"

SUMMARY="$ROOT_DIR/results/summary/benchmark_summary.csv"

mkdir -p "$ROOT_DIR/results/raw"
mkdir -p "$ROOT_DIR/results/summary"
mkdir -p "$ROOT_DIR/results/plots"

rm -f "$SUMMARY"
cd "$BUILD_DIR"

ROWS_LIST=(100000 1000000)
THREAD_LIST=(1 2 4 8)
OLTP_LIST=(80 50 20)
BATCH_LIST=(16)

MODE_LIST=(
    row
    full_scalar
    full_simd
    selective_simd
)

OPERATIONS_PER_THREAD=1000
REPEAT_COUNT=5

for mode in "${MODE_LIST[@]}"
do
    for rows in "${ROWS_LIST[@]}"
    do
        for threads in "${THREAD_LIST[@]}"
        do
            for oltp in "${OLTP_LIST[@]}"
            do
                for batch in "${BATCH_LIST[@]}"
                do
                    for run in $(seq 1 "$REPEAT_COUNT")
                    do
                        echo "-------------------"
                        echo "Mode=$mode"
                        echo "Rows=$rows"
                        echo "Threads=$threads"
                        echo "OLTP=$oltp%"
                        echo "Batch=$batch"
                        echo "Run=$run/$REPEAT_COUNT"
                        echo "-------------------"

                        ./htap_engine \
                            "$rows" \
                            "$threads" \
                            "$OPERATIONS_PER_THREAD" \
                            "$oltp" \
                            "$batch" \
                            "$run" \
                            "$mode"
                    done
                done
            done
        done
    done
done

echo
echo "Benchmark suite complete."
echo "Summary:"
echo "$SUMMARY"
