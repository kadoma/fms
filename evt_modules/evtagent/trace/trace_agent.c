/************************************************************
 * Copyright (C) inspur Inc. <http://www.inspur.com>
 * FileName:    main.c
 * Author:      Inspur OS Team 
                wanghuanbj@inspur.com
 * Date:        20xx-xx-xx
 * Description: 
 *
 ************************************************************/
 
#include "wrap.h"
#include "evt_agent.h"
#include "fmd_event.h"
#include "fmd_api.h"
#include "logging.h"
#include "fmd_module.h"
#include "fmd_log.h"


static int
fmd_log_event(fmd_event_t *pevt)
{
	int type, fd;
	int tsize, ecsize, bufsize;
	char *dir;
	char *eclass = NULL;
	char times[26];
	char buf[128];

	char dev_name[32];
	int  err_dev_id = 0;
	time_t evtime;
	
	evtime = pevt->ev_create;
	eclass = pevt->ev_class;
	
	strcpy(dev_name, pevt->dev_name);
	err_dev_id = pevt->ev_err_id;

	if (strncmp(eclass, "ereport.", 8) == 0) {
		type = FMD_LOG_ERROR;
		dir = "/var/log/fms/trace/serd";
	} else if (strncmp(eclass, "fault.", 6) == 0) {
		type = FMD_LOG_FAULT;
		dir = "/var/log/fms/trace/fault";
	} else if (strncmp(eclass, "list.", 5) == 0) {
		type = FMD_LOG_LIST;
		dir = "/var/log/fms/trace/list";
	}

	if ((fd = fmd_log_open(dir, type)) < 0) {
		wr_log("", WR_LOG_ERROR, 
			"FMD: failed to record log for event: %s\n", eclass);
		return LIST_LOGED_FAILED;
	}

	fmd_get_time(times, evtime);
	memset(buf, 0, 128);
	snprintf(buf, sizeof(buf), "\n%s\t%s\t%s\t%d\t\n", times, eclass, dev_name, err_dev_id);

	tsize = strlen(times) + 1;
	ecsize = strlen(eclass) + 1;
	bufsize = tsize + ecsize + 3*sizeof(uint64_t) + 11;

	if (fmd_log_write(fd, buf, bufsize) != 0) {
		wr_log("", WR_LOG_ERROR, 
			"FMD: failed to write log file for event: %s\n", eclass);
		return LIST_LOGED_FAILED;
	}
	fmd_log_close(fd);

	return LIST_LOGED_SUCCESS;
}


/**
 *	handle the fault.* event
 *
 *	@result:
 *		list.* event
 */
fmd_event_t *
trace_handle_event(fmd_t *pfmd, fmd_event_t *event)
{
	wr_log("", WR_LOG_DEBUG, "trace agent ev_handle fault event to list event.");
	int ret = 0;
	
	if(event->ev_flag == AGENT_TOLOG)
	{
		ret = fmd_log_event(event);
		if(ret == 0)
		{
			wr_log("", WR_LOG_ERROR, "trace log sucess.");
			return (fmd_event_t *)fmd_create_listevent(event, LIST_REPAIRED_SUCCESS);
		}
	}

	if(event->ev_flag == AGENT_TODO)
	{
		ret = fmd_log_event(event);
		if(ret != 0)
			wr_log("", WR_LOG_ERROR, "trace log agent todo success.");
		return (fmd_event_t *)fmd_create_listevent(event, LIST_REPAIRED_SUCCESS);
	}

	return NULL;

}


static agent_modops_t trace_ops = {
	.evt_handle = trace_handle_event,
};


fmd_module_t *
fmd_module_init(char *path, fmd_t *p_fmd)
{
	wr_log("trace agent", WR_LOG_DEBUG, "call fmd init agent.");
	return (fmd_module_t *)agent_init(&trace_ops, path, p_fmd);
}

void
fmd_module_finit(fmd_module_t *mp)
{
	return;
}

