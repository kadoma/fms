#ifndef __MEMDEV_H__
#define __MEMDEV_H__

#include "cpumem.h"

extern void dump_memdev(int fd, char *buf, 
	struct dmi_memdev *md, unsigned long addr);

#endif
