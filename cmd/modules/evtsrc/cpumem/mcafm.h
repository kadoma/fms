/*
 * mcafm.h
 *
 * Copyright (C) 2009 Inspur, Inc.  All rights reserved.
 * Copyright (C) 2009-2010 Fault Managment System Development Team
 *
 * Created on: Apr 07, 2010
 *      Author: Inspur OS Team
 *  
 * Description:
 *      mcafm.h
 */

#ifndef _MCAFM_H
#define _MCAFM_H

#include "mca_fm.h"

#define CACHE_CHECK		0
#define TLB_CHECK		1
#define BUS_CHECK		2
#define REG_FILE_CHECK		3
#define MS_CHECK		4
#define MEM_DEV_CHECK		5
#define SEL_DEV_CHECK		6
#define PCI_BUS_CHECK		7
#define SMBIOS_DEV_CHECK	8
#define PCI_COMP_CHECK		9
#define PLAT_SPECIFIC_CHECK	10
#define HOST_CTLR_CHECK		11
#define PLAT_BUS_CHECK		12

typedef struct fm_cpuid_info {
	int processor_id;
	int core_id;
	int hyper_thread_id;
} fm_cpuid_info_t;

typedef struct fm_sal_log_timestamp {
	u8 slh_second;		/* Second (0..59) */
	u8 slh_minute;		/* Minute (0..59) */
	u8 slh_hour;		/* Hour (0..23) */
	u8 slh_day;		/* Day (1..31) */
	u8 slh_month;		/* Month (1..12) */
	u8 slh_year;		/* Year (00..99) */
	u8 slh_century;		/* Century (19, 20, 21, ...) */
} fm_sal_log_timestamp_t;

typedef struct fm_cache_check_info {
	fm_cpuid_info_t *fm_cpuid_info;
	fm_sal_log_timestamp_t *fm_sal_log_timestamp;
	u8 mcc;                 /* Machine check corrected */
	u8 level;		/* Cache level */
	u8 dc;			/* Failure in dcache */
	u8 ic;			/* Failure in icache */
	
	u8 dl;                  /* Failure in data part of cache line */
	u8 tl;                  /* Failure in tag part of cache line */
	char *mesi;		/* Cache line state */
	u8 way;			/* Way in which the error occurred */
	u32 index;		/* Cache line index */
	char *op;               /* Type of cache operation that 
				 * caused the machine check. */
	u64 target_addr;	/* Target address */
} fm_cache_check_info_t;

typedef struct fm_tlb_check_info {
	fm_cpuid_info_t *fm_cpuid_info;
	fm_sal_log_timestamp_t *fm_sal_log_timestamp;
	u8 mcc;			/* Machine check corrected */
	u8 level;		/* TLB level */
	u8 dtr;			/* Data Translation Register */
	u8 itr;			/* Instruction Translation Register */
	u8 dtc;			/* Data Translation Cache */
	u8 itc;			/* Instruction Translation Cache */

	char *op;		/* Type of TLB operation that 
				 * caused the machine check. */
	u8 tr_slot;		/* Slot# of TR where error occurred */
} fm_tlb_check_info_t;

typedef struct fm_bus_check_info {
	fm_cpuid_info_t *fm_cpuid_info;
	fm_sal_log_timestamp_t *fm_sal_log_timestamp;
	u8 mcc;			/* Machine check corrected */
	u8 ib;			/* Internal Bus Error */
	u8 eb;			/* External Bus Error */

	u8 size;		/* Transaction size */
	u8 cc;			/* Cache to cache transfer */
	char *type;		/* Type of Bus operation that 
				 * caused the machine check. */
	char *bsi;		/* Status information */
} fm_bus_check_info_t;

typedef struct fm_reg_file_check_info {
	fm_cpuid_info_t *fm_cpuid_info;
	fm_sal_log_timestamp_t *fm_sal_log_timestamp;
	u8 mcc;			/* Machine check corrected */
	u8 reg_num;		/* Register number */

	char *id;		/* Register file identifier */
	char *op;		/* Type of register Operation that 
				 * caused the machine check. */
} fm_reg_file_check_info_t;

typedef struct fm_ms_check_info {
	fm_cpuid_info_t *fm_cpuid_info;
	fm_sal_log_timestamp_t *fm_sal_log_timestamp;
	u8 mcc;			/* Machine check corrected */
	u8 level;		/* Level: */

	u8 way;			/* Way of structure */
	u8 index;
	char *array_id;		/* Array identification */
	char *op;		/* Type of Operation that caused the machine check. */
} fm_ms_check_info_t;

typedef struct fm_mem_dev_check_info {
	fm_sal_log_timestamp_t *fm_sal_log_timestamp;
	u64 error_status;	/* Error Status: */
	u64 physical_addr;	/* Physical Address: */
	u64 addr_mask;		/* Address Mask: */
	u16 node;		/* Node: */
	u16 card;		/* Card: */
	u16 module;		/* Module: */
	u16 bank;		/* Bank: */
	u16 device;		/* Device: */
	u16 row;		/* Row: */
	u16 column;		/* Column: */
	u16 bit_position;	/* Bit Position: */
	u64 requestor_id;	/* Requestor Address: */
	u64 responder_id;	/* Responder Address: */
	u64 target_id;		/* Target Address: */
	u64 bus_spec_data;	/* Bus Specific Data: */
	u8 oem_id[16];		/* OEM Memory Controller ID: */
//	u8 oem_data[1];		/* Variable length data */
} fm_mem_dev_check_info_t;

typedef struct fm_sel_dev_check_info {
	fm_sal_log_timestamp_t *fm_sal_log_timestamp;
	u16 record_id;		/* Record ID: */
	u8 record_type;		/* Record Type: */
	u8 timestamp[4];	/* Time Stamp: */
	u16 generator_id;	/* Generator ID: */
	u8 evm_rev;		/* Message Format Version: */
	u8 sensor_type;		/* Sensor Type: */
	u8 sensor_num;		/* Sensor Number: */
	u8 event_dir;		/* Event Direction Type: */
	u8 event_data1;		/* Data1: */
	u8 event_data2;		/* Data2: */
	u8 event_data3;		/* Data3: */
} fm_sel_dev_check_info_t;

typedef struct fm_pci_bus_check_info {
	fm_sal_log_timestamp_t *fm_sal_log_timestamp;
	u64 err_status;		/* Error Status: */
	u16 err_type;		/* Error Type: */
	u16 bus_id;		/* Bus ID: */
	u64 bus_address;	/* Bus Address: */
	u64 bus_data;		/* Bus Data: */
	u64 bus_cmd;		/* Bus Command: */
	u64 requestor_id;	/* Requestor ID: */
	u64 responder_id;	/* Responder ID: */
	u64 target_id;		/* Target ID: */
//	u8 oem_data[1];		/* Variable length data */
} fm_pci_bus_check_info_t;

typedef struct fm_smbios_dev_check_info {
	fm_sal_log_timestamp_t *fm_sal_log_timestamp;
	u8 event_type;		/* Event Type: */
	u8 time_stamp[6];	/* Time Stamp: */
	u8 data[1];		/* data of variable length, length == slsmb_length */
} fm_smbios_dev_check_info_t;

typedef struct fm_pci_comp_check_info {
	fm_sal_log_timestamp_t *fm_sal_log_timestamp;
	u64 err_status;		/* Error Status: */
	u16 vendor_id;		/* Vendor Id: */
	u16 device_id;		/* Device Id: */
	u8 class_code[3];	/* Class Code: */
	u8 func_num;		/* Func: */
	u8 dev_num;		/* Dev: */
	u8 bus_num;		/* Bus: */
	u8 seg_num;		/* Seg: */
} fm_pci_comp_check_info_t;

typedef struct fm_plat_specific_check_info {
	fm_sal_log_timestamp_t *fm_sal_log_timestamp;
	u64 err_status;		/* Error Status: */
	efi_guid_t guid;	/* GUID: */
//	u8 oem_data[1];		/* platform specific variable length data */
} fm_plat_specific_check_info_t;

typedef struct fm_host_ctlr_check_info {
	fm_sal_log_timestamp_t *fm_sal_log_timestamp;
	u64 err_status;		/* Error Status: */
	u64 requestor_id;	/* Requestor ID: */
	u64 responder_id;	/* Responder ID: */
	u64 target_id;		/* Target ID: */
	u64 bus_spec_data;	/* Bus Specific Data: */
//	u8 oem_data[1];		/* Variable length OEM data */
} fm_host_ctlr_check_info_t;

typedef struct fm_plat_bus_check_info {
	fm_sal_log_timestamp_t *fm_sal_log_timestamp;
	u64 err_status;		/* Error Status: */
	u64 requestor_id;	/* Requestor ID: */
	u64 responder_id;	/* Responder ID: */
	u64 target_id;		/* Target ID: */
	u64 bus_spec_data;	/* Bus Specific Data: */
//	u8 oem_data[1];		/* Variable length OEM data */
} fm_plat_bus_check_info_t;

#endif  /* _MCAFM_H */
