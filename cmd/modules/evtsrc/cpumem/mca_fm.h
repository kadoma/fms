/*
 * mca_fm.h
 *
 * Copyright (C) 2009 Inspur, Inc.  All rights reserved.
 * Copyright (C) 2009-2010 Fault Managment System Development Team
 *
 * Created on: Apr 07, 2010
 *      Author: Inspur OS Team
 *  
 * Description:
 * 	Machine check handling specific defines
 *
 * Copyright (C) 1999 Silicon Graphics, Inc.
 * Copyright (C) Vijay Chander (vijay@engr.sgi.com)
 * Copyright (C) Srinivasa Thirumalachar (sprasad@engr.sgi.com)
 * 2003-11-05 Strip down to the bare minimum required to decode SAL records.
 *	      Keith Owens <kaos@sgi.com>
 * 2003-11-16 Break out oem data decoder so each platform can handle the
 *	      oem data as it likes.
 *	      Keith Owens <kaos@sgi.com>
 */

#ifndef _MCA_FM_H
#define _MCA_FM_H

#include <fmd_list.h>

/* Minimal declarations so we can use the kernel definitions of SAL records */
#include <stdint.h>
typedef uint8_t  u8;
typedef int8_t   s8;
typedef uint16_t u16;
typedef int16_t  s16;
typedef uint32_t u32;
typedef int32_t  s32;
typedef uint64_t u64;
typedef int64_t  s64;

#include "sal.h"

#define platform_mem_dev_err_print prt_oem_data
#define platform_pci_bus_err_print prt_oem_data
#define platform_pci_comp_err_print prt_oem_data
#define platform_plat_specific_err_print prt_oem_data
#define platform_host_ctlr_err_print prt_oem_data
#define platform_plat_bus_err_print prt_oem_data

#define FM_EREPORT_CPUMEM_CACHE_ERR		"cache-error"
#define FM_EREPORT_CPUMEM_TLB_ERR     		"tlb-error"
#define FM_EREPORT_CPUMEM_BUS_ERR     		"bus-error"
#define FM_EREPORT_CPUMEM_REG_FILE_ERR  	"reg-file-error"
#define FM_EREPORT_CPUMEM_MS_ERR     		"ms-error"
#define FM_EREPORT_CPUMEM_MEM_DEV_ERR   	"mem-dev-error"
#define FM_EREPORT_CPUMEM_SEL_DEV_ERR   	"sel-dev-error"
#define FM_EREPORT_CPUMEM_PCI_BUS_ERR   	"pci-bus-error"
#define FM_EREPORT_CPUMEM_SMBIOS_DEV_ERR     	"smbios-dev-error"
#define FM_EREPORT_CPUMEM_PCI_COMP_ERR     	"pci-comp-error"
#define FM_EREPORT_CPUMEM_PLAT_SPECIFIC_ERR     "plat-specific-error"
#define FM_EREPORT_CPUMEM_HOST_CTLR_ERR     	"host-ctlr-error"
#define FM_EREPORT_CPUMEM_PLAT_BUS_ERR     	"plat-bus-error"

typedef struct cpumem_monitor {
	nvlist_t *cache_err;
	nvlist_t *tlb_err;
	nvlist_t *bus_err;
	nvlist_t *reg_file_err;
	nvlist_t *ms_err;
	nvlist_t *mem_dev_err;
	nvlist_t *sel_dev_err;
	nvlist_t *pci_bus_err;
	nvlist_t *smbios_dev_err;
	nvlist_t *pci_comp_err;
	nvlist_t *plat_specific_err;
	nvlist_t *host_ctlr_err;
	nvlist_t *plat_bus_err;
	struct list_head *faults;
} cpumem_monitor_t;

extern void platform_info_check (cpumem_monitor_t *cmp, sal_log_record_header_t *lh);

#endif /* _MCA_FM_H */
