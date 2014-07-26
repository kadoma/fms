/*
 * agent.proc.c
 *
 *  Created on: Jan 19, 2011
 *      Author: Inspur OS Team
 *
 *  Descriptions:
 *  	Agent for proc module
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <fmd_agent.h>
#include <fmd_event.h>
#include <fmd_api.h>
#include "agent.proc.h"

/**
 *
 *
 *
 */
fmd_event_t *
proc_handle_event(fmd_t *pfmd, fmd_event_t *e)
{
	fmd_debug;
	int 	ret = 0;
	char	command[LINE_MAX];
	struct 	proc_mon *proc_info;

	/* copy to private buffer */
	proc_info = (struct proc_mon*)malloc(sizeof(struct proc_mon));
	if ( proc_info == NULL )
		return fmd_create_listevent(e, LIST_LOG);/*TODO*/
	memcpy(proc_info, e->ev_data->data, sizeof(struct proc_mon));

	/* restart process */
	switch ( proc_info->operation ) 
	{
		case PROC_OP_RESTART:	/* restart the process */
			memset(command, 0, sizeof(command));
			sprintf(command, "%s %s", proc_info->command, proc_info->args);
			system(command);
			break;
		case PROC_OP_NOTHING:
			break;
		default:
			break;
	}

	return fmd_create_listevent(e, LIST_LOG);
}


static agent_modops_t proc_mops = {
	.evt_handle = proc_handle_event,
};


fmd_module_t *
fmd_init(const char *path, fmd_t *pfmd)
{
	fmd_debug;
	return (fmd_module_t *)agent_init(&proc_mops, path, pfmd);
}


void
fmd_fini(fmd_module_t *mp)
{
}
