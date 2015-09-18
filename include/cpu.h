
#ifndef __CPU_H__
#define __CPU_H__ 1

#include <fmd_topo.h>

topo_cpu_t * fmd_topo_read_cpu(const char *dir, int nodeid, int processorid);
void fmd_topo_walk_cpu(const char *dir, int nodeid, fmd_topo_t *ptopo);
int cpu_sibling(topo_cpu_t *pcpu, topo_cpu_t *ppcpu);
int fmd_topo_cpu(const char *dir, const char *prefix, fmd_topo_t *ptopo); 

#endif
