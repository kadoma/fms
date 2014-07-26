/*
 * sal.h
 *
 * Copyright (C) 2009 Inspur, Inc.  All rights reserved.
 * Copyright (C) 2009-2010 Fault Managment System Development Team
 *
 * Created on: Apr 07, 2010
 *      Author: Inspur OS Team
 *  
 * Description:
 *
 * System Abstraction Layer definitions.
 *
 * This is based on version 2.5 of the manual "IA-64 System
 * Abstraction Layer".
 *
 * Copyright (C) 2001 Intel
 * Copyright (C) 2002 Jenna Hall <jenna.s.hall@intel.com>
 * Copyright (C) 2001 Fred Lewis <frederick.v.lewis@intel.com>
 * Copyright (C) 1998, 1999, 2001, 2003 Hewlett-Packard Co
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 * Copyright (C) 1999 Srinivasa Prasad Thirumalachar <sprasad@sprasad.engr.sgi.com>
 *
 * 02/01/04 J. Hall Updated Error Record Structures to conform to July 2001
 *		    revision of the SAL spec.
 * 01/01/03 fvlewis Updated Error Record Structures to conform with Nov. 2000
 *                  revision of the SAL spec.
 * 99/09/29 davidm	Updated for SAL 2.6.
 * 00/03/29 cfleck      Updated SAL Error Logging info for processor (SAL 2.6)
 *                      (plus examples of platform error info structures from smariset @ Intel)
 * 2003-11-05 Strip down to the bare minimum required to decode SAL records.
 *	      Keith Owens <kaos@sgi.com>
 * 2005-12-14 Correct the definition of the severity code and validation bits.
 *	      Keith Owens <kaos@sgi.com>
 */

#ifndef _SAL_H
#define _SAL_H

//#include <asm/fpu.h>
#include "efi.h"
#include "pal.h"
#include "mca_fm.h"

/*
 * Definition of the SAL Error Log from the SAL spec
 */

/* SAL Error Record Section GUID Definitions */
#define SAL_PROC_DEV_ERR_SECT_GUID  \
    EFI_GUID(0xe429faf1, 0x3cb7, 0x11d4, 0xbc, 0xa7, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81)
#define SAL_PLAT_MEM_DEV_ERR_SECT_GUID  \
    EFI_GUID(0xe429faf2, 0x3cb7, 0x11d4, 0xbc, 0xa7, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81)
#define SAL_PLAT_SEL_DEV_ERR_SECT_GUID  \
    EFI_GUID(0xe429faf3, 0x3cb7, 0x11d4, 0xbc, 0xa7, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81)
#define SAL_PLAT_PCI_BUS_ERR_SECT_GUID  \
    EFI_GUID(0xe429faf4, 0x3cb7, 0x11d4, 0xbc, 0xa7, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81)
#define SAL_PLAT_SMBIOS_DEV_ERR_SECT_GUID  \
    EFI_GUID(0xe429faf5, 0x3cb7, 0x11d4, 0xbc, 0xa7, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81)
#define SAL_PLAT_PCI_COMP_ERR_SECT_GUID  \
    EFI_GUID(0xe429faf6, 0x3cb7, 0x11d4, 0xbc, 0xa7, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81)
#define SAL_PLAT_SPECIFIC_ERR_SECT_GUID  \
    EFI_GUID(0xe429faf7, 0x3cb7, 0x11d4, 0xbc, 0xa7, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81)
#define SAL_PLAT_HOST_CTLR_ERR_SECT_GUID  \
    EFI_GUID(0xe429faf8, 0x3cb7, 0x11d4, 0xbc, 0xa7, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81)
#define SAL_PLAT_BUS_ERR_SECT_GUID  \
    EFI_GUID(0xe429faf9, 0x3cb7, 0x11d4, 0xbc, 0xa7, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81)

struct ia64_fpreg {
	union {
		unsigned long bits[2];
		long double __dummy;    /* force 16-byte alignment */
	} u;
};

/* Definition of version  according to SAL spec for logging purposes */
typedef struct sal_log_revision {
	u8 minor;		/* BCD (0..99) */
	u8 major;		/* BCD (0..99) */
} sal_log_revision_t;

/* Definition of timestamp according to SAL spec for logging purposes */
typedef struct sal_log_timestamp {
	u8 slh_second;		/* Second (0..59) */
	u8 slh_minute;		/* Minute (0..59) */
	u8 slh_hour;		/* Hour (0..23) */
	u8 slh_reserved;
	u8 slh_day;		/* Day (1..31) */
	u8 slh_month;		/* Month (1..12) */
	u8 slh_year;		/* Year (00..99) */
	u8 slh_century;		/* Century (19, 20, 21, ...) */
} sal_log_timestamp_t;

/* Definition of log record  header structures */
typedef struct sal_log_record_header {
	u64 id;				/* Unique monotonically increasing ID */
	sal_log_revision_t revision;	/* Major and Minor revision of header */
	u8 severity;			/* Error Severity */
	u8 validation_bits;;		/* Validation Bits */
	u32 len;			/* Length of this error log in bytes */
	sal_log_timestamp_t timestamp;	/* Timestamp */
	efi_guid_t platform_guid;	/* Unique OEM Platform ID */
} sal_log_record_header_t;

/* Definition of log section header structures */
typedef struct sal_log_sec_header {
	efi_guid_t guid;		/* Unique Section ID */
	sal_log_revision_t revision;	/* Major and Minor revision of Section */
	u16 reserved;
	u32 len;			/* Section length */
} sal_log_section_hdr_t;

typedef struct sal_log_mod_error_info {
	struct {
		u64 check_info              : 1,
		    requestor_identifier    : 1,
		    responder_identifier    : 1,
		    target_identifier       : 1,
		    precise_ip              : 1,
		    reserved                : 59;
	} valid;
	u64 check_info;
	u64 requestor_identifier;
	u64 responder_identifier;
	u64 target_identifier;
	u64 precise_ip;
} sal_log_mod_error_info_t;

typedef struct sal_processor_static_info {
	struct {
		u64 minstate        : 1,
		    br              : 1,
		    cr              : 1,
		    ar              : 1,
		    rr              : 1,
		    fr              : 1,
		    reserved        : 58;
	} valid;
	pal_min_state_area_t min_state_area;
	u64 br[8];
	u64 cr[128];
	u64 ar[128];
	u64 rr[8];
	struct ia64_fpreg fr[128];
} sal_processor_static_info_t;

struct sal_cpuid_info {
	u64 regs[5];
	u64 reserved;
};

typedef struct sal_log_processor_info {
	sal_log_section_hdr_t header;
	struct {
		u64 proc_error_map      : 1,
		    proc_state_param    : 1,
		    proc_cr_lid         : 1,
		    psi_static_struct   : 1,
		    num_cache_check     : 4,
		    num_tlb_check       : 4,
		    num_bus_check       : 4,
		    num_reg_file_check  : 4,
		    num_ms_check        : 4,
		    cpuid_info          : 1,
		    reserved1           : 39;
	} valid;
	u64 proc_error_map;
	u64 proc_state_parameter;
	u64 proc_cr_lid;
	/*
	 * The rest of this structure consists of variable-length arrays, which can't be
	 * expressed in C.
	 */
	sal_log_mod_error_info_t info[0];
	/*
	 * This is what the rest looked like if C supported variable-length arrays:
	 *
	 * sal_log_mod_error_info_t cache_check_info[.valid.num_cache_check];
	 * sal_log_mod_error_info_t tlb_check_info[.valid.num_tlb_check];
	 * sal_log_mod_error_info_t bus_check_info[.valid.num_bus_check];
	 * sal_log_mod_error_info_t reg_file_check_info[.valid.num_reg_file_check];
	 * sal_log_mod_error_info_t ms_check_info[.valid.num_ms_check];
	 * struct sal_cpuid_info cpuid_info;
	 * sal_processor_static_info_t processor_static_info;
	 */
} sal_log_processor_info_t;

/* Given a sal_log_processor_info_t pointer, return a pointer to the processor_static_info: */
#define SAL_LPI_PSI_INFO(l)									\
({	sal_log_processor_info_t *_l = (l);							\
	((sal_processor_static_info_t *)							\
	 ((char *) _l->info + ((_l->valid.num_cache_check + _l->valid.num_tlb_check		\
				+ _l->valid.num_bus_check + _l->valid.num_reg_file_check	\
				+ _l->valid.num_ms_check) * sizeof(sal_log_mod_error_info_t)	\
			       + sizeof(struct sal_cpuid_info))));				\
})

/* platform error log structures */

typedef struct sal_log_mem_dev_err_info {
	sal_log_section_hdr_t header;
	struct {
		u64 error_status    : 1,
		    physical_addr   : 1,
		    addr_mask       : 1,
		    node            : 1,
		    card            : 1,
		    module          : 1,
		    bank            : 1,
		    device          : 1,
		    row             : 1,
		    column          : 1,
		    bit_position    : 1,
		    requestor_id    : 1,
		    responder_id    : 1,
		    target_id       : 1,
		    bus_spec_data   : 1,
		    oem_id          : 1,
		    oem_data        : 1,
		    reserved        : 47;
	} valid;
	u64 error_status;
	u64 physical_addr;
	u64 addr_mask;
	u16 node;
	u16 card;
	u16 module;
	u16 bank;
	u16 device;
	u16 row;
	u16 column;
	u16 bit_position;
	u64 requestor_id;
	u64 responder_id;
	u64 target_id;
	u64 bus_spec_data;
	u8 oem_id[16];
	u8 oem_data[1];			/* Variable length data */
} sal_log_mem_dev_err_info_t;

typedef struct sal_log_sel_dev_err_info {
	sal_log_section_hdr_t header;
	struct {
		u64 record_id       : 1,
		    record_type     : 1,
		    generator_id    : 1,
		    evm_rev         : 1,
		    sensor_type     : 1,
		    sensor_num      : 1,
		    event_dir       : 1,
		    event_data1     : 1,
		    event_data2     : 1,
		    event_data3     : 1,
		    reserved        : 54;
	} valid;
	u16 record_id;
	u8 record_type;
	u8 timestamp[4];
	u16 generator_id;
	u8 evm_rev;
	u8 sensor_type;
	u8 sensor_num;
	u8 event_dir;
	u8 event_data1;
	u8 event_data2;
	u8 event_data3;
} sal_log_sel_dev_err_info_t;

typedef struct sal_log_pci_bus_err_info {
	sal_log_section_hdr_t header;
	struct {
		u64 err_status      : 1,
		    err_type        : 1,
		    bus_id          : 1,
		    bus_address     : 1,
		    bus_data        : 1,
		    bus_cmd         : 1,
		    requestor_id    : 1,
		    responder_id    : 1,
		    target_id       : 1,
		    oem_data        : 1,
		    reserved        : 54;
	} valid;
	u64 err_status;
	u16 err_type;
	u16 bus_id;
	u32 reserved;
	u64 bus_address;
	u64 bus_data;
	u64 bus_cmd;
	u64 requestor_id;
	u64 responder_id;
	u64 target_id;
	u8 oem_data[1];			/* Variable length data */
} sal_log_pci_bus_err_info_t;

typedef struct sal_log_smbios_dev_err_info {
	sal_log_section_hdr_t header;
	struct {
		u64 event_type      : 1,
		    length          : 1,
		    time_stamp      : 1,
		    data            : 1,
		    reserved1       : 60;
	} valid;
	u8 event_type;
	u8 length;
	u8 time_stamp[6];
	u8 data[1];			/* data of variable length, length == slsmb_length */
} sal_log_smbios_dev_err_info_t;

typedef struct sal_log_pci_comp_err_info {
	sal_log_section_hdr_t header;
	struct {
		u64 err_status      : 1,
		    comp_info       : 1,
		    num_mem_regs    : 1,
		    num_io_regs     : 1,
		    reg_data_pairs  : 1,
		    oem_data        : 1,
		    reserved        : 58;
	} valid;
	u64 err_status;
	struct {
		u16 vendor_id;
		u16 device_id;
		u8 class_code[3];
		u8 func_num;
		u8 dev_num;
		u8 bus_num;
		u8 seg_num;
		u8 reserved[5];
	} comp_info;
	u32 num_mem_regs;
	u32 num_io_regs;
	u64 reg_data_pairs[1];
	/*
	 * array of address/data register pairs is num_mem_regs + num_io_regs elements
	 * long.  Each array element consists of a u64 address followed by a u64 data
	 * value.  The oem_data array immediately follows the reg_data_pairs array
	 */
	u8 oem_data[1];			/* Variable length data */
} sal_log_pci_comp_err_info_t;

typedef struct sal_log_plat_specific_err_info {
	sal_log_section_hdr_t header;
	struct {
		u64 err_status      : 1,
		    guid            : 1,
		    oem_data        : 1,
		    reserved        : 61;
	} valid;
	u64 err_status;
	efi_guid_t guid;
	u8 oem_data[1];			/* platform specific variable length data */
} sal_log_plat_specific_err_info_t;

typedef struct sal_log_host_ctlr_err_info {
	sal_log_section_hdr_t header;
	struct {
		u64 err_status      : 1,
		    requestor_id    : 1,
		    responder_id    : 1,
		    target_id       : 1,
		    bus_spec_data   : 1,
		    oem_data        : 1,
		    reserved        : 58;
	} valid;
	u64 err_status;
	u64 requestor_id;
	u64 responder_id;
	u64 target_id;
	u64 bus_spec_data;
	u8 oem_data[1];			/* Variable length OEM data */
} sal_log_host_ctlr_err_info_t;

typedef struct sal_log_plat_bus_err_info {
	sal_log_section_hdr_t header;
	struct {
		u64 err_status      : 1,
		    requestor_id    : 1,
		    responder_id    : 1,
		    target_id       : 1,
		    bus_spec_data   : 1,
		    oem_data        : 1,
		    reserved        : 58;
	} valid;
	u64 err_status;
	u64 requestor_id;
	u64 responder_id;
	u64 target_id;
	u64 bus_spec_data;
	u8 oem_data[1];			/* Variable length OEM data */
} sal_log_plat_bus_err_info_t;

/* Overall platform error section structure */
typedef union sal_log_platform_err_info {
	sal_log_mem_dev_err_info_t mem_dev_err;
	sal_log_sel_dev_err_info_t sel_dev_err;
	sal_log_pci_bus_err_info_t pci_bus_err;
	sal_log_smbios_dev_err_info_t smbios_dev_err;
	sal_log_pci_comp_err_info_t pci_comp_err;
	sal_log_plat_specific_err_info_t plat_specific_err;
	sal_log_host_ctlr_err_info_t host_ctlr_err;
	sal_log_plat_bus_err_info_t plat_bus_err;
} sal_log_platform_err_info_t;

/* SAL log over-all, multi-section error record structure (processor+platform) */
typedef struct err_rec {
	sal_log_record_header_t sal_elog_header;
	sal_log_processor_info_t proc_err;
	sal_log_platform_err_info_t plat_err;
	u8 oem_data_pad[1024];
} ia64_err_rec_t;

#endif /* _ASM_IA64_PAL_H */
