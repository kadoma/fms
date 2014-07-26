/*
 * mpio_evtsrc.c
 *
 * Copyright (C) 2009 Inspur, Inc.  All rights reserved.
 * Copyright (C) 2009-2010 Fault Managment System Development Team
 *
 * Created on: Dec, 2, 2010
 *      Author: Inspur OS Team
 *  
 * Description: 
 *      mpio error evtsrc module
 */

#include <stdio.h>
#include <string.h>
#include <limits.h>

#include <fmd.h>
#include <fmd_evtsrc.h>
#include <fmd_module.h>
#include <protocol.h>
#include <nvpair.h>
#include "mpio.h"

int
mpio_fm_event_post(nvlist_t *nvl, char *fullclass, uint64_t ena)
{
	sprintf(nvl->name, FM_CLASS);
	strcpy(nvl->value, fullclass);
	nvl->rscid = ena;

	printf("MPIO Module: post %s\n", fullclass);

	return 0;
}

struct list_head *
mpio_probe(evtsrc_module_t *emp)
{
	int	ena;
	char 	*raw_msg;
	char	fullclass[PATH_MAX];
	char 	dm_name[PATH_MAX];

	struct list_head *head = nvlist_head_alloc();
	nvlist_t *nvl = nvlist_alloc();
	nvlist_add_nvlist(head, nvl);
	
	/* Read msg from kernel via netlink */
	raw_msg = get_mpio();		
	if (raw_msg == NULL) {
		fprintf(stdout, "MPIO: No error message\n");
		return NULL;
	}

	/* get error type */
	memset(dm_name, 0, sizeof(dm_name));
	if ( sscanf(raw_msg, "DM.%s.%*s.%*s.%*s.%*s", dm_name) <= 0) {
		fprintf(stderr, "MPIO: Invalid message\n");
		return NULL;
	}
	
	/* fill full ereport class */
	if ( strcmp(dm_name, "PATH_FAILED") == 0 )
		snprintf(fullclass, sizeof(fullclass),
				"%s.io.mpio.%s",
				FM_EREPORT_CLASS, "fail");
	else if ( strcmp(dm_name, "PATH_REINSTATED") == 0)
		snprintf(fullclass, sizeof(fullclass),
				"%s.io.mpio.%s",
				FM_EREPORT_CLASS, "reinstated");
	else 
		snprintf(fullclass, sizeof(fullclass),
				"%s.io.mpio.%s",
				FM_EREPORT_CLASS, "unknown");
	
	/* fault location */
	ena = 0;/* FIXME */
//	ena = topo_get_storageid(emp->module.mod_fmd, "sd#");

	/* post error */
	if (mpio_fm_event_post(nvl, fullclass, ena) != 0) {
		nvlist_free(nvl);
		nvl = NULL;
	}

	return head;
}

static evtsrc_modops_t mpio_mops = {
	.evt_probe = mpio_probe,
};

fmd_module_t *
fmd_init(const char *path, fmd_t *pfmd)
{
	return (fmd_module_t *)evtsrc_init(&mpio_mops, path, pfmd);
}

void
fmd_fini(fmd_module_t *mp)
{
}

