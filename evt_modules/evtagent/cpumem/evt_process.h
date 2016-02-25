#ifndef __CMEA_EVT_PROCESS_H__
#define __CMEA_EVT_PROCESS_H__

#include "cpumem.h"

struct cmea_evt_data {
	int cpu_action;  /* =1: Flush CPU cache. =2: CPU offline */
};

extern void cpumem_err_printf(int fd, struct fms_cpumem *fc);
extern int cmea_evt_processing(char *evt_type, struct cmea_evt_data *edata, void *data);
extern inline void line_indent(int fd);

#ifdef TEST_CMEA
extern int test_cpu_offine_on_cache(int cpu, unsigned clevel, unsigned ctype);
extern int test_memory_page_offline(uint64_t addr);
#endif /* TEST_CMEA */

#endif /* __CMEA_EVT_PROCESS_H__ */