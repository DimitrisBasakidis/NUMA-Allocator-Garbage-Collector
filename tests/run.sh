#!/bin/bash

# Enable strict error handling
set -euo pipefail

# Default flag values
USE_VALGRIND=false
USE_DEBUG=false

print_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -v       Run tests under Valgrind"
    echo "  -d       Run 'make debug' instead of 'make'"
    echo "  -h       Show this help message and exit"
    echo ""
    echo "Examples:"
    echo "  $0           Compile and run normally"
    echo "  $0 -d        Use make debug and run normally"
    echo "  $0 -v        Run tests with Valgrind"
    echo "  $0 -d -v     Run make debug and test with Valgrind"
}

# Parse flags
for arg in "$@"; do
    case $arg in
        -v) USE_VALGRIND=true ;;
        -d) USE_DEBUG=true ;;
        -h) print_usage; exit 0 ;;
        *) echo "Unknown option: $arg"; print_usage; exit 1 ;;
    esac
done

# Choose make target
if $USE_DEBUG; then
    echo "Running: make debug"
    make debug
else
    echo "Running: make"
    make
fi

# Array of test sources (without extensions)
tests=("hash" "simple" "randomAllocations" "vectors")

# Object file dependencies (adjust paths if needed)
OBJS="numa.o util.o allocator.o cppGarbageCollector.o"

# Compiler and flags
CXX=g++
CXXFLAGS="-g -O0 -std=c++17"

# Compile and run each test
for test in "${tests[@]}"; do
    echo "Compiling $test.cpp..."
    $CXX $CXXFLAGS "$test.cpp" $OBJS -o "$test"

    echo "Running $test..."
    if $USE_VALGRIND; then
        valgrind --leak-check=full --show-leak-kinds=all "./$test"
    else
        ./"$test"
    fi

    echo "--------------------------------------------"

    # Delete the binary
    rm "$test"
done

