/*
 * agent.disk.c
 *
 *  Created on: Jan 10, 2011
 *      Author: Inspur OS Team
 *
 *  Descriptions:
 *  	Agent for disk module
 */

#include <stdio.h>
#include <fcntl.h>
#include <fmd_agent.h>
#include <fmd_event.h>
#include <fmd_api.h>

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
	int ret = 0;

	/* warning */
	if ((strstr(e->ev_class, "predictive-failure") != NULL) ||
	    (strstr(e->ev_class, "self-test-failure") != NULL) ||
	    (strstr(e->ev_class, "disk-error") != NULL))
//		warning();
		return fmd_create_listevent(e, LIST_LOG);

	/* offline */
	if ((strstr(e->ev_class, "over-temperature") != NULL) ||
	    (strstr(e->ev_class, "fault.io.sata.") != NULL)) {
		char buf[64];
		char *disk = NULL;
		disk = topo_storage_by_storageid(pfmd, e->ev_rscid);

		memset(buf, 0, 64 * sizeof(char));
		sprintf(buf, "/sys/block/%s/device/delete", disk);
		if ((ret = disk_offline(buf)) == 0)
			return fmd_create_listevent(e, LIST_ISOLATED_SUCCESS);
		else
			return fmd_create_listevent(e, LIST_ISOLATED_FAILED);
	}

	return fmd_create_listevent(e, LIST_LOG); // TODO
}


static agent_modops_t disk_mops = {
	.evt_handle = disk_handle_event,
};


fmd_module_t *
fmd_init(const char *path, fmd_t *pfmd)
{
	fmd_debug;
	return (fmd_module_t *)agent_init(&disk_mops, path, pfmd);
}


void
fmd_fini(fmd_module_t *mp)
{
}
