/************************************************************
 * Copyright (C) inspur Inc. <http://www.inspur.com>
 * FileName:    agent_disk.c
 * Author:      Inspur OS Team 

 * Date:        20xx-xx-xx
 * Description: the fault manager system main function
 *
 ************************************************************/

#include <stdio.h>
#include <fcntl.h>
#include <evt_agent.h>
#include <fmd_event.h>
#include <unistd.h>

#include <fmd_api.h>
#include "logging.h"
#include "fmd_module.h"



/**
 *	Offline disk
 *
 *	@result:
 *		#echo 1 > /sys/block/sd#/device/delete
 */

int
disk_offline(char *file)
{
	int fd = 0;
	if ((fd = open(file, O_RDWR)) == -1) {
		perror("Disk Agent");
		return (-1);
	}

	write(fd, "1", 1);
	close(fd);

	return 0;
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

	/* warning */
	if ((strstr(e->ev_class, "space-insufficient") != NULL))
//		warning();
		return (fmd_event_t *)fmd_create_listevent(e, LIST_LOG);

	/* offline */
	if ((strstr(e->ev_class, "over-temperature") != NULL) ||
	    (strstr(e->ev_class, "unhealthy") != NULL) ||
	    (strstr(e->ev_class, "badblocks") != NULL)) {
		char buf[64];
		char *disk = NULL;
		disk = e->dev_name;

		memset(buf, 0, 64 * sizeof(char));
		sprintf(buf, "/sys/block/%s/device/delete", disk);
		if (disk_offline(buf) == 0)
			return (fmd_event_t *)fmd_create_listevent(e, LIST_ISOLATED_SUCCESS);
		else
			return (fmd_event_t *)fmd_create_listevent(e, LIST_ISOLATED_FAILED);
	}

	return (fmd_event_t *)fmd_create_listevent(e, LIST_LOG); // TODO
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
