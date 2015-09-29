/*
 * Copyright (C) inspur Inc.  <http://www.inspur.com>
 * FileName:	cpumem_evtsrc.c
 * Author:		Inspur OS Team
 *				changxch@inspur.com
 * Date:		2015-08-04
 * Description:	
 *				cpumem error evtsrc module.
 *
 */
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <unistd.h>

#include "cmes_base.h"
#include "cpumem_evtsrc.h"
#include "logging.h"
#include "dmi.h"
#include "intel.h"
#include "p4.h"

#include "fmd_event.h"
#include "fmd_common.h"
#include "fmd_nvpair.h"
#include "evt_src.h"
#include "list.h"

static int cpumem_fm_event_post(struct list_head *head, nvlist_t *nvl,
		char *fullclass, struct fms_cpumem *cm);


enum cputype cputype = CPU_GENERIC;	

char *logfn = LOG_DEV_FILENAME; 
int mfd = -1;

#define PAGE_SHIFT 12
#define PAGE_SIZE (1UL << PAGE_SHIFT)

struct mcefd_data {
	unsigned loglen;
	unsigned recordlen;
	char *buf;
};
static struct mcefd_data md;

static char *cputype_name[] = {
	[CPU_GENERIC] = "generic CPU",
	[CPU_P6OLD] = "Intel PPro/P2/P3/old Xeon",
	[CPU_CORE2] = "Intel Core", /* 65nm and 45nm */
	[CPU_K8] = "AMD K8 and derivates",
	[CPU_P4] = "Intel P4",
	[CPU_NEHALEM] = "Intel Xeon 5500 series / Core i3/5/7 (\"Nehalem/Westmere\")",
	[CPU_DUNNINGTON] = "Intel Xeon 7400 series",
	[CPU_TULSA] = "Intel Xeon 7100 series",
	[CPU_INTEL] = "Intel generic architectural MCA",
	[CPU_XEON75XX] = "Intel Xeon 7500 series",
	[CPU_SANDY_BRIDGE] = "Sandy Bridge", /* Fill in better name */
	[CPU_SANDY_BRIDGE_EP] = "Sandy Bridge EP", /* Fill in better name */
	[CPU_IVY_BRIDGE] = "Ivy Bridge", /* Fill in better name */
	[CPU_IVY_BRIDGE_EPEX] = "Intel Xeon v2 (Ivy Bridge) EP/EX", /* Fill in better name */
	[CPU_HASWELL] = "Haswell", /* Fill in better name */
	[CPU_HASWELL_EPEX] = "Intel Xeon v3 (Haswell) EP/EX",
	[CPU_BROADWELL] = "Broadwell",
	[CPU_KNIGHTS_LANDING] = "Knights Landing",
	[CPU_ATOM] = "ATOM",
};

static enum cputype 
setup_cpuid(u32 cpuvendor, u32 cpuid)
{
	u32 family, model;

	parse_cpuid(cpuid, &family, &model);

	switch (cpuvendor) { 
	case X86_VENDOR_INTEL:
	        return select_intel_cputype(family, model);
	default:
		wr_log(CMES_LOG_DOMAIN, WR_LOG_WARNING,
			"Unknown CPU type vendor %u family %x model %x", 
			cpuvendor, family, model);
		return CPU_GENERIC;
	}
}

static void 
mce_cpuid(struct mce *m)
{	
	if (m->cpuid) {
		enum cputype t = setup_cpuid(m->cpuvendor, m->cpuid);
		cputype = t;
	}
}

static void 
mce_prepare(struct mce *m)
{
	mce_cpuid(m);
	if (!m->time)
		m->time = time(NULL);
}

static void 
resolveaddr(unsigned long addr, struct fms_cpumem *fc)
{
	if (addr && do_dmi)
		dmi_decodeaddr(addr, fc);
}

static void 
dump_mce(struct mce *m, unsigned recordlen, struct fms_cpumem **fc, int *fc_num) 
{
	int i;
	struct mc_msg mm[MCE_MSG_MAXNUM];
	int mm_num = 0;
	u64 track = 0;
	struct fms_cpumem *cm;
	u64 cpu = m->extcpu ? m->extcpu : m->cpu;
		
	/* should not happen */
	if (!m->finished)
		wr_log(CMES_LOG_DOMAIN, WR_LOG_WARNING,
			"MCE not finished?");

	memset(mm, 0, MCE_MSG_MAXNUM * sizeof mm[0]);
	switch (cputype) { 
	CASE_INTEL_CPUS:
		decode_intel_mc(m, cputype, recordlen, mm, &mm_num);
		break;
		/* add handlers for other CPUs here */
	default:
		break;
	}
	
	if(!mm_num) {
		wr_log(CMES_LOG_DOMAIN, WR_LOG_DEBUG, 
			"MCE msg is null");
		return ;
	}
		

	*fc = xcalloc(mm_num, sizeof(struct fms_cpumem));
	cm = *fc;
	for (i = 0; i < mm_num; i++) {
		cm[i].cpu = cpu;
		cm[i].status = m->status;
		cm[i].misc = m->misc;
		cm[i].addr = m->addr;
		cm[i].mcgstatus = m->mcgstatus;
		cm[i].ip = m->ip;
		cm[i].tsc = m->tsc;
		cm[i].time = m->time;
		cm[i].cs = m->cs;
		cm[i].bank = m->bank;
			
		if (recordlen >= offsetof(struct mce, cpuid) && m->mcgcap)
			cm[i].mcgcap = m->mcgcap;
		if (recordlen >= offsetof(struct mce, apicid))
			cm[i].apicid = m->apicid;
		if (recordlen >= offsetof(struct mce, socketid))
			cm[i].socketid = m->socketid;
		if (recordlen >= offsetof(struct mce, cpuid) && m->cpuid) {
			cm[i].cpuid = m->cpuid;
			cm[i].cpuvendor = m->cpuvendor;
		}

		if (m->status & MCI_STATUS_UC) 
			cm[i].flags |= FMS_CPUMEM_UC;
		
		/* Yellow bit cache threshold supported */
		if ((m->mcgcap == 0 || (m->mcgcap & MCG_TES_P)) && !(m->status & MCI_STATUS_UC)) {
			track = (m->status >> 53) & 3;
			if(track == 2)
				cm[i].flags |= FMS_CPUMEM_YELLOW;
		}
		
		resolveaddr(m->addr, &cm[i]);

		if(cm[i].flags & FMS_CPUMEM_PAGE_ERROR){
		    /* int ps = getpagesize();	 kernel PAGE_SHIFT 12 */	
			cm[i].maddr = m->addr & ~((u64)PAGE_SIZE-1);
			cm[i].fid = cm[i].maddr;
		} else {
			cm[i].fid = (mm[i].id | (cpu << 31));
		}
		strcpy(cm[i].fname, mm[i].name);
		
		strncpy(cm[i].mnemonic, mm[i].mnemonic, FMS_PATH_MAX - 1);
		xfree(mm[i].mnemonic);
		strncpy(cm[i].desc, mm[i].desc, FMS_CPUMEM_FAULT_DESC_LEN - 1);
		xfree(mm[i].desc);
		
		cm[i].mem_ch = mm[i].mem_ch;
		cm[i].clevel = mm[i].clevel;
		cm[i].ctype = mm[i].ctype;
		cm[i].cputype = cputype;
	}
	*fc_num = mm_num;
}

static struct list_head * 
process_mcefd()
{
	int len, count;
	int flags;
	int i, j;
	char fullclass[FMS_PATH_MAX];
	nvlist_t *nvl;
	struct list_head *head = nvlist_head_alloc();
	
	if (md.recordlen == 0) {
		wr_log(CMES_LOG_DOMAIN, WR_LOG_WARNING, "no data in mce record");
		return NULL;
	}

	len = read(mfd, md.buf, md.recordlen * md.loglen); 
	if (len < 0) {
		wr_log(CMES_LOG_DOMAIN, WR_LOG_ERROR,
			"Failed to read mcelog, %s", strerror(errno));
		return NULL;
	}
	if (len == 0) {
//		wr_log(CMES_LOG_DOMAIN, WR_LOG_DEBUG, "%s no data", LOG_DEV_FILENAME);
		return NULL;
	}
	
	count = len / (int)md.recordlen;
	if (count == (int)md.loglen) {
		if ((ioctl(mfd, MCE_GETCLEAR_FLAGS, &flags) == 0) &&
		    (flags & (1 << MCE_OVERFLOW)))
			wr_log(CMES_LOG_DOMAIN, WR_LOG_WARNING, "MCE buffer is overflowed.");
	}
	
	for (i = 0; i < count; i++) {
		struct mce *mce = (struct mce *)(md.buf + i*md.recordlen);
		struct fms_cpumem *fc = NULL;
		int fc_num = 0;

		mce_prepare(mce);

		dump_mce(mce, md.recordlen, &fc, &fc_num);
		wr_log(CMES_LOG_DOMAIN, WR_LOG_DEBUG,
			"fc_num: %d, status:%llx", fc_num, mce->status);
		
		for (j = 0; j < fc_num; j++) {
			struct fms_cpumem *cm = NULL;

			cm = xcalloc(1, sizeof *cm);
			memcpy(cm, &fc[j], sizeof *cm);

			if(!(cm->flags & FMS_CPUMEM_YELLOW))
				snprintf(fullclass, sizeof(fullclass),
					"%s.cpu.%s",
					FM_EREPORT_CLASS, cm->mnemonic);
			else
				snprintf(fullclass, sizeof(fullclass),
					"%s.cpu.%s",
					FM_FAULT_CLASS, cm->mnemonic);
			
			if (cm->memdev) {	/* debug */
				wr_log(CMES_LOG_DOMAIN, WR_LOG_DEBUG, 
					"memdev: handle %0x", cm->memdev->header.handle);
			} 
			
			nvl = nvlist_alloc();
			if (nvl == NULL) {
				wr_log(CMES_LOG_DOMAIN, WR_LOG_ERROR,
						"out of memory\n");
				break;
			}
			if (cpumem_fm_event_post(head, nvl, fullclass, cm) != 0 ) {
				nvlist_free(nvl);
				nvl = NULL;
			} 
		}

		if (fc) {
			xfree(fc);
			fc = NULL;
			fc_num = 0;
		}
	}

	if (md.recordlen > sizeof(struct mce)) {
		wr_log(CMES_LOG_DOMAIN, WR_LOG_WARNING,
				"%lu bytes ignored in each record, consider an update",
				(unsigned long)md.recordlen - sizeof(struct mce)); 
	}
	
	return head;
}

static int 
is_cpu_supported(void)
{ 
	enum { 
		VENDOR = 1, 
		FAMILY = 2, 
		MODEL = 4, 
		MHZ = 8, 
		FLAGS = 16, 
		ALL = 0x1f 
	} seen = 0;
	FILE *f;
	static int checked;

	if (checked)
		return 1;
	checked = 1;

	f = fopen("/proc/cpuinfo","r");
	if (f != NULL) { 
		int family = 0; 
		int model = 0;
		char vendor[64] = { 0 };
		char *line = NULL;
		size_t linelen = 0; 
		double mhz;

		while (getdelim(&line, &linelen, '\n', f) > 0 && seen != ALL) { 
			if (sscanf(line, "vendor_id : %63[^\n]", vendor) == 1) 
				seen |= VENDOR;
			if (sscanf(line, "cpu family : %d", &family) == 1)
				seen |= FAMILY;
			if (sscanf(line, "model : %d", &model) == 1)
				seen |= MODEL;
			/* We use only Mhz of the first CPU, assuming they are the same
			   (there are more sanity checks later to make this not as wrong
			   as it sounds) */
			if (sscanf(line, "cpu MHz : %lf", &mhz) == 1) { 
				seen |= MHZ;
			}
			if (!strncmp(line, "flags", 5) && isspace(line[6])) {
				seen |= FLAGS;
			}			      
		}
		
		if (seen == ALL) {
			if (!strcmp(vendor,"AuthenticAMD")) {
				if (family == 15) {
					cputype = CPU_K8;
				} else if (family >= 16) {
					wr_log(CMES_LOG_DOMAIN, WR_LOG_ERROR, 
						"AMD Processor family %d: does not support this processor.", family);
					return 0;
				}
			} else if (!strcmp(vendor,"GenuineIntel"))
				cputype = select_intel_cputype(family, model);
			/* Add checks for other CPUs here */	
		} else {
			wr_log(CMES_LOG_DOMAIN, WR_LOG_WARNING, "Cannot parse /proc/cpuinfo"); 
		} 
		fclose(f);
		free(line);
	} else
		wr_log(CMES_LOG_DOMAIN, WR_LOG_WARNING, "Cannot open /proc/cpuinfo");

	return 1;
} 

static int
mcedata_init()
{
	mfd = open(logfn, O_RDONLY); 
	if (mfd < 0) {
		wr_log(CMES_LOG_DOMAIN, WR_LOG_ERROR,
			"Cannot open `%s'", logfn);
		return -1;
	}
	
	if (ioctl(mfd, MCE_GET_RECORD_LEN, &md.recordlen) < 0) {
		wr_log(CMES_LOG_DOMAIN, WR_LOG_ERROR, 
			"Failed to mce get record len, %s", strerror(errno));
		close(mfd);
		return -1;
	}
	if (ioctl(mfd, MCE_GET_LOG_LEN, &md.loglen) < 0) {
		wr_log(CMES_LOG_DOMAIN, WR_LOG_ERROR, 
			"Failed to mce get log len, %s", strerror(errno));
		close(mfd);
		return -1;
	}
	
	md.buf = xcalloc(md.recordlen, md.loglen); 
	return 0;
}

static int
cpumem_init(void)
{
	if (!is_cpu_supported()) {
		wr_log(CMES_LOG_DOMAIN, WR_LOG_ERROR, "CPU is unsupported");
		return -1;
	}
	wr_log(CMES_LOG_DOMAIN, WR_LOG_DEBUG, 
		"CPU TYPE: %s", cputype_name[cputype]);

	checkdmi();
	if (!do_dmi)
		closedmi();

	if (mcedata_init()) {
		return -1;
	}

	return 0;
}	

static void
cpumem_release(void)
{
	if (do_dmi)
		closedmi();

	if(mfd != -1)
		close(mfd);
}

/*
 * Input
 *  head	- list of all event
 *  nvl		- current event
 *  fullclass
 *  ena
 *  data	- additional Information
 */
static int
cpumem_fm_event_post(struct list_head *head, nvlist_t *nvl,
		char *fullclass, struct fms_cpumem *cm)
{
	sprintf(nvl->name, cm->fname);
	strcpy(nvl->value, fullclass);
	nvl->dev_id = cm->fid;
	nvl->data = cm;
	
	nvlist_add_nvlist(head, nvl);

	wr_log(CMES_LOG_DOMAIN, WR_LOG_DEBUG, "post %s", fullclass);

	return 0;
}

static struct list_head *
cpumem_probe(evtsrc_module_t *emp)
{	
	struct list_head *head;

	head = process_mcefd();
	
	return head;
}

#ifndef TEST_CMES

static evtsrc_modops_t cpumem_mops = {
	.evt_probe = cpumem_probe,
};

fmd_module_t *
fmd_module_init(char *path, fmd_t *pfmd)
{
	if(cpumem_init())
		return NULL;
	
	return (fmd_module_t *)evtsrc_init(&cpumem_mops, path, pfmd);
}

void
fmd_module_finit(fmd_module_t *mp)
{
	cpumem_release();
}

#endif

/* 
 * for test cpumem event source.
 * make -f Makefile.test 
 */
#ifdef TEST_CMES

int
cmes_init_test()
{
	return cpumem_init();
}

struct list_head *
cmes_test()
{
	return process_mcefd();
}

void
cmes_release_test()
{
	cpumem_release();
}

#endif /* TEST_CMES */
