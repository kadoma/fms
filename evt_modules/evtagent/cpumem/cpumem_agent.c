/*
 * Copyright (C) inspur Inc.  <http://www.inspur.com>
 * FileName:	cpumem_agent.c
 * Author:		Inspur OS Team
 *				changxch@inspur.com
 * Date:		2015-08-04
 * Description:	
 *				Process machine check error, and log.
 *
 */
#include <string.h>
#include <ctype.h>

#include "cache.h"
#include "evt_agent.h"
#include "evt_process.h"
#include "cmea_base.h"
#include "cpumem.h"

#include "fmd_log.h"

char *processor_flags;
double cpumhz;

static int
__cpuinfo_init()
{
	enum { 
		MHZ = 1, 
		FLAGS = 2, 
		ALL = 0x3 
	} seen = 0;
	FILE *f;

	f = fopen("/proc/cpuinfo", "r");
	if (f != NULL) { 
		char *line = NULL;
		size_t linelen = 0; 
		double mhz;

		while (getdelim(&line, &linelen, '\n', f) > 0  && seen != ALL) { 
			/* We use only Mhz of the first CPU, assuming they are the same
			   (there are more sanity checks later to make this not as wrong
			   as it sounds) */
			if (sscanf(line, "cpu MHz : %lf", &mhz) == 1) { 
					cpumhz = mhz;
				seen |= MHZ;
			}
			if (!strncmp(line, "flags", 5) && isspace(line[6])) {
				processor_flags = line;
				line = NULL;
				linelen = 0;
				seen |= FLAGS;
			}			      
		} 

		wr_log(CMEA_LOG_DOMAIN, WR_LOG_DEBUG, 
			"cpumhz: %f, processor_flags: %s", 
			cpumhz, processor_flags); 
		
		fclose(f);
		free(line);
	} else
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_WARNING, 
			"Cannot open /proc/cpuinfo");

	return 0;
}

static int 
cpumem_agent_init(void)
{
	__cpuinfo_init();

	return 0;
}

static void 
cpumem_agent_release(void)
{
	caches_release();
}

static inline int
__handle_fault_event(fmd_event_t *e)
{
	char *evt_class;
	char *evt_type = NULL;
	struct cmea_evt_data edata;

	evt_class = e->ev_class;
	evt_type = strrchr(evt_class, '.');
	if(!evt_type) {
		return LIST_REPAIRED_FAILED;
	}
	evt_type++;

	memset(&edata, 0, sizeof edata);
	edata.cpu_action = e->ev_refs;

	return (cmea_evt_processing(evt_type, &edata, e->data));
}

static int
__log_event(fmd_event_t *pevt) 
{
	int type, fd;
	char *dir;
	char *eclass = NULL;
	char times[26];
	char buf[512];

	char dev_name[32];
	uint64_t dev_id = 0;
	time_t evtime;
	
	evtime = pevt->ev_create;
	eclass = pevt->ev_class;
	
	strcpy(dev_name, pevt->dev_name);
	dev_id = pevt->dev_id;

	if (pevt->event_type == EVENT_FAULT) {
		type = FMD_LOG_FAULT;
		dir = "/var/log/fms/cpumem/fault";
	} else if (pevt->event_type == EVENT_LIST) {
		type = FMD_LOG_LIST;
		dir = "/var/log/fms/cpumem/list";
	} else {
		type = FMD_LOG_ERROR;
		dir = "/var/log/fms/cpumem/serd";
	}

	if ((fd = fmd_log_open(dir, type)) < 0) {
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_ERROR, 
			"failed to record log for event: %s\n", eclass);
		return -1;
	}

	fmd_get_time(times, evtime);
	memset(buf, 0, sizeof buf);
	snprintf(buf, sizeof(buf), "%s\t%s\t%s:\t%llu\t\n", times, eclass, dev_name, 
		(long long unsigned int)dev_id);

	if (fmd_log_write(fd, buf, strlen(buf)) != 0) {
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_ERROR, 
			"failed to write log file for event: %s\n", eclass);
		return -1;
	}
	fmd_log_close(fd);

	return 0;
}

#ifndef TEST_CMEA

fmd_event_t *
cpumem_handle_event(fmd_t *pfmd, fmd_event_t *e)
{
	int action = 0;
	int ret = 0;
	uint64_t ev_flag;

	ev_flag = e->ev_flag;
	wr_log(CMEA_LOG_DOMAIN, WR_LOG_DEBUG,
		"handle event, class: %s  flag: %08x,", 
		e->ev_class, ev_flag);

	__log_event(e); 
	
	switch (ev_flag) {
	case AGENT_TODO:
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_DEBUG, 
			"cpumem agent handle fault event.");
		ret = __handle_fault_event(e);
		e->event_type = EVENT_LIST;
		__log_event(e);
		action = 1;
		break;
	}

	if (!action) {
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_DEBUG, 
			"cpumem agent log event.");
		return NULL;
	}
	
	wr_log(CMEA_LOG_DOMAIN, WR_LOG_DEBUG,
			"handle event result: %08x", 
			ret);
	
	return (fmd_event_t *)fmd_create_listevent(e, ret);
}

static agent_modops_t cpumem_mops = {
	.evt_handle = cpumem_handle_event,
};

fmd_module_t *
fmd_module_init(char *path, fmd_t *pfmd)
{
	if (cpumem_agent_init()) {
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_ERROR,
			"failed to cpumem agent init");
		return NULL;
	}
	
	return (fmd_module_t *)agent_init(&cpumem_mops, path, pfmd);
}

void
fmd_module_finit(fmd_module_t *mp)
{
	cpumem_agent_release();
}

#endif
