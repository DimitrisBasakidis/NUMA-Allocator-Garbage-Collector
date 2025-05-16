#include <iostream>
#include "../garbage-collector/cppGarbageCollector.h"

struct MyStruct : public Traceable {
    int data[10];
};

struct MyObject : public Traceable {
    int value;
    MyObject *next;
};


int main() {
  gcInit(1024 * 1024 * 10);

  MyObject* obj1;
  MyStruct* obj2;
  for (int i = 0; i < 10000; i++) {
    obj1 = new MyObject();
    obj2 = new MyStruct();
  }

  obj1 = nullptr;
  obj2 = nullptr;

  gc();
  gcFree();
  return 0;
}
