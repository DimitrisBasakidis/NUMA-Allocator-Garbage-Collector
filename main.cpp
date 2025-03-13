#include <iostream>
 
#include "garbage-collector/cpp_allocator.h"

struct MyStruct {
    int data[1000];
};

int main() {

    init_alloc(1024 * 1024 * 24); // 24 MB allocator initialization
    // Allocate with global new (uses NUMA local allocation)
    MyStruct* obj = new MyStruct;
    
    // Allocate an array
    // MyStruct* arr = new MyStruct[10];

    std::cout << "NUMA allocated object at: " << obj << std::endl;
    // std::cout << "NUMA allocated array at: " << arr << std::endl;
    //
    // // Free memory
    delete obj;
    // delete[] arr;
    free_allocator();
    return 0;
}

