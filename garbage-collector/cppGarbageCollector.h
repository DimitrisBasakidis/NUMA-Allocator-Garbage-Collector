#ifndef NUMA_ALLOCATOR_CPP_H
#define NUMA_ALLOCATOR_CPP_H

#include <new>
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
extern size_t gc_threshold_bytes;
extern size_t current_allocated_bytes;

void gcInit(size_t heapSize);
void gcFree();
void gc();

struct Traceable {

  ObjectHeader *getHeader() { return traceInfo.at(this); }
  static void *operator new(size_t size) {
    void *object = allocate_localy(size);
    if (!object) {
      std::cerr << "[GC HANDLER] Allocation failed. Trying GC...\n";
      gc();
      object = allocate_localy(size);  // Try again after GC

      if (!object) {
        std::cerr << "NUMA Allocation failed after GC. Aborting.\n";
        return NULL;
        std::abort();
      }
    }

    if (!object) {
      std::cerr << "NUMA Allocation failed!\n";
      std::abort();  // Or fallback to malloc to isolate the problem
    }
 
    auto header = new ObjectHeader{.marked = false, .size = size};
    traceInfo.insert(std::make_pair((Traceable *)object, header));

    current_allocated_bytes += size;
    if (current_allocated_bytes > gc_threshold_bytes) {
      #ifdef DEBUG
        std::cout << "[GC HANDLER] invoking gc()" << std::endl;
      #endif
      current_allocated_bytes = 0;
    }

    return object;
  }

  static void *operator new[](size_t size) {
    void *object = allocate_localy(size);
    if (!object) {
        std::cerr << "[GC HANDLER] Array allocation failed. Trying GC...\n";
        gc();
        object = allocate_localy(size);  // Try again after GC

        if (!object) {
            std::cerr << "NUMA Array allocation failed after GC. Aborting.\n";
            std::abort();
        }
    }

    if (!object) {
        std::cerr << "NUMA Array allocation failed!\n";
        std::abort();
    }

    auto header = new ObjectHeader{.marked = false, .size = size};
    traceInfo.insert(std::make_pair((Traceable *)object, header));

    current_allocated_bytes += size;
    if (current_allocated_bytes > gc_threshold_bytes) {
#ifdef DEBUG
        std::cout << "[GC HANDLER] invoking gc() after array allocation" << std::endl;
#endif
        current_allocated_bytes = 0;
    }

    return object;
}
 //  static void *operator numa_new(size_t size) {
 //    void *object = allocate_interleaved(size);
 //    if (!object) 
 //      std::cerr << "NUMA Allocation failed!\n";
 //    }
 // 
 //    auto header = new ObjectHeader{.marked = false, .size = size};
 //    traceInfo.insert(std::make_pair((Traceable *)object, header));
 //
 //    return object;
 //  }
};

#endif
