#ifndef __CMEA_CACHE_H__
#define __CMEA_CACHE_H__

extern int cache_to_cpus(int cpu, unsigned level, unsigned type, 
		  int *cpulen, unsigned **cpumap);
void caches_release(void);

#endif
