/*
 * agent.apache.c
 *
 *  Created on: Nov 15, 2010
 *      Author: Inspur OS Team
 *
 *  Descriptions:
 *  	Agent for apache module
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
apache_handle_event(fmd_t *pfmd, fmd_event_t *e)
{
	fmd_debug;

	system("service httpd stop");
	system("service httpd start");
	return fmd_create_listevent(e, LIST_RECOVER_SUCCESS);
}


static agent_modops_t apache_mops = {
	.evt_handle = apache_handle_event,
};


fmd_module_t *
fmd_init(const char *path, fmd_t *pfmd)
{
	fmd_debug;
	return (fmd_module_t *)agent_init(&apache_mops, path, pfmd);
}


void
fmd_fini(fmd_module_t *mp)
{
}
