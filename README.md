NUMA-Allocator-Garbage-Collector

This project implements a NUMA-aware memory allocator combined with a fully automatic garbage collector written in C++. It enables applications to allocate memory efficiently across NUMA nodes and automatically reclaim unused memory without requiring explicit deallocation.
Features

    NUMA-aware allocation

    Tracing garbage collection

    Plug-and-play memory management (no manual freeing required)

    Valgrind-compatible testing

    Includes test examples: hash table, simple objects, random allocations, vectors

Build and Test
Prerequisites

    Linux with NUMA support

    make, g++, and valgrind installed

Building

To compile the project, use:

make            # for release build
make debug      # for debug build (includes debug symbols)

and you can create your own .cpp file to test the allocator out

Test Suite

A run.sh script is provided to compile and run all tests easily.
Usage

./run.sh [OPTIONS]

Options
Flag	Description
-d	Use make debug instead of make
-v	Run tests with valgrind
-h	Show help/usage message
Examples

./run.sh             # Normal build and run
./run.sh -d          # Debug build and run
./run.sh -v          # Run all tests under valgrind
./run.sh -d -v       # Debug build and run with valgrind

Project Structure
File/Folder	Description
allocator.*	NUMA-aware memory allocator implementation
cppGarbageCollector.*	Garbage collector logic
tests/	Contains test programs
run.sh	Script to build and run all tests
