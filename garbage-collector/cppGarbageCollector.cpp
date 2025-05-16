#include "cppGarbageCollector.h"
#include <csetjmp>
#include <vector>

std::unordered_map<Traceable *, ObjectHeader *> traceInfo;
size_t gc_threshold_bytes;
size_t current_allocated_bytes = 0;

intptr_t *__rbp;
intptr_t *__rsp;
intptr_t *__stackBegin;

void gcInit(size_t heapSize) {
  gc_threshold_bytes = heapSize * 0.75;
  #ifdef DEBUG
    std::cout << "[GC INIT] gc_threshold_bytes is " << gc_threshold_bytes << std::endl;
  #endif
  init_allocator(heapSize);
  __READ_RBP();
  __stackBegin = (intptr_t *)*__rbp;
}

void gcFree() {
  gc();
  free_allocator();
}

std::vector<Traceable *> getPointers(Traceable *object) {

  auto p = (uint8_t *)object;
  auto end = (p + object->getHeader()->size);
  std::vector<Traceable *> result;

  #ifdef DEBUG
  std::cout << "[GC PTRS] Scanning Object at " << object << " (Size: " << object->getHeader()->size << std::endl;
  #endif

  while (p < end) {
    auto address = (Traceable *)*(uintptr_t *)p;
    if (traceInfo.count(address) != 0) {
      #ifdef DEBUG
        std::cout << "[GC PTRS] Found Pointer: " << address << " inside Object at " << object << "\n";
      #endif

      result.emplace_back(address);
    }
    p++;
  }
  return result;
}

void scanRegistersForRoots(std::vector<Traceable *> &roots) {
    void *rax, *rbx, *rcx, *rdx, *rsi, *rdi;
    
    __asm__ volatile (
        "movq %%rax, %0\n"
        "movq %%rbx, %1\n"
        "movq %%rcx, %2\n"
        "movq %%rdx, %3\n"
        "movq %%rsi, %4\n"
        "movq %%rdi, %5\n"
        : "=r"(rax), "=r"(rbx), "=r"(rcx), "=r"(rdx), "=r"(rsi), "=r"(rdi)  // Outputs
    );

    std::vector<void*> regs = {rax, rbx, rcx, rdx, rsi, rdi};

    for (void* ptr : regs) {
        auto address = (Traceable *)ptr;
        if (traceInfo.count(address) != 0) {
          #ifdef DEBUG
            std::cout << "[GC ROOTS] Found Root in Register: " << address << "\n";
          #endif
          roots.emplace_back(address);
        }
    }
}


std::vector<Traceable*> getRegisterRoots() {
    void* regs[6] = {};
    asm volatile (
        "movq %%rax, %0\n\t"
        "movq %%rbx, %1\n\t"
        "movq %%rcx, %2\n\t"
        "movq %%rdx, %3\n\t"
        "movq %%rsi, %4\n\t"
        "movq %%rdi, %5\n\t"
        : "=r"(regs[0]), "=r"(regs[1]), "=r"(regs[2]),
          "=r"(regs[3]), "=r"(regs[4]), "=r"(regs[5])
        :
        : "memory"
    );

    std::vector<Traceable*> result;
    for (void* reg : regs) {
        auto ptr = reinterpret_cast<Traceable*>(reg);
        if (traceInfo.count(ptr)) {
          #ifdef DEBUG
            std::cout << "[GC ROOTS] Found Register Root: " << ptr << std::endl;
          #endif
          result.push_back(ptr);
        }
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

  #ifdef DEBUG
    std::cout << "[GC ROOTS] Scanning stack from " << static_cast<void *>(rsp) << " to " << static_cast<void *>(top) << std::endl;
  #endif

  while (rsp < top) {
    auto address = (Traceable *)*(uintptr_t *)rsp;
    if (traceInfo.count(address) != 0) {
      #ifdef DEBUG
        std::cout << "[GC ROOTS] Found Root: " << address << "\n";
      #endif
      result.emplace_back(address);
    }
    rsp++;
  }

  auto regRoots = getRegisterRoots();
  result.insert(result.end(), regRoots.begin(), regRoots.end());

  #ifdef DEBUG
    std::cout << "[GC ROOTS] Total Roots Found: " << result.size() << "\n";
  #endif

  return result;
}

void mark() {
  auto worklist = getRoots();

  #ifdef DEBUG
    std::cout << "[GC MARK] Found " << worklist.size() << " root objects.\n";
  #endif

  while (!worklist.empty()) {
    auto o = worklist.back();
    worklist.pop_back();
    auto header = o->getHeader();

    if (!header) {
      #ifdef DEBUG
        std::cerr << "[GC ERROR] Object with no header found! Skipping...\n";
      #endif

      continue;
    }

    #ifdef DEBUG
      std::cout << "[GC MARK] Checking Object at " << o << " | Marked: " << header->marked << "\n";
    #endif

    if (!header->marked) {
      header->marked = true;
      #ifdef DEBUG
        std::cout << "[GC MARK] Marked Object at " << o << "\n";
      #endif

      auto references = getPointers(o);

      #ifdef DEBUG
        std::cout << "[GC MARK] Found " << references.size() << " references from Object at " << o << "\n";
      #endif

      for (const auto &p : references) worklist.push_back(p);
    }
  }
}

void sweep() {

  #ifdef DEBUG
    std::cout << "[GC SWEEP] Starting garbage collection sweep...\n";
  #endif

  size_t live_objects = 0, collected_objects = 0;
  auto it = traceInfo.cbegin();

  while (it != traceInfo.cend()) {
    Traceable *ptr = it->first;
    ObjectHeader *header = it->second;

    if (it->second->marked) {
      it->second->marked = false;
      live_objects++;
      #ifdef DEBUG
        std::cout << "[GC SWEEP] Object at " << ptr << " is still reachable.\n";
      #endif
      ++it;
    } else {
      #ifdef DEBUG
        std::cout << "[GC SWEEP] Collecting Object at " << ptr << "\n";
      #endif
      Traceable *ptr = it->first;
      ObjectHeader *ptr2 = it->second;
      delete ptr2;
      it = traceInfo.erase(it);
      deallocate(ptr);
      collected_objects++;
    }
  }
  #ifdef DEBUG
    std::cout << "[GC SWEEP] Completed. Live Objects: " << live_objects << ", Collected Objects: " << collected_objects << "\n";
  #endif
}

void gc() {
  mark();
  sweep();
}
