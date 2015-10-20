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

#include "wrap.h"


static int
__disk_log_event(fmd_event_t *pevt)
{
	int type, fd;
	char *dir;
	char *eclass = NULL;
	char times[26];
	char buf[512];

	char dev_name[32];
	time_t evttime;

	evttime = pevt->ev_create;
	eclass = pevt->ev_class;

	strcpy(dev_name, pevt->dev_name);

	if (strncmp(eclass, "ereport.", 8) == 0) {
		type = FMD_LOG_ERROR;
		dir = "/var/log/fms/disk/serd";
	} else if (strncmp(eclass, "fault.", 6) == 0) {
		type = FMD_LOG_FAULT;
		dir = "/var/log/fms/disk/fault";
	} else if (strncmp(eclass, "list.", 5) == 0) {
		type = FMD_LOG_LIST;
		dir = "/var/log/fms/disk/list";
	}

	if ((fd = fmd_log_open(dir, type)) < 0) {
		wr_log("disk_agent", WR_LOG_ERROR,
			"failed to record log for event: %s\n", eclass);
		return LIST_LOGED_FAILED;
	}

	fmd_get_time(times, evttime);
	memset(buf, 0, sizeof buf);
	snprintf(buf, sizeof(buf), "%s\t%s\t%s\t,please take action.\n", times, eclass, dev_name);

	if (fmd_log_write(fd, buf, strlen(buf)) != 0) {
		wr_log("disk agent", WR_LOG_ERROR,
			"failed to write log file for event: %s\n", eclass);
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
disk_handle_event(fmd_t *pfmd, fmd_event_t *e)
{
	fmd_debug;

	int action = 0;
	uint64_t ev_flag;
	ev_flag = e->ev_flag;
	switch (ev_flag) {
	case AGENT_TODO:
		wr_log("disk agent handle", WR_LOG_DEBUG,
			"disk agent handle fault event.");
		//action = __handle_disk_fault_event(e);
		break;
	case AGENT_TOLOG:
		wr_log("disk agent record log", WR_LOG_DEBUG,
			"disk agent record log event.");
		action = __disk_log_event(e);
		break;
	default:
		action = -1;
		wr_log("disk agent ", WR_LOG_ERROR,
			"unknown handle event flag");
	}

	return (fmd_event_t *)fmd_create_listevent(e, action);
}


static agent_modops_t disk_mops = {
	.evt_handle = disk_handle_event,
};


fmd_module_t *
fmd_module_init(char *path, fmd_t *pfmd)
{
	fmd_debug;
	return (fmd_module_t *)agent_init(&disk_mops, path, pfmd);
}

void
fmd_module_finit(fmd_module_t *mp)
{
	return;
}
