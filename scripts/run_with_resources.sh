#!/bin/bash

set -e

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"
OUTPUT="$ROOT_DIR/results/raw/resource_trace.csv"
echo "timestamp,pid,cpu_percent,rss_kb" > "$OUTPUT"

cd "$BUILD_DIR"

./htap_engine 1000000 4 5000 50 16 &
PID=$!

while kill -0 "$PID" 2>/dev/null
do
    TIMESTAMP=$(date +%s)
    PS_OUTPUT=$(ps -p "$PID" -o %cpu=,rss=)
    CPU=$(echo "$PS_OUTPUT" | awk '{print $1}')
    RSS=$(echo "$PS_OUTPUT" | awk '{print $2}')

    echo "$TIMESTAMP,$PID,$CPU,$RSS" >> "$OUTPUT"
    sleep 1

done

wait "$PID"

echo "Resources trace saved to:"
echo "$OUTPUT"
