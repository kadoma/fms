/*
 * agent.cpu.c
 *
 *  Created on: Jan 19, 2010
 *      Author: Inspur OS Team
 *
 *  Descriptions:
 *  	Agent for cpu
 */

#include <stdio.h>
#include <fmd_agent.h>
#include <fmd_event.h>
#include <fmd_api.h>

/**
 *	handle the fault.* event
 *
 *	@result:
 *		list.* event
 */
fmd_event_t *
cpu_handle_event(fmd_t *pfmd, fmd_event_t *e)
{
	fmd_debug;
	return fmd_create_listevent(e, LIST_LOG);
}


static agent_modops_t cpu_mops = {
	.evt_handle = cpu_handle_event,
};


fmd_module_t *
fmd_init(const char *path, fmd_t *pfmd)
{
	fmd_debug;
	return (fmd_module_t *)agent_init(&cpu_mops, path, pfmd);
}


void
fmd_fini(fmd_module_t *mp)
{
}
