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
	int  dev_id = 0;
    time_t evtime;
	
	evtime = pevt->ev_create;
	eclass = pevt->ev_class;
	
	strcpy(dev_name, pevt->dev_name);
	dev_id = (pevt->dev_id >> 16) & 0xFF;

	if(pevt->event_type == EVENT_LIST)
	{
		dir = "/var/log/fms/trace/list";
		type = FMD_LOG_LIST;
        
	}else if(pevt->event_type == EVENT_FAULT)
	{
		dir = "/var/log/fms/trace/fault";
		type = FMD_LOG_FAULT;
        
	}else{
		dir = "/var/log/fms/trace/serd";
		type = FMD_LOG_ERROR;
	}

	if ((fd = fmd_log_open(dir, type)) < 0) {
		wr_log("", WR_LOG_ERROR, 
			"FMD: failed to record log for event: %s\n", eclass);
		return -1;
	}

	fmd_get_time(times, evtime);
	memset(buf, 0, 128);
	snprintf(buf, sizeof(buf), "%s\t%s\t%s:\t%d\n", times, eclass, dev_name, dev_id);

	if(fmd_log_write(fd, buf, strlen(buf)) != 0){
		wr_log("", WR_LOG_ERROR, 
			"FMD: failed to write log file for event: %s\n", eclass);
		return -1;
	}
	fmd_log_close(fd);

	return -1;
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

	if(event->ev_flag == AGENT_TODO)
	{
        //todo some
        wr_log("trace agent", WR_LOG_NORMAL, "trace agent to do something and return list event.");
        event->event_type = EVENT_LIST;
        // to log list event
        fmd_log_event(event);
        return (fmd_event_t *)fmd_create_listevent(event, LIST_ISOLATED_SUCCESS);
	}
    // every fault, ereport ,serd to log.
	fmd_log_event(event);

	return NULL;
}


static agent_modops_t trace_ops = {
	.evt_handle = trace_handle_event,
};


fmd_module_t *
fmd_module_init(char *path, fmd_t *p_fmd)
{
	wr_log("trace agent", WR_LOG_DEBUG, "trace agent thread init");
	return (fmd_module_t *)agent_init(&trace_ops, path, p_fmd);
}

void
fmd_module_finit(fmd_module_t *mp)
{
	return;
}

