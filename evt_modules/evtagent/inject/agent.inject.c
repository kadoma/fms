
#include "wrap.h"
#include "evt_agent.h"
#include "fmd_event.h"
#include "fmd_api.h"

/**
 *	handle the fault.* event
 *
 *	@result:
 *		list.* event
 */
fmd_event_t *
inject_handle_event(fmd_t *pfmd, fmd_event_t *event)
{
	wr_log("", WR_LOG_DEBUG, "inject agent ev_handle fault event to list event.");
	fmd_event_t *pevt = NULL;
	
	int ret = fmd_log_event(event);
	if(ret == 0)
		pevt = fmd_create_listevent(event, LIST_LOGED_SUCCESS);
	else
		pevt = fmd_create_listevent(event, LIST_LOGED_FAILED);
	
	return pevt;
}


static agent_modops_t inject_mops = {
	.evt_handle = inject_handle_event,
};


fmd_module_t *
fmd_module_ini(char *path, fmd_t *pfmd)
{
	fmd_debug;
	return (fmd_module_t *)agent_init(&inject_mops, path, pfmd);
}

void
fmd_module_fini(fmd_module_t *mp)
{
	return;
}
