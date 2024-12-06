#include "allocator.h"
#include "numa.h"
#include <stdio.h>


int main() {
    parse_cpus_to_node();
    init_allocator(1024 * 40);

    //for (int i = 0; i < BINS; i++) printf("the starting addr for free list %d is %p\n", i, free_lists_starting_addr[i]);


    print_heap(1);
    int *ptr = allocate_interleaved(sizeof(int) * 32);

    int *ptr2 = allocate_interleaved(sizeof(int) * 2);
    int *ptr3 = allocate_interleaved(sizeof(int) *400);
  //  int *ptr2 = numa_alloc(sizeof(int) * 10, 1);
    int *ptr4 = allocate_interleaved(sizeof(int) * 512);
    int *ptr5 = allocate_interleaved(sizeof(int) * 1512);
//	numa_free(ptr, 1);
//	numa_free(ptr2, 1);
//	numa_free(ptr3, 1);
	deallocate(ptr);
	deallocate(ptr2);
	deallocate(ptr3);
	deallocate(ptr4);
	deallocate(ptr5);
  //  print_heap(1);




    free_allocator();
    return 0;
}
