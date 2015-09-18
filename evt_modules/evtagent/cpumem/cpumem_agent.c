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

#include "cache.h"
#include "evt_agent.h"
#include "evt_process.h"
#include "cmea_base.h"
#include "cpumem.h"

#include "fmd_log.h"

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

	memset(&edata, 0, sizeof edata);
	edata.cpu_action = e->ev_refs;
	if(cmea_evt_processing(evt_type, &edata, e->data)) {
		return LIST_REPAIRED_FAILED;
	}

	return LIST_REPAIRED_SUCCESS;
}

fmd_event_t *
cpumem_handle_event(fmd_t *pfmd, fmd_event_t *e)
{
	int action = 0;
	uint64_t ev_flag;

	ev_flag = e->ev_flag;
	switch (ev_flag) {
	case AGENT_TODO:
		wr_log("", WR_LOG_DEBUG, 
			"cpumem agent handle fault event.");
		action = __handle_fault_event(e);
		break;
	case AGENT_TOLOG:
		wr_log("", WR_LOG_DEBUG, 
			"cpumem agent log event.");
		action = fmd_log_event(e); 
		break;
	default:
		action = -1;  // can happen?? 
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_ERROR,
			"unknown handle event flag");
	}
	
	return (fmd_event_t *)fmd_create_listevent(e, action);
}

static agent_modops_t cpumem_mops = {
	.evt_handle = cpumem_handle_event,
};

fmd_module_t *
fmd_module_init(char *path, fmd_t *pfmd)
{
	return (fmd_module_t *)agent_init(&cpumem_mops, path, pfmd);
}

void
fmd_module_finit(fmd_module_t *mp)
{
	caches_release();
}
