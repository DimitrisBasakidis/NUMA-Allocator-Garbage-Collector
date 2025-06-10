#include <stdio.h>
#include <stdint.h>

#include <stdlib.h>
#include <string.h>
#include <time.h>

// Uncomment to use NUMA allocator
//#define NUMA_ALLOC

#ifdef NUMA_ALLOC
#include "../allocator/allocator.h"
#ifdef LOCAL
  #define ALLOCATE(size) allocate_localy(size)
#else
  #define ALLOCATE(size) allocate_interleaved(size)
#endif
#define DEALLOCATE(ptr) deallocate(ptr)
#else
#define ALLOCATE(size) malloc(size)
#define DEALLOCATE(ptr) free(ptr)
#endif

#define NUM_ALLOCATIONS 10000
const int ALLOC_SIZES[] = {16, 32, 64, 128, 256, 512, 1024, 2048};
#define NUM_SIZES (sizeof(ALLOC_SIZES) / sizeof(ALLOC_SIZES[0]))

typedef struct {
    char* ptr;
    int size;
    int valid;
    unsigned char pattern;
} Block;

void print_block_contents(int index, Block *block) {
    if (!block->valid || !block->ptr) {
        printf("[BLOCK %2d] <freed>\n", index);
        return;
    }

    printf("[BLOCK %2d] ptr=%p, size=%3d, pattern=0x%02X: ", 
           index, block->ptr, block->size, block->pattern);

    for (int i = 0; i < block->size; ++i) {
      if (i > 15) break;
        printf("%02X ", (unsigned char)block->ptr[i]);
        }
    printf("\n");
}


int main() {
    Block blocks[NUM_ALLOCATIONS];
    int i, j;

    srand(4);

#ifdef NUMA_ALLOC
    init_allocator(1024 * 1024 * 200); 
#endif

    clock_t start = clock();

    // Phase 1: Allocation + Write
    for (i = 0; i < NUM_ALLOCATIONS; ++i) {
        int size = ALLOC_SIZES[rand() % NUM_SIZES];
        unsigned char pattern = i % 256;

        char* p = (char*)ALLOCATE(size);
        if (!p) {
            fprintf(stderr, "Allocation failed at %d\n", i);
            return 1;
        }

        // printf("[DEBUG] Allocating block %d at %p, size=%d, pattern=0x%02X\n", i, (void*)p, size, pattern);


        memset(p, pattern, size);

        blocks[i].ptr = p;
        blocks[i].size = size;
        blocks[i].pattern = pattern;
        blocks[i].valid = 1;

        printf("[ALLOC] Block %d: size=%d bytes, filled with 0x%02X\n", i, size, pattern);
    }


    // Phase 2: Random deallocation of ~50% of blocks
    for (i = 0; i < NUM_ALLOCATIONS; ++i) {
        if (rand() % 2 == 0) {
            DEALLOCATE(blocks[i].ptr);
            blocks[i].ptr = NULL;
            blocks[i].valid = 0;
            printf("[FREE ] Block %d: freed\n", i);
        }
    }

    // Phase 3: Reallocate into freed slots
    for (i = 0; i < NUM_ALLOCATIONS; ++i) {
       if (!blocks[i].valid) {

        // printf("[DEBUG] invald block %d\n", i);
            int size = ALLOC_SIZES[rand() % NUM_SIZES];
            unsigned char pattern = (i + 123) % 256;

            char* p = (char*)ALLOCATE(size);
            if (!p) {
                fprintf(stderr, "Re-allocation failed at %d\n", i);
                return 2;
            }


        // printf("[DEBUG] Allocating block %d at %p, size=%d, pattern=0x%02X\n", i, (void*)p, size, pattern);

            blocks[i].ptr = p;
            blocks[i].size = size;
            blocks[i].pattern = pattern;
            blocks[i].valid = 1;

            memset(p, pattern, size);

            printf("[REALC] Block %d: size=%d bytes, filled with 0x%02X\n", i, size, pattern);
        }
    }
    // Phase 4: Verify memory contents
    int errors = 0;
    for (i = 0; i < NUM_ALLOCATIONS; ++i) {
        if (!blocks[i].valid || !blocks[i].ptr) continue;
        for (j = 0; j < blocks[i].size; ++j) {
            if ((unsigned char)blocks[i].ptr[j] != blocks[i].pattern) {
                fprintf(stderr, "[CORRUPT] Block %d, size=%d byte %d: expected 0x%02X, got 0x%02X\n",
                        i,blocks[i].size, j, blocks[i].pattern, blocks[i].ptr[j]);
                errors++;
                break;
            }
        }
    }

    if (errors == 0)
        printf("[VERIFY] All blocks passed verification.\n");
    else
        printf("[VERIFY] Total corrupted blocks: %d\n", errors);

    // Phase 5: Free everything
    for (i = 0; i < NUM_ALLOCATIONS; ++i) {
        if (blocks[i].valid && blocks[i].ptr) {
            DEALLOCATE(blocks[i].ptr);
        }
    }

    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC * 1000;
#ifdef NUMA_ALLOC 
#ifdef LOCAL 
    printf("[LOCAL] ");
#else
    printf("[INTERLEAVED] ");
#endif
#else 
    printf("[MALLOC] ");
#endif
    printf("Test completed in %.2f ms\n", elapsed);

#ifdef NUMA_ALLOC
    free_allocator();
#endif

    return 0;
}

