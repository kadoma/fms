/*
 * agent.raid.c
 *
 *  Created on: Jun 28, 2011
 *      Author: Inspur OS Team
 *
 *  Descriptions:
 *  	Agent for raid module
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <fmd_agent.h>
#include <fmd_event.h>
#include <fmd_api.h>

/**
 *
 *
 *
 */
fmd_event_t *
raid_handle_event(fmd_t *pfmd, fmd_event_t *e)
{
	/* TODO */
	return fmd_create_listevent(e, LIST_LOG);
}


static agent_modops_t raid_mops = {
	.evt_handle = raid_handle_event,
};


fmd_module_t *
fmd_init(const char *path, fmd_t *pfmd)
{
	fmd_debug;
	return (fmd_module_t *)agent_init(&raid_mops, path, pfmd);
}


void
fmd_fini(fmd_module_t *mp)
{
}
