/************************************************************
 * Copyright (C) inspur Inc. <http://www.inspur.com>
 * FileName:    fmd_topo.c
 * Author:      Inspur OS Team
                wang.leibj@inspur.com
 * Date:        2015-08-14
 * Description: the topo function
 *
 ************************************************************/
#include <stdio.h>
#include <ctype.h>
#include <pci.h>
#include <dmi.h>
#include <cpu.h>
#include <fmd.h>
#include <logging.h>
#include <fmd_errno.h>

/* global defs */
fmd_topo_t *ptopo;

/**
 * _print_cpu_topo
 *
 *
 * @param
 * @return
 */
void
_print_cpu_topo(fmd_topo_t *pptopo)
{
    struct list_head *pos = NULL;
    topo_cpu_t *pcpu = NULL;

    /* traverse cpu */
    printf("            CPU   \n");
    printf("node  socket  core  thread\n");
    list_for_each(pos, &pptopo->list_cpu) {
        pcpu = list_entry(pos, topo_cpu_t, list);
        printf("%3d%7d%7d%7d\n", pcpu->cpu_chassis,
            pcpu->cpu_socket, pcpu->cpu_core, pcpu->cpu_thread);
    } /* list_for_each */
}

/**
 * _print_pci_topo
 *
 *
 * @param
 * @return
 */
#if 0
void
_print_pci_topo(fmd_topo_t *pptopo)
{
    struct list_head *pos = NULL;
    topo_pci_t *ppci = NULL;

    /* traverse pci */
    printf("----    PCI(e)    ----\n");
    list_for_each(pos, &pptopo->list_pci) {
        ppci = list_entry(pos, topo_pci_t, list);

        printf("%04x:%02x:%02x.%1x-%04x:%02x:%02x.%1x", ppci->pci_chassis, ppci->pci_hostbridge,
            ppci->pci_slot, ppci->pci_func, ppci->pci_subdomain, ppci->pci_subbus,
            ppci->pci_subslot, ppci->pci_subfunc);

        if (ppci->pci_name != NULL) {
            printf(" name:%s\n", ppci->pci_name);
        } else
            printf("\n");
    }
}
#endif
/**
 *_print_mem_topo
 *
 *
 *@param
 *@return
 */
void
_print_mem_topo(fmd_topo_t *pptopo)
{
    struct list_head *pos = NULL;
    topo_mem_t *pmem = NULL;

    /* traverse mem */
    printf("           MEMORY \n");
    printf("node  socket  controller  dimm    size\n");
    list_for_each(pos,&pptopo->list_mem){
        pmem = list_entry(pos,topo_mem_t,list);

        printf("  %-7d%-9d%-9d%-7d%ld\n",pmem->mem_chassis,
            pmem->mem_socket,pmem->mem_controller,pmem->mem_dimm,pmem->end-pmem->start);
    }/* list_for_each */
}

/**
 *_print_disk_top
 *
 *
 * @param
 * @return
 */
 void
_print_disk_topo(fmd_topo_t *pptopo)
{
    struct list_head *pos = NULL;
    topo_storage_t *pstr = NULL;

    /* traverse storage */
    printf("            DISK\n");
    list_for_each(pos,&pptopo->list_storage){
        pstr = list_entry(pos,topo_storage_t,list);

        printf("%04x:%02x:%02x.%1x-%04x:%02x:%02x.%1x", pstr->storage_chassis, pstr->storage_hostbridge,
                        pstr->storage_slot, pstr->storage_func, pstr->storage_host, pstr->storage_channel,
                        pstr->storage_target, pstr->storage_lun);

        if (pstr->storage_name != NULL) {
                        printf("  name:%s\n", pstr->storage_name);
                } else
                        printf("\n");
    }
}

/**
 * topo_stat
 *
 * @param
 * @return
 */
void
_stat_topo(void)
{
#if 0
    struct list_head *pos = NULL, *ppos = NULL;
    topo_cpu_t *pcpu = NULL;
    topo_pci_t *ppci = NULL;

    /* traverse cpu */
    list_for_each(pos, &ptopo->list_cpu) {
        pcpu = list_entry(pos, topo_cpu_t, list);
                pcpu->cpu_socket, pcpu->cpu_core, pcpu->cpu_thread);
    } /* list_for_each */

    /* traverse */
    list_for_each(pos, &ptopo->list_pci) {
        ppci = list_entry(pos, topo_pci_t, list);
            ppci->pci_slot, ppci->pci_func);

        if(ppci->pci_subbus != (uint8_t)-1) {
                ppci->pci_subdomain, ppci->pci_subbus);
        } else
    }
#endif
}


/**
 * _fmd_topo
 *
 * @param
 * @return
 *
 * @desc
 *     entry point
 */
int
_fmd_topo(fmd_t *fmd)
{
    int ret;
    struct list_head *pos, *ppos;
    topo_cpu_t *pcpu = NULL, *ppcpu = NULL;
    ptopo = &fmd->fmd_topo;
    /* initialization */
    INIT_LIST_HEAD(&ptopo->list_cpu);
    INIT_LIST_HEAD(&ptopo->list_mem);
    INIT_LIST_HEAD(&ptopo->list_pci);
    INIT_LIST_HEAD(&ptopo->list_pci_host);
    INIT_LIST_HEAD(&ptopo->list_pci_bridge);
    INIT_LIST_HEAD(&ptopo->list_storage);
    ptopo->tp_root = NULL;

    /**
     * CPU
     * TOPO
     */
    ret = fmd_topo_cpu(DIR_SYS_CPU, "node", ptopo);
    if(ret < 0) {
/* Only for DEBUG */
/* Maybe SMP-Arch */
        fmd_topo_walk_cpu("/sys/devices/system/cpu", 0, ptopo);
    }

    /* setting up thread id */
    list_for_each(pos, &ptopo->list_cpu) {
        pcpu = list_entry(pos, topo_cpu_t, list);

        list_for_each(ppos, &ptopo->list_cpu) {
            ppcpu = list_entry(ppos, topo_cpu_t, list);
            if(ppcpu == pcpu || pcpu->flag == 1)
                continue;
            if(cpu_sibling(pcpu, ppcpu)) {
                if(ppcpu->processor > pcpu->processor) {
                    ppcpu->cpu_thread = 1;
                } else {
                    pcpu->cpu_thread = 1;
                }
                pcpu->flag = 1;
                ppcpu->flag = 1;
                break;
            } /* cpu_sibling */
        } /* list_for_each */
    } /* list_for_each */

    /**
     * MEMORY
     * TOPO
     */
    ret = fmd_topo_dmi(ptopo);
    if(ret < 0) {
        wr_log("",WR_LOG_ERROR,"Failed to get memory topology info.\n");
        return ret;
    }

    /**
     * PCI
     * TOPO
     */
    ret = fmd_topo_pci(DIR_SYS_PCI, ptopo);
    if(ret < 0) {
        wr_log("",WR_LOG_ERROR,"Failed to get pci topology info.\n");
        return ret;
    }

    return FMD_SUCCESS;
}


/**
 * _update_topo
 *
 * @param
 * @return
 *
 * @desc
 *    entry point
 */
int
_update_topo(fmd_t *fmd)
{
    int ret;

    /* FIXME: before this, need to release memory. */
    memset(&fmd->fmd_topo, 0, sizeof (fmd_topo_t));

    if ((ret = _fmd_topo(fmd)) == 0)
        return FMD_SUCCESS;
    else
        return ret;
}
