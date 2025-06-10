#!/bin/bash

# Enable strict error handling
set -euo pipefail

# Default flag values
USE_VALGRIND=false
USE_DEBUG=false
EVAL_ONLY=false

print_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -v       Run tests under Valgrind"
    echo "  -d       Run 'make debug' instead of 'make'"
    echo "  -eval    Show evaluation results for the allocator"
    echo "  -h       Show this help message and exit"
    echo ""
    echo "Examples:"
    echo "  $0           Compile and run normally"
    echo "  $0 -d        Use make debug and run normally"
    echo "  $0 -v        Run tests with Valgrind"
    echo "  $0 -d -v     Run make debug and test with Valgrind"
    echo "  $0 -eval       Compile and compare NUMA vs malloc results"
}

# Parse flags
for arg in "$@"; do
    case $arg in
        -v) USE_VALGRIND=true ;;
        -d) USE_DEBUG=true ;;
        -eval) EVAL_ONLY=true ;;
        -h) print_usage; exit 0 ;;
        *) echo "Unknown option: $arg"; print_usage; exit 1 ;;
    esac
done

if $EVAL_ONLY; then
    if [ $# -ne 1 ]; then
        echo "[ERROR] -eval must be used alone."
        print_usage
        exit 1
    fi

    echo "[INFO] Running allocator evaluation..."

    make numa_alloc 

    # Compile with NUMA local allocations
    gcc -D_GNU_SOURCE -DNUMA_ALLOC -Wall -DLOCAL -Wextra -O2 eval_allocator.c allocator.o numa.o util.o -o eval_allocator_numa -pthread -lm

    # Compile with NUMA interleaved allocations
    gcc -D_GNU_SOURCE -DNUMA_ALLOC -DINTERLEAVED -Wall -Wextra -O2 eval_allocator.c allocator.o numa.o util.o -o eval_allocator_numa_int -pthread -lm

    # Compile with malloc
    gcc -D_GNU_SOURCE -Wall -Wextra -O2 eval_allocator.c allocator.o numa.o util.o -o eval_allocator -pthread -lm

    gcc -o eval_mixed eval_allocator_mixed.c allocator.o numa.o util.o -pthread -lm
    gcc -DNUMA_ALLOC -DLOCAL -o eval_mixed_local eval_allocator_mixed.c allocator.o numa.o util.o -pthread -lm
    gcc -DNUMA_ALLOC -DINTERLEAVED -o eval_mixed_int eval_allocator_mixed.c allocator.o numa.o util.o -pthread -lm

    # Run and capture results
    ./eval_allocator_numa > numa_eval.txt
    ./eval_allocator_numa_int > numa_eval_int.txt
    ./eval_allocator > malloc_eval.txt

    ./eval_mixed > malloc_mixed.txt
    ./eval_mixed_local > numa_mixed_local.txt
    ./eval_mixed_int > numa_mixed_int.txt

    echo "[INFO] Comparing outputs for heap allocations test..."
    diff malloc_eval.txt numa_eval.txt || {
      echo "[DIFF] Differences found between malloc and NUMA local output."
    }

    diff malloc_eval.txt numa_eval_int.txt || {
      echo "[DIFF] Differences found between malloc and NUMA interleaved output."
    }

    echo "[INFO] Comparing outputs for mixed allocations and deallocations test..."

    diff malloc_mixed.txt numa_mixed_local.txt || {
      echo "[DIFF] Differences found between malloc and NUMA local output."
    }

    diff malloc_mixed.txt numa_mixed_int.txt || {
      echo "[DIFF] Differences found between malloc and NUMA interleaved output."
      exit 0;
    }
fi

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

