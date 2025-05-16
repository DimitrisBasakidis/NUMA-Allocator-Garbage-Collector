CC = gcc
CFLAGS = -Wall -Wextra -O2
DEFINES = -D_GNU_SOURCE

all: numa_alloc cppAlloc
debug: debugCppAlloc

util.o: 
	$(CC) $(CFLAGS) $(DEFINES) -c ../allocator/util.c

numa.o: ../allocator/numa.c
	$(CC) $(CFLAGS) -c ../allocator/numa.c

allocator.o: numa.o
	$(CC) $(CFLAGS) $(DEFINES) -c ../allocator/allocator.c 

numa_alloc: allocator.o numa.o util.o
	$(CC) $(DEFINES) $(CFLAGS) ../allocator/main.c allocator.o numa.o util.o -o numa_alloc -pthread -lm

cppAlloc: numa_alloc
	g++ ../garbage-collector/cppGarbageCollector.cpp -c
	# g++ main.cpp numa.o util.o allocator.o cppGarbageCollector.o -o cppAlloc

debugCppAlloc: numa_alloc
	g++ -DDEBUG ../garbage-collector/cppGarbageCollector.cpp -c
	# g++ -DDEBUG main.cpp numa.o util.o allocator.o cppGarbageCollector.o -o debugCppAlloc

clean:
	rm -f *.o numa_alloc
	rm -f *.o cppAlloc
	rm -f *.o debugCppAlloc
