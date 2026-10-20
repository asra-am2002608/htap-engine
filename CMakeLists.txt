cmake_minimum_required(VERSION 3.20)

if(APPLE)
    set(CMAKE_OSX_SYSROOT
        "/Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk"
        CACHE PATH "macOS SDK path"
        FORCE
    )
endif()

project(htap_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSION OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(htap_engine src/main.cpp src/row_store.cpp src/data_generator.cpp src/column_store.cpp src/selective_column_store.cpp src/workload_profiler.cpp src/change_buffer.cpp src/workload_runner.cpp src/metrics.cpp)

target_include_directories(htap_engine PRIVATE include)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(htap_engine PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -O2
    )
endif()