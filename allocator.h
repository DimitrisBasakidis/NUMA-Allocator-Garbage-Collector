#ifndef NUMA_ALLOCATOR
#define NUMA_ALLOCATOR

typedef struct {
    void *starting_addr;
    size_t size;
    struct free_block *next;
} free_block;

typedef struct {
    void *start_addr;
    size_t heap_size;
    unsigned numa_node;
    free_block *free_list;
    pthread_mutex_t lock;
} numa_heap;


void init_allocator(size_t heap_size);
void free_allocator(void);

void *allocate(size_t size);
void *deallocate(void *ptr);

#endif

