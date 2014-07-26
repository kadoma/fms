/*
 * raid_evtsrc.c
 *
 * Copyright (C) 2009 Inspur, Inc.  All rights reserved.
 * Copyright (C) 2009-2010 Fault Managment System Development Team
 *
 * Created on: Tue, Jun 28, 2011
 *      Author: Inspur OS Team
 *  
 * Description: 
 *      raid monitor evtsrc module
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <syslog.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <errno.h>

#include <fmd.h>
#include <fmd_evtsrc.h>
#include <fmd_module.h>
#include <protocol.h>
#include <nvpair.h>

#define	RAID_STAT_FAILED	"failed"
/*
 * raid_fm_event_post
 *  fill the event field to nvl
 *	and add nvl to list 
 *
 * Input
 *	fullclass	- full name of fault
 *	ena			- resource id
 *
 * Output
 *	head		- list that contain one or mor events
 *	nvl			- current event
 *
 * Return value
 *	0
 */
static int
raid_fm_event_post(struct list_head *head,
		nvlist_t *nvl, char *fullclass, uint64_t ena)
{
	sprintf(nvl->name, FM_CLASS);
	strcpy(nvl->value, fullclass);
	nvl->rscid = ena;
	nvlist_add_nvlist(head, nvl);

	printf("Raid module: post %s\n", fullclass);
	return 0;
}

/*
 * raid_probe
 *  probe the raid event and send to fms
 *
 * Input
 *  emp		event source module pointer
 *
 * Return val
 *  return the nvlist which contain the event.
 *  NULL if failed
 */
struct list_head *
raid_probe(evtsrc_module_t *emp)
{
	FILE	*fp_mdstat;
	char	raid_info[LINE_MAX];
	int		str_length;
	int		fault_disk_num;
	int		all_disk_num;
	char	ch;

	uint64_t			ena;
	char				fullclass[PATH_MAX];
	nvlist_t			*nvl;
	struct list_head	*fault;

	fault = nvlist_head_alloc();
	if ( fault == NULL ) {
		fprintf(stderr, "RAID: alloc list_head error\n");
		return NULL;
	}

	/* Open raid status file. */
	fp_mdstat = fopen("/proc/mdstat", "r");
	if (fp_mdstat == NULL)
	{
		fprintf(stderr, "RAID: open /proc/mdstat error!\n");
		return NULL;
	}

	/* Judge raid status. */
	fgets(raid_info, LINE_MAX, fp_mdstat);
	str_length = strlen(raid_info);
	if (str_length < 18)
	{
		fprintf(stderr, "RAID: no raid disk is created, please check!\n");
		goto raid_probe_exit;
	}
	ch = fgetc(fp_mdstat);
	if (ch == 'u')
	{
		fprintf(stderr, "RAID: no used devices, please check!\n");
		goto raid_probe_exit;
	}
	
	/*  Get number of useful disk and number of fault disk. */
	fault_disk_num = 0;
	all_disk_num = 0;
	while (ch != EOF)
	{
		ch = fgetc(fp_mdstat);
		if (ch == 'U')			/* work disk */
			all_disk_num++;
		if (ch == '_')			/* fault disk */
			fault_disk_num++;
	}

	/* post event */
	if (fault_disk_num != 0) {	/* some disk have problem */
		nvl = nvlist_alloc();
		if ( nvl == NULL ) {
			fprintf(stderr, "RAID: unable to alloc nvlist\n");
			goto raid_probe_exit;
		}
		memset(fullclass, 0, sizeof(fullclass));
		sprintf(fullclass, 
				"%s.io.raid.%s",
				FM_EREPORT_CLASS, 
				RAID_STAT_FAILED);
		ena = 0;/*TODO*/
		if ( raid_fm_event_post(fault, nvl, 
					fullclass, 
					ena) != 0 ) {
			free(nvl);
			nvl = NULL;
			goto raid_probe_exit;
		}
		fclose(fp_mdstat);
		return fault;
	} /* end of event post */

raid_probe_exit:
	fclose(fp_mdstat);
	return NULL;
}

static evtsrc_modops_t raid_mops = {
	.evt_probe = raid_probe,
};

fmd_module_t *
fmd_init(const char *path, fmd_t *pfmd)
{
	return (fmd_module_t *)evtsrc_init(&raid_mops, path, pfmd);
}

void
fmd_fini(fmd_module_t *mp)
{
}

