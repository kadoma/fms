/*
 * Copyright (C) inspur Inc.  <http://www.inspur.com>
 * FileName:	evt_process.c
 * Author:		Inspur OS Team
 *				changxch@inspur.com
 * Date:		2015-08-04
 * Description:	
 *				Machine Check Event Process. 
 *
 */

#include <stdint.h> 
#include <errno.h>
#include <regex.h>
#include <unistd.h>
#include <time.h>
#include <linux/kernel-page-flags.h>

#include "evt_process.h"
#include "cmea_base.h"
#include "libkfm_cpumem.h"
#include "cpumem.h"
#include "logging.h"
#include "memdev.h"
#include "sysfs.h"
#include "tsc.h"
#include "cache.h"

extern double cpumhz;

static int evt_handle_cache(struct cmea_evt_data *edata, void *data);
static int evt_handle_tlb(struct cmea_evt_data *edata, void *data);
static int evt_handle_bus(struct cmea_evt_data *edata, void *data);
static int evt_handle_pcu(struct cmea_evt_data *edata, void *data);
static int evt_handle_qpi(struct cmea_evt_data *edata, void *data);
static int evt_handle_mc(struct cmea_evt_data *edata, void *data);
static int evt_handle_mempage(struct cmea_evt_data *edata, void *data);


static struct evt_handler {
	char *evt_type;
	int (*handler)(struct cmea_evt_data *, void *);
} g_evt_handler[] = {
	{"cache", 		evt_handle_cache 	},
	{"tlb", 		evt_handle_tlb 		},
//	{"bus",         evt_handle_bus      },  //todo.....
//	{"pcu",         evt_handle_pcu      },  //todo.....
	{"qpi",         evt_handle_qpi      },
//	{"mc",          evt_handle_mc       },  //todo.....
	{"mempage", 	evt_handle_mempage 	},
	/* end */
	{NULL, 			NULL}
};

int
cmea_evt_processing(char *evt_type, struct cmea_evt_data *edata, void *data)
{
	int i;
	char *etype = NULL;

	for (i = 0; g_evt_handler[i].evt_type; i++) {
		etype = g_evt_handler[i].evt_type;
		if (strncmp(etype, evt_type, strlen(etype)) == 0) {
			return g_evt_handler[i].handler(edata, data);
		}
	}
	
	wr_log(CMEA_LOG_DOMAIN, WR_LOG_WARNING, 
		"event type[%s] is no handler", evt_type);
	return -1;
}

static int 
memory_page_offline(uint64_t addr)
{
	int ret;
	struct kfm_mempage_u mempage;
	
	/* if page already offline, return success. */
	memset(&mempage, 0, sizeof mempage);
	mempage.addr = addr;
	if (kfm_mempage_get_info(&mempage)) {
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_DEBUG, 
			"failted to get mempage[0x%llx], %s",
			addr,
			strerror(errno));
		return -1;
	}
	if (!(mempage.flags >> KPF_NOPAGE & 1) &&
		(mempage.flags >> KPF_HWPOISON & 1)) {
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_DEBUG, 
			"mempage[0x%llx] already offline", addr);
		return 0;
	}
	
	ret = sysfs_write("/sys/devices/system/memory/soft_offline_page", 
		"%#llx", (unsigned long long)addr);

	if(ret > 0)
		return 0;

	return -1;
}

static int
is_cpu_offline(int cpu)
{
	FILE *f;
	char *line = NULL;
	size_t linesz = 0;
	int u;

	f = fopen("/proc/cpuinfo", "r");
	if (!f) {
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_ERROR, 
			"failed to open /proc/cpuinfo, %s",
			strerror(errno));

		return 0;
	}

	while (getdelim(&line, &linesz, '\n', f) > 0) {
		if (sscanf(line, "processor : %u\n", &u) == 1) { 
			if (u == cpu) {
				xfree(line);
				fclose(f);
				return 0;
			}	
		}
	}
	
	xfree(line);
	fclose(f);
	return 1;
}

static int
do_cpu_offline(int cpu)
{
	char buf[256];
	int ret;

	if (is_cpu_offline(cpu)) {
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_DEBUG, 
			"CPU %d already is offine", cpu);
		return 0;
	}
	
	sprintf(buf, "/sys/devices/system/cpu/cpu%d/online", cpu);

	wr_log(CMEA_LOG_DOMAIN, WR_LOG_DEBUG, 
		"%s", buf);
	
	if (!sysfs_available(buf, F_OK)) {
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_WARNING, 
			"CPU %d is no allowed offline", cpu);
		return -1;
	}
	
	ret = sysfs_write(buf, "0"); /* 0: offline, 1: online */

	if(ret > 0)
		return 0;

	return -1;
}


#define BITS_PER_U (sizeof(unsigned) * 8)
#define test_bit(i, a) (((unsigned *)(a))[(i) / BITS_PER_U] & (1U << ((i) % BITS_PER_U)))

static int
cpu_offine_on_cache(int cpu, unsigned clevel, unsigned ctype)
{
	unsigned *cpumask;
	int cpumasklen;
	int i;
	int r, ret = 0;
		
	if (cache_to_cpus(cpu, clevel, ctype, &cpumasklen, &cpumask) == 0)
	{
		for (i = 0; i < cpumasklen * 8; i++) {
			if (test_bit(i, cpumask)) {
				r = do_cpu_offline(i);

				if(!r)
					wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL, 
						"cpu %d offline sucess", 
						i);
				else
					wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL, 
						"cpu %d offline fail, %s", 
						i, strerror(errno));

				if(r)
					ret = -1;
			}
		}
	} else { /* no find cache */
		ret = do_cpu_offline(cpu);

		if(!ret)
			wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL, 
				"cpu %d offline sucess", 
				cpu);
		else
			wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL, 
				"cpu %d offline fail, %s", 
				cpu, strerror(errno));
	}

	return ret;
}

static inline int
__handle_cache(struct cmea_evt_data *edata, void *data)
{
	int ret = 0;
	struct fms_cpumem *fc;
	struct kfm_cache_u cache;

	if(!edata || !data)
		return -1;
	
	fc = (struct fms_cpumem*)data;
	switch (edata->cpu_action) {
	case 1:
		/* flush cache */
		memset(&cache, 0, sizeof cache);
		cache.cpu = fc->cpu;
		ret = kfm_cache_flush(&cache);
		break;
	case 2:
		/* cpu offline */
		ret = cpu_offine_on_cache(fc->cpu, fc->clevel, fc->ctype);
		break;
	default:
		return -1;
	}
	
//	dump_evt(data);
	return ret;
}

static int 
evt_handle_cache(struct cmea_evt_data *edata, void *data)
{
	return __handle_cache(edata, data);
}

static int 
evt_handle_tlb(struct cmea_evt_data *edata, void *data)
{
	return __handle_cache(edata, data);
}

static int 
evt_handle_bus(struct cmea_evt_data *edata, void *data)
{
	struct fms_cpumem *fc;
	int cpu;
	
	if(!edata || !data)
		return -1;

	fc = (struct fms_cpumem*)data;
	cpu = fc->cpu;
	
	return do_cpu_offline(cpu);
}

static int 
evt_handle_pcu(struct cmea_evt_data *edata, void *data)
{
	struct fms_cpumem *fc;
	int cpu;
	
	if(!edata || !data)
		return -1;

	fc = (struct fms_cpumem*)data;
	cpu = fc->cpu;
	
	return do_cpu_offline(cpu);
}

static int 
evt_handle_qpi(struct cmea_evt_data *edata, void *data)
{
	struct fms_cpumem *fc;
	int cpu;
	
	if(!edata || !data)
		return -1;

	fc = (struct fms_cpumem*)data;
	cpu = fc->cpu;
	
	return do_cpu_offline(cpu);
}

static int 
evt_handle_mc(struct cmea_evt_data *edata, void *data)
{
	struct fms_cpumem *fc;
	int cpu;
	
	if(!edata || !data)
		return -1;

	fc = (struct fms_cpumem*)data;
	cpu = fc->cpu;
	
	return do_cpu_offline(cpu);
}

static int 
evt_handle_mempage(struct cmea_evt_data *edata, void *data)
{	
	int ret = 0;
	struct fms_cpumem *fc;

	if(!data)
		return -1;

	fc = (struct fms_cpumem*)data;

	if(fc->flags & FMS_CPUMEM_PAGE_ERROR) {
		ret = memory_page_offline(fc->addr);

		if(!ret)
			wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL, 
				"mempage %llx offline sucess", 
				fc->maddr);
		else
			wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL, 
				"mempage %llx offline fail, %s", 
				fc->maddr, strerror(errno));
	}
	
//	dump_evt(data);
	
	return ret;
}


/* process, for log */
static char *
extended_bankname(unsigned bank) 
{
	static char buf[64];
	
	switch (bank) { 
	case MCE_THERMAL_BANK:
		return "THERMAL EVENT";
	case MCE_TIMEOUT_BANK:
		return "Timeout waiting for exception on other CPUs";

		/* add more extended banks here */
	
	default:
		sprintf(buf, "Undecoded extended event %x", bank);
		return buf;
	} 
}

char *
intel_bank_name(int num)
{
	static char bname[64];
	sprintf(bname, "BANK %d", num);
	return bname;
}

static char *
bankname(unsigned bank, enum cputype cputype) 
{ 
	static char numeric[64] = {0};
	
	if (bank >= MCE_EXTENDED_BANK) 
		return extended_bankname(bank);

	switch (cputype) { 
	CASE_INTEL_CPUS:
		return intel_bank_name(bank);
	/* add banks of other cpu types here */
	default:
		sprintf(numeric, "BANK %d", bank); 
		return numeric;
	}

	return numeric;
} 

static void 
print_tsc(int cpunum, __u64 tsc, unsigned long time, 
        enum cputype cputype) 
{ 
	int ret = -1;
	char *buf = NULL;

	if (!time) 
		ret = decode_tsc_current(&buf, cpunum, cputype, cpumhz, tsc);
	
	wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL,
		"TSC %llx %s", tsc, ret >= 0 && buf ? buf : "");
	free(buf);
}

static void
print_mcg(__u64 mcgstatus)
{
	wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL, "MCG status:");
	if (mcgstatus & MCG_STATUS_RIPV)
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL, "RIPV ");
	if (mcgstatus & MCG_STATUS_EIPV)
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL, "EIPV ");
	if (mcgstatus & MCG_STATUS_MCIP)
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL, "MCIP ");
	if (mcgstatus & MCG_STATUS_LMCES)
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL, "LMCE ");
}

static const char *arstate[4] = { 
	[0] = "UCNA",		/* UnCorrected No Action reauired */
	[1] = "AR", 		/* Action Required  */
	[2] = "SRAO",		/* Software Recoverable Action Optional */
	[3] = "SRAR"		/* Software Recoverable Action Required */
};

static void
print_mci(__u64 status, unsigned mcgcap)
{
	wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL, "MCi status:");
	if (!(status & MCI_STATUS_VAL))
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL, 
			"Machine check not valid");

	if (status & MCI_STATUS_OVER)
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL, "Error overflow");
	
	if (status & MCI_STATUS_UC) 
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL, "Uncorrected error");
	else
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL, "Corrected error");

	if (status & MCI_STATUS_EN)
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL, "Error enabled");

	if (status & MCI_STATUS_MISCV) 
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL, "MCi_MISC register valid");

	if (status & MCI_STATUS_ADDRV)
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL, "MCi_ADDR register valid");

	if (status & MCI_STATUS_PCC)
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL, "Processor context corrupt");

	if (status & (MCI_STATUS_S|MCI_STATUS_AR))
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL, 
			"%s\n", 
			arstate[(status >> 55) & 3]);

	if ((mcgcap & MCG_SER_P) && (status & MCI_STATUS_FWST)) {
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL,
			"Firmware may have updated this error\n");
	}
}

static char *vendor[] = {
	[0] = "Intel",
	[1] = "Cyrix",
	[2] = "AMD",
	[3] = "UMC", 
	[4] = "vendor 4",
	[5] = "Centaur",
	[6] = "vendor 6",
	[7] = "Transmeta",
	[8] = "NSC"
};

static char *
cpuvendor_name(uint32_t cpuvendor)
{
	return (cpuvendor < NELE(vendor)) ? vendor[cpuvendor] : "Unknown vendor";
}

void
dump_evt(void *data)
{
	struct fms_cpumem *fc;

	if(!data)
		return ;

	fc = (struct fms_cpumem *)data;

	wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL, 
		"CPU %d %s", fc->cpu, bankname(fc->bank, fc->cputype));

	if (fc->tsc)
		print_tsc(fc->cpu, fc->tsc, fc->time, fc->cputype);

	if (fc->ip)
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL,
			"RIP%s %02x:%llx", 
			!(fc->mcgstatus & MCG_STATUS_EIPV) ? " !INEXACT!" : "",
			fc->cs, fc->ip);

	if (fc->status & MCI_STATUS_MISCV)
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL,
			"MISC %llx ", fc->misc);
	if (fc->status & MCI_STATUS_ADDRV)
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL,
			"ADDR %llx ", fc->addr);

	if (fc->time) {
		time_t t = fc->time;
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL,
			"TIME %llu %s", fc->time, ctime(&t));
	}

	if (fc->bank == MCE_THERMAL_BANK) { 
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL,
			"%s", fc->desc);
		return ;
	}
	
	print_mcg(fc->mcgstatus);
	print_mci(fc->status, fc->mcgcap);
	
	wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL, "MCA: ");
	wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL, "%s", fc->desc);

	/* decode all status bits here */
	wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL, 
		"STATUS %llx MCGSTATUS %llx\n", fc->status, fc->mcgstatus);

	if (fc->mcgcap)
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL, 
			"MCGCAP %llx ", fc->mcgcap);
	if (fc->apicid)
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL, 
			"APICID %x ", fc->apicid);
	if (fc->socketid)
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL, 
			"SOCKETID %x", fc->socketid);
	if (fc->cpuid) {
		uint32_t fam, mod;
		parse_cpuid(fc->cpuid, &fam, &mod);
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_NORMAL,
			"CPUID Vendor %s Family %u Model %u",
			cpuvendor_name(fc->cpuvendor), 
			fam,
			mod);
	}

	/* dump memory device, if any */
	if (fc->memdev) {
		dump_memdev(fc->memdev, fc->addr);
	}
}

#ifdef TEST_CMEA

int
test_cpu_offine_on_cache(int cpu, unsigned clevel, unsigned ctype)
{
	return cpu_offine_on_cache(cpu, clevel, ctype);
}

int
test_memory_page_offline(uint64_t addr)
{
	return memory_page_offline(addr);
}

#endif /* TEST_CMEA */