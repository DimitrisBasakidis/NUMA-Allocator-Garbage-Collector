#include <iostream>
#include <vector>
#include <random>
#include <ctime>
#include "../garbage-collector/cppGarbageCollector.h"

struct DummyObject : public Traceable {
  int id;
  DummyObject(int id) : id(id) {}
};

int main() {
  gcInit(100 * 1024 * 1024); // 100MB GC pool

  std::vector<Traceable*> objects;
  std::mt19937 rng(time(nullptr));
  std::uniform_int_distribution<int> dist(1, 1000000);

  // Allocate many dummy objects and store in vector
  for (int i = 0; i < 10000; ++i) {
    int id = dist(rng);
    objects.push_back(new DummyObject(id));
  }

  std::cout << "Allocated 10,000 objects into vector.\n";

  // Now drop references by clearing the vector
  objects.clear();
  std::cout << "Cleared vector.\n";

  // Trigger garbage collection
  gc();
  gcFree();

  return 0;
}

