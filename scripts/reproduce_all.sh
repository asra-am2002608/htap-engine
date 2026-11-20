#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build_repro"

cd "$ROOT_DIR"

echo "Cleaning reproduction build"
rm -rf "$BUILD_DIR"

echo "Configuring project"
cmake -S . -B "$BUILD_DIR"

echo "Building project"
cmake --build "$BUILD_DIR" -j

echo "Aggregating repeated benchmark runs"
python3 scripts/analyze_repeated_runs.py

echo "Aggregating freshness results"
python3 scripts/analyze_freshness.py

echo "Regenerating paper figures"
python3 scripts/plot_results.py

echo
echo "Reproduction complete."
echo "Summary files:"
echo "$ROOT_DIR/results/summary"
echo
echo "Figures:"
echo "$ROOT_DIR/results/plots"