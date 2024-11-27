#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

#include "numa.h"

unsigned cpu_on_node[MAX_CPUS];

size_t get_numa_nodes_num(void) {
    struct dirent **name_list;
    size_t num_nodes = 0;

    int n = scandir("/sys/devices/system/node/", &name_list, NULL, alphasort);

    if (n < 0) return -1;

    for (int i = 0; i < n; i++) {
        if (strncmp(name_list[i]->d_name, "node", 4) == 0) num_nodes++;
        free(name_list[i]);
    }
    free(name_list);

    return num_nodes;
}

void parse_cpu_list(const char *cpulist, int node, int *cpu_on_node) {
    char *token;
    char buffer[256];
    strncpy(buffer, cpulist, sizeof(buffer) - 1);

    token = strtok(buffer, ",");
    while (token) {
        int start, end;

        // Check for range (e.g., "0-3")
        if (sscanf(token, "%d-%d", &start, &end) == 2) {
            for (int cpu = start; cpu <= end; cpu++) {
                cpu_on_node[cpu] = node;
            }
        }
        // Check for single CPU (e.g., "0")
        else if (sscanf(token, "%d", &start) == 1) {
            cpu_on_node[start] = node;
        }

        token = strtok(NULL, ",");
    }
}

void parse_cpus_to_node(void) {
    size_t node = 0U;
    memset(cpu_on_node, -1, sizeof(cpu_on_node));

    for (;;) { 
        char path[128];
	snprintf(path, sizeof(path), "/sys/devices/system/node/node%ld/cpulist", node);
        FILE *file = fopen(path, "r");

	if (!file) break;

	char buffer[256];
	if (fgets(buffer, sizeof(buffer), file)) {
	    parse_cpu_list(buffer, node, cpu_on_node);
	}
	node++;
    }
}
/*

int main() {
    parse_cpus_to_node();

    for (int cpu = 0; cpu < MAX_CPUS; cpu++) {
        if (cpu_on_node[cpu] != -1) {
            printf("CPU %d belongs to NUMA node %d\n", cpu, cpu_on_node[cpu]);
        }
    }

    return 0;
}*/
