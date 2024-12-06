#ifndef UTIL
#define UTIL

#include "allocator.h"
#include "numa.h"

extern numa_heap **numa_heaps;

size_t get_bin_index(size_t size);
void print_allocation_info(void *ptr, size_t size);
void print_heap(int node);

#endif
