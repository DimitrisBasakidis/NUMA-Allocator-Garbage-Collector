#include <math.h>
#include <stdio.h>
#include <unistd.h>

#include "util.h"

size_t get_bin_index(size_t size) {
    for (size_t index = 0U; index < BINS; index++){
	if (size <= (16 * pow(2, index))) return index;
    }

    return BINS;
}

void print_allocation_info(void *ptr, size_t size) {
    // Get the CPU the thread is running on
    int cpu = sched_getcpu();
    if (cpu == -1) {
        perror("sched_getcpu failed");
        return;
    }

    // Determine the NUMA node for the current CPU
    int numa_node = cpu_on_node[cpu];

    // Print debug information
    printf("Allocated memory at address: %p\n", ptr);
    printf("Allocation size: %zu bytes\n", size);
    printf("Thread is running on CPU: %d\n", cpu);
    if (numa_node != -1) {
        printf("CPU belongs to NUMA node: %d\n", numa_node);
    } else {
        printf("NUMA node information unavailable for CPU: %d\n", cpu);
    }
}

void print_heap(numa_heap **numa_heaps, int node) {
    if ((size_t)node >= get_numa_nodes_num()) {
        fprintf(stderr, "Invalid NUMA node: %d\n", node);
        return;
    }

    numa_heap *heap = numa_heaps[node];
    printf("Heap for NUMA Node %d:\n", node);
    printf("  Start Address: %p\n", heap->start_addr);
    printf("  Total Size: %zu bytes\n", heap->heap_size);
    printf("  Free Lists:\n");

    for (size_t bin = 0; bin < BINS; bin++) {
        printf("  Bin %zu:\n", bin);
        free_block *current = heap->free_list[bin];

        while (current) {
            printf("    Block Address: %p, Size: %zu bytes\n", current->starting_addr, current->size);
            current = (free_block *) current->next;
        } 
    }
}


