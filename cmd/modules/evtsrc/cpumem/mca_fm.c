/*
 * mca_fm.c
 *
 * Copyright (C) 2009 Inspur, Inc.  All rights reserved.
 * Copyright (C) 2009-2010 Fault Managment System Development Team
 *
 * Created on: Apr 07, 2010
 *      Author: Inspur OS Team
 *  
 * Description:
 * 	Generic MCA handling layer
 */

#include <errno.h>
#include <stdarg.h>
#include <signal.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <sys/poll.h>

#include <protocol.h>
#include <nvpair.h>
#include "mca_fm.h"
//#include "sal.h"
#include "mcafm.h"

#define ARRAY_SIZE(x) (sizeof((x))/sizeof((x)[0]))

#if 0
int debug;

struct decode_bits {
	int start;
	int length;
	const char *name;
	const char *y;
	const char *n;
};

struct reg_fmt {
	char * const name;
	int offset;
};

static int indent;

static sal_log_record_header_t *decode_oem_record_start;
static int decode_oem_fd_data;
static int decode_oem_use_sal;
static int decode_oem_cpu;
static int *decode_oem_oemdata_fd;

/* Equivalent of printf(), with automatic indentation at the start of each line */
static int iprintf(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
static int iprintf_count;

static int
iprintf(const char *fmt, ...)
{
	char buf[1000000];	/* the decoded oem data from the kernel is one big print */
	char c, *p, *p1;
	va_list args;
	int len;
	static int startofline = 1;
	va_start(args, fmt);
	len = vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	p = buf;
	do {
		if (startofline) {
			int i;
			for (i = 0; i < indent; ++i)
				fputs("  ", stdout);
			startofline = 0;
			if (++iprintf_count > 1000000 /* arbitrary */) {
				printf("\n\nError: iprintf line count exceeded, aborting output\n\n");
				fflush(stdout);
				exit(1);
			}
		}
		p1 = strchr(p, '\n');
		if (p1) {
			++p1;
			c = *p1;
			*p1 = '\0';
		} else
			c = '\0';
		fputs(p, stdout);
		if (p1) {
			startofline = 1;
			p = p1;
		}
		*p = c;
	} while (*p);
	return len;
}

/* Standard format for string plus value */
static int
prval(const char *string, u64 val)
{
	return iprintf("%-25s: 0x%016lx\n", string, val);
}


/* Print a formatted GUID. */
static void
prt_guid (efi_guid_t *p_guid)
{
	char out[40];
	iprintf("GUID = %s\n", efi_guid_unparse(p_guid, out));
}

static void
hexdump (const unsigned char *p, unsigned long n_ch)
{
	int i, j;
	if (!p)
		return;
	for (i = 0; i < n_ch;) {
		iprintf("%p ", (void *)p);
		for (j = 0; (j < 16) && (i < n_ch); i++, j++, p++) {
			iprintf("%02x ", *p);
		}
		iprintf("\n");
	}
}


static void
prt_record_header (sal_log_record_header_t *rh)
{
	iprintf("SAL RECORD HEADER:  Record buffer = %p,  header size = %ld\n",
	       (void *)rh, sizeof(sal_log_record_header_t));
	++indent;
	hexdump((unsigned char *)rh, sizeof(sal_log_record_header_t));
	iprintf("Total record length = %d\n", rh->len);
	prt_guid(&rh->platform_guid);
	--indent;
	iprintf("End of SAL RECORD HEADER\n");
}

static void
prt_section_header (sal_log_section_hdr_t *sh)
{
	iprintf("SAL SECTION HEADER:  Record buffer = %p,  header size = %ld\n",
	       (void *)sh, sizeof(sal_log_section_hdr_t));
	++indent;
	hexdump((unsigned char *)sh, sizeof(sal_log_section_hdr_t));
	iprintf("Length of section & header = %d\n", sh->len);
	prt_guid(&sh->guid);
	--indent;
	iprintf("End of SAL SECTION HEADER\n");
}
#endif

/* Check the machine check information related to cache error(s) */
static void
cache_check_info_check (cpumem_monitor_t *cmp, int i, 
		sal_log_mod_error_info_t *cache_check_info, 
		fm_cpuid_info_t *cpuid, fm_sal_log_timestamp_t *time)
{
	char fullclass[PATH_MAX];
	char *faultname;
//	nvlist_t *nvl = cmp->cache_err;

	pal_mc_error_info_t check_info = { cache_check_info->check_info };
	pal_cache_check_info_t *info = &check_info.pme_cache;
	static char * op[] = {
		"Unknown/unclassified",			/*  0 */
		"Load",
		"Store",
		"Instruction fetch or prefetch",
		"Data fetch",
		"Snoop",				/*  5 */
		"Cast out",
		"Move in",
	};
	static char * mesi[] = {
		"Invalid",
		"Shared",
		"Exclusive",
		"Modified",
	};

	if (!cache_check_info->valid.check_info) {
//		iprintf("%s: invalid cache_check_info[%d]\n", __FUNCTION__, i);
		return;
	}
	
	struct fm_cache_check_info *fm_info =
		(struct fm_cache_check_info*)malloc(sizeof(struct fm_cache_check_info));

	fm_info->level = info->level;

	if (info->op < ARRAY_SIZE(op)) {	//Operation:
		fm_info->op = op[info->op];
	} else {
		fm_info->op = "reserved";
	}

	if (info->mv) {
		if (info->mesi < ARRAY_SIZE(mesi)) {	//Mesi:
			fm_info->mesi = mesi[info->mesi];
		} else {
			fm_info->mesi = "reserved";
		}
	}

	fm_info->index = info->index;           //Cache line index
	if (info->wiv)
		fm_info->way = info->way;	//Way:
	if (info->ic)
		fm_info->ic = info->ic;		//Cache: Instruction
	if (info->dc)
		fm_info->dc = info->dc;		//Cache: Data
	if (info->tl)
		fm_info->tl = info->tl; 	//Line: Tag
	if (info->dl)
		fm_info->dl = info->dl;		//Line: Data
	if (cache_check_info->valid.target_identifier)
		/* Hope target address is saved in target_identifier */
		if (info->tv)
			fm_info->target_addr = cache_check_info->target_identifier;
				/* Target Addr: 0x%lx,", target_addr */
	if (info->mcc) {
		fm_info->mcc = info->mcc;		//MC: Corrected
		faultname = "cache";
	} else 
		faultname = "cache_uc";

	fm_info->fm_cpuid_info = cpuid;
	fm_info->fm_sal_log_timestamp = time;

	(void) snprintf(fullclass, sizeof(fullclass), "%s.cpu.intel.l%d%s",
		FM_EREPORT_CLASS, info->level, faultname);

	cpumem_fm_event_post(cmp->cache_err, fullclass, cpuid->processor_id, -1);
	printf("cpumem_fm_event_post(fm_info, CACHE_CHECK);\n");
}

/* Check the machine check information related to tlb error(s) */
static void
tlb_check_info_check (cpumem_monitor_t *cmp, int i, 
		sal_log_mod_error_info_t *tlb_check_info, 
		fm_cpuid_info_t *cpuid, fm_sal_log_timestamp_t *time)

{
	char fullclass[PATH_MAX];
	char *faultname;
	char *tlb;
//	nvlist_t *nvl = cmp->tlb_err;

	pal_mc_error_info_t check_info = { tlb_check_info->check_info };
	pal_tlb_check_info_t *info = &check_info.pme_tlb;
//	static const char * const op[] = {
	static char * op[] = {
		"Unknown/unclassified",			/*  0 */
		"Load",
		"Store",
		"Instruction fetch or prefetch",
		"Data fetch",
		"TLB shoot down",			/*  5 */
		"TLB probe instruction",
		"Move in (VHPT fill)",
		"Purge",
	};

	if (!tlb_check_info->valid.check_info) {
//		iprintf("%s: invalid tlb_check_info[%d]\n", __FUNCTION__, i);
		return;                 /* If check info data not valid, skip it */
	}

	struct fm_tlb_check_info *fm_info =
		(struct fm_tlb_check_info*)malloc(sizeof(struct fm_tlb_check_info));
	
	if (info->trv)
		fm_info->tr_slot = info->tr_slot;	//Slot# of TR where error occurred
	fm_info->level = info->level;			//TLB Level
	if (info->dtr) {
		fm_info->dtr = info->dtr;		//Data Translation Register
		tlb = "d";
	}
	if (info->itr) {
		fm_info->itr = info->itr;		//Instruction Translation Register
		tlb = "i";
	}
	if (info->dtc) {
		fm_info->dtr = info->dtc;		//Data Translation Cache
		tlb = "d";
	}
	if (info->itc) {
		fm_info->itc = info->itc;		//Instruction Translation Cache
		tlb = "i";
	}
	if (info->op < ARRAY_SIZE(op)) {		//Cache Operation:
		fm_info->op = op[info->op];
	} else {
		fm_info->op = "reserved";
	}

	if (info->mcc) {
		fm_info->mcc = info->mcc;		//MC: Corrected
		faultname = "tlb";
	} else
		faultname = "tlb_uc";

	fm_info->fm_cpuid_info = cpuid;
	fm_info->fm_sal_log_timestamp = time;

	(void) snprintf(fullclass, sizeof(fullclass), "%s.cpu.intel.l%d%s%s",
		FM_EREPORT_CLASS, info->level, tlb, faultname);

	cpumem_fm_event_post(cmp->tlb_err, fullclass, cpuid->processor_id, -1);
	printf("cpumem_fm_event_post(fm_info, TLB_CHECK);\n");
}

/* Check the machine check information related to bus error(s) */
static void
bus_check_info_check (cpumem_monitor_t *cmp, int i, 
		sal_log_mod_error_info_t *bus_check_info, 
		fm_cpuid_info_t *cpuid, fm_sal_log_timestamp_t *time)
{
	char fullclass[PATH_MAX];
        char *faultname;
//      nvlist_t *nvl = cmp->bus_err;
	
	pal_mc_error_info_t check_info = { bus_check_info->check_info };
	pal_bus_check_info_t *info = &check_info.pme_bus;
//	static const char * const type[] = {
	static char * type[] = {
		"Unknown/unclassified",			/*  0 */
		"Partial read",
		"Partial write",
		"Full line read",
		"Full line write",
		"Implicit or explicit write-back",	/*  5 */
		"Snoop probe",
		"ptc.g",
		"Write coalescing",
		"I/O space read",
		"I/O space write",			/* 10 */
		"IPI",
		"Interrupt acknowledge",
	};
	/* FIXME: Are these SGI specific or generic bsi values? */
//	static const char * const bsi[] = {
	static char * bsi[] = {
		"Unknown/unclassified",			/*  0 */
		"Berr",
		"BINIT",
		"Hard fail",
	};

	if (!bus_check_info->valid.check_info) {
//		iprintf("%s: invalid bus_check_info[%d]\n", __FUNCTION__, i);
		return;                 /* If check info data not valid, skip it */
	}

	struct fm_bus_check_info *fm_info =
		(struct fm_bus_check_info*)malloc(sizeof(struct fm_bus_check_info));

	fm_info->size = info->size;		//Transaction size: %d", info->size
	if (info->ib && info->mcc && (info->type == 9 || info->type == 10) ) {
		fm_info->ib = info->ib;		//Internal Bus Error
		faultname = "bus_interconnect_io";
	}
	if (info->ib && !info->mcc && (info->type == 9 || info->type == 10) ) {
		faultname = "bus_interconnect_io_uc";
	}
	if (info->ib && info->mcc && (info->type >= 1 && info->type <= 8) ) {
		faultname = "bus_interconnect_memory";
	}
	if (info->ib && !info->mcc && (info->type >= 1 && info->type <= 8) ) {
		faultname = "bus_interconnect_memory_uc";
	}
	if (info->eb && info->mcc) {
		fm_info->eb = info->eb;		//External Bus Error
		faultname = "bus_exterconnect";
	}
	if (info->eb && !info->mcc) {
		faultname = "bus_exterconnect_uc";
	}
	if (info->cc)
		fm_info->cc = info->cc;		//Cache to cache transfer
	if (info->type < ARRAY_SIZE(type)) {
		fm_info->type = type[info->type];	//Type:
	} else {
		fm_info->type = "reserved";
	}
	
	if (info->bsi < ARRAY_SIZE(bsi)) {	//Status information
		fm_info->bsi = bsi[info->bsi];
	} else {
		fm_info->bsi = "processor specific";
	}
	
	if (info->mcc)
		fm_info->mcc = info->mcc;	//MC: Corrected

	fm_info->fm_cpuid_info = cpuid;
	fm_info->fm_sal_log_timestamp = time;

	(void) snprintf(fullclass, sizeof(fullclass), "%s.cpu.intel.%s",
	        FM_EREPORT_CLASS, faultname);

	cpumem_fm_event_post(cmp->bus_err, fullclass, cpuid->processor_id, -1);
	printf("cpumem_fm_event_post(fm_info, BUS_CHECK);\n");
}

/* Check the machine check information related to reg_file error(s) */
static void
reg_file_check_info_check (cpumem_monitor_t *cmp, int i, 
		sal_log_mod_error_info_t *reg_file_check_info, 
		fm_cpuid_info_t *cpuid, fm_sal_log_timestamp_t *time)
{
	char fullclass[PATH_MAX];
        char *faultname;
//      nvlist_t *nvl = cmp->reg_file_err;

	pal_mc_error_info_t check_info = { reg_file_check_info->check_info };
	pal_reg_file_check_info_t *info = &check_info.pme_reg_file;
//	static const char * const id[] = {
	static char * id[] = {
		"Unknown/unclassified",			/*  0 */
		"General register (bank 1)",
		"General register (bank 0)",
		"Floating point register",
		"Branch register",
		"Predicate register",			/*  5 */
		"Application register",
		"Control register",
		"Region register",
		"Protection key register",
		"Data breakpoint register",		/* 10 */
		"Instruction breakpoint register",
		"Performance monitor control register",
		"Performance monitor data register",
	};
//	static const char * const op[] = {
	static char * op[] = {
		"Unknown",
		"Read",
		"Write",
	};

	if (!reg_file_check_info->valid.check_info) {
//		iprintf("%s: invalid reg_file_check_info[%d]\n", __FUNCTION__, i);
		return;                 /* If check info data not valid, skip it */
	}

	struct fm_reg_file_check_info *fm_info =
		(struct fm_reg_file_check_info*)malloc(sizeof(struct fm_reg_file_check_info));

	if (info->id < ARRAY_SIZE(id)) {	//Register file identifier
		fm_info->id = id[info->id];
	} else {
		fm_info->id = "reserved";
	}
	
	if (info->op < ARRAY_SIZE(op)) {	//Type of register Operation
		fm_info->op = op[info->op];
	} else {
		fm_info->op = "processor specific";
	}
	
	fm_info->reg_num = info->reg_num;	//Register number
	
	if (info->mcc) {
		fm_info->mcc = info->mcc;	//MC: Corrected
		faultname = "reg_file";
	} else 
		faultname = "reg_file_uc";

	fm_info->fm_cpuid_info = cpuid;
	fm_info->fm_sal_log_timestamp = time;

	(void) snprintf(fullclass, sizeof(fullclass), "%s.cpu.intel.%s",
	        FM_EREPORT_CLASS, faultname);

	cpumem_fm_event_post(cmp->reg_file_err, fullclass, cpuid->processor_id, -1);
	printf("cpumem_fm_event_post(fm_info, REG_FILE_CHECK);\n");
}

/* Check the machine check information related to ms error(s).
 * For some reason, SAL calls this 'ms' (micro structure?), PAL calls it 'uarch'
 * (micro-architectural).
 */
static void
ms_check_info_check (cpumem_monitor_t *cmp, int i, 
		sal_log_mod_error_info_t *ms_check_info, 
		fm_cpuid_info_t *cpuid, fm_sal_log_timestamp_t *time)
{
	char fullclass[PATH_MAX];
        char *faultname;
//      nvlist_t *nvl = cmp->ms_err;

	pal_mc_error_info_t check_info = { ms_check_info->check_info };
	pal_uarch_check_info_t *info = &check_info.pme_uarch;
//	static const char * const array_id[] = {
	static char * array_id[] = {
		"Unknown/unclassified",
	};
//	static const char * const op[] = {
	static char * op[] = {
		"Unknown",
		"Read/load",
		"Write/store",
	};
	/* SGI specific mappings of sid/op.
	 * FIXME: need some generic way to handle implementation specific
	 * definitions.
	 * sid*_op values start at index ARRAY_SIZE(op).
	 */
//	static const char * const sid0_op[] = {
	static char * sid0_op[] = {
		"Illegal ISA transfer",
	};
//	static const char * const sid1_op[] = {
	static char * sid1_op[] = {
		"Internal proc timer expired, BINIT signaled",
		"Internal proc timer reached half-way point, MCA signaled",
	};
//	static const char * const sid8_op[] = {
	static char * sid8_op[] = {
		"Over-temp detected, cpu power reduced",
		"Proc temp returned to normal, normal pwr restored",
	};
//	const char * const *sid_op;
	char **sid_op;
	int size;

	if (!ms_check_info->valid.check_info) {
//		iprintf("%s: invalid uarch_check_info[%d]\n", __FUNCTION__, i);
		return;                 /* If check info data not valid, skip it */
	}

	struct fm_ms_check_info *fm_info =
		(struct fm_ms_check_info*)malloc(sizeof(struct fm_ms_check_info));

	switch(info->sid) {
	case 0:
		sid_op = sid0_op;
		size = ARRAY_SIZE(sid0_op);
		break;
	case 1:
		sid_op = sid1_op;
		size = ARRAY_SIZE(sid1_op);
		break;
	case 8:
		sid_op = sid8_op;
		size = ARRAY_SIZE(sid8_op);
		break;
	default:
		sid_op = NULL;
		size = 0;
	}
	
	if (info->array_id < ARRAY_SIZE(array_id)) {	//Array identification
		fm_info->array_id = array_id[info->array_id];
	} else {
		fm_info->array_id = "implementation specific";
	}

	if (info->op < ARRAY_SIZE(op)) {	//Operation: %d (%s)
		fm_info->op = op[info->op];
	} else if (info->op - ARRAY_SIZE(op) < size) {
		fm_info->op = sid_op[info->op - ARRAY_SIZE(op)];
	} else {
		fm_info->op = "implementation specific";
	}

	fm_info->level = info->level;		//Level:

	if (info->wv)
		fm_info->way = info->way;	//Way of structure
	if (info->xv)
		fm_info->index = info->index;	//Index or set of the uarch structure that failed.
	
	if (info->mcc) {
 		fm_info->mcc = info->mcc;	//MC: Corrected
		faultname = "ms";
	} else
		faultname = "ms_uc";

	fm_info->fm_cpuid_info = cpuid;
	fm_info->fm_sal_log_timestamp = time;

	(void) snprintf(fullclass, sizeof(fullclass), "%s.cpu.intel.l%d%s",
	        FM_EREPORT_CLASS, info->level, faultname);

	cpumem_fm_event_post(cmp->ms_err, fullclass, cpuid->processor_id, -1);
	printf("cpumem_fm_event_post(fm_info, MS_CHECK);\n");
}

/* Format and Check the platform memory device error record section data */
static void
mem_dev_err_info_check (cpumem_monitor_t *cmp, sal_log_mem_dev_err_info_t *mdei, 
		fm_sal_log_timestamp_t *time)
{
	char fullclass[PATH_MAX];
	char *faultname;
//	nvlist_t *nvl = cmp->mem_dev_err;

	struct fm_mem_dev_check_info *fm_info =
		(struct fm_mem_dev_check_info*)malloc(sizeof(struct fm_mem_dev_check_info));

	if (mdei->valid.error_status)
		fm_info->error_status = mdei->error_status;	//Error Status:
	if (mdei->valid.physical_addr)
		fm_info->physical_addr = mdei->physical_addr;	//Physical Address:
	if (mdei->valid.addr_mask)
		fm_info->addr_mask = mdei->addr_mask;		//Address Mask:
	if (mdei->valid.node)
		fm_info->node = mdei->node;			//Node:
	if (mdei->valid.card)
		fm_info->card = mdei->card;			//Card:
	if (mdei->valid.module)
		fm_info->module = mdei->module;			//Module:
	if (mdei->valid.bank)
		fm_info->bank = mdei->bank;			//Bank:
	if (mdei->valid.device)
		fm_info->device = mdei->device;			//Device:
	if (mdei->valid.row)
		fm_info->row = mdei->row;			//Row:
	if (mdei->valid.column)
		fm_info->column = mdei->column;			//Column:
	if (mdei->valid.bit_position)
		fm_info->bit_position = mdei->bit_position;	//Bit Position:
	if (mdei->valid.target_id)
		fm_info->target_id = mdei->target_id;		//Target Address:
	if (mdei->valid.requestor_id)
		fm_info->requestor_id = mdei->requestor_id;	//Requestor Address:
	if (mdei->valid.responder_id)
		fm_info->responder_id = mdei->responder_id;	//Responder Address:
	if (mdei->valid.bus_spec_data)
		fm_info->bus_spec_data = mdei->bus_spec_data;	//Bus Specific Data:

	if (mdei->valid.oem_id) {
		u8  *p_data = &(mdei->oem_id[0]);
		int i;

		for (i = 0; i < 16; i++, p_data++)
			fm_info->oem_id[i] = *p_data;		//OEM Memory Controller ID:
	}

	fm_info->fm_sal_log_timestamp = time;

	faultname = "mem_dev";

	(void) snprintf(fullclass, sizeof(fullclass), "%s.cpu.intel.%s",
	        FM_EREPORT_CLASS, faultname);

        cpumem_fm_event_post(cmp->mem_dev_err, fullclass, -1, fm_info->physical_addr);
	printf("cpumem_fm_event_post(fm_info, MEM_DEV_CHECK);\n");
}

/* Format and Check the platform SEL device error record section data */
static void
sel_dev_err_info_check (cpumem_monitor_t *cmp, sal_log_sel_dev_err_info_t *sdei, 
		fm_sal_log_timestamp_t *time)
{
	char fullclass[PATH_MAX];
        char *faultname;
//      nvlist_t *nvl = cmp->sel_dev_err;

	int     i;
	struct fm_sel_dev_check_info *fm_info =
		(struct fm_sel_dev_check_info*)malloc(sizeof(struct fm_sel_dev_check_info));

	if (sdei->valid.record_id)
		fm_info->record_id = sdei->record_id;		//Record ID:
	if (sdei->valid.record_type)
		fm_info->record_type = sdei->record_type;	//Record Type:
	for (i = 0; i < 4; i++)
		fm_info->timestamp[i] = sdei->timestamp[i];	//Time Stamp:
	if (sdei->valid.generator_id)
		fm_info->generator_id = sdei->generator_id;	//Generator ID:
	if (sdei->valid.evm_rev)
		fm_info->evm_rev = sdei->evm_rev;		//Message Format Version:
	if (sdei->valid.sensor_type)
		fm_info->sensor_type = sdei->sensor_type;	//Sensor Type:
	if (sdei->valid.sensor_num)
		fm_info->sensor_num = sdei->sensor_num;		//Sensor Number:
	if (sdei->valid.event_dir)
		fm_info->event_dir = sdei->event_dir;		//Event Direction Type:
	if (sdei->valid.event_data1)
		fm_info->event_data1 = sdei->event_data1;	//Data1:
	if (sdei->valid.event_data2)
		fm_info->event_data2 = sdei->event_data2;	//Data2:
	if (sdei->valid.event_data3)
		fm_info->event_data3 = sdei->event_data3;	//Data3:

	fm_info->fm_sal_log_timestamp = time;

	faultname = "sel_dev";

        (void) snprintf(fullclass, sizeof(fullclass), "%s.cpu.intel.%s",
	        FM_EREPORT_CLASS, faultname);

        cpumem_fm_event_post(cmp->sel_dev_err, fullclass, -1, -1);
	printf("cpumem_fm_event_post(fm_info, SEL_DEV_CHECK);\n");
}

/* Format and Check the platform PCI bus error record section data */
static void
pci_bus_err_info_check (cpumem_monitor_t *cmp, sal_log_pci_bus_err_info_t *pbei, 
		fm_sal_log_timestamp_t *time)
{
	char fullclass[PATH_MAX];
        char *faultname;
//      nvlist_t *nvl = cmp->pci_bus_err;

	struct fm_pci_bus_check_info *fm_info =
		(struct fm_pci_bus_check_info*)malloc(sizeof(struct fm_pci_bus_check_info));

	if (pbei->valid.err_status)
		fm_info->err_status = pbei->err_status;		//Error Status:
	if (pbei->valid.err_type)
		fm_info->err_type = pbei->err_type;		//Error Type:
	if (pbei->valid.bus_id)
		fm_info->bus_id = pbei->bus_id;			//Bus ID:
	if (pbei->valid.bus_address)
		fm_info->bus_address = pbei->bus_address;	//Bus Address:
	if (pbei->valid.bus_data)
		fm_info->bus_data = pbei->bus_data;		//Bus Data:
	if (pbei->valid.bus_cmd)
		fm_info->bus_cmd = pbei->bus_cmd;		//Bus Command:
	if (pbei->valid.requestor_id)
		fm_info->requestor_id = pbei->requestor_id;	//Requestor ID:
	if (pbei->valid.responder_id)
		fm_info->responder_id = pbei->responder_id;	//Responder ID:
	if (pbei->valid.target_id)
		fm_info->target_id = pbei->target_id;		//Target ID:

	fm_info->fm_sal_log_timestamp = time;

	faultname = "pci_bus";

	(void) snprintf(fullclass, sizeof(fullclass), "%s.cpu.intel.%s",
	        FM_EREPORT_CLASS, faultname);

	cpumem_fm_event_post(cmp->pci_bus_err, fullclass, -1, -1);
	printf("cpumem_fm_event_post(fm_info, PCI_BUS_CHECK);\n");
}

/* Format and Check the platform SMBIOS device error record section data */
static void
smbios_dev_err_info_check (cpumem_monitor_t *cmp, 
		sal_log_smbios_dev_err_info_t *sdei, fm_sal_log_timestamp_t *time)
{
	char fullclass[PATH_MAX];
        char *faultname;
//      nvlist_t *nvl = cmp->smbios_dev_err;

	u8      i;
	struct fm_smbios_dev_check_info *fm_info =
		(struct fm_smbios_dev_check_info*)malloc(sizeof(struct fm_smbios_dev_check_info));

	if (sdei->valid.event_type)
		fm_info->event_type = sdei->event_type;			//Event Type:
	if (sdei->valid.time_stamp) {
		for (i = 0; i < 6; i++)
			fm_info->time_stamp[i] = sdei->time_stamp[i];	//Time Stamp:
	}
	if ((sdei->valid.data) && (sdei->valid.length)) {
		for (i = 0; i < sdei->length; i++)
			fm_info->data[i] = sdei->data[i];		//Data:
	}

	fm_info->fm_sal_log_timestamp = time;

	faultname = "smbios_dev";

        (void) snprintf(fullclass, sizeof(fullclass), "%s.cpu.intel.%s",
	        FM_EREPORT_CLASS, faultname);

	cpumem_fm_event_post(cmp->smbios_dev_err, fullclass, -1, -1);
	printf("cpumem_fm_event_post(fm_info, SMBIOS_DEV_CHECK);\n");
}

/* Format and Check the platform PCI component error record section data */
static void
pci_comp_err_info_check (cpumem_monitor_t *cmp, sal_log_pci_comp_err_info_t *pcei, 
		fm_sal_log_timestamp_t *time)
{
	char fullclass[PATH_MAX];
        char *faultname;
//      nvlist_t *nvl = cmp->pci_comp_err;

	struct fm_pci_comp_check_info *fm_info =
		(struct fm_pci_comp_check_info*)malloc(sizeof(struct fm_pci_comp_check_info));

	if (pcei->valid.err_status)
		fm_info->err_status = pcei->err_status;		//Error Status:
	if (pcei->valid.comp_info) {
		fm_info->vendor_id = pcei->comp_info.vendor_id;	//"Component Info: Vendor Id = 
		fm_info->device_id = pcei->comp_info.device_id;	//Device Id =
		fm_info->class_code[0] = pcei->comp_info.class_code[0];	//Class Code = 0x%02x%02x%02x,
		fm_info->class_code[1] = pcei->comp_info.class_code[1];
		fm_info->class_code[2] = pcei->comp_info.class_code[2];
		fm_info->seg_num = pcei->comp_info.seg_num;	//Seg/Bus/Dev/Func = %d/%d/%d/%d\n",
		fm_info->bus_num = pcei->comp_info.bus_num;
		fm_info->dev_num = pcei->comp_info.dev_num;
		fm_info->func_num = pcei->comp_info.func_num;
	}

	fm_info->fm_sal_log_timestamp = time;

	faultname = "pci_comp";

        (void) snprintf(fullclass, sizeof(fullclass), "%s.cpu.intel.%s",
	        FM_EREPORT_CLASS, faultname);

        cpumem_fm_event_post(cmp->pci_comp_err, fullclass, -1, -1);
	printf("cpumem_fm_event_post(fm_info, PCI_COMP_CHECK);\n");
}

/* Format and Check the platform specific error record section data */
static void
plat_specific_err_info_check (cpumem_monitor_t *cmp, 
		sal_log_plat_specific_err_info_t *psei, fm_sal_log_timestamp_t *time)
{
	char fullclass[PATH_MAX];
	char *faultname;
//	nvlist_t *nvl = cmp->plat_specific_err;

	struct fm_plat_specific_check_info *fm_info =
		(struct fm_plat_specific_check_info*)malloc(sizeof(struct fm_plat_specific_check_info));

	if (psei->valid.err_status)
		fm_info->err_status = psei->err_status;		//Error Status:
	if (psei->valid.guid)
		fm_info->guid = psei->guid;

	fm_info->fm_sal_log_timestamp = time;

	faultname = "plat_specific";

        (void) snprintf(fullclass, sizeof(fullclass), "%s.cpu.intel.%s",
	        FM_EREPORT_CLASS, faultname);

        cpumem_fm_event_post(cmp->plat_specific_err, fullclass, -1, -1);
	printf("cpumem_fm_event_post(fm_info, PLAT_SPECIFIC_CHECK);\n");
}

/* Format and Check the platform host controller error record section data */
static void
host_ctlr_err_info_check (cpumem_monitor_t *cmp, 
		sal_log_host_ctlr_err_info_t *hcei, fm_sal_log_timestamp_t *time)
{
	char fullclass[PATH_MAX];
        char *faultname;
//      nvlist_t *nvl = cmp->host_ctlr_err;

	struct fm_host_ctlr_check_info *fm_info =
		(struct fm_host_ctlr_check_info*)malloc(sizeof(struct fm_host_ctlr_check_info));

	if (hcei->valid.err_status)
		fm_info->err_status = hcei->err_status;		//Error Status:
	if (hcei->valid.requestor_id)
		fm_info->requestor_id = hcei->requestor_id;	//Requestor ID:
	if (hcei->valid.responder_id)
		fm_info->responder_id = hcei->responder_id;	//Responder ID:
	if (hcei->valid.target_id)
		fm_info->target_id = hcei->target_id;		//Target ID:
	if (hcei->valid.bus_spec_data)
		fm_info->bus_spec_data = hcei->bus_spec_data;	//Bus Specific Data:
	
	fm_info->fm_sal_log_timestamp = time;

	faultname = "host_ctlr";

        (void) snprintf(fullclass, sizeof(fullclass), "%s.cpu.intel.%s",
	        FM_EREPORT_CLASS, faultname);

        cpumem_fm_event_post(cmp->host_ctlr_err, fullclass, -1, -1);
	printf("cpumem_fm_event_post(fm_info, HOST_CTLR_CHECK);\n");
}

/* Format and Check the platform bus error record section data */
static void
plat_bus_err_info_check (cpumem_monitor_t *cmp, sal_log_plat_bus_err_info_t *pbei,
	        fm_sal_log_timestamp_t *time)
{
	char fullclass[PATH_MAX];
	char *faultname;
//	nvlist_t *nvl = cmp->plat_bus_err;

	struct fm_plat_bus_check_info *fm_info =
		(struct fm_plat_bus_check_info*)malloc(sizeof(struct fm_plat_bus_check_info));

	if (pbei->valid.err_status)
		fm_info->err_status = pbei->err_status;		//Error Status:
	if (pbei->valid.requestor_id)
		fm_info->requestor_id = pbei->requestor_id;	//Requestor ID:
	if (pbei->valid.responder_id)
		fm_info->responder_id = pbei->responder_id;	//Responder ID:
	if (pbei->valid.target_id)
		fm_info->target_id = pbei->target_id;		//Target ID:
	if (pbei->valid.bus_spec_data)
		fm_info->bus_spec_data = pbei->bus_spec_data;	//Bus Specific Data:

	fm_info->fm_sal_log_timestamp = time;

	faultname = "plat_bus";

	(void) snprintf(fullclass, sizeof(fullclass), "%s.cpu.intel.%s",
	        FM_EREPORT_CLASS, faultname);

	cpumem_fm_event_post(cmp->plat_bus_err, fullclass, -1, -1);
	printf("cpumem_fm_event_post(fm_info, PLAT_BUS_CHECK);\n");
}

/* Check the processor device error record. */
static void
proc_dev_err_info_check (cpumem_monitor_t *cmp, sal_log_processor_info_t *slpi, 
		fm_sal_log_timestamp_t *time)
{
	size_t  d_len = slpi->header.len - sizeof(sal_log_section_hdr_t);
	int                         i;
	sal_log_mod_error_info_t    *p_data;
	struct fm_cpuid_info *cpuid = 
		(struct fm_cpuid_info*)malloc(sizeof(struct fm_cpuid_info));

	struct error_map {
		u64 cid			: 4,
		    tid			: 4,
		    eic			: 4,
		    edc			: 4,
		    eit			: 4,
		    edt			: 4,
		    ebh			: 4,
		    erf			: 4,
		    ems			: 16,
		    rsvd		: 16;
	} erm;
	memcpy(&erm, &slpi->proc_error_map, sizeof(erm));

#if 0
	prval("processor error map", slpi->proc_error_map);
	++indent;
	iprintf("processor code id: %d\n", erm.cid);
	iprintf("logical thread id: %d\n", erm.tid);

	if (debug) {
		char    *p_data = (char *)&slpi->valid;
		iprintf("SAL_PROC_DEV_ERR SECTION DATA:  Data buffer = %p, "
		       "Data size = %ld\n", (void *)p_data, d_len);
		hexdump((unsigned char *)p_data, d_len);
		iprintf("End of SAL_PROC_DEV_ERR SECTION DATA\n");
	}

	proc_summary(slpi);

	++indent;
	if (slpi->valid.proc_cr_lid)
		proc_decode_lid(slpi);
	if (slpi->valid.proc_state_param)
		proc_decode_state_parameter(slpi);
	if (slpi->valid.proc_error_map)
		proc_decode_error_map(slpi);
	--indent;
#endif

	/*
	 *  Note: March 2001 SAL spec states that if the number of elements in any
	 *  of  the MOD_ERROR_INFO_STRUCT arrays is zero, the entire array is
	 *  absent. Also, current implementations only allocate space for number of
	 *  elements used.  So we walk the data pointer from here on.
	 */
	p_data = &slpi->info[0];

	cpuid->processor_id = slpi->proc_cr_lid;
	cpuid->core_id = erm.cid;
	cpuid->hyper_thread_id = erm.tid;
#if 0
#define check_info_debug(type) \
	if (debug && slpi->valid.num_ ## type ## _check) \
		iprintf("num_" # type "_check %d " # type "_check_info 0x%p\n", slpi->valid.num_ ## type ## _check, p_data);
#endif
	/* Check the cache_check information if any*/
//	check_info_debug(cache);
	for (i = 0 ; i < slpi->valid.num_cache_check; i++, p_data++)
		cache_check_info_check(cmp, i, p_data, cpuid, time);

	/* Check the tlb_check information if any*/
//	check_info_debug(tlb);
	for (i = 0 ; i < slpi->valid.num_tlb_check; i++, p_data++)
		tlb_check_info_check(cmp, i, p_data, cpuid, time);

	/* Check the bus_check information if any*/
//	check_info_debug(bus);
	for (i = 0 ; i < slpi->valid.num_bus_check; i++, p_data++)
		bus_check_info_check(cmp, i, p_data, cpuid, time);

	/* Check the reg_file_check information if any*/
//	check_info_debug(reg_file);
	for (i = 0 ; i < slpi->valid.num_reg_file_check; i++, p_data++)
		reg_file_check_info_check(cmp, i, p_data, cpuid, time);

	/* Check the ms_check information if any*/
//	check_info_debug(ms);
	for (i = 0 ; i < slpi->valid.num_ms_check; i++, p_data++)
		ms_check_info_check(cmp, i, p_data, cpuid, time);

}

#if 0
	/* Check CPUID registers if any*/
	if (slpi->valid.cpuid_info) {
		u64     *p = (u64 *)p_data;
//		if (debug)
//			iprintf("cpuid_info 0x%p\n", p_data);
		iprintf("CPUID Regs: %#lx %#lx %#lx %#lx\n", p[0], p[1], p[2], p[3]);
		p_data++;
	}

	/* Check processor static info if any */
	if (slpi->valid.psi_static_struct) {
		sal_processor_static_info_t *spsi = SAL_LPI_PSI_INFO(slpi);
		if (debug)
			iprintf("psi_static_struct start 0x%p end 0x%p\n", spsi, spsi+1);
		if (spsi->valid.minstate) {
			iprintf("\n");
			min_state_print(spsi);
		}
	
	}
#endif

/* Format and Check the SAL Platform Error Record. */
void
platform_info_check (cpumem_monitor_t *cmp, sal_log_record_header_t *lh)
{
	sal_log_section_hdr_t	*slsh;
	int			n_sects;
	int			ercd_pos;

	int iprintf_count = 0;
	int debug = 0;

//	if (debug > 1)
//		prt_record_header(lh);

	if ((ercd_pos = sizeof(sal_log_record_header_t)) >= lh->len) {
		printf("%s: truncated SAL error record. len = %d\n",
			       __FUNCTION__, lh->len);
		return;
	}

	/* Check record header info */
	struct fm_sal_log_timestamp *time =
		(struct fm_sal_log_timestamp*)malloc(sizeof(struct fm_sal_log_timestamp));

	time->slh_century = lh->timestamp.slh_century;
	time->slh_year = lh->timestamp.slh_year;
	time->slh_month = lh->timestamp.slh_month;
	time->slh_day = lh->timestamp.slh_day;
	time->slh_hour = lh->timestamp.slh_hour;
	time->slh_minute = lh->timestamp.slh_minute;
	time->slh_second = lh->timestamp.slh_second;

	for (n_sects = 0; (ercd_pos < lh->len); n_sects++, ercd_pos += slsh->len) {
		/* point to next section header */
		slsh = (sal_log_section_hdr_t *)((char *)lh + ercd_pos);
		if (slsh->len < sizeof(*slsh) || slsh->len > 1000000 /* arbitrary */) {
			printf("\n\nError: section header length (%d) is out of range\n\n", slsh->len);
//			prt_record_header(lh);
//			prt_section_header(slsh);
			return;
		}

//		if (debug > 1) {
//			prt_section_header(slsh);
//			if (efi_guidcmp(slsh->guid, SAL_PROC_DEV_ERR_SECT_GUID) != 0) {
//				size_t  d_len = slsh->len - sizeof(sal_log_section_hdr_t);
//				char    *p_data = (char *)&((sal_log_mem_dev_err_info_t *)slsh)->valid;
//				iprintf("Start of Platform Err Data Section:  Data buffer = %p, "
//				       "Data size = %ld\n", (void *)p_data, d_len);
//				hexdump((unsigned char *)p_data, d_len);
//				iprintf("End of Platform Err Data Section\n");
//			}
//		}

		/*
		 *  Now process the various record sections
		 */
		if (efi_guidcmp(slsh->guid, SAL_PROC_DEV_ERR_SECT_GUID) == 0) {
//			iprintf("Processor Device Error Info Section\n");
//			++indent;
			proc_dev_err_info_check(cmp, (sal_log_processor_info_t *)slsh, time);
//			--indent;
		} else if (efi_guidcmp(slsh->guid, SAL_PLAT_MEM_DEV_ERR_SECT_GUID) == 0) {
//			++indent;
//			iprintf("Platform Memory Device Error Info Section\n");
			mem_dev_err_info_check(cmp, (sal_log_mem_dev_err_info_t *)slsh, time);
//			--indent;
		} else if (efi_guidcmp(slsh->guid, SAL_PLAT_SEL_DEV_ERR_SECT_GUID) == 0) {
//			++indent;
//			iprintf("Platform SEL Device Error Info Section\n");
			sel_dev_err_info_check(cmp, (sal_log_sel_dev_err_info_t *)slsh, time);
//			--indent;
		} else if (efi_guidcmp(slsh->guid, SAL_PLAT_PCI_BUS_ERR_SECT_GUID) == 0) {
//			iprintf("Platform PCI Bus Error Info Section\n");
//			++indent;
			pci_bus_err_info_check(cmp, (sal_log_pci_bus_err_info_t *)slsh, time);
//			--indent;
		} else if (efi_guidcmp(slsh->guid, SAL_PLAT_SMBIOS_DEV_ERR_SECT_GUID) == 0) {
//			iprintf("Platform SMBIOS Device Error Info Section\n");
//			++indent;
			smbios_dev_err_info_check(cmp, (sal_log_smbios_dev_err_info_t *)slsh, time);
//			--indent;
		} else if (efi_guidcmp(slsh->guid, SAL_PLAT_PCI_COMP_ERR_SECT_GUID) == 0) {
//			iprintf("Platform PCI Component Error Info Section\n");
//			++indent;
			pci_comp_err_info_check(cmp, (sal_log_pci_comp_err_info_t *)slsh, time);
//			--indent;
		} else if (efi_guidcmp(slsh->guid, SAL_PLAT_SPECIFIC_ERR_SECT_GUID) == 0) {
//			iprintf("Platform Specific Error Info Section\n");
//			++indent;
			plat_specific_err_info_check(cmp, (sal_log_plat_specific_err_info_t *)slsh, time);
//			--indent;
		} else if (efi_guidcmp(slsh->guid, SAL_PLAT_HOST_CTLR_ERR_SECT_GUID) == 0) {
//			iprintf("Platform Host Controller Error Info Section\n");
//			++indent;
			host_ctlr_err_info_check(cmp, (sal_log_host_ctlr_err_info_t *)slsh, time);
//			--indent;
		} else if (efi_guidcmp(slsh->guid, SAL_PLAT_BUS_ERR_SECT_GUID) == 0) {
//			iprintf("Platform Bus Error Info Section\n");
//			++indent;
			plat_bus_err_info_check(cmp, (sal_log_plat_bus_err_info_t *)slsh, time);
//			--indent;
		} else {
			char out[40];
			printf("%s: unsupported record section %s\n", __FUNCTION__,
				   efi_guid_unparse(&slsh->guid, out));
			continue;
		}
	}

	if (debug > 1)
		printf("%s: found %d sections in SAL error record. len = %d\n",
			       __FUNCTION__, n_sects, lh->len);
	if (!n_sects)
		printf("No Platform Error Info Sections found\n");
	return;
}

