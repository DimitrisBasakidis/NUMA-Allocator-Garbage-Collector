CC = gcc
CFLAGS = -Wall -Wextra -O2
DEFINES = -D_GNU_SOURCE

all: numa_alloc cpp_alloc

util.o: 
	$(CC) $(CFLAGS) $(DEFINES) -c allocator/util.c

numa.o: allocator/numa.c
	$(CC) $(CFLAGS) -c allocator/numa.c

allocator.o: numa.o
	$(CC) $(CFLAGS) $(DEFINES) -c allocator/allocator.c 

numa_alloc: allocator.o numa.o util.o
	$(CC) $(DEFINES) $(CFLAGS) allocator/main.c allocator.o numa.o util.o -o numa_alloc -pthread -lm
	g++ garbage-collector/cpp_allocator.cpp -c

cpp_alloc: numa_alloc
	g++ main.cpp numa.o util.o allocator.o cpp_allocator.o -o cpp_alloc


clean:
	rm -f *.o numa_alloc
