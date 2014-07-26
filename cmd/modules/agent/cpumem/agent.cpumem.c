/*
 * agent.cpumem.c
 *
 *  Created on: Jan 10, 2011
 *      Author: Inspur OS Team
 *
 *  Descriptions:
 *  	Agent for cpumem module
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <fmd_agent.h>
#include <fmd_event.h>
#include <fmd_api.h>

/**
 *	Offline cpu
 *
 *	@result:
 *		#echo 0 > /sys/devices/system/cpu/cpu#/online
 */
int
cpu_offline(char *file)
{
	int fd = 0;
	if ((fd = open(file, O_RDWR)) == -1) {
		perror("Cpumem Agent");
		return (-1);
	}

	write(fd, "0", 1);
	close(fd);

	return 0;
}

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
cpumem_handle_event(fmd_t *pfmd, fmd_event_t *e)
{
	fmd_debug;
	int ret = 0;

	/* Memory offline */
	if (strstr(e->ev_class, "fault.cpu.intel.mem_dev") != NULL ){
		ret = pages_offline(pfmd, e->ev_rscid);
		if ( ret == 0 )
			return fmd_create_listevent(e, LIST_ISOLATED_SUCCESS);
		else
			return fmd_create_listevent(e, LIST_ISOLATED_FAILED);
	}
	/* CPU offline */
	else if (strstr(e->ev_class, "fault.cpu.intel") != NULL) {
		char buf[64];
		int cpu = -1;
		cpu = topo_cpu_by_cpuid(pfmd, e->ev_rscid);					/* get CPUID */
		if (cpu < 0)
			return fmd_create_listevent(e, LIST_ISOLATED_FAILED);	

		memset(buf, 0, 64 * sizeof(char));
		sprintf(buf, "/sys/devices/system/cpu/cpu%d/online", cpu);
		if ((ret = cpu_offline(buf)) == 0)
			return fmd_create_listevent(e, LIST_ISOLATED_SUCCESS);
		else
			return fmd_create_listevent(e, LIST_ISOLATED_FAILED);
	}

	return fmd_create_listevent(e, LIST_LOG);	// TODO
}


static agent_modops_t cpumem_mops = {
	.evt_handle = cpumem_handle_event,
};


fmd_module_t *
fmd_init(const char *path, fmd_t *pfmd)
{
	fmd_debug;
	return (fmd_module_t *)agent_init(&cpumem_mops, path, pfmd);
}


void
fmd_fini(fmd_module_t *mp)
{
}
