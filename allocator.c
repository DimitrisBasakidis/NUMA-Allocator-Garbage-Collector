#include <assert.h>

#include "allocator.h"
#include "numa.h"

numa_heap **numa_heaps;

void init_allocator(size_t heap_size) {
    assert(heap_size);
    size_t nodes =  get_numa_nodes_num();
    size_t size = nodes * sizeof(struct numa_heap *);

    numa_heaps = (numa_heap **) alloc(size);

    for (size_t i = 0U; i < nodes; i++) {
	numa_heaps[i] = (numa_heap *) alloc(sizeof(struct numa_heap));
    }

}

void *alloc(size_t size) {
    void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (ptr == MAP_FAILED) {
    	fprintf(stderr, "mmap failed\n");
	return NULL;
    }
	
    return ptr;
}


