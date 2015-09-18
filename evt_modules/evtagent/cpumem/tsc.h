#ifndef __CMEA_TSC_H__
#define __CMEA_TSC_H__

enum cputype;
extern int decode_tsc_current(char **buf, int cpunum, enum cputype cputype, 
		       double mhz, unsigned long long tsc);

#endif