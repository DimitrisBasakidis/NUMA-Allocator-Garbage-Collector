# Variables
CC = gcc
CFLAGS = -Wall -Wextra -O2
DEFINES = -D_GNU_SOURCE

# Targets
all: numa_alloc

numa.o: numa.c
	$(CC) $(CFLAGS) -c numa.c

numa_alloc: allocator.c numa.o
	$(CC) $(DEFINES) $(CFLAGS) allocator.c numa.o -o numa_alloc -pthread -lm

clean:
	rm -f *.o numa_alloc


