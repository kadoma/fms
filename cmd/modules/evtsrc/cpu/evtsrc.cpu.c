/*
 * evtsrc.cpu.c
 *
 *  Created on: Jan 16, 2010
 *      Author: Inspur OS Team
 *
 *  Descriptions:
 *  	evtsrc for cpu
 */

#include <stdio.h>
#include <stdint.h>
#include <fmd.h>
#include <nvpair.h>
#include <fmd_evtsrc.h>
#include <fmd_module.h>

struct list_head *
cpu_probe(evtsrc_module_t *emp)
{
	fmd_debug;

	return NULL;
}


/**
 * cpu_create
 *
 * TODO nvlist operations
 */
fmd_event_t *
cpu_create(evtsrc_module_t *emp, nvlist_t *pnv)
{
/*
	fmd_t *pfmd = ((fmd_module_t *)emp)->mod_fmd;
	//TODO
	int processor = 1;
	uint64_t rscid = topo_get_cpuid(pfmd, processor);
	char *eclass = "ereport.cpu.cache.d.l1";
	fmd_debug;
	return (fmd_event_t *)fmd_create_ereport(pfmd, eclass, rscid, pnv);
*/
	return NULL;
}


static evtsrc_modops_t cpu_mops = {
	.evt_probe = cpu_probe,
	.evt_create = cpu_create,
};


fmd_module_t *
fmd_init(const char *path, fmd_t *pfmd)
{
	fmd_debug;
	return (fmd_module_t *)evtsrc_init(&cpu_mops, path, pfmd);
}

void
fmd_fini(fmd_module_t *mp)
{
}
