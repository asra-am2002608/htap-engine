#Hardware-Accelerated Dual-Format in-Memory Engine for HTAP

## Objective
This project evaluates a selective dual-format HTAP architecture using:

- a primary in-memory row store for OLTP
- selectively materialized analytical columns
- asynchronous batched synchronization
- scalar and SIMD /vectorized OLAP execution

## Current Status
development of the environment and initial c++ build system configured. 

## Requirements
- C++20-compatible compiler
- CMake 3.20+
- python 3.12+
- Python packages listed in `requirements.txt`

## Build
'''Bash
mkdir -p build
cd build
cmake ..
cmake --build .