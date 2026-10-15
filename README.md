#Hardware-Accelerated Dual-Format in-Memory Engine for HTAP

## Objective
This project evaluates a selective dual-format HTAP archetecture using:

- a primary in-memory row store for OLTP
- selectivly materialized analatical columns
- asynchronous batched syncroniczation
- scalar and SIMD /vectorized OLAP execution

## Current Status
development of the environment and initial c++ build system configured. 

## Requirements
- c++20 compiler
-CMAke 3.20+
-python 3,12+

## Build
'''Bash
mkdire -p build
cd build
cmake ..
cmake --build .