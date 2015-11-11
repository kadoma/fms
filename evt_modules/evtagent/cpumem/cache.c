/*
 * Copyright (C) inspur Inc.  <http://www.inspur.com>
 * FileName:	cache.c
 * Author:		Inspur OS Team
 *				changxch@inspur.com
 * Date:		2015-08-04
 * Description:	
 *				Parse cache information. 
 *
 */

#define _GNU_SOURCE 1
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>
#include <assert.h>
#include <ctype.h>

#include "sysfs.h"
#include "cache.h"
#include "logging.h"
#include "cache.h"
#include "cmea_base.h"
#include "cpumem.h"

struct cache { 
	unsigned level;
	/* Numerical values must match MCACOD */
	enum { INSTR, DATA, UNIFIED } type; 
	unsigned *cpumap;
	unsigned cpumaplen;
};

struct cache **caches;
static unsigned cachelen;

#define PREFIX "/sys/devices/system/cpu"
#define MIN_CPUS 8
#define MIN_INDEX 4

static struct map type_map[] = {
	{ "Instruction", INSTR },
	{ "Data", DATA },
	{ "Unified", UNIFIED },
	{ },
};

static void 
more_cpus(int cpu)
{
	int old = cachelen;
	if (!cachelen)
		cachelen = MIN_CPUS/2;	
	cachelen *= 2;
	caches = realloc(caches, cachelen * sizeof(struct cache *));
	if(!caches) {
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_ERROR,
			"realloc out of memory");
		abort();
		return;
	}
	memset(caches + old, 0, (cachelen - old) * sizeof(struct cache *));
}

static unsigned 
cpumap_len(char *s)
{
	unsigned len = 0, width = 0;
	
	do {
		if (isxdigit(*s))
			width++;
		else {
			len += round_up(width * 4, BITS_PER_INT) / 8;
			width = 0;
		}
	} while (*s++);
	
	return len;
}

static void 
parse_cpumap(char *map, unsigned *buf, unsigned len)
{
	char *s;
	int c;

	c = 0;
	s = map + strlen(map);
	for (;;) { 
		s = memrchr(map, ',', s - map);
		if (!s) 
			s = map;
		else
			s++;
		buf[c++] = strtoul(s, NULL, 16);
		if (s == map)
			break;
		s--;
	}
}

static void
read_cpu_map(struct cache *c, char *cfn)
{
	char *map = read_field(cfn, "shared_cpu_map");
	c->cpumaplen = cpumap_len(map);
	c->cpumap = xcalloc(1, c->cpumaplen);
	parse_cpumap(map, c->cpumap, c->cpumaplen);
	xfree(map);
}

static int 
read_caches(void)
{
	DIR *cpus = opendir(PREFIX);
	struct dirent *de;
	
	if (!cpus) { 
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_ERROR,
			"Cannot read cache topology from %s", 
			PREFIX);
		return -1;
	}
	
	while ((de = readdir(cpus)) != NULL) {
		unsigned cpu;
		
		if (sscanf(de->d_name, "cpu%u", &cpu) == 1) { 
			struct stat st;
			char *fn;
			int i;
			int numindex;

			asprintf(&fn, "%s/%s/cache", PREFIX, de->d_name);
			if (!stat(fn, &st)) {
				numindex = st.st_nlink - 2;
				wr_log(CMEA_LOG_DOMAIN, WR_LOG_DEBUG, "st_nlink: %d", numindex);
				if (numindex < 0)
					numindex = MIN_INDEX;
				if (cachelen <= cpu)
					more_cpus(cpu);
				caches[cpu] = xcalloc(1, sizeof(struct cache) * 
						     (numindex+1));
				for (i = 0; i < numindex; i++) {
					char *cfn;
					struct cache *c = caches[cpu] + i;
					asprintf(&cfn, "%s/index%d", fn, i);
					c->type = read_field_map(cfn, "type", type_map);
					c->level = read_field_num(cfn, "level");
					read_cpu_map(c, cfn);
					xfree(cfn);
				}
			}
			xfree(fn);
		}
	}
	closedir(cpus);
	return 0;
}

void
caches_release(void)
{
	int i; 

	if(!caches)
		return ;
		
	for (i = 0; i < cachelen; i++)
	{
		if (caches[i])
			xfree(caches[i]);
	}
	xfree(caches);
	caches = NULL;
	cachelen = 0;
}

int 
cache_to_cpus(int cpu, unsigned level, unsigned type, 
		   int *cpulen, unsigned **cpumap)
{
	struct cache *c;
	
	if (!caches) {
		if (read_caches() < 0)
			return -1;
		if (!caches) { 
			wr_log(CMEA_LOG_DOMAIN, WR_LOG_ERROR,
				"No caches found in sysfs");
			return -1;
		}
	}

	/* Maybe, cpu hotplug, add new cpu */
	if (caches && !caches[cpu]) {
		caches_release();
		if (read_caches() < 0)
			return -1;
	}
		
	for (c = caches[cpu]; c && c->cpumap; c++) {
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_DEBUG,
			"cpu %d level %d type %d\n", 
			cpu, c->level, c->type);
		
		if (c->level == level && c->type == type) { 
			*cpumap = c->cpumap;
			*cpulen = c->cpumaplen;
			return 0;
		}
	}
	
	wr_log(CMEA_LOG_DOMAIN, WR_LOG_DEBUG,
		"Cannot find sysfs cache for CPU %d",
		cpu);
	return -1;
}
