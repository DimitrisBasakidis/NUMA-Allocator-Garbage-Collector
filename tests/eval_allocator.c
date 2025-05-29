#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

int main() {
    char* ptrs[NUM_ALLOCATIONS];
    int sizes[NUM_ALLOCATIONS];
    int i, j;

#ifdef NUMA_ALLOC
    init_allocator(1024 * 1024 * 50);
#endif

    srand(8);
    clock_t start = clock();

    // Allocation + Write
    for (i = 0; i < NUM_ALLOCATIONS; ++i) {
        sizes[i] = ALLOC_SIZES[rand() % NUM_SIZES];
        ptrs[i] = ALLOCATE(sizes[i]);
        if (!ptrs[i]) {
            fprintf(stderr, "[ERROR] Allocation failed at %d\n", i);
            return 1;
        }
        memset(ptrs[i], i, sizes[i]);
        printf("[ALLOC] Block %d: size=%d bytes, filled with 0x%02X\n", i, sizes[i], i);
    }

    // Read + Verify
    for (i = 0; i < NUM_ALLOCATIONS; ++i) {
        for (j = 0; j < sizes[i]; ++j) {
            char expected = (char)(i);
            if (ptrs[i][j] != expected) {
                fprintf(stderr, "\n[CORRUPTION DETECTED] Allocation %d, byte %d:\n", i, j);
                fprintf(stderr, "  Expected: 0x%02X, Found: 0x%02X\n", expected, (unsigned char)ptrs[i][j]);
                fprintf(stderr, "  Block size: %d bytes\n", sizes[i]);
                return 2;
            }
        }
        // if (i % 20000 == 0) {
            // printf("[VERIFY] Block %d verified successfully.\n", i);
        // }
    }

    // Free
    for (i = 0; i < NUM_ALLOCATIONS; ++i) {
        DEALLOCATE(ptrs[i]);
        // if (i % 20000 == 0) {
            // printf("[FREE] Block %d deallocated.\n", i);
        // }
    }

    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC * 1000;
    printf("[DONE] Test completed in %.2f ms\n", elapsed);

#ifdef NUMA_ALLOC
    free_allocator();  // If needed by your allocator
#endif

    return 0;
}

