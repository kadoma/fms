/************************************************************
 * Copyright (C) inspur Inc. <http://www.inspur.com>
 * FileName:    main.c
 * Author:      Inspur OS Team 
                wanghuanbj@inspur.com
 * Date:        20xx-xx-xx
 * Description: 
 *
 ************************************************************/
 
#include "wrap.h"
#include "evt_agent.h"
#include "fmd_event.h"
#include "fmd_api.h"
#include "logging.h"
#include "fmd_module.h"

/**
 *	handle the fault.* event
 *
 *	@result:
 *		list.* event
 */
fmd_event_t *
trace_handle_event(fmd_t *pfmd, fmd_event_t *event)
{
	wr_log("", WR_LOG_DEBUG, "trace agent ev_handle fault event to list event.");
	
	int ret = fmd_log_event(event);

	// fault.* to list.*
	if(ret == 0)
		return (fmd_event_t *)fmd_create_listevent(event, LIST_LOGED_SUCCESS);
	else
		return (fmd_event_t *)fmd_create_listevent(event, LIST_LOGED_FAILED);
}


static agent_modops_t trace_ops = {
	.evt_handle = trace_handle_event,
};


fmd_module_t *
fmd_module_init(char *path, fmd_t *p_fmd)
{
	wr_log("trace agent", WR_LOG_DEBUG, "call fmd init agent.");
	return (fmd_module_t *)agent_init(&trace_ops, path, p_fmd);
}

void
fmd_module_finit(fmd_module_t *mp)
{
	return;
}

