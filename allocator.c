#include <assert.h>
#include <sched.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <pthread.h>
#include <unistd.h>

#include "allocator.h"
#include "numa.h"

numa_heap **numa_heaps;

void *mem_alloc(size_t size) {
    void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (ptr == MAP_FAILED) {
    	fprintf(stderr, "mmap failed\n");
	return NULL;
    }
	
    return ptr;
}

void restore_thread_affinity(void) {
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set); // Start with an empty CPU set

    // Add all available CPUs to the set
    for (size_t cpu = 0U; cpu < sysconf(_SC_NPROCESSORS_CONF); cpu++) {
        CPU_SET(cpu, &cpu_set);
    }

    // Apply the CPU set to allow execution on all CPUs
    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set) < 0) {
        fprintf(stderr, "Failed to restore thread affinity\n");
	return;
    }
}

void set_thread_affinity(int node) {
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);  // create empty cpu set

    //  add CPUs that belond to the specified NUMA node to the cpu set
    for (size_t cpu = 0U; cpu < sysconf(_SC_NPROCESSORS_CONF); cpu++) {
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
    if (node >= get_numa_nodes_num()) {
        fprintf(stderr, "Invalid NUMA node: %d\n", node);
        return;
    }

    pthread_mutex_lock(&numa_heaps[node]->lock); // Ensure thread-safe access

    numa_heap *heap = numa_heaps[node];
    printf("Heap for NUMA Node %d:\n", node);
    printf("  Start Address: %p\n", heap->start_addr);
    printf("  Total Size: %zu bytes\n", heap->heap_size);
    printf("  Free List:\n");

    free_block *current = heap->free_list;
    while (current) {
        printf("    Block Address: %p, Size: %zu bytes\n", (void *)current->starting_addr, current->size);
        current = current->next;
    }

    pthread_mutex_unlock(&heap->lock);
}


void init_allocator(size_t heap_size) {
    assert(heap_size > 0);
    size_t nodes =  get_numa_nodes_num();
    size_t size = nodes * sizeof(struct numa_heap *);

    numa_heaps = (numa_heap **) mem_alloc(size);

    for (size_t i = 0U; i < nodes; i++) {
	numa_heaps[i] = (numa_heap *) mem_alloc(sizeof(numa_heap));

    	//printf("before setting affinity running on %d\n", sched_getcpu());
	set_thread_affinity(i);

	numa_heaps[i]->start_addr = mem_alloc(heap_size);
	touch_memory(numa_heaps[i]->start_addr, heap_size);
	//print_allocation_info(numa_heaps[i]->start_addr, heap_size);

	numa_heaps[i]->heap_size = heap_size;
	numa_heaps[i]->numa_node = i;
	//numa_heaps[i]->free_list = (free_block *) numa_heaps[i]->start_addr;

/*	
	numa_heaps[i]->free_list = mem_alloc(sizeof(free_block));
	numa_heaps[i]->free_list->starting_addr = numa_heaps[i]->start_addr;
	numa_heaps[i]->free_list->size = heap_size;
	numa_heaps[i]->free_list->next = NULL;
	*/
	numa_heaps[i]->free_list = NULL;

	if (pthread_mutex_init(&numa_heaps[i]->lock, NULL) != 0) {
            fprintf(stderr, "Failed to initialize mutex for NUMA heap %zu\n", i);
            return;
        }

        printf("Initialized NUMA heap for node %zu in address %p with size %zu bytes in cpu %d\n", i, numa_heaps[i]->start_addr, heap_size, sched_getcpu());
    }

    restore_thread_affinity();
    //printf("after restoring affinity running on %d\n", sched_getcpu());
}

void *numa_alloc(size_t size, int node) {
    assert(node < get_numa_nodes_num());
    numa_heap *heap = numa_heaps[node];

    pthread_mutex_lock(&heap->lock);

    if (heap->heap_size < size) {
    	fprintf(stderr, "not enough memory in NUMA node %d heap\n", node); 
	pthread_mutex_unlock(&heap->lock);
	return NULL;
    }
    
    void *allocated = heap->start_addr;
    free_block *new_block = (free_block *)mem_alloc(sizeof(free_block));
    new_block->starting_addr = heap->start_addr;
    new_block->size = size;
    new_block->next = NULL;

    if (heap->free_list == NULL) heap->free_list = new_block;
    else {
         free_block *ptr = heap->free_list;
	 while (ptr->next != NULL) ptr = ptr->next;
	 ptr->next = new_block;
    }

    heap->start_addr = (char *)heap->start_addr + size;
    heap->heap_size -= size;


    pthread_mutex_unlock(&heap->lock);
    return allocated;
}

int main() {
    parse_cpus_to_node();
    init_allocator(1024 * 1024);
    int *ptr = numa_alloc(sizeof(int) * 10, 1);
    int *ptr2 = numa_alloc(sizeof(int) * 10, 1);
    int *ptr3 = numa_alloc(sizeof(int) * 10, 1);
    int *ptr4 = numa_alloc(sizeof(int) * 10, 1);
    int *ptr5 = numa_alloc(sizeof(int) * 10, 1);
    int *ptr6 = numa_alloc(sizeof(int) * 10, 1);

    for (int i = 0; i < 10; i++) ptr[i] = i;
    
    for (int i = 0; i < 10; i++) printf("ptr[%d] = %d\n", i, ptr[i]);

 //   print_allocation_info(ptr, 10);
    print_heap(1);
   // print_heap(0);
/*
    for (int cpu = 0; cpu < MAX_CPUS; cpu++) {
        if (cpu_on_node[cpu] != -1) {
            printf("CPU %d belongs to NUMA node %d\n", cpu, cpu_on_node[cpu]);
        }
    }
*/


    return 0;
}

