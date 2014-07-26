/*
 * agent.mpio.c
 *
 *  Created on: Jan 10, 2011
 *      Author: Inspur OS Team
 *
 *  Descriptions:
 *  	Agent for MPIO module
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
mpio_handle_event(fmd_t *pfmd, fmd_event_t *e)
{
	fmd_debug;
	int ret = 0;

	/* warning */
	if (strstr(e->ev_class, "unknown") != NULL)
		return fmd_create_listevent(e, LIST_LOG);
	else {
		/* FIXME: failure need grade */
//		warning();
		return fmd_create_listevent(e, LIST_LOG);
	}

	return fmd_create_listevent(e, LIST_LOG);	// TODO
}


static agent_modops_t mpio_mops = {
	.evt_handle = mpio_handle_event,
};


fmd_module_t *
fmd_init(const char *path, fmd_t *pfmd)
{
	fmd_debug;
	return (fmd_module_t *)agent_init(&mpio_mops, path, pfmd);
}


void
fmd_fini(fmd_module_t *mp)
{
}
