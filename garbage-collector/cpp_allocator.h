#ifndef NUMA_ALLOCATOR_CPP_H
#define NUMA_ALLOCATOR_CPP_H

#include <new>  // For C++ memory allocation
#include <cstdlib>

extern "C" {
  void init_allocator(size_t size);
    void* allocate_localy(size_t size);
    void* allocate_interleaved(size_t size);
    void deallocate(void* ptr);
    void free_allocator() ;
}

// Custom new using NUMA local allocation
void init_alloc(size_t size);
void* operator new(size_t size) noexcept(false);
void* operator new[](size_t size) noexcept(false);
void operator delete(void* ptr) noexcept;
void operator delete[](void* ptr) noexcept;

// Alternative interleaved allocation function
void* numa_new_interleaved(size_t size);

#endif  // NUMA_ALLOCATOR_CPP_H
