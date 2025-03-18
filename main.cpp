#include <iostream>
 
#include "garbage-collector/cppAllocator.h"

// struct MyStruct {
//     int data[10];
// };

struct MyObject : public Traceable {
    int value;
    MyObject *next;
};

int main() {
  gcInit();
    init_allocator(1024 * 1024);  // Initialize allocator

    MyObject *obj1 = new MyObject();
    MyObject *obj2 = new MyObject();
    MyObject *obj3 = new MyObject();
    MyObject *obj4 = new MyObject();

    obj1->next = obj2;
    obj2->next = obj3;
    obj3->next = nullptr;

    std::cout << "[TEST] Created 3 objects: " << obj1 << ", " << obj2 
              << ", " << obj3 << "\n";

    gc();  // Run garbage collection

    return 0;
}
