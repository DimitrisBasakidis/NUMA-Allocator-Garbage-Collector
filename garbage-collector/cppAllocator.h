#ifndef NUMA_ALLOCATOR_CPP_H
#define NUMA_ALLOCATOR_CPP_H

#include <new>  // For C++ memory allocation
#include <cstdlib>
#include <cstdint>
#include <unordered_map>
#include <iostream>

#define __READ_RBP() __asm__ volatile("movq %%rbp, %0" : "=r"(__rbp))
#define __READ_RSP() __asm__ volatile("movq %%rsp, %0" : "=r"(__rsp))

extern "C" {
  void init_allocator(size_t size);
  void* allocate_localy(size_t size);
  void* allocate_interleaved(size_t size);
  void deallocate(void* ptr);
  void free_allocator();
}

typedef struct ObjectHeader {
  bool marked;
  size_t size;
} ObjectHeader;

struct Traceable;
extern std::unordered_map<Traceable *, ObjectHeader *> traceInfo;

struct Traceable {
    ObjectHeader *getHeader() { return traceInfo.at(this); }
    static void *operator new(size_t size) {
    void *object = allocate_localy(size);

    if (!object) {
      std::cerr << "NUMA Allocation failed!\n";
    }
 
    auto header = new ObjectHeader{.marked = false, .size = size};
    traceInfo.insert(std::make_pair((Traceable *)object, header));

    return object;
  }
};

void gcInit();
void gc();

#endif
