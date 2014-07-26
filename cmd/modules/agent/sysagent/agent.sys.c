/*
 * agent.sys.c
 *
 *  Created on: Jan 10, 2011
 *      Author: Inspur OS Team
 *
 *  Descriptions:
 *  	Agent for SysMon module
 */

#include <stdio.h>
#include <fmd_agent.h>
#include <fmd_event.h>
#include <fmd_api.h>

/*
 * pages_offline
 * 	offline memory pages which contains addr
 *
 * Inputs
 *	addr	- address that fault occur
 *
 * Return value
 *	0		- if success
 *	non-0	- if failed
 */
static int
pages_offline(fmd_t *pfmd, uint64_t rscid)
{
	uint64_t	s_addr, e_addr;
	char		cmd[128];
	int			ret;

	/* get memory range */
	s_addr = topo_start_by_memid(pfmd, rscid);
	e_addr = topo_end_by_memid(pfmd, rscid);
	
	/* hot remove memory */
	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "mem-hotplug -r -s %lx -l %lx", s_addr, e_addr-s_addr);
	ret = system(cmd);
	if ( WEXITSTATUS(ret) ) 
		return 1;
	else
		return 0;
}


/**
 *	handle the fault.* event
 *
 *	@result:
 *		list.* event
 */
fmd_event_t *
sysagent_handle_event(fmd_t *pfmd, fmd_event_t *e)
{
	fmd_debug;
	int ret = 0;

	/* offline */
	if (strstr(e->ev_class, "fault.cpu.intel.quickpath.mem_page_ce") != NULL) {
		if ((ret = pages_offline(pfmd, e->ev_rscid)) == 0)
			return fmd_create_listevent(e, LIST_ISOLATED_SUCCESS);
		else
			return fmd_create_listevent(e, LIST_ISOLATED_FAILED);
	} else {
//		warning();
		return fmd_create_listevent(e, LIST_LOG);
	}

	/* never reach */
	return fmd_create_listevent(e, LIST_LOG);
}


static agent_modops_t sysagent_mops = {
	.evt_handle = sysagent_handle_event,
};


fmd_module_t *
fmd_init(const char *path, fmd_t *pfmd)
{
	fmd_debug;
	return (fmd_module_t *)agent_init(&sysagent_mops, path, pfmd);
}


void
fmd_fini(fmd_module_t *mp)
{
}
