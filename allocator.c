#include <assert.h>
#include <sched.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>

#include "allocator.h"
#include "numa.h"

numa_heap **numa_heaps;
void ***free_lists_starting_addr;
free_block *last;

void *mem_alloc(size_t size) {
    void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (ptr == MAP_FAILED) {
    	fprintf(stderr, "mmap failed\n");
	return NULL;
    }
	
    return ptr;
}

void mem_dealloc(void *ptr, size_t size) {
    if (munmap(ptr, size) < 0) {
    	fprintf(stderr, "munmap failed\n");
    }

    return;
}

/*
 * After pinning a thread to the NUMA node and allocating the memory block the thread should be able
 * to run on any CPU in the system, but without restoring it's affinity its pinned on the NUMA node.
 */
void restore_thread_affinity(void) {
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set); // Start with an empty CPU set

    // Add all available CPUs to the set
    for (size_t cpu = 0U; cpu < (size_t) sysconf(_SC_NPROCESSORS_CONF); cpu++) {
        CPU_SET(cpu, &cpu_set);
    }

    // Apply the CPU set to allow execution on all CPUs
    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set) < 0) {
        fprintf(stderr, "Failed to restore thread affinity\n");
	return;
    }
}

/*
 * Thread affinity must be set to a specified cpu in order to be able to allocate memory on it
 * so this function creates a set and based on the cpu_on_node array it puts them into the corret
 * node set.
 */ 
void set_thread_affinity(int node) {
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);  // create empty cpu set

    //  add CPUs that belond to the specified NUMA node to the cpu set
    for (size_t cpu = 0U; cpu < (size_t) sysconf(_SC_NPROCESSORS_CONF); cpu++) {
        if (cpu_on_node[cpu] == node) CPU_SET(cpu, &cpu_set);
    }

    // restrict the thread to the CPU in the target NUMA node
    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set) < 0) {
        fprintf(stderr, "sched_setaffinity failed\n");
	return;
    }
}

/*
 * memory allocation starts off as vitrual memory and this function it 
 * iterates through the memory to enforce physical allocation
 * *page = 0; triggers a page fault which causes the OS to allocate a 
 * physical page of memory on the NUMA node that the thread is bound to.
 */
void touch_memory(void *ptr, size_t size) {
    size_t page_size = sysconf(_SC_PAGESIZE);

    // iterate over the memory in page-sized increments
    for (size_t offset = 0U; offset < size; offset += page_size) {
    	volatile char *page = (char *)ptr + offset;
	*page = 0; // Write on the page for physical memory allocation 
    }
}

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

void print_heap(int node) {
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

void initialize_free_lists(numa_heap **heap_addr) {
    numa_heap *heap = *heap_addr;
    size_t bin_capacity = heap->heap_size / BINS; // Divide heap into bin-sized chunks
    size_t current_offset = 0;
    for (size_t index = 0U; index < BINS; index++){
	long bin_size = 16 * pow(2, index);
        bin_capacity = heap->heap_size / BINS;

    	if (index == BINS - 1) {
            bin_capacity = heap->heap_size - current_offset;
        }

        size_t remaining = bin_capacity;
 
        while (remaining >= (long unsigned int) bin_size) {
	    free_block *new_block = (free_block *) mem_alloc(sizeof(free_block));
            new_block->starting_addr = (free_block *)((char *)heap->start_addr + current_offset);
            new_block->size = bin_size;
	    new_block->next = NULL;

	    if (heap->free_list[index] == NULL) { 
	        heap->free_list[index] = new_block;
		last = new_block;
	    } else {
		last->next = (struct free_block *) new_block;
		last = new_block;
		    /*
		new_block->next = (struct free_block *) heap->free_list[index];
		heap->free_list[index] = new_block;
		*/
	    }

            current_offset += bin_size;
            remaining -= bin_size;
        }
	if (heap->free_list[index])
	    free_lists_starting_addr[index] = heap->free_list[index]->starting_addr;
    }

}

void init_allocator(size_t heap_size) {
    assert(heap_size > 0);
    size_t nodes =  get_numa_nodes_num();
    size_t size = nodes * sizeof(struct numa_heap *);

    numa_heaps = (numa_heap **) mem_alloc(size);
    free_lists_starting_addr = (void ***) mem_alloc(sizeof(void ***));

    for (size_t i = 0U; i < nodes; i++) {
	numa_heaps[i] = (numa_heap *) mem_alloc(sizeof(numa_heap));
	free_lists_starting_addr[i] = (void **) mem_alloc(sizeof(void **));

	set_thread_affinity(i);

	numa_heaps[i]->start_addr = mem_alloc(heap_size);
	touch_memory(numa_heaps[i]->start_addr, heap_size);

	numa_heaps[i]->heap_size = heap_size;
	numa_heaps[i]->numa_node = i;

	for (size_t bin = 0U; bin < BINS; bin++) {
	   numa_heaps[i]->free_list[bin] = NULL;
	}

        initialize_free_lists(&numa_heaps[i]);
	
	if (pthread_mutex_init(&numa_heaps[i]->lock, NULL) != 0) {
            fprintf(stderr, "Failed to initialize mutex for NUMA heap %zu\n", i);
            return;
        }

        printf("Initialized NUMA heap for node %zu in address %p with size %zu bytes in cpu %d\n", i, numa_heaps[i]->start_addr, heap_size, sched_getcpu());
    }

    restore_thread_affinity();
}


void *numa_alloc(size_t size, int node) {
    assert((size_t) node < get_numa_nodes_num());
    numa_heap *heap = numa_heaps[node];

    pthread_mutex_lock(&heap->lock);

    size_t bin_index = get_bin_index(size);

    //printf("index %ld\n", bin_index);
    if (bin_index == BINS) {
	    // implement direcly mmap
    	pthread_mutex_unlock(&heap->lock);
	return NULL;
    }
    free_block *ptr = heap->free_list[bin_index];
    free_block *prev = NULL;

    while (ptr != NULL) {
	if (ptr->size >= size) {
	    void *temp = ptr->starting_addr;
	    if (prev != NULL) prev->next = ptr->next;
	    else heap->free_list[bin_index] = (free_block *) ptr->next;

	    mem_dealloc(ptr, ptr->size);
            pthread_mutex_unlock(&heap->lock);
            return temp;
	}
    
    	prev = ptr;
	ptr = (free_block *) ptr->next;
    }

    pthread_mutex_unlock(&heap->lock);
    return NULL;
}

void free_allocator(void) {
    size_t nodes =  get_numa_nodes_num();

    for (size_t i = 0U; i < nodes; i++) {
	numa_heap *heap = numa_heaps[i];

            break;
	for (size_t bin = 0U; bin < BINS; bin++) {
	    free_block *ptr = heap->free_list[bin];
	    free_block *to_free = NULL;

	    while (ptr != NULL) {
	        to_free = ptr;
	        ptr = (free_block *) ptr->next;
	        mem_dealloc(to_free, sizeof(free_block));
	    }
	}
	if (heap->start_addr != NULL) mem_dealloc(heap->start_addr, heap->heap_size);

	pthread_mutex_destroy(&heap->lock);
	mem_dealloc(heap, sizeof(numa_heap));
    }

    mem_dealloc(numa_heaps, nodes * sizeof(numa_heap *));
}

void numa_free(void *ptr, int node) {
    assert((size_t) node < get_numa_nodes_num());
    assert(ptr != NULL);

    numa_heap *heap = numa_heaps[node];
    pthread_mutex_lock(&heap->lock);

    size_t bin_index = -1;
    for (size_t i = 0; i < BINS - 1; i++) {
	void *temp = free_lists_starting_addr[i + 1];
	if (!temp) continue;

	if (ptr <= temp) {	
	    bin_index = i + 1;
	    break;
	}
    }
    free_block *to_free = (free_block *) mem_alloc(sizeof(free_block));
    to_free->starting_addr = ptr;

    to_free->size = 16 * pow(2, bin_index);
    to_free->next = NULL;

    if (heap->free_list[bin_index] == NULL) {
        heap->free_list[bin_index] = to_free;
    } else {
	to_free->next = (struct free_block *) heap->free_list[bin_index];
	heap->free_list[bin_index] = to_free;
    }

    pthread_mutex_unlock(&heap->lock);
}


int main() {
    parse_cpus_to_node();
    init_allocator(1024 * 1024);
    /*

    for (int i = 0; i < get_numa_nodes_num(); i++) {
        print_heap(i); // Print the heap state to verify the blocks and bins
    }
    */
    //for (int i = 0; i < BINS; i++) printf("the starting addr for free list %d is %p\n", i, free_lists_starting_addr[i]);



//    print_heap(1);
    int *ptr = numa_alloc(sizeof(int) * 32 , 1);
    int *ptr2 = numa_alloc(sizeof(int) * 10, 1);
    int *ptr3 = numa_alloc(sizeof(int) * 512, 1);
    int *ptr4 = numa_alloc(sizeof(int) * 128, 1);

  //  print_heap(1);
/*
    int *ptr3 = numa_alloc(sizeof(int) * 40, 1);
    int *ptr4 = numa_alloc(sizeof(int) * 90, 1);
    int *ptr5 = numa_alloc(sizeof(int) *1000, 1);
    int *ptr6 = numa_alloc(sizeof(int) * 32, 1);
    for (int i = 0; i < 2; i++) ptr2[i] = 99;
    
    for (int i = 0; i < 10; i++) printf("ptr[%d] = %d\n", i, ptr2[i]);

*/
    //print_heap(1);
//	printf("allocated pointer with address %p\n", ptr);
	numa_free(ptr, 1);
	numa_free(ptr2, 1);
	numa_free(ptr3, 1);
	numa_free(ptr4, 1);
    print_heap(1);
    /*

    print_heap(1);

    print_heap(1);
    for (int cpu = 0; cpu < MAX_CPUS; cpu++) {
        if (cpu_on_node[cpu] != -1) {
            printf("CPU %d belongs to NUMA node %d\n", cpu, cpu_on_node[cpu]);
        }
    }
*/

    free_allocator();
    return 0;
}

