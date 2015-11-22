/************************************************************
 * Copyright (C) inspur Inc. <http://www.inspur.com>
 * FileName:    agent_disk.c
 * Author:      Inspur OS Team 
			guoms@inspur.com
 * Date:        2015-08-04
 * Description: Process disk check error, and log
 *
 ************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <evt_agent.h>
#include <fmd_event.h>
#include <unistd.h>

#include <fmd_api.h>
#include "logging.h"
#include "fmd_module.h"
#include "fmd_log.h"
#include "fm_disk.h"

#include "wrap.h"


static int
disk_log_event(fmd_event_t *pevt)
{
	int type, fd;
	int tsize, ecsize, bufsize;
	char *dir;
	char *eclass = NULL;
	char times[26];
	char buf[128];

	char dev_name[32];
	char dev_detail[32];
	time_t evttime;

	evttime = pevt->ev_create;
	eclass = pevt->ev_class;

	strcpy(dev_name, pevt->dev_name);

	if(pevt->event_type == EVENT_LIST)
	{
		dir = "/var/log/fms/disk/list";
		type = FMD_LOG_LIST;
        
	}else if(pevt->event_type == EVENT_FAULT)
	{
		dir = "/var/log/fms/disk/fault";
		type = FMD_LOG_FAULT;
        
	}else{
		dir = "/var/log/fms/disk/serd";
		type = FMD_LOG_ERROR;
	}


	if ((fd = fmd_log_open(dir, type)) < 0) {
		wr_log("disk_agent", WR_LOG_ERROR,
			"FMD:failed to record log for event: %s\n", eclass);
		return -1;
	}

	fmd_get_time(times, evttime);
	memset(buf, 0, 128);
	snprintf(buf, sizeof(buf), "%s\t%s\t%s\n", times, eclass, dev_name);

	if (fmd_log_write(fd, buf, strlen(buf)) != 0) {
		wr_log("disk_agent", WR_LOG_ERROR, "FMD: failed to write log file for event: %s\n", eclass);
		return -1;
	}
    
	fmd_log_close(fd);
	return 0;
}

/**
 *	handle the fault.* event
 *
 *	@result:
 *		list.* event
 */
fmd_event_t *
disk_handle_event(fmd_t *pfmd, fmd_event_t *event)
{
	wr_log("disk_agent", WR_LOG_DEBUG, "disk agent ev_handle fault event to list event.");

	if(event->ev_flag == AGENT_TODO)
	{
        //todo some
        wr_log("disk_agent", WR_LOG_NORMAL, "disk agent to do something and return list event.");
        event->event_type = EVENT_LIST;
        // to log list event
        disk_log_event(event);
        
        //only isolated is in repaired_db
        return (fmd_event_t *)fmd_create_listevent(event, LIST_ISOLATED_SUCCESS);
	}
    
	// every fault, ereport ,serd to log.
	disk_log_event(event);

	return NULL;
}


static agent_modops_t disk_mops = {
	.evt_handle = disk_handle_event,
};


fmd_module_t *
fmd_module_init(char *path, fmd_t *pfmd)
{
	wr_log("disk_agent", WR_LOG_DEBUG, "disk_agent thread init");
	return (fmd_module_t *)agent_init(&disk_mops, path, pfmd);
}

void
fmd_module_finit(fmd_module_t *mp)
{
	return;
}
