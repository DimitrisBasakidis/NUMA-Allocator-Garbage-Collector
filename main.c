#include "allocator.h"
#include "numa.h"
#include <stdio.h>


int main() {
    parse_cpus_to_node();
    init_allocator(1024 * 20);

    //for (int i = 0; i < BINS; i++) printf("the starting addr for free list %d is %p\n", i, free_lists_starting_addr[i]);



    int *ptr = allocate(sizeof(int) * 32 , 0);
  //  int *ptr2 = numa_alloc(sizeof(int) * 10, 1);
   // int *ptr3 = numa_alloc(sizeof(int) * 2, 1);
    int *ptr4 = allocate(sizeof(int) * 512, 1);
//	numa_free(ptr, 1);
//	numa_free(ptr2, 1);
//	numa_free(ptr3, 1);
	deallocate(ptr4);
	deallocate(ptr);
  //  print_heap(1);




    free_allocator();
    return 0;
}
