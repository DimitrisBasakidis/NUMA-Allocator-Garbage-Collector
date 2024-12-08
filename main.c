#include <stdio.h>
#include <stdlib.h>

#include "allocator.h"

#define NUM_ITERATIONS 10
#define MAX_THREADS 16

// Timing utilities
double get_time_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e9 + ts.tv_nsec;
}

// Benchmark 1: Latency for single-threaded allocations
void benchmark_latency(size_t alloc_size) {
    printf("Benchmarking Latency for Allocations of Size %zu Bytes:\n", alloc_size);

    double start_time = get_time_ns();
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        void *ptr = allocate_localy(alloc_size); // NUMA allocator
        deallocate(ptr);
    }
    double numa_time = (get_time_ns() - start_time) / NUM_ITERATIONS;

    start_time = get_time_ns();
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        void *ptr = malloc(alloc_size); // Standard malloc
        free(ptr);
    }
    double malloc_time = (get_time_ns() - start_time) / NUM_ITERATIONS;

    printf("NUMA Allocator Latency: %.2f ns\n", numa_time);
    printf("Malloc Latency: %.2f ns\n\n", malloc_time);
}

// Benchmark 2: Throughput with multiple threads
void *thread_alloc_work(void *arg) {
    size_t alloc_size = *(size_t *)arg;
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        void *ptr = allocate_localy(alloc_size);
        deallocate(ptr);
    }
    return NULL;
}

void benchmark_throughput(size_t alloc_size, int num_threads) {
    printf("Benchmarking Throughput with %d Threads and Allocations of Size %zu Bytes:\n", num_threads, alloc_size);

    pthread_t threads[MAX_THREADS];
    double start_time = get_time_ns();

    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, thread_alloc_work, &alloc_size);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    double numa_time = (get_time_ns() - start_time) / NUM_ITERATIONS / num_threads;

    start_time = get_time_ns();

    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, thread_alloc_work, &alloc_size);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    double malloc_time = (get_time_ns() - start_time) / NUM_ITERATIONS / num_threads;

    printf("NUMA Allocator Throughput: %.2f ns per allocation\n", numa_time);
    printf("Malloc Throughput: %.2f ns per allocation\n\n", malloc_time);
}

// Benchmark 3: NUMA Locality Test
void benchmark_numa_locality(size_t alloc_size) {
    printf("Benchmarking NUMA Locality for Allocations of Size %zu Bytes:\n", alloc_size);

    void *ptr = allocate_localy(alloc_size);
    double start_time = get_time_ns();

    // Access the allocated memory
    for (size_t i = 0; i < alloc_size; i += 64) {
        ((char *)ptr)[i] = i % 256;
    }

    double numa_access_time = get_time_ns() - start_time;

    ptr = malloc(alloc_size);
    start_time = get_time_ns();

    for (size_t i = 0; i < alloc_size; i += 64) {
        ((char *)ptr)[i] = i % 256;
    }

    double malloc_access_time = get_time_ns() - start_time;

    printf("NUMA Allocator Access Time: %.2f ns\n", numa_access_time / alloc_size);
    printf("Malloc Access Time: %.2f ns\n\n", malloc_access_time / alloc_size);
}

// Main benchmarking function
int main() {
    init_allocator(1024 * 1024 * 24);
    size_t sizes[] = {64, 256, 1024, 4096, 16384}; // Different allocation sizes
    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
        benchmark_latency(sizes[i]);
        benchmark_throughput(sizes[i], 4); // 4 threads
        benchmark_numa_locality(sizes[i]);
    }

    free_allocator();
    return 0;
}
