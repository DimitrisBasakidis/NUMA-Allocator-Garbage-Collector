#ifndef NUMA
#define NUMA

#define MAX_CPUS 256

extern unsigned cpu_on_node[MAX_CPUS];

size_t get_numa_nodes_num(void);
void parse_cpus_to_node(void);

#endif

