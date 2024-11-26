#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>

int get_num_numa_nodes() {
    const char *path = "/sys/devices/system/node/";
    struct dirent **namelist;
    int num_nodes = 0;

    // Use scandir to fetch all entries in the directory
    int n = scandir(path, &namelist, NULL, alphasort);
    if (n < 0) {
        perror("scandir failed");
        return -1;
    }

    // Count entries that start with "node"
    for (int i = 0; i < n; i++) {
        if (strncmp(namelist[i]->d_name, "node", 4) == 0) {
            num_nodes++;
        }
        free(namelist[i]); // Free memory allocated for this entry
    }

    free(namelist); // Free the array of pointers
    return num_nodes;
}

int main() {
    int num_nodes = get_num_numa_nodes();
    if (num_nodes < 0) {
        fprintf(stderr, "Failed to get NUMA nodes\n");
        return 1;
    }

    printf("Number of NUMA nodes: %d\n", num_nodes);
    return 0;
}
