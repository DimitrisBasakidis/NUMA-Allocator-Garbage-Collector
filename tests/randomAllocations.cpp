#include <iostream>
#include <random>
#include <ctime>
#include "../garbage-collector/cppGarbageCollector.h"

struct Size0_16 : public Traceable {
    uint8_t x;
};

// Size ≈ 16 bytes
struct Size16_32 : public Traceable {
    uint64_t a;
    uint32_t b;
};

// Size ≈ 32 bytes
struct Size32_64 : public Traceable {
    uint64_t a[4];
};

// Size ≈ 64 bytes
struct Size64_128 : public Traceable {
    uint64_t a[8];
};

// Size ≈ 128 bytes
struct Size128_256 : public Traceable {
    uint64_t a[16];
};

// Size ≈ 256 bytes
struct Size256_512 : public Traceable {
    uint64_t a[32];
};

// Size ≈ 512 bytes
struct Size512_1024 : public Traceable {
    uint64_t a[64];
};

// Size ≈ 1024 bytes
struct Size1024_1536 : public Traceable {
    uint64_t a[128];
};

// Size ≈ 1536 bytes
struct Size1536_2048 : public Traceable {
    uint64_t a[192];
};

// Size ≈ 2048 bytes
struct Size2048 : public Traceable {
    uint64_t a[256];
};



int main() {
  gcInit(1024 * 1024 * 200);
  std::mt19937 rng(time(nullptr));  // Random number generator
  std::uniform_int_distribution<int> dist(0, 9);  // 10 struct types

  for (int i = 0; i < 10000; ++i) {
    for (int j = 0; j < 4; ++j) {
      int choice = dist(rng);

      Traceable* obj = nullptr;

      switch (choice) {
        case 0: obj = new Size0_16(); break;
        case 1: obj = new Size16_32(); break;
        case 2: obj = new Size32_64(); break;
        case 3: obj = new Size64_128(); break;
        case 4: obj = new Size128_256(); break;
        case 5: obj = new Size256_512(); break;
        case 6: obj = new Size512_1024(); break;
        case 7: obj = new Size1024_1536(); break;
        case 8: obj = new Size1536_2048(); break;
        case 9: obj = new Size2048(); break;
      }

      obj = nullptr;
    }
  }

  gc();
  gcFree();

  return 0;
}
