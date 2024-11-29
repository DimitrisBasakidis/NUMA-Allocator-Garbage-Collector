# Variables
CC = gcc
CFLAGS = -Wall -Wextra -O2

# Targets
all: numa_alloc

numa.o: numa.c
	$(CC) $(CFLAGS) -c numa.c

numa_alloc: allocator.c numa.o
	$(CC) $(CFLAGS) allocator.c -v numa.o -o numa_alloc -pthread

clean:
	rm -f *.o numa_alloc


