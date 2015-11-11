/*
 * Copyright (C) inspur Inc.  <http://www.inspur.com>
 * FileName:	p4.c
 * Author:		Inspur OS Team
 *				changxch@inspur.com
 * Date:		2015-08-04
 * Description:	
 *				decode mce for P4/Xeon and Core2 family. 
 *
 */

#define _GNU_SOURCE 1
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "cpumem_evtsrc.h"
#include "cpumem.h"
#include "p4.h"
#include "intel.h"
#include "bitfield.h"
#include "haswell.h"

/* decode mce for P4/Xeon and Core2 family */

static void decode_memory_controller(u32 mca, u64 status, struct mc_msg *mm);

static char * 
get_TT_str(__u8 t)
{
	static char* TT[] = {"Instruction", "Data", "Generic", "Unknown"};
	if (t >= NELE(TT)) {
		return "UNKNOWN";
	}
	
	return TT[t];
}

static char * __unused
fms_get_TT_str(__u8 t)
{
	static char* TT[] = {"i", "d", "g", "u"};
	if (t >= NELE(TT)) {
		return "u";		/* UNKNOWN */
	}
	
	return TT[t];
}

static char * 
get_LL_str(__u8 ll)
{
	static char* LL[] = {"Level-0", "Level-1", "Level-2", "Level-3"};
	if (ll > NELE(LL)) {
		return "UNKNOWN";
	}

	return LL[ll];
}

static char * __unused
fms_get_LL_str(__u8 ll)
{
	static char* LL[] = {"l0", "l1", "l2", "l3"};
	if (ll > NELE(LL)) {
		return "u";		/* UNKNOWN */
	}

	return LL[ll];
}


static char * 
get_RRRR_str(__u8 rrrr)
{
	static struct {
		__u8 value;
		char* str;
	} RRRR [] = {
		{0, "Generic"}, {1, "Read"},
		{2, "Write" }, {3, "Data-Read"},
		{4, "Data-Write"}, {5, "Instruction-Fetch"},
		{6, "Prefetch"}, {7, "Eviction"},
		{8, "Snoop"}
	};
	unsigned i;

	for (i = 0; i < (int)NELE(RRRR); i++) {
		if (RRRR[i].value == rrrr) {
			return RRRR[i].str;
		}
	}

	return "UNKNOWN";
}

static char * __unused
fms_get_RRRR_str(__u8 rrrr)
{
	static struct {
		__u8 value;
		char* str;
	} RRRR [] = {
		{0, "generic"}, {1, "rd"},
		{2, "wr" }, {3, "drd"},
		{4, "dwr"}, {5, "ird"},
		{6, "prefetch"}, {7, "eviction"},
		{8, "snoop"}
	};
	unsigned i;

	for (i = 0; i < (int)NELE(RRRR); i++) {
		if (RRRR[i].value == rrrr) {
			return RRRR[i].str;
		}
	}

	return "ukn";	/* UNKNOWN */
}

static char * 
get_PP_str(__u8 pp)
{
	static char* PP[] = {
		"Local-CPU-originated-request",
		"Responed-to-request",
		"Observed-error-as-third-party",
		"Generic"
	};
	if (pp >= NELE(PP)) {
		return "UNKNOWN";
	}

	return PP[pp];
}

static char * 
get_T_str(__u8 t)
{
	static char* T[] = {"Request-did-not-timeout", "Request-timed-out"};
	if (t >= NELE(T)) {
		return "UNKNOWN";
	}

	return T[t];
}

static char * 
get_II_str(__u8 i)
{
	static char* II[] = {"Memory-access", "Reserved", "IO", "Other-transaction"};

	if (i >= NELE(II)) {
		return "UNKNOWN";
	}

	return II[i];
}

static void
fms_decode_base_mca(int no, struct mc_msg *mm, u64 status)
{
	static char *msg[] = {
		[0] = "none",
		[1] = "unclassified",
		[2] = "mrom_parity",
		[3] = "external",
		[4] = "frc",
		[5] = "internal_parity",
		[6] = "smm_hcav",
	};

	if(no >= NELE(msg))
		return ;
	
	asprintf(&mm->mnemonic, "%s%s", msg[no], FMS_MCI_STATUS_UC(status));
	strcpy(mm->name, "cpu");
}

static int 
decode_mca(u64 status, u64 misc, int cpu, int socket, 
		struct mc_msg *mm, int *mm_num)
{
#define TLB_LL_MASK      0x3  /*bit 0, bit 1*/
#define TLB_LL_SHIFT     0x0
#define TLB_TT_MASK      0xc  /*bit 2, bit 3*/
#define TLB_TT_SHIFT     0x2 

#define CACHE_LL_MASK    0x3  /*bit 0, bit 1*/
#define CACHE_LL_SHIFT   0x0
#define CACHE_TT_MASK    0xc  /*bit 2, bit 3*/
#define CACHE_TT_SHIFT   0x2
#define CACHE_RRRR_MASK  0xF0 /*bit 4, bit 5, bit 6, bit 7 */
#define CACHE_RRRR_SHIFT 0x4

#define BUS_LL_MASK      0x3  /* bit 0, bit 1*/
#define BUS_LL_SHIFT     0x0
#define BUS_II_MASK      0xc  /*bit 2, bit 3*/
#define BUS_II_SHIFT     0x2
#define BUS_RRRR_MASK    0xF0 /*bit 4, bit 5, bit 6, bit 7 */
#define BUS_RRRR_SHIFT   0x4
#define BUS_T_MASK       0x100 /*bit 8*/
#define BUS_T_SHIFT      0x8   
#define BUS_PP_MASK      0x600 /*bit 9, bit 10*/
#define BUS_PP_SHIFT     0x9

	u32 mca;
	static char *msg[] = {
		[0] = "No Error",
		[1] = "Unclassified",
		[2] = "Microcode ROM parity error",
		[3] = "External error",
		[4] = "FRC error",
		[5] = "Internal parity error",
		[6] = "SMM Handler Code Access Violation",
	};
	int mn = *mm_num;

	mca = status & 0xffff;
	if (mca & (1UL << 12)) {
		wr_log(CMES_LOG_DOMAIN, WR_LOG_DEBUG,
			"corrected filtering (some unreported errors in same region)");
		mca &= ~(1UL << 12);
	}

	if(!mca) {
		wr_log(CMES_LOG_DOMAIN, WR_LOG_DEBUG, 
			"none general error");
		return -1;
	}

	if(*mm_num >= MCE_MSG_MAXNUM) {
		wr_log(CMES_LOG_DOMAIN, WR_LOG_ERROR,
			"MCE msg num[%d] is too small", MCE_MSG_MAXNUM);
		return -1;
	}
	
	if (mca < NELE(msg)) {
		asprintf(&mm[mn].desc, "%s", msg[mca]);
		fms_decode_base_mca(mca, &mm[mn], status);
		*mm_num = ++mn;
		return 0;
	}

	if ((mca >> 2) == 3) { 
		unsigned levelnum;
		char *level;
		
		levelnum = mca & 3;
		level = get_LL_str(levelnum);
		
//		asprintf(&mm[mn].mnemonic, "%sgcache_hierarchy%s", fms_get_LL_str(levelnum), 
//			FMS_MCI_STATUS_UC(status));
		asprintf(&mm[mn].mnemonic, "cache%s", FMS_MCI_STATUS_UC(status));
		asprintf(&mm[mn].desc, "%s Generic cache hierarchy error", level);
		mm[mn].clevel = levelnum;
		mm[mn].ctype = -1;
		strcpy(mm[mn].name, "cpu");
		mm[mn].id = (mca & 3) | 0xC;  /* cache type: unknown {1100}=0xC*/
	} else if (test_prefix(4, mca)) {
		unsigned levelnum, typenum;
		char *level, *type;
		
		typenum = (mca & TLB_TT_MASK) >> TLB_TT_SHIFT;
		type = get_TT_str(typenum);
		levelnum = (mca & TLB_LL_MASK) >> TLB_LL_SHIFT;
		level = get_LL_str(levelnum);
		
		asprintf(&mm[mn].desc, "%s TLB %s Error", type, level);
//		asprintf(&mm[mn].mnemonic, "%s%stlb%s", fms_get_LL_str(levelnum), 
//			fms_get_TT_str(typenum), FMS_MCI_STATUS_UC(status));
		asprintf(&mm[mn].mnemonic, "tlb%s", FMS_MCI_STATUS_UC(status));
		mm[mn].clevel = levelnum;
		mm[mn].ctype = typenum;
		strcpy(mm[mn].name, "cpu");
		mm[mn].id = mca & (TLB_TT_MASK | TLB_LL_MASK);
	} else if (test_prefix(8, mca)) {
		unsigned typenum = (mca & CACHE_TT_MASK) >> CACHE_TT_SHIFT;
		unsigned levelnum = (mca & CACHE_LL_MASK) >> CACHE_LL_SHIFT;
		unsigned rrrrnum = (mca & CACHE_RRRR_MASK) >> CACHE_RRRR_SHIFT;
		char *type = get_TT_str(typenum);
		char *level = get_LL_str(levelnum);
		char *rrrr = get_RRRR_str(rrrrnum);
		
		asprintf(&mm[mn].desc, "%s CACHE %s %s Error", type, level, rrrr);
//		asprintf(&mm[mn].mnemonic, "%s%scache_%s%s", fms_get_LL_str(levelnum), 
//			fms_get_TT_str(typenum), fms_get_RRRR_str(rrrrnum), FMS_MCI_STATUS_UC(status));
		asprintf(&mm[mn].mnemonic, "cache%s", FMS_MCI_STATUS_UC(status));
		mm[mn].clevel = levelnum;
		mm[mn].ctype = typenum;
		strcpy(mm[mn].name, "cpu");
		mm[mn].id = mca & (CACHE_TT_MASK | CACHE_LL_MASK);
	} else if (test_prefix(10, mca)) {
		if (mca == 0x400) {
			asprintf(&mm[mn].desc, "Internal Timer error");
			asprintf(&mm[mn].mnemonic, "internal_timer%s", FMS_MCI_STATUS_UC(status));
			strcpy(mm[mn].name, "cpu");
		} else {
			asprintf(&mm[mn].desc, "Internal unclassified error: %x", mca & 0xffff);
			asprintf(&mm[mn].mnemonic, "internal_unclassified%s", FMS_MCI_STATUS_UC(status));
			strcpy(mm[mn].name, "cpu");
		}	
	} else if (test_prefix(11, mca)) {
		char *level, *pp, *rrrr, *ii, *timeout;

		level = get_LL_str((mca & BUS_LL_MASK) >> BUS_LL_SHIFT);
		pp = get_PP_str((mca & BUS_PP_MASK) >> BUS_PP_SHIFT);
		rrrr = get_RRRR_str((mca & BUS_RRRR_MASK) >> BUS_RRRR_SHIFT);
		ii = get_II_str((mca & BUS_II_MASK) >> BUS_II_SHIFT);
		timeout = get_T_str((mca & BUS_T_MASK) >> BUS_T_SHIFT);

		
		asprintf(&mm[mn].desc, "BUS error: %d %d %s %s %s %s %s", socket, cpu,
			level, pp, rrrr, ii, timeout);
//		asprintf(&mm[mn].mnemonic, "%sbus_%s%s", 
//			fms_get_LL_str((mca & BUS_LL_MASK) >> BUS_LL_SHIFT), 
//			fms_get_RRRR_str((mca & BUS_RRRR_MASK) >> BUS_RRRR_SHIFT), 
//			FMS_MCI_STATUS_UC(status));
		asprintf(&mm[mn].mnemonic, "bus%s", FMS_MCI_STATUS_UC(status));
		strcpy(mm[mn].name, "cpu");
		mm[mn].id = mca & BUS_LL_MASK;
		
		/* IO MCA - reported as bus/interconnect with specific PP,T,RRRR,II,LL values
		 * and MISCV set. MISC register points to root port that reported the error
		 * need to cross check with AER logs for more details.
		 * See: http://www.intel.com/content/www/us/en/architecture-and-technology/enhanced-mca-logging-xeon-paper.html
		 */
		if ((status & MCI_STATUS_MISCV) &&
		    (status & 0xefff) == 0x0e0b) {
			int	seg, bus, dev, fn;
			char *tmp;

			seg = EXTRACT(misc, 32, 39);
			bus = EXTRACT(misc, 24, 31);
			dev = EXTRACT(misc, 19, 23);
			fn = EXTRACT(misc, 16, 18);
			
			asprintf(&tmp, "%s\nIO MCA reported by root port %x:%02x:%02x.%x",
				mm[mn].desc, seg, bus, dev, fn);
			xfree(mm[mn].desc);
			mm[mn].desc = tmp;
		}
	} else if (test_prefix(7, mca)) {
		decode_memory_controller(mca, status, &mm[mn]);
	} else {
		if(!mm) {
			asprintf(&mm[mn].desc,"Unknown Error %x", mca);
			asprintf(&mm[mn].mnemonic, "unknown%s", FMS_MCI_STATUS_UC(status));
			strcpy(mm[mn].name, "cpu");
		} else 
			--mm;
	}
	
	*mm_num = ++mn;
	return 0;
}

static int 
decode_mci(__u64 status, __u64 misc, int cpu, int socket, 
		struct mc_msg *mm, int *mm_num)
{
	return decode_mca(status, misc, cpu, socket, mm, mm_num);
}

static void 
decode_thermal(struct mce *log, int cpu, struct mc_msg *mm, int *mm_num)
{
	int mn = *mm_num;

	if(*mm_num >= MCE_MSG_MAXNUM) {
		wr_log(CMES_LOG_DOMAIN, WR_LOG_ERROR,
			"MCE msg num[%d] is too small", MCE_MSG_MAXNUM);
		return ;
	}

	strcpy(mm->name, "cpu");
	if (log->status & 1) {
		asprintf(&mm[mn].mnemonic, "tmp_high");
		asprintf(&mm[mn].desc, "Processor %d heated above trip temperature. Throttling enabled.", cpu);
		mm->id = 1;
	} else {
		asprintf(&mm[mn].mnemonic, "tmp_low");
		asprintf(&mm[mn].desc, "Processor %d below trip temperature. Throttling disabled.", cpu);
	} 

	*mm_num = mn+1;
}

void 
decode_intel_mc(struct mce *log, int cputype, unsigned size, 
		struct mc_msg *mm, int *mm_num)
{
	int socket = size > offsetof(struct mce, socketid) ? (int)log->socketid : -1;
	int cpu = log->extcpu ? log->extcpu : log->cpu;

	if (log->bank == MCE_THERMAL_BANK) { 
		decode_thermal(log, cpu, mm, mm_num);
		return ;
	}
		
	/* Model specific addon information */
	switch (cputype) {
	case CPU_HASWELL_EPEX:
		hsw_decode_model(cputype, log->bank, log->status, log->misc, mm, mm_num);
		break;
	//add other cpus
	}

	decode_mci(log->status, log->misc, cpu, socket, mm, mm_num);
}


/* Generic architectural memory controller encoding */
static char *mmm_mnemonic[] = { 
	"GEN", "RD", "WR", "AC", "MS", "RES5", "RES6", "RES7" 
};

static char *mmm_desc[] = { 
	"Generic undefined request",
	"Memory read error",
	"Memory write error",
	"Address/Command error",
	"Memory scrubbing error",
	"Reserved 5",
	"Reserved 6",
	"Reserved 7"
};

static char *fms_mmm_mnemonic[] __unused = { 
	"gen", "rd", "wr", "ac", "ms", "res5", "res6", "res7" 
};

static void 
decode_memory_controller(u32 mca, u64 status, struct mc_msg *mm)
{
	char channel[30];
	__u8 ch;
	
	if ((mca & 0xf) == 0xf) {
		strcpy(channel, "unspecified"); 
		ch = -1; 
	} else {
		sprintf(channel, "%u", mca & 0xf);
		ch = mca & 0xf;
	}
	
	asprintf(&mm->desc, "MEMORY CONTROLLER %s_CHANNEL%s_ERR\nTransaction: %s", 
		mmm_mnemonic[(mca >> 4) & 7],
		channel,
		mmm_desc[(mca >> 4) & 7]);
//	asprintf(&mm->mnemonic, "memctrl_%s_channel%s", 
//		fms_mmm_mnemonic[(mca >> 4) & 7],
//		FMS_MCI_STATUS_UC(status));
	
	/* memory page error */
	if ((status & MCI_STATUS_ADDRV) && !(status & MCI_STATUS_UC)) {
		mm->flags |= FMS_CPUMEM_PAGE_ERROR;
		asprintf(&mm->mnemonic, "mempage");
		strcpy(mm->name, "memory");
	} else {
		asprintf(&mm->mnemonic, "mem%s", FMS_MCI_STATUS_UC(status));
		strcpy(mm->name, "memory");
	}
	
	mm->mem_ch = ch;
}
