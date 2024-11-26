#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

#include "numa.h"


size_t get_numa_nodes_num(void) {
//    const char *path = "/sys/devices/system/node/";
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

int main() {
    int num_nodes = get_numa_nodes_num();
    if (num_nodes < 0) {
        fprintf(stderr, "Failed to get NUMA nodes\n");
        return 1;
    }

    printf("Number of NUMA nodes: %d\n", num_nodes);
    return 0;
}
