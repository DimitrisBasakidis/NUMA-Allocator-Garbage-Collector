#include "cppAllocator.h"
#include <csetjmp>
#include <vector>

std::unordered_map<Traceable *, ObjectHeader *> traceInfo;

intptr_t *__rbp;
intptr_t *__rsp;
intptr_t *__stackBegin;

void gcInit() {
  __READ_RBP();
  __stackBegin = (intptr_t *)*__rbp;
}

std::vector<Traceable *> getPointers(Traceable *object) {

  auto p = (uint8_t *)object;
  auto end = (p + object->getHeader()->size);
  std::vector<Traceable *> result;
  std::cout << "[GC PTRS] Scanning Object at " << object 
              << " (Size: " << object->getHeader()->size << ")\n";

  while (p < end) {
    auto address = (Traceable *)*(uintptr_t *)p;
    if (traceInfo.count(address) != 0) {
      std::cout << "[GC PTRS] Found Pointer: " << address 
                      << " inside Object at " << object << "\n";
      result.emplace_back(address);
    }
    p++;
  }
  return result;
}

std::vector<Traceable *> getRoots() {
  std::vector<Traceable *> result;

  jmp_buf jb;
  setjmp(jb);

  __READ_RSP();
  auto rsp = (uint8_t *)__rsp;
  auto top = (uint8_t *)__stackBegin;

  std::cout << "[GC ROOTS] Scanning stack from " 
              << static_cast<void *>(rsp) << " to " 
              << static_cast<void *>(top) << "\n";


  while (rsp < top) {
    auto address = (Traceable *)*(uintptr_t *)rsp;
    if (traceInfo.count(address) != 0) {
      std::cout << "[GC ROOTS] Found Root: " << address << "\n";
      result.emplace_back(address);
    }
    rsp++;
  }
  std::cout << "[GC ROOTS] Total Roots Found: " << result.size() << "\n";
  return result;
}

void mark() {
  auto worklist = getRoots();
  std::cout << "[GC MARK] Found " << worklist.size() << " root objects.\n";

  while (!worklist.empty()) {
    auto o = worklist.back();
    worklist.pop_back();
    auto header = o->getHeader();

     if (!header) {
      std::cerr << "[GC ERROR] Object with no header found! Skipping...\n";
      continue;
    }
    std::cout << "[GC MARK] Checking Object at " << o << " | Marked: " << header->marked << "\n";

    if (!header->marked) {
      header->marked = true;
      std::cout << "[GC MARK] Marked Object at " << o << "\n";

      auto references = getPointers(o);
      std::cout << "[GC MARK] Found " << references.size() << " references from Object at " << o << "\n";

      for (const auto &p : references) worklist.push_back(p);
    }
  }
}

void sweep() {
    std::cout << "[GC SWEEP] Starting garbage collection sweep...\n";
    size_t live_objects = 0, collected_objects = 0;
  auto it = traceInfo.cbegin();
  while (it != traceInfo.cend()) {
        Traceable *ptr = it->first;
        ObjectHeader *header = it->second;

    if (it->second->marked) {
      it->second->marked = false;
       live_objects++;
      std::cout << "[GC SWEEP] Object at " << ptr << " is still reachable.\n";
      ++it;
    } else {
      std::cout << "[GC SWEEP] Collecting Object at " << ptr << "\n";
      Traceable *ptr = it->first;  // Store before eiirasing
      it = traceInfo.erase(it);    // Remove from GC
      deallocate(ptr);  // Use NUMA-aware deallocation}
    }
  }

   std::cout << "[GC SWEEP] Completed. Live Objects: " << live_objects
              << ", Collected Objects: " << collected_objects << "\n";
}

void gc() {
  mark();
  sweep();
}
