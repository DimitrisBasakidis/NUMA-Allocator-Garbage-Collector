#include <iostream>
 
#include "garbage-collector/cppAllocator.h"

extern std::unordered_map<Traceable *, ObjectHeader *> traceInfo;

struct MyStruct : public Traceable {
    int data[10];
};

struct MyObject : public Traceable {
    int value;
    MyObject *next;
};


int main() {
    gcInit(1024 * 1024);

    for (int i = 0; i < 3; i++) {
      MyObject* obj2 = new MyObject();  // Not a GC-tracked objet
      obj2 = NULL;
      MyStruct* obj3 = new MyStruct();  // Not a GC-tracked object
      obj3 = NULL;
    }
  
    gc();
    gcFree();
    return 0;
}
