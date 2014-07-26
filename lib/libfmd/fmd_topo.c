/*
 * fmd_topo.c
 *
 *  Created on: Dec 20, 2010
 *      Author: Inspur OS Team
 *  
 *  Description:
 *  	fmd_topo.c
 */

#include <stdio.h>
#include <syslog.h>
#include <fmd.h>
#include <fmd_list.h>
#include <fmd_topo.h>

#define MEM_LOC_SUPPORT	0		/* TODO we didn't support memory locate yet */

/**
 * topo_get_cpuid
 *
 * @return
 * 	0: error
 * 	!0:OK
 */
uint64_t
topo_get_cpuid(fmd_t *pfmd, int cpunm)
{
	topo_cpu_t *pcpu;
	struct list_head *pos;
	fmd_topo_t *ptopo = &pfmd->fmd_topo;
	uint64_t rscid;

	list_for_each(pos, &ptopo->list_cpu) {
		pcpu = list_entry(pos, topo_cpu_t, list);

		syslog(LOG_DEBUG, "pcpu->processor=%d, cpunm=%d\n",
				pcpu->processor, cpunm);

		if (pcpu->processor == cpunm) {
			rscid = pcpu->cpu_id;
			return rscid;
		}
	}

	return 0;
}

/**
 * topo_get_pcid
 *
 * @param
 *	pciname: eg. eth0
 * @return
 *      0: error
 *      !0:OK
 */
uint64_t
topo_get_pcid(fmd_t *pfmd, char *pciname)
{
	topo_pci_t *ppci;
	struct list_head *pos;
	fmd_topo_t *ptopo = &pfmd->fmd_topo;

	list_for_each(pos, &ptopo->list_pci) {
		ppci = list_entry(pos, topo_pci_t, list);

		if (ppci->pci_name != NULL) {
			if (strcmp(ppci->pci_name, pciname) == 0)
				return ppci->pci_id;
		}
	}

	return 0;
}

/**
 * topo_get_memid
 *
 * @return
 *      0: error
 *      !0:OK
 */
uint64_t
topo_get_memid(fmd_t *pfmd, uint64_t addr)
{
#if MEM_LOC_SUPPORT
	topo_mem_t *pmem;
	struct list_head *pos;
	fmd_topo_t *ptopo = &pfmd->fmd_topo;

	list_for_each(pos, &ptopo->list_mem) {
		pmem = list_entry(pos, topo_mem_t, list);
		if ((addr >= pmem->start) && (addr <= pmem->end))
			return pmem->mem_id;
	}

	return 0;
#else
	return addr;
#endif
}

/**
 * topo_get_storageid
 *
 * @param
 *	storage_name: eg. sda
 * @return
 *      0: error
 *      !0:OK
 */
uint64_t
topo_get_storageid(fmd_t *pfmd, char *storage_name)
{
	topo_storage_t *pstr;
	struct list_head *pos;
	fmd_topo_t *ptopo = &pfmd->fmd_topo;

	list_for_each(pos, &ptopo->list_storage) {
		pstr = list_entry(pos, topo_storage_t, list);
		if (pstr->storage_name == NULL) {
#ifdef FMD_DEBUG
			syslog(LOG_ERR, "Empty storage name!\n");
#endif
			continue;		
		}
		if (strcmp(pstr->storage_name, storage_name) == 0)
			return pstr->storage_id;
	}

	return 0;
}


/**
 * topo_cpu_by_cpuid
 *
 * @param
 *	rscid
 * @return
 *
 */
int
topo_cpu_by_cpuid(fmd_t *pfmd, uint64_t rscid)
{
	topo_cpu_t *pcpu;
	struct list_head *pos;
	fmd_topo_t *ptopo = &pfmd->fmd_topo;

	list_for_each(pos, &ptopo->list_cpu) {
		pcpu = list_entry(pos, topo_cpu_t, list);

		if (pcpu->cpu_id == rscid)
			return pcpu->processor;
	}

	return (-1);
}


/**
 * topo_start_by_memid
 *
 * @param
 *	rscid
 * @return
 *
 */
uint64_t
topo_start_by_memid(fmd_t *pfmd, uint64_t rscid)
{
#if MEM_LOC_SUPPORT
	topo_mem_t *pmem;
	struct list_head *pos;
	fmd_topo_t *ptopo = &pfmd->fmd_topo;

	list_for_each(pos, &ptopo->list_mem) {
		pmem = list_entry(pos, topo_mem_t, list);

		if (pmem->mem_id == rscid)
			return pmem->start;
	}

	return (uint64_t)(-1);
#else
	return (rscid & 0xFFFFFFFFFFFF0000);
#endif
}


/**
 * topo_end_by_memid
 *
 * @param
 *	rscid
 * @return
 *
 */
uint64_t
topo_end_by_memid(fmd_t *pfmd, uint64_t rscid)
{
#if MEM_LOC_SUPPORT
	topo_mem_t *pmem;
	struct list_head *pos;
	fmd_topo_t *ptopo = &pfmd->fmd_topo;

	list_for_each(pos, &ptopo->list_mem) {
		pmem = list_entry(pos, topo_mem_t, list);

		if (pmem->mem_id == rscid)
			return pmem->end;
	}

	return (uint64_t)(-1);
#else
	return ((rscid+0x10000) & 0xFFFFFFFFFFFF0000);
#endif
}


/**
 * topo_pci_by_pcid
 *
 * @param
 *	rscid
 * @return
 *
 */
char *
topo_pci_by_pcid(fmd_t *pfmd, uint64_t rscid)
{
	topo_pci_t *ppci;
	struct list_head *pos;
	fmd_topo_t *ptopo = &pfmd->fmd_topo;

	list_for_each(pos, &ptopo->list_pci) {
		ppci = list_entry(pos, topo_pci_t, list);

		if (ppci->pci_id == rscid)
			return ppci->pci_name;
	}

	return NULL;
}


/**
 * topo_storage_by_storageid
 *
 * @param
 *	rscid
 * @return
 *
 */
char *
topo_storage_by_storageid(fmd_t *pfmd, uint64_t rscid)
{
	topo_storage_t *pstr;
	struct list_head *pos;
	fmd_topo_t *ptopo = &pfmd->fmd_topo;

	list_for_each(pos, &ptopo->list_storage) {
		pstr = list_entry(pos, topo_storage_t, list);

		if (pstr->storage_id == rscid)
			return pstr->storage_name;
	}

	return NULL;
}
