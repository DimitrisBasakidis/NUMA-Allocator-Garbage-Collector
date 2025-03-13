#include "cpp_allocator.h"

// C++ global new using NUMA local allocation
void* operator new(size_t size) noexcept(false) {
    void* ptr = allocate_localy(size);
    if (!ptr) throw std::bad_alloc();
    return ptr;
}

void init_alloc(size_t size) { 
  init_allocator(size);
}

void* operator new[](size_t size) noexcept(false) {
    void* ptr = allocate_localy(size);
    if (!ptr) throw std::bad_alloc();
    return ptr;
}

// C++ global delete
void operator delete(void* ptr) noexcept {
    if (ptr) deallocate(ptr);
}

void operator delete[](void* ptr) noexcept {
    if (ptr) deallocate(ptr);
}

// Optional: C++ interleaved allocation
void* numa_new_interleaved(size_t size) {
    return allocate_interleaved(size);
}

